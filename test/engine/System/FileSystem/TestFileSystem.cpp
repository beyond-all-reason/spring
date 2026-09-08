#include <algorithm>
#include <string>
#include <vector>
#include <nowide/cstdio.hpp>
#include <sys/stat.h>
#include "System/Log/ILog.h"

#include <catch_amalgamated.hpp>

// needs to be included after catch
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"

namespace {
	struct PrepareFileSystem {
		PrepareFileSystem() {
			oldDir = FileSystem::GetCwd();
			testCwd = GetTempDir();
			if (!testCwd.empty()) {
				FileSystem::ChDir(testCwd);

				// create files and dirs to test
				FileSystem::CreateDirectory("testDir");
				WriteFile("testFile.txt", "a");
			}
		}
		~PrepareFileSystem() {
			if (!testCwd.empty()) {
				// delete files and dirs created in the ctor
				FileSystem::DeleteFile("testFile.txt");
				FileSystem::DeleteFile("testDir");
			}

			FileSystem::ChDir(oldDir);
			if (!testCwd.empty()) {
				FileSystem::DeleteFile(testCwd);
			}
		}
		static void WriteFile(const std::string& filePath, const std::string& content) {
			FILE* testFile = nowide::fopen(filePath.c_str(), "w");
			if (testFile != nullptr) {
				fprintf(testFile, "%s", content.c_str());
				fclose(testFile);
				testFile = nullptr;
			} else {
				FAIL("Failed to create test-file " + filePath);
			}
		}
		std::string GetTempDir() {
			if (testCwd.empty()) {
				char* tmpDir = std::tmpnam(nullptr);
				if (tmpDir != nullptr) {
					testCwd = tmpDir;
					FileSystem::CreateDirectory(testCwd);
					if (!FileSystem::DirIsWritable(testCwd)) {
						FAIL("Failed to create temporary test dir");
					}
				} else {
					FAIL("Failed to get temporary file name");
				}
			}
			return testCwd;
		}

		std::string testCwd;
	private:
		std::string oldDir;
	};
}

PrepareFileSystem pfs;

TEST_CASE("FileExists")
{
	CHECK(FileSystem::FileExists(u8"testFile.txt"));
	CHECK_FALSE(FileSystem::FileExists(u8"testFile99.txt"));
	CHECK_FALSE(FileSystem::FileExists(u8"testDir"));
	CHECK_FALSE(FileSystem::FileExists(u8"testDir99"));
}


TEST_CASE("GetFileSize")
{
	CHECK(FileSystem::GetFileSize("testFile.txt") == 1);
	CHECK(FileSystem::GetFileSize("testFile99.txt") == -1);
	CHECK(FileSystem::GetFileSize("testDir") == -1);
	CHECK(FileSystem::GetFileSize("testDir99") == -1);
}


TEST_CASE("GetFileModificationDate")
{
	CHECK(FileSystem::GetFileModificationDate("testDir") != "");
	CHECK(FileSystem::GetFileModificationDate("testFile.txt") != "");
	CHECK(FileSystem::GetFileModificationDate("not_there") == "");
}


TEST_CASE("CreateDirectory")
{
	// create & exists
	CHECK(FileSystem::DirIsWritable("./"));
	CHECK(FileSystem::DirExists(u8"testDir"));
	CHECK(FileSystem::DirExists(u8"testDir///"));
	CHECK(FileSystem::DirExists(u8"testDir////./"));
	CHECK(FileSystem::ComparePaths("testDir", "testDir////./"));
	CHECK_FALSE(FileSystem::ComparePaths("testDir", "test Dir2"));
	CHECK(FileSystem::CreateDirectory("testDir")); // already exists
	CHECK(FileSystem::CreateDirectory("testDir1")); // should be created
	CHECK(FileSystem::CreateDirectory("test Dir2")); // should be created

	// check if exists & no overwrite
	CHECK(FileSystem::CreateDirectory("test Dir2")); // already exists
	CHECK(FileSystem::DirIsWritable("test Dir2"));
	CHECK_FALSE(FileSystem::CreateDirectory("testFile.txt")); // file with this name already exists

	// delete temporaries
	CHECK(FileSystem::DeleteFile("testDir1"));
	CHECK(FileSystem::DeleteFile("test Dir2"));

	// check if really deleted
	CHECK_FALSE(FileSystem::DirExists(u8"testDir1"));
	CHECK_FALSE(FileSystem::DirExists(u8"test Dir2"));
}


TEST_CASE("GetDirectory")
{
#define CHECK_DIR_EXTRACTION(path, dir) \
		CHECK(FileSystem::GetDirectory(path) == dir)

	CHECK_DIR_EXTRACTION("testFile.txt", "");
	CHECK_DIR_EXTRACTION("./foo/testFile.txt", "./foo/");

#undef CHECK_DIR_EXTRACTION
}


TEST_CASE("GetExtensionLowerCase")
{
	CHECK(FileSystem::GetExtensionLowerCase("SCRIPT.COB") == "cob");
}


#define CHECK_NORM_PATH(path, normPath) \
		CHECK(FileSystem::GetNormalizedPath(path) == normPath)

TEST_CASE("GetNormalizedPath - basic paths") 
{
	CHECK_NORM_PATH("foo/bar", "foo/bar");
	CHECK_NORM_PATH("foo\\bar", "foo/bar");
	CHECK_NORM_PATH("/foo/bar", "/foo/bar");
	CHECK_NORM_PATH("C:/foo/bar", "C:/foo/bar");
}

TEST_CASE("GetNormalizedPath - multiple slashes") 
{
	CHECK_NORM_PATH("foo///bar", "foo/bar");
	CHECK_NORM_PATH("foo\\\\\\bar", "foo/bar");
	CHECK_NORM_PATH("//foo//bar//", "/foo/bar/");
	CHECK_NORM_PATH("C:\\\\foo\\\\bar", "C:/foo/bar");
}

TEST_CASE("GetNormalizedPath - current directory") 
{
	CHECK_NORM_PATH("./foo/bar", "foo/bar");
	CHECK_NORM_PATH(".\\foo\\bar", "foo/bar");
	CHECK_NORM_PATH("foo/./bar", "foo/bar");
	CHECK_NORM_PATH("foo/.", "foo/");
	CHECK_NORM_PATH(".", ".");
	CHECK_NORM_PATH("./", ".");
	CHECK_NORM_PATH("./.", ".");
}

TEST_CASE("GetNormalizedPath - parent directory") 
{
	CHECK_NORM_PATH("foo/bar/..", "foo/");
	CHECK_NORM_PATH("foo/bar/../baz", "foo/baz");
	CHECK_NORM_PATH("foo/../bar", "bar");
	CHECK_NORM_PATH("./foo/../bar", "bar");
	CHECK_NORM_PATH("foo/bar/../../baz", "baz");
	CHECK_NORM_PATH("../foo", "../foo");
	CHECK_NORM_PATH("../../foo", "../../foo");
	CHECK_NORM_PATH("..", "..");
}

TEST_CASE("GetNormalizedPath - mixed cases") 
{
	CHECK_NORM_PATH("./foo/./bar/../baz", "foo/baz");
	CHECK_NORM_PATH("foo//./bar//..//baz", "foo/baz");
	CHECK_NORM_PATH("./a/b/c/../../d", "a/d");
	CHECK_NORM_PATH("C:\\foo\\.\\bar\\..\\baz", "C:/foo/baz");
}

TEST_CASE("GetNormalizedPath - Windows drives") 
{
	CHECK_NORM_PATH("C:/", "C:/");
	CHECK_NORM_PATH("C:\\", "C:/");
	CHECK_NORM_PATH("D:\\foo\\bar", "D:/foo/bar");
	CHECK_NORM_PATH("C:/foo/../bar", "C:/bar");
}

TEST_CASE("GetNormalizedPath - absolute paths") 
{
	CHECK_NORM_PATH("/", "/");
	CHECK_NORM_PATH("/foo", "/foo");
	CHECK_NORM_PATH("/foo/../bar", "/bar");
	CHECK_NORM_PATH("/foo/./bar", "/foo/bar");
}

TEST_CASE("GetNormalizedPath - trailing slashes") 
{
	CHECK_NORM_PATH("foo/bar/", "foo/bar/");
	CHECK_NORM_PATH("foo/bar//", "foo/bar/");
	CHECK_NORM_PATH("./foo/", "foo/");
	CHECK_NORM_PATH("foo\\bar\\", "foo/bar/");
	CHECK_NORM_PATH("foo\\bar\\\\", "foo/bar/");
	CHECK_NORM_PATH(".\\foo\\", "foo/");
	CHECK_NORM_PATH("C:\\foo\\", "C:/foo/");
}

TEST_CASE("GetNormalizedPath - with file extensions") 
{
	CHECK_NORM_PATH("./foo/bar.txt", "foo/bar.txt");
	CHECK_NORM_PATH("foo/../bar.log", "bar.log");
	CHECK_NORM_PATH("./a/b/../c.txt", "a/c.txt");
}

TEST_CASE("GetNormalizedPath - UTF-8 support") 
{
	CHECK_NORM_PATH("./文档/测试.txt", "文档/测试.txt");
	CHECK_NORM_PATH("папка/файл.log", "папка/файл.log");
	CHECK_NORM_PATH("./日本語/../テスト", "テスト");
	CHECK_NORM_PATH("C:\\français\\café\\..\\thé.txt", "C:/français/thé.txt");
	CHECK_NORM_PATH("./مجلد/ملف.txt", "مجلد/ملف.txt");
}

TEST_CASE("GetNormalizedPath - spaces") 
{
	CHECK_NORM_PATH("foo bar/baz", "foo bar/baz");
	CHECK_NORM_PATH("./my folder/test.txt", "my folder/test.txt");
	CHECK_NORM_PATH("C:\\Program Files\\app", "C:/Program Files/app");
}

TEST_CASE("GetNormalizedPath - special characters") 
{
	CHECK_NORM_PATH("foo-bar_baz", "foo-bar_baz");
	CHECK_NORM_PATH("./file (1).txt", "file (1).txt");
	CHECK_NORM_PATH("foo@bar/baz#123", "foo@bar/baz#123");
}

TEST_CASE("GetNormalizedPath - edge cases with .. at boundaries") 
{
	CHECK_NORM_PATH("./..", "..");
	CHECK_NORM_PATH("foo/..", ".");
	CHECK_NORM_PATH("foo/../..", "..");
	CHECK_NORM_PATH("./foo/bar/../../..", "..");
}

TEST_CASE("GetNormalizedPath - original failing tests")
{
	CHECK_NORM_PATH("/home/userX/.spring/foo/bar///./../test.log", "/home/userX/.spring/foo/test.log");
	CHECK_NORM_PATH("./symLinkToHome/foo/bar///./../test.log", "symLinkToHome/foo/test.log");
	CHECK_NORM_PATH(".\\symLinkToHome\\foo\\bar\\\\\\.\\..\\test.log", "symLinkToHome/foo/test.log");
	CHECK_NORM_PATH("C:\\foo\\bar\\\\\\.\\..\\test.log", "C:/foo/test.log");
}

#undef CHECK_NORM_PATH


// Regression test for commit c3b8b6397a (#2235): the std::filesystem-based
// FindFiles implementation was emitting absolute paths because it pushed the
// iterated entry path verbatim (including the dataDir prefix). The contract is
// that matches are relative to dataDir, i.e. `dir + <entry below dir>` only.
// This is what made VFS.DirList / VFS.SubDirs (raw mode) return absolute paths.
TEST_CASE("FindFiles - matches are relative to the data dir")
{
	// dataDir is the search root that must NOT appear in the results
	const std::string dataDir = FileSystem::EnsurePathSepAtEnd(FileSystem::ForwardSlashes(pfs.testCwd));

	// build a small tree under the data dir
	REQUIRE(FileSystem::CreateDirectory("findDir"));
	REQUIRE(FileSystem::CreateDirectory("findDir/sub"));
	PrepareFileSystem::WriteFile("findDir/a.txt",     "a");
	PrepareFileSystem::WriteFile("findDir/b.lua",     "b");
	PrepareFileSystem::WriteFile("findDir/sub/c.txt", "c");

	const std::string anyRegex = FileSystem::ConvertGlobToRegex("*");
	const std::string txtRegex = FileSystem::ConvertGlobToRegex("*.txt");

	SECTION("files, non-recursive") {
		std::vector<std::string> matches;
		FileSystem::FindFiles(matches, dataDir, "findDir/", anyRegex, 0);
		std::sort(matches.begin(), matches.end());

		// the dataDir prefix must not leak into the results
		for (const std::string& m: matches)
			CHECK(m.rfind(dataDir, 0) != 0);

		REQUIRE(matches.size() == 2);
		CHECK(matches[0] == "findDir/a.txt");
		CHECK(matches[1] == "findDir/b.lua");
	}

	SECTION("pattern is honoured") {
		std::vector<std::string> matches;
		FileSystem::FindFiles(matches, dataDir, "findDir/", txtRegex, 0);

		REQUIRE(matches.size() == 1);
		CHECK(matches[0] == "findDir/a.txt");
	}

	SECTION("recursive descends but keeps paths relative") {
		std::vector<std::string> matches;
		FileSystem::FindFiles(matches, dataDir, "findDir/", anyRegex, FileQueryFlags::RECURSE);
		std::sort(matches.begin(), matches.end());

		REQUIRE(matches.size() == 3);
		CHECK(matches[0] == "findDir/a.txt");
		CHECK(matches[1] == "findDir/b.lua");
		CHECK(matches[2] == "findDir/sub/c.txt");
	}

	SECTION("dirs only, with trailing slash") {
		std::vector<std::string> matches;
		FileSystem::FindFiles(matches, dataDir, "findDir/", anyRegex,
			FileQueryFlags::ONLY_DIRS | FileQueryFlags::INCLUDE_DIRS);

		REQUIRE(matches.size() == 1);
		CHECK(matches[0] == "findDir/sub/");
	}

	SECTION("absolute lookup (empty dataDir) stays absolute") {
		// the absolute-path branch passes dataDir="" and dir=<absolute path>;
		// results should remain absolute, prefixed by the requested dir
		const std::string absDir = dataDir + "findDir/";
		std::vector<std::string> matches;
		FileSystem::FindFiles(matches, "", absDir, txtRegex, 0);

		REQUIRE(matches.size() == 1);
		CHECK(matches[0] == absDir + "a.txt");
	}

	// cleanup (bottom-up: files before their dirs)
	FileSystem::DeleteFile("findDir/sub/c.txt");
	FileSystem::DeleteFile("findDir/a.txt");
	FileSystem::DeleteFile("findDir/b.lua");
	FileSystem::DeleteFile("findDir/sub");
	FileSystem::DeleteFile("findDir");
}