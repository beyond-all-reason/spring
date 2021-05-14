/*
	streflop: STandalone REproducible FLOating-Point
	Nicolas Brodu, 2006
	Code released according to the GNU Lesser General Public License

	Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP,
	and IBM MP lib.
	Uses SoftFloat too.

	Please read the history and copyright information in the documentation
	provided with the source code
*/

// Included by the main streflop include file
// module broken apart for logical code separation
#ifndef STREFLOP_MATH_H
#define STREFLOP_MATH_H

// just in case, should already be included
#include "streflop.h"

// Names from the libm conversion
namespace streflop_libm {
#ifdef Extended
using streflop::Extended;
#endif

	extern StreflopSimple __ieee754_sqrtf(StreflopSimple x);
	extern StreflopSimple __cbrtf(StreflopSimple x);
	extern StreflopSimple __ieee754_hypotf(StreflopSimple x, StreflopSimple y);
	extern StreflopSimple __ieee754_expf(StreflopSimple x);
	extern StreflopSimple __ieee754_logf(StreflopSimple x);
	extern StreflopSimple __ieee754_log2f(StreflopSimple x);
	extern StreflopSimple __ieee754_exp2f(StreflopSimple x);
	extern StreflopSimple __ieee754_log10f(StreflopSimple x);
	extern StreflopSimple __ieee754_powf(StreflopSimple x, StreflopSimple y);
	extern StreflopSimple __sinf(StreflopSimple x);
	extern StreflopSimple __cosf(StreflopSimple x);
	extern StreflopSimple __tanhf(StreflopSimple x);
	extern StreflopSimple __tanf(StreflopSimple x);
	extern StreflopSimple __ieee754_acosf(StreflopSimple x);
	extern StreflopSimple __ieee754_asinf(StreflopSimple x);
	extern StreflopSimple __atanf(StreflopSimple x);
	extern StreflopSimple __ieee754_atan2f(StreflopSimple x, StreflopSimple y);
	extern StreflopSimple __ieee754_coshf(StreflopSimple x);
	extern StreflopSimple __ieee754_sinhf(StreflopSimple x);
	extern StreflopSimple __ieee754_acoshf(StreflopSimple x);
	extern StreflopSimple __asinhf(StreflopSimple x);
	extern StreflopSimple __ieee754_atanhf(StreflopSimple x);
	extern StreflopSimple __fabsf(StreflopSimple x);
	extern StreflopSimple __floorf(StreflopSimple x);
	extern StreflopSimple __ceilf(StreflopSimple x);
	extern StreflopSimple __truncf(StreflopSimple x);
	extern StreflopSimple __ieee754_fmodf(StreflopSimple x, StreflopSimple y);
	extern StreflopSimple __ieee754_remainderf(StreflopSimple x, StreflopSimple y);
	extern StreflopSimple __remquof(StreflopSimple x, StreflopSimple y, int *quo);
	extern StreflopSimple __rintf(StreflopSimple x);
	extern long int __lrintf(StreflopSimple x);
	extern long long int __llrintf(StreflopSimple x);
	extern StreflopSimple __roundf(StreflopSimple x);
	extern long int __lroundf(StreflopSimple x);
	extern long long int __llroundf(StreflopSimple x);
	extern StreflopSimple __nearbyintf(StreflopSimple x);
	extern StreflopSimple __frexpf(StreflopSimple x, int *exp);
	extern StreflopSimple __ldexpf(StreflopSimple value, int exp);
	extern StreflopSimple __logbf(StreflopSimple x);
	extern int __ilogbf(StreflopSimple x);
	extern StreflopSimple __copysignf(StreflopSimple x, StreflopSimple y);
	extern int __signbitf(StreflopSimple x);
	extern StreflopSimple __nextafterf(StreflopSimple x, StreflopSimple y);
	extern StreflopSimple __expm1f(StreflopSimple x);
	extern StreflopSimple __log1pf(StreflopSimple x);
	extern StreflopSimple __erff(StreflopSimple x);
	extern StreflopSimple __ieee754_j0f(StreflopSimple x);
	extern StreflopSimple __ieee754_j1f(StreflopSimple x);
	extern StreflopSimple __ieee754_jnf(int n, StreflopSimple x);
	extern StreflopSimple __ieee754_y0f(StreflopSimple x);
	extern StreflopSimple __ieee754_y1f(StreflopSimple x);
	extern StreflopSimple __ieee754_ynf(int n, StreflopSimple x);
	extern StreflopSimple __scalbnf(StreflopSimple x, int n);
	extern StreflopSimple __scalblnf(StreflopSimple x, long int n);
	extern int __fpclassifyf(StreflopSimple x);
	extern int __isnanf(StreflopSimple x);
	extern int __isinff(StreflopSimple x);
	extern StreflopDouble __ieee754_sqrt(StreflopDouble x);
	extern StreflopDouble __cbrt(StreflopDouble x);
	extern StreflopDouble __ieee754_hypot(StreflopDouble x, StreflopDouble y);
	extern StreflopDouble __ieee754_exp(StreflopDouble x);
	extern StreflopDouble __ieee754_log(StreflopDouble x);
	extern StreflopDouble __ieee754_log2(StreflopDouble x);
	extern StreflopDouble __ieee754_exp2(StreflopDouble x);
	extern StreflopDouble __ieee754_log10(StreflopDouble x);
	extern StreflopDouble __ieee754_pow(StreflopDouble x, StreflopDouble y);
	extern StreflopDouble __sin(StreflopDouble x);
	extern StreflopDouble __cos(StreflopDouble x);
	extern StreflopDouble tan(StreflopDouble x);
	extern StreflopDouble __ieee754_acos(StreflopDouble x);
	extern StreflopDouble __ieee754_asin(StreflopDouble x);
	extern StreflopDouble atan(StreflopDouble x);
	extern StreflopDouble __ieee754_atan2(StreflopDouble x, StreflopDouble y);
	extern StreflopDouble __ieee754_cosh(StreflopDouble x);
	extern StreflopDouble __ieee754_sinh(StreflopDouble x);
	extern StreflopDouble __tanh(StreflopDouble x);
	extern StreflopDouble __ieee754_acosh(StreflopDouble x);
	extern StreflopDouble __asinh(StreflopDouble x);
	extern StreflopDouble __ieee754_atanh(StreflopDouble x);
	extern StreflopDouble __fabs(StreflopDouble x);
	extern StreflopDouble __floor(StreflopDouble x);
	extern StreflopDouble __ceil(StreflopDouble x);
	extern StreflopDouble __trunc(StreflopDouble x);
	extern StreflopDouble __ieee754_fmod(StreflopDouble x, StreflopDouble y);
	extern StreflopDouble __ieee754_remainder(StreflopDouble x, StreflopDouble y);
	extern StreflopDouble __remquo(StreflopDouble x, StreflopDouble y, int *quo);
	extern StreflopDouble __rint(StreflopDouble x);
	extern long int __lrint(StreflopDouble x);
	extern long long int __llrint(StreflopDouble x);
	extern StreflopDouble __round(StreflopDouble x);
	extern long int __lround(StreflopDouble x);
	extern long long int __llround(StreflopDouble x);
	extern StreflopDouble __nearbyint(StreflopDouble x);
	extern StreflopDouble __frexp(StreflopDouble x, int *exp);
	extern StreflopDouble __ldexp(StreflopDouble value, int exp);
	extern StreflopDouble __logb(StreflopDouble x);
	extern int __ilogb(StreflopDouble x);
	extern StreflopDouble __copysign(StreflopDouble x, StreflopDouble y);
	extern int __signbit(StreflopDouble x);
	extern StreflopDouble __nextafter(StreflopDouble x, StreflopDouble y);
	extern StreflopDouble __expm1(StreflopDouble x);
	extern StreflopDouble __log1p(StreflopDouble x);
	extern StreflopDouble __erf(StreflopDouble x);
	extern StreflopDouble __ieee754_j0(StreflopDouble x);
	extern StreflopDouble __ieee754_j1(StreflopDouble x);
	extern StreflopDouble __ieee754_jn(int n, StreflopDouble x);
	extern StreflopDouble __ieee754_y0(StreflopDouble x);
	extern StreflopDouble __ieee754_y1(StreflopDouble x);
	extern StreflopDouble __ieee754_yn(int n, StreflopDouble x);
	extern StreflopDouble __scalbn(StreflopDouble x, int n);
	extern StreflopDouble __scalbln(StreflopDouble x, long int n);
	extern int __fpclassify(StreflopDouble x);
	extern int __isnan(StreflopDouble x);
	extern int __isinf(StreflopDouble x);
#ifdef Extended
	extern Extended __ieee754_sqrtl(Extended x);
	extern Extended __cbrtl(Extended x);
	extern Extended __ieee754_hypotl(Extended x, Extended y);
	extern Extended __ieee754_expl(Extended x);
	extern Extended __ieee754_logl(Extended x);
	extern Extended __sinl(Extended x);
	extern Extended __cosl(Extended x);
	extern Extended __tanl(Extended x);
	extern Extended __ieee754_asinl(Extended x);
	extern Extended __atanl(Extended x);
	extern Extended __ieee754_atan2l(Extended x, Extended y);
	extern Extended __ieee754_coshl(Extended x);
	extern Extended __ieee754_sinhl(Extended x);
	extern Extended __tanhl(Extended x);
	extern Extended __ieee754_acoshl(Extended x);
	extern Extended __asinhl(Extended x);
	extern Extended __ieee754_atanhl(Extended x);
	extern Extended __fabsl(Extended x);
	extern Extended __floorl(Extended x);
	extern Extended __ceill(Extended x);
	extern Extended __truncl(Extended x);
	extern Extended __ieee754_fmodl(Extended x, Extended y);
	extern Extended __ieee754_remainderl(Extended x, Extended y);
	extern Extended __remquol(Extended x, Extended y, int *quo);
	extern Extended __rintl(Extended x);
	extern long int __lrintl(Extended x);
	extern long long int __llrintl(Extended x);
	extern Extended __roundl(Extended x);
	extern long int __lroundl(Extended x);
	extern long long int __llroundl(Extended x);
	extern Extended __nearbyintl(Extended x);
	extern Extended __frexpl(Extended x, int *exp);
	extern Extended __ldexpl(Extended value, int exp);
	extern Extended __logbl(Extended x);
	extern int __ilogbl(Extended x);
	extern Extended __copysignl(Extended x);
	extern int __signbitl(Extended x);
	extern Extended __nextafterl(Extended x, Extended y);
	extern Extended __expm1l(Extended x);
	extern Extended __log1pl(Extended x);
	extern Extended __erfl(Extended x);
	extern Extended __ieee754_j0l(Extended x);
	extern Extended __ieee754_j1l(Extended x);
	extern Extended __ieee754_jnl(int n, Extended x);
	extern Extended __ieee754_y0l(Extended x);
	extern Extended __ieee754_y1l(Extended x);
	extern Extended __ieee754_ynl(int n, Extended x);
	extern Extended __scalbnl(Extended x, int n);
	extern Extended __scalblnl(Extended x, long int n);
	extern int __fpclassifyl(Extended x);
	extern int __isnanl(Extended x);
	extern int __isinfl(Extended x);
#endif // Extended
}


// Wrappers in our own namespace
namespace streflop {

// Stolen from math.h. All floating-point numbers can be put in one of these categories.
enum
{
	STREFLOP_FP_NAN = 0,
	STREFLOP_FP_INFINITE = 1,
	STREFLOP_FP_ZERO = 2,
	STREFLOP_FP_SUBNORMAL = 3,
	STREFLOP_FP_NORMAL = 4
};


// StreflopSimple and double are present in all configurations

	inline StreflopSimple sqrt(StreflopSimple x) {return streflop_libm::__ieee754_sqrtf(x);}
	inline StreflopSimple cbrt(StreflopSimple x) {return streflop_libm::__cbrtf(x);}
	inline StreflopSimple hypot(StreflopSimple x, StreflopSimple y) {return streflop_libm::__ieee754_hypotf(x,y);}

	inline StreflopSimple exp(StreflopSimple x) {return streflop_libm::__ieee754_expf(x);}
	inline StreflopSimple log(StreflopSimple x) {return streflop_libm::__ieee754_logf(x);}
	inline StreflopSimple log2(StreflopSimple x) {return streflop_libm::__ieee754_log2f(x);}
	inline StreflopSimple exp2(StreflopSimple x) {return streflop_libm::__ieee754_exp2f(x);}
	inline StreflopSimple log10(StreflopSimple x) {return streflop_libm::__ieee754_log10f(x);}
	inline StreflopSimple pow(StreflopSimple x, StreflopSimple y) {return streflop_libm::__ieee754_powf(x,y);}

	inline StreflopSimple sin(StreflopSimple x) {return streflop_libm::__sinf(x);}
	inline StreflopSimple cos(StreflopSimple x) {return streflop_libm::__cosf(x);}
	inline StreflopSimple tan(StreflopSimple x) {return streflop_libm::__tanf(x);}
	inline StreflopSimple acos(StreflopSimple x) {return streflop_libm::__ieee754_acosf(x);}
	inline StreflopSimple asin(StreflopSimple x) {return streflop_libm::__ieee754_asinf(x);}
	inline StreflopSimple atan(StreflopSimple x) {return streflop_libm::__atanf(x);}
	inline StreflopSimple atan2(StreflopSimple x, StreflopSimple y) {return streflop_libm::__ieee754_atan2f(x,y);}

	inline StreflopSimple cosh(StreflopSimple x) {return streflop_libm::__ieee754_coshf(x);}
	inline StreflopSimple sinh(StreflopSimple x) {return streflop_libm::__ieee754_sinhf(x);}
	inline StreflopSimple tanh(StreflopSimple x) {return streflop_libm::__tanhf(x);}
	inline StreflopSimple acosh(StreflopSimple x) {return streflop_libm::__ieee754_acoshf(x);}
	inline StreflopSimple asinh(StreflopSimple x) {return streflop_libm::__asinhf(x);}
	inline StreflopSimple atanh(StreflopSimple x) {return streflop_libm::__ieee754_atanhf(x);}

	inline StreflopSimple fabs(StreflopSimple x) {return streflop_libm::__fabsf(x);}
	inline StreflopSimple floor(StreflopSimple x) {return streflop_libm::__floorf(x);}
	inline StreflopSimple ceil(StreflopSimple x) {return streflop_libm::__ceilf(x);}
	inline StreflopSimple trunc(StreflopSimple x) {return streflop_libm::__truncf(x);}
	inline StreflopSimple fmod(StreflopSimple x, StreflopSimple y) {return streflop_libm::__ieee754_fmodf(x,y);}
	inline StreflopSimple remainder(StreflopSimple x, StreflopSimple y) {return streflop_libm::__ieee754_remainderf(x,y);}
	inline StreflopSimple remquo(StreflopSimple x, StreflopSimple y, int *quo) {return streflop_libm::__remquof(x,y,quo);}
	inline StreflopSimple rint(StreflopSimple x) {return streflop_libm::__rintf(x);}
	inline long int lrint(StreflopSimple x) {return streflop_libm::__lrintf(x);}
	inline long long int llrint(StreflopSimple x) {return streflop_libm::__llrintf(x);}
	inline StreflopSimple round(StreflopSimple x) {return streflop_libm::__roundf(x);}
	inline long int lround(StreflopSimple x) {return streflop_libm::__lroundf(x);}
	inline long long int llround(StreflopSimple x) {return streflop_libm::__llroundf(x);}
	inline StreflopSimple nearbyint(StreflopSimple x) {return streflop_libm::__nearbyintf(x);}

	inline StreflopSimple frexp(StreflopSimple x, int *exp) {return streflop_libm::__frexpf(x,exp);}
	inline StreflopSimple ldexp(StreflopSimple value, int exp) {return streflop_libm::__ldexpf(value,exp);}
	inline StreflopSimple logb(StreflopSimple x) {return streflop_libm::__logbf(x);}
	inline int ilogb(StreflopSimple x) {return streflop_libm::__ilogbf(x);}
	inline StreflopSimple copysign(StreflopSimple x) {return streflop_libm::__copysignf(x,x);}
#undef signbit
	inline int signbit (StreflopSimple x) {return streflop_libm::__signbitf(x);}
	inline StreflopSimple nextafter(StreflopSimple x, StreflopSimple y) {return streflop_libm::__nextafterf(x,y);}

	inline StreflopSimple expm1(StreflopSimple x) {return streflop_libm::__expm1f(x);}
	inline StreflopSimple log1p(StreflopSimple x) {return streflop_libm::__log1pf(x);}
	inline StreflopSimple erf(StreflopSimple x) {return streflop_libm::__erff(x);}
	inline StreflopSimple j0(StreflopSimple x) {return streflop_libm::__ieee754_j0f(x);}
	inline StreflopSimple j1(StreflopSimple x) {return streflop_libm::__ieee754_j1f(x);}
	inline StreflopSimple jn(int n, StreflopSimple x) {return streflop_libm::__ieee754_jnf(n,x);}
	inline StreflopSimple y0(StreflopSimple x) {return streflop_libm::__ieee754_y0f(x);}
	inline StreflopSimple y1(StreflopSimple x) {return streflop_libm::__ieee754_y1f(x);}
	inline StreflopSimple yn(int n, StreflopSimple x) {return streflop_libm::__ieee754_ynf(n,x);}
	inline StreflopSimple scalbn(StreflopSimple x, int n) {return streflop_libm::__scalbnf(x,n);}
	inline StreflopSimple scalbln(StreflopSimple x, long int n) {return streflop_libm::__scalblnf(x,n);}

#undef fpclassify
	inline int fpclassify(StreflopSimple x) {return streflop_libm::__fpclassifyf(x);}
#undef isnan
	inline int isnan(StreflopSimple x) {return streflop_libm::__isnanf(x);}
#undef isinf
	inline int isinf(StreflopSimple x) {return streflop_libm::__isinff(x);}
#undef isfinite
	inline int isfinite(StreflopSimple x) {return !(isnan(x) || isinf(x));}

	// Stolen from math.h and inlined instead of macroized.
	// Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
#undef isnormal
	inline int isnormal(StreflopSimple x) {return fpclassify(x) == STREFLOP_FP_NORMAL;}

	// Constants user may set
	#define StreflopSimplePositiveInfinity 0x7F800000
	#define StreflopSimpleNegativeInfinity 0xFF800000
	#define StreflopSimpleNaN 0x7FC00000 // note: quiet NaN

	/** Generic C99 "macros" for unordered comparison
		Defined as inlined for each type, thanks to C++ overloading
	*/
#if defined isunordered
#undef isunordered
#endif // defined isunordered
	inline bool isunordered(StreflopSimple x, StreflopSimple y) {
		return (fpclassify(x) == STREFLOP_FP_NAN) || (fpclassify(y) == STREFLOP_FP_NAN);
	}
#if defined isgreater
#undef isgreater
#endif // defined isgreater
	inline bool isgreater(StreflopSimple x, StreflopSimple y) {
		return (!isunordered(x,y)) && (x > y);
	}
#if defined isgreaterequal
#undef isgreaterequal
#endif // defined isgreaterequal
	inline bool isgreaterequal(StreflopSimple x, StreflopSimple y) {
		return (!isunordered(x,y)) && (x >= y);
	}
#if defined isless
#undef isless
#endif // defined isless
	inline bool isless(StreflopSimple x, StreflopSimple y) {
		return (!isunordered(x,y)) && (x < y);
	}
#if defined islessequal
#undef islessequal
#endif // defined islessequal
	inline bool islessequal(StreflopSimple x, StreflopSimple y) {
		return (!isunordered(x,y)) && (x <= y);
	}
#if defined islessgreater
#undef islessgreater
#endif // defined islessgreater
	inline bool islessgreater(StreflopSimple x, StreflopSimple y) {
		return (!isunordered(x,y)) && ((x < y) || (x > y));
	}


// Add xxxf alias to ease porting existing code to streflop
// Additionally, using xxxf(number) avoids potential confusion

	inline StreflopSimple sqrtf(StreflopSimple x) {return sqrt(x);}
	inline StreflopSimple cbrtf(StreflopSimple x) {return cbrt(x);}
	inline StreflopSimple hypotf(StreflopSimple x, StreflopSimple y) {return hypot(x, y);}

	inline StreflopSimple expf(StreflopSimple x) {return exp(x);}
	inline StreflopSimple logf(StreflopSimple x) {return log(x);}
	inline StreflopSimple log2f(StreflopSimple x) {return log2(x);}
	inline StreflopSimple exp2f(StreflopSimple x) {return exp2(x);}
	inline StreflopSimple log10f(StreflopSimple x) {return log10(x);}
	inline StreflopSimple powf(StreflopSimple x, StreflopSimple y) {return pow(x, y);}

	inline StreflopSimple sinf(StreflopSimple x) {return sin(x);}
	inline StreflopSimple cosf(StreflopSimple x) {return cos(x);}
	inline StreflopSimple tanf(StreflopSimple x) {return tan(x);}
	inline StreflopSimple acosf(StreflopSimple x) {return acos(x);}
	inline StreflopSimple asinf(StreflopSimple x) {return asin(x);}
	inline StreflopSimple atanf(StreflopSimple x) {return atan(x);}
	inline StreflopSimple atan2f(StreflopSimple x, StreflopSimple y) {return atan2(x, y);}

	inline StreflopSimple coshf(StreflopSimple x) {return cosh(x);}
	inline StreflopSimple sinhf(StreflopSimple x) {return sinh(x);}
	inline StreflopSimple tanhf(StreflopSimple x) {return tanh(x);}
	inline StreflopSimple acoshf(StreflopSimple x) {return acosh(x);}
	inline StreflopSimple asinhf(StreflopSimple x) {return asinh(x);}
	inline StreflopSimple atanhf(StreflopSimple x) {return atanh(x);}

	inline StreflopSimple fabsf(StreflopSimple x) {return fabs(x);}
	inline StreflopSimple floorf(StreflopSimple x) {return floor(x);}
	inline StreflopSimple ceilf(StreflopSimple x) {return ceil(x);}
	inline StreflopSimple truncf(StreflopSimple x) {return trunc(x);}
	inline StreflopSimple fmodf(StreflopSimple x, StreflopSimple y) {return fmod(x,y);}
	inline StreflopSimple remainderf(StreflopSimple x, StreflopSimple y) {return remainder(x,y);}
	inline StreflopSimple remquof(StreflopSimple x, StreflopSimple y, int *quo) {return remquo(x, y, quo);}
	inline StreflopSimple rintf(StreflopSimple x) {return rint(x);}
	inline long int lrintf(StreflopSimple x) {return lrint(x);}
	inline long long int llrintf(StreflopSimple x) {return llrint(x);}
	inline StreflopSimple roundf(StreflopSimple x) {return round(x);}
	inline long int lroundf(StreflopSimple x) {return lround(x);}
	inline long long int llroundf(StreflopSimple x) {return llround(x);}
	inline StreflopSimple nearbyintf(StreflopSimple x) {return nearbyint(x);}

	inline StreflopSimple frexpf(StreflopSimple x, int *exp) {return frexp(x, exp);}
	inline StreflopSimple ldexpf(StreflopSimple value, int exp) {return ldexp(value,exp);}
	inline StreflopSimple logbf(StreflopSimple x) {return logb(x);}
	inline int ilogbf(StreflopSimple x) {return ilogb(x);}
	inline StreflopSimple copysignf(StreflopSimple x) {return copysign(x);}
	inline int signbitf(StreflopSimple x) {return signbit(x);}
	inline StreflopSimple nextafterf(StreflopSimple x, StreflopSimple y) {return nextafter(x, y);}

	inline StreflopSimple expm1f(StreflopSimple x) {return expm1(x);}
	inline StreflopSimple log1pf(StreflopSimple x) {return log1p(x);}
	inline StreflopSimple erff(StreflopSimple x) {return erf(x);}
	inline StreflopSimple j0f(StreflopSimple x) {return j0(x);}
	inline StreflopSimple j1f(StreflopSimple x) {return j1(x);}
	inline StreflopSimple jnf(int n, StreflopSimple x) {return jn(n, x);}
	inline StreflopSimple y0f(StreflopSimple x) {return y0(x);}
	inline StreflopSimple y1f(StreflopSimple x) {return y1(x);}
	inline StreflopSimple ynf(int n, StreflopSimple x) {return yn(n, x);}
	inline StreflopSimple scalbnf(StreflopSimple x, int n) {return scalbn(x, n);}
	inline StreflopSimple scalblnf(StreflopSimple x, long int n) {return scalbln(x, n);}

	inline int fpclassifyf(StreflopSimple x) {return fpclassify(x);}
	inline int isnanf(StreflopSimple x) {return isnan(x);}
	inline int isinff(StreflopSimple x) {return isinf(x);}
	inline int isfinitef(StreflopSimple x) {return isfinite(x);}
	inline int isnormalf(StreflopSimple x) {return isnormal(x);}

	inline bool isunorderedf(StreflopSimple x, StreflopSimple y) {return isunordered(x, y);}
	inline bool isgreaterf(StreflopSimple x, StreflopSimple y) {return isgreater(x, y);}
	inline bool isgreaterequalf(StreflopSimple x, StreflopSimple y) {return isgreaterequal(x, y);}
	inline bool islessf(StreflopSimple x, StreflopSimple y) {return isless(x, y);}
	inline bool islessequalf(StreflopSimple x, StreflopSimple y) {return islessequal(x, y);}
	inline bool islessgreaterf(StreflopSimple x, StreflopSimple y) {return islessgreater(x, y);}


// Declare StreflopDouble functions
// StreflopSimple and double are present in all configurations

	inline StreflopDouble sqrt(StreflopDouble x) {return streflop_libm::__ieee754_sqrt(x);}
	inline StreflopDouble cbrt(StreflopDouble x) {return streflop_libm::__cbrt(x);}
	inline StreflopDouble hypot(StreflopDouble x, StreflopDouble y) {return streflop_libm::__ieee754_hypot(x,y);}

	inline StreflopDouble exp(StreflopDouble x) {return streflop_libm::__ieee754_exp(x);}
	inline StreflopDouble log(StreflopDouble x) {return streflop_libm::__ieee754_log(x);}
	inline StreflopDouble log2(StreflopDouble x) {return streflop_libm::__ieee754_log2(x);}
	inline StreflopDouble exp2(StreflopDouble x) {return streflop_libm::__ieee754_exp2(x);}
	inline StreflopDouble log10(StreflopDouble x) {return streflop_libm::__ieee754_log10(x);}
	inline StreflopDouble pow(StreflopDouble x, StreflopDouble y) {return streflop_libm::__ieee754_pow(x,y);}

	inline StreflopDouble sin(StreflopDouble x) {return streflop_libm::__sin(x);}
	inline StreflopDouble cos(StreflopDouble x) {return streflop_libm::__cos(x);}
	inline StreflopDouble tan(StreflopDouble x) {return streflop_libm::tan(x);}
	inline StreflopDouble acos(StreflopDouble x) {return streflop_libm::__ieee754_acos(x);}
	inline StreflopDouble asin(StreflopDouble x) {return streflop_libm::__ieee754_asin(x);}
	inline StreflopDouble atan(StreflopDouble x) {return streflop_libm::atan(x);}
	inline StreflopDouble atan2(StreflopDouble x, StreflopDouble y) {return streflop_libm::__ieee754_atan2(x,y);}

	inline StreflopDouble cosh(StreflopDouble x) {return streflop_libm::__ieee754_cosh(x);}
	inline StreflopDouble sinh(StreflopDouble x) {return streflop_libm::__ieee754_sinh(x);}
	inline StreflopDouble tanh(StreflopDouble x) {return streflop_libm::__tanh(x);}
	inline StreflopDouble acosh(StreflopDouble x) {return streflop_libm::__ieee754_acosh(x);}
	inline StreflopDouble asinh(StreflopDouble x) {return streflop_libm::__asinh(x);}
	inline StreflopDouble atanh(StreflopDouble x) {return streflop_libm::__ieee754_atanh(x);}

	inline StreflopDouble fabs(StreflopDouble x) {return streflop_libm::__fabs(x);}
	inline StreflopDouble floor(StreflopDouble x) {return streflop_libm::__floor(x);}
	inline StreflopDouble ceil(StreflopDouble x) {return streflop_libm::__ceil(x);}
	inline StreflopDouble trunc(StreflopDouble x) {return streflop_libm::__trunc(x);}
	inline StreflopDouble fmod(StreflopDouble x, StreflopDouble y) {return streflop_libm::__ieee754_fmod(x,y);}
	inline StreflopDouble remainder(StreflopDouble x, StreflopDouble y) {return streflop_libm::__ieee754_remainder(x,y);}
	inline StreflopDouble remquo(StreflopDouble x, StreflopDouble y, int *quo) {return streflop_libm::__remquo(x,y,quo);}
	inline StreflopDouble rint(StreflopDouble x) {return streflop_libm::__rint(x);}
	inline long int lrint(StreflopDouble x) {return streflop_libm::__lrint(x);}
	inline long long int llrint(StreflopDouble x) {return streflop_libm::__llrint(x);}
	inline StreflopDouble round(StreflopDouble x) {return streflop_libm::__round(x);}
	inline long int lround(StreflopDouble x) {return streflop_libm::__lround(x);}
	inline long long int llround(StreflopDouble x) {return streflop_libm::__llround(x);}
	inline StreflopDouble nearbyint(StreflopDouble x) {return streflop_libm::__nearbyint(x);}

	inline StreflopDouble frexp(StreflopDouble x, int *exp) {return streflop_libm::__frexp(x, exp);}
	inline StreflopDouble ldexp(StreflopDouble value, int exp) {return streflop_libm::__ldexp(value,exp);}
	inline StreflopDouble logb(StreflopDouble x) {return streflop_libm::__logb(x);}
	inline int ilogb(StreflopDouble x) {return streflop_libm::__ilogb(x);}
	inline StreflopDouble copysign(StreflopDouble x) {return streflop_libm::__copysign(x,x);}
	inline int signbit(StreflopDouble x) {return streflop_libm::__signbit(x);}
	inline StreflopDouble nextafter(StreflopDouble x, StreflopDouble y) {return streflop_libm::__nextafter(x,y);}

	inline StreflopDouble expm1(StreflopDouble x) {return streflop_libm::__expm1(x);}
	inline StreflopDouble log1p(StreflopDouble x) {return streflop_libm::__log1p(x);}
	inline StreflopDouble erf(StreflopDouble x) {return streflop_libm::__erf(x);}
	inline StreflopDouble j0(StreflopDouble x) {return streflop_libm::__ieee754_j0(x);}
	inline StreflopDouble j1(StreflopDouble x) {return streflop_libm::__ieee754_j1(x);}
	inline StreflopDouble jn(int n, StreflopDouble x) {return streflop_libm::__ieee754_jn(n,x);}
	inline StreflopDouble y0(StreflopDouble x) {return streflop_libm::__ieee754_y0(x);}
	inline StreflopDouble y1(StreflopDouble x) {return streflop_libm::__ieee754_y1(x);}
	inline StreflopDouble yn(int n, StreflopDouble x) {return streflop_libm::__ieee754_yn(n,x);}
	inline StreflopDouble scalbn(StreflopDouble x, int n) {return streflop_libm::__scalbn(x,n);}
	inline StreflopDouble scalbln(StreflopDouble x, long int n) {return streflop_libm::__scalbln(x,n);}

	inline int fpclassify(StreflopDouble x) {return streflop_libm::__fpclassify(x);}
	inline int isnan(StreflopDouble x) {return streflop_libm::__isnan(x);}
	inline int isinf(StreflopDouble x) {return streflop_libm::__isinf(x);}
	inline int isfinite(StreflopDouble x) {return !(isnan(x) || isinf(x));}

	// Stolen from math.h and inlined instead of macroized.
	// Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
	inline int isnormal(StreflopDouble x) {return fpclassify(x) == STREFLOP_FP_NORMAL;}

	#define StreflopDoublePositiveInfinity 0x7FF0000000000000
	#define StreflopDoubleNegativeInfinity 0xFFF0000000000000
	#define StreflopDoubleNaN 0x7FF8000000000000 // note: quiet positive NaN

	/** Generic C99 "macros" for unordered comparison
		Defined as inlined for each type, thanks to C++ overloading
	*/
	inline bool isunordered(StreflopDouble x, StreflopDouble y) {
		return (fpclassify(x) == STREFLOP_FP_NAN) || (fpclassify (y) == STREFLOP_FP_NAN);
	}
	inline bool isgreater(StreflopDouble x, StreflopDouble y) {
		return (!isunordered(x,y)) && (x > y);
	}
	inline bool isgreaterequal(StreflopDouble x, StreflopDouble y) {
		return (!isunordered(x,y)) && (x >= y);
	}
	inline bool isless(StreflopDouble x, StreflopDouble y) {
		return (!isunordered(x,y)) && (x < y);
	}
	inline bool islessequal(StreflopDouble x, StreflopDouble y) {
		return (!isunordered(x,y)) && (x <= y);
	}
	inline bool islessgreater(StreflopDouble x, StreflopDouble y) {
		return (!isunordered(x,y)) && ((x < y) || (x > y));
	}

// Extended are not always available
#ifdef Extended

	inline Extended cbrt(Extended x) {return streflop_libm::__cbrtl(x);}
	inline Extended hypot(Extended x, Extended y) {return streflop_libm::__ieee754_hypotl(x,y);}


// Missing from libm: temporarily switch to StreflopDouble and execute the StreflopDouble version,
// then switch back to Extended and return the result
	inline Extended sqrt(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = sqrt(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}

	inline Extended exp(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = exp(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = log(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log2(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = log2(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended exp2(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = exp2(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log10(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = log10(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended pow(Extended x, Extended y) {streflop_init<StreflopDouble>(); StreflopDouble res = pow(StreflopDouble(x), StreflopDouble(y)); streflop_init<Extended>(); return Extended(res);}
	inline Extended sin(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = sin(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended cos(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = cos(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended tan(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = tan(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended acos(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = acos(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended asin(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = asin(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended atan(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = atan(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended atan2(Extended x, Extended y) {streflop_init<StreflopDouble>(); StreflopDouble res = atan2(StreflopDouble(x), StreflopDouble(y)); streflop_init<Extended>(); return Extended(res);}

	inline Extended cosh(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = cosh(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended sinh(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = sinh(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended tanh(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = tanh(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended acosh(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = acosh(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended asinh(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = asinh(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended atanh(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = atanh(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}


	inline Extended expm1(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = expm1(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log1p(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = log1p(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended erf(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = erf(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended j0(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = j0(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended j1(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = j1(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended jn(int n, Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = jn(n,StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended y0(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = y0(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended y1(Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = y1(StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended yn(int n, Extended x) {streflop_init<StreflopDouble>(); StreflopDouble res = yn(n,StreflopDouble(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended scalbn(Extended x, int n) {streflop_init<StreflopDouble>(); StreflopDouble res = scalbn(StreflopDouble(x),n); streflop_init<Extended>(); return Extended(res);}
	inline Extended scalbln(Extended x, long int n) {streflop_init<StreflopDouble>(); StreflopDouble res = scalbln(StreflopDouble(x),n); streflop_init<Extended>(); return Extended(res);}



	inline Extended fabs(Extended x) {return streflop_libm::__fabsl(x);}
	inline Extended floor(Extended x) {return streflop_libm::__floorl(x);}
	inline Extended ceil(Extended x) {return streflop_libm::__ceill(x);}
	inline Extended trunc(Extended x) {return streflop_libm::__truncl(x);}
	inline Extended fmod(Extended x, Extended y) {return streflop_libm::__ieee754_fmodl(x,y);}
	inline Extended remainder(Extended x, Extended y) {return streflop_libm::__ieee754_remainderl(x,y);}
	inline Extended remquo(Extended x, Extended y, int *quo) {return streflop_libm::__remquol(x,y,quo);}
	inline Extended rint(Extended x) {return streflop_libm::__rintl(x);}
	inline long int lrint(Extended x) {return streflop_libm::__lrintl(x);}
	inline long long int llrint(Extended x) {return streflop_libm::__llrintl(x);}
	inline Extended round(Extended x) {return streflop_libm::__roundl(x);}
	inline long int lround(Extended x) {return streflop_libm::__lroundl(x);}
	inline long long int llround(Extended x) {return streflop_libm::__llroundl(x);}
	inline Extended nearbyint(Extended x) {return streflop_libm::__nearbyintl(x);}

	inline Extended frexp(Extended x, int *exp) {return streflop_libm::__frexpl(x,exp);}
	inline Extended ldexp(Extended value, int exp) {return streflop_libm::__ldexpl(value,exp);}
	inline Extended logb(Extended x) {return streflop_libm::__logbl(x);}
	inline int ilogb(Extended x) {return streflop_libm::__ilogbl(x);}
	inline Extended copysign(Extended x) {return streflop_libm::__copysignl(x);}
	inline int signbit (Extended x) {return streflop_libm::__signbitl(x);}
	inline Extended nextafter(Extended x, Extended y) {return streflop_libm::__nextafterl(x,y);}

	inline int fpclassify(Extended x) {return streflop_libm::__fpclassifyl(x);}
	inline int isnan(Extended x) {return streflop_libm::__isnanl(x);}
	inline int isinf(Extended x) {return streflop_libm::__isinfl(x);}
	inline int isfinite(Extended x) {return !(isnan(x) || isinf(x));}

	// Stolen from math.h and inlined instead of macroized.
	// Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
	inline int isnormal(Extended x) {return fpclassify(x) == STREFLOP_FP_NORMAL;}

	// Constants user may set
	extern const Extended ExtendedPositiveInfinity;
	extern const Extended ExtendedNegativeInfinity;
	// Non-signaling NaN used for returning such results
	// Standard lets a large room for implementing different kinds of NaN
	// These NaN can be used for custom purposes
	// Threated as an integer, the bit pattern here may be incremented to get at least 2^20 different NaN custom numbers!
	// Note that when switching the left-most bit to 1, you can get another bunch of negative NaNs, whatever this mean.
	extern const Extended ExtendedNaN;

	/** Generic C99 "macros" for unordered comparison
		Defined as inlined for each type, thanks to C++ overloading
	*/
	inline bool isunordered(Extended x, Extended y) {
		return (fpclassify(x) == STREFLOP_FP_NAN) || (fpclassify (y) == STREFLOP_FP_NAN);
	}
	inline bool isgreater(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x > y);
	}
	inline bool isgreaterequal(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x >= y);
	}
	inline bool isless(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x < y);
	}
	inline bool islessequal(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x <= y);
	}
	inline bool islessgreater(Extended x, Extended y) {
		return (!isunordered(x,y)) && ((x < y) || (x > y));
	}


// Add xxxl alias to ease porting existing code to streflop
// Additionally, using xxxl(number) avoids potential confusion

	inline Extended sqrtl(Extended x) {return sqrt(x);}
	inline Extended cbrtl(Extended x) {return cbrt(x);}
	inline Extended hypotl(Extended x, Extended y) {return hypot(x, y);}

	inline Extended expl(Extended x) {return exp(x);}
	inline Extended logl(Extended x) {return log(x);}
	inline Extended log2l(Extended x) {return log2(x);}
	inline Extended exp2l(Extended x) {return exp2(x);}
	inline Extended log10l(Extended x) {return log10(x);}
	inline Extended powl(Extended x, Extended y) {return pow(x, y);}

	inline Extended sinl(Extended x) {return sin(x);}
	inline Extended cosl(Extended x) {return cos(x);}
	inline Extended tanl(Extended x) {return tan(x);}
	inline Extended acosl(Extended x) {return acos(x);}
	inline Extended asinl(Extended x) {return asin(x);}
	inline Extended atanl(Extended x) {return atan(x);}
	inline Extended atan2l(Extended x, Extended y) {return atan2(x, y);}

	inline Extended coshl(Extended x) {return cosh(x);}
	inline Extended sinhl(Extended x) {return sinh(x);}
	inline Extended tanhl(Extended x) {return tanh(x);}
	inline Extended acoshl(Extended x) {return acosh(x);}
	inline Extended asinhl(Extended x) {return asinh(x);}
	inline Extended atanhl(Extended x) {return atanh(x);}

	inline Extended fabsl(Extended x) {return fabs(x);}
	inline Extended floorl(Extended x) {return floor(x);}
	inline Extended ceill(Extended x) {return ceil(x);}
	inline Extended truncl(Extended x) {return trunc(x);}
	inline Extended fmodl(Extended x, Extended y) {return fmod(x,y);}
	inline Extended remainderl(Extended x, Extended y) {return remainder(x,y);}
	inline Extended remquol(Extended x, Extended y, int *quo) {return remquo(x, y, quo);}
	inline Extended rintl(Extended x) {return rint(x);}
	inline long int lrintl(Extended x) {return lrint(x);}
	inline long long int llrintl(Extended x) {return llrint(x);}
	inline Extended roundl(Extended x) {return round(x);}
	inline long int lroundl(Extended x) {return lround(x);}
	inline long long int llroundl(Extended x) {return llround(x);}
	inline Extended nearbyintl(Extended x) {return nearbyint(x);}

	inline Extended frexpl(Extended x, int *exp) {return frexp(x, exp);}
	inline Extended ldexpl(Extended value, int exp) {return ldexp(value,exp);}
	inline Extended logbl(Extended x) {return logb(x);}
	inline int ilogbl(Extended x) {return ilogb(x);}
	inline Extended copysignl(Extended x) {return copysign(x);}
	inline int signbitl(Extended x) {return signbit(x);}
	inline Extended nextafterl(Extended x, Extended y) {return nextafter(x, y);}

	inline Extended expm1l(Extended x) {return expm1(x);}
	inline Extended log1pl(Extended x) {return log1p(x);}
	inline Extended erfl(Extended x) {return erf(x);}
	inline Extended j0l(Extended x) {return j0(x);}
	inline Extended j1l(Extended x) {return j1(x);}
	inline Extended jnl(int n, Extended x) {return jn(n, x);}
	inline Extended y0l(Extended x) {return y0(x);}
	inline Extended y1l(Extended x) {return y1(x);}
	inline Extended ynl(int n, Extended x) {return yn(n, x);}
	inline Extended scalbnl(Extended x, int n) {return scalbn(x, n);}
	inline Extended scalblnl(Extended x, long int n) {return scalbln(x, n);}

	inline int fpclassifyl(Extended x) {return fpclassify(x);}
	inline int isnanl(Extended x) {return isnan(x);}
	inline int isinfl(Extended x) {return isinf(x);}
	inline int isfinitel(Extended x) {return isfinite(x);}
	inline int isnormall(Extended x) {return isnormal(x);}

	inline bool isunorderedl(Extended x, Extended y) {return isunordered(x, y);}
	inline bool isgreaterl(Extended x, Extended y) {return isgreater(x, y);}
	inline bool isgreaterequall(Extended x, Extended y) {return isgreaterequal(x, y);}
	inline bool islessl(Extended x, Extended y) {return isless(x, y);}
	inline bool islessequall(Extended x, Extended y) {return islessequal(x, y);}
	inline bool islessgreaterl(Extended x, Extended y) {return islessgreater(x, y);}


#endif // Extended

} // namespace streflop

#endif // STREFLOP_MATH_H
