/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_BASE_H
#define _ARCHIVE_BASE_H

#include <string>
#include <vector>
#include <cinttypes>
#include <memory>
#include <semaphore>

#include "ArchiveTypes.h"
#include "System/Sync/SHA512.hpp"
#include "System/ScopedResource.h"
#include "System/UnorderedMap.hpp"

/**
 * @brief Abstraction of different archive types
 *
 * Loosely resembles STL container:
 * for (uint32_t fid = 0; fid < NumFiles(); ++fid) {
 * 	//stuff
 * }
 */
class IArchive
{
public:
	struct SFileInfo {
		std::string fileName;
		std::string specialFileName; // overloaded meaning
		int32_t size = -1;
		uint32_t modTime = 0;
	};
protected:
	IArchive(const std::string& archiveFile);
public:
	virtual ~IArchive() {}

	virtual int GetType() const = 0;

	virtual bool IsOpen() = 0;
	const std::string& GetArchiveFile() const { return archiveFile; }

	/**
	 * @return The amount of files in the archive, does not change during
	 * lifetime
	 */
	virtual uint32_t NumFiles() const = 0;
	/**
	 * Returns whether the supplied fileId is valid and available in this
	 * archive.
	 */
	inline bool IsFileId(uint32_t fileId) const {
		return (fileId < NumFiles());
	}
	/**
	 * Returns true if the file exists in this archive.
	 * @param normalizedFilePath VFS path to the file in lower-case,
	 *   using forward-slashes, for example "maps/mymap.smf"
	 * @return true if the file exists in this archive, false otherwise
	 */
	bool FileExists(const std::string& normalizedFilePath) const {
		return (lcNameIndex.find(normalizedFilePath) != lcNameIndex.end());
	}

	/**
	 * Returns the fileID of a file.
	 * @param filePath VFS path to the file, for example "maps/myMap.smf"
	 * @return fileID of the file, NumFiles() if not found
	 */
	uint32_t FindFile(const std::string& filePath) const;
	/**
	 * Fetches the content of a file by its ID.
	 * @param fid file ID in [0, NumFiles())
	 * @param buffer on success, this will be filled with the contents
	 *   of the file
	 * @return true if the file was found, and its contents have been
	 *   successfully read into buffer
	 * @see GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer)
	 */
	virtual bool GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer) = 0;
	/**
	 * Fetches the content of a file by its name.
	 * @param name VFS path to the file, for example "maps/myMap.smf"
	 * @param buffer on success, this will be filled with the contents
	 *   of the file
	 * @return true if the file was found, and its contents have been
	 *   successfully read into buffer
	 * @see GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer)
	 */
	bool GetFile(const std::string& name, std::vector<std::uint8_t>& buffer);

	uint32_t ExtractedSize() const {
		uint32_t size = 0;

		// no archive should be larger than 4GB when extracted
		for (uint32_t fid = 0; fid < NumFiles(); fid++) {
			auto fs = FileSize(fid);
			size += fs;
		}

		return size;
	}

	/**
	 * Fetches the name of a file by its ID.
	 */
	virtual const std::string& FileName(uint32_t fid) const = 0;

	/**
	 * Fetches the size of a file by its ID.
	 */
	virtual int32_t FileSize(uint32_t fid) const = 0;

	/**
	 * Fetches the name, size and modTime of a file by its ID.
	 */
	virtual SFileInfo FileInfo(uint32_t fid) const = 0;

	/**
	 * Returns true if the cost of reading the file is qualitatively relative
	 * to its file-size.
	 * This is mainly useful in the case of solid archives,
	 * which may make the reading of a single small file over proportionally
	 * expensive.
	 * The returned value is usually relative to certain arbitrary chosen
	 * constants.
	 * Most implementations may always return true.
	 * @return true if cost is ~ relative to its file-size
	 */
	virtual bool HasLowReadingCost(uint32_t fid) const { return true; }

	/**
	 * @return true if archive type can be packed solid (which is VERY slow when reading)
	 */
	virtual bool CheckForSolid() const { return false; }
	/**
	 * Fetches the (SHA512) hash of a file by its ID.
	 */
	virtual bool CalcHash(uint32_t fid, sha512::raw_digest& hash, std::vector<std::uint8_t>& fb);
protected:
	static uint32_t GetSpinningDiskParallelAccessNum();
	auto AcquireSemaphoreScoped() const { // fake const
		return spring::ScopedNullResource(
			[this]() { if (sem) sem->acquire(); },
			[this]() { if (sem) sem->release(); }
		);
	}
protected:
	// Spring expects the contents of archives to be case-independent
	// this map (which must be populated by subclass archives) is kept
	// to allow converting back from lowercase to original case
	spring::unordered_map<std::string, uint32_t> lcNameIndex;

protected:
	/// "ExampleArchive.sdd"
	const std::string archiveFile;
	uint32_t parallelAccessNum = 0;
	std::unique_ptr<std::counting_semaphore<32>> sem;
};

#endif // _ARCHIVE_BASE_H
