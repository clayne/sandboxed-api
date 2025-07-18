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

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//sandboxed_api/bazel:build_defs.bzl", "sapi_platform_copts")
load("//sandboxed_api/bazel:proto.bzl", "sapi_proto_library")
load("//sandboxed_api/bazel:sapi.bzl", "sapi_library")

licenses(["notice"])

sapi_proto_library(
    name = "sum_params_proto",
    srcs = ["sum_params.proto"],
    visibility = ["//visibility:public"],
    alwayslink = 1,
)

cc_library(
    name = "sum",
    srcs = [
        "sum.c",
        "sum_cpp.cc",
    ],
    copts = sapi_platform_copts(),
    visibility = ["//visibility:public"],
    deps = [
        ":sum_params_cc_proto",
        "@abseil-cpp//absl/log",
    ],
    alwayslink = 1,  # All functions are linked into depending binaries
)

sapi_library(
    name = "sum-sapi",
    functions = [
        "sum",
        "sums",
        "addf",
        "sub",
        "mul",
        "divs",
        "muld",
        "crash",
        "violate",
        "sumarr",
        "testptr",
        "read_int",
        "sleep_for_sec",
        "sumproto",
    ],
    generator_version = 2,
    input_files = [
        "sum.c",
        "sum_cpp.cc",
    ],
    lib = ":sum",
    lib_name = "Sum",
    namespace = "",
    visibility = ["//visibility:public"],
    deps = [":sum_params_cc_proto"],
)

# A quick'n'dirty testing binary
cc_binary(
    name = "main_sum",
    srcs = ["main_sum.cc"],
    copts = sapi_platform_copts(),
    deps = [
        ":sum-sapi",
        ":sum_params_cc_proto",
        "//sandboxed_api:sapi",
        "//sandboxed_api:vars",
        "//sandboxed_api/util:status",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/base:log_severity",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

# For now we only test exit status from the binary
sh_test(
    name = "main_sum_test",
    srcs = ["main_sum_test.sh"],
    data = [":main_sum"],
)
