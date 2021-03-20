/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IInfoTextureHandler.h"
#include "Modern/InfoTextureHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"


IInfoTextureHandler* infoTextureHandler = nullptr;


void IInfoTextureHandler::Create()
{
	infoTextureHandler = new CInfoTextureHandler();
}
