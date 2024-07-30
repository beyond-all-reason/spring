/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <streflop/streflop_cond.h> //! must happen before OffscreenGLContext.h, which includes agl.h
#include "System/GameLoadThread.h"

#include <functional>

#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringThreading.h"


CGameLoadThread::CGameLoadThread(std::function<void()> f)
{
	thread = spring::thread(std::bind(&CGameLoadThread::WrapFunc, this, f));
}


void CGameLoadThread::join()
{
	if (!thread.joinable())
		return;

	thread.join();
}


__FORCE_ALIGN_STACK__
void CGameLoadThread::WrapFunc(std::function<void()> f)
{
	Threading::SetThreadName("gameload");

	// init streflop
	// not needed to maintain sync (precision flags are
	// per-process) but fpu exceptions are per-thread
	streflop::streflop_init<streflop::Simple>();

	try {
		f();
	} CATCH_SPRING_ERRORS
}

