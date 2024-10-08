/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OFFSCREENGLCONTEXT_H
#define _OFFSCREENGLCONTEXT_H

#include "Rendering/GL/myGL.h"

#include <functional>
#include "System/Threading/SpringThreading.h"

/**
 * @brief CGameLoadThread
 * Runs a std::bind in an additional thread with an offscreen OpenGL context.
 * (Don't try render to the 'screen' a.k.a. default framebuffer in that thread, the results will be undetermistic)
 */
class CGameLoadThread
{
public:
	CGameLoadThread() = default;
	CGameLoadThread(std::function<void()> f);
	~CGameLoadThread() { join(); }
	CGameLoadThread(const CGameLoadThread& t) = delete;
	CGameLoadThread(CGameLoadThread&& t) { *this = std::move(t); }

	CGameLoadThread& operator = (const CGameLoadThread& t) = delete;
	CGameLoadThread& operator = (CGameLoadThread&& t) {
		thread = std::move(t.thread);
		return *this;
	};

	void join();
	bool joinable() const { return (thread.joinable()); }

private:
	void WrapFunc(std::function<void()> f);

	spring::thread thread;
};


#endif // _OFFSCREENGLCONTEXT_H
