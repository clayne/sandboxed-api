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

# The 'custom_fork' example demonstrates how to:
# - create a custom fork-server, which will prepare and fork a sandboxee
#   from the current process

load("@com_google_sandboxed_api//sandboxed_api/bazel:build_defs.bzl", "sapi_platform_copts")

package(default_visibility = [
    "@com_google_sandboxed_api//sandboxed_api/sandbox2:__subpackages__",
])

licenses(["notice"])

# Executor
cc_binary(
    name = "custom_fork_sandbox",
    srcs = ["custom_fork_sandbox.cc"],
    copts = sapi_platform_copts(),
    data = [":custom_fork_bin"],
    deps = [
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/log:globals",
        "@com_google_absl//absl/log:initialize",
        "@com_google_absl//absl/time",
        "@com_google_sandboxed_api//sandboxed_api:config",
        "@com_google_sandboxed_api//sandboxed_api/sandbox2",
        "@com_google_sandboxed_api//sandboxed_api/sandbox2:comms",
        "@com_google_sandboxed_api//sandboxed_api/sandbox2:fork_client",
        "@com_google_sandboxed_api//sandboxed_api/util:runfiles",
    ],
)

# Sandboxee
cc_binary(
    name = "custom_fork_bin",
    srcs = ["custom_fork_bin.cc"],
    copts = sapi_platform_copts(),
    deps = [
        "@com_google_absl//absl/base:log_severity",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/log:globals",
        "@com_google_absl//absl/log:initialize",
        "@com_google_sandboxed_api//sandboxed_api/sandbox2:comms",
        "@com_google_sandboxed_api//sandboxed_api/sandbox2:forkingclient",
        "@com_google_sandboxed_api//sandboxed_api/util:raw_logging",
    ],
)

# Test
sh_test(
    name = "custom_fork_sandbox_test",
    srcs = ["custom_fork_sandbox_test.sh"],
    data = [":custom_fork_sandbox"],
    tags = ["no_qemu_user_mode"],
)
