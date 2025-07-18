# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Description: test cases for sandbox2 unit tests.
#
# Some of the following cc_binary options avoid dynamic linking which uses a
# lot of syscalls (open, mmap, etc.):
#   linkstatic = True                 Default for cc_binary
#   features = ["fully_static_link"]  Adds -static
#
# Note that linking fully static with an unmodified glibc is not generally
# considered safe, due to glibc relying heavily on loading shared objects at
# runtime.
# The rule of thumb when it is safe to do so is when the program either only
# uses plain syscalls (bypassing any libc altogether) or if it does not use
# any networking and none of the functionality from cstdio/stdio.h (due to
# auto-loading of locale-specific shared objecs).

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//sandboxed_api/bazel:build_defs.bzl", "sapi_platform_copts")

package(default_visibility = [
    "//sandboxed_api/sandbox2:__subpackages__",
])

licenses(["notice"])

cc_binary(
    name = "abort",
    testonly = True,
    srcs = ["abort.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = ["//sandboxed_api/util:raw_logging"],
)

cc_binary(
    name = "add_policy_on_syscalls",
    testonly = True,
    srcs = ["add_policy_on_syscalls.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_binary(
    name = "buffer",
    testonly = True,
    srcs = ["buffer.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:buffer",
    ],
)

cc_binary(
    name = "ipc",
    testonly = True,
    srcs = ["ipc.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:client",
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/util:raw_logging",
        "@abseil-cpp//absl/strings",
    ],
)

cc_binary(
    name = "malloc_system",
    testonly = True,
    srcs = ["malloc.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_binary(
    name = "minimal_dynamic",
    testonly = True,
    srcs = ["minimal.cc"],
    copts = sapi_platform_copts(),
)

cc_binary(
    name = "minimal",
    testonly = True,
    srcs = ["minimal.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_binary(
    name = "personality",
    testonly = True,
    srcs = ["personality.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_binary(
    name = "pidcomms",
    testonly = True,
    srcs = ["pidcomms.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:client",
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/util:raw_logging",
    ],
)

cc_binary(
    name = "policy",
    testonly = True,
    srcs = ["policy.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api:config",
        "@abseil-cpp//absl/base:core_headers",
    ],
)

cc_binary(
    name = "sandbox_detection",
    testonly = True,
    srcs = ["sandbox_detection.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:client",
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/sandbox2:util",
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_binary(
    name = "sanitizer",
    testonly = True,
    srcs = ["sanitizer.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_binary(
    name = "close_fds",
    testonly = True,
    srcs = ["close_fds.cc"],
    copts = sapi_platform_copts(),
    deps = [
        "//sandboxed_api/sandbox2:sanitizer",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
    ],
)

cc_binary(
    name = "sleep",
    testonly = True,
    srcs = ["sleep.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_library(
    name = "symbolize_lib",
    testonly = True,
    srcs = ["symbolize_lib.cc"],
    hdrs = ["symbolize_lib.h"],
    copts = sapi_platform_copts([
        "-fno-omit-frame-pointer",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
    ]),
    features = ["fully_static_link"],
    deps = [
        "@abseil-cpp//absl/base:core_headers",
    ],
)

cc_binary(
    name = "symbolize",
    testonly = True,
    srcs = ["symbolize.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        ":symbolize_lib",
        "//sandboxed_api/util:raw_logging",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/strings",
    ],
)

cc_binary(
    name = "tsync",
    testonly = True,
    srcs = ["tsync.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:client",
        "//sandboxed_api/sandbox2:comms",
    ],
)

cc_binary(
    name = "starve",
    testonly = True,
    srcs = ["starve.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_binary(
    name = "limits",
    testonly = True,
    srcs = ["limits.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
)

cc_binary(
    name = "namespace",
    testonly = True,
    srcs = ["namespace.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/util:file_base",
        "//sandboxed_api/util:fileops",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/strings",
    ],
)

cc_binary(
    name = "network_proxy",
    testonly = True,
    srcs = ["network_proxy.cc"],
    copts = sapi_platform_copts(),
    deps = [
        "//sandboxed_api/sandbox2:client",
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/sandbox2/network_proxy:client",
        "//sandboxed_api/util:fileops",
        "//sandboxed_api/util:status",
        "@abseil-cpp//absl/base:log_severity",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_binary(
    name = "custom_fork",
    testonly = True,
    srcs = ["custom_fork.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/sandbox2:forkingclient",
        "//sandboxed_api/util:raw_logging",
    ],
)

cc_binary(
    name = "util_communicate",
    testonly = True,
    srcs = ["util_communicate.cc"],
    copts = sapi_platform_copts(),
)

cc_binary(
    name = "posix_timers",
    testonly = True,
    srcs = ["posix_timers.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    linkopts = ["-lrt"],
    deps = [
        "@abseil-cpp//absl/base:log_severity",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/time",
    ],
)

cc_binary(
    name = "execveat",
    testonly = True,
    srcs = ["execveat.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:client",
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/sandbox2:util",
    ],
)

cc_binary(
    name = "terminate_process_group",
    testonly = True,
    srcs = ["terminate_process_group.cc"],
    data = [":terminate_process_group_sandboxee"],
    deps = [
        "//sandboxed_api:testing",
        "//sandboxed_api/sandbox2",
        "//sandboxed_api/sandbox2:comms",
        "//sandboxed_api/sandbox2:executor",
        "//sandboxed_api/sandbox2:global_forkserver",
        "//sandboxed_api/sandbox2:policybuilder",
        "//sandboxed_api/sandbox2:result",
        "@abseil-cpp//absl/base:log_severity",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_binary(
    name = "terminate_process_group_sandboxee",
    testonly = True,
    srcs = ["terminate_process_group_sandboxee.cc"],
    copts = sapi_platform_copts(),
    features = ["fully_static_link"],
    deps = [
        "//sandboxed_api/sandbox2:comms",
        "@abseil-cpp//absl/log:check",
    ],
)
