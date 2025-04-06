/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PoolArchive.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <string>
#include <cassert>
#include <cstring>
#include <iostream>
#include <format>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Threading/SpringThreading.h"
#include "System/Threading/ThreadPool.h"
#include "System/Misc/SpringTime.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"


CPoolArchiveFactory::CPoolArchiveFactory(): IArchiveFactory("sdp")
{
}

IArchive* CPoolArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CPoolArchive(filePath);
}



static uint32_t parse_uint32(uint8_t c[4])
{
	uint32_t i = 0;
	i = c[0] << 24 | i;
	i = c[1] << 16 | i;
	i = c[2] <<  8 | i;
	i = c[3] <<  0 | i;
	return i;
}

static bool gz_really_read(gzFile file, voidp buf, uint32_t len)
{
	return (gzread(file, reinterpret_cast<char*>(buf), len) == len);
}

CPoolArchive::CPoolArchive(const std::string& name)
	: CBufferedArchive(name)
{
	{
		auto isOnSpinningDisk = FileSystem::IsPathOnSpinningDisk(CPoolArchive::GetPoolRootDirectory(archiveFile));
		// each file is one gzip instance, can MT
		parallelAccessNum = isOnSpinningDisk ? GetSpinningDiskParallelAccessNum() : ThreadPool::GetNumThreads();
	}

	char c_name[255];
	uint8_t c_md5sum[16];
	uint8_t c_crc32[4];
	uint8_t c_size[4];
	uint8_t length;

	gzFile in = gzopen(name.c_str(), "rb");

	if (in == nullptr)
		throw content_error("[" + std::string(__func__) + "] could not open " + name);

	files.reserve(1024);
	stats.reserve(1024);

	// get pool dir from .sdp absolute path
	poolRootDir = GetPoolRootDirectory(name);

	while (gz_really_read(in, &length, 1)) {
		if (!gz_really_read(in, &c_name, length)) break;
		if (!gz_really_read(in, &c_md5sum, 16)) break;
		if (!gz_really_read(in, &c_crc32, 4)) break;
		if (!gz_really_read(in, &c_size, 4)) break;

		FileData& f = files.emplace_back();
		FileStat& s = stats.emplace_back();

		f.name = std::string(c_name, length);

		std::memcpy(&f.md5sum, &c_md5sum, sizeof(f.md5sum));
		std::memset(&f.shasum, 0, sizeof(f.shasum));

		f.crc32 = parse_uint32(c_crc32);
		f.size = parse_uint32(c_size);
		f.modTime = 0; // it's expensive and wasteful to set it here, set in FileInfo() instead

		s.fileIndx = files.size() - 1;
		s.readTime = 0;

		lcNameIndex[f.name] = s.fileIndx;
	}

	isOpen = gzeof(in);
	gzclose(in);
}

CPoolArchive::~CPoolArchive()
{
	const std::string& archiveFile = GetArchiveFile();
	const std::pair<uint64_t, uint64_t>& sums = GetSums();

	const unsigned long numZipFiles = files.size();
	const unsigned long sumInflSize = sums.first / 1024;
	const unsigned long sumReadTime = sums.second / (1000 * 1000);

	LOG_L(L_INFO, "[%s] archiveFile=\"%s\" numZipFiles=%lu sumInflSize=%lukb sumReadTime=%lums", __func__, archiveFile.c_str(), numZipFiles, sumInflSize, sumReadTime);

	std::partial_sort(stats.begin(), stats.begin() + std::min(stats.size(), size_t(10)), stats.end());

	// show top-10 worst access times
	for (size_t n = 0; n < std::min(stats.size(), size_t(10)); n++) {
		const FileStat& s = stats[n];
		const FileData& f = files[s.fileIndx];

		const unsigned long indx = s.fileIndx;
		const unsigned long time = s.readTime / (1000 * 1000);

		LOG_L(L_INFO, "\tfile=\"%s\" indx=%lu inflSize=%ukb readTime=%lums", f.name.c_str(), indx, f.size / 1024, time);
	}
}

const std::string& CPoolArchive::FileName(uint32_t fid) const
{
	assert(IsFileId(fid));
	return files[fid].name;
}

int32_t CPoolArchive::FileSize(uint32_t fid) const
{
	assert(IsFileId(fid));
	return files[fid].size;
}

IArchive::SFileInfo CPoolArchive::FileInfo(uint32_t fid) const
{
	assert(IsFileId(fid));
	auto& file = files[fid];

	if (file.modTime == 0) {
		auto scopedSemAcq = AcquireSemaphoreScoped();
		file.modTime = FileSystemAbstraction::GetFileModificationTime(GetPoolFilePath(poolRootDir, file.md5sum)); // file.modTime is mutable
	}

	return IArchive::SFileInfo{
		.fileName = file.name,
		.specialFileName = GetPoolFileName(file.md5sum),
		.size = static_cast<int32_t>(file.size),
		.modTime = file.modTime
	};
}

bool CPoolArchive::CalcHash(uint32_t fid, sha512::raw_digest& hash, std::vector<std::uint8_t>& fb)
{
	assert(IsFileId(fid));

	const FileData& fd = files[fid];

	// pool-entry hashes are not calculated until GetFileImpl, must check JIT
	if (fd.shasum == sha512::NULL_RAW_DIGEST)
		GetFileImpl(fid, fb);

	hash = fd.shasum;
	return (fd.shasum != sha512::NULL_RAW_DIGEST);
}

std::string CPoolArchive::GetPoolRootDirectory(const std::string& sdpName)
{
	// get pool dir from .sdp absolute path
	assert(FileSystem::IsAbsolutePath(sdpName));
	std::string poolRootDir = FileSystem::GetParent(FileSystem::GetDirectory(sdpName));
	assert(!poolRootDir.empty());

	return poolRootDir;
}

std::string CPoolArchive::GetPoolFileName(const std::array<uint8_t, 16>& md5Sum)
{
	static constexpr const char table[] = "0123456789abcdef";
	char c_hex[32];

	for (int i = 0; i < 16; ++i) {
		c_hex[2 * i    ] = table[(md5Sum[i] >> 4) & 0xF];
		c_hex[2 * i + 1] = table[(md5Sum[i]     ) & 0xF];
	}

	const std::string prefix(c_hex    ,  2);
	const std::string pstfix(c_hex + 2, 30);
	return std::format("{}/{}.gz", prefix, pstfix);
}

std::string CPoolArchive::GetPoolFilePath(const std::string& poolRootDir, const std::string& poolFile)
{
	std::string rpath = std::format("{}/pool/{}", poolRootDir, poolFile);
	return FileSystem::FixSlashes(rpath);
}

std::string CPoolArchive::GetPoolFilePath(const std::string& poolRootDir, const std::array<uint8_t, 16>& md5Sum)
{
	return GetPoolFilePath(poolRootDir, GetPoolFileName(md5Sum));
}

int CPoolArchive::GetFileImpl(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	auto& f = files[fid];
	auto& s = stats[fid];

	const auto path = GetPoolFilePath(poolRootDir, f.md5sum);

	const spring_time startTime = spring_now();


	buffer.clear();
	buffer.resize(f.size);

	const auto GzRead = [&f, &path, &buffer](bool report) -> int {
		gzFile in = gzopen(path.c_str(), "rb");

		if (in == nullptr)
			return -1;

		const int bytesRead = (buffer.empty()) ? 0 : gzread(in, reinterpret_cast<char*>(buffer.data()), buffer.size());

		if (bytesRead < 0 && report) {
			int errnum;
			const char* errgz = gzerror(in, &errnum);
			const char* errsys = std::strerror(errnum);

			LOG_L(L_ERROR, "[PoolArchive::%s] could not read file GZIP reason: \"%s\", SYSTEM reason: \"%s\" (bytesRead=%d fileSize=%u)", __func__, errgz, errsys, bytesRead, f.size);
		}

		gzclose(in);

		return bytesRead;
	};

	int bytesRead = Z_ERRNO;
	static constexpr int readRetries = 1000;

	//Try to workaround occasional crashes
	// (bytesRead=-1 fileSize=XXXX)
	int readTry;
	for (readTry = 0; readTry < readRetries; ++readTry) {
		bytesRead = GzRead(readTry == 0);
		if (bytesRead == buffer.size())
			break;

		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	s.readTime = (spring_now() - startTime).toNanoSecsi();

	if (bytesRead != buffer.size()) {
		LOG_L(L_ERROR, "[PoolArchive::%s] failed to read file \"%s\" after %d tries", __func__, path.c_str(), readRetries);
		buffer.clear();
		return 0;
	}
	if (readTry > 0) {
		LOG_L(L_WARNING, "[PoolArchive::%s] could read file \"%s\" only after %d tries", __func__, path.c_str(), readTry);
	}

	sha512::calc_digest(buffer.data(), buffer.size(), f.shasum.data());
	return 1;
}
