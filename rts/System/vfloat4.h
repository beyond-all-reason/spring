/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VFLOAT4_H__
#define VFLOAT4_H__

#include <immintrin.h>

#include <cassert>

#include "System/BranchPrediction.h"
#include "lib/streflop/streflop_cond.h"
#include "System/creg/creg_cond.h"
#include "System/FastMath.h"
#ifdef _MSC_VER
#include "System/Platform/Win/win32.h"
#endif

/**
 * @brief vfloat4 class
 *
 * Contains a set of 3 float numbers.
 * Usually used to represent a vector in
 * space as x/y/z.
 */
class vfloat4
{
public:
	CR_DECLARE_STRUCT(vfloat4)


	/**
	 * @brief default Constructor
	 * With parameters, initializes x/y/z to 0.0f.
	 */
	constexpr vfloat4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

	/**
	 * @brief Constructor
	 * @param x float x
	 * @param y float y
	 * @param z float z
	 *
	 * With parameters, initializes x/y/z to the given floats.
	 */
	constexpr vfloat4(const float x, const float y, const float z)
			: x(x), y(y), z(z), w(0.0f) {}

	/**
	 * @brief float[3] Constructor
	 * @param f float[3] to assign
	 *
	 * With parameters, initializes x/y/z to the given float[3].
	 */
	constexpr vfloat4(const float f[3]) : x(f[0]), y(f[1]), z(f[2]), w(0.0f) {}

	//constexpr vfloat4(const float f[4], int _ignored) : x(f[0]), y(f[1]), z(f[2]), w(f[3]) {}

	vfloat4(const __m128 f) { _mm_storeu_ps(xyzw, f); }

	/**
	 * @brief operator =
	 * @param f float[3] to assign
	 *
	 * Sets the vfloat4 to the given float[3].
	 */
	vfloat4& operator= (const float f[3]) {
		x = f[0];
		y = f[1];
		z = f[2];
		return *this;
	}

	/**
	 * @brief Copy x, y, z into float[3]
	 * @param f float[3] to copy values into
	 *
	 * Sets the float[3] to this vfloat4.
	 */
	void copyInto(float f[3]) const {
		f[0] = x;
		f[1] = y;
		f[2] = z;
	}


	/**
	 * @brief operator +
	 * @param f vfloat4 reference to add.
	 * @return sum of vfloat4s
	 *
	 * When adding another vfloat4, will
	 * calculate the sum of the positions in
	 * space (adds the x/y/z components individually)
	 */
	vfloat4 operator+ (const vfloat4& f) const {
		//float res[4];

		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_add_ps(a, b);
		//_mm_storeu_ps(res, result);

		return vfloat4(result);
	}

	/**
	 * @brief operator +
	 * @return sum of vfloat4+float
	 * @param f single float to add
	 *
	 * When adding just a float, the point is
	 * increased in all directions by that float.
	 */
	vfloat4 operator+ (const float f) const {
		//float res[4];

		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_set_ps1(f);
		__m128 result = _mm_add_ps(a, b);
		//_mm_storeu_ps(res, result);

		return vfloat4(result);
	}

	/**
	 * @brief operator +=
	 * @param f vfloat4 reference to add.
	 *
	 * Just like adding a vfloat4, but updates this
	 * vfloat4 with the new sum.
	 */
	vfloat4& operator+= (const vfloat4& f) {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_add_ps(a, b);
		_mm_storeu_ps(xyzw, result);

		return *this;
	}

	/**
	 * @brief operator -
	 * @param f vfloat4 to subtract
	 * @return difference of vfloat4s
	 *
	 * Decreases the vfloat4 by another vfloat4,
	 * subtracting each x/y/z component individually.
	 */
	vfloat4 operator- (const vfloat4& f) const {
		//float res[4];

		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_sub_ps(a, b);
		//_mm_storeu_ps(res, result);

		return vfloat4(result);
	}

	/**
	 * @brief operator -
	 * @return difference of vfloat4 and float
	 * @param f float to subtract
	 *
	 * When subtracting a single fixed float,
	 * decreases all three x/y/z components by that amount.
	 */
	vfloat4 operator- (const float f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_set_ps1(f);
		__m128 result = _mm_sub_ps(a, b);

		return vfloat4(result);
	}

	/**
	 * @brief operator -=
	 * @param f vfloat4 to subtract
	 *
	 * Same as subtracting a vfloat4, but stores
	 * the new vfloat4 inside this one.
	 */
	void operator-= (const vfloat4& f) {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_sub_ps(a, b);
		_mm_storeu_ps(xyzw, result);
	}


	/**
	 * @brief operator -
	 * @return inverted vfloat4
	 *
	 * When negating the vfloat4, inverts all three
	 * x/y/z components.
	 */
	vfloat4 operator- () const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_set_ps1(-1.0f);
		__m128 result = _mm_mul_ps(a, b);

		return vfloat4(result);
	}


	/**
	 * @brief operator *
	 * @param f vfloat4 to multiply
	 * @return product of vfloat4s
	 *
	 * When multiplying by another vfloat4,
	 * multiplies each x/y/z component individually.
	 */
	vfloat4 operator* (const vfloat4& f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_mul_ps(a, b);

		return vfloat4(result);
	}

	/**
	 * @brief operator *
	 * @param f float to multiply
	 * @return product of vfloat4 and float
	 *
	 * When multiplying by a single float, multiplies
	 * each x/y/z component by that float.
	 */
	vfloat4 operator* (const float f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_set_ps1(f);
		__m128 result = _mm_mul_ps(a, b);

		return vfloat4(result);
	}

	/**
	 * @brief operator *=
	 * @param f vfloat4 to multiply
	 *
	 * Same as multiplying a vfloat4, but stores
	 * the new vfloat4 inside this one.
	 */
	void operator*= (const vfloat4& f) {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_mul_ps(a, b);
		_mm_storeu_ps(xyzw, result);
	}

	/**
	 * @brief operator *=
	 * @param f float to multiply
	 *
	 * Same as multiplying a float, but stores
	 * the new vfloat4 inside this one.
	 */
	vfloat4& operator*= (const float f) {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_set_ps1(f);
		__m128 result = _mm_mul_ps(a, b);
		_mm_storeu_ps(xyzw, result);

		return *this;
	}

	/**
	 * @brief operator /
	 * @param f vfloat4 to divide
	 * @return divided vfloat4
	 *
	 * When dividing by a vfloat4, divides
	 * each x/y/z component individually.
	 */
	vfloat4 operator/ (const vfloat4& f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_div_ps(a, b);
		
		return vfloat4(result);
	}

	/**
	 * @brief operator /
	 * @param f float to divide
	 * @return vfloat4 divided by float
	 *
	 * When dividing by a single float, divides
	 * each x/y/z component by that float.
	 */
	vfloat4 operator/ (const float f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_set_ps1(f);
		__m128 result = _mm_div_ps(a, b);

		return vfloat4(result);
	}

	/**
	 * @brief operator /=
	 * @param f vfloat4 to divide
	 *
	 * Same as dividing by a vfloat4, but stores
	 * the new values inside this vfloat4.
	 */
	void operator/= (const vfloat4& f) {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);
		__m128 result = _mm_div_ps(a, b);
		_mm_storeu_ps(xyzw, result);
	}

	/**
	 * @brief operator /=
	 * @param f float to divide
	 *
	 * Same as dividing by a single float, but stores
	 * the new values inside this vfloat4.
	 */
	void operator/= (const float f) {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_set_ps1(f);
		__m128 result = _mm_div_ps(a, b);
		_mm_storeu_ps(xyzw, result);
	}


	/**
	 * @brief operator ==
	 * @param f vfloat4 to test
	 * @return whether vfloat4s are equal under default cmp_eps tolerance in x/y/z
	 *
	 * Tests if this vfloat4 is equal to another, by
	 * checking each x/y/z component individually.
	 */
	bool operator== (const vfloat4& f) const {
		return (equals(f));
	}

	/**
	 * @brief operator !=
	 * @param f vfloat4 to test
	 * @return whether vfloat4s are not equal
	 *
	 * Tests if this vfloat4 is not equal to another, by
	 * checking each x/y/z component individually.
	 */
	bool operator!= (const vfloat4& f) const {
		return (!equals(f));
	}


	/**
	 * @brief operator[]
	 * @param t index in xyz array
	 * @return float component at index
	 *
	 * Array access for x/y/z components
	 * (index 0 is x, index 1 is y, index 2 is z)
	 */
	float& operator[] (const int t) {
		return (&x)[t];
	}

	/**
	 * @brief operator[] const
	 * @param t index in xyz array
	 * @return const float component at index
	 *
	 * Same as plain [] operator but used in
	 * a const context
	 */
	const float& operator[] (const int t) const {
		return (&x)[t];
	}

	/**
	 * @see operator==
	 */
	bool equals(const vfloat4& f, const vfloat4& eps = vfloat4(cmp_eps(), cmp_eps(), cmp_eps())) const;


	/**
	 * @brief binary vfloat4 equality
	 * @param f vfloat4 to compare to
	 * @return const whether the two vfloat4 are binary same
	 *
	 */
	bool same(const vfloat4& f) const {
		return x == f.x && y == f.y && z == f.z;
	}

	/**
	 * @brief dot product
	 * @param f vfloat4 to use
	 * @return dot product of vfloat4s
	 *
	 * Calculates the dot product of this and
	 * another vfloat4 (sums the products of each
	 * x/y/z component).
	 */
	float dot(const vfloat4& f) const {
		// return (x * f.x) + (y * f.y) + (z * f.z);
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);

		__m128 v = _mm_mul_ps(a, b);

		// Combine values into a scalar
		__m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1));
		__m128 sums = _mm_add_ss(v, shuf);
		       shuf = _mm_movehl_ps(v, v);
		       sums = _mm_add_ss(sums, shuf);

		// This would be the SSE 4.1 version
		//__m128 result = _mm_dp_ps(a, b, 0x71); /* 7 = (0111) parts to dot product, 1 = (0001) store result */

		return _mm_cvtss_f32(sums);
	}

	/**
	 * @brief dot2D product
	 * @param f vfloat4 to use
	 * @return 2D dot product of vfloat4s
	 *
	 * Calculates the 2D dot product of this and
	 * another vfloat4 (sums the products of
	 * x/z components).
	 */
	float dot2D(const vfloat4& f) const {
		return (x * f.x) + (z * f.z);
	}

	/**
	 * @brief cross product
	 * @param f vfloat4 to use
	 * @return cross product of two vfloat4s
	 *
	 * Calculates the cross product of this and
	 * another vfloat4:
	 * (y1*z2 - z1*y2, z1*x2 - x1*z2, x1*y2 - y1*x2)
	 * 
	 * Ref: Method #5 from https://geometrian.com/programming/tutorials/cross-product/index.php
	 */
	vfloat4 cross(const vfloat4& f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);

		__m128 a1 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3,0,2,1));
		__m128 b1 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3,1,0,2));

		__m128 tmp = _mm_mul_ps(a1, b);
		__m128 a2 = _mm_mul_ps(a1, b1);
		__m128 b2 = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(3,0,2,1));

		return vfloat4(_mm_sub_ps(a2, b2));

		// Original Code
		// return vfloat4(
		// 		(y * f.z) - (z * f.y),
		// 		(z * f.x) - (x * f.z),
		// 		(x * f.y) - (y * f.x));

		// SSE Model
		// tmp       | (a2)        (b2)
		// (a1* b)   | (a1* b1)    (tmp shuffled)
		// (y * f.x) | (y * f.z) - (z * f.y)
		// (z * f.y) | (z * f.x) - (x * f.z)
		// (x * f.z) | (x * f.y) - (y * f.x)
	}

	// float hsum_ps_sse1(__m128 v) {                                  // v = [ D C | B A ]
	// 	__m128 shuf   = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));  // [ C D | A B ]
	// 	__m128 sums   = _mm_add_ps(v, shuf);      // sums = [ D+C C+D | B+A A+B ]
	// 	shuf          = _mm_movehl_ps(shuf, sums);      //  [   C   D | D+C C+D ]  // let the compiler avoid a mov by reusing shuf
	// 	sums          = _mm_add_ss(sums, shuf);
	// 	return    _mm_cvtss_f32(sums);
	// }

	vfloat4 rotate(float angle, const vfloat4& axis) {
		const float ca = math::cos(angle);
		const float sa = math::sin(angle);

		//Rodrigues' rotation formula
		// https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
		return (*this) * ca + axis.cross(*this) * sa + axis * axis.dot(*this) * (1.0f - ca);
	}

	/**
	 * @brief distance between vfloat4s
	 * @param f vfloat4 to compare against
	 * @return float distance between vfloat4s
	 *
	 * Calculates the distance between this vfloat4
	 * and another vfloat4 (sums the differences in each
	 * x/y/z component, square root for pythagorean theorem)
	 */
	float distance(const vfloat4& f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);

		__m128 delta = _mm_sub_ps(a, b);
		__m128 v = _mm_mul_ps(delta, delta);

		// Combine values into a scalar
		__m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1));
		__m128 sums = _mm_add_ss(v, shuf);
		       shuf = _mm_movehl_ps(v, v);
		       sums = _mm_add_ss(sums, shuf);

		return _mm_cvtss_f32(_mm_sqrt_ss(sums));

		// const float dx = x - f.x;
		// const float dy = y - f.y;
		// const float dz = z - f.z;
		// return math::sqrt(dx*dx + dy*dy + dz*dz);
	}

	/**
	 * @brief distance2D between vfloat4s (only x and z)
	 * @param f vfloat4 to compare against
	 * @return 2D distance between vfloat4s
	 *
	 * Calculates the distance between this vfloat4
	 * and another vfloat4 2-dimensionally (that is,
	 * only using the x and z components).  Sums the
	 * differences in the x and z components, square
	 * root for pythagorean theorem
	 */
	float distance2D(const vfloat4& f) const {
		const float dx = x - f.x;
		const float dz = z - f.z;
		return math::sqrt(dx*dx + dz*dz);
	}


	/**
	 * @brief SqDistance between vfloat4s squared
	 * @param f vfloat4 to compare against
	 * @return float squared distance between vfloat4s
	 *
	 * Returns the squared distance of 2 vfloat4s
	 */
	float SqDistance(const vfloat4& f) const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 b = _mm_loadu_ps(f.xyzw);

		__m128 delta = _mm_sub_ps(a, b);
		__m128 v = _mm_mul_ps(delta, delta);

		// Combine values into a scalar
		__m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1));
		__m128 sums = _mm_add_ss(v, shuf);
		       shuf = _mm_movehl_ps(v, v);
		       sums = _mm_add_ss(sums, shuf);

		return _mm_cvtss_f32(sums);

		// const float dx = x - f.x;
		// const float dy = y - f.y;
		// const float dz = z - f.z;
		// return (dx*dx + dy*dy + dz*dz);
	}

	/**
	 * @brief SqDistance2D between vfloat4s (only x and z)
	 * @param f vfloat4 to compare against
	 * @return 2D squared distance between vfloat4s
	 *
	 * Returns the squared 2d-distance of 2 vfloat4s
	 */
	float SqDistance2D(const vfloat4& f) const {
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (dx*dx + dz*dz);
	}


	/**
	 * @brief Length of this vector
	 * @return float length of vector
	 *
	 * Returns the length of this vector
	 * (squares and sums each x/y/z component,
	 * square root for pythagorean theorem)
	 */
	float Length() const {
		//assert(x!=0.f || y!=0.f || z!=0.f);

		__m128 a = _mm_loadu_ps(xyzw);
		__m128 v = _mm_mul_ps(a, a);

		// Combine values into a scalar
		__m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1));
		__m128 sums = _mm_add_ss(v, shuf);
		       shuf = _mm_movehl_ps(v, v);
		       sums = _mm_add_ss(sums, shuf);

		return _mm_cvtss_f32(_mm_sqrt_ss(sums));

		// return math::sqrt(SqLength());
	}

	// vec4 version
	// __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1));
	// __m128 sums = _mm_add_ps(v, shuf);
	//        shuf = _mm_movehl_ps(sums, sums);
	//        sums = _mm_add_ss(sums, shuf);

	/**
	 * @brief 2-dimensional length of this vector
	 * @return 2D float length of vector
	 *
	 * Returns the 2-dimensional length of this vector
	 * (squares and sums only the x and z components,
	 * square root for pythagorean theorem)
	 */
	float Length2D() const {
		//assert(x!=0.f || y!=0.f || z!=0.f);
		return math::sqrt(SqLength2D());
	}

	/**
	 * @brief length squared
	 * @return length squared
	 *
	 * Returns the length of this vector squared.
	 */
	float SqLength() const {
		__m128 a = _mm_loadu_ps(xyzw);
		__m128 v = _mm_mul_ps(a, a);

		// Combine values into a scalar
		__m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1));
		__m128 sums = _mm_add_ss(v, shuf);
		       shuf = _mm_movehl_ps(v, v);
		       sums = _mm_add_ss(sums, shuf);

		return _mm_cvtss_f32(sums);

		// return (x*x + y*y + z*z);
	}

	/**
	 * @brief 2-dimensional length squared
	 * @return 2D length squared
	 *
	 * Returns the 2-dimensional length of this
	 * vector squared.
	 */
	float SqLength2D() const {
		return (x*x + z*z);
	}

	/**
	 * normalize vector in-place, return its old length
	 */
	float LengthNormalize() {
		const float len = Length();

		if (likely(len > nrm_eps()))
			(*this) *= (1.0f / len);

		return len;
	}

	float LengthNormalize2D() {
		y = 0.0f; return LengthNormalize();
	}


	/**
	 * @brief normalizes the vector using one of Normalize implementations
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	vfloat4& Normalize() {
#if defined(__SUPPORT_SNAN__)
#ifndef BUILDING_AI
		return SafeNormalize();
#endif
		return UnsafeNormalize();
#else
		return SafeNormalize();
#endif
	}

	vfloat4& Normalize2D() {
		y = 0.0f; return Normalize();
	}


	/**
	 * @brief normalizes the vector without checking for zero vector
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	vfloat4& UnsafeNormalize() {
		float len = SqLength();

		__m128 length = _mm_load_ss(&len);
		__m128 isqrt = _mm_rsqrt_ss(length);
		float normv = _mm_cvtss_f32(isqrt);

		return ((*this) *= normv);

		//return ((*this) *= math::isqrt(SqLength()));
	}

	vfloat4& UnsafeNormalize2D() {
		y = 0.0f; return UnsafeNormalize();
	}


	/**
	 * @brief normalizes the vector safely (check for *this == ZeroVector)
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	vfloat4& SafeNormalize() {
		const float sql = SqLength();

		if (likely(sql > nrm_eps())){
			__m128 length = _mm_load_ss(&sql);
			__m128 isqrt = _mm_rsqrt_ss(length);
			float normv = _mm_cvtss_f32(isqrt);

			(*this) *= normv;
		}
		//	(*this) *= math::isqrt(sql);

		return *this;
	}

	vfloat4& SafeNormalize2D() {
		y = 0.0f; return SafeNormalize();
	}


	/**
	 * @brief normalizes the vector approximately
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each x/y/z component by
	 * the vector's approx. length.
	 */
	vfloat4& ANormalize() {
#if defined(__SUPPORT_SNAN__)
#ifndef BUILDING_AI
		return SafeANormalize();
#endif
		return UnsafeANormalize();
#else
		return SafeANormalize();
#endif
	}

	vfloat4& ANormalize2D() {
		y = 0.0f; return ANormalize();
	}


	/**
	 * @brief normalizes the vector approximately without checking
	 *        for ZeroVector
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each x/y/z component by
	 * the vector's approx. length.
	 */
	vfloat4& UnsafeANormalize() {
		//assert(SqLength() > nrm_eps());

		float sql = SqLength();
		assert(sql > nrm_eps());

		__m128 length = _mm_load_ss(&sql);
		__m128 isqrt = _mm_rsqrt_ss(length);
		float normv = _mm_cvtss_f32(isqrt);

		return ((*this) *= normv);

		// return ((*this) *= math::isqrt(SqLength()));
	}

	vfloat4& UnsafeANormalize2D() {
		y = 0.0f; return UnsafeANormalize();
	}


	/**
	 * @brief normalizes the vector approximately and safely
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each x/y/z component by
	 * the vector's approximate length, if (this != ZeroVector),
	 * else do nothing.
	 */
	vfloat4& SafeANormalize() {
		const float sql = SqLength();

		if (likely(sql > nrm_eps())){
			__m128 length = _mm_load_ss(&sql);
			__m128 isqrt = _mm_rsqrt_ss(length);
			float normv = _mm_cvtss_f32(isqrt);

			(*this) *= normv;
		}
		//	(*this) *= math::isqrt(sql);

		return *this;
	}

	vfloat4& SafeANormalize2D() {
		y = 0.0f; return SafeANormalize();
	}


	static bool CheckNaN(float c) { return (!math::isnan(c) && !math::isinf(c)); }

	bool CheckNaNs() const { return (CheckNaN(x) && CheckNaN(y) && CheckNaN(z)); }
	void AssertNaNs() const {
		assert(CheckNaN(x));
		assert(CheckNaN(y));
		assert(CheckNaN(z));
	}


	/**
	 * @brief Check against FaceHeightmap bounds
	 *
	 * Check if this vector is in bounds [0 .. mapDims.mapxy-1]
	 * @note THIS IS THE WRONG SPACE! _ALL_ WORLD SPACE POSITIONS SHOULD BE IN VertexHeightmap RESOLUTION!
	 * @see #IsInMap
	 */
	bool IsInBounds() const;
	/**
	 * @brief Check against FaceHeightmap bounds
	 *
	 * Check if this vector is in map [0 .. mapDims.mapxy]
	 * @note USE THIS!
	 */
	bool IsInMap() const;


	/**
	 * @brief Clamps to FaceHeightmap
	 *
	 * Clamps to the `face heightmap` resolution [0 .. mapDims.mapxy-1] * SQUARE_SIZE
	 * @note THIS IS THE WRONG SPACE! _ALL_ WORLD SPACE POSITIONS SHOULD BE IN VertexHeightmap RESOLUTION!
	 * @deprecated  use ClampInMap instead, but see the note!
	 * @see #ClampInMap
	 */
	void ClampInBounds();

	/**
	 * @brief Clamps to VertexHeightmap
	 *
	 * Clamps to the `vertex heightmap`/`opengl space` resolution [0 .. mapDims.mapxy] * SQUARE_SIZE
	 * @note USE THIS!
	 */
	void ClampInMap();

	vfloat4 cClampInBounds() const { vfloat4 f = *this; f.ClampInBounds(); return f; }
	vfloat4 cClampInMap() const { vfloat4 f = *this; f.ClampInMap(); return f; }

	static vfloat4 min(const vfloat4 v1, const vfloat4 v2);
	static vfloat4 max(const vfloat4 v1, const vfloat4 v2);
	static vfloat4 fabs(const vfloat4 v);
	static vfloat4 sign(const vfloat4 v);

	static constexpr float cmp_eps() { return 1e-04f; }
	static constexpr float nrm_eps() { return 1e-12f; }

	/**
	 * @brief max x pos
	 *
	 * Static value containing the maximum x position (:= mapDims.mapx-1)
	 * @note maxxpos is set after loading the map.
	 */
	static float maxxpos;

	/**
	 * @brief max z pos
	 *
	 * Static value containing the maximum z position (:= mapDims.mapy-1)
	 * @note maxzpos is set after loading the map.
	 */
	static float maxzpos;


public:
	union {
		struct { float x,y,z,w; };
		struct { float r,g,b,a; };
		struct { float x1,y1,x2,y2; };
		struct { float s,t,p,q; };
		struct { float xstart, ystart, xend, yend; };
		struct { float xyz[3]; };
		struct { float xyzw[4]; };
	};
};

#endif /* vfloat4SSE_H */

