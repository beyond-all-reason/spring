/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IArchive.h"

#include <cassert>

#include "System/StringUtil.h"
#include "System/Threading/ThreadPool.h"

IArchive::IArchive(const std::string& archiveFile)
	: archiveFile(archiveFile)
{
	static_assert(decltype(sem)::element_type::max() >= ThreadPool::MAX_THREADS);
}

uint32_t IArchive::FindFile(const std::string& filePath) const
{
	const std::string& normalizedFilePath = StringToLower(filePath);
	const auto it = lcNameIndex.find(normalizedFilePath);

	if (it != lcNameIndex.end())
		return it->second;

	return NumFiles();
}

bool IArchive::GetFile(const std::string& name, std::vector<std::uint8_t>& buffer)
{
	const uint32_t fid = FindFile(name);

	if (!IsFileId(fid))
		return false;

	GetFile(fid, buffer);
	return true;
}

bool IArchive::CalcHash(uint32_t fid, sha512::raw_digest& hash, std::vector<std::uint8_t>& fb)
{
	// NOTE: should be possible to avoid a re-read for buffered archives
	if (!GetFile(fid, fb))
		return false;

	if (fb.empty())
		return false;

	sha512::calc_digest(fb.data(), fb.size(), hash.data());
	return true;
}

uint32_t IArchive::GetSpinningDiskParallelAccessNum()
{
#ifdef _WIN32
	static constexpr int NUM_PARALLEL_FILE_READS_SD = 4;
#else
	// Linux FS even on spinning disk seems far more tolerant to parallel reads, use all threads
	const int NUM_PARALLEL_FILE_READS_SD = ThreadPool::GetNumThreads();
#endif // _WIN32

	return NUM_PARALLEL_FILE_READS_SD;
}