/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LINE_DRAWER_H
#define _LINE_DRAWER_H

#include <vector>
#include <array>

#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/myGL.h"

class CLineDrawer {
	public:
		CLineDrawer() {
			for (auto& line : allLines) {
				line.reserve(128);
			}
		}

		void Configure(bool useColorRestarts_, bool useRestartColor_, const float* restartColor_, float restartAlpha_) {
			restartAlpha = restartAlpha_;
			restartColor = restartColor_;
			useRestartColor = useRestartColor_;
			useColorRestarts = useColorRestarts_;
		}

		void SetupLineStipple();
		void UpdateLineStipple();
		               
		void StartPath(const float3& pos, const float* color) {
			lastPos = pos;
			lastColor = color;
			Restart();
		}
		void FinishPath() const {} // noop, left for compatibility
		void DrawLine(const float3& endPos, const float* color);
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
		const float* restartColor = nullptr;
		
		float3 lastPos = {};
		const float* lastColor = nullptr;
		
		float stippleTimer = 0.0f;

		// queue all lines and draw them in one go later
		struct LinePair {
			GLenum type;
			std::vector<GLfloat> verts;
			std::vector<GLfloat> colors;
		};

		std::array<std::vector<LinePair>, 2> allLines; // [0] = solid, [1] = strippled
};

extern CLineDrawer lineDrawer;

#endif // _LINE_DRAWER_H
