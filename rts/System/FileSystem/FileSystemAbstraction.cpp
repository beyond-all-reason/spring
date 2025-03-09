/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(_MSC_VER) && !defined(S_ISDIR)
#	define S_ISDIR(m) (((m) & 0170000) == 0040000)
#endif

#include "FileSystemAbstraction.h"

#include <cassert>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <filesystem>

#include <fmt/printf.h>
#include <fmt/format.h>

#include "FileQueryFlags.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"

#include "System/SpringRegex.h"
#include "System/TimeUtil.h"
#include "System/Platform/Misc.h"

#ifndef _WIN32
	#include <dirent.h>
	#include <sstream>
	#include <unistd.h>
	#include <ctime>
	#include <fstream>
#else
	#include <windows.h>
	#include <io.h>
	#include <direct.h>
	#include <fstream>
	#include <winioctl.h>
	// Win-API redefines these, which breaks things
	#if defined(CreateDirectory)
		#undef CreateDirectory
	#endif
	#if defined(DeleteFile)
		#undef DeleteFile
	#endif
#endif



std::string FileSystemAbstraction::RemoveLocalPathPrefix(const std::string& path)
{
	std::string p(path);

	if ((p.length() >= 2) && (p[0] == '.') && IsPathSeparator(p[1])) {
	    p.erase(0, 2);
	}

	return p;
}

bool FileSystemAbstraction::IsFSRoot(const std::string& p)
{

#ifdef _WIN32
	// examples: "C:\", "C:/", "C:", "c:", "D:"
	bool isFsRoot = (p.length() >= 2 && p[1] == ':' &&
			((p[0] >= 'a' && p[0] <= 'z') || (p[0] >= 'A' && p[0] <= 'Z')) &&
			(p.length() == 2 || (p.length() == 3 && IsPathSeparator(p[2]))));
#else
	// examples: "/"
	bool isFsRoot = (p.length() == 1 && IsNativePathSeparator(p[0]));
#endif

	return isFsRoot;
}

bool FileSystemAbstraction::IsPathSeparator(char aChar) { return ((aChar == cPS_WIN32) || (aChar == cPS_POSIX)); }
bool FileSystemAbstraction::IsNativePathSeparator(char aChar) { return (aChar == cPS); }

bool FileSystemAbstraction::HasPathSepAtEnd(const std::string& path)
{
	bool pathSepAtEnd = false;

	if (!path.empty()) {
		pathSepAtEnd = IsNativePathSeparator(path.at(path.size() - 1));
	}

	return pathSepAtEnd;
}

std::string& FileSystemAbstraction::EnsurePathSepAtEnd(std::string& path)
{
	if (path.empty()) {
		path += "." sPS;
	} else if (!HasPathSepAtEnd(path)) {
		path += cPS;
	}

	return path;
}
std::string FileSystemAbstraction::EnsurePathSepAtEnd(const std::string& path)
{
	std::string pathCopy(path);
	EnsurePathSepAtEnd(pathCopy);
	return pathCopy;
}

void FileSystemAbstraction::EnsureNoPathSepAtEnd(std::string& path)
{
	if (HasPathSepAtEnd(path)) {
		path.resize(path.size() - 1);
	}
}

std::string FileSystemAbstraction::EnsureNoPathSepAtEnd(const std::string& path)
{
	std::string pathCopy(path);
	EnsureNoPathSepAtEnd(pathCopy);
	return pathCopy;
}

std::string FileSystemAbstraction::StripTrailingSlashes(const std::string& path)
{
	size_t len = path.length();

	while (len > 0) {
		if (IsPathSeparator(path.at(len - 1))) {
			--len;
		} else {
			break;
		}
	}

	return path.substr(0, len);
}

std::string FileSystemAbstraction::GetParent(const std::string& path)
{
	//TODO uncomment and test if it breaks other code
	/*try {
		const boost::filesystem::path p(path);
		return p.parent_path.string();
	} catch (const boost::filesystem::filesystem_error& ex) {
		return "";
	}*/

	std::string parent = path;
	EnsureNoPathSepAtEnd(parent);

	static const char* PATH_SEP_REGEX = sPS_POSIX sPS_WIN32;
	const std::string::size_type slashPos = parent.find_last_of(PATH_SEP_REGEX);

	if (slashPos == std::string::npos)
		return "";

	parent.resize(slashPos + 1);
	return parent;
}

size_t FileSystemAbstraction::GetFileSize(const std::string& file)
{
	std::error_code ec;
	auto size = static_cast<int32_t>(std::filesystem::file_size(file, ec));
	if (ec)
		return 0;

	return size;
}

bool FileSystemAbstraction::IsReadableFile(const std::string& file)
{
	// Exclude directories!
	if (!FileExists(file))
		return false;

#ifdef _WIN32
	return (_access(StripTrailingSlashes(file).c_str(), 4) == 0);
#else
	return (access(file.c_str(), R_OK | F_OK) == 0);
#endif
}

uint32_t FileSystemAbstraction::GetFileModificationTime(const std::string& file)
{
#ifdef _WIN32
	auto h = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
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

std::string FileSystemAbstraction::GetFileModificationDate(const std::string& file)
{
	const std::time_t t = GetFileModificationTime(file);

	if (t == 0)
		return "";

	const struct tm* clk = std::gmtime(&t);

	return fmt::sprintf("%d%02d%02d%02d%02d%02d", 1900 + clk->tm_year, clk->tm_mon + 1, clk->tm_mday, clk->tm_hour, clk->tm_min, clk->tm_sec);
}


bool FileSystemAbstraction::IsPathOnSpinningDisk(const std::string& path)
{
#ifdef _WIN32
	std::string volumePath; volumePath.resize(64);
	if (!::GetVolumePathNameA(path.c_str(), volumePath.data(), volumePath.size())) {
		LOG_L(L_WARNING, "[%s] GetVolumePathNameA error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
		return true;
	}

	std::string volumeName; volumeName.resize(1024);
	if (!::GetVolumeNameForVolumeMountPointA(volumePath.data(), volumeName.data(), volumeName.size())) {
		LOG_L(L_WARNING, "[%s] GetVolumeNameForVolumeMountPointA error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
		return true;
	}

	auto length = strlen(volumeName.c_str());
	if (length && volumeName[length - 1] == L'\\')
		volumeName[length - 1] = L'\0';

	HANDLE volHandle = ::CreateFileA(volumeName.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (volHandle == INVALID_HANDLE_VALUE) {
		LOG_L(L_WARNING, "[%s] CreateFileA error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
		return true;
	}

	if (volHandle == INVALID_HANDLE_VALUE) {
		LOG_L(L_WARNING, "[%s] GetVolumeHandleForFile error: '%s'", __func__, Platform::GetLastErrorAsString().c_str());
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

	std::ifstream rotf(rotFileName, std::ios::in);
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

char FileSystemAbstraction::GetNativePathSeparator()
{
	#ifndef _WIN32
	return '/';
	#else
	return '\\';
	#endif
}

bool FileSystemAbstraction::IsAbsolutePath(const std::string& path)
{
	//TODO uncomment this and test if there are conflicts in the code when this returns true but other custom code doesn't (e.g. with IsFSRoot)
	//const boost::filesystem::path f(file);
	//return f.is_absolute();

#ifdef _WIN32
	return ((path.length() > 1) && (path[1] == ':'));
#else
	return ((path.length() > 0) && (path[0] == '/'));
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
bool FileSystemAbstraction::MkDir(const std::string& dir)
{
	// First check if directory exists. We'll return success if it does.
	if (DirExists(dir))
		return true;


	// If it doesn't exist we try to mkdir it and return success if that succeeds.
#ifndef _WIN32
	const bool dirCreated = (::mkdir(dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0);
#else
	const bool dirCreated = (::_mkdir(StripTrailingSlashes(dir).c_str()) == 0);
#endif

	if (!dirCreated)
		LOG_L(L_WARNING, "[FSA::%s] error '%s' creating directory '%s'", __func__, strerror(errno), dir.c_str());

	return dirCreated;
}


bool FileSystemAbstraction::DeleteFile(const std::string& file)
{
#ifdef _WIN32
	if (DirExists(file)) {
		if (!RemoveDirectory(StripTrailingSlashes(file).c_str())) {
			LOG_L(L_WARNING, "[FSA::%s] error '%s' deleting directory '%s'", __func__, Platform::GetLastErrorAsString().c_str(), file.c_str());
			return false;
		}
		return true;
	}
#endif
	if (remove(file.c_str()) != 0) {
		LOG_L(L_WARNING, "[FSA::%s] error '%s' deleting file '%s'", __func__, strerror(errno), file.c_str());
		return false;
	}

	return true;
}


bool FileSystemAbstraction::FileExists(const std::string& file)
{
	struct stat info;
	return ((stat(file.c_str(), &info) == 0 && !S_ISDIR(info.st_mode)));
}

bool FileSystemAbstraction::DirExists(const std::string& dir)
{
	struct stat info;
	return ((stat(StripTrailingSlashes(dir).c_str(), &info) == 0 && S_ISDIR(info.st_mode)));
}


bool FileSystemAbstraction::DirIsWritable(const std::string& dir)
{
#ifdef _WIN32
	// this exists because _access does not do the right thing
	// see http://msdn.microsoft.com/en-us/library/1w06ktdy(VS.71).aspx
	// for now, try to create a temporary file in a directory and open it
	// to rule out the possibility of it being created in the virtual store
	// TODO perhaps use SECURITY_DESCRIPTOR winapi calls here

	std::string testfile = dir + "\\__$testfile42$.test";
	std::ofstream os(testfile.c_str());

	if (os.fail())
		return false;

	const char* testdata = "THIS IS A TEST";
	os << testdata;
	os.close();

	// this part should only be needed when there is no manifest embedded
	std::ifstream is(testfile.c_str());

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
}


bool FileSystemAbstraction::ComparePaths(const std::string& path1, const std::string& path2)
{
#ifndef _WIN32
	struct stat info1, info2;
	const int ret1 = stat(path1.c_str(), &info1);
	const int ret2 = stat(path2.c_str(), &info2);

	// If either files doesn't exist, return false
	if (ret1 || ret2)
		return false;

	return (info1.st_dev == info2.st_dev) && (info1.st_ino == info2.st_ino);
#else
	HANDLE h1 = CreateFile(
		path1.c_str(),
		0,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		0);

	if (h1 == INVALID_HANDLE_VALUE)
		return false;

	HANDLE h2 = CreateFile(
		path2.c_str(),
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
		info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
		&& info1.nFileIndexHigh == info2.nFileIndexHigh
		&& info1.nFileIndexLow == info2.nFileIndexLow
		&& info1.nFileSizeHigh == info2.nFileSizeHigh
		&& info1.nFileSizeLow == info2.nFileSizeLow
		&& info1.ftLastWriteTime.dwLowDateTime
		== info2.ftLastWriteTime.dwLowDateTime
		&& info1.ftLastWriteTime.dwHighDateTime
		== info2.ftLastWriteTime.dwHighDateTime;
#endif
}


std::string FileSystemAbstraction::GetSpringExecutableDir()
{
	std::string springFullName = Platform::GetProcessExecutableFile();
	std::size_t pos = springFullName.find_last_of("/\\");
	if (pos != std::string::npos)
		return EnsurePathSepAtEnd(springFullName.substr(0, pos));
	else
		return "";
}

std::string FileSystemAbstraction::GetCwd()
{
#ifndef _WIN32
	#define GETCWD getcwd
#else
	#define GETCWD _getcwd
#endif

	char path[1024];

	if (GETCWD(path, sizeof(path)) != nullptr)
		return path;

	return "";
}

void FileSystemAbstraction::ChDir(const std::string& dir)
{
#ifndef _WIN32
	const int err = chdir(dir.c_str());
#else
	const int err = _chdir(StripTrailingSlashes(dir).c_str());
#endif

	if (err) {
		throw content_error("Could not chdir into " + dir);
	}
}

static void FindFiles(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const spring::regex& regexPattern, int flags)
{
#ifdef _WIN32
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile((datadir + dir + "\\*").c_str(), &wfd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(wfd.cFileName,".") && strcmp(wfd.cFileName ,"..")) {
				if (!(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
					if ((flags & FileQueryFlags::ONLY_DIRS) == 0) {
						if (spring::regex_match(wfd.cFileName, regexPattern)) {
							matches.push_back(dir + wfd.cFileName);
						}
					}
				} else {
					if (flags & FileQueryFlags::INCLUDE_DIRS) {
						if (spring::regex_match(wfd.cFileName, regexPattern)) {
							matches.push_back(dir + wfd.cFileName + "\\");
						}
					}
					if (flags & FileQueryFlags::RECURSE) {
						FindFiles(matches, datadir, dir + wfd.cFileName + "\\", regexPattern, flags);
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
		} else {
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

void FileSystemAbstraction::FindFiles(std::vector<std::string>& matches, const std::string& dataDir, const std::string& dir, const std::string& regex, int flags)
{
	const spring::regex regexPattern(regex);
	::FindFiles(matches, dataDir, dir, regexPattern, flags);
}

