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

// Implementation of the sandbox2::Regs class.

#include "sandboxed_api/sandbox2/regs.h"

#include <elf.h>  // IWYU pragma: keep // used for NT_PRSTATUS inside an ifdef
#include <sys/ptrace.h>
#include <sys/uio.h>  // IWYU pragma: keep // used for iovec

#include <cerrno>
#include <cstdint>

#include "absl/base/optimization.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "sandboxed_api/config.h"

namespace sandbox2 {

#ifndef NT_ARM_SYSTEM_CALL
#define NT_ARM_SYSTEM_CALL 0x404
#endif

absl::Status Regs::Fetch() {
#ifdef SAPI_X86_64
  if (ptrace(PTRACE_GETREGS, pid_, 0, &user_regs_) == -1L) {
    return absl::ErrnoToStatus(
        errno, absl::StrCat("ptrace(PTRACE_GETREGS, pid=", pid_, ") failed"));
  }
#endif
  if constexpr (sapi::host_cpu::IsPPC64LE() || sapi::host_cpu::IsArm64() ||
                sapi::host_cpu::IsArm()) {
    iovec pt_iov = {&user_regs_, sizeof(user_regs_)};

    if (ptrace(PTRACE_GETREGSET, pid_, NT_PRSTATUS, &pt_iov) == -1L) {
      return absl::ErrnoToStatus(
          errno,
          absl::StrCat("ptrace(PTRACE_GETREGSET, pid=", pid_, ") failed"));
    }
    if (pt_iov.iov_len != sizeof(user_regs_)) {
      return absl::InternalError(absl::StrCat(
          "ptrace(PTRACE_GETREGSET, pid=", pid_,
          ") size returned: ", pt_iov.iov_len,
          " different than sizeof(user_regs_): ", sizeof(user_regs_)));
    }

    // On AArch64, we are not done yet. Read the syscall number.
    if constexpr (sapi::host_cpu::IsArm64()) {
      iovec sys_iov = {&syscall_number_, sizeof(syscall_number_)};

      if (ptrace(PTRACE_GETREGSET, pid_, NT_ARM_SYSTEM_CALL, &sys_iov) == -1L) {
        return absl::ErrnoToStatus(
            errno, absl::StrCat("ptrace(PTRACE_GETREGSET, pid=", pid_,
                                ", NT_ARM_SYSTEM_CALL)"));
      }
      if (sys_iov.iov_len != sizeof(syscall_number_)) {
        return absl::InternalError(absl::StrCat(
            "ptrace(PTRACE_GETREGSET, pid=", pid_,
            ", NT_ARM_SYSTEM_CALL) size returned: ", sys_iov.iov_len,
            " different than sizeof(syscall_number_): ",
            sizeof(syscall_number_)));
      }
    }
  }
  return absl::OkStatus();
}

absl::Status Regs::Store() {
#ifdef SAPI_X86_64
  if (ptrace(PTRACE_SETREGS, pid_, 0, &user_regs_) == -1) {
    return absl::ErrnoToStatus(
        errno, absl::StrCat("ptrace(PTRACE_SETREGS, pid=", pid_, ")"));
  }
#endif
  if constexpr (sapi::host_cpu::IsPPC64LE() || sapi::host_cpu::IsArm64() ||
                sapi::host_cpu::IsArm()) {
    iovec pt_iov = {&user_regs_, sizeof(user_regs_)};

    if (ptrace(PTRACE_SETREGSET, pid_, NT_PRSTATUS, &pt_iov) == -1L) {
      return absl::ErrnoToStatus(
          errno,
          absl::StrCat("ptrace(PTRACE_SETREGSET, pid=", pid_, ") failed"));
    }

    // Store syscall number on AArch64.
    if constexpr (sapi::host_cpu::IsArm64()) {
      iovec sys_iov = {&syscall_number_, sizeof(syscall_number_)};

      if (ptrace(PTRACE_SETREGSET, pid_, NT_ARM_SYSTEM_CALL, &sys_iov) == -1L) {
        return absl::ErrnoToStatus(
            errno, absl::StrCat("ptrace(PTRACE_SETREGSET, pid=", pid_,
                                ", NT_ARM_SYSTEM_CALL) failed"));
      }
    }
  }
  return absl::OkStatus();
}

absl::Status Regs::SkipSyscallReturnValue(uintptr_t value) {
#if defined(SAPI_X86_64)
  user_regs_.orig_rax = -1;
  user_regs_.rax = value;
#elif defined(SAPI_PPC64_LE)
  user_regs_.gpr[0] = -1;
  user_regs_.gpr[3] = value;
#elif defined(SAPI_ARM64)
  syscall_number_ = -1;
  user_regs_.regs[0] = value;
#elif defined(SAPI_ARM)
  user_regs_.orig_x0 = -1;
  user_regs_.regs[7] = value;
#endif
  return Store();
}

Syscall Regs::ToSyscall(sapi::cpu::Architecture syscall_arch) const {
#if defined(SAPI_X86_64)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kX8664)) {
    auto syscall = user_regs_.orig_rax;
    Syscall::Args args = {user_regs_.rdi, user_regs_.rsi, user_regs_.rdx,
                          user_regs_.r10, user_regs_.r8,  user_regs_.r9};
    auto sp = user_regs_.rsp;
    auto ip = user_regs_.rip;
    return Syscall(syscall_arch, syscall, args, pid_, sp, ip);
  }
  if (syscall_arch == sapi::cpu::kX86) {
    auto syscall = user_regs_.orig_rax & 0xFFFFFFFF;
    Syscall::Args args = {
        user_regs_.rbx & 0xFFFFFFFF, user_regs_.rcx & 0xFFFFFFFF,
        user_regs_.rdx & 0xFFFFFFFF, user_regs_.rsi & 0xFFFFFFFF,
        user_regs_.rdi & 0xFFFFFFFF, user_regs_.rbp & 0xFFFFFFFF};
    auto sp = user_regs_.rsp & 0xFFFFFFFF;
    auto ip = user_regs_.rip & 0xFFFFFFFF;
    return Syscall(syscall_arch, syscall, args, pid_, sp, ip);
  }
#elif defined(SAPI_PPC64_LE)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kPPC64LE)) {
    auto syscall = user_regs_.gpr[0];
    Syscall::Args args = {user_regs_.orig_gpr3, user_regs_.gpr[4],
                          user_regs_.gpr[5],    user_regs_.gpr[6],
                          user_regs_.gpr[7],    user_regs_.gpr[8]};
    auto sp = user_regs_.gpr[1];
    auto ip = user_regs_.nip;
    return Syscall(syscall_arch, syscall, args, pid_, sp, ip);
  }
#elif defined(SAPI_ARM64)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kArm64)) {
    Syscall::Args args = {
        // First argument should be orig_x0, which is not available to ptrace on
        // AArch64 (see
        // https://undo.io/resources/arm64-vs-arm32-whats-different-linux-programmers/),
        // as it will have been overwritten. For our use case, though, using
        // regs[0] is fine, as we are always called on syscall entry and never
        // on exit.
        user_regs_.regs[0], user_regs_.regs[1], user_regs_.regs[2],
        user_regs_.regs[3], user_regs_.regs[4], user_regs_.regs[5],
    };
    auto sp = user_regs_.sp;
    auto ip = user_regs_.pc;
    return Syscall(syscall_arch, syscall_number_, args, pid_, sp, ip);
  }
#elif defined(SAPI_ARM)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kArm)) {
    Syscall::Args args = {
        user_regs_.orig_x0, user_regs_.regs[1], user_regs_.regs[2],
        user_regs_.regs[3], user_regs_.regs[4], user_regs_.regs[5],
    };
    auto sp = user_regs_.regs[13];
    auto ip = user_regs_.pc;
    return Syscall(syscall_arch, user_regs_.regs[7], args, pid_, sp, ip);
  }
#endif
  return Syscall(pid_);
}

int64_t Regs::GetReturnValue(sapi::cpu::Architecture syscall_arch) const {
#if defined(SAPI_X86_64)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kX8664)) {
    return static_cast<int64_t>(user_regs_.rax);
  }
  if (syscall_arch == sapi::cpu::kX86) {
    return static_cast<int32_t>(user_regs_.rax & 0xFFFFFFFF);
  }
#elif defined(SAPI_PPC64_LE)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kPPC64LE)) {
    return static_cast<int64_t>(user_regs_.gpr[3]);
  }
#elif defined(SAPI_ARM64)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kArm64)) {
    return static_cast<int64_t>(user_regs_.regs[0]);
  }
#elif defined(SAPI_ARM)
  if (ABSL_PREDICT_TRUE(syscall_arch == sapi::cpu::kArm)) {
    return static_cast<int32_t>(user_regs_.regs[0]);
  }
#endif
  return -1;
}

}  // namespace sandbox2
