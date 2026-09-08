/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <filesystem>
#include <initializer_list>

/**
 * Native file-system handling abstraction.
 * This contains only functions that have to implemented differently on
 * different OSs.
 * @note do not use this directly, but use FileSystem instead
 * @see FileSystem
 */
class FileSystem
{
public:
	static bool MkDir(const std::string& dir);
	static bool DeleteFile(const std::string& file);

	/// Returns true if the file exists, and is not a directory
	static bool FileExists(const std::filesystem::path& file);

	/// Returns true if the file exists, and is not a directory
	static bool FileExists(const std::string& file);

	static bool DirExists(const std::filesystem::path& dir);
	static bool DirExists(const std::string& dir);

	/// oddly, this is non-trivial on Windows
	static bool DirIsWritable(const std::string& dir);

	static bool ComparePaths(const std::string& path1, const std::string& path2);

	static std::string GetEngineExecutableDir();
	static std::string GetCwd();
	static void ChDir(const std::string& dir);

	/**
	 * Removes "./" or ".\" from the start of a path string.
	 */
	static std::string RemoveLocalPathPrefix(const std::string& path);

	/**
	 * Returns true if path matches regex ...
	 * on windows:          ^[a-zA-Z]\:[\\/]?$
	 * on all other systems: ^/$
	 */
	static bool IsFSRoot(const std::string& path);

	/**
	 * Returns true if the supplied char is a path separator,
	 * that is either '\' or '/'.
	 */
	static bool IsPathSeparator(char aChar);
	static bool IsPathSeparator(char8_t wChar);
	static bool IsPathSeparator(wchar_t wChar);

	/**
	 * Returns true if the path ends with the platform native path separator.
	 * That is '\' on windows and '/' on POSIX.
	 */
	static bool HasPathSepAtEnd(const std::u8string& path);
	static bool HasPathSepAtEnd(const std::string& path);

	/**
	 * Ensures the path ends with the platform native path separator.
	 * Converts the empty string to ".\" or "./" respectively.
	 * @see #HasPathSepAtEnd()
	 */
	static std::string EnsurePathSepAtEnd(const std::string& path);
	static std::string EnsurePathSepAtEnd(const std::u8string& path);

	/**
	 * Ensures the path does not end with the platform native path separator.
	 * @see #HasPathSepAtEnd()
	 */
	static std::string EnsureNoPathSepAtEnd(const std::string& path);

	/**
	 * Ensures the path does not end with a path separator.
	 */
	static std::string StripTrailingSlashes(const std::string& path);

	/**
	 * Returns the path to the parent of the given path element.
	 * @return the paths parent, including the trailing path separator,
	 *         or "" on error
	 */
	static std::string GetParent(const std::string& path);

	/**
	 * @brief get filesize
	 *
	 * @return the files size or -1, if the file does not exist. Returns
	 *          also 0 if the file is a directory.
	 */
	static int32_t GetFileSize(const std::string& file);

	// custom functions
	static bool IsReadableFile(const std::string& file);

	static uint32_t GetFileModificationTime(const std::string& file);
	/**
	 * Returns the last file modification time formatted in a sort friendly
	 * way, with second resolution.
	 * 23:58:59 30 January 1999 -> "19990130235859"
	 *
	 * @return  the last file modification time as described above,
	 *          or "" on error
	 */
	static std::string GetFileModificationDate(const std::string& file);

	/**
	 * Returns if the file or directory reside on spinning disk
	 * @return true if on a spinning disk
	 */
	static bool IsPathOnSpinningDisk(const std::string& path);

	static char GetNativePathSeparator();
	static bool IsAbsolutePath(const std::string& path);

	/**
	 * @brief list files/dirs matching @p regex under @p dataDir + @p dir
	 *
	 * Each match is returned as `dir + <path relative to dir>`. @p dataDir is the
	 * filesystem root used only to locate the search directory and is NEVER part of
	 * the result. Consequently the result is absolute iff @p dir is absolute:
	 *   - relative dir:  dataDir = "/root/", dir = "sub/"   -> "sub/file"
	 *   - absolute dir:  dataDir = "",       dir = "/abs/"  -> "/abs/file"
	 */
	static void FindFiles(std::vector<std::string>& matches, const std::string& dataDir, const std::string& dir, const std::string& regex, int flags);

	/**
	 * @brief remove a file
	 *
	 * Operates on the current working directory.
	 */
	static bool Remove(const std::string& file);

	/**
	 * @brief creates a directory recursively
	 *
	 * Works like mkdir -p, ie. attempts to create parent directories too.
	 * Operates on the current working directory.
	 * @return true if the postcondition of this function is that dir exists
	 *   in the write directory.
	 */
	static bool CreateDirectory(const std::string& dir);
	///@}


	static bool TouchFile(const std::string& filePath);

	/// @name convenience
	///@{
	/**
	 * @brief Returns the directory part of a path
	 * "/home/user/.spring/test.txt" -> "/home/user/.spring/"
	 * "test.txt" -> ""
	 */
	static std::string GetDirectory(const std::string& path);
	/**
	 * @brief Returns the filename part of a path
	 * "/home/user/.spring/test.txt" -> "test.txt"
	 */
	static std::string GetFilename(const std::string& path);
	/**
	 * @brief Returns the basename part of a path
	 * This is equivalent to the filename without extension.
	 * "/home/user/.spring/test.txt" -> "test"
	 */
	static std::string GetBasename(const std::string& path);
	/**
	 * @brief Returns the extension of the filename part of the path
	 * "/home/user/.spring/test.txt" -> "txt"
	 * "/home/user/.spring/test.txt..." -> "txt"
	 * "/home/user/.spring/test.txt. . ." -> "txt"
	 */
	static std::string GetExtensionLowerCase(const std::string& path);
	/**
	 * Converts the given path into a canonicalized one.
	 * CAUTION: be careful where using this, as it easily allows to link to
	 * outside a certain parent dir, for example a data-dir.
	 * @param path could be something like
	 *   "./symLinkToHome/foo/bar///./../test.log"
	 * @return with the example given in path, it could be
	 *   "./symLinkToHome/foo/test.log"
	 */
	static std::string GetNormalizedPath(const std::string& path);
	/**
	 * @brief Converts a glob expression to a regex
	 * @param glob string containing glob
	 * @return string containing regex
	 */
	static std::string ConvertGlobToRegex(const std::string& glob);

	static std::string Concatenate(const std::initializer_list<std::string_view>& list);

	static std::filesystem::path ForwardSlashes(const std::filesystem::path& path);
	static std::string ForwardSlashes(const std::string& path);

	static std::string NativeSlashes(const std::string& path);
	///@}

	/**
	 * @brief does a little checking of a filename
	 */
	static bool CheckFile(const std::string& file);
	static bool CheckFile(const std::filesystem::path& file);
	//	static bool CheckDir(const std::string& dir) const;

	static const std::string& GetCacheDir();
};