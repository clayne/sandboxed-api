// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sandboxed_api/sandbox2/stack_trace.h"

#include <sys/types.h>

#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/base/log_severity.h"
#include "absl/log/check.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "sandboxed_api/sandbox2/allowlists/all_syscalls.h"
#include "sandboxed_api/sandbox2/allowlists/namespaces.h"
#include "sandboxed_api/sandbox2/executor.h"
#include "sandboxed_api/sandbox2/global_forkclient.h"
#include "sandboxed_api/sandbox2/policy.h"
#include "sandboxed_api/sandbox2/policybuilder.h"
#include "sandboxed_api/sandbox2/result.h"
#include "sandboxed_api/sandbox2/sandbox2.h"
#include "sandboxed_api/testing.h"
#include "sandboxed_api/util/fileops.h"

namespace sandbox2 {

class StackTraceTestPeer {
 public:
  static StackTraceTestPeer& GetInstance() {
    static auto* peer = new StackTraceTestPeer();
    return *peer;
  }
  std::unique_ptr<internal::SandboxPeer> SpawnFn(
      std::unique_ptr<Executor> executor, std::unique_ptr<Policy> policy) {
    if (crash_unwind_) {
      policy = PolicyBuilder().CollectStacktracesOnSignal(false).BuildOrDie();
      crash_unwind_ = false;
    }
    return old_spawn_fn_(std::move(executor), std::move(policy));
  }
  void ReplaceSpawnFn() {
    old_spawn_fn_ = internal::SandboxPeer::spawn_fn_;
    internal::SandboxPeer::spawn_fn_ = +[](std::unique_ptr<Executor> executor,
                                           std::unique_ptr<Policy> policy) {
      return GetInstance().SpawnFn(std::move(executor), std::move(policy));
    };
  }
  void RestoreSpawnFn() { internal::SandboxPeer::spawn_fn_ = old_spawn_fn_; }
  void CrashNextUnwind() { crash_unwind_ = true; }

 private:
  internal::SandboxPeer::SpawnFn old_spawn_fn_;
  bool crash_unwind_ = false;
};

struct ScopedSpawnOverride {
  ScopedSpawnOverride() { StackTraceTestPeer::GetInstance().ReplaceSpawnFn(); }
  ~ScopedSpawnOverride() { StackTraceTestPeer::GetInstance().RestoreSpawnFn(); }
  ScopedSpawnOverride(ScopedSpawnOverride&&) = delete;
  ScopedSpawnOverride& operator=(ScopedSpawnOverride&&) = delete;
  ScopedSpawnOverride(const ScopedSpawnOverride&) = delete;
  ScopedSpawnOverride& operator=(const ScopedSpawnOverride&) = delete;

  void CrashNextUnwind() {
    StackTraceTestPeer::GetInstance().CrashNextUnwind();
  }
};

namespace {

namespace file_util = ::sapi::file_util;
using ::sapi::CreateDefaultPermissiveTestPolicy;
using ::sapi::GetTestSourcePath;
using ::testing::_;
using ::testing::Contains;
using ::testing::ContainsRegex;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::StartsWith;

struct TestCase {
  std::string testname = "CrashMe";
  int testno = 1;
  int testmode = 1;
  int final_status = Result::SIGNALED;
  std::string function_name = testname;
  std::string full_function_description = "CrashMe(char)";
  std::function<void(PolicyBuilder*)> modify_policy;
  absl::Duration wall_time_limit = absl::ZeroDuration();
};

class StackTraceTest : public ::testing::TestWithParam<TestCase> {};

// Test that symbolization of stack traces works.
void SymbolizationWorksCommon(TestCase param) {
  const std::string path = GetTestSourcePath("sandbox2/testcases/symbolize");
  std::vector<std::string> args = {path, absl::StrCat(param.testno),
                                   absl::StrCat(param.testmode)};

  PolicyBuilder builder = CreateDefaultPermissiveTestPolicy(path);
  if (param.modify_policy) {
    param.modify_policy(&builder);
  }
  SAPI_ASSERT_OK_AND_ASSIGN(auto policy, builder.TryBuild());

  Sandbox2 s2(std::make_unique<Executor>(path, args), std::move(policy));
  ASSERT_TRUE(s2.RunAsync());
  s2.set_walltime_limit(param.wall_time_limit);
  auto result = s2.AwaitResult();

  EXPECT_THAT(result.final_status(), Eq(param.final_status));
  EXPECT_THAT(result.stack_trace(), Contains(StartsWith(param.function_name)));
  // Check that demangling works as well.
  EXPECT_THAT(result.stack_trace(),
              Contains(StartsWith(param.full_function_description)));
  EXPECT_THAT(result.stack_trace(), Contains(StartsWith("RunTest")));
  EXPECT_THAT(result.stack_trace(), Contains(StartsWith("main")));
  if (param.testmode == 2) {
    EXPECT_THAT(result.stack_trace(),
                Contains(StartsWith("RecurseA")).Times(5));
    EXPECT_THAT(result.stack_trace(),
                Contains(StartsWith("RecurseB")).Times(5));
  } else if (param.testmode == 3) {
    EXPECT_THAT(result.stack_trace(), Contains(StartsWith("LibCallCallback")));
    EXPECT_THAT(result.stack_trace(), Contains(StartsWith("LibRecurse")));
    EXPECT_THAT(result.stack_trace(),
                Contains(StartsWith("LibRecurseA")).Times(5));
    EXPECT_THAT(result.stack_trace(),
                Contains(StartsWith("LibRecurseB")).Times(5));
  }
}

void SymbolizationWorksWithModifiedPolicy(
    std::function<void(PolicyBuilder*)> modify_policy) {
  TestCase test_case;
  test_case.modify_policy = std::move(modify_policy);
  SymbolizationWorksCommon(test_case);
}

TEST_P(StackTraceTest, SymbolizationWorksWithoutnNamespaces) {
  TestCase test_case = GetParam();
  auto old_modify_policy = test_case.modify_policy;
  test_case.modify_policy = [old_modify_policy](PolicyBuilder* builder) {
    *builder = PolicyBuilder();
    builder->DefaultAction(AllowAllSyscalls())
        .DisableNamespaces(NamespacesToken());
    if (old_modify_policy) {
      old_modify_policy(builder);
    }
  };
  SymbolizationWorksCommon(test_case);
}

TEST_P(StackTraceTest, SymbolizationWorks) {
  SymbolizationWorksCommon(GetParam());
}

TEST(StackTraceTest, SymbolizationWorksSandboxedLibunwindProcDirMounted) {
  SymbolizationWorksWithModifiedPolicy(
      [](PolicyBuilder* builder) { builder->AddDirectory("/proc"); });
}

TEST(StackTraceTest, SymbolizationWorksSandboxedLibunwindProcFileMounted) {
  SymbolizationWorksWithModifiedPolicy([](PolicyBuilder* builder) {
    builder->AddFile("/proc/sys/vm/overcommit_memory");
  });
}

TEST(StackTraceTest, SymbolizationWorksSandboxedLibunwindSysDirMounted) {
  SymbolizationWorksWithModifiedPolicy(
      [](PolicyBuilder* builder) { builder->AddDirectory("/sys"); });
}

TEST(StackTraceTest, SymbolizationWorksSandboxedLibunwindSysFileMounted) {
  SymbolizationWorksWithModifiedPolicy([](PolicyBuilder* builder) {
    builder->AddFile("/sys/devices/system/cpu/online");
  });
}

size_t FileCountInDirectory(const std::string& path) {
  std::vector<std::string> fds;
  std::string error;
  CHECK(file_util::fileops::ListDirectoryEntries(path, &fds, &error));
  return fds.size();
}

TEST(StackTraceTest, ForkEnterNsLibunwindDoesNotLeakFDs) {
  // Very first sanitization might create some fds (e.g. for initial
  // namespaces).
  SymbolizationWorksCommon({});

  // Get list of open FDs in the global forkserver.
  pid_t forkserver_pid = GlobalForkClient::GetPid();
  std::string forkserver_fd_path =
      absl::StrCat("/proc/", forkserver_pid, "/fd");
  size_t filecount_before = FileCountInDirectory(forkserver_fd_path);

  SymbolizationWorksCommon({});

  EXPECT_THAT(filecount_before, Eq(FileCountInDirectory(forkserver_fd_path)));
}

TEST(StackTraceTest, CompactStackTrace) {
  EXPECT_THAT(CompactStackTrace({}), IsEmpty());
  EXPECT_THAT(CompactStackTrace({"_start"}), ElementsAre("_start"));
  EXPECT_THAT(CompactStackTrace({
                  "_start",
                  "main",
                  "recursive_call",
                  "recursive_call",
                  "recursive_call",
                  "tail_call",
              }),
              ElementsAre("_start", "main", "recursive_call",
                          "(previous frame repeated 2 times)", "tail_call"));
  EXPECT_THAT(CompactStackTrace({
                  "_start",
                  "main",
                  "recursive_call",
                  "recursive_call",
                  "recursive_call",
                  "recursive_call",
              }),
              ElementsAre("_start", "main", "recursive_call",
                          "(previous frame repeated 3 times)"));
}

TEST(StackTraceTest, RecursiveStackTrace) {
  // Very first sandbox run will initialize spawn_fn_
  SKIP_SANITIZERS;
  ScopedSpawnOverride spawn_override;
  SymbolizationWorksCommon({});
  absl::ScopedMockLog log;
  EXPECT_CALL(
      log,
      Log(absl::LogSeverity::kInfo, _,
          ContainsRegex(
              "Libunwind execution status: SYSCALL VIOLATION.*Stack: \\w+")));
  spawn_override.CrashNextUnwind();
  const std::string path = GetTestSourcePath("sandbox2/testcases/symbolize");
  std::vector<std::string> args = {path, absl::StrCat(1), absl::StrCat(1)};
  PolicyBuilder builder = CreateDefaultPermissiveTestPolicy(path);
  SAPI_ASSERT_OK_AND_ASSIGN(auto policy, builder.TryBuild());

  Sandbox2 s2(std::make_unique<Executor>(path, args), std::move(policy));
  log.StartCapturingLogs();
  ASSERT_TRUE(s2.RunAsync());
  auto result = s2.AwaitResult();
  EXPECT_THAT(result.final_status(), Eq(Result::SIGNALED));
}

TEST(StackTraceTest, SymbolizationEnablesMonitor) {
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, StartsWith("Failed to enable unotify monitor")))
      .Times(0);
  log.StartCapturingLogs();
  SymbolizationWorksCommon({});
}

INSTANTIATE_TEST_SUITE_P(
    Instantiation, StackTraceTest,
    ::testing::Values(
        TestCase{
            .testname = "CrashMe",
            .testno = 1,
            .final_status = Result::SIGNALED,
            .full_function_description = "CrashMe(char)",
        },
        TestCase{
            .testname = "ViolatePolicy",
            .testno = 2,
            .final_status = Result::VIOLATION,
            .full_function_description = "ViolatePolicy(int)",
        },
        TestCase{
            .testname = "ExitNormally",
            .testno = 3,
            .final_status = Result::OK,
            .full_function_description = "ExitNormally(int)",
            .modify_policy =
                [](PolicyBuilder* builder) {
                  builder->CollectStacktracesOnExit(true);
                },
        },
        TestCase{
            .testname = "SleepForXSeconds",
            .testno = 4,
            .final_status = Result::TIMEOUT,
            .full_function_description = "SleepForXSeconds(int)",
            .wall_time_limit = absl::Seconds(1),
        },
        TestCase{
            .testname = "ViolatePolicyRecursive",
            .testno = 2,
            .testmode = 2,
            .final_status = Result::VIOLATION,
            .function_name = "ViolatePolicy",
            .full_function_description = "ViolatePolicy(int)",
        },
        TestCase{
            .testname = "ViolatePolicyRecursiveLib",
            .testno = 2,
            .testmode = 3,
            .final_status = Result::VIOLATION,
            .function_name = "ViolatePolicy",
            .full_function_description = "ViolatePolicy(int)",
        },
        TestCase{
            .testname = "ViolatePolicyForked",
            .testno = 5,
            .final_status = Result::VIOLATION,
            .function_name = "ViolatePolicy",
            .full_function_description = "ViolatePolicy(int)",
        }),
    [](const ::testing::TestParamInfo<TestCase>& info) {
      return info.param.testname;
    });

}  // namespace
}  // namespace sandbox2
