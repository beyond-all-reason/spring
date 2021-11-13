# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find the MiniZip library (sub-part of zlib)
# Find the native MiniZip includes and library (static or shared)
#
#  MiniZip_INCLUDE_DIR - where to find zip.h, unzip.h, ioapi.h, etc.
#  MiniZip_LIBRARIES   - List of libraries when using minizip.
#  MiniZip_FOUND       - True if minizip was found.

Include(FindPackageHandleStandardArgs)

If     (MiniZip_INCLUDE_DIR)
  # Already in cache, be silent
  Set(MiniZip_FIND_QUIETLY TRUE)
EndIf  (MiniZip_INCLUDE_DIR)

Find_Path(MiniZip_INCLUDE_DIR minizip/zip.h)

Set(MiniZip_NAMES minizip)
Find_Library(MiniZip_LIBRARY NAMES ${MiniZip_NAMES})

# handle the QUIETLY and REQUIRED arguments and set MiniZip_FOUND to TRUE if
# all listed variables are TRUE
Find_Package_Handle_Standard_Args(MiniZip DEFAULT_MSG MiniZip_LIBRARY MiniZip_INCLUDE_DIR)

If     (MiniZip_FOUND)
  Set(MiniZip_LIBRARIES ${MiniZip_LIBRARY})
Else   (MiniZip_FOUND)
  Set(MiniZip_LIBRARIES)
EndIf  (MiniZip_FOUND)

Mark_As_Advanced(MiniZip_LIBRARY MiniZip_INCLUDE_DIR)
