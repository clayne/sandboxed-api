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

#include "sandboxed_api/testing.h"

#include <cstdlib>
#include <string>

#include "absl/strings/string_view.h"
#include "sandboxed_api/config.h"
#include "sandboxed_api/sandbox2/allowlists/all_syscalls.h"
#include "sandboxed_api/sandbox2/policybuilder.h"
#include "sandboxed_api/util/path.h"

namespace sapi {

sandbox2::PolicyBuilder CreateDefaultPermissiveTestPolicy(
    absl::string_view binary_path) {
  sandbox2::PolicyBuilder builder;
  // Don't restrict the syscalls at all.
  builder.DefaultAction(sandbox2::AllowAllSyscalls());
  if (IsCoverageRun()) {
    builder.AddDirectory(absl::NullSafeStringView(getenv("COVERAGE_DIR")),
                         /*is_ro=*/false);
  }
  if constexpr (sapi::sanitizers::IsAny()) {
    builder.AddLibrariesForBinary(binary_path);
  }
  if constexpr (sapi::sanitizers::IsAny()) {
    builder.AddDirectory("/proc");
  }
  builder.AllowTcMalloc();
  return builder;
}

std::string GetTestTempPath(absl::string_view name) {
  // When using Bazel, the environment variable TEST_TMPDIR is guaranteed to be
  // set.
  // See https://bazel.build/reference/test-encyclopedia#initial-conditions for
  // details.
  const char* test_tmpdir = getenv("TEST_TMPDIR");
  return file::JoinPath(test_tmpdir ? test_tmpdir : ".", name);
}

std::string GetTestSourcePath(absl::string_view name) {
  // Like in GetTestTempPath(), when using Bazel, the environment variable
  // TEST_SRCDIR is guaranteed to be set.
  const char* test_srcdir = getenv("TEST_SRCDIR");
  return file::JoinPath(test_srcdir ? test_srcdir : ".",
                        "com_google_sandboxed_api/sandboxed_api", name);
}

}  // namespace sapi
