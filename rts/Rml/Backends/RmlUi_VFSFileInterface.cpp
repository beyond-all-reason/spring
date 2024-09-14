// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#include "RmlUi_VFSFileInterface.h"

#include "System/FileSystem/FileHandler.h"

VFSFileInterface::VFSFileInterface()
{
}

Rml::FileHandle VFSFileInterface::Open(const Rml::String& path)
{
	const std::string mode = SPRING_VFS_RAW_FIRST;
	CFileHandler* fh = new CFileHandler(path, mode);
	if (!fh->FileExists()) {
		delete fh;
		return (Rml::FileHandle) nullptr;
	}
	return (Rml::FileHandle)fh;
}

void VFSFileInterface::Close(Rml::FileHandle file)
{
	((CFileHandler*)file)->Close();
	delete (CFileHandler*)file;
}

size_t VFSFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file)
{
	return ((CFileHandler*)file)->Read(buffer, size);
}

bool VFSFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
	std::ios_base::seekdir seekdir;
	switch (origin) {
		case SEEK_CUR:
			seekdir = std::ios_base::cur;
			break;
		case SEEK_END:
			seekdir = std::ios_base::end;
			break;
		case SEEK_SET:
		default:
			seekdir = std::ios_base::beg;
			break;
	}
	((CFileHandler*)file)->Seek(offset, seekdir);
	// Doesn't seem like the result of this function is ever actually checked
	return true;
}

size_t VFSFileInterface::Tell(Rml::FileHandle file)
{
	return ((CFileHandler*)file)->GetPos();
}