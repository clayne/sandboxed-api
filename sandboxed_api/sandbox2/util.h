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

// The sandbox2::util namespace provides various, uncategorized, functions
// useful for creating sandboxes.

#ifndef SANDBOXED_API_SANDBOX2_UTIL_H_
#define SANDBOXED_API_SANDBOX2_UTIL_H_

#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/base/macros.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "sandboxed_api/util/fileops.h"

namespace sandbox2 {

namespace internal {

// Magic values used to detect if the current process is running inside
// Sandbox2.
inline constexpr int64_t kMagicSyscallNo = 0xff000fdb;  // 4278194139
inline constexpr int kMagicSyscallErr = 0x000000fdb;    // 4059

}  // namespace internal

namespace util {

void DumpCoverageData();

// An char ptr array limited by the terminating nullptr entry (like environ
// or argv).
class CharPtrArray {
 public:
  CharPtrArray(char* const* array);
  static CharPtrArray FromStringVector(const std::vector<std::string>& vec);

  const std::vector<const char*>& array() const { return array_; }

  const char* const* data() const { return array_.data(); }

  std::vector<std::string> ToStringVector() const;

 private:
  CharPtrArray(const std::vector<std::string>& vec);

  const std::string content_;
  std::vector<const char*> array_;
};

// Converts an array of char* (terminated by a nullptr, like argv, or environ
// arrays), to an std::vector<std::string>.
ABSL_DEPRECATE_AND_INLINE()
inline void CharPtrArrToVecString(char* const* arr,
                                  std::vector<std::string>* vec) {
  *vec = sandbox2::util::CharPtrArray(arr).ToStringVector();
}

// Returns the program name (via /proc/self/comm) for a given PID.
std::string GetProgName(pid_t pid);

// Given a resource descriptor FD and a PID, returns link of /proc/PID/fds/FD.
absl::StatusOr<std::string> GetResolvedFdLink(pid_t pid, uint32_t fd);

// Returns the command line (via /proc/self/cmdline) for a given PID. The
// argument separators '\0' are converted to spaces.
std::string GetCmdLine(pid_t pid);

// Returns the specified line from /proc/<pid>/status for a given PID. 'value'
// is a field name like "Threads" or "Tgid".
std::string GetProcStatusLine(int pid, const std::string& value);

// Invokes a syscall, avoiding on-stack argument promotion, as it might happen
// with vararg syscall() function.
long Syscall(long sys_no,  // NOLINT
             uintptr_t a1 = 0, uintptr_t a2 = 0, uintptr_t a3 = 0,
             uintptr_t a4 = 0, uintptr_t a5 = 0, uintptr_t a6 = 0);

// Fork based on clone() which updates glibc's PID/TID caches - Based on:
// https://chromium.googlesource.com/chromium/src/+/9eb564175dbd452196f782da2b28e3e8e79c49a5%5E!/
//
// Return values as for 'man 2 fork'.
pid_t ForkWithFlags(int flags);

// Creates a new memfd.
absl::StatusOr<sapi::file_util::fileops::FDCloser> CreateMemFd(
    const char* name = "buffer_file");

ABSL_DEPRECATED("Use absl::StatusOr<FDCloser> version instead.")
inline bool CreateMemFd(int* fd, const char* name = "buffer_file") {
  absl::StatusOr<sapi::file_util::fileops::FDCloser> fd_closer =
      CreateMemFd(name);
  if (!fd_closer.ok()) {
    LOG(ERROR) << "Could not create memfd: " << fd_closer.status();
    return false;
  }
  *fd = fd_closer->Release();
  return true;
}

// Executes a the program given by argv and the specified environment and
// captures any output to stdout/stderr.
absl::StatusOr<int> Communicate(const std::vector<std::string>& argv,
                                const std::vector<std::string>& envv,
                                std::string* output);

// Returns signal description.
std::string GetSignalName(int signo);

// Returns the socket address family as a string ("AF_INET", ...)
std::string GetAddressFamily(int addr_family);

// Returns rlimit resource name
std::string GetRlimitName(int resource);

// Returns ptrace event name
std::string GetPtraceEventName(int event);

namespace internal {
// Reads `data`'s length of bytes from `ptr` in `pid`, returns number of bytes
// read or an error.
absl::StatusOr<size_t> ReadBytesFromPidWithReadv(pid_t pid, uintptr_t ptr,
                                                 absl::Span<char> data);

// Writes `data` to `ptr` in `pid`, returns number of bytes written or an error.
absl::StatusOr<size_t> WriteBytesToPidWithWritev(pid_t pid, uintptr_t ptr,
                                                 absl::Span<const char> data);

// Reads `data`'s length of bytes from `ptr` in `pid`, returns number of bytes
// read or an error.
absl::StatusOr<size_t> ReadBytesFromPidWithReadvInSplitChunks(
    pid_t pid, uintptr_t ptr, absl::Span<char> data);

// Reads `data`'s length of bytes from `ptr` in `pid`, returns number of bytes
// read or an error.
absl::StatusOr<size_t> ReadBytesFromPidWithProcMem(pid_t pid, uintptr_t ptr,
                                                   absl::Span<char> data);

// Writes `data` to `ptr` in `pid`, returns number of bytes written or an error.
absl::StatusOr<size_t> WriteBytesToPidWithProcMem(pid_t pid, uintptr_t ptr,
                                                  absl::Span<const char> data);
};  // namespace internal

// Reads `data`'s length of bytes from `ptr` in `pid`, returns number of bytes
// read or an error.
absl::StatusOr<size_t> ReadBytesFromPidInto(pid_t pid, uintptr_t ptr,
                                            absl::Span<char> data);

// Writes `data` to `ptr` in `pid`, returns number of bytes written or an error.
absl::StatusOr<size_t> WriteBytesToPidFrom(pid_t pid, uintptr_t remote_ptr,
                                           absl::Span<const char> data);

// Reads `size` bytes from the given `ptr` address, or returns an error.
absl::StatusOr<std::vector<uint8_t>> ReadBytesFromPid(pid_t pid, uintptr_t ptr,
                                                      size_t size);

// Reads a path string (NUL-terminated, shorter than PATH_MAX) from another
// process memory.
absl::StatusOr<std::string> ReadCPathFromPid(pid_t pid, uintptr_t ptr);

// Wrapper for execveat(2).
int Execveat(int dirfd, const char* pathname, const char* const argv[],
             const char* const envp[], int flags, uintptr_t extra_arg = 0);

// Returns true if the current process is running inside Sandbox2.
absl::StatusOr<bool> IsRunningInSandbox2();

}  // namespace util
}  // namespace sandbox2

#endif  // SANDBOXED_API_SANDBOX2_UTIL_H_
