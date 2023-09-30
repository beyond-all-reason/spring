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
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi/Core/Profiling.h>
#include <SDL.h>
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"

// #include "System/FileSystem/ArchiveScanner.h"
// #include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/DataDirsAccess.h"
// #include "System/FileSystem/FileHandler.h"
// #include "System/FileSystem/FileSystem.h"
// #include "System/FileSystem/FileSystemInitializer.h"

#if defined RMLUI_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#else
#if !(SDL_VIDEO_RENDER_OGL)
#error "Only the OpenGL SDL backend is supported."
#endif
#endif

/**
		Custom render interface example for the SDL/GL3 backend.

		Overloads the OpenGL3 render interface to load textures through SDL_image's built-in texture loading functionality.
 */
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
		const std::string &fsFullPath = dataDirsAccess.LocateFile(path);
		return (Rml::FileHandle)std::fopen(fsFullPath.c_str(), "rb");
	}

	void Close(Rml::FileHandle file)
	{
		fclose((FILE *)file);
	}

	size_t Read(void *buffer, size_t size, Rml::FileHandle file)
	{
		return fread(buffer, 1, size, (FILE *)file);
	}

	bool Seek(Rml::FileHandle file, long offset, int origin)
	{
		return fseek((FILE *)file, offset, origin) == 0;
	}

	size_t Tell(Rml::FileHandle file)
	{
		return ftell((FILE *)file);
	};
};

/**
		Global data used by this backend.

		Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData
{
	SystemInterface_SDL system_interface;
	RenderInterface_GL3_SDL render_interface;
	VFSFileInterface file_interface;

	SDL_Window *window = nullptr;
	SDL_GLContext glcontext = nullptr;
	std::vector<Rml::Context *> contexts;

	bool running = true;
};
static Rml::UniquePtr<BackendData> data;

bool RmlGui::Initialize(SDL_Window *target_window, SDL_GLContext target_glcontext)
{
	RMLUI_ASSERT(!data);

	data = Rml::MakeUnique<BackendData>();

	if (!data->render_interface)
	{
		data.reset();
		fprintf(stderr, "Could not initialize OpenGL3 render interface.");
		return false;
	}

	data->window = target_window;
	data->glcontext = target_glcontext;

	// Rml::SetFileInterface(&data->file_interface);
	Rml::SetSystemInterface(RmlGui::GetSystemInterface());
	Rml::SetRenderInterface(RmlGui::GetRenderInterface());

	data->system_interface.SetWindow(target_window);
	data->render_interface.SetViewport(1500, 1500);

	return true;
}

void RmlGui::Shutdown()
{
	RMLUI_ASSERT(data);

	// SDL_GL_DeleteContext(data->glcontext);
	// SDL_DestroyWindow(data->window);

	// data.reset();

	// SDL_Quit();
}

Rml::SystemInterface *RmlGui::GetSystemInterface()
{
	RMLUI_ASSERT(data);
	return &data->system_interface;
}

Rml::RenderInterface *RmlGui::GetRenderInterface()
{
	RMLUI_ASSERT(data);
	return &data->render_interface;
}

bool RmlGui::ProcessEvents(Rml::Context *context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

#if defined RMLUI_PLATFORM_EMSCRIPTEN

	// Ideally we would hand over control of the main loop to emscripten:
	//
	//  // Hand over control of the main loop to the WebAssembly runtime.
	//  emscripten_set_main_loop_arg(EventLoopIteration, (void*)user_data_handle, 0, true);
	//
	// The above is the recommended approach. However, as we don't control the main loop here we have to make due with another approach. Instead, use
	// Asyncify to yield by sleeping.
	// Important: Must be linked with option -sASYNCIFY
	emscripten_sleep(1);

#endif

	bool result = data->running;
	data->running = true;

	SDL_Event ev;
	int has_event = 0;
	if (power_save)
		has_event = SDL_WaitEventTimeout(&ev, static_cast<int>(Rml::Math::Min(context->GetNextUpdateDelay(), (double)10.0) * 1000));
	else
		has_event = SDL_PollEvent(&ev);
	while (has_event)
	{
		switch (ev.type)
		{
		case SDL_QUIT:
		{
			result = false;
		}
		break;
		case SDL_KEYDOWN:
		{
			const Rml::Input::KeyIdentifier key = RmlSDL::ConvertKey(ev.key.keysym.sym);
			const int key_modifier = RmlSDL::GetKeyModifierState();
			const float native_dp_ratio = 1.f;

			// See if we have any global shortcuts that take priority over the context.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, true))
				break;
			// Otherwise, hand the event over to the context by calling the input handler as normal.
			if (!RmlSDL::InputEventHandler(context, ev))
				break;
			// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, false))
				break;
		}
		break;
		case SDL_WINDOWEVENT:
		{
			switch (ev.window.event)
			{
			case SDL_WINDOWEVENT_SIZE_CHANGED:
			{
				Rml::Vector2i dimensions(ev.window.data1, ev.window.data2);
				data->render_interface.SetViewport(dimensions.x, dimensions.y);
			}
			break;
			}
			RmlSDL::InputEventHandler(context, ev);
		}
		break;
		default:
		{
			RmlSDL::InputEventHandler(context, ev);
		}
		break;
		}
		has_event = SDL_PollEvent(&ev);
	}

	return result;
}

void RmlGui::RequestExit()
{
	RMLUI_ASSERT(data);

	data->running = false;
}

void RmlGui::CreateContext()
{
	Rml::Context *context = Rml::CreateContext("overlay", Rml::Vector2i(1500, 1500));
	Rml::Debugger::Initialise(context);
	RmlGui::AddContext(context);
}

void RmlGui::CreateOverlayContext()
{
	Rml::Context *context = Rml::CreateContext("overlay", Rml::Vector2i(1500, 1500));
	Rml::Debugger::Initialise(context);
	RmlGui::AddContext(context);
	Rml::ElementDocument *document = context->LoadDocument("assets/demo.rml");
	if (document)
		document->Show();
}
void RmlGui::AddContext(Rml::Context *context)
{
	data->contexts.push_back(context);
}

void RmlGui::Update()
{
	RMLUI_ASSERT(data);
	// TODO: define if headless?
	for (auto &context : data->contexts)
	{
		context->Update();
	}
}

void RmlGui::RenderFrame()
{
	RMLUI_ASSERT(data);

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
	RMLUI_ASSERT(data);

	// data->render_interface.Clear();
	data->render_interface.BeginFrame();
}

void RmlGui::PresentFrame()
{
	RMLUI_ASSERT(data);

	data->render_interface.EndFrame();
	// SDL_GL_SwapWindow(data->window);

	// Optional, used to mark frames during performance profiling.
	RMLUI_FrameMark;
}
