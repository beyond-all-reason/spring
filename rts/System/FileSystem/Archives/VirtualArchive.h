/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _VIRTUAL_ARCHIVE_H
#define _VIRTUAL_ARCHIVE_H

#include "IArchiveFactory.h"
#include "IArchive.h"

#include <vector>
#include <string>


class CVirtualArchive;

/**
 * Creates virtual non-existing archives which can be created during runtime
 */
class CVirtualArchiveFactory : public IArchiveFactory {
public:
	CVirtualArchiveFactory();
	virtual ~CVirtualArchiveFactory();

	/**
	 * Registers and creates a new virtual archive to the factory
	 * @param filePath Path with which the archive can be found and opened
	 */
	CVirtualArchive* AddArchive(const std::string& fileName);

private:
	IArchive* DoCreateArchive(const std::string& fileName) const;

	std::vector<CVirtualArchive*> archives;
};

extern CVirtualArchiveFactory* virtualArchiveFactory;

/**
 * A file in a virtual archive
 */
class CVirtualFile
{
public:
	CVirtualFile(int _fid, const std::string& _name): name(_name), fid(_fid) {}

	void WriteZip(void* zf) const;

public:
	std::vector<std::uint8_t> buffer;

	const std::string name;
	int fid;
};


/**
 * An opened virtual archive, as archives get deleted after being
 * opened with IArchiveFactory::DoCreateArchive all the data in a
 * virtual archive is lost.
 * Therefore an 'opened' class is created and the actual archive is
 * preserved
 */
class CVirtualArchiveOpen : public IArchive
{
public:
	CVirtualArchiveOpen(CVirtualArchive* archive, const std::string& fileName);

	int GetType() const override { return ARCHIVE_TYPE_SDV; }

	// virtual archives are stored in memory and as such always open
	bool IsOpen() override { return true; }
	uint32_t NumFiles() const override;
	bool GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer) override;
	const std::string& FileName(uint32_t fid) const override;
	int32_t FileSize(uint32_t fid) const override;
	SFileInfo FileInfo(uint32_t fid) const override;

private:
	CVirtualArchive* archive;
};


/**
 * A virtual archive
 */
class CVirtualArchive
{
public:
	CVirtualArchive(const std::string& _fileName)
		: fileName(_fileName)
	{}

	CVirtualArchiveOpen* Open();
	CVirtualFile* GetFilePtr(uint32_t fid) { return &files[fid]; }

	uint32_t AddFile(const std::string& file);
	uint32_t NumFiles() const { return (files.size()); }

	bool GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer);
	const std::string& FileName(uint32_t fid) const;
	int32_t FileSize(uint32_t fid) const;
	IArchive::SFileInfo FileInfo(uint32_t fid) const;

	const std::string& GetFileName() const { return fileName; }
	const spring::unordered_map<std::string, uint32_t>& GetNameIndex() const { return lcNameIndex; }

	void WriteToFile();

private:
	friend class CVirtualArchiveOpen;

	std::string fileName;
	std::vector<CVirtualFile> files;
	spring::unordered_map<std::string, uint32_t> lcNameIndex;
};

#endif // _VIRTUAL_ARCHIVE_H
