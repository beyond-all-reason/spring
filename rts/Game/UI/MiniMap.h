/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MINIMAP_H
#define MINIMAP_H

#include <string>
#include <deque>
#include "InputReceiver.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/RenderBuffersFwd.h"
#include "System/Matrix44f.h"
#include "System/Color.h"
#include "System/float3.h"
#include "System/type2.h"


class CUnit;
namespace icon {
	class CIconData;
}

namespace Shader {
	struct IProgramObject;
}

class CMiniMap : public CInputReceiver {
public:
	CMiniMap();
	~CMiniMap() override;

	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx, int dy, int button);
	void MouseRelease(int x, int y, int button);
	void MouseWheel(bool up, float delta);
	void MoveView(int x, int y) { MoveView(GetMapPosition(x, y)); }
	bool IsAbove(int x, int y);
	bool IsInside(int x, int y);
	std::string GetTooltip(int x, int y);
	void Draw() override;
	void DrawForReal(bool useNormalizedCoors = true, bool updateTex = false, bool luaCall = false);
	void Update();

	void ConfigCommand(const std::string& command);
	void ConfigNotify(const std::string& key, const std::string& value);

	float3 GetMapPosition(int x, int y) const;
	CUnit* GetSelectUnit(const float3& pos) const;

	void UpdateGeometry();
	void SetGeometry(int px, int py, int sx, int sy);

	void AddNotification(float3 pos, float3 color, float alpha);

	bool  FullProxy()   const { return fullProxy; }
	bool  ProxyMode()   const { return proxyMode; }
	float CursorScale() const { return cursorScale; }

	void SetMinimized(bool state) { minimized = state; }
	bool GetMinimized() const { return minimized; }
	bool GetMaximized() const { return maximized; }

	int GetPosX()  const { return curPos.x; }
	int GetPosY()  const { return curPos.y; }
	int GetSizeX() const { return curDim.x; }
	int GetSizeY() const { return curDim.y; }
	float GetUnitSizeX() const { return unitSizeX; }
	float GetUnitSizeY() const { return unitSizeY; }

	enum RotationOptions { ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270 };

	void SetRotation(RotationOptions state);
	float GetRotation() const { return static_cast<int>(rotation) * math::HALFPI; }
	int minimapCanFlip = 0;
	RotationOptions GetRotationOption() const { return rotation; }

	void SetSlaveMode(bool value);
	bool GetSlaveMode() const { return slaveDrawMode; }

	bool UseUnitIcons() const { return useIcons; }
	bool UseSimpleColors() const { return simpleColors; }

	const uint8_t* GetMyTeamIconColor() const { return &myColor[0]; }
	const uint8_t* GetAllyTeamIconColor() const { return &allyColor[0]; }
	const uint8_t* GetEnemyTeamIconColor() const { return &enemyColor[0]; }

	const CMatrix44f& GetViewMat(unsigned int idx) const { return viewMats[idx]; }
	const CMatrix44f& GetProjMat(unsigned int idx) const { return projMats[idx]; }

	void ApplyConstraintsMatrix() const;

protected:
	enum MINIMAP_POSITION { MINIMAP_POSITION_LEFT, MINIMAP_POSITION_RIGHT, MINIMAP_POSITION_CENTER };

	void ParseGeometry(const std::string& geostr);
	void ToggleMaximized(bool maxspect);
	void SetMaximizedGeometry();
	void SetAspectRatioGeometry(const float& viewSizeX, const float& viewSizeY,
                              const float& viewPosX = 0, const float& viewPosY = 0,
                              const MINIMAP_POSITION position = MINIMAP_POSITION_CENTER);
	void LoadDualViewport() const;

	void ConfigUpdate();

	void SelectUnits(int x, int y);
	void ProxyMousePress(int x, int y, int button);
	void ProxyMouseRelease(int x, int y, int button);
	void MoveView(const float3& mapPos);

	bool RenderCachedTexture(bool useGeom);
	void DrawBackground() const;
	void DrawUnitIcons() const;
	void DrawUnitRanges() const;
	void DrawWorldStuff() const;
	void DrawCameraFrustumAndMouseSelection();
	void SetClipPlanes(const bool lua) const;

	void DrawFrame();
	void DrawNotes();
	void DrawButtons();
	void DrawMinimizedButtonQuad() const;
	void DrawMinimizedButtonLoop() const;

	void DrawUnitHighlight(const CUnit* unit);
	void DrawCircle(TypedRenderBuffer<VA_TYPE_C>& rb, const float3& pos, SColor color, float radius) const;
	const icon::CIconData* GetUnitIcon(const CUnit* unit, float& scale) const;

	void UpdateTextureCache();
	void ResizeTextureCache();

protected:
	int2 curPos;
	int2 curDim;
	int2 tmpPos;
	int2 oldPos;
	int2 oldDim;

	float minimapRefreshRate = 0.0f;

	float unitBaseSize = 0.0f;
	float unitExponent = 0.0f;

	float unitSizeX = 0.0f;
	float unitSizeY = 0.0f;
	float unitSelectRadius = 0.0f;

	bool aspectRatio = false;
	bool fullProxy = false;
	bool proxyMode = false;
	bool selecting = false;
	bool maxspect = false;
	bool maximized = false;
	bool minimized = false;
	bool mouseEvents = true; // if false, MousePress is not handled
	bool mouseLook = false;
	bool mouseMove = false;
	bool mouseResize = false;

	RotationOptions rotation = ROTATION_0;

	bool slaveDrawMode = false;
	bool simpleColors = false;

	bool showButtons = false;
	bool drawProjectiles = false;
	bool useIcons = true;

	bool renderToTexture = true;
	bool multisampledFBO = false;

	struct IntBox {
		bool Inside(int x, int y) const {
			return ((x >= xmin) && (x <= xmax) && (y >= ymin) && (y <= ymax));
		}
		void GetBoxRenderData(TypedRenderBuffer<VA_TYPE_2DC>& rb, SColor col) const;
		void GetTextureBoxRenderData(TypedRenderBuffer<VA_TYPE_2DT>& rb) const;
		int xmin, xmax, ymin, ymax;
		float xminTx, xmaxTx, yminTx, ymaxTx;  // texture coordinates
	};

	int buttonSize = 0;

	int drawCommands = 0;
	float cursorScale = 0.0f;

	IntBox mapBox;
	IntBox buttonBox;
	IntBox moveBox;
	IntBox resizeBox;
	IntBox minimizeBox;
	IntBox maximizeBox;

	SColor myColor;
	SColor allyColor;
	SColor enemyColor;

	// transforms for [0] := Draw, [1] := DrawInMiniMap, [2] := Lua DrawInMiniMap
	CMatrix44f viewMats[3];
	CMatrix44f projMats[3];

	FBO fbo;
	FBO fboResolve;
	GLuint minimapTex = 0;
	int2 minimapTexSize;

	GLuint buttonsTextureID;

	struct Notification {
		float creationTime;
		float3 pos;
		float color[4];
	};
	std::deque<Notification> notes;

	Shader::IProgramObject* bgShader = nullptr;

	CUnit* lastClicked = nullptr;
};


extern CMiniMap* minimap;


#endif /* MINIMAP_H */
