/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IArchive.h"

#include "System/StringUtil.h"

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

