/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This file has to be C99 compatible, as it is not only used by the engine,
 * but also by AIs.
 */

#ifndef _EXPORTDEFINES_H
#define _EXPORTDEFINES_H

#include "System/MainDefines.h" // for __arch32__ / __arch64__

#ifndef EXTERNALIZER
	#if defined(__cplusplus)
		#define EXTERNALIZER extern "C"
		#define EXTERNALIZER_B EXTERNALIZER {
		#define EXTERNALIZER_E }
	#else // __cplusplus
		// we dont have to export if we are in C already
		#define EXTERNALIZER
		#define EXTERNALIZER_B
		#define EXTERNALIZER_E
	#endif // __cplusplus
#endif // EXTERNALIZER

// extern declaration that will work across
// different platforms and compilers
#ifndef SHARED_EXPORT
	#if defined _WIN32 || defined __CYGWIN__
		#define EXPORT_CLAUSE __declspec(dllexport)
		#define SPRING_API
	#elif __GNUC__ >= 4
		// Support for '-fvisibility=hidden'.
		#define EXPORT_CLAUSE __attribute__ ((visibility("default")))
		#define SPRING_API EXPORT_CLAUSE
	#else // _WIN32 || defined __CYGWIN__
		// Older versions of gcc have everything visible; no need for fancy stuff.
		#define EXPORT_CLAUSE
		#define SPRING_API
	#endif // _WIN32 || defined __CYGWIN__
	#define SHARED_EXPORT EXTERNALIZER EXPORT_CLAUSE
#endif // SHARED_EXPORT

// calling convention declaration that will work across
// different and compilers
// for all 64bit platforms, we do not specify a calling convention,
// as they all support only a single one anyway
// for windows 32bit, we use stdcall
// for non-windows 32bit, we use cdecl

#ifndef SPRING_CALLING_CONVENTION
	#if defined _WIN32
		#define SPRING_CALLING_CONVENTION stdcall
		#define SPRING_CALLING_CONVENTION_2 __stdcall
	#elif defined __POWERPC__
		#define SPRING_CALLING_CONVENTION
		#define SPRING_CALLING_CONVENTION_2
	#else
		#define SPRING_CALLING_CONVENTION cdecl
		#define SPRING_CALLING_CONVENTION_2 __cdecl
	#endif // defined _WIN32
#endif // SPRING_CALLING_CONVENTION

#ifndef CALLING_CONV
	#if defined __arch64__
		#define CALLING_CONV
	#elif defined _WIN32
		#define CALLING_CONV SPRING_CALLING_CONVENTION_2
	#elif __GNUC__
		#define CALLING_CONV __attribute__ ((SPRING_CALLING_CONVENTION))
	#else // defined _WIN64 ...
		#define CALLING_CONV SPRING_CALLING_CONVENTION_2
	#endif // defined _WIN64 ...
#endif // CALLING_CONV

#define EXPORT(type) SHARED_EXPORT type CALLING_CONV


#endif // _EXPORTDEFINES_H
