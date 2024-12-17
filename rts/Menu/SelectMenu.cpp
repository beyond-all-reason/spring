/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectMenu.h"

#include <SDL_keycode.h>
#include <functional>
#include <sstream>
#include <stack>

#include "SelectionWidget.h"
#include "System/AIScriptHandler.h"
#include "Game/ClientSetup.h"
#include "Game/GameVersion.h"
#include "Game/GlobalUnsynced.h"
#include "Game/PreGame.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Input/InputHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/MsgStrings.h"
#include "System/StartScriptGen.h"
#include "Rendering/GlobalRendering.h"
#include "aGui/Gui.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/Button.h"
#include "aGui/LineEdit.h"
#include "aGui/TextElement.h"
#include "aGui/Window.h"
#include "aGui/Picture.h"
#include "aGui/List.h"
#include "alphanum.hpp"

using std::string;
using agui::Button;
using agui::HorizontalLayout;

CONFIG(std::string, address).defaultValue("").description("Last Ip/hostname used as direct connect in the menu.");
CONFIG(std::string, LastSelectedSetting).defaultValue("").description("Stores the previously selected setting, when editing settings within the Spring main menu.");
CONFIG(std::string, MenuArchive).defaultValue("Spring Bitmaps").description("Archive name for the default Menu.");

class ConnectWindow : public agui::Window {
public:
	ConnectWindow() : agui::Window("Connect to server") {
		agui::gui->AddElement(this);
		SetPos(0.5, 0.5);
		SetSize(0.4, 0.2);

		agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
		HorizontalLayout* input = new HorizontalLayout(wndLayout);
		/*agui::TextElement* label = */new agui::TextElement("Address:", input); // will be deleted in input
		address = new agui::LineEdit(input);
		address->DefaultAction = std::bind(&ConnectWindow::Finish, this, true);
		address->SetFocus(true);
		address->SetContent(configHandler->GetString("address"));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* connect = new Button("Connect", buttons);
		connect->Clicked = std::bind(&ConnectWindow::Finish, this, true);
		Button* close = new Button("Close", buttons);
		close->Clicked = std::bind(&ConnectWindow::Finish, this, false);
		GeometryChange();
	}

	OnClickStringType Connect;
	agui::LineEdit* address;

private:
	void Finish(bool connect) {
		if (connect)
			Connect(address->GetContent());
		else
			WantClose();
	};
};

class SettingsWindow : public agui::Window {
public:
	SettingsWindow(std::string &name) : agui::Window(name) {
		agui::gui->AddElement(this);
		SetPos(0.5, 0.5);
		SetSize(0.4, 0.2);

		agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
		HorizontalLayout* input = new HorizontalLayout(wndLayout);
		/*agui::TextElement* value_label = */new agui::TextElement("Value:", input); // will be deleted in input
		value = new agui::LineEdit(input);
		value->DefaultAction = std::bind(&SettingsWindow::Finish, this, true);
		value->SetFocus(true);
		if (configHandler->IsSet(name))
			value->SetContent(configHandler->GetString(name));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* ok = new Button("OK", buttons);
		ok->Clicked = std::bind(&SettingsWindow::Finish, this, true);
		Button* close = new Button("Cancel", buttons);
		close->Clicked = std::bind(&SettingsWindow::Finish, this, false);
		GeometryChange();
	}

	OnClickStringType OK;
	agui::LineEdit* value;

private:
	void Finish(bool set) {
		if (set)
			OK(title + " = " + value->GetContent());
		else
			WantClose();
	};
};




SelectMenu::SelectMenu(std::shared_ptr<ClientSetup> setup)
: GuiElement(nullptr)
, clientSetup(setup)
, conWindow(nullptr)
, settingsWindow(nullptr)
, curSelect(nullptr)
{
	SetPos(0, 0);
	SetSize(1, 1);
	agui::gui->AddElement(this, true);

	{ // GUI stuff
		agui::Picture* background = new agui::Picture(this);

		{
			// can not conflict with LuaMenu archive, just keep in VFS if it was not already
			vfsHandler->SetName("SelMenuVFS");
			vfsHandler->AddArchiveIf(configHandler->GetString("MenuArchive"), false);
			vfsHandler->SetName("SpringVFS");

			//TODO: select by resolution / aspect ratio with fallback image
			const std::vector<std::string> files = CFileHandler::FindFiles("bitmaps/ui/background/", "*");

			if (!files.empty())
				background->Load(files[ guRNG.NextInt(files.size()) ]);
		}

		selw = new SelectionWidget(this);
		agui::VerticalLayout* menu = new agui::VerticalLayout(this);
		menu->SetPos(0.1, 0.5);
		menu->SetSize(0.4, 0.4);
		menu->SetBorder(1.2f);
		/*agui::TextElement* title = */new agui::TextElement("Spring " + SpringVersion::GetFull(), menu); // will be deleted in menu
		Button* testGame = new Button("Test Game", menu);
		testGame->Clicked = std::bind(&SelectMenu::Single, this);

		Button* playDemo = new Button("Play Demo", menu);
		playDemo->Clicked = std::bind(&SelectMenu::Demo, this);

		Button* loadGame = new Button("Load Game", menu);
		loadGame->Clicked = std::bind(&SelectMenu::Load, this);

		userSetting = configHandler->GetString("LastSelectedSetting");
		Button* editsettings = new Button("Edit Settings", menu);
		editsettings->Clicked = std::bind(&SelectMenu::ShowSettingsList, this);

		Button* directConnect = new Button("Direct Connect", menu);
		directConnect->Clicked = std::bind(&SelectMenu::ShowConnectWindow, this, true);

		Button* quit = new Button("Quit", menu);
		quit->Clicked = std::bind(&SelectMenu::Quit, this);
		background->GeometryChange();
	}

	ShowConnectWindow(!clientSetup->isHost);
}

SelectMenu::~SelectMenu()
{
	ShowConnectWindow(false);
	ShowSettingsWindow(false, "");
	CleanWindow();
}

bool SelectMenu::Draw()
{
	spring_msecs(10).sleep(true);
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
	ClearScreen();
	agui::gui->Draw();

	return true;
}


void SelectMenu::Demo()
{
	const auto demoSelectedCB = [&](const std::string& userDemo) {
		if (pregame != nullptr)
			return;

		clientSetup->isHost = true;
		clientSetup->myPlayerName += " (spec)";
		clientSetup->demoFile = userDemo;

		pregame = new CPreGame(clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadDemoFile, clientSetup->demoFile);
		//pregame->LoadDemoFile(clientSetup->demoFile);

		return (agui::gui->RmElement(this));
	};

	if (selw->userDemo == SelectionWidget::NoDemoSelect) {
		selw->ShowDemoList(demoSelectedCB);
		return;
	}
}

void SelectMenu::Load()
{
	const auto loadSelectedCB = [&](const std::string& userSave) {
		if (pregame != nullptr)
			return;

		clientSetup->isHost = true;
		clientSetup->saveFile = userSave;

		pregame = new CPreGame(clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadSaveFile, clientSetup->saveFile);
		//pregame->LoadSaveFile(clientSetup->saveFile);

		return (agui::gui->RmElement(this));
	};

	if (selw->userLoad == SelectionWidget::NoSaveSelect) {
		selw->ShowSavegameList(loadSelectedCB);
		return;
	}
}

void SelectMenu::Single()
{
	if (selw->userMod == SelectionWidget::NoModSelect) {
		selw->ShowModList();
		return;
	}
	if (selw->userMap == SelectionWidget::NoMapSelect) {
		selw->ShowMapList();
		return;
	}
	if (selw->userScript == SelectionWidget::NoScriptSelect) {
		selw->ShowScriptList();
		return;
	}

	if (pregame == nullptr) {
		// in case of double-click
		if (selw->userScript == SelectionWidget::SandboxAI)
			selw->userScript.clear();

		pregame = new CPreGame(clientSetup);
		pregame->AsyncExecute(&CPreGame::LoadSetupScript, StartScriptGen::CreateDefaultSetup(selw->userMap, selw->userMod, selw->userScript, clientSetup->myPlayerName));
		//pregame->LoadSetupScript(StartScriptGen::CreateDefaultSetup(selw->userMap, selw->userMod, selw->userScript, clientSetup->myPlayerName));
		return (agui::gui->RmElement(this));
	}
}

void SelectMenu::Quit()
{
	gu->globalQuit = true;
	return (agui::gui->RmElement(this));
}

void SelectMenu::ShowConnectWindow(bool show)
{
	if (show && !conWindow)
	{
		conWindow = new ConnectWindow();
		conWindow->Connect = (std::bind(&SelectMenu::DirectConnect, this, std::placeholders::_1));
		conWindow->WantClose = std::bind(&SelectMenu::ShowConnectWindow, this, false);
	}
	else if (!show && conWindow)
	{
		agui::gui->RmElement(conWindow);
		conWindow = nullptr;
	}
}

void SelectMenu::ShowSettingsWindow(bool show, std::string name)
{
	if (show) {
		if (settingsWindow) {
			agui::gui->RmElement(settingsWindow);
			settingsWindow = nullptr;
		}
		settingsWindow = new SettingsWindow(name);
		settingsWindow->OK = std::bind(&SelectMenu::ShowSettingsWindow, this, false, std::placeholders::_1);
		settingsWindow->WantClose = std::bind(&SelectMenu::ShowSettingsWindow, this, false, "");
	}
	else if (!show && settingsWindow) {
		agui::gui->RmElement(settingsWindow);
		settingsWindow = nullptr;
		const size_t p = name.find(" = ");
		if (p != std::string::npos) {
			configHandler->SetString(name.substr(0, p), name.substr(p + 3));
			ShowSettingsList();
		}
		if (curSelect != nullptr)
			curSelect->list->SetFocus(true);
	}
}

void SelectMenu::ShowSettingsList()
{
	if (curSelect == nullptr) {
		curSelect = new ListSelectWnd("Select setting");
		curSelect->Selected = std::bind(&SelectMenu::SelectSetting, this, std::placeholders::_1);
		curSelect->WantClose = std::bind(&SelectMenu::CleanWindow, this);
	}
	curSelect->list->RemoveAllItems();

	typedef std::map<std::string, std::string, doj::alphanum_less<std::string> > DataSorted;
	const std::map<std::string, std::string>& data = configHandler->GetData();
	const DataSorted dataSorted(data.begin(), data.end());

	for (const auto& item: dataSorted)
		curSelect->list->AddItem(item.first + " = " + item.second, "");

	if (data.find(userSetting) != data.end())
		curSelect->list->SetCurrentItem(userSetting + " = " + configHandler->GetString(userSetting));

	curSelect->list->RefreshQuery();
}

void SelectMenu::SelectSetting(std::string setting) {
	size_t p = setting.find(" = ");
	if(p != std::string::npos)
		setting = setting.substr(0, p);
	userSetting = setting;
	configHandler->SetString("LastSelectedSetting", userSetting);
	ShowSettingsWindow(true, userSetting);
}

void SelectMenu::CleanWindow() {
	if (curSelect) {
		ShowSettingsWindow(false, "");
		agui::gui->RmElement(curSelect);
		curSelect = nullptr;
	}
}

void SelectMenu::DirectConnect(const std::string& addr)
{
	configHandler->SetString("address", addr);

	clientSetup->hostIP = addr;
	clientSetup->isHost = false;

	pregame = new CPreGame(clientSetup);
	return (agui::gui->RmElement(this));
}

bool SelectMenu::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_KEYDOWN: {
			if (ev.key.keysym.sym == SDLK_ESCAPE) {
				LOG("[SelectMenu] user exited");
				Quit();
			} else if (ev.key.keysym.sym == SDLK_RETURN) {
				Single();
				return true;
			}
			break;
		}
	}
	return false;
}
