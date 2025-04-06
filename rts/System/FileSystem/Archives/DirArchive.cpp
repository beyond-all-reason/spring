/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DirArchive.h"

#include <assert.h>
#include <fstream>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Threading/ThreadPool.h"
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
	{
		auto isOnSpinningDisk = FileSystem::IsPathOnSpinningDisk(archiveFile);
		// just a file, can MT
		parallelAccessNum = isOnSpinningDisk ? GetSpinningDiskParallelAccessNum() : ThreadPool::GetNumThreads();
		sem = std::make_unique<decltype(sem)::element_type>(parallelAccessNum);
	}

	const std::vector<std::string>& found = dataDirsAccess.FindFiles(dirName, "*", FileQueryFlags::RECURSE);

	for (const std::string& f: found) {
		// strip our own name off.. & convert to forward slashes
		std::string origName(f, dirName.length());
		FileSystem::ForwardSlashes(origName);

		std::string rawFileName = dataDirsAccess.LocateFile(dirName + origName);
		FileSystem::FixSlashes(rawFileName);
		files.emplace_back(origName, std::move(rawFileName), -1, 0);

		// convert to lowercase and store
		lcNameIndex[StringToLower(origName)] = files.size() - 1;
	}
}


bool CDirArchive::GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	auto scopedSemAcq = AcquireSemaphoreScoped();

	std::ifstream ifs(files[fid].rawFileName.c_str(), std::ios::in | std::ios::binary);

	if (ifs.bad() || !ifs.is_open())
		return false;

	ifs.seekg(0, std::ios_base::end);
	buffer.resize(ifs.tellg());
	ifs.seekg(0, std::ios_base::beg);
	ifs.clear();

	if (!buffer.empty())
		ifs.read((char*)&buffer[0], buffer.size());

	return true;
}

const std::string& CDirArchive::FileName(uint32_t fid) const
{
	return files[fid].fileName;
}

int32_t CDirArchive::FileSize(uint32_t fid) const
{
	assert(IsFileId(fid));
	auto& file = files[fid];

	// check if not cached
	if (file.size == -1) {
		file.size = FileSystem::GetFileSize(files[fid].rawFileName);
	}	

	return file.size;
}

IArchive::SFileInfo CDirArchive::FileInfo(uint32_t fid) const
{
	assert(IsFileId(fid));
	IArchive::SFileInfo fi;
	auto& file = files[fid];
	fi.fileName = file.fileName;

	// check if not cached, file.size and file.modTime are mutable
	if (auto ifs = (file.size == -1), ifm = (file.modTime == 0); ifs || ifm) {
		auto scopedSemAcq = AcquireSemaphoreScoped();
		if (ifs)
			file.size = FileSystem::GetFileSize(file.rawFileName);

		if (ifm)
			file.modTime = FileSystemAbstraction::GetFileModificationTime(file.rawFileName);
	}

	fi.specialFileName = file.rawFileName;
	fi.size = file.size;
	fi.modTime = file.modTime;

	return fi;
}