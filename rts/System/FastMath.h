/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FASTMATH_H
#define FASTMATH_H

#include "System/simd_compat.h"
#include <cinttypes>

// Tell streflop_cond.h not to define math::sqrt(float) - we'll provide a faster one
#define MATH_SQRT_OVERRIDE 1
#include "lib/streflop/streflop_cond.h"
#include "System/MainDefines.h"
#include "System/MathConstants.h"

#ifdef _MSC_VER
#define __builtin_sqrtf sqrt_sse
#endif

/**
 * @brief Fast math routines
 *
 * Contains faster alternatives for the more time
 * consuming standard math routines. These functions
 * are not as accurate, however, but are generally
 * acceptable for most applications.
 *
 */

namespace fastmath {
	/****************** Square root functions ******************/

	/**
	* @brief DEPRECATED - Use isqrt_nosse instead.
	*
	* Removed because:
	* - Much slower than isqrt_nosse on some AMD processors
	* - Produces different results on Intel vs AMD (non-deterministic)
	* - Unsafe for synced multiplayer code
	*/
	[[deprecated("Use isqrt_nosse instead")]]
	float isqrt_sse(float x) = delete;

	/**
	* @brief Sync-safe. Calculates square root using SSE instructions.
	*
	* Slower than std::sqrtf, much faster than streflop
	*/
	__FORCE_ALIGN_STACK__
	inline float sqrt_sse(float x)
	{
		__m128 vec = _mm_set_ss(x);
		vec = _mm_sqrt_ss(vec);
		return _mm_cvtss_f32(vec);
	}


	/**
	* @brief Calculates 1/sqrt(x) with less accuracy
	*
	* Gets a very good first guess and then uses one
	* iteration of newton's method for improved accuracy.
	* Average relative error: 0.093%
	* Highest relative error: 0.175%
	*
	* see "The Math Behind The Fast Inverse Square Root Function Code"
	* by Charles McEniry [2007] for a mathematical derivation of this
	* method (or Chris Lomont's 2003 "Fast Inverse Square Root" paper)
	*
	* It has been found to give slightly different results on different Intel CPUs.
	* Possible cause: 32bit vs 64bit. Use with care.
	*/
	inline float isqrt_nosse(float x) {
		float xh = 0.5f * x;
		std::int32_t i = *(std::int32_t*) &x;
		// "magic number" which makes a very good first guess
		i = 0x5f375a86 - (i >> 1);
		x = *(float*) &i;
		// Newton's method. One iteration for less accuracy but more speed.
		x = x * (1.5f - xh * (x * x));
		return x;
	}

	/**
	* @brief Calculates 1/sqrt(x) with more accuracy
	*
	* Gets a very good first guess and then uses two
	* iterations of newton's method for improved accuracy.
	* Average relative error: 0.00017%
	* Highest relative error: 0.00047%
	*
	*/
	inline float isqrt2_nosse(float x) {
		float xh = 0.5f * x;
		std::int32_t i = *(std::int32_t*) &x;
		// "magic number" which makes a very good first guess
		i = 0x5f375a86 - (i >> 1);
		x = *(float*) &i;
		// Newton's method. Two iterations for more accuracy but less speed.
		x = x * (1.5f - xh * (x * x));
		x = x * (1.5f - xh * (x * x));
		return x;

	}


	/****************** Square root ******************/


	/** Calculate sqrt using builtin sqrtf. (very fast) */
	inline float sqrt_builtin(float x)
	{
		return __builtin_sqrtf(x);
	}

	/**
	* @brief A (possibly very) inaccurate and numerically unstable, but fast, version of sqrt.
	*
	* Use with care.
	*/
	inline float apxsqrt(float x) {
		return x * fastmath::isqrt_nosse(x);
	}

	/**
	* @brief A (possibly very) inaccurate and numerically unstable, but fast, version of sqrt.
	*
	* Use with care. This should be a little bit better, albeit slower, than fastmath::sqrt.
	*/
	inline float apxsqrt2(float x) {
		return x * fastmath::isqrt2_nosse(x);
	}


	/****************** Trigonometric functions ******************/

	/**
	* @brief calculates the sine of x
	*
	* Range reduces x to -PI ... PI, and then uses the
	* sine approximation method as described at
	* http://www.devmaster.net/forums/showthread.php?t=5784
	*
	* Average percentage error: 0.15281632393574715%
	* Highest percentage error: 0.17455324009559584%
	*/
	inline float sin(float x) {
		/* range reduce to -PI ... PI, as the approximation
		method only works well for that range. */
		x = x - ((int)(x * math::INVPI2)) * math::TWOPI;
		if (x > math::HALFPI) {
			x = -x + math::PI;
		} else if (x < math::NEGHALFPI ) {
			x = -x - math::PI;
		}
		/* approximation */
		x = (math::PIU4) * x + (math::PISUN4) * x * math::fabs(x);
		x = 0.225f * (x * math::fabs(x) - x) + x;
		return x;
	}

	/**
	* @brief calculates the cosine of x
	*
	* Adds half of pi to x and then uses the sine method.
	*/
	inline float cos(float x) {
		return fastmath::sin(x + math::HALFPI);
	}


	/**
	* @brief fast version of std::floor
	*
	* About 2x faster than glibc ones.
	*
	* Unlike std::floor, this one returns the different (positive)
	* result for negative float zero input (0.0f).
	*/
	template<typename T>
	inline T floor(T f)
	{
		//return (f >= 0) ? int(f) : int(f+0.000001f)-1;
		// it's about the same performance as the former code above,
		// but without arbitratry number shenanigans
		// Perf comparison:
		// https://quick-bench.com/q/rwmaN33UJ4cTEViBGqQYuVgyyOc
		T truncX = static_cast<T>(static_cast<int>(f));
		return truncX - static_cast<T>(truncX > f);
	}
}


namespace math {
	template<typename T>
	inline float sqrt(T x) { return fastmath::sqrt_sse(static_cast<float>(x)); }
	template<typename T>
	inline float sqrtf(T x) { return fastmath::sqrt_sse(static_cast<float>(x)); }
	template<typename T>
	inline float isqrt(T x) { return fastmath::isqrt2_nosse(static_cast<float>(x)); }
	using fastmath::floor;
}

#endif
