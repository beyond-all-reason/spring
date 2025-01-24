/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _7ZIP_ARCHIVE_H
#define _7ZIP_ARCHIVE_H

#include <vector>
#include <array>
#include <string>
#include <optional>

#include <7z.h>
#include <7zFile.h>

#include "IArchiveFactory.h"
#include "IArchive.h"
#include "System/Threading/SpringThreading.h"

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
class CSevenZipArchive : public IArchive
{
public:
	CSevenZipArchive(const std::string& name);
	virtual ~CSevenZipArchive();

	int GetType() const override { return ARCHIVE_TYPE_SD7; }

	bool IsOpen() override { return isOpen; }

	uint32_t NumFiles() const override { return (fileEntries.size()); }
	bool GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer) override;
	void FileInfo(uint32_t fid, std::string& name, int& size) const override;

	static constexpr int MAX_THREADS = 32;
private:
	struct PerThreadData {
		CFileInStream archiveStream;
		CSzArEx db;
		CLookToRead2 lookStream;
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
		std::string origName;
	};

	std::vector<FileEntry> fileEntries;

	std::array<std::optional<PerThreadData>, MAX_THREADS> perThreadData;

	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen = false;
};

#endif // _7ZIP_ARCHIVE_H
