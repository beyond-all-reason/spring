/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <array>
#include <memory>

#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/GL/RenderBuffersFwd.h"
#include "Rendering/Colors.h"

class CLineDrawer {
	public:
		CLineDrawer();
		void Configure(bool useColorRestarts_, bool useRestartColor_, const float* restartColor_, float restartAlpha_) {
			restartAlpha = restartAlpha_;
			restartColor = restartColor_;
			useRestartColor = useRestartColor_;
			useColorRestarts = useColorRestarts_;
		}

		void SetupLineStipple();
		void UpdateLineStipple();
		               
		void StartPath(const float3& pos, const SColor& color) {
			lastPos = pos;
			lastColor = color;
			Restart();
		}
		void FinishPath() const {} // noop, left for compatibility
		void DrawLine(const float3& endPos, const SColor& color);
		void DrawLineAndIcon(int cmdID, const float3& endPos, const float* color) {
			cursorIcons.AddIcon(cmdID, endPos);
			DrawLine(endPos, color);
		}
		void DrawIconAtLastPos(int cmdID) {
			cursorIcons.AddIcon(cmdID, lastPos);
		}
		void Break(const float3& endPos, const float* color) {
			lastPos = endPos;
			lastColor = color;
		}
		void Restart();
		/// now same as restart
		void RestartSameColor() { Restart(); } //reportedly broken
		void RestartWithColor(const float* color) {
			lastColor = color;
			Restart();
		}
		const float3& GetLastPos() const { return lastPos; }
		void DrawAll();

	private:
		bool lineStipple = false;
		bool useColorRestarts = false;
		bool useRestartColor = false;
		float restartAlpha = 0.0f;
		SColor restartColor = {};
		
		float3 lastPos = {};
		SColor lastColor = {};
		
		float stippleTimer = 0.0f;

		std::array<std::unique_ptr<TypedRenderBuffer<VA_TYPE_C>>, 4> rbs; // [0] = solid, [1] = strippled, [2] = solid && color-restart, [3] = strippled && color-restart
};

extern CLineDrawer lineDrawer;