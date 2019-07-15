# Copyright 2019 Google LLC. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_subdirectory(lib)

# sandboxed_api/examples/stringop:main_stringop
add_executable(main_stringop
  main_stringop.cc
)
target_link_libraries(main_stringop PRIVATE
  absl::memory
  absl::strings
  absl::time
  glog::glog
  sapi::flags
  sapi::sapi
  sapi::status
  sapi::stringop_sapi
  sapi::test_main
  sapi::vars
)
gtest_discover_tests(main_stringop)