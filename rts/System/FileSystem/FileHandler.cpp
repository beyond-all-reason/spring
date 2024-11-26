/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FileHandler.h"

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cstring>

#include "System/SpringRegex.h"

#include "FileQueryFlags.h"
#include "FileSystem.h"

#ifndef TOOLS
	#include "VFSHandler.h"
	#include "DataDirsAccess.h"
	#include "System/StringUtil.h"
	#include "System/Platform/Misc.h"
#endif

using std::string;



/******************************************************************************/
/******************************************************************************/

CFileHandler::CFileHandler(const char* fileName, const char* modes)
{
	Close();
	Open(fileName, modes);
}


CFileHandler::CFileHandler(const string& fileName, const string& modes)
{
	Close();
	Open(fileName, modes);
}



/******************************************************************************/

std::span<const uint8_t> CFileHandler::GetSpan() const
{
	return mmap ?
		std::span(mmap->data(), mmap->size()) :
		std::span(fileBuffer.data(), fileBuffer.size());
}

bool CFileHandler::TryReadFromPWD(const string& fileName)
{
#ifndef TOOLS
	if (FileSystem::IsAbsolutePath(fileName))
		return false;
	const std::string fullpath(Platform::GetOrigCWD() + fileName);
#else
	const std::string fullpath(fileName);
#endif
	std::error_code ec;
	std::filesystem::directory_entry entry(fileName, ec);
	if (ec)
		return false;

	if (!entry.exists())
		return false;

	if (!entry.is_regular_file())
		return false;

	fileSize = entry.file_size();
	if (fileSize == 0)
		return false;

	mmap = std::make_unique<mio::ummap_source>(fileName);
	if (mmap->size() != fileSize) {
		mmap = nullptr;
		return false;
	}

	return true;
}


bool CFileHandler::TryReadFromRawFS(const string& fileName)
{
#ifndef TOOLS
	const string rawpath = dataDirsAccess.LocateFile(fileName);

	std::error_code ec;
	std::filesystem::directory_entry entry(rawpath, ec);
	if (ec)
		return false;

	if (!entry.exists())
		return false;

	if (!entry.is_regular_file())
		return false;

	fileSize = entry.file_size();
	if (fileSize == 0)
		return false;

	mmap = std::make_unique<mio::ummap_source>(rawpath);
	if (mmap->size() != fileSize) {
		mmap = nullptr;
		return false;
	}

	return true;
#else
	return false;
#endif
}


bool CFileHandler::TryReadFromVFS(const string& fileName, int section)
{
#ifndef TOOLS
	if (vfsHandler == nullptr)
		return (loadCode = -2, false);

	if ((loadCode = vfsHandler->LoadFile(StringToLower(fileName), fileBuffer, (CVFSHandler::Section) section)) == 1) {
		// capacity can exceed size if FH was used to open more than one file
		// assert(fileBuffer.size() == fileBuffer.capacity());

		fileSize = fileBuffer.size();
		return true;
	}
#else
	return false;
#endif
}


void CFileHandler::Open(const string& fileName, const string& modes)
{
	this->fileName = fileName;
	for (char c: modes) {
#ifndef TOOLS
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if ((section != CVFSHandler::Section::Error) && TryReadFromVFS(fileName, section))
			break;

		if ((c == SPRING_VFS_RAW[0]) && TryReadFromRawFS(fileName))
			break;

#endif
		if ((c == SPRING_VFS_PWD[0]) && TryReadFromPWD(fileName))
			break;
	}
}

void CFileHandler::Close()
{
	filePos = 0;
	fileSize = -1;
	loadCode = -3;

	mmap = nullptr;
	fileBuffer.clear();
}



/******************************************************************************/

bool CFileHandler::FileExists(const std::string& filePath, const std::string& modes)
{
	for (char c: modes) {
#ifndef TOOLS
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if ((section != CVFSHandler::Section::Error) && vfsHandler->FileExists(filePath, section) == 1)
			return true;

		if ((c == SPRING_VFS_RAW[0]) && FileSystem::FileExists(dataDirsAccess.LocateFile(filePath)))
			return true;
#endif
		if (c == SPRING_VFS_PWD[0]) {
#ifndef TOOLS
			if (!FileSystem::IsAbsolutePath(filePath)) {
				const std::string fullpath(Platform::GetOrigCWD() + filePath);
				if (FileSystem::FileExists(fullpath))
					return true;
			}
#else
			const std::string fullpath(filePath);
			if (FileSystem::FileExists(fullpath))
				return true;
#endif
		}
	}

	return false;
}


int CFileHandler::Read(void* buf, int length)
{
	if (fileSize <= 0)
		return 0;

	if (fileBuffer.empty() && !mmap)
		return 0;

	const auto span = GetSpan();

	if ((length + filePos) > fileSize)
		length = fileSize - filePos;

	if (length > 0) {
		assert(span.size() >= (filePos + length));
		std::memcpy(buf, span.data() + filePos, length);
		filePos += length;
	}

	return length;
}


int CFileHandler::ReadString(void* buf, int length)
{
	assert(buf != nullptr);
	assert(length > 0);
	const int pos = GetPos();
	const int rlen = Read(buf, length);

	if (rlen < length)
		((char*)buf)[rlen] = 0;

	const int slen = strlen((const char*)buf);

	if (rlen > 0)
		Seek(pos + slen + 1, std::ios_base::beg);

	return slen;
}


void CFileHandler::Seek(int length, std::ios_base::seekdir where)
{
	switch (where) {
		case std::ios_base::beg: { filePos = length; } break;
		case std::ios_base::cur: { filePos += length; } break;
		case std::ios_base::end: { filePos = fileSize + length; } break;
		default: {} break;
	}
}

bool CFileHandler::Eof() const
{
	return (filePos >= fileSize);
}


int CFileHandler::GetPos()
{
	return filePos;
}


bool CFileHandler::LoadStringData(string& data)
{
	if (!FileExists())
		return false;

	assert(data.empty());

	data.clear();
	data.resize(fileSize, 0);

	Read(&data[0], fileSize);
	return true;
}

std::string CFileHandler::GetFileExt() const
{
	return FileSystem::GetExtension(fileName);
}

std::string CFileHandler::GetFileAbsolutePath(const std::string& filePath, const std::string& modes)
{
	for (char c: modes) {
#ifndef TOOLS
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if ((section != CVFSHandler::Section::Error) && vfsHandler->FileExists(filePath, section) == 1)
			return vfsHandler->GetFileAbsolutePath(filePath, section);

		if ((c == SPRING_VFS_RAW[0]) && FileSystem::FileExists(dataDirsAccess.LocateFile(filePath)))
			return dataDirsAccess.LocateFile(filePath);
#endif
		if (c == SPRING_VFS_PWD[0]) {
#ifndef TOOLS
			if (!FileSystem::IsAbsolutePath(filePath)) {
				const std::string fullpath(Platform::GetOrigCWD() + filePath);
				if (FileSystem::FileExists(fullpath))
					return fullpath;
			}
#else
			const std::string fullpath(filePath);
			if (FileSystem::FileExists(fullpath))
				return fullpath;
#endif
		}
	}

	return "";
}

std::string CFileHandler::GetArchiveContainingFile(const std::string& filePath, const std::string& modes)
{
	for (char c: modes) {
#ifndef TOOLS
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if ((section != CVFSHandler::Section::Error) && vfsHandler->FileExists(filePath, section) == 1)
			return vfsHandler->GetFileArchiveName(filePath, section);
#endif
	}

	return "";
}

/******************************************************************************/

std::vector<string> CFileHandler::FindFiles(const string& path, const string& pattern)
{
#ifndef TOOLS
	return DirList(path, pattern, SPRING_VFS_ALL, false);
#else
	return {};
#endif
}


/******************************************************************************/

std::vector<string> CFileHandler::DirList(
	const string& path,
	const string& pattern,
	const string& modes,
	bool recursive
) {
	std::vector<string> fileSet;

#ifndef TOOLS
	const std::string& pat = (pattern.empty())? "*" : pattern;

	for (char c: modes) {
		const CVFSHandler::Section section = CVFSHandler::GetModeSection(c);

		if (section != CVFSHandler::Section::Error)
			InsertVFSFiles(fileSet, path, pat, recursive, section);

		if (c == SPRING_VFS_RAW[0])
			InsertRawFiles(fileSet, path, pat, recursive);
	}

	std::stable_sort(fileSet.begin(), fileSet.end());
	fileSet.erase(std::unique(fileSet.begin(), fileSet.end()), fileSet.end());
#endif

	return fileSet;
}


bool CFileHandler::InsertRawFiles(
	std::vector<string>& fileSet,
	const string& path,
	const string& pattern,
	bool recursive
) {
#ifndef TOOLS
	const int flags = recursive * FileQueryFlags::RECURSE;
	std::vector<string> found = dataDirsAccess.FindFiles(path, pattern, flags);

	fileSet.reserve(fileSet.size() + found.size());

	for (std::string& f: found) {
		fileSet.emplace_back(std::move(f));
	}
#endif

	return true;
}


bool CFileHandler::InsertVFSFiles(
	std::vector<string>& fileSet,
	const string& path,
	const string& pattern,
	bool recursive,
	int section
) {
#ifndef TOOLS
	if (vfsHandler == nullptr)
		return false;

	std::string prefix = path;
	if (path.find_last_of("\\/") != (path.size() - 1))
		prefix += '/';

	const spring::regex regexpattern{FileSystem::ConvertGlobToRegex(pattern), spring::regex::icase};
	const std::vector<string>& found = vfsHandler->GetFilesInDir(path, recursive, (CVFSHandler::Section) section);

	fileSet.reserve(fileSet.size() + found.size());

	for (const std::string& f: found) {
		if (!spring::regex_match(f, regexpattern))
			continue;

		fileSet.emplace_back(prefix + f);
	}
#endif

	return true;
}


/******************************************************************************/

std::vector<string> CFileHandler::SubDirs(
	const string& path,
	const string& pattern,
	const string& modes,
	bool recursive
) {
	std::vector<string> dirSet;

#ifndef TOOLS
	const std::string& pat = (pattern.empty())? "*" : pattern;

	for (char c: modes) {
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if (section != CVFSHandler::Section::Error)
			InsertVFSDirs(dirSet, path, pat, recursive, section);

		if (c == SPRING_VFS_RAW[0])
			InsertRawDirs(dirSet, path, pat, recursive);
	}

	std::stable_sort(dirSet.begin(), dirSet.end());
	dirSet.erase(std::unique(dirSet.begin(), dirSet.end()), dirSet.end());
#endif

	return dirSet;
}


bool CFileHandler::InsertRawDirs(
	std::vector<string>& dirSet,
	const string& path,
	const string& pattern,
	bool recursive
) {
#ifndef TOOLS
	const int flags = FileQueryFlags::ONLY_DIRS | (recursive * FileQueryFlags::RECURSE);
	std::vector<string> found = dataDirsAccess.FindFiles(path, pattern, flags);

	dirSet.reserve(dirSet.size() + found.size());

	for (std::string& dir: found) {
		dirSet.emplace_back(std::move(dir));
	}
#endif

	return true;
}


bool CFileHandler::InsertVFSDirs(
	std::vector<string>& dirSet,
	const string& path,
	const string& pattern,
	bool recursive,
	int section
) {
#ifndef TOOLS
	if (vfsHandler == nullptr)
		return false;

	string prefix = path;
	if (path.find_last_of("\\/") != (path.size() - 1))
		prefix += '/';

	const spring::regex regexpattern{FileSystem::ConvertGlobToRegex(pattern), spring::regex::icase};
	const std::vector<string>& found = vfsHandler->GetDirsInDir(path, recursive, (CVFSHandler::Section) section);

	dirSet.reserve(dirSet.size() + found.size());

	for (const std::string& f: found) {
		if (!spring::regex_match(f, regexpattern))
			continue;

		dirSet.emplace_back(prefix + f);
	}
#endif

	return true;
}


/******************************************************************************/

string CFileHandler::AllowModes(const string& modes, const string& allowed)
{
	string newModes;
	for (char mode: modes) {
		if (allowed.find(mode) != string::npos) {
			newModes += mode;
		}
	}
	return newModes;
}

string CFileHandler::ForbidModes(const string& modes, const string& forbidden)
{
	string newModes;
	for (char mode: modes) {
		if (forbidden.find(mode) == string::npos) {
			newModes += mode;
		}
	}
	return newModes;
}


/******************************************************************************/
/******************************************************************************/
