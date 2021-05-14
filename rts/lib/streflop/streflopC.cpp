/* Copyright (C) 2009 Tobi Vollebregt */

#include "streflopC.h"

#include "streflop_cond.h"

#ifdef __cplusplus
extern "C" {
#endif

void streflop_init_StreflopSimple() {
	streflop::streflop_init<StreflopSimple>();
}

void streflop_init_StreflopDouble() {
	streflop::streflop_init<StreflopDouble>();
}

StreflopSimple sqrtf(StreflopSimple x) {return streflop::sqrtf(x);}
StreflopSimple cbrtf(StreflopSimple x) {return streflop::cbrtf(x);}
StreflopSimple hypotf(StreflopSimple x, StreflopSimple y) {return streflop::hypotf(x,y);}

StreflopSimple expf(StreflopSimple x) {return streflop::expf(x);}
StreflopSimple logf(StreflopSimple x) {return streflop::logf(x);}
StreflopSimple log2f(StreflopSimple x) {return streflop::log2f(x);}
StreflopSimple exp2f(StreflopSimple x) {return streflop::exp2f(x);}
StreflopSimple log10f(StreflopSimple x) {return streflop::log10f(x);}
StreflopSimple powf(StreflopSimple x, StreflopSimple y) {return streflop::powf(x,y);}

StreflopSimple sinf(StreflopSimple x) {return streflop::sinf(x);}
StreflopSimple cosf(StreflopSimple x) {return streflop::cosf(x);}
StreflopSimple tanf(StreflopSimple x) {return streflop::tanf(x);}
StreflopSimple acosf(StreflopSimple x) {return streflop::acosf(x);}
StreflopSimple asinf(StreflopSimple x) {return streflop::asinf(x);}
StreflopSimple atanf(StreflopSimple x) {return streflop::atanf(x);}
StreflopSimple atan2f(StreflopSimple x, StreflopSimple y) {return streflop::atan2f(x,y);}

StreflopSimple coshf(StreflopSimple x) {return streflop::coshf(x);}
StreflopSimple sinhf(StreflopSimple x) {return streflop::sinhf(x);}
StreflopSimple tanhf(StreflopSimple x) {return streflop::tanhf(x);}
StreflopSimple acoshf(StreflopSimple x) {return streflop::acoshf(x);}
StreflopSimple asinhf(StreflopSimple x) {return streflop::asinhf(x);}
StreflopSimple atanhf(StreflopSimple x) {return streflop::atanhf(x);}

StreflopSimple fabsf(StreflopSimple x) {return streflop::fabsf(x);}
StreflopSimple floorf(StreflopSimple x) {return streflop::floorf(x);}
StreflopSimple ceilf(StreflopSimple x) {return streflop::ceilf(x);}
StreflopSimple truncf(StreflopSimple x) {return streflop::truncf(x);}
StreflopSimple fmodf(StreflopSimple x, StreflopSimple y) {return streflop::fmodf(x,y);}
StreflopSimple remainderf(StreflopSimple x, StreflopSimple y) {return streflop::remainderf(x,y);}
StreflopSimple remquof(StreflopSimple x, StreflopSimple y, int *quo) {return streflop::remquof(x,y,quo);}
StreflopSimple rintf(StreflopSimple x) {return streflop::rintf(x);}
long int lrintf(StreflopSimple x) {return streflop::lrintf(x);}
long long int llrintf(StreflopSimple x) {return streflop::llrintf(x);}
StreflopSimple roundf(StreflopSimple x) {return streflop::roundf(x);}
long int lroundf(StreflopSimple x) {return streflop::lroundf(x);}
long long int llroundf(StreflopSimple x) {return streflop::llroundf(x);}
StreflopSimple nearbyintf(StreflopSimple x) {return streflop::nearbyintf(x);}

StreflopSimple frexpf(StreflopSimple x, int *exp) {return streflop::frexpf(x,exp);}
StreflopSimple ldexpf(StreflopSimple value, int exp) {return streflop::ldexpf(value,exp);}
StreflopSimple logbf(StreflopSimple x) {return streflop::logbf(x);}
int ilogbf(StreflopSimple x) {return streflop::ilogbf(x);}
StreflopSimple copysignf(StreflopSimple x) {return streflop::copysignf(x);}
int signbitf(StreflopSimple x) {return streflop::signbitf(x);}
StreflopSimple nextafterf(StreflopSimple x, StreflopSimple y) {return streflop::nextafterf(x,y);}

StreflopSimple expm1f(StreflopSimple x) {return streflop::expm1f(x);}
StreflopSimple log1pf(StreflopSimple x) {return streflop::log1pf(x);}
StreflopSimple erff(StreflopSimple x) {return streflop::erff(x);}
StreflopSimple j0f(StreflopSimple x) {return streflop::j0f(x);}
StreflopSimple j1f(StreflopSimple x) {return streflop::j1f(x);}
StreflopSimple jnf(int n, StreflopSimple x) {return streflop::jnf(n,x);}
StreflopSimple y0f(StreflopSimple x) {return streflop::y0f(x);}
StreflopSimple y1f(StreflopSimple x) {return streflop::y1f(x);}
StreflopSimple ynf(int n, StreflopSimple x) {return streflop::ynf(n,x);}
StreflopSimple scalbnf(StreflopSimple x, int n) {return streflop::scalbnf(x,n);}
StreflopSimple scalblnf(StreflopSimple x, long int n) {return streflop::scalblnf(x,n);}

int fpclassifyf(StreflopSimple x) {return streflop::fpclassifyf(x) ? 1 : 0;}
int isnanf(StreflopSimple x) {return streflop::isnanf(x) ? 1 : 0;}
int isinff(StreflopSimple x) {return streflop::isinff(x) ? 1 : 0;}

StreflopDouble sqrt(StreflopDouble x) {return streflop::sqrt(x);}
StreflopDouble cbrt(StreflopDouble x) {return streflop::cbrt(x);}
StreflopDouble hypot(StreflopDouble x, StreflopDouble y) {return streflop::hypot(x,y);}

StreflopDouble exp(StreflopDouble x) {return streflop::exp(x);}
StreflopDouble log(StreflopDouble x) {return streflop::log(x);}
StreflopDouble log2(StreflopDouble x) {return streflop::log2(x);}
StreflopDouble exp2(StreflopDouble x) {return streflop::exp2(x);}
StreflopDouble log10(StreflopDouble x) {return streflop::log10(x);}
StreflopDouble pow(StreflopDouble x, StreflopDouble y) {return streflop::pow(x,y);}

StreflopDouble sin(StreflopDouble x) {return streflop::sin(x);}
StreflopDouble cos(StreflopDouble x) {return streflop::cos(x);}
StreflopDouble tan(StreflopDouble x) {return streflop_libm::tan(x);}
StreflopDouble acos(StreflopDouble x) {return streflop::acos(x);}
StreflopDouble asin(StreflopDouble x) {return streflop::asin(x);}
StreflopDouble atan(StreflopDouble x) {return streflop_libm::atan(x);}
StreflopDouble atan2(StreflopDouble x, StreflopDouble y) {return streflop::atan2(x,y);}

StreflopDouble cosh(StreflopDouble x) {return streflop::cosh(x);}
StreflopDouble sinh(StreflopDouble x) {return streflop::sinh(x);}
StreflopDouble tanh(StreflopDouble x) {return streflop::tanh(x);}
StreflopDouble acosh(StreflopDouble x) {return streflop::acosh(x);}
StreflopDouble asinh(StreflopDouble x) {return streflop::asinh(x);}
StreflopDouble atanh(StreflopDouble x) {return streflop::atanh(x);}

StreflopDouble fabs(StreflopDouble x) {return streflop::fabs(x);}
StreflopDouble floor(StreflopDouble x) {return streflop::floor(x);}
StreflopDouble ceil(StreflopDouble x) {return streflop::ceil(x);}
StreflopDouble trunc(StreflopDouble x) {return streflop::trunc(x);}
StreflopDouble fmod(StreflopDouble x, StreflopDouble y) {return streflop::fmod(x,y);}
StreflopDouble remainder(StreflopDouble x, StreflopDouble y) {return streflop::remainder(x,y);}
StreflopDouble remquo(StreflopDouble x, StreflopDouble y, int *quo) {return streflop::remquo(x,y,quo);}
StreflopDouble rint(StreflopDouble x) {return streflop::rint(x);}
long int lrint(StreflopDouble x) {return streflop::lrint(x);}
long long int llrint(StreflopDouble x) {return streflop::llrint(x);}
StreflopDouble round(StreflopDouble x) {return streflop::round(x);}
long int lround(StreflopDouble x) {return streflop::lround(x);}
long long int llround(StreflopDouble x) {return streflop::llround(x);}
StreflopDouble nearbyint(StreflopDouble x) {return streflop::nearbyint(x);}

StreflopDouble frexp(StreflopDouble x, int *exp) {return streflop::frexp(x, exp);}
StreflopDouble ldexp(StreflopDouble value, int exp) {return streflop::ldexp(value,exp);}
StreflopDouble logb(StreflopDouble x) {return streflop::logb(x);}
int ilogb(StreflopDouble x) {return streflop::ilogb(x);}
StreflopDouble copysign(StreflopDouble x) {return streflop::copysign(x);}
int signbit(StreflopDouble x) {return streflop::signbit(x);}
StreflopDouble nextafter(StreflopDouble x, StreflopDouble y) {return streflop::nextafter(x,y);}

StreflopDouble expm1(StreflopDouble x) {return streflop::expm1(x);}
StreflopDouble log1p(StreflopDouble x) {return streflop::log1p(x);}
StreflopDouble erf(StreflopDouble x) {return streflop::erf(x);}
StreflopDouble j0(StreflopDouble x) {return streflop::j0(x);}
StreflopDouble j1(StreflopDouble x) {return streflop::j1(x);}
StreflopDouble jn(int n, StreflopDouble x) {return streflop::jn(n,x);}
StreflopDouble y0(StreflopDouble x) {return streflop::y0(x);}
StreflopDouble y1(StreflopDouble x) {return streflop::y1(x);}
StreflopDouble yn(int n, StreflopDouble x) {return streflop::yn(n,x);}
StreflopDouble scalbn(StreflopDouble x, int n) {return streflop::scalbn(x,n);}
StreflopDouble scalbln(StreflopDouble x, long int n) {return streflop::scalbln(x,n);}

int fpclassify(StreflopDouble x) {return streflop::fpclassify(x);}
int isnan(StreflopDouble x) {return streflop::isnan(x);}
int isinf(StreflopDouble x) {return streflop::isinf(x);}

#ifdef __cplusplus
} // extern "C"
#endif
