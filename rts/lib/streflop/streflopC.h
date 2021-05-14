/* Copyright (C) 2009 Tobi Vollebregt */

/*
	Serves as a C compatible interface to the most basic streflop functions.
*/

#ifndef STREFLOP_C_H
#define STREFLOP_C_H

#ifndef StreflopSimple
	#define StreflopSimple float
	#define StreflopDouble double
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	STREFLOP_FP_NAN = 0,
	STREFLOP_FP_INFINITE = 1,
	STREFLOP_FP_ZERO = 2,
	STREFLOP_FP_SUBNORMAL = 3,
	STREFLOP_FP_NORMAL = 4
};

/// Initializes the FPU to single precision
void streflop_init_StreflopSimple();

/// Initializes the FPU to double precision
void streflop_init_StreflopDouble();

StreflopSimple sqrtf(StreflopSimple x);
StreflopSimple cbrtf(StreflopSimple x);
StreflopSimple hypotf(StreflopSimple x, StreflopSimple y);

StreflopSimple expf(StreflopSimple x);
StreflopSimple logf(StreflopSimple x);
StreflopSimple log2f(StreflopSimple x);
StreflopSimple exp2f(StreflopSimple x);
StreflopSimple log10f(StreflopSimple x);
StreflopSimple powf(StreflopSimple x, StreflopSimple y);

StreflopSimple sinf(StreflopSimple x);
StreflopSimple cosf(StreflopSimple x);
StreflopSimple tanf(StreflopSimple x);
StreflopSimple acosf(StreflopSimple x);
StreflopSimple asinf(StreflopSimple x);
StreflopSimple atanf(StreflopSimple x);
StreflopSimple atan2f(StreflopSimple x, StreflopSimple y);

StreflopSimple coshf(StreflopSimple x);
StreflopSimple sinhf(StreflopSimple x);
StreflopSimple tanhf(StreflopSimple x);
StreflopSimple acoshf(StreflopSimple x);
StreflopSimple asinhf(StreflopSimple x);
StreflopSimple atanhf(StreflopSimple x);

StreflopSimple fabsf(StreflopSimple x);
StreflopSimple floorf(StreflopSimple x);
StreflopSimple ceilf(StreflopSimple x);
StreflopSimple truncf(StreflopSimple x);
StreflopSimple fmodf(StreflopSimple x, StreflopSimple y);
StreflopSimple remainderf(StreflopSimple x, StreflopSimple y);
StreflopSimple remquof(StreflopSimple x, StreflopSimple y, int *quo);
StreflopSimple rintf(StreflopSimple x);
long int lrintf(StreflopSimple x);
long long int llrintf(StreflopSimple x);
StreflopSimple roundf(StreflopSimple x);
long int lroundf(StreflopSimple x);
long long int llroundf(StreflopSimple x);
StreflopSimple nearbyintf(StreflopSimple x);

StreflopSimple frexpf(StreflopSimple x, int *exp);
StreflopSimple ldexpf(StreflopSimple value, int exp);
StreflopSimple logbf(StreflopSimple x);
int ilogbf(StreflopSimple x);
StreflopSimple copysignf(StreflopSimple x);
int signbitf(StreflopSimple x);
StreflopSimple nextafterf(StreflopSimple x, StreflopSimple y);

StreflopSimple expm1f(StreflopSimple x);
StreflopSimple log1pf(StreflopSimple x);
StreflopSimple erff(StreflopSimple x);
StreflopSimple j0f(StreflopSimple x);
StreflopSimple j1f(StreflopSimple x);
StreflopSimple jnf(int n, StreflopSimple x);
StreflopSimple y0f(StreflopSimple x);
StreflopSimple y1f(StreflopSimple x);
StreflopSimple ynf(int n, StreflopSimple x);
StreflopSimple scalbnf(StreflopSimple x, int n);
StreflopSimple scalblnf(StreflopSimple x, long int n);

int fpclassifyf(StreflopSimple x);
int isnanf(StreflopSimple x);
int isinff(StreflopSimple x);
int isfinitef(StreflopSimple x);
int isnormalf(StreflopSimple x);

int isunorderedf(StreflopSimple x, StreflopSimple y);
int isgreaterf(StreflopSimple x, StreflopSimple y);
int isgreaterequalf(StreflopSimple x, StreflopSimple y);
int islessf(StreflopSimple x, StreflopSimple y);
int islessequalf(StreflopSimple x, StreflopSimple y);
int islessgreaterf(StreflopSimple x, StreflopSimple y);

StreflopDouble sqrt(StreflopDouble x);
StreflopDouble cbrt(StreflopDouble x);
StreflopDouble hypot(StreflopDouble x, StreflopDouble y);

StreflopDouble exp(StreflopDouble x);
StreflopDouble log(StreflopDouble x);
StreflopDouble log2(StreflopDouble x);
StreflopDouble exp2(StreflopDouble x);
StreflopDouble log10(StreflopDouble x);
StreflopDouble pow(StreflopDouble x, StreflopDouble y);

StreflopDouble sin(StreflopDouble x);
StreflopDouble cos(StreflopDouble x);
StreflopDouble tan(StreflopDouble x);
StreflopDouble acos(StreflopDouble x);
StreflopDouble asin(StreflopDouble x);
StreflopDouble atan(StreflopDouble x);
StreflopDouble atan2(StreflopDouble x, StreflopDouble y);

StreflopDouble cosh(StreflopDouble x);
StreflopDouble sinh(StreflopDouble x);
StreflopDouble tanh(StreflopDouble x);
StreflopDouble acosh(StreflopDouble x);
StreflopDouble asinh(StreflopDouble x);
StreflopDouble atanh(StreflopDouble x);

StreflopDouble fabs(StreflopDouble x);
StreflopDouble floor(StreflopDouble x);
StreflopDouble ceil(StreflopDouble x);
StreflopDouble trunc(StreflopDouble x);
StreflopDouble fmod(StreflopDouble x, StreflopDouble y);
StreflopDouble remainder(StreflopDouble x, StreflopDouble y);
StreflopDouble remquo(StreflopDouble x, StreflopDouble y, int *quo);
StreflopDouble rint(StreflopDouble x);
long int lrint(StreflopDouble x);
long long int llrint(StreflopDouble x);
StreflopDouble round(StreflopDouble x);
long int lround(StreflopDouble x);
long long int llround(StreflopDouble x);
StreflopDouble nearbyint(StreflopDouble x);

StreflopDouble frexp(StreflopDouble x, int *exp);
StreflopDouble ldexp(StreflopDouble value, int exp);
StreflopDouble logb(StreflopDouble x);
int ilogb(StreflopDouble x);
StreflopDouble copysign(StreflopDouble x);
int signbit(StreflopDouble x);
StreflopDouble nextafter(StreflopDouble x, StreflopDouble y);

StreflopDouble expm1(StreflopDouble x);
StreflopDouble log1p(StreflopDouble x);
StreflopDouble erf(StreflopDouble x);
StreflopDouble j0(StreflopDouble x);
StreflopDouble j1(StreflopDouble x);
StreflopDouble jn(int n, StreflopDouble x);
StreflopDouble y0(StreflopDouble x);
StreflopDouble y1(StreflopDouble x);
StreflopDouble yn(int n, StreflopDouble x);
StreflopDouble scalbn(StreflopDouble x, int n);
StreflopDouble scalbln(StreflopDouble x, long int n);

int fpclassify(StreflopDouble x);
int isnan(StreflopDouble x);
int isinf(StreflopDouble x);
inline int isfinite(StreflopDouble x) {return !(isnan(x) || isinf(x));}

// Stolen from math.h and inlined instead of macroized.
// Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
inline int isnormal(StreflopDouble x) {return fpclassify(x) == STREFLOP_FP_NORMAL;}

/** Generic C99 "macros" for unordered comparison
	Defined as inlined for each type, thanks to C++ overloading
*/
inline int isunordered(StreflopDouble x, StreflopDouble y) {
	return (fpclassify(x) == STREFLOP_FP_NAN) || (fpclassify (y) == STREFLOP_FP_NAN);
}
inline int isgreater(StreflopDouble x, StreflopDouble y) {
	return (!isunordered(x,y)) && (x > y);
}
inline int isgreaterequal(StreflopDouble x, StreflopDouble y) {
	return (!isunordered(x,y)) && (x >= y);
}
inline int isless(StreflopDouble x, StreflopDouble y) {
	return (!isunordered(x,y)) && (x < y);
}
inline int islessequal(StreflopDouble x, StreflopDouble y) {
	return (!isunordered(x,y)) && (x <= y);
}
inline int islessgreater(StreflopDouble x, StreflopDouble y) {
	return (!isunordered(x,y)) && ((x < y) || (x > y));
}

#ifdef __cplusplus
} // "C"
#endif

#endif // STREFLOP_C_H
