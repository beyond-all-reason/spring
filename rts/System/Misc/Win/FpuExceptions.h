/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FPU_EXCEPTIONS_H__
#define FPU_EXCEPTIONS_H__

/* This code can be used to trap FPU/SSE exceptions on Windows.
 * It's not actually referenced in the binary but is there just
 * in case it's needed for debugging in the future. */

/*
To invoke:
        fe_reset_traps();
        std::signal(SIGFPE,&sigfpe_handler); // install handler
*/

// based on: https://stackoverflow.com/questions/30175524/enabling-floating-point-exceptions-on-mingw-gcc

#ifdef _WIN32

#include <cerrno>
#include <cfenv>
#include <cfloat> //or #include <float.h> // defines _controlfp_s
#include <cmath>
#include <csignal>
#include <iostream>

#include <inttypes.h>
#include <xmmintrin.h>

void feenableexcept(uint16_t fpflags)
{
	unsigned int new_word(0);
	if (fpflags & FE_INVALID)
		new_word |= _EM_INVALID;
	if (fpflags & FE_DIVBYZERO)
		new_word |= _EM_ZERODIVIDE;
	if (fpflags & FE_OVERFLOW)
		new_word |= _EM_OVERFLOW;
	unsigned int cw(0);
	_controlfp_s(&cw, ~new_word, _MCW_EM);
}
#endif

void fe_reset_traps()
{
	std::feclearexcept(FE_ALL_EXCEPT); // clear x87 FE state
#ifdef __SSE__
	_MM_SET_EXCEPTION_STATE(0); // clear SSE FE state
#endif
	feenableexcept(FE_DIVBYZERO | FE_INVALID); // set x87 FE mask
#ifdef __SSE__
	// set SSE FE mask (orientation of this command is different from the above)
	_MM_SET_EXCEPTION_MASK(_MM_MASK_DENORM | _MM_MASK_UNDERFLOW | _MM_MASK_INEXACT | _MM_MASK_OVERFLOW);
#endif
}

void sigfpe_handler(int sig)
{
	std::signal(sig, SIG_DFL); // block signal, if needed
	std::cerr << "A floating point exception was encountered. Exiting.\n";
	fe_reset_traps();                  // in testing mode the throw may not exit, so reset traps
	std::signal(sig, &sigfpe_handler); // reinstall handler
	throw std::exception();
}

#endif
