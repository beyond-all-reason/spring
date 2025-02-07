/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#ifdef _WIN32
# define _GDI32_
# ifdef _DLL
#  undef _DLL
# endif
# include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glext.h> //gl.h may not include all extensions
