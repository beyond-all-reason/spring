#ifndef _SIMD_COMPAT_H
#define _SIMD_COMPAT_H

#ifdef SSE2NEON
    #include "lib/sse2neon/sse2neon.h"
    // sse2neon leaks <fenv.h>'s FE_XXX macros, which collide with the ones streflop
    // redefines and trigger a #warning. Undef them here so streflop gets a clean slate.
    #undef FE_INVALID
    #undef FE_DENORMAL
    #undef FE_DIVBYZERO
    #undef FE_OVERFLOW
    #undef FE_UNDERFLOW
    #undef FE_INEXACT
    #undef FE_ALL_EXCEPT
    #undef FE_TONEAREST
    #undef FE_DOWNWARD
    #undef FE_UPWARD
    #undef FE_TOWARDZERO
#else
    #ifdef _MSC_VER
        #include <intrin.h>   // MSVC umbrella
    #else
        #include <x86intrin.h> // GCC / Clang umbrella
    #endif
    #include <immintrin.h>
    #include <xmmintrin.h>
    #include <emmintrin.h>
#endif

#endif // _SIMD_COMPAT_H
