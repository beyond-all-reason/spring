/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "DirArchive.h"

#include <assert.h>
#include <filesystem>

#include <mio/mmap.hpp>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/StringUtil.h"


CDirArchiveFactory::CDirArchiveFactory()
	: IArchiveFactory("sdd")
{
}

IArchive* CDirArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CDirArchive(filePath);
}


CDirArchive::CDirArchive(const std::string& archiveName)
	: IArchive(archiveName)
	, dirName(archiveName + '/')
{
	const std::vector<std::string>& found = dataDirsAccess.FindFiles(dirName, "*", FileQueryFlags::RECURSE);

	for (const std::string& f: found) {
		// strip our own name off.. & convert to forward slashes
		std::string origName(f, dirName.length());

		FileSystem::ForwardSlashes(origName);
		// convert to lowercase and store
		searchFiles.push_back(origName);

		lcNameIndex[StringToLower(origName)] = searchFiles.size() - 1;
	}
}


bool CDirArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	const std::string rawpath = dataDirsAccess.LocateFile(dirName + searchFiles[fid]);

	std::error_code ec;
	std::filesystem::directory_entry entry(rawpath, ec);
	if (ec)
		return false;

	if (!entry.exists())
		return false;

	if (!entry.is_regular_file())
		return false;

	mio::ummap_source mmap(rawpath);
	if (!mmap.is_open()) {
		return false;
	}

	if (!mmap.is_mapped()) {
		return false;
	}

	buffer.resize(mmap.size());
	std::memcpy(buffer.data(), mmap.data(), mmap.size());

	return true;
}

void CDirArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));

	name = searchFiles[fid];
	const std::string rawPath = dataDirsAccess.LocateFile(dirName + name);
	std::error_code ec;
	std::filesystem::directory_entry entry(rawPath, ec);

	if (ec) {
		size = 0;
		return;
	}

	if (!entry.exists()) {
		size = 0;
		return;
	}

	if (!entry.is_regular_file()) {
		size = 0;
		return;
	}

	size = entry.file_size();
}
