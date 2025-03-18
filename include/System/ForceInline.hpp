#pragma once

// https://meghprkh.github.io/blog/posts/c++-force-inline/

#if defined(__clang__)
#define RECOIL_FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline

#elif defined(__GNUC__)
#define RECOIL_FORCE_INLINE [[gnu::always_inline]] inline

#elif defined(_MSC_VER)
#pragma warning(error: 4714)
#define RECOIL_FORCE_INLINE __forceinline

#else
#error Unsupported compiler
#endif