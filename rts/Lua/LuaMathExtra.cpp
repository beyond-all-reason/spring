/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "lib/streflop/streflop_cond.h"
#include "System/SpringMath.h"
#include "LuaMathExtra.h"
#include "LuaInclude.h"
#include "LuaUtils.h"

#include "System/Misc/TracyDefs.h"

static const lua_Number POWERS_OF_TEN[] = {1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f, 10000000.0f};

/******************************************************************************
 * math extensions
 *
 * Note: there are no bit shift. Use those Lua functions instead for 24 bits bitshift
 * 24 bits because only the 24 bits of the mantissa can be easily used in a 32 bit float
 * bitshift functions (<<, >> equivalent)
 *
 *     -- left shift
 *     local function lsh(value,shift)
 *         return (value*(2^shift)) % 2^24
 *     end
 *
 *     -- right shift
 *     local function rsh(value,shift)
 *         return math.floor(value/2^shift) % 2^24
 *     end
 *
 * @see rts/Lua/LuaMathExtra.cpp
******************************************************************************/

bool LuaMathExtra::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "hypot",  hypot);
	LuaPushNamedCFunc(L, "diag",   diag);
	LuaPushNamedCFunc(L, "clamp",  clamp);
	LuaPushNamedCFunc(L, "sgn",    sgn);
	LuaPushNamedCFunc(L, "mix",    mix);
	LuaPushNamedCFunc(L, "round",  round);
	LuaPushNamedCFunc(L, "erf",    erf);
	LuaPushNamedCFunc(L, "smoothstep", smoothstep);
	LuaPushNamedCFunc(L, "normalize", normalize);

	LuaPushNamedCFunc(L, "bit_or",   bit_or);
	LuaPushNamedCFunc(L, "bit_and",  bit_and);
	LuaPushNamedCFunc(L, "bit_xor",  bit_xor);
	LuaPushNamedCFunc(L, "bit_inv",  bit_inv);
	LuaPushNamedCFunc(L, "bit_bits", bit_bits);

	return true;
}


/******************************************************************************/
/******************************************************************************/


/***
 * Returns the length of hypotenuse of right angle triangle with sides x and y,
 * equivalent to `sqrt(x*x + y*y)`, but has better numerical stability and
 * internally handles intermediate overflows/underflows, but is also slower.
 *
 * @function math.hypot
 * @param x number
 * @param y number
 * @return number hypotenuse `sqrt(x*x+y*y)`
 */
int LuaMathExtra::hypot(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	lua_pushnumber(L, math::hypot(luaL_checknumber_noassert(L, 1), luaL_checknumber_noassert(L, 2)));
	return 1;
}


/***
 * Returns the length of the diagonal of an n-dimensional box (or the length of
 * an n-component vector). Rather quick method that does not handle intermediate
 * overflows/underflows nor is made for numerical stability.
 *
 * @function math.diag
 * @param x number
 * @param ... number
 * @return number diagonal
 */
int LuaMathExtra::diag(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	lua_Number res = 0.0f;

	for (int i = lua_gettop(L); i >= 1; i--) {
		res += Square(luaL_checknumber_noassert(L, i));
	}

	lua_pushnumber(L, math::sqrt(res));
	return 1;
}


/*** Returns x clamped to min and max boundaries.
 *
 * @function math.clamp
 * @param value number
 * @param min number
 * @param max number
 * @return number clamped
 */
int LuaMathExtra::clamp(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	const lua_Number lbound = luaL_checknumber_noassert(L, 2);
	const lua_Number ubound = luaL_checknumber_noassert(L, 3);

	if (lbound > ubound) {
		luaL_error(L, "Invalid math.%s parameters, lower bound(%f) is greater than upper bound(%f)", __func__, lbound, ubound);
		return 0;
	}

	lua_pushnumber(L, std::clamp(luaL_checknumber_noassert(L, 1), lbound, ubound));
	return 1;
}


/*** Returns 0 if x == 0, 1 if x > 0, -1 if x < 0
 *
 * @function math.sgn
 * @param x number
 * @return number sign
 */
int LuaMathExtra::sgn(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	const lua_Number x = luaL_checknumber_noassert(L, 1);

	if (x != 0.0f) {
		// engine's version returns -1 for sgn(0)
		lua_pushnumber(L, Sign(x));
	} else {
		lua_pushnumber(L, 0.0f);
	}

	return 1;
}


/*** Returns linear interpolation between x and y with ratio a (x+(y-x)*a).
 *
 * @function math.mix
 * @param x number
 * @param y number
 * @param a number
 * @return number mixed (x+(y-x)*a)
 */
int LuaMathExtra::mix(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	const lua_Number x = luaL_checknumber_noassert(L, 1);
	const lua_Number y = luaL_checknumber_noassert(L, 2);
	const lua_Number a = luaL_checknumber_noassert(L, 3);

	lua_pushnumber(L, ::mix(x, y, a));
	return 1;
}


/***
 * Returns x rounded to n decimals, if n is omitted or <=0, rounds to nearest
 * integer. Note that Spring's Lua interpreter uses 32-bit floats for all
 * numbers so max. precision is ~7 decimal digits.
 *
 * @function math.round
 * @param x number
 * @param decimals number
 * @return number rounded
 */
int LuaMathExtra::round(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	const lua_Number x = luaL_checknumber_noassert(L, 1);

	if (lua_gettop(L) > 1) {
		// round number to <n> decimals
		// Spring's Lua interpreter uses 32-bit floats,
		// therefore max. accuracy is ~7 decimal digits
		const int i = std::min(7, int(sizeof(POWERS_OF_TEN) / sizeof(float)) - 1);
		const int n = std::clamp(luaL_checkint(L, 2), 0, i);

		const lua_Number xinteg = math::floor(x);
		const lua_Number xfract = x - xinteg;

		lua_pushnumber(L, xinteg + math::floor((xfract * POWERS_OF_TEN[n]) + 0.5f) / POWERS_OF_TEN[n]);
	} else {
		lua_pushnumber(L, math::floor(x + 0.5f));
	}

	return 1;
}


/*** Returns erf(x), the Gauss error function, between -1 and 1.
 *
 * @function math.erf
 * @param x number
 * @return number erf
 */
int LuaMathExtra::erf(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	lua_pushnumber(L, math::erf(luaL_checknumber_noassert(L, 1)));
	return 1;
}


/*** Applies the smoothstep function
 *
 * @function math.smoothstep
 * Clamps and rescales v to a value between [0; 1] based on the edges and then applies the smoothstep function.
 * For example math.smoothstep(10, 25, 15) is 0.259, because 15 is 0.333 of the way from 10 to 25, and smoothstep(0.333) is 0.259
 * @param edge0 number
 * @param edge1 number
 * @param v number
 * @return number smoothstep
 */
int LuaMathExtra::smoothstep(lua_State* L) {
	RECOIL_DETAILED_TRACY_ZONE;
	lua_pushnumber(L, ::smoothstep(luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3)));
	return 1;
}


/*** Returns the normalize vector of an given vector.
 *
 * @function math.normalize
 * @param x number
 * @param ... number
 * @return number ... normalized
 */
int LuaMathExtra::normalize(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	lua_Number res = 0.0f;
	const int param = lua_gettop(L);
	for (int i = param; i >= 1; i--) {
		res += Square(luaL_checknumber_noassert(L, i));
	}
	if likely(res > float3::nrm_eps())
		res = math::sqrt(res);
	else
	{
		for (int i = 1; i <= param; ++i) {
			lua_pushnumber(L, 0);
		}
		return param;
	}
	for (int i = 1; i <= param; ++i) {
		float tmp = luaL_checknumber_noassert(L, i);
		tmp = tmp / res;
		lua_pushnumber(L, tmp);
	}
	return param;
}

/******************************************************************************/
/******************************************************************************/


// spring's lua uses FLOATS as its number type which can only represent
// integer values up to 1<<24 exactly
static constexpr int EXACT_INTEGER_MASK = 0x00FFFFFF; // 2^24


static inline unsigned int luaL_checkuint(lua_State* L, int index)
{
	return (unsigned int)luaL_checkint(L, index);
}

/*** Returns the bitwise OR of all arguments. Only use up to 24 bit integers.
 *
 * @function math.bit_or
 * @param ... integer
 * @return integer result
 */
int LuaMathExtra::bit_or(lua_State* L)
{
	unsigned int result = 0x00000000;
	for (int i = 1; !lua_isnone(L, i); i++) {
		result = result | luaL_checkuint(L, i);
	}
	lua_pushnumber(L, result & EXACT_INTEGER_MASK);
	return 1;
}


/*** Returns the bitwise AND of all arguments. Only use up to 24 bit integers.
 *
 * @function math.bit_and
 * @param ... integer
 * @return integer result
 */
int LuaMathExtra::bit_and(lua_State* L)
{
	unsigned int result = 0xFFFFFFFF;
	for (int i = 1; !lua_isnone(L, i); i++) {
		result = result & luaL_checkuint(L, i);
	}
	lua_pushnumber(L, result & EXACT_INTEGER_MASK);
	return 1;
}


/*** Returns the bitwise XOR of all arguments. Only use up to 24 bit integers.
 *
 * @function math.bit_xor
 * @param ... integer
 * @return integer result
 */
int LuaMathExtra::bit_xor(lua_State* L)
{
	unsigned int result = 0x00000000;
	for (int i = 1; !lua_isnone(L, i); i++) {
		result = result ^ luaL_checkuint(L, i);
	}
	lua_pushnumber(L, result & EXACT_INTEGER_MASK);
	return 1;
}


/*** Returns the bitwise NOT of the 24 bit integer argument.
 *
 * @function math.bit_inv
 * @param value integer
 * @return integer result
 */
int LuaMathExtra::bit_inv(lua_State* L)
{
	const unsigned int result = ~luaL_checkuint(L, 1);
	lua_pushnumber(L, result & EXACT_INTEGER_MASK);
	return 1;
}


/*** Set each of the bits of a 24 bit integer. Returns result = result OR (1 << a1) OR (1 << a2) OR ...;)
 *
 * @function math.bit_bits
 * @param ... integer
 * @return integer result
 */
int LuaMathExtra::bit_bits(lua_State* L)
{
	unsigned int result = 0x00000000;
	for (int i = 1; !lua_isnone(L, i); i++) {
		const int bit = (unsigned int)luaL_checkint(L, i);
		result = result | (1 << bit);
	}
	lua_pushnumber(L, result & EXACT_INTEGER_MASK);
	return 1;
}