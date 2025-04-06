/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _7ZIP_ARCHIVE_H
#define _7ZIP_ARCHIVE_H

#include <vector>
#include <array>
#include <string>
#include <optional>
#include <bitset>

#include <7z.h>
#include <7zFile.h>

#include "IArchiveFactory.h"
#include "BufferedArchive.h"
#include "System/Threading/AtomicFirstIndex.hpp"

/**
 * Creates LZMA/7zip compressed, single-file archives.
 * @see CSevenZipArchive
 */
class CSevenZipArchiveFactory : public IArchiveFactory {
public:
	CSevenZipArchiveFactory(): IArchiveFactory("sd7") {}
	bool CheckForSolid() const { return true; }
private:
	IArchive* DoCreateArchive(const std::string& filePath) const;
};


/**
 * An LZMA/7zip compressed, single-file archive.
 */
class CSevenZipArchive : public CBufferedArchive
{
public:
	CSevenZipArchive(const std::string& name);
	virtual ~CSevenZipArchive();

	int GetType() const override { return ARCHIVE_TYPE_SD7; }

	bool IsOpen() override { return isOpen.any(); }

	uint32_t NumFiles() const override { return (fileEntries.size()); }
	const std::string& FileName(uint32_t fid) const override;
	int32_t FileSize(uint32_t fid) const override;
	SFileInfo FileInfo(uint32_t fid) const override;

	bool CheckForSolid() const override { return considerSolid; }
protected:
	int GetFileImpl(uint32_t fid, std::vector<std::uint8_t>& buffer) override;
private:
	static constexpr int MAX_THREADS = 32;

	Recoil::AtomicFirstIndex<uint32_t> afi;

	struct PerThreadData {
		CFileInStream archiveStream;
		CSzArEx db;
		CLookToRead2 lookStream;
		UInt32 blockIndex = 0xFFFFFFFF;
		size_t outBufferSize = 0;
		Byte* outBuffer = nullptr;
	};

	void OpenArchive(int tnum);

	static inline spring::mutex archiveLock;
	static constexpr size_t INPUT_BUF_SIZE = (size_t)1 << 18;

	struct FileEntry {
		int fp;
		/**
		 * Real/unpacked size of the file in bytes.
		 */
		int size;
		uint32_t modTime;
		std::string origName;
	};

	std::vector<FileEntry> fileEntries;

	std::array<std::optional<PerThreadData>, MAX_THREADS> perThreadData;

	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	std::bitset<MAX_THREADS> isOpen = { false };
	bool considerSolid = false;
};

#endif // _7ZIP_ARCHIVE_H
