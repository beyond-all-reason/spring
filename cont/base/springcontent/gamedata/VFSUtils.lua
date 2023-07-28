-- file kept for back-compatibility while games migrate away
function RecursiveFileSearch(startingDir, fileType, vfsMode)
	return VFS.DirList(startingDir, fileType, vfsMode, true)
end
