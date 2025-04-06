/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ZipArchive.h"

#include <algorithm>
#include <stdexcept>
#include <cassert>

#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeUtil.h"

IArchive* CZipArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CZipArchive(filePath);
}

CZipArchive::CZipArchive(const std::string& archiveName)
	: CBufferedArchive(archiveName)
{
	static_assert(ThreadPool::MAX_THREADS <= CZipArchive::MAX_THREADS, "MAX_THREADS mismatch");
	static_assert(sizeof(decltype(afi)::ValueType) * 8 >= ThreadPool::MAX_THREADS);

	std::scoped_lock lck(archiveLock); //not needed?

	unzFile zip = nullptr;

	if ((zip = unzOpen(archiveName.c_str())) == nullptr) {
		LOG_L(L_ERROR, "[%s] error opening \"%s\"", __func__, archiveName.c_str());
		return;
	}

	unz_global_info64 globalZipInfo;

	memset(&globalZipInfo, 0, sizeof(globalZipInfo));
	unzGetGlobalInfo64(zip, &globalZipInfo);

	// We need to map file positions to speed up opening later
	fileEntries.reserve(globalZipInfo.number_entry);

	for (int ret = unzGoToFirstFile(zip); ret == UNZ_OK; ret = unzGoToNextFile(zip)) {
		unz_file_info info;
		char fName[512];

		unzGetCurrentFileInfo(zip, &info, fName, sizeof(fName), nullptr, 0, nullptr, 0);

		if (fName[0] == 0)
			continue;

		const size_t fNameLen = strlen(fName);

		// exclude directory names
		if ((fName[fNameLen - 1] == '/') || (fName[fNameLen - 1] == '\\'))
			continue;

		unz_file_pos fp{};
		unzGetFilePos(zip, &fp);

		const auto& fd = fileEntries.emplace_back(
			std::move(fp), //fp
			info.uncompressed_size, //size
			fName, //origName
			info.crc, //crc
			static_cast<uint32_t>(CTimeUtil::DosTimeToTime64(info.dosDate)) //modTime
		);

		lcNameIndex.emplace(StringToLower(fd.origName), fileEntries.size() - 1);
	}

	zipPerThread[0] = zip;

	parallelAccessNum = ThreadPool::GetNumThreads(); // will open NumThreads parallel archives, this way GetFile() is no longer needs to be mutex locked
	sem = std::make_unique<decltype(sem)::element_type>(parallelAccessNum);
	const auto maxBitMask = (1u << parallelAccessNum) - 1;
	afi.SetMaxBitsMask(maxBitMask);
}

CZipArchive::~CZipArchive()
{
	std::scoped_lock lck(archiveLock); //not needed?

	for (auto& zip : zipPerThread) {
		if (zip) {
			unzClose(zip);
			zip = nullptr;
		}
	}
}


const std::string& CZipArchive::FileName(uint32_t fid) const
{
	assert(IsFileId(fid));
	return fileEntries[fid].origName;
}

int32_t CZipArchive::FileSize(uint32_t fid) const
{
	assert(IsFileId(fid));
	return fileEntries[fid].size;
}

IArchive::SFileInfo CZipArchive::FileInfo(uint32_t fid) const
{
	assert(IsFileId(fid));
	const auto& fe = fileEntries[fid];
	return IArchive::SFileInfo {
		.fileName = fe.origName,
		.specialFileName = "",
		.size = fe.size,
		.modTime = fe.modTime
	};
}

// To simplify things, files are always read completely into memory from
// the zip-file, since zlib does not provide any way of reading more
// than one file at a time
int CZipArchive::GetFileImpl(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	// this below will lead to expensive on-demand creation of thisThreadZip
	// in case actual number of parallel threads entering this function is
	// less than ThreadPool::GetThreadNum(). E.g. when counting_semaphore
	// dictates for less than ThreadPool::GetThreadNum() simultaneous IO operations
	//unzFile& thisThreadZip = zipPerThread[ThreadPool::GetThreadNum()];

	const auto tnum = afi.AcquireScoped();
	assert(tnum < parallelAccessNum);
	unzFile& thisThreadZip = zipPerThread[tnum];

	if (!thisThreadZip) {
		thisThreadZip = unzOpen(GetArchiveFile().c_str());
	}

	// Prevent opening files on missing/invalid archives
	if (thisThreadZip == nullptr)
		return -4;

	assert(IsFileId(fid));

	unzGoToFilePos(thisThreadZip, &fileEntries[fid].fp);

	unz_file_info fi;
	unzGetCurrentFileInfo(thisThreadZip, &fi, nullptr, 0, nullptr, 0, nullptr, 0);

	if (unzOpenCurrentFile(thisThreadZip) != UNZ_OK)
		return -3;

	buffer.clear();
	buffer.resize(fi.uncompressed_size);

	int ret = 1;

	if (!buffer.empty() && unzReadCurrentFile(thisThreadZip, buffer.data(), buffer.size()) != buffer.size())
		ret -= 2;
	if (unzCloseCurrentFile(thisThreadZip) == UNZ_CRCERROR)
		ret -= 1;

	if (ret != 1)
		buffer.clear();

	return ret;
}

