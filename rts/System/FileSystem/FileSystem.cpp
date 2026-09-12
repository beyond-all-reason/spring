/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

 /**
  * Glob conversion by Chris Han (based on work by Nathaniel Smith).
  */

#include "FileSystem.h"

#include <cassert>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <variant>

#include <fmt/printf.h>
#include <fmt/format.h>

#include "FileQueryFlags.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"

#include "System/SpringRegex.h"
#include "System/TimeUtil.h"
#include "System/Platform/Misc.h"
#include "System/Cpp17Compat.hpp"
#include "System/ForceInline.hpp"

#include <nowide/fstream.hpp>
#include <nowide/cstdio.hpp>

#ifdef _WIN32
	#include <windows.h>
	#include <winioctl.h> //needed for IsPathOnSpinningDisk()
	#include <nowide/convert.hpp>
	#include <io.h>

	// Win-API redefines these, which breaks things
	#if defined(DeleteFile)
		#undef DeleteFile
	#endif
	#if defined(CreateDirectory)
		#undef CreateDirectory
	#endif
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <dirent.h>
#endif // _WIN32

namespace fs = std::filesystem;

namespace Impl {
	RECOIL_FORCE_INLINE std::u8string StoreStringAsUTF8(const std::string& s) {
		return std::u8string(reinterpret_cast<const char8_t*>(s.c_str()), s.size());
	}
	RECOIL_FORCE_INLINE std::string StoreUTF8AsString(const std::u8string& utf8) {
		return std::string(reinterpret_cast<const char*>(utf8.c_str()));
	}
	RECOIL_FORCE_INLINE std::string StoreUTF8AsString(const std::u8string_view& utf8) {
		return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
	}
	RECOIL_FORCE_INLINE std::string StorePathAsString(const fs::path& path) {
		return StoreUTF8AsString(path.u8string());
	}
}

std::string FileSystem::RemoveLocalPathPrefix(const std::string& pStr)
{
	auto u8str = Recoil::filesystem::u8path(pStr).generic_u8string();

	if ((u8str.length() >= 2) && (u8str[0] == u8'.') && IsPathSeparator(u8str[1])) {
		u8str.erase(0, 2);
	}

	return Impl::StoreUTF8AsString(u8str);
}

bool FileSystem::IsFSRoot(const std::string& pStr)
{
	const auto p = Recoil::filesystem::u8path(pStr);
	const auto rp = p.root_path();
	return !rp.empty() && p == rp;
}

bool FileSystem::IsPathSeparator(char    aChar) { return ((aChar == cPS_WIN32) || (aChar == cPS_POSIX)); }
bool FileSystem::IsPathSeparator(char8_t wChar) { return ((wChar == cPS_WIN32) || (wChar == cPS_POSIX)); }
bool FileSystem::IsPathSeparator(wchar_t aChar) { return ((aChar == cPS_WIN32) || (aChar == cPS_POSIX)); }

bool FileSystem::HasPathSepAtEnd(const std::u8string& path)
{
	return !path.empty() && IsPathSeparator(path.at(path.size() - 1));
}

bool FileSystem::HasPathSepAtEnd(const std::string& pStr)
{
	const auto path = Recoil::filesystem::u8path(pStr).generic_u8string();
	return HasPathSepAtEnd(path);
}

std::string FileSystem::EnsurePathSepAtEnd(const std::string& pStr)
{
	auto path = Recoil::filesystem::u8path(pStr);
	path /= "";

	return Impl::StoreUTF8AsString(path.lexically_normal().generic_u8string());
}

std::string FileSystem::EnsurePathSepAtEnd(const std::u8string& pStr)
{
	auto path = std::filesystem::path(pStr);
	path /= "";

	return Impl::StorePathAsString(path.lexically_normal().generic_u8string());
}

std::string FileSystem::EnsureNoPathSepAtEnd(const std::string& pStr)
{
	auto path = Recoil::filesystem::u8path(pStr).lexically_normal().generic_u8string();

	if (HasPathSepAtEnd(path)) {
		path.resize(path.size() - 1);
	}

	return Impl::StoreUTF8AsString(path);
}

std::string FileSystem::StripTrailingSlashes(const std::string& pStr)
{
	auto path = Recoil::filesystem::u8path(pStr).generic_u8string();
	size_t len = path.length();

	while (len > 0) {
		if (IsPathSeparator(path.at(len - 1))) {
			--len;
		} else {
			break;
		}
	}

	return Impl::StoreUTF8AsString(path.substr(0, len));
}

std::string FileSystem::GetParent(const std::string& pathStr)
{
	const auto path = Recoil::filesystem::u8path(EnsureNoPathSepAtEnd(pathStr));
	if (!path.has_parent_path())
		return "";

	const auto ppath = path.parent_path();

	return EnsurePathSepAtEnd(ppath.generic_u8string());
}

int32_t FileSystem::GetFileSize(const std::string& fileStr)
{
	const auto file = Recoil::filesystem::u8path(fileStr);
	if (DirExists(file)) {
		LOG_L(L_WARNING, "[FSA::%s] error '%s' reading file size '%s'", __func__, "the file is directory", fileStr.c_str());
		return -1;
	}

	std::error_code ec;
	auto size = static_cast<int32_t>(fs::file_size(file, ec));
	if (ec) {
		LOG_L(L_WARNING, "[FSA::%s] error '%s' reading file size '%s'", __func__, ec.message().c_str(), fileStr.c_str());
		return -1;
	}

	return size;
}

bool FileSystem::IsReadableFile(const std::string& fileStr)
{
	const auto file = Recoil::filesystem::u8path(fileStr);

	// Exclude directories!
	if (!FileExists(file))
		return false;

	std::error_code ec;
	auto perms = fs::status(file, ec).permissions();

	if (ec)
		return false;

	return
		(perms & fs::perms::owner_read ) != fs::perms::none ||
		(perms & fs::perms::group_read ) != fs::perms::none ||
		(perms & fs::perms::others_read) != fs::perms::none;
}

uint32_t FileSystem::GetFileModificationTime(const std::string& file)
{
#ifdef _WIN32
	auto h = CreateFile(nowide::widen(file).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		LOG_L(L_WARNING, "[FSA::%s] error '%s' getting last modification time of file '%s'", __func__, Platform::GetLastErrorAsString().c_str(), file.c_str());
		return 0;
	}
	if (FILETIME ft; GetFileTime(h, nullptr, nullptr, &ft)) {
		CloseHandle(h);
		return static_cast<uint32_t>(CTimeUtil::NTFSTimeToTime64(ft.dwLowDateTime, ft.dwHighDateTime));
	}
	else {
		LOG_L(L_WARNING, "[FSA::%s] error '%s' getting last modification time of file '%s'", __func__, Platform::GetLastErrorAsString().c_str(), file.c_str());
		CloseHandle(h);
		return 0;
	}
#else
	struct stat info;

	if (stat(file.c_str(), &info) != 0) {
		LOG_L(L_WARNING, "[FSA::%s] error '%s' getting last modification time of file '%s'", __func__, strerror(errno), file.c_str());
		return 0;
	}

	return info.st_mtime;
#endif
}

std::string FileSystem::GetFileModificationDate(const std::string& file)
{
	const std::time_t t = GetFileModificationTime(file);

	if (t == 0)
		return "";

	const struct tm* clk = std::gmtime(&t);

	return fmt::sprintf("%d%02d%02d%02d%02d%02d", 1900 + clk->tm_year, clk->tm_mon + 1, clk->tm_mday, clk->tm_hour, clk->tm_min, clk->tm_sec);
}


bool FileSystem::IsPathOnSpinningDisk(const std::string& path)
{
#ifdef _WIN32
	std::wstring volumePath; volumePath.resize(64);
	if (!::GetVolumePathName(nowide::widen(path).c_str(), volumePath.data(), volumePath.size())) {
		LOG_L(L_WARNING, "[%s] GetVolumePathNameA error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
		return true;
	}

	std::wstring volumeName; volumeName.resize(1024);
	if (!::GetVolumeNameForVolumeMountPoint(volumePath.data(), volumeName.data(), volumeName.size())) {
		LOG_L(L_WARNING, "[%s] GetVolumeNameForVolumeMountPointA error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
		return true;
	}

	auto length = ::wcslen(volumeName.c_str());
	if (length && volumeName[length - 1] == L'\\')
		volumeName[length - 1] = L'\0';

	HANDLE volHandle = ::CreateFile(volumeName.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (volHandle == INVALID_HANDLE_VALUE) {
		LOG_L(L_WARNING, "[%s] CreateFileA error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
		return true;
	}

	STORAGE_PROPERTY_QUERY query{};
	query.PropertyId = StorageDeviceSeekPenaltyProperty;
	query.QueryType = PropertyStandardQuery;
	DWORD bytesWritten;
	DEVICE_SEEK_PENALTY_DESCRIPTOR result{};

	if (!::DeviceIoControl(volHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &result, sizeof(result), &bytesWritten, nullptr) || bytesWritten != sizeof(result))
	{
		LOG_L(L_WARNING, "[%s] DeviceIoControl error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
		CloseHandle(volHandle);
		return true;
	}
	CloseHandle(volHandle);
	return result.IncursSeekPenalty;
#else
	struct stat info;

	if (stat(path.c_str(), &info) != 0) {
		LOG_L(L_WARNING, "[%s] Error '%s' getting stat() for file '%s'", __func__, Platform::GetLastErrorAsString().c_str(), path.c_str());
		return true;
	}

	// the partition
	// devId: (info.st_dev >> 8) : (info.st_dev & 0xFF));

	// the physical disk
	// devId: (info.st_dev >> 8) : (0));

	// we need the physical disk
	std::string devName; devName.resize(1024);
	if (readlink(fmt::format("/sys/dev/block/{}:{}", info.st_dev >> 8, 0).c_str(), devName.data(), devName.size()) > 0) {
		devName.resize(strlen(devName.c_str()));
	}
	else {
		LOG_L(L_WARNING, "[%s] Error '%s' getting readlink() for file '%s'", __func__, Platform::GetLastErrorAsString().c_str(), path.c_str());
		return true;
	}

	auto ss = devName.rfind('/');
	if (ss == std::string::npos) {
		LOG_L(L_WARNING, "[%s] Error finding the device name for file '%s'", __func__, devName.c_str());
		return true;
	}

	devName = devName.substr(ss + 1);
	std::string rotFileName = fmt::format("/sys/block/{}/queue/rotational", devName);

	nowide::ifstream rotf(rotFileName, std::ios::in);
	if (rotf.bad() || !rotf.is_open()) {
		LOG_L(L_WARNING, "[%s] Error '%s' opening file '%s'", __func__, Platform::GetLastErrorAsString().c_str(), rotFileName.c_str());
		return true;
	}

	char rot = '\0';
	if (!rotf.read(&rot, 1)) {
		LOG_L(L_WARNING, "[%s] Error '%s' reading file '%s'", __func__, Platform::GetLastErrorAsString().c_str(), rotFileName.c_str());
		return true;
	}
	return rot == '1';
#endif
}

char FileSystem::GetNativePathSeparator()
{
	#ifndef _WIN32
	return '/';
	#else
	return '\\';
	#endif
}

bool FileSystem::IsAbsolutePath(const std::string& pathStr)
{
#if 1
	auto path = Recoil::filesystem::u8path(pathStr);
	return path.is_absolute();
#else
	#ifdef _WIN32
		return ((pathStr.length() > 1) && (pathStr[1] == ':'));
	#else
		return ((pathStr.length() > 0) && (pathStr[0] == '/'));
	#endif
#endif
}


/**
 * @brief creates a rwxr-xr-x dir in the writedir
 *
 * Returns true if the postcondition of this function is that dir exists in
 * the write directory.
 *
 * Note that this function does not check access to the dir, ie. if you've
 * created it manually with 0000 permissions then this function may return
 * true, subsequent operation on files inside the directory may still fail.
 *
 * As a rule of thumb, set identical permissions on identical items in the
 * data directory, ie. all subdirectories the same perms, all files the same
 * perms.
 */
bool FileSystem::MkDir(const std::string& dirStr)
{
	auto dir = Recoil::filesystem::u8path(dirStr);
	if (!CheckFile(dir))
		return false;

	// First check if directory exists. We'll return success if it does.
	if (DirExists(dir))
		return true;

	std::error_code ec;
	fs::create_directory(dir, ec);
	if (ec) {
		LOG_L(L_WARNING, "[FSA::%s] error '%s' creating directory '%s'", __func__, ec.message().c_str(), dirStr.c_str());
		return false;
	}

	// Set permissions to rwxr-xr-x
	fs::permissions(
		dir,
		fs::perms::owner_all  |   // rwx for owner
		fs::perms::group_read |   // r-- for group
		fs::perms::group_exec |   // --x for group (combined makes r-x)
		fs::perms::others_read |  // r-- for others
		fs::perms::others_exec,   // --x for others (combined makes r-x)
		fs::perm_options::replace
	);

	return true;
}

bool FileSystem::DeleteFile(const std::string& fileStr)
{
	auto file = Recoil::filesystem::u8path(fileStr);

	std::error_code ec;
	return (fs::remove(file, ec) && !ec);

}

bool FileSystem::FileExists(const fs::path& path)
{
	return fs::exists(path) && !fs::is_directory(path);
}

bool FileSystem::FileExists(const std::string& fileStr)
{
	auto file = Recoil::filesystem::u8path(FileSystem::GetNormalizedPath(fileStr));
	return FileExists(file);
}

bool FileSystem::DirExists(const fs::path& dir)
{
	return fs::exists(dir) && fs::is_directory(dir);
}

bool FileSystem::DirExists(const std::string& dirStr)
{
	auto dir = Recoil::filesystem::u8path(dirStr);
	return DirExists(dir);
}


bool FileSystem::DirIsWritable(const std::string& dirStr)
{
#if 1
	auto dir = Recoil::filesystem::u8path(dirStr);
	if (!DirExists(dir))
		return false;

	std::error_code ec;
	auto perms = fs::status(dir, ec).permissions();
	if (ec)
		return false;

	return
		(perms & fs::perms::owner_write ) != fs::perms::none ||
		(perms & fs::perms::group_write ) != fs::perms::none ||
		(perms & fs::perms::others_write) != fs::perms::none;
#else
	#ifdef _WIN32
		// this exists because _access does not do the right thing
		// see http://msdn.microsoft.com/en-us/library/1w06ktdy(VS.71).aspx
		// for now, try to create a temporary file in a directory and open it
		// to rule out the possibility of it being created in the virtual store
		// TODO perhaps use SECURITY_DESCRIPTOR winapi calls here

		std::string testfile = dirStr + "\\__$testfile42$.test";
		nowide::ofstream os(testfile.c_str());

		if (os.fail())
			return false;

		const char* testdata = "THIS IS A TEST";
		os << testdata;
		os.close();

		// this part should only be needed when there is no manifest embedded
		nowide::ifstream is(testfile.c_str());

		if (is.fail())
			return false; // the file most likely exists in the virtual store

		std::string input;
		getline(is, input);

		if (input != testdata) {
			unlink(testfile.c_str());
			return false;
		}

		is.close();
		unlink(testfile.c_str());
		return true;
	#else
		return (access(dir.c_str(), W_OK) == 0);
	#endif
#endif
}

bool FileSystem::ComparePaths(const std::string& path1, const std::string& path2)
{
	const auto nPath1 = GetNormalizedPath(path1);
	const auto nPath2 = GetNormalizedPath(path2);
#ifndef _WIN32
	struct stat info1, info2;
	const int ret1 = stat(nPath1.c_str(), &info1);
	const int ret2 = stat(nPath2.c_str(), &info2);

	// If either files doesn't exist, return false
	if (ret1 || ret2)
		return false;

	return (info1.st_dev == info2.st_dev) && (info1.st_ino == info2.st_ino);
#else
	HANDLE h1 = CreateFile(
		nowide::widen(nPath1).c_str(),
		0,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		0);

	if (h1 == INVALID_HANDLE_VALUE)
		return false;

	HANDLE h2 = CreateFile(
		nowide::widen(nPath2).c_str(),
		0,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		0);

	if (h2 == INVALID_HANDLE_VALUE) {
		CloseHandle(h1);
		return false;
	}

	BY_HANDLE_FILE_INFORMATION info1, info2;
	if (!GetFileInformationByHandle(h1, &info1)) {
		CloseHandle(h1);
		CloseHandle(h2);
		return false;
	}

	if (!GetFileInformationByHandle(h2, &info2)) {
		CloseHandle(h1);
		CloseHandle(h2);
		return false;
	}

	CloseHandle(h1);
	CloseHandle(h2);

	return
		info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber &&
		info1.nFileIndexHigh == info2.nFileIndexHigh &&
		info1.nFileIndexLow == info2.nFileIndexLow &&
		info1.nFileSizeHigh == info2.nFileSizeHigh &&
		info1.nFileSizeLow == info2.nFileSizeLow &&
		info1.ftLastWriteTime.dwLowDateTime == info2.ftLastWriteTime.dwLowDateTime &&
		info1.ftLastWriteTime.dwHighDateTime == info2.ftLastWriteTime.dwHighDateTime;
#endif
}


std::string FileSystem::GetEngineExecutableDir()
{
	return GetDirectory(Platform::GetProcessExecutableFile());
}

std::string FileSystem::GetCwd()
{
	return Impl::StorePathAsString(fs::current_path());
}

void FileSystem::ChDir(const std::string& dirStr)
{
	auto dir = Recoil::filesystem::u8path(dirStr);
	fs::current_path(dir); //setting path
}

namespace Impl {
	void FindFiles(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const spring::regex& regexPattern, int flags)
	{
	#ifdef _WIN32
		WIN32_FIND_DATA wfd;
		HANDLE hFind = FindFirstFile(nowide::widen(datadir + dir + "\\*").c_str(), &wfd);

		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				const auto cFileName = nowide::narrow(wfd.cFileName);
				if (cFileName != "." && cFileName != "..") {
					if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						if ((flags & FileQueryFlags::ONLY_DIRS) == 0) {
							if (spring::regex_match(cFileName, regexPattern)) {
								matches.push_back(dir + cFileName);
							}
						}
					}
					else {
						if (flags & FileQueryFlags::INCLUDE_DIRS) {
							if (spring::regex_match(cFileName, regexPattern)) {
								matches.push_back(dir + cFileName + "\\");
							}
						}
						if (flags & FileQueryFlags::RECURSE) {
							FindFiles(matches, datadir, dir + cFileName + "\\", regexPattern, flags);
						}
					}
				}
			} while (FindNextFile(hFind, &wfd));
			FindClose(hFind);
		}
	#else
		DIR* dp;
		struct dirent* ep;

		if ((dp = opendir((datadir + dir).c_str())) == nullptr)
			return;

		while ((ep = readdir(dp))) {
			// exclude hidden files
			if (ep->d_name[0] == '.')
				continue;

			// is it a file? (we just treat sockets / pipes / fifos / character&block devices as files...)
			// (need to stat because d_type is DT_UNKNOWN on linux :-/)
			struct stat info;
			if (stat((datadir + dir + ep->d_name).c_str(), &info) != 0)
				continue;

			if (!S_ISDIR(info.st_mode)) {
				if ((flags & FileQueryFlags::ONLY_DIRS) == 0) {
					if (spring::regex_match(ep->d_name, regexPattern)) {
						matches.push_back(dir + ep->d_name);
					}
				}
			}
			else {
				// or a directory?
				if (flags & FileQueryFlags::INCLUDE_DIRS) {
					if (spring::regex_match(ep->d_name, regexPattern)) {
						matches.push_back(dir + ep->d_name + "/");
					}
				}
				if (flags & FileQueryFlags::RECURSE) {
					FindFiles(matches, datadir, dir + ep->d_name + "/", regexPattern, flags);
				}
			}
		}

		closedir(dp);
	#endif
	}

	void FindFilesStd(std::vector<std::string>& matches, const std::string& dataDir, const std::string& dirStr, const spring::regex& regexPattern, int flags)
	{
		const std::string dirFullStr = FileSystem::ForwardSlashes(dataDir + dirStr);

		const fs::path dirFullPath = Recoil::filesystem::u8path(dirFullStr);
		if (!fs::exists(dirFullPath))
			return;

		// Each match is `dirStr + <entry below dirStr>`; the dataDir prefix must not leak
		// in, so prepend dirStr ourselves instead of emitting the iterated full path.
		const std::string dirRelPrefix = FileSystem::ForwardSlashes(dirStr);

		std::variant<fs::directory_iterator, fs::recursive_directory_iterator> dirIterator;
		if ((flags & FileQueryFlags::RECURSE) != 0)
			dirIterator = fs::recursive_directory_iterator(dirFullPath);
		else
			dirIterator = fs::directory_iterator(dirFullPath);

		std::visit([&](auto&& dirIterator) {
			for (const fs::directory_entry& entry : dirIterator) {
				const bool isDir = fs::is_directory(entry);

				// need directory, but it's not a directory
				if ((flags & FileQueryFlags::ONLY_DIRS   ) != 0 && !isDir)
					continue;

				// don't need directories, but it's a directory
				if ((flags & FileQueryFlags::INCLUDE_DIRS) == 0 &&  isDir)
					continue;

				// hope std::regex_match will not trip up on UTF-8, if it does, will need to convert to std::wregex
				// the previous implementation relied on checking the filename only
				const std::u8string entryPathFnStr = entry.path().filename().generic_u8string();

				if (spring::regex_match(StoreUTF8AsString(entryPathFnStr), regexPattern)) {
					const std::u8string entryRelStr = entry.path().lexically_relative(dirFullPath).generic_u8string();
					std::string entryPathStr = dirRelPrefix + Impl::StoreUTF8AsString(entryRelStr);

					// the previous convention to add a trailing slash
					if (isDir && !entryPathStr.empty() && entryPathStr.back() != '/') {
						entryPathStr += '/';
					}
					matches.emplace_back(std::move(entryPathStr));
				}
			}
		}, std::move(dirIterator));
	}
}

void FileSystem::FindFiles(std::vector<std::string>& matches, const std::string& dataDir, const std::string& dir, const std::string& regex, int flags)
{
	const spring::regex regexPattern(regex);
#if 1
	Impl::FindFilesStd(matches, dataDir, dir, regexPattern, flags);
#else
	Impl::FindFiles   (matches, dataDir, dir, regexPattern, flags);
#endif
}

/**
 * @brief quote macro
 * @param c Character to test
 * @param str string currently being built
 *
 * Given an std::string str that we are assembling,
 * and an upcoming char c, will append
 * an extra '\\' to quote the character if necessary.
 * The do-while is used for legalizing the ';' in "QUOTE(c, regex);".
 */
#define QUOTE(c,str)			\
	do {					\
		if (!(isalnum(c) || (c) == '_'))	\
			str += '\\';		\
		str += c;				\
	} while (0)

std::string FileSystem::ConvertGlobToRegex(const std::string& glob)
{
	std::string regex;
	regex.reserve(glob.size() << 1);
	int braces = 0;
	for (std::string::const_iterator i = glob.begin(); i != glob.end(); ++i) {
		char c = *i;
#ifdef DEBUG
		if (braces >= 5) {
			LOG_L(L_WARNING, "%s: braces nested too deeply\n%s", __FUNCTION__, glob.c_str());
		}
#endif
		switch (c) {
			case '*':
				regex += ".*";
				break;
			case '?':
				regex += '.';
				break;
			case '{':
				braces++;
				regex += '(';
				break;
			case '}':
#ifdef DEBUG
				if (braces == 0) {
					LOG_L(L_WARNING, "%s: closing brace without an equivalent opening brace\n%s", __FUNCTION__, glob.c_str());
				}
#endif
				regex += ')';
				braces--;
				break;
			case ',':
				if (braces > 0) {
					regex += '|';
				} else {
					QUOTE(c, regex);
				}
				break;
			case '\\':
				++i;
#ifdef DEBUG
				if (i == glob.end()) {
					LOG_L(L_WARNING, "%s: pattern ends with backslash\n%s", __FUNCTION__, glob.c_str());
				}
#endif
				QUOTE(*i, regex);
				break;
			default:
				QUOTE(c, regex);
				break;
		}
	}

#ifdef DEBUG
	if (braces > 0) {
		LOG_L(L_WARNING, "%s: unterminated brace expression\n%s", __FUNCTION__, glob.c_str());
	} else if (braces < 0) {
		LOG_L(L_WARNING, "%s: too many closing braces\n%s", __FUNCTION__, glob.c_str());
	}
#endif

	return regex;
}

#undef QUOTE

std::string FileSystem::Concatenate(const std::initializer_list<std::string_view>& list)
{
	std::filesystem::path p;
	for (const auto& li : list) {
		p /= Recoil::filesystem::u8path(li);
	}

	return Impl::StorePathAsString(p);
}


std::filesystem::path FileSystem::ForwardSlashes(const std::filesystem::path& path)
{
	auto u8path = path.generic_u8string();
	std::replace_if(u8path.begin(), u8path.end(), [](auto ch) { return ch == u8'\\'; }, u8'/');
	return std::filesystem::path(u8path);
}

std::string FileSystem::ForwardSlashes(const std::string& path)
{
	auto u8path = Recoil::filesystem::u8path(path).generic_u8string();
	std::replace_if(u8path.begin(), u8path.end(), [](auto ch) { return ch == u8'\\'; }, u8'/');

	return Impl::StoreUTF8AsString(u8path);
}

std::string FileSystem::NativeSlashes(const std::string& path)
{
	auto u8path = Recoil::filesystem::u8path(path).generic_u8string();
	std::replace_if(u8path.begin(), u8path.end(), [](auto ch) { return IsPathSeparator(ch); }, cPS);

	return Impl::StoreUTF8AsString(u8path);
}

bool FileSystem::CreateDirectory(const std::string& dirStr)
{
	auto dir = Recoil::filesystem::u8path(dirStr);
	if (!CheckFile(dir))
		return false;

	// First check if directory exists. We'll return success if it does.
	if (DirExists(dir))
		return true;

	std::error_code ec;
	fs::create_directories(dir, ec);
	if (ec) {
		LOG_L(L_WARNING, "[FS::%s] error '%s' creating directory '%s'", __func__, ec.message().c_str(), dirStr.c_str());
		return false;
	}

	// Set permissions to rwxr-xr-x
	fs::permissions(
		dir,
		fs::perms::owner_all |   // rwx for owner
		fs::perms::group_read |   // r-- for group
		fs::perms::group_exec |   // --x for group (combined makes r-x)
		fs::perms::others_read |  // r-- for others
		fs::perms::others_exec,   // --x for others (combined makes r-x)
		fs::perm_options::replace,
		ec
	);
	if (ec) {
		LOG_L(L_WARNING, "[FS::%s] error '%s' creating directory '%s'", __func__, ec.message().c_str(), dirStr.c_str());
		return false;
	}

	return true;
}


bool FileSystem::TouchFile(const std::string& filePathStr)
{
	const auto filePath = Recoil::filesystem::u8path(filePathStr);
	if (!CheckFile(filePath))
		return false;

	// Try to create/open the file
	nowide::fstream file(filePath, std::ios::in | std::ios::out | std::ios::app | std::ios::binary);
	if (!file.is_open())
		return false;

	file.close();

	// Check for read access again
	if (fs::exists(filePath)) {
		auto perms = fs::status(filePath).permissions();
		return (perms & fs::perms::owner_read) != fs::perms::none;
	}

	return false;
}


std::string FileSystem::GetDirectory(const std::string& pStr)
{
	auto u8str = Recoil::filesystem::u8path(pStr).generic_u8string();

	const size_t s = u8str.find_last_of(u8"\\/");

	if (s != std::string::npos)
		return Impl::StoreUTF8AsString(u8str.substr(0, s + 1));

	return ""; // XXX return "./"? (a short test caused a crash because CFileHandler used in Lua couldn't find a file in the base-dir)
}

std::string FileSystem::GetFilename(const std::string& pathStr)
{
	const auto p = Recoil::filesystem::u8path(pathStr);
	return Impl::StorePathAsString(p.filename());
}

std::string FileSystem::GetBasename(const std::string& pathStr)
{
	const auto p = Recoil::filesystem::u8path(pathStr);
	return Impl::StorePathAsString(p.stem());
}

std::string FileSystem::GetExtensionLowerCase(const std::string& pathStr)
{
	const auto p = Recoil::filesystem::u8path(pathStr);
	auto ext = p.extension().generic_u8string();
	if (ext.empty())
		return "";

	assert(ext[0] == u8'.');
	return StringToLower(Impl::StorePathAsString(ext.substr(1, ext.length() - 1)));
}

std::string FileSystem::GetNormalizedPath(const std::string& path) {
	return Impl::StoreUTF8AsString(std::filesystem::path(ForwardSlashes(path)).lexically_normal().generic_u8string());
}

bool FileSystem::CheckFile(const std::string& file)
{
	const auto p = Recoil::filesystem::u8path(file);
	return CheckFile(p);
}

bool FileSystem::CheckFile(const std::filesystem::path& p)
{
	// Don't allow code to escape from the data directories.
	// Note: this does NOT mean this is a SAFE fopen function:
	// symlink-, hardlink-, you name it-attacks are all very well possible.
	// The check is just meant to "enforce" certain coding behaviour.
	//
	return p.generic_u8string().find(u8"..") == std::string::npos;
}

bool FileSystem::Remove(const std::string& file)
{
	if (!CheckFile(file))
		return false;

	return FileSystem::DeleteFile(FileSystem::GetNormalizedPath(file));
}

const std::string& FileSystem::GetCacheDir()
{
	static const std::string cacheBaseDir = "cache";
	return cacheBaseDir;
}