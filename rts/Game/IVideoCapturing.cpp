/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IVideoCapturing.h"

#if       defined AVI_CAPTURING
#include "AviVideoCapturing.h"
#else  // defined AVI_CAPTURING
#include "DummyVideoCapturing.h"
#endif // defined AVI_CAPTURING

#include "Rendering/GlobalRendering.h"

#include "System/Misc/TracyDefs.h"


IVideoCapturing* IVideoCapturing::GetInstance()
{
#if       defined AVI_CAPTURING
	static AviVideoCapturing instance;
#else  // defined AVI_CAPTURING
	static DummyVideoCapturing instance;
#endif // defined AVI_CAPTURING

	return &instance;
}


void IVideoCapturing::FreeInstance()
{
	RECOIL_DETAILED_TRACY_ZONE;
	SetCapturing(false);
}


bool IVideoCapturing::SetCapturing(bool enable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool isCapturing = GetInstance()->IsCapturing();

	if (!isCapturing && enable) {
		GetInstance()->StartCapturing();
		return true;
	}
	if (isCapturing && !enable) {
		GetInstance()->StopCapturing();
		return true;
	}

	return false;
}

