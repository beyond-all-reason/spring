/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include "GL/glew.h"
#ifndef _WIN32
//   #include "GL/glxew.h"
#endif
#include "glewstub.h"

#ifdef __cplusplus
extern "C" {
#endif

int gladLoadGL(void) {
   return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

