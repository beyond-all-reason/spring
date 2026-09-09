/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Button.h"

#include "Gui.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"

namespace agui
{

Button::Button(const std::string& _label, GuiElement* _parent)
	: GuiElement(_parent)
{
	Label(_label);
}

void Button::Label(const std::string& _label)
{
	label = _label;
	clicked = false;
	hovered = false;
}

#ifdef HEADLESS
void Button::DrawSelf() {}
#else
void Button::DrawSelf()
{
	const float opacity = Opacity();

	DrawBox(GL_QUADS, { 0.8f, 0.8f, 0.8f, opacity });

	if (clicked) {
		glBlendFunc(GL_ONE, GL_ONE); // additive blending

		DrawBox(GL_QUADS, { 0.2f, 0.0f, 0.0f, opacity });
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLineWidth(1.49f);
		DrawBox(GL_LINE_LOOP, { 1.0f, 0.0f, 0.0f, opacity / 2.f });
		glLineWidth(1.0f);
	} else if (hovered) {
		glBlendFunc(GL_ONE, GL_ONE); // additive blending
		DrawBox(GL_QUADS, { 0.0f, 0.0f, 0.2f, opacity });
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLineWidth(1.49f);
		DrawBox(GL_LINE_LOOP, { 1.0f, 1.0f, 1.0f, opacity / 2.0f });
		glLineWidth(1.0f);
	}

	font->Begin();
	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity);
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	font->glPrint(pos[0] + size[0]/2, pos[1] + size[1]/2, 1.0, FONT_CENTER | FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM, label);
	font->End();
}
#endif

bool Button::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_EVENT_MOUSE_BUTTON_DOWN: {
			int mx = int(ev.button.x);
			int my = int(ev.button.y);
			bool mo = MouseOver(mx, my);
			bool moRoot = gui->MouseOverElement(GetRoot(), mx, my);
			if ((ev.button.button == SDL_BUTTON_LEFT)
					&& mo
					&& moRoot)
			{
				clicked = true;
			}
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			int mx = int(ev.button.x);
			int my = int(ev.button.y);
			bool mo = MouseOver(mx, my);
			if ((ev.button.button == SDL_BUTTON_LEFT)
					&& mo
					&& clicked)
			{
				if (Clicked) {
					Clicked();
				}
				clicked = false;
				return true;
			}
			break;
		}
		case SDL_EVENT_MOUSE_MOTION: {
			int mx = int(ev.motion.x);
			int my = int(ev.motion.y);
			bool mo = MouseOver(mx, my);
			if (mo
					&& gui->MouseOverElement(GetRoot(), mx, my))
			{
				hovered = true;
			} else {
				hovered = false;
				clicked = false;
			}
			break;
		}
	}
	return false;
}

} // namespace agui
