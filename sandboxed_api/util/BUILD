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

load("@com_google_sandboxed_api//sandboxed_api/bazel:build_defs.bzl", "sapi_platform_copts")
load("@com_google_sandboxed_api//sandboxed_api/bazel:proto.bzl", "sapi_proto_library")

package(default_visibility = [
    "@com_google_sandboxed_api//sandboxed_api:__subpackages__",
])

licenses(["notice"])

# Another helper library with filesystem and path utility functions.
cc_library(
    name = "file_base",
    srcs = ["path.cc"],
    hdrs = ["path.h"],
    copts = sapi_platform_copts(),
    deps = ["@com_google_absl//absl/strings"],
)

cc_test(
    name = "file_base_test",
    size = "small",
    srcs = ["path_test.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":file_base",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# String file routines
cc_library(
    name = "file_helpers",
    srcs = ["file_helpers.cc"],
    hdrs = ["file_helpers.h"],
    copts = sapi_platform_copts(),
    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "file_helpers_test",
    size = "small",
    srcs = ["file_helpers_test.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":file_helpers",
        ":status_matchers",
        "@com_google_googletest//:gtest_main",
    ],
)

# Portable filesystem and path utility functions
cc_library(
    name = "fileops",
    srcs = ["fileops.cc"],
    hdrs = ["fileops.h"],
    copts = sapi_platform_copts(),
    deps = [
        ":strerror",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "fileops_test",
    size = "small",
    srcs = ["fileops_test.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":file_helpers",
        ":fileops",
        ":status_matchers",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_sandboxed_api//sandboxed_api:testing",
    ],
)

sapi_proto_library(
    name = "proto_arg",
    srcs = ["proto_arg.proto"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "proto_helper",
    srcs = ["proto_helper.cc"],
    hdrs = ["proto_helper.h"],
    copts = sapi_platform_copts(),
    deps = [
        ":proto_arg_cc_proto",
        ":status",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# Small support library emulating verbose logging using Abseil's raw logging
# facility.
cc_library(
    name = "raw_logging",
    srcs = ["raw_logging.cc"],
    hdrs = ["raw_logging.h"],
    copts = sapi_platform_copts(),
    deps = [
        ":strerror",
        "@com_google_absl//absl/base:config",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/base:log_severity",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "runfiles",
    srcs = ["runfiles_bazel.cc"],
    hdrs = ["runfiles.h"],
    copts = sapi_platform_copts(),
    visibility = ["//visibility:public"],
    deps = [
        ":file_base",
        "@bazel_tools//tools/cpp/runfiles",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_sandboxed_api//sandboxed_api/util:raw_logging",
    ],
)

sapi_proto_library(
    name = "status_proto",
    srcs = ["status.proto"],
)

# Implementations of utility functions not released with absl::Status.
cc_library(
    name = "status",
    srcs = ["status.cc"],
    hdrs = [
        "status.h",
        "status_macros.h",
    ],
    copts = sapi_platform_copts(),
    visibility = ["//visibility:public"],
    deps = [
        ":status_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:cord",
    ],
)

# gMock matchers for absl::Status and absl::StatusOr<T> and a gUnit printer
# extension. Adapted from the version in Asylo.
cc_library(
    name = "status_matchers",
    testonly = 1,
    hdrs = ["status_matchers.h"],
    copts = sapi_platform_copts(),
    visibility = ["//visibility:public"],
    deps = [
        ":status",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_absl//absl/types:optional",
        "@com_google_googletest//:gtest",
    ],
)

# Tests for the Status utility.
cc_test(
    name = "status_test",
    srcs = ["status_test.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":status",
        ":status_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_googletest//:gtest_main",
    ],
)

# Tests for the Status macros.
cc_test(
    name = "status_macros_test",
    srcs = ["status_macros_test.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":status",
        ":status_matchers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# Internal thread-safe helper to format system error messages. Mostly
# equivalent to base/strerror.h.
cc_library(
    name = "strerror",
    srcs = ["strerror.cc"],
    hdrs = ["strerror.h"],
    copts = sapi_platform_copts(),
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "strerror_test",
    srcs = ["strerror_test.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":strerror",
        ":thread",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "temp_file",
    srcs = ["temp_file.cc"],
    hdrs = ["temp_file.h"],
    copts = sapi_platform_copts(),
    deps = [
        ":status",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "temp_file_test",
    srcs = ["temp_file_test.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":file_base",
        ":fileops",
        ":status_matchers",
        ":temp_file",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_sandboxed_api//sandboxed_api:testing",
    ],
)

cc_library(
    name = "thread",
    hdrs = ["thread.h"],
    copts = sapi_platform_copts(),
    deps = [
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/strings:string_view",
    ],
)
