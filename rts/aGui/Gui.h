/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_H
#define GUI_H

#include <list>
#include "System/Input/InputHandler.h"

union SDL_Event;

namespace agui
{

class GuiElement;

class Gui
{
public:
	Gui();
	~Gui();

	void Clean();
	void Draw();
	void AddElement(GuiElement*, bool asBackground = false);
	/// deletes the element on the next draw
	void RmElement(GuiElement*);

	void UpdateScreenGeometry(int screenx, int screeny, int screenOffsetX, int screenOffsetY);

	bool MouseOverElement(const GuiElement*, int x, int y) const;

	bool HandleEvent(const SDL_Event& ev);
private:
	InputHandler::HandlerTokenT inputCon;

	struct GuiItem
	{
		GuiItem(GuiElement* el, bool back) : element(el), asBackground(back) {};
		GuiElement* element;
		bool asBackground;
	};
	typedef std::list<GuiItem> ElList;
	ElList elements;
	ElList toBeRemoved;
	ElList toBeAdded;
};

extern Gui* gui;

}

#endif
