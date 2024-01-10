/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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
 *
 */

#include "RmlUi_Backend.h"
#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_GL3.h"
#include "RmlUi_Renderer_Headless.h"
#include <RmlUi/Core.h>
#include <RmlUi/Lua.h>
#include <RmlUi/Debugger.h>
#include <RmlUi/Core/Profiling.h>
#include <SDL.h>
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Lua/LuaUI.h"
#include <RmlSolLua/RmlSolLua.h>
#include "System/FileSystem/FileHandler.h"
#include <functional>
#include <tracy/Tracy.hpp>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/Input/InputHandler.h"
#include "Rml/RmlInputReceiver.h"
#include "Game/UI/MouseHandler.h"

void createContext(const std::string &name);

class RenderInterface_GL3_SDL : public RenderInterface_GL3
{
public:
	RenderInterface_GL3_SDL() {}

	bool LoadTexture(Rml::TextureHandle &texture_handle, Rml::Vector2i &texture_dimensions, const Rml::String &source) override
	{
		CBitmap bmp;
		if (!bmp.Load(source))
		{
			return false;
		}
		SDL_Surface *surface = bmp.CreateSDLSurface();

		bool success = false;
		if (surface)
		{
			texture_dimensions.x = surface->w;
			texture_dimensions.y = surface->h;

			if (surface->format->format != SDL_PIXELFORMAT_RGBA32)
			{
				SDL_SetSurfaceAlphaMod(surface, SDL_ALPHA_OPAQUE);
				SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

				SDL_Surface *new_surface = SDL_CreateRGBSurfaceWithFormat(0, surface->w, surface->h, 32, SDL_PIXELFORMAT_RGBA32);
				if (!new_surface)
					return false;

				if (SDL_BlitSurface(surface, 0, new_surface, 0) != 0)
					return false;

				SDL_FreeSurface(surface);
				surface = new_surface;
			}

			success = RenderInterface_GL3::GenerateTexture(texture_handle, (const Rml::byte *)surface->pixels, texture_dimensions);
			SDL_FreeSurface(surface);
		}

		return success;
	}
};

class VFSFileInterface : public Rml::FileInterface
{
public:
	VFSFileInterface() {}
	Rml::FileHandle Open(const Rml::String &path)
	{
		// LOG_L(L_FATAL, "[SpringApp::%s]OPENING: %s %d", __func__, path.c_str(), CLuaHandle::GetDevMode());
		const std::string mode = SPRING_VFS_RAW_FIRST;
		CFileHandler *fh = new CFileHandler(path, mode);
		if (!fh->FileExists())
		{
			delete fh;
			return (Rml::FileHandle) nullptr;
		}
		return (Rml::FileHandle)fh;
	}

	void Close(Rml::FileHandle file)
	{
		((CFileHandler *)file)->Close();
		delete (CFileHandler *)file;
	}

	size_t Read(void *buffer, size_t size, Rml::FileHandle file)
	{
		return ((CFileHandler *)file)->Read(buffer, size);
	}

	bool Seek(Rml::FileHandle file, long offset, int origin)
	{
		std::ios_base::seekdir seekdir;
		switch (origin)
		{
		case SEEK_CUR:
			seekdir = std::ios_base::cur;
			break;
		case SEEK_END:
			seekdir = std::ios_base::end;
			break;
		case SEEK_SET:
		default:
			seekdir = std::ios_base::beg;
			break;
		}
		((CFileHandler *)file)->Seek(offset, seekdir);
		// TODO: need to detect seek failure and then return false?
		return true;
	}

	size_t Tell(Rml::FileHandle file)
	{
		return ((CFileHandler *)file)->GetPos();
	};
};

/**
		Global data used by this backend.

		Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData
{
	SystemInterface_SDL system_interface;
#ifndef HEADLESS
	RenderInterface_GL3_SDL render_interface;
#else
	RenderInterface_Headless render_interface;
#endif
	VFSFileInterface file_interface;

	SDL_Window *window = nullptr;
	SDL_GLContext glcontext = nullptr;
	std::vector<Rml::Context *> contexts;
	InputHandler::SignalType::connection_type inputCon;
	CRmlInputReceiver inputReceiver;

	// make atomic_bool?
	bool initialized = false;
	bool debuggerAttached = false;
	int winX = 1;
	int winY = 1;
	lua_State *ls;
};
static Rml::UniquePtr<BackendData> data;

bool RmlInitialized()
{
	return data && data->initialized;
}

bool RmlGui::Initialize(SDL_Window *target_window, SDL_GLContext target_glcontext, int winX, int winY)
{
	data = Rml::MakeUnique<BackendData>();

	if (!data->render_interface)
	{
		data.reset();
		fprintf(stderr, "Could not initialize OpenGL3 render interface.");
		return false;
	}

	data->window = target_window;
	data->glcontext = target_glcontext;

	Rml::SetFileInterface(&data->file_interface);
	Rml::SetSystemInterface(RmlGui::GetSystemInterface());
	Rml::SetRenderInterface(RmlGui::GetRenderInterface());

	data->system_interface.SetWindow(target_window);
	data->render_interface.SetViewport(winX, winY);
	data->winX = winX;
	data->winY = winY;

	Rml::Initialise();

	Rml::LoadFontFace("fonts/FreeSansBold.otf", true);
	data->inputCon = input.AddHandler(&RmlGui::ProcessEvent);
	data->initialized = true;
	return true;
}

bool RmlGui::InitializeLua(lua_State *lua_state)
{
	if (!RmlInitialized())
	{
		return false;
	}
	sol::state_view lua(lua_state);
	data->ls = lua_state;
	Rml::SolLua::SolLuaPlugin *slp = Rml::SolLua::Initialise(&lua, createContext);
	data->system_interface.SetTranslationTable(&slp->translationTable);
	return true;
}

void RmlGui::Shutdown()
{
	if (!RmlInitialized())
	{
		return;
	}
	data->initialized = false;

	// SDL_GL_DeleteContext(data->glcontext);
	// SDL_DestroyWindow(data->window);
	Rml::Shutdown();
	// data.reset();

	// SDL_Quit();
}

void RmlGui::Reload()
{
	if (!RmlInitialized())
	{
		return;
	}
	LOG_L(L_FATAL, "[SpringApp::%s] reloading: ", __func__);
	SDL_Window *window = data->window;
	SDL_GLContext glcontext = data->glcontext;
	int winX = data->winX;
	int winY = data->winY;
	RmlGui::Shutdown();
	RmlGui::Initialize(window, glcontext, winX, winY);
}

void RmlGui::ToggleDebugger(int contextIndex)
{
	if (data->debuggerAttached)
	{
		Rml::Debugger::Initialise(data->contexts[contextIndex]);
		Rml::Debugger::SetVisible(true);
	}
	else
	{
		Rml::Debugger::Shutdown();
	}
	data->debuggerAttached = !data->debuggerAttached;
}

Rml::SystemInterface *RmlGui::GetSystemInterface()
{
	return &data->system_interface;
}

Rml::RenderInterface *RmlGui::GetRenderInterface()
{
	return &data->render_interface;
}

bool RmlGui::IsActive()
{
	if (!RmlInitialized())
	{
		return false;
	}
	return data->inputReceiver.IsAbove(0, 0);
}

void createContext(const std::string &name)
{
	Rml::Context *context = Rml::CreateContext(name, Rml::Vector2i(data->winX, data->winY));
	// Rml::Debugger::Initialise(context);
	RmlGui::AddContext(context);
}

void RmlGui::CreateContext(const std::string &name)
{
	createContext(name);
}

void RmlGui::AddContext(Rml::Context *context)
{
	data->contexts.push_back(context);
}

void RmlGui::Update()
{
	ZoneScopedN("RmlGui Update");
	if (!RmlInitialized())
	{
		return;
	}
#ifndef HEADLESS
	for (auto &context : data->contexts)
	{
		context->Update();
	}
#endif
}

void RmlGui::RenderFrame()
{
	ZoneScopedN("RmlGui Draw");
	if (!RmlInitialized())
	{
		return;
	}

#ifndef HEADLESS
	RmlGui::BeginFrame();
	for (auto &context : data->contexts)
	{
		context->Render();
	}
	RmlGui::PresentFrame();
#endif
}

void RmlGui::BeginFrame()
{
	// data->render_interface.Clear();
	data->render_interface.BeginFrame();
}

void RmlGui::PresentFrame()
{
	data->render_interface.EndFrame();
	RMLUI_FrameMark;
}

/*
	Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMouseMove(int x, int y, int dx, int dy, int button)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		result |= !RmlSDL::EventMouseMove(context, x, y);
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
	Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMousePress(int x, int y, int button)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		bool handled = !RmlSDL::EventMousePress(context, x, y, button);
		result |= handled;
		if (!handled)
		{
			Rml::Element *el = context->GetFocusElement();
			if (el)
			{
				el->Blur();
			}
		}
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
	Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMouseRelease(int x, int y, int button)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		result |= !RmlSDL::EventMouseRelease(context, x, y, button);
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
	Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMouseWheel(float delta)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		result |= !RmlSDL::EventMouseWheel(context, delta);
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
	Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessKeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		auto kc = RmlSDL::ConvertKey(keyCode);
		result |= !RmlSDL::EventKeyDown(context, kc);
	}
	return result;
}

bool RmlGui::ProcessKeyReleased(int keyCode, int scanCode)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		result |= !RmlSDL::EventKeyUp(context, RmlSDL::ConvertKey(keyCode));
	}
	return result;
}

bool RmlGui::ProcessTextInput(const std::string &text)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		result |= !RmlSDL::EventTextInput(context, text);
	}
	return result;
}

bool processContextEvent(Rml::Context *context, const SDL_Event &event)
{
	switch (event.type)
	{
	case SDL_WINDOWEVENT:
	{
		switch (event.window.event)
		{
		case SDL_WINDOWEVENT_SIZE_CHANGED:
		{
			Rml::Vector2i dimensions(event.window.data1, event.window.data2);
			data->render_interface.SetViewport(dimensions.x, dimensions.y);
			data->winX = dimensions.x;
			data->winY = dimensions.y;
		}
		break;
		}
		RmlSDL::InputEventHandler(context, event);
	}
	break;
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEWHEEL:
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	case SDL_TEXTINPUT:
		break; // handled elsewhere
	default:
	{
		RmlSDL::InputEventHandler(context, event);
	}
	break;
	}
	// these events are not captured, and should continue propogating
	return false;
}

bool RmlGui::ProcessEvent(const SDL_Event &event)
{
	if (!RmlInitialized())
	{
		return false;
	}
	bool result = false;
	for (auto &context : data->contexts)
	{
		result |= processContextEvent(context, event);
	}
	return result;
}
