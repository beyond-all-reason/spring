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
		struct LinePair {
			size_t hash;
			VA_TYPE_C p0;
			VA_TYPE_C p1;
		};
	public:
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
		void Restart() {
			if (!useColorRestarts)
				forceRestart = true;
		}
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

		bool forceRestart = false;
		bool useColorRestarts = false;
		bool useRestartColor = false;

		float restartAlpha = 0.0f;
		SColor restartColor = {};
		
		float3 lastPos = {};
		SColor lastColor = {};
		
		float stippleTimer = 0.0f;

		std::array<std::vector<LinePair>, 2> vertexCaches;
};

extern CLineDrawer lineDrawer;