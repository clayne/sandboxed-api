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

#include "sandboxed_api/sandbox2/mounts.h"

#include <unistd.h>

#include <cstddef>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "sandboxed_api/sandbox2/mount_tree.pb.h"
#include "sandboxed_api/testing.h"
#include "sandboxed_api/util/path.h"
#include "sandboxed_api/util/temp_file.h"

namespace sandbox2 {
namespace {

namespace file = ::sapi::file;
using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::sapi::CreateNamedTempFileAndClose;
using ::sapi::CreateTempDir;
using ::sapi::GetTestSourcePath;
using ::sapi::GetTestTempPath;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::StrEq;

constexpr size_t kTmpfsSize = 1024;

TEST(MountTreeTest, TestInvalidFilenames) {
  Mounts mounts;

  EXPECT_THAT(mounts.AddFile(""), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFile("a"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFileAt("/a", ""),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFileAt("", "/a"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFileAt("/a", "a"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFile("/"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFileAt("/a", "/"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.Remove("/"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(MountTreeTest, TestAddFile) {
  Mounts mounts;

  EXPECT_THAT(mounts.AddFile("/a"), IsOk());
  EXPECT_THAT(mounts.AddFile("/b"), IsOk());
  EXPECT_THAT(mounts.AddFile("/c/d"), IsOk());
  EXPECT_THAT(mounts.AddFile("/c/e"), IsOk());
  EXPECT_THAT(mounts.AddFile("/c/dd/e"), IsOk());

  EXPECT_THAT(mounts.AddFileAt("/a", "/f"), IsOk());
}

TEST(MountTreeTest, TestAddDir) {
  Mounts mounts;

  EXPECT_THAT(mounts.AddDirectoryAt("/a", "/a"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt("/c/d", "/c/d"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt("/c/d/e", "/c/d/e"), IsOk());
}

TEST(MountTreeTest, TestAddTmpFs) {
  Mounts mounts;

  EXPECT_THAT(mounts.AddTmpfs("/a", kTmpfsSize), IsOk());
  EXPECT_THAT(mounts.AddTmpfs("/a/b", kTmpfsSize), IsOk());
  EXPECT_THAT(mounts.AddFile("/a/b/c"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt("/a/b/d", "/a/b/d"), IsOk());
}

TEST(MountTreeTest, TestRemove) {
  Mounts mounts;
  EXPECT_THAT(mounts.AddTmpfs("/a", kTmpfsSize), IsOk());
  EXPECT_THAT(mounts.AddFile("/b/c/d"), IsOk());
  EXPECT_THAT(mounts.AddFile("/c/c/d"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt("/d/b/d", "/d/b/d"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt("/e/b/d", "/e/b/d"), IsOk());
  EXPECT_THAT(mounts.Remove("/a/b"), StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(mounts.Remove("/a"), IsOk());
  EXPECT_THAT(mounts.Remove("/b/c/d/e"), StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(mounts.Remove("/b/c/e"), StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(mounts.Remove("/b/c/d"), IsOk());
  EXPECT_THAT(mounts.Remove("/c"), IsOk());
  EXPECT_THAT(mounts.Remove("/d/b/d/e"), StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(mounts.Remove("/d/b/d"), IsOk());
  EXPECT_THAT(mounts.Remove("/e"), IsOk());
  EXPECT_THAT(mounts.Remove("/f"), StatusIs(absl::StatusCode::kNotFound));
}

TEST(MountTreeTest, TestMultipleInsertionFileSymlink) {
  Mounts mounts;

  SAPI_ASSERT_OK_AND_ASSIGN(std::string path,
                            CreateNamedTempFileAndClose(
                                file::JoinPath(GetTestTempPath(), "testdir_")));
  SAPI_ASSERT_OK_AND_ASSIGN(std::string symlink_path,
                            CreateNamedTempFileAndClose(
                                file::JoinPath(GetTestTempPath(), "testdir_")));

  ASSERT_THAT(unlink(symlink_path.c_str()), Eq(0));
  ASSERT_THAT(symlink(path.c_str(), symlink_path.c_str()), Eq(0));

  EXPECT_THAT(mounts.AddFileAt(path, "/a"), IsOk());
  EXPECT_THAT(mounts.AddFileAt(path, "/a"), IsOk());
  EXPECT_THAT(mounts.AddFileAt(symlink_path, "/a"), IsOk());
}

TEST(MountTreeTest, TestMultipleInsertionUpgradeToWritable) {
  Mounts mounts;
  EXPECT_THAT(mounts.AddFile("/a"), IsOk());
  EXPECT_THAT(mounts.AddFile("/a", /*is_ro=*/false), IsOk());
  EXPECT_THAT(mounts.AddDirectory("/b"), IsOk());
  EXPECT_THAT(mounts.AddDirectory("/b", /*is_ro=*/false), IsOk());
  EXPECT_THAT(mounts.AddFile("/c", /*is_ro=*/false), IsOk());
  EXPECT_THAT(mounts.AddFile("/c"), IsOk());
  EXPECT_THAT(mounts.AddDirectory("/d", /*is_ro=*/false), IsOk());
  EXPECT_THAT(mounts.AddDirectory("/d"), IsOk());
}

TEST(MountTreeTest, TestMultipleInsertionDirSymlink) {
  Mounts mounts;

  SAPI_ASSERT_OK_AND_ASSIGN(
      std::string path,
      CreateTempDir(file::JoinPath(GetTestTempPath(), "testdir_")));
  SAPI_ASSERT_OK_AND_ASSIGN(std::string symlink_path,
                            CreateNamedTempFileAndClose(
                                file::JoinPath(GetTestTempPath(), "testdir_")));

  ASSERT_THAT(unlink(symlink_path.c_str()), Eq(0));
  ASSERT_THAT(symlink(path.c_str(), symlink_path.c_str()), Eq(0));

  EXPECT_THAT(mounts.AddDirectoryAt(path, "/a"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt(path, "/a"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt(symlink_path, "/a"), IsOk());
  EXPECT_THAT(mounts.AddDirectoryAt(symlink_path, "/a"), IsOk());
}

TEST(MountTreeTest, TestMultipleInsertion) {
  Mounts mounts;

  EXPECT_THAT(mounts.AddFile("/c/d"), IsOk());

  EXPECT_THAT(mounts.AddFile("/c"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(mounts.AddFileAt("/f", "/c"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(mounts.AddDirectoryAt("/f", "/c"), IsOk());

  EXPECT_THAT(mounts.AddFile("/c/d/e"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(mounts.AddFileAt("/f", "/c/d/e"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(mounts.AddDirectoryAt("/f", "/c/d/e"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MountTreeTest, TestEvilNullByte) {
  Mounts mounts;
  // Create the filename with a null byte this way as g4 fix forces newlines
  // otherwise.
  std::string filename = "/a/b";
  filename[2] = '\0';

  EXPECT_THAT(mounts.AddFile(filename),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFileAt(filename, "/a"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddFileAt("/a", filename),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddDirectoryAt(filename, "/a"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddDirectoryAt("/a", filename),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.AddTmpfs(filename, kTmpfsSize),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(mounts.Remove(filename),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(MountTreeTest, TestMinimalDynamicBinary) {
  Mounts mounts;
  EXPECT_THAT(mounts.AddMappingsForBinary(
                  GetTestSourcePath("sandbox2/testcases/minimal_dynamic")),
              IsOk());
  EXPECT_THAT(mounts.AddFile("/lib/x86_64-linux-gnu/libc.so.6"), IsOk());
}

TEST(MountTreeTest, TestList) {
  struct TestCase {
    absl::string_view path;
    bool is_ro;
  };
  constexpr TestCase kTestCases[] = {
      // NOTE: Directories have a trailing '/'; files don't.
      {"/a/b", true},          // File
      {"/a/c/", true},         // Directory
      {"/a/c/d/e/f/g", true},  // File
      {"/h", true},            // File
      {"/i/j/k", false},       // File
      {"/i/l/", false},        // Directory
  };

  Mounts mounts;

  // Create actual directories and files on disk and selectively add
  for (const auto& test_case : kTestCases) {
    const auto inside_path = test_case.path;
    const std::string outside_path = absl::StrCat("/some/dir/", inside_path);
    if (absl::EndsWith(outside_path, "/")) {
      ASSERT_THAT(
          mounts.AddDirectoryAt(file::CleanPath(outside_path),
                                file::CleanPath(inside_path), test_case.is_ro),
          IsOk());
    } else {
      ASSERT_THAT(
          mounts.AddFileAt(file::CleanPath(outside_path),
                           file::CleanPath(inside_path), test_case.is_ro),
          IsOk());
    }
  }

  ASSERT_THAT(mounts.AddTmpfs(file::CleanPath("/d"), 1024 * 1024), IsOk());

  std::vector<std::string> outside_entries;
  std::vector<std::string> inside_entries;
  mounts.RecursivelyListMounts(&outside_entries, &inside_entries);

  EXPECT_THAT(inside_entries, ElementsAreArray({
                                  "R /a/b",
                                  "R /a/c/",
                                  "R /a/c/d/e/f/g",
                                  "R /h",
                                  "W /i/j/k",
                                  "W /i/l/",
                                  "/d",
                              }));
  EXPECT_THAT(outside_entries, ElementsAreArray({
                                   absl::StrCat("/some/dir/", "a/b"),
                                   absl::StrCat("/some/dir/", "a/c/"),
                                   absl::StrCat("/some/dir/", "a/c/d/e/f/g"),
                                   absl::StrCat("/some/dir/", "h"),
                                   absl::StrCat("/some/dir/", "i/j/k"),
                                   absl::StrCat("/some/dir/", "i/l/"),
                                   absl::StrCat("tmpfs: size=", 1024 * 1024),
                               }));
}

TEST(MountTreeTest, TestIsWritable) {
  MountTree::Node nodes[7];
  MountTree::FileNode* fn0 = nodes[0].mutable_file_node();
  fn0->set_writable(false);
  fn0->set_outside("foo");
  MountTree::FileNode* fn1 = nodes[1].mutable_file_node();
  fn1->set_writable(true);
  fn1->set_outside("bar");
  MountTree::DirNode* dn0 = nodes[2].mutable_dir_node();
  dn0->set_writable(false);
  dn0->set_outside("foo");
  MountTree::DirNode* dn1 = nodes[3].mutable_dir_node();
  dn1->set_writable(true);
  dn1->set_outside("bar");
  MountTree::RootNode* rn0 = nodes[4].mutable_root_node();
  rn0->set_writable(false);
  MountTree::RootNode* rn1 = nodes[5].mutable_root_node();
  rn1->set_writable(true);
  MountTree::TmpfsNode* tn0 = nodes[6].mutable_tmpfs_node();
  tn0->set_tmpfs_options("option1");

  EXPECT_FALSE(internal::IsWritable(nodes[0]));
  EXPECT_TRUE(internal::IsWritable(nodes[1]));
  EXPECT_FALSE(internal::IsWritable(nodes[2]));
  EXPECT_TRUE(internal::IsWritable(nodes[3]));
  EXPECT_FALSE(internal::IsWritable(nodes[4]));
  EXPECT_TRUE(internal::IsWritable(nodes[5]));
  EXPECT_FALSE(internal::IsWritable(nodes[6]));
}

TEST(MountTreeTest, TestHasSameTarget) {
  MountTree::Node nodes[10];
  MountTree::FileNode* fn0 = nodes[0].mutable_file_node();
  fn0->set_writable(false);
  fn0->set_outside("foo");
  MountTree::FileNode* fn1 = nodes[1].mutable_file_node();
  fn1->set_writable(true);
  fn1->set_outside("foo");
  MountTree::FileNode* fn2 = nodes[2].mutable_file_node();
  fn2->set_writable(false);
  fn2->set_outside("bar");
  MountTree::DirNode* dn0 = nodes[3].mutable_dir_node();
  dn0->set_writable(false);
  dn0->set_outside("foo");
  MountTree::DirNode* dn1 = nodes[4].mutable_dir_node();
  dn1->set_writable(true);
  dn1->set_outside("foo");
  MountTree::DirNode* dn2 = nodes[5].mutable_dir_node();
  dn2->set_writable(false);
  dn2->set_outside("bar");
  MountTree::TmpfsNode* tn0 = nodes[6].mutable_tmpfs_node();
  tn0->set_tmpfs_options("option1");
  MountTree::TmpfsNode* tn1 = nodes[7].mutable_tmpfs_node();
  tn1->set_tmpfs_options("option2");
  MountTree::RootNode* rn0 = nodes[8].mutable_root_node();
  rn0->set_writable(false);
  MountTree::RootNode* rn1 = nodes[9].mutable_root_node();
  rn1->set_writable(true);

  // Compare same file nodes
  EXPECT_TRUE(internal::HasSameTarget(nodes[0], nodes[0]));
  // Compare almost same file nodes (ro vs rw)
  EXPECT_TRUE(internal::HasSameTarget(nodes[0], nodes[1]));
  // Compare different file nodes
  EXPECT_FALSE(internal::HasSameTarget(nodes[0], nodes[2]));
  // Compare file node with dir node
  EXPECT_FALSE(internal::HasSameTarget(nodes[0], nodes[3]));

  // Compare same dir nodes
  EXPECT_TRUE(internal::HasSameTarget(nodes[3], nodes[3]));
  // Compare almost same dir nodes (ro vs rw)
  EXPECT_TRUE(internal::HasSameTarget(nodes[3], nodes[4]));
  // Compare different dir nodes
  EXPECT_FALSE(internal::HasSameTarget(nodes[3], nodes[5]));
  // Compare dir node with tmpfs node
  EXPECT_FALSE(internal::HasSameTarget(nodes[3], nodes[6]));

  // Compare same tmpfs nodes
  EXPECT_TRUE(internal::HasSameTarget(nodes[6], nodes[6]));
  // Compare different tmpfs nodes
  EXPECT_FALSE(internal::HasSameTarget(nodes[6], nodes[7]));
  // Compare dir node with root node
  EXPECT_FALSE(internal::HasSameTarget(nodes[6], nodes[8]));

  // Compare same root nodes
  EXPECT_TRUE(internal::HasSameTarget(nodes[8], nodes[8]));
  // Compare almost same root nodes (ro vs rw)
  EXPECT_TRUE(internal::HasSameTarget(nodes[8], nodes[9]));
}

TEST(MountTreeTest, TestNodeEquivalence) {
  MountTree::Node nodes[8];
  MountTree::FileNode* fn0 = nodes[0].mutable_file_node();
  fn0->set_writable(false);
  fn0->set_outside("foo");
  MountTree::FileNode* fn1 = nodes[1].mutable_file_node();
  fn1->set_writable(false);
  fn1->set_outside("bar");
  MountTree::DirNode* dn0 = nodes[2].mutable_dir_node();
  dn0->set_writable(false);
  dn0->set_outside("foo");
  MountTree::DirNode* dn1 = nodes[3].mutable_dir_node();
  dn1->set_writable(false);
  dn1->set_outside("bar");
  MountTree::TmpfsNode* tn0 = nodes[4].mutable_tmpfs_node();
  tn0->set_tmpfs_options("option1");
  MountTree::TmpfsNode* tn1 = nodes[5].mutable_tmpfs_node();
  tn1->set_tmpfs_options("option2");
  MountTree::RootNode* rn0 = nodes[6].mutable_root_node();
  rn0->set_writable(false);
  MountTree::RootNode* rn1 = nodes[7].mutable_root_node();
  rn1->set_writable(true);

  for (const MountTree::Node& n : nodes) {
    ASSERT_TRUE(n.IsInitialized());
  }
  // Compare same file nodes
  EXPECT_TRUE(internal::IsEquivalentNode(nodes[0], nodes[0]));
  // Compare with different file node
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[0], nodes[1]));
  // compare file node with dir node
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[0], nodes[2]));

  // Compare same dir nodes
  EXPECT_TRUE(internal::IsEquivalentNode(nodes[2], nodes[2]));
  // Compare with different dir node
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[2], nodes[3]));
  // Compare dir node with tmpfs node
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[2], nodes[4]));

  // Compare same tmpfs nodes
  EXPECT_TRUE(internal::IsEquivalentNode(nodes[4], nodes[4]));
  // Compare with different tmpfs nodes
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[4], nodes[5]));
  // Compare tmpfs node with root node
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[4], nodes[6]));

  // Compare same root nodes
  EXPECT_TRUE(internal::IsEquivalentNode(nodes[6], nodes[6]));
  // Compare different root node
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[6], nodes[7]));
  // Compare root node with file node
  EXPECT_FALSE(internal::IsEquivalentNode(nodes[6], nodes[0]));
}

TEST(MountsResolvePathTest, Files) {
  Mounts mounts;
  ASSERT_THAT(mounts.AddFileAt("/A", "/a"), IsOk());
  ASSERT_THAT(mounts.AddFileAt("/B", "/d/b"), IsOk());
  ASSERT_THAT(mounts.AddFileAt("/C/D/E", "/d/c/e/f/h"), IsOk());
  std::string resolved;
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/a"));
  EXPECT_THAT(resolved, StrEq("/A"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/b"));
  EXPECT_THAT(resolved, StrEq("/B"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/c/e/f/h"));
  EXPECT_THAT(resolved, StrEq("/C/D/E"));
  ASSERT_THAT(mounts.ResolvePath("/f"), StatusIs(absl::StatusCode::kNotFound));
  ASSERT_THAT(mounts.ResolvePath("/d"), StatusIs(absl::StatusCode::kNotFound));
  ASSERT_THAT(mounts.ResolvePath("/d/c/e/f"),
              StatusIs(absl::StatusCode::kNotFound));
  ASSERT_THAT(mounts.ResolvePath("/d/d"),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(MountsResolvePathTest, Dirs) {
  Mounts mounts;
  ASSERT_THAT(mounts.AddDirectoryAt("/A", "/a"), IsOk());
  ASSERT_THAT(mounts.AddDirectoryAt("/B", "/d/b"), IsOk());
  ASSERT_THAT(mounts.AddDirectoryAt("/C/D/E", "/d/c/e/f/h"), IsOk());
  ASSERT_THAT(mounts.AddFileAt("/J/G/H", "/d/c/e/f/h/j"), IsOk());
  ASSERT_THAT(mounts.AddDirectoryAt("/K/L/M", "/d/c/e/f/h/k"), IsOk());
  std::string resolved;
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/a"));
  EXPECT_THAT(resolved, StrEq("/A"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/a/b/c/d/e"));
  EXPECT_THAT(resolved, StrEq("/A/b/c/d/e"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/b"));
  EXPECT_THAT(resolved, StrEq("/B"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/c/e/f/h"));
  EXPECT_THAT(resolved, StrEq("/C/D/E"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/c/e/f/h/i"));
  EXPECT_THAT(resolved, StrEq("/C/D/E/i"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/c/e/f/h/j"));
  EXPECT_THAT(resolved, StrEq("/J/G/H"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/c/e/f/h/k"));
  EXPECT_THAT(resolved, StrEq("/K/L/M"));
  SAPI_ASSERT_OK_AND_ASSIGN(resolved, mounts.ResolvePath("/d/c/e/f/h/k/a"));
  EXPECT_THAT(resolved, StrEq("/K/L/M/a"));
  ASSERT_THAT(mounts.ResolvePath("/f"), StatusIs(absl::StatusCode::kNotFound));
  ASSERT_THAT(mounts.ResolvePath("/d"), StatusIs(absl::StatusCode::kNotFound));
  ASSERT_THAT(mounts.ResolvePath("/d/c/e/f"),
              StatusIs(absl::StatusCode::kNotFound));
  ASSERT_THAT(mounts.ResolvePath("/d/d"),
              StatusIs(absl::StatusCode::kNotFound));
}

}  // namespace
}  // namespace sandbox2
