/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _7ZIP_ARCHIVE_H
#define _7ZIP_ARCHIVE_H

#include <7z.h>
#include <7zFile.h>

#include "IArchiveFactory.h"
#include "BufferedArchive.h"
#include <vector>
#include <string>
#include "IArchive.h"

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

	bool IsOpen() override { return isOpen; }

	unsigned int NumFiles() const override { return (fileEntries.size()); }
	int GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer) override;
	void FileInfo(unsigned int fid, std::string& name, int& size) const override;

private:
	static inline spring::mutex archiveLock;

	// actual data is in BufferedArchive
	struct FileEntry {
		int fp;
		/**
		 * Real/unpacked size of the file in bytes.
		 */
		int size;
		std::string origName;
	};

	std::vector<FileEntry> fileEntries;

	UInt32 blockIndex = 0xFFFFFFFF;
	size_t outBufferSize = 0;
	Byte* outBuffer = nullptr;

	CFileInStream archiveStream;
	CSzArEx db;
	CLookToRead2 lookStream;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen = false;
};

#endif // _7ZIP_ARCHIVE_H
