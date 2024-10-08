/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <string>
#include "AssIO.h"

#include "System/FileSystem/FileHandler.h"

#include "System/Misc/TracyDefs.h"

AssVFSStream::AssVFSStream(const std::string& pFile, const std::string& pMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	file = new CFileHandler(pFile, SPRING_VFS_ZIP);
}

AssVFSStream::~AssVFSStream()
{
	RECOIL_DETAILED_TRACY_ZONE;
	delete file;
}

size_t AssVFSStream::Read( void* pvBuffer, size_t pSize, size_t pCount)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Spring VFS only supports reading chars. Need to convert.
	int length = pSize * pCount;
	return file->Read(pvBuffer, length);
}

size_t AssVFSStream::Write( const void* pvBuffer, size_t pSize, size_t pCount)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// FAKE. Shouldn't need to write back to VFS
	return pSize * pCount;
}

aiReturn AssVFSStream::Seek( size_t pOffset, aiOrigin pOrigin)
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch(pOrigin){
		case aiOrigin_SET: // from start of file
			if ( pOffset >= file->FileSize() ) return AI_FAILURE;
			file->Seek( pOffset, std::ios_base::beg );
			break;
		case aiOrigin_CUR: // from current position in file
			if ( pOffset >= ( file->FileSize() + file->GetPos() ) ) return AI_FAILURE;
			file->Seek( pOffset, std::ios_base::cur );
			break;
		case aiOrigin_END: // reverse from end of file
			if ( pOffset >= file->FileSize() ) return AI_FAILURE;
			file->Seek( pOffset, std::ios_base::end );
			break;
		case _AI_ORIGIN_ENFORCE_ENUM_SIZE: ; // this prevents a compile-warning
	}
	return AI_SUCCESS;
}

size_t AssVFSStream::Tell() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return file->GetPos();
}


size_t AssVFSStream::FileSize() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	int filesize = file->FileSize();
	if ( filesize < 0 ) filesize = 0;
	return filesize;
}

void AssVFSStream::Flush () // FAKE
{
}


// Check whether a specific file exists
bool AssVFSSystem::Exists( const char* pFile) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CFileHandler file(pFile);
	return file.FileExists();
}

// Get the path delimiter character we'd like to get
char AssVFSSystem::getOsSeparator() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return '/';
}

// open a custom stream
Assimp::IOStream* AssVFSSystem::Open( const char* pFile, const char* pMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return new AssVFSStream( pFile, pMode );
}

void AssVFSSystem::Close( Assimp::IOStream* pFile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	delete pFile;
}
