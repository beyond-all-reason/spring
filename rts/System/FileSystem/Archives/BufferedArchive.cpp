/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BufferedArchive.h"
#include "System/GlobalConfig.h"
#include "System/MainDefines.h"
#include "System/Log/ILog.h"

#include <cassert>

CBufferedArchive::~CBufferedArchive()
{
	size_t cacheSize = 0;
	size_t fileCount = 0;

	for (const auto& [numAccessed, gotBuffered, fileData] : fileCache) {
		if (gotBuffered) {
			cacheSize += fileData.size();
			fileCount++;
		}
	}

	if (fileCount <= 1 || cacheSize <= 1)
		return;

	LOG_L(L_INFO, "[%s][name=%s] %u bytes cached in %u files", __func__, archiveFile.c_str(), cacheSize, fileCount);
}

bool CBufferedArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	int ret = 0;

	if (!globalConfig.vfsCacheArchiveFiles || noCache) {
		if ((ret = GetFileImpl(fid, buffer)) != 1)
			LOG_L(L_WARNING, "[BufferedArchive::%s(fid=%u)][noCache=%d,vfsCache=%d] name=%s ret=%d size=" _STPF_, __func__, fid, static_cast<int>(noCache), static_cast<int>(globalConfig.vfsCacheArchiveFiles), archiveFile.c_str(), ret, buffer.size());

		return (ret == 1);
	}

	// NumFiles is virtual, can't do this in ctor
	{
		std::scoped_lock lck(mutex);
		if (fileCache.empty())
			fileCache.resize(NumFiles());
	}

	// numAccessed/gotBuffered are not atomic, and simultaneous access to the same fid will cause issues
	// however, the access pattern is such that each thread accesses a different fid, so this should be fine
	auto& [numAccessed, gotBuffered, fileData] = fileCache[fid];

	numAccessed++;

	if (gotBuffered) {
		buffer.assign(fileData.begin(), fileData.end());
		return true;
	}

	ret = (GetFileImpl(fid, buffer) == 1);

	if (numAccessed == 2 && ret) {
		fileData.assign(buffer.begin(), buffer.end());
		gotBuffered = true;
	}
	
	return ret;
}
