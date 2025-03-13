/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DIR_ARCHIVE_H
#define _DIR_ARCHIVE_H

#include <vector>

#include "IArchiveFactory.h"
#include "IArchive.h"


/**
 * Creates file-system/dir oriented archives.
 * @see CDirArchive
 */
class CDirArchiveFactory : public IArchiveFactory {
public:
	CDirArchiveFactory();
private:
	IArchive* DoCreateArchive(const std::string& filePath) const;
};


/**
 * Archive implementation which falls back to the regular file-system.
 * ie. a directory and all its contents are treated as an archive by this
 * class.
 */
class CDirArchive : public IArchive
{
public:
	CDirArchive(const std::string& archiveName);

	int GetType() const override { return ARCHIVE_TYPE_SDD; }

	bool IsOpen() override { return true; }

	uint32_t NumFiles() const override { return (files.size()); }
	bool GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer) override;
	const std::string& FileName(uint32_t fid) const override;
	int32_t FileSize(uint32_t fid) const override;
	SFileInfo FileInfo(uint32_t fid) const override;
private:
	/// "ExampleArchive.sdd/"
	const std::string dirName;

	struct Files {
		std::string fileName;
		std::string rawFileName;
		mutable int32_t size = -1;
		mutable uint32_t modTime = 0;
	};

	std::vector<Files> files;
};

#endif // _DIR_ARCHIVE_H
