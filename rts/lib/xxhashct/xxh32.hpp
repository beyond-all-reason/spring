/**
 * MIT License
 *
 * Copyright (c) 2021 Zachary Arnaise
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef XXHASHCT_XXH32_HPP
#define XXHASHCT_XXH32_HPP

#include <cstdint>

struct xxh32 {
	static constexpr uint32_t hash(const char* input, uint32_t len, uint32_t seed)
	{
		return finalize(
		    (len >= 16 ? h16bytes(input, len, seed) : seed + PRIME5) + len, (input) + (len & ~0xF), len & 0xF);
	}

private:
	static constexpr uint32_t PRIME1 = 0x9E3779B1U;
	static constexpr uint32_t PRIME2 = 0x85EBCA77U;
	static constexpr uint32_t PRIME3 = 0xC2B2AE3DU;
	static constexpr uint32_t PRIME4 = 0x27D4EB2FU;
	static constexpr uint32_t PRIME5 = 0x165667B1U;

	// 32-bit rotate left.
	static constexpr uint32_t rotl(uint32_t x, int r) { return ((x << r) | (x >> (32 - r))); }

	// Normal stripe processing routine.
	static constexpr uint32_t round(uint32_t acc, const uint32_t input)
	{
		return rotl(acc + (input * PRIME2), 13) * PRIME1;
	}

	static constexpr uint32_t avalanche_step(const uint32_t h, const int rshift, const uint32_t prime)
	{
		return (h ^ (h >> rshift)) * prime;
	}

	// Mixes all bits to finalize the hash.
	static constexpr uint32_t avalanche(const uint32_t h)
	{
		return avalanche_step(avalanche_step(avalanche_step(h, 15, PRIME2), 13, PRIME3), 16, 1);
	}

#ifdef XXH32_BIG_ENDIAN
	static constexpr uint32_t endian32(const char* v)
	{
		return uint32_t(uint8_t(v[3])) | (uint32_t(uint8_t(v[2])) << 8) | (uint32_t(uint8_t(v[1])) << 16) |
		       (uint32_t(uint8_t(v[0])) << 24);
	}
#else
	static constexpr uint32_t endian32(const char* v)
	{
		return uint32_t(uint8_t(v[0])) | (uint32_t(uint8_t(v[1])) << 8) | (uint32_t(uint8_t(v[2])) << 16) |
		       (uint32_t(uint8_t(v[3])) << 24);
	}
#endif // XXH32_BIG_ENDIAN

	static constexpr uint32_t fetch32(const char* p, const uint32_t v) { return round(v, endian32(p)); }

	// Processes the last 0-15 bytes of p.
	static constexpr uint32_t finalize(const uint32_t h, const char* p, uint32_t len)
	{
		return (len >= 4) ? finalize(rotl(h + (endian32(p) * PRIME3), 17) * PRIME4, p + 4, len - 4) :
		       (len > 0)  ? finalize(rotl(h + (uint8_t(*p) * PRIME5), 11) * PRIME1, p + 1, len - 1) :
		                    avalanche(h);
	}

	static constexpr uint32_t
	h16bytes(const char* p, uint32_t len, const uint32_t v1, const uint32_t v2, const uint32_t v3, const uint32_t v4)
	{
		return (len >= 16) ? h16bytes(p + 16, len - 16, fetch32(p, v1), fetch32(p + 4, v2), fetch32(p + 8, v3),
		                         fetch32(p + 12, v4)) :
		                     rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18);
	}

	static constexpr uint32_t h16bytes(const char* p, uint32_t len, const uint32_t seed)
	{
		return h16bytes(p, len, seed + PRIME1 + PRIME2, seed + PRIME2, seed, seed - PRIME1);
	}
};

#endif /* !defined XXHASHCT_XXH32_HPP */
