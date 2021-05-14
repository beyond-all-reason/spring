// Includes Math.h in turn
#include "streflop.h"

namespace streflop {

// Extended are not always available
#ifdef Extended

    const Extended ExtendedZero(0.0f);
    const Extended ExtendedPositiveInfinity = Extended(1.0f) / ExtendedZero;
    const Extended ExtendedNegativeInfinity = Extended(-1.0f) / ExtendedZero;
    // TODO: non-signaling version
    const Extended ExtendedNaN = ExtendedPositiveInfinity + ExtendedNegativeInfinity;

#endif



    // Default environment. Initialized to 0, and really set on first access
#if defined(STREFLOP_X87)
    fpenv_t FE_DFL_ENV = 0;
#elif defined(STREFLOP_SSE)
    fpenv_t FE_DFL_ENV = {0,0};
#elif defined(STREFLOP_SOFT)
    fpenv_t FE_DFL_ENV = {42,0,0};
#else
#error STREFLOP: Invalid combination or unknown FPU type.
#endif

}
