/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BufferedArchive.h"
#include "System/GlobalConfig.h"
#include "System/MainDefines.h"
#include "System/Log/ILog.h"

#include <cassert>

CBufferedArchive::~CBufferedArchive()
{
	// filter archives for which only {map,mod}info.lua was accessed
	if (cacheSize <= 1 || fileCount <= 1)
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
	if (fileCache.empty())
		fileCache.resize(NumFiles());

	FileBuffer& fb = fileCache.at(fid);

	fb.numAccessed++;
	if (!fb.populated) {
		if (fb.numAccessed > 1) {
			fb.exists = ((ret = GetFileImpl(fid, fb.data)) == 1);
			fb.populated = true;

			cacheSize += fb.data.size();
			fileCount += fb.exists;
		}
		else { // most files are only accessed once, don't bother with those
			ret = GetFileImpl(fid, buffer);
			return (ret == 1);
		}
	}

	if (!fb.exists) {
		LOG_L(L_WARNING, "[BufferedArchive::%s(fid=%u)][!fb.exists] name=%s ret=%d size=" _STPF_, __func__, fid, archiveFile.c_str(), ret, fb.data.size());
		return false;
	}

	if (buffer.size() != fb.data.size())
		buffer.resize(fb.data.size());

	// TODO: zero-copy access
	std::copy(fb.data.begin(), fb.data.end(), buffer.begin());
	return true;
}
