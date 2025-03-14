/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef THREADSIGNALHANDLER_H
#define THREADSIGNALHANDLER_H

#include <ucontext.h>
#include <functional>
#include <memory>

namespace Threading {
	class ThreadControls;

	void ThreadStart(
		std::function<void()> taskFunc,
		std::shared_ptr<ThreadControls>* ppCtlsReturn,
		ThreadControls* tempCtls
	);
}

#endif // THREADSIGNALHANDLER_H
