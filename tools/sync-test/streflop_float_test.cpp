// STREFLOP Float Comparison Test
// Generates deterministic floating-point results for cross-architecture comparison.
// See STREFLOP_FLOAT_TEST_PROMPT.md for specification.
//
// Usage: streflop-float-test [-n NUM_INPUTS] [--hash-only] [--no-text] [OUTPUT_PREFIX]
//   -n NUM_INPUTS   Number of random inputs to generate (default: 10000)
//   --hash-only     Compute and print hash only, no file output (for billion-scale)
//   --no-text       Skip text output (auto-set when N > 10000)
//
// Scales from 10K to 1B+ inputs. Streams results directly to disk.

#include "streflop.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

// --- Configuration macros (set by CMake) ---
#ifndef SFTEST_MODE
#define SFTEST_MODE "UNKNOWN"
#endif
#ifndef SFTEST_COMPILER
#define SFTEST_COMPILER "unknown"
#endif

// --- Architecture detection ---
#if defined(__aarch64__) || defined(__arm64__)
static const char* ARCH_STR = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
static const char* ARCH_STR = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
static const char* ARCH_STR = "x86";
#else
static const char* ARCH_STR = "unknown";
#endif

// --- Deterministic PRNG (xorshift32, independent of streflop) ---
struct XorShift32 {
	uint32_t s;
	explicit XorShift32(uint32_t seed) : s(seed ? seed : 1) {}
	uint32_t next() {
		s ^= s << 13;
		s ^= s >> 17;
		s ^= s << 5;
		return s;
	}
	// Uniform float in [0, 1)
	float uniform01() {
		return (next() >> 8) * (1.0f / 16777216.0f);
	}
	// Uniform float in [lo, hi)
	float range(float lo, float hi) {
		return lo + uniform01() * (hi - lo);
	}
};

// --- Bit extraction ---
static uint32_t f32bits(float f) {
	uint32_t b;
	memcpy(&b, &f, 4);
	return b;
}

static uint64_t f64bits(double d) {
	uint64_t b;
	memcpy(&b, &d, 8);
	return b;
}

// --- FNV-1a 64-bit streaming hash ---
static const uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
static const uint64_t FNV_PRIME        = 0x100000001b3ULL;

struct FNV1a {
	uint64_t hash = FNV_OFFSET_BASIS;

	void feed(const void* data, size_t len) {
		auto* p = static_cast<const uint8_t*>(data);
		for (size_t i = 0; i < len; i++) {
			hash ^= p[i];
			hash *= FNV_PRIME;
		}
	}

	void feed_u8(uint8_t v)   { feed(&v, 1); }
	void feed_u32(uint32_t v) { feed(&v, 4); }
	void feed_u64(uint64_t v) { feed(&v, 8); }
};

// --- Streaming output state ---
struct StreamState {
	FILE* bin_fp = nullptr;
	FILE* txt_fp = nullptr;
	FNV1a hasher;
	uint32_t count = 0;
	long count_file_offset = 0;  // position of count field in binary for seek-back
	uint64_t progress_interval = 1000000; // report every N tests

	bool init_binary(const char* path, const char* mode, const char* arch) {
		bin_fp = fopen(path, "wb");
		if (!bin_fp) {
			fprintf(stderr, "ERROR: Cannot open %s for writing\n", path);
			return false;
		}
		// Write header: magic + version + mode_str + arch_str + count_placeholder
		fwrite("SFLT", 1, 4, bin_fp);
		uint32_t version = 1;
		fwrite(&version, 4, 1, bin_fp);
		fwrite(mode, 1, strlen(mode) + 1, bin_fp);
		fwrite(arch, 1, strlen(arch) + 1, bin_fp);
		// Remember position for count, write placeholder 0
		count_file_offset = ftell(bin_fp);
		uint32_t zero = 0;
		fwrite(&zero, 4, 1, bin_fp);
		return true;
	}

	bool init_text(const char* path, const char* mode, const char* arch, const char* compiler) {
		txt_fp = fopen(path, "w");
		if (!txt_fp) {
			fprintf(stderr, "ERROR: Cannot open %s for writing\n", path);
			return false;
		}
		time_t now = time(nullptr);
		struct tm* t = localtime(&now);
		char datebuf[32];
		strftime(datebuf, sizeof(datebuf), "%Y-%m-%d", t);

		fprintf(txt_fp, "# STREFLOP Float Test Results\n");
		fprintf(txt_fp, "# Mode:     %s\n", mode);
		fprintf(txt_fp, "# Arch:     %s\n", arch);
		fprintf(txt_fp, "# Compiler: %s\n", compiler);
		fprintf(txt_fp, "# Date:     %s\n", datebuf);
		fprintf(txt_fp, "# FP-contract: off\n");
		fprintf(txt_fp, "#\n");
		fprintf(txt_fp, "# %-8s %-6s %-6s %-14s %-18s %-18s %s\n",
		        "TestID", "Prec", "Cat", "Operation", "InputA(hex)", "Result(hex)", "Result(value)");
		return true;
	}

	void emit_record(char prec, uint64_t result_bits,
	                 const char* cat, const char* op,
	                 uint64_t input_a, double result_val) {
		uint32_t id = ++count;

		// Hash: id + prec + result_bits (same data as binary record)
		hasher.feed_u32(id);
		hasher.feed_u8(static_cast<uint8_t>(prec));
		hasher.feed_u64(result_bits);

		// Binary output
		if (bin_fp) {
			fwrite(&id, 4, 1, bin_fp);
			fwrite(&prec, 1, 1, bin_fp);
			fwrite(&result_bits, 8, 1, bin_fp);
		}

		// Text output
		if (txt_fp) {
			if (prec == 'F') {
				fprintf(txt_fp, "  %-8u %-6c %-6s %-14s %08X         %08X           %.9g\n",
				        id, prec, cat, op,
				        (uint32_t)input_a,
				        (uint32_t)result_bits,
				        result_val);
			} else {
				fprintf(txt_fp, "  %-8u %-6c %-6s %-14s %016llX %016llX %.17g\n",
				        id, prec, cat, op,
				        (unsigned long long)input_a,
				        (unsigned long long)result_bits,
				        result_val);
			}
		}

		// Progress
		if (count % progress_interval == 0) {
			printf("  ... %u tests completed\n", count);
			fflush(stdout);
		}
	}

	void finalize() {
		if (bin_fp) {
			// Seek back and write actual count
			fseek(bin_fp, count_file_offset, SEEK_SET);
			fwrite(&count, 4, 1, bin_fp);
			fclose(bin_fp);
			bin_fp = nullptr;
		}
		if (txt_fp) {
			fclose(txt_fp);
			txt_fp = nullptr;
		}
	}
};

static StreamState g_stream;

// --- Recording helpers ---
static void rec_f1(const char* cat, const char* op, float in, float res) {
	g_stream.emit_record('F', f32bits(res), cat, op, f32bits(in), res);
}

static void rec_f2(const char* cat, const char* op, float a, float b, float res) {
	g_stream.emit_record('F', f32bits(res), cat, op, f32bits(a), res);
	(void)b; // b is implicit in the deterministic test sequence
}

static void rec_d1(const char* cat, const char* op, double in, double res) {
	g_stream.emit_record('D', f64bits(res), cat, op, f64bits(in), res);
}

static void rec_d2(const char* cat, const char* op, double a, double b, double res) {
	g_stream.emit_record('D', f64bits(res), cat, op, f64bits(a), res);
	(void)b;
}

// --- Input generation ---
// Generates N floats spanning normal, edge, and stress-test ranges.
static std::vector<float> generate_inputs(int N) {
	std::vector<float> v;
	v.reserve(N);
	XorShift32 rng(0xDEADBEEF);

	// First: special values (16 values)
	v.push_back(0.0f);
	v.push_back(-0.0f);
	v.push_back(1.0f);
	v.push_back(-1.0f);
	v.push_back(0.5f);
	v.push_back(-0.5f);
	v.push_back(2.0f);
	v.push_back(-2.0f);
	float inf_val;
	uint32_t inf_bits = 0x7F800000;
	memcpy(&inf_val, &inf_bits, 4);
	v.push_back(inf_val);           // +INF
	uint32_t ninf_bits = 0xFF800000;
	float ninf_val;
	memcpy(&ninf_val, &ninf_bits, 4);
	v.push_back(ninf_val);          // -INF
	uint32_t nan_bits = 0x7FC00000;
	float nan_val;
	memcpy(&nan_val, &nan_bits, 4);
	v.push_back(nan_val);           // NaN
	v.push_back(1e-40f);            // denormal
	v.push_back(1e-38f);            // near-zero normal
	v.push_back(1e+38f);            // near-max
	v.push_back(-1e-40f);           // negative denormal
	v.push_back(-1e+38f);           // negative near-max

	// Stress-test values for transcendentals (64 values)
	float pi_f = 3.14159265358979323846f;
	for (int i = 0; i < 16; i++) {
		float t = (float)i / 15.0f;
		v.push_back(t * pi_f);            // 0 to pi
		v.push_back(t * pi_f * 0.5f);     // 0 to pi/2
		v.push_back(t * 2.0f - 1.0f);     // -1 to 1 (for asin/acos domain)
		v.push_back(t * 0.999f);           // near 0 to near 1
	}

	// Random normal-range values (fill remaining)
	while ((int)v.size() < N) {
		float kind = rng.uniform01();
		if (kind < 0.25f) {
			v.push_back(rng.range(-1.0f, 1.0f));        // small
		} else if (kind < 0.50f) {
			v.push_back(rng.range(-1000.0f, 1000.0f));   // medium
		} else if (kind < 0.75f) {
			v.push_back(rng.range(-1e6f, 1e6f));         // large
		} else {
			v.push_back(rng.range(1e-10f, 1e-4f));       // tiny positive
		}
	}

	return v;
}

// --- Category A: Basic arithmetic (uses hardware FPU via Simple type) ---
static void test_arithmetic(const std::vector<float>& inputs, int limit) {
	int N = (int)inputs.size();
	// Use consecutive pairs; skip special values at front to avoid INF/NaN in arithmetic
	int start = 16; // skip specials
	for (int i = start; i + 1 < N && i < start + limit; i += 2) {
		streflop::Simple a(inputs[i]);
		streflop::Simple b(inputs[i + 1]);

		// Avoid division by zero
		streflop::Simple b_safe = (b == streflop::Simple(0.0f)) ? streflop::Simple(1.0f) : b;

		rec_f2("arith", "add", inputs[i], inputs[i + 1], (float)(a + b));
		rec_f2("arith", "sub", inputs[i], inputs[i + 1], (float)(a - b));
		rec_f2("arith", "mul", inputs[i], inputs[i + 1], (float)(a * b));
		rec_f2("arith", "div", inputs[i], (float)b_safe, (float)(a / b_safe));

		// FMA-like: a * b + next value (tests if compiler emits FMA)
		if (i + 2 < N) {
			streflop::Simple c(inputs[i + 2]);
			rec_f2("arith", "muladd", inputs[i], inputs[i + 1], (float)(a * b + c));
		}
	}
}

// --- Category B: Streflop transcendentals (bundled libm) ---
static void test_transcendentals(const std::vector<float>& inputs, int limit) {
	int N = (int)inputs.size();

	for (int i = 0; i < N && i < limit; i++) {
		float fi = inputs[i];
		streflop::Simple x(fi);

		// Skip non-finite inputs for functions that don't handle them well
		float fx = (float)x;
		uint32_t xbits = f32bits(fx);
		bool is_finite = ((xbits & 0x7F800000) != 0x7F800000);
		if (!is_finite) continue;

		// Functions valid for all finite x
		rec_f1("trans", "sqrt_abs", fi, (float)streflop::sqrt(streflop::fabs(x)));
		rec_f1("trans", "fabs",     fi, (float)streflop::fabs(x));
		rec_f1("trans", "floor",    fi, (float)streflop::floor(x));
		rec_f1("trans", "ceil",     fi, (float)streflop::ceil(x));
		rec_f1("trans", "round",    fi, (float)streflop::round(x));
		rec_f1("trans", "trunc",    fi, (float)streflop::trunc(x));
		rec_f1("trans", "sin",      fi, (float)streflop::sin(x));
		rec_f1("trans", "cos",      fi, (float)streflop::cos(x));
		rec_f1("trans", "tan",      fi, (float)streflop::tan(x));
		rec_f1("trans", "sinh",     fi, (float)streflop::sinh(x));
		rec_f1("trans", "cosh",     fi, (float)streflop::cosh(x));
		rec_f1("trans", "tanh",     fi, (float)streflop::tanh(x));

		// exp/log — need positive or specific domain
		if (fx > -80.0f && fx < 80.0f) {
			rec_f1("trans", "exp", fi, (float)streflop::exp(x));
		}
		if (fx > 0.0f) {
			rec_f1("trans", "log",   fi, (float)streflop::log(x));
			rec_f1("trans", "log2",  fi, (float)streflop::log2(x));
			rec_f1("trans", "log10", fi, (float)streflop::log10(x));
			rec_f1("trans", "sqrt",  fi, (float)streflop::sqrt(x));
			rec_f1("trans", "cbrt",  fi, (float)streflop::cbrt(x));
		}

		// asin/acos — domain [-1, 1]
		if (fx >= -1.0f && fx <= 1.0f) {
			rec_f1("trans", "asin", fi, (float)streflop::asin(x));
			rec_f1("trans", "acos", fi, (float)streflop::acos(x));
		}

		// atan — full domain
		rec_f1("trans", "atan", fi, (float)streflop::atan(x));

		// atan2, pow, fmod — use next value as second argument
		if (i + 1 < N) {
			float fi2 = inputs[i + 1];
			streflop::Simple y(fi2);
			float fy = (float)y;
			uint32_t ybits = f32bits(fy);
			bool y_finite = ((ybits & 0x7F800000) != 0x7F800000);
			if (y_finite) {
				rec_f2("trans", "atan2", fi, fi2, (float)streflop::atan2(x, y));
				if (fx > 0.0f && fy > -20.0f && fy < 20.0f) {
					rec_f2("trans", "pow", fi, fi2, (float)streflop::pow(x, y));
				}
				if (fy != 0.0f) {
					rec_f2("trans", "fmod", fi, fi2, (float)streflop::fmod(x, y));
				}
			}
		}
	}
}

// --- Category C: Double precision (arithmetic only) ---
// NOTE: streflop's bundled libm only has flt-32 (float) implementations.
// Double-precision transcendentals (sin, cos, etc.) are declared in SMath.h
// but have no implementation. Only double arithmetic is testable.
static void test_double_precision(const std::vector<float>& inputs, int limit) {
	int N = (int)inputs.size();
	int start = 16;

	for (int i = start; i + 1 < N && i < start + limit; i += 2) {
		double da = (double)inputs[i];
		double db = (double)inputs[i + 1];
		streflop::Double a(da);
		streflop::Double b(db);

		streflop::Double b_safe = (b == streflop::Double(0.0)) ? streflop::Double(1.0) : b;

		rec_d2("double", "add", da, db, (double)(a + b));
		rec_d2("double", "sub", da, db, (double)(a - b));
		rec_d2("double", "mul", da, db, (double)(a * b));
		rec_d2("double", "div", da, (double)b_safe, (double)(a / b_safe));

		// FMA-like: a * b + c
		if (i + 2 < N) {
			streflop::Double c((double)inputs[i + 2]);
			rec_d2("double", "muladd", da, db, (double)(a * b + c));
		}
	}
}

// --- Construct float from raw IEEE 754 bits ---
static float make_float(uint32_t bits) {
	float f;
	memcpy(&f, &bits, 4);
	return f;
}

// --- Category E: Edge cases (denormals, ±inf, ±NaN, ±0.0f) ---
// Exhaustively tests all combinations of special IEEE 754 values through
// arithmetic and unary operations. These are the values most likely to
// differ across architectures if FPU handling diverges.
static void test_edge_cases() {
	// Comprehensive set of IEEE 754 edge values
	float edges[] = {
		// Zeros
		0.0f,
		-0.0f,
		// Infinities
		make_float(0x7F800000),   // +inf
		make_float(0xFF800000),   // -inf
		// NaN variants
		make_float(0x7FC00000),   // quiet NaN (QNaN)
		make_float(0x7F800001),   // signaling NaN (SNaN)
		make_float(0xFFC00000),   // negative quiet NaN
		make_float(0x7FC0DEAD),   // QNaN with payload
		// Denormals (subnormals)
		make_float(0x00000001),   // smallest positive denormal (FLT_TRUE_MIN ~1.4e-45)
		make_float(0x80000001),   // smallest negative denormal
		make_float(0x007FFFFF),   // largest positive denormal
		make_float(0x807FFFFF),   // largest negative denormal
		make_float(0x00400000),   // mid-range positive denormal
		make_float(0x00000100),   // small positive denormal
		make_float(0x00000002),   // second smallest positive denormal
		make_float(0x80000002),   // second smallest negative denormal
		// Boundary normals
		make_float(0x00800000),   // FLT_MIN (smallest positive normal ~1.175e-38)
		make_float(0x80800000),   // -FLT_MIN
		make_float(0x7F7FFFFF),   // FLT_MAX (~3.4e+38)
		make_float(0xFF7FFFFF),   // -FLT_MAX
		make_float(0x00800001),   // just above FLT_MIN
		make_float(0x7F7FFFFE),   // just below FLT_MAX
		// Common values
		1.0f, -1.0f,
		0.5f, -0.5f,
		2.0f, -2.0f,
	};
	int N = (int)(sizeof(edges) / sizeof(edges[0]));

	// --- All-pairs binary arithmetic (N x N x 5 ops) ---
	for (int i = 0; i < N; i++) {
		streflop::Simple a(edges[i]);
		for (int j = 0; j < N; j++) {
			streflop::Simple b(edges[j]);

			rec_f2("edge", "add", edges[i], edges[j], (float)(a + b));
			rec_f2("edge", "sub", edges[i], edges[j], (float)(a - b));
			rec_f2("edge", "mul", edges[i], edges[j], (float)(a * b));
			rec_f2("edge", "div", edges[i], edges[j], (float)(a / b));

			// muladd: a * b + a (uses self as third operand for determinism)
			rec_f2("edge", "muladd", edges[i], edges[j], (float)(a * b + a));
		}
	}

	// --- Unary operations on all edge values ---
	for (int i = 0; i < N; i++) {
		streflop::Simple x(edges[i]);

		rec_f1("edge", "fabs",  edges[i], (float)streflop::fabs(x));
		rec_f1("edge", "floor", edges[i], (float)streflop::floor(x));
		rec_f1("edge", "ceil",  edges[i], (float)streflop::ceil(x));
		rec_f1("edge", "round", edges[i], (float)streflop::round(x));
		rec_f1("edge", "trunc", edges[i], (float)streflop::trunc(x));

		// sqrt(|x|) — domain-safe
		rec_f1("edge", "sqrt_abs", edges[i], (float)streflop::sqrt(streflop::fabs(x)));

		// Trig on all values (including inf/NaN — should produce NaN deterministically)
		rec_f1("edge", "sin", edges[i], (float)streflop::sin(x));
		rec_f1("edge", "cos", edges[i], (float)streflop::cos(x));
		rec_f1("edge", "atan", edges[i], (float)streflop::atan(x));

		// exp/log on all values (may produce inf/NaN — that's the point)
		rec_f1("edge", "exp", edges[i], (float)streflop::exp(x));
		rec_f1("edge", "log_abs", edges[i], (float)streflop::log(streflop::fabs(x) + streflop::Simple(1e-45f)));
	}

	// NOTE: No double-precision edge cases here. streflop only bundles
	// flt-32 libm; double-precision delegates to system libm (glibc on
	// Linux, Apple libm on macOS) which differs at extreme values.
	// Double-precision is NOT sync-critical — engine uses Simple (float32).
}

// --- Category D: Compound operations (simulate engine patterns) ---
static void test_compound(const std::vector<float>& inputs, int limit) {
	int N = (int)inputs.size();
	int start = 80; // past specials and trig stress values

	// Normalize float3: x/sqrt(x*x + y*y + z*z)
	for (int i = start; i + 2 < N && i < start + limit; i += 3) {
		streflop::Simple x(inputs[i]);
		streflop::Simple y(inputs[i + 1]);
		streflop::Simple z(inputs[i + 2]);
		streflop::Simple len2 = x * x + y * y + z * z;
		if ((float)len2 > 0.0f) {
			streflop::Simple len = streflop::sqrt(len2);
			streflop::Simple nx = x / len;
			rec_f1("compnd", "norm_x", inputs[i], (float)nx);
		}
	}

	// Dot product: a.x*b.x + a.y*b.y + a.z*b.z
	for (int i = start; i + 5 < N && i < start + limit; i += 6) {
		streflop::Simple ax(inputs[i]),   ay(inputs[i + 1]), az(inputs[i + 2]);
		streflop::Simple bx(inputs[i + 3]), by(inputs[i + 4]), bz(inputs[i + 5]);
		streflop::Simple dot = ax * bx + ay * by + az * bz;
		rec_f2("compnd", "dot3", inputs[i], inputs[i + 3], (float)dot);
	}

	// Linear interpolation: a + t*(b-a), t in [0,1]
	int lerp_limit = (limit <= 300) ? (limit * 2 / 3) : limit; // backward compat: 300 -> 200
	for (int i = start; i + 1 < N && i < start + lerp_limit; i += 2) {
		streflop::Simple a(inputs[i]);
		streflop::Simple b(inputs[i + 1]);
		streflop::Simple t(0.3f);
		streflop::Simple lerp_val = a + t * (b - a);
		rec_f2("compnd", "lerp", inputs[i], inputs[i + 1], (float)lerp_val);
	}

	// Distance: sqrt((x1-x2)^2 + (y1-y2)^2 + (z1-z2)^2)
	for (int i = start; i + 5 < N && i < start + limit; i += 6) {
		streflop::Simple x1(inputs[i]),   y1(inputs[i + 1]), z1(inputs[i + 2]);
		streflop::Simple x2(inputs[i + 3]), y2(inputs[i + 4]), z2(inputs[i + 5]);
		streflop::Simple dx = x1 - x2, dy = y1 - y2, dz = z1 - z2;
		streflop::Simple dist = streflop::sqrt(dx * dx + dy * dy + dz * dz);
		rec_f2("compnd", "dist3", inputs[i], inputs[i + 3], (float)dist);
	}

	// Accumulated sum of 1000 small values (tests rounding accumulation)
	{
		streflop::Simple acc(0.0f);
		XorShift32 rng(12345);
		for (int i = 0; i < 1000; i++) {
			float small = rng.range(1e-6f, 1e-3f);
			acc += streflop::Simple(small);
		}
		rec_f1("compnd", "accum1000", 0.0f, (float)acc);
	}

	// Accumulated sum with alternating signs (catastrophic cancellation test)
	{
		streflop::Simple acc(0.0f);
		XorShift32 rng(54321);
		for (int i = 0; i < 1000; i++) {
			float small = rng.range(1e-4f, 1e-2f);
			if (i & 1) small = -small;
			acc += streflop::Simple(small);
		}
		rec_f1("compnd", "cancel1000", 0.0f, (float)acc);
	}
}

// --- Usage ---
static void print_usage(const char* argv0) {
	printf("Usage: %s [-n NUM_INPUTS] [--hash-only] [--no-text] [OUTPUT_PREFIX]\n", argv0);
	printf("\n");
	printf("Options:\n");
	printf("  -n NUM_INPUTS   Number of random inputs (default: 10000)\n");
	printf("  --hash-only     Compute hash only, no file output\n");
	printf("  --no-text       Skip text file output\n");
	printf("\n");
	printf("Scale guide:\n");
	printf("  -n 10000      ~47K tests,  ~600 KB binary  (default, backward compat)\n");
	printf("  -n 1000000    ~5M tests,   ~65 MB binary   (~30 seconds)\n");
	printf("  -n 10000000   ~50M tests,  ~650 MB binary  (~5 minutes)\n");
	printf("  -n 100000000  ~500M tests, ~6.5 GB binary  (~1 hour)\n");
	printf("  --hash-only   any scale, no disk I/O       (for billion+)\n");
}

// --- Main ---
int main(int argc, char* argv[]) {
	// Parse command line
	int num_inputs = 10000;
	bool hash_only = false;
	bool write_text = true;
	const char* custom_prefix = nullptr;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
			num_inputs = atoi(argv[++i]);
			if (num_inputs < 100) {
				fprintf(stderr, "ERROR: -n must be >= 100\n");
				return 1;
			}
		} else if (strcmp(argv[i], "--hash-only") == 0) {
			hash_only = true;
		} else if (strcmp(argv[i], "--no-text") == 0) {
			write_text = false;
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		} else if (argv[i][0] != '-') {
			custom_prefix = argv[i];
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			print_usage(argv[0]);
			return 1;
		}
	}

	// Auto-disable text for large runs
	if (num_inputs > 10000 && write_text && !hash_only) {
		write_text = false;
		printf("Note: text output auto-disabled for N > 10000 (use --no-text to suppress this message)\n");
	}

	// Determine test limits per category
	// For N <= 10000: use original hardcoded limits (backward compat)
	// For N > 10000: process all inputs (no cap)
	const int arith_limit    = (num_inputs <= 10000) ? 2000 : num_inputs;
	const int trans_limit    = (num_inputs <= 10000) ? 2000 : num_inputs;
	const int double_limit   = (num_inputs <= 10000) ? 1000 : num_inputs;
	const int compound_limit = (num_inputs <= 10000) ? 300  : num_inputs;

	printf("STREFLOP Float Comparison Test\n");
	printf("  Mode:       %s\n", SFTEST_MODE);
	printf("  Arch:       %s\n", ARCH_STR);
	printf("  Compiler:   %s\n", SFTEST_COMPILER);
	printf("  FP-contract: off (set by CMake)\n");
	printf("  Inputs:     %d\n", num_inputs);
	printf("  Hash-only:  %s\n", hash_only ? "yes" : "no");
	printf("\n");

	// Estimate output size
	if (!hash_only) {
		// Rough estimate: ~5 tests per input on average
		double est_tests = (double)num_inputs * 5.0;
		double est_bytes = est_tests * 13.0;
		const char* unit = "bytes";
		if (est_bytes > 1e9) { est_bytes /= 1e9; unit = "GB"; }
		else if (est_bytes > 1e6) { est_bytes /= 1e6; unit = "MB"; }
		else if (est_bytes > 1e3) { est_bytes /= 1e3; unit = "KB"; }
		printf("  Estimated binary output: ~%.1f %s\n\n", est_bytes, unit);
	}

	// Set progress interval based on scale
	if (num_inputs >= 10000000)
		g_stream.progress_interval = 10000000;
	else if (num_inputs >= 1000000)
		g_stream.progress_interval = 1000000;
	else if (num_inputs >= 100000)
		g_stream.progress_interval = 100000;
	else
		g_stream.progress_interval = 0xFFFFFFFF; // effectively disabled for small runs

	// Initialize streflop FPU state
	printf("Initializing streflop (%s)...\n", SFTEST_MODE);
	streflop::streflop_init<streflop::Simple>();
	printf("  Simple init OK\n");
	streflop::streflop_init<streflop::Double>();
	printf("  Double init OK\n");

	// Determine output prefix
	char prefix[256];
	if (custom_prefix) {
		snprintf(prefix, sizeof(prefix), "%s", custom_prefix);
	} else {
		const char* mode_short = SFTEST_MODE;
		if (strncmp(mode_short, "STREFLOP_", 9) == 0)
			mode_short += 9;
		snprintf(prefix, sizeof(prefix), "streflop_results_%s_%s", mode_short, ARCH_STR);
	}

	// Open output files
	if (!hash_only) {
		char bin_path[512];
		snprintf(bin_path, sizeof(bin_path), "%s.bin", prefix);
		printf("Opening binary output: %s\n", bin_path);
		if (!g_stream.init_binary(bin_path, SFTEST_MODE, ARCH_STR))
			return 1;

		if (write_text) {
			char txt_path[512];
			snprintf(txt_path, sizeof(txt_path), "%s.txt", prefix);
			printf("Opening text output:   %s\n", txt_path);
			if (!g_stream.init_text(txt_path, SFTEST_MODE, ARCH_STR, SFTEST_COMPILER))
				return 1;
		}
	}
	printf("\n");

	// Generate inputs
	printf("Generating %d deterministic input values", num_inputs);
	if (num_inputs > 1000000) {
		printf(" (%.0f MB)...\n", (double)num_inputs * 4.0 / 1e6);
	} else {
		printf("...\n");
	}
	fflush(stdout);

	clock_t t_start = clock();
	auto inputs = generate_inputs(num_inputs);
	clock_t t_gen = clock();
	printf("  Generated in %.2f seconds\n\n",
	       (double)(t_gen - t_start) / CLOCKS_PER_SEC);

	// Run test categories (with cumulative hash checkpoints for mismatch diagnosis)
	printf("Running Category A: Basic arithmetic (limit=%d)...\n", arith_limit);
	uint32_t before = g_stream.count;
	test_arithmetic(inputs, arith_limit);
	printf("  %u tests  [hash: %016llX]\n", g_stream.count - before, (unsigned long long)g_stream.hasher.hash);

	printf("Running Category B: Transcendentals (limit=%d)...\n", trans_limit);
	before = g_stream.count;
	test_transcendentals(inputs, trans_limit);
	printf("  %u tests  [hash: %016llX]\n", g_stream.count - before, (unsigned long long)g_stream.hasher.hash);

	printf("Running Category C: Double precision (limit=%d)...\n", double_limit);
	before = g_stream.count;
	test_double_precision(inputs, double_limit);
	printf("  %u tests  [hash: %016llX]\n", g_stream.count - before, (unsigned long long)g_stream.hasher.hash);

	printf("Running Category D: Compound operations (limit=%d)...\n", compound_limit);
	before = g_stream.count;
	test_compound(inputs, compound_limit);
	printf("  %u tests  [hash: %016llX]\n", g_stream.count - before, (unsigned long long)g_stream.hasher.hash);

	printf("Running Category E: Edge cases (denormals, inf, NaN, zero)...\n");
	before = g_stream.count;
	test_edge_cases();
	printf("  %u tests  [hash: %016llX]\n", g_stream.count - before, (unsigned long long)g_stream.hasher.hash);

	clock_t t_end = clock();
	double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;

	printf("\n");
	printf("Total: %u tests in %.2f seconds (%.0f tests/sec)\n",
	       g_stream.count, elapsed,
	       elapsed > 0 ? g_stream.count / elapsed : 0);

	// Finalize output
	g_stream.finalize();

	if (!hash_only) {
		char bin_path[512];
		snprintf(bin_path, sizeof(bin_path), "%s.bin", prefix);
		// Report file size
		FILE* sz = fopen(bin_path, "rb");
		if (sz) {
			fseek(sz, 0, SEEK_END);
			long fsize = ftell(sz);
			fclose(sz);
			if (fsize > 1000000000)
				printf("Binary file: %s (%.2f GB)\n", bin_path, fsize / 1e9);
			else if (fsize > 1000000)
				printf("Binary file: %s (%.1f MB)\n", bin_path, fsize / 1e6);
			else
				printf("Binary file: %s (%.1f KB)\n", bin_path, fsize / 1e3);
		}
	}

	// Print hash — this is the key output for large-scale verification
	printf("\n");
	printf("============================================================\n");
	printf("VERIFICATION HASH (FNV-1a of all %u result records)\n", g_stream.count);
	printf("  Hash: %016llX\n", (unsigned long long)g_stream.hasher.hash);
	printf("  Tests: %u\n", g_stream.count);
	printf("  Mode: %s on %s\n", SFTEST_MODE, ARCH_STR);
	printf("============================================================\n");
	printf("\n");

	if (!hash_only) {
		char bin_path[512];
		snprintf(bin_path, sizeof(bin_path), "%s.bin", prefix);
		printf("Compare with:\n");
		printf("  python3 compare_results.py <reference>.bin %s\n", bin_path);
		printf("Or just compare the hash above across architectures.\n");
	} else {
		printf("Hash-only mode: share the hash string above for cross-arch comparison.\n");
		printf("If hashes match -> BIT-EXACT. If not, re-run with file output to find divergence.\n");
	}

	return 0;
}
