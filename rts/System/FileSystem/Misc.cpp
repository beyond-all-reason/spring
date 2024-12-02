#include "Misc.hpp"

#include "FileSystem.h"
#include "DataDirsAccess.h"
#include "System/Config/ConfigHandler.h"

std::vector<std::string> FileSystemMisc::GetSplashScreenFiles()
{
	const std::string cwd = FileSystem::EnsurePathSepAtEnd(FileSystemAbstraction::GetCwd());
	const std::string ssd = FileSystem::EnsurePathSepAtEnd(configHandler->GetString("SplashScreenDir"));

	std::vector<std::string> splashScreenFiles = dataDirsAccess.FindFiles(FileSystem::IsAbsolutePath(ssd) ? ssd : cwd + ssd, "*.{png,jpg}", 0);

	if (splashScreenFiles.empty()) {
		auto logoPath = FileSystem::EnsurePathSepAtEnd(FileSystem::GetNormalizedPath(FileSystem::EnsurePathSepAtEnd(FileSystemAbstraction::GetSpringExecutableDir()) + "base"));
		splashScreenFiles = dataDirsAccess.FindFiles(logoPath, "*.{png,jpg}", 0);
	}

	return splashScreenFiles;
}