#include <vector>
#include <string>

#include "MouseBindings.h"

#include "Game/UI/MouseHandler.h"

#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/StringUtil.h"

#include "System/Log/ILog.h"

CMouseBindings mouseBindings;

const std::string DEFAULT_FILENAME = "mousebinds.txt";

static const std::vector<std::string> defaultBindings = {
    // "bind 1 defaultpress"
};



CMouseBindings::BindingModifiers::BindingModifiers(bool alt, bool ctrl, bool meta, bool shift, bool any) {
    if (alt) { raw = raw | Modifier::KS_ALT; }
    if (ctrl) { raw = raw | Modifier::KS_CTRL; }
    if (meta) { raw = raw | Modifier::KS_META; }
    if (shift) { raw = raw | Modifier::KS_SHIFT; }
    if (any) { raw = raw | Modifier::KS_ANYMOD; }
}



CMouseBindings::MouseBinding::MouseBinding(const std::string& rawline, spring::unsynced_map<std::string, int> aliases, spring::unsynced_map<std::string, int> modifiers) {
    std::vector<std::string> words = CSimpleParser::Tokenize(rawline, 1);


    if (words.size() < 1)
        return;
    rawChain = words[0];
    
    if (words.size() < 2)
        return;
    action = Action(words[1]);
    
    std::vector<std::string> rawMousePresses = CSimpleParser::Split(rawChain, ",");
    pressChain.reserve(rawMousePresses.size());
    for (std::string mousePressRaw : rawMousePresses) {
        std::vector<std::string> rawKeys = CSimpleParser::Split(rawChain, "+");
        int modifierBitfield = 0;
        for (int i = 0; i < rawKeys.size() - 1; i++) {
            const int* modifierCandidate = modifiers.try_get(rawKeys[i]);
            if (modifierCandidate != nullptr)
                modifierBitfield |= *modifierCandidate;
        }

        int mouseButton = 0;

        if (rawKeys.size() > 0) {
            std::string rawMouseButton = rawKeys.back();
            const int* aliasCandidate = aliases.try_get(rawMouseButton);
            if (aliasCandidate != nullptr) {
                mouseButton = *aliasCandidate;
            } else {
                mouseButton = atoi(rawMouseButton.c_str());
            }
        }

        pressChain.push_back(MousePress(
            BindingModifiers(modifierBitfield),
            mouseButton,
            mousePressRaw
        ));
    }
}



void CMouseBindings::Init()
{
    aliases.insert("left", 1);
    aliases.insert("middle", 2);
    aliases.insert("right", 3);

    modifiers.insert("alt", CMouseBindings::BindingModifiers::Modifier::KS_ALT);
    modifiers.insert("ctrl", CMouseBindings::BindingModifiers::Modifier::KS_CTRL);
    modifiers.insert("shift", CMouseBindings::BindingModifiers::Modifier::KS_SHIFT);
    modifiers.insert("meta", CMouseBindings::BindingModifiers::Modifier::KS_META);
    modifiers.insert("any", CMouseBindings::BindingModifiers::Modifier::KS_ANYMOD);

    Load(DEFAULT_FILENAME);

    RegisterAction("mousebind");
    RegisterAction("mouseunbind");
    RegisterAction("mouseunbindall");
    RegisterAction("mousereload");
    RegisterAction("mousebindsave");
    RegisterAction("mousedebug");
    RegisterAction("mousebindlist");

    if (debugEnabled)
        LOG("[MouseBindings] Init");

    Load(DEFAULT_FILENAME);
}

void CMouseBindings::Kill() {
    aliases.clear();
    modifiers.clear();
    bindingList.clear();
}



void CMouseBindings::PushAction(const Action& action) {
    ExecuteCommand(action.command, action.extra);
}

bool CMouseBindings::ExecuteCommand(const std::string& line) {
    const std::vector<std::string> words = CSimpleParser::Tokenize(line, 1);

	if (words.empty())
		return false;

    return ExecuteCommand(StringToLower(words[0]), words[1]);
}

bool CMouseBindings::ExecuteCommand(const std::string& command, const std::string& extra) {
	if (command == "mousebind") {
        Bind(extra);
    } else if (command == "mouseunbind") {
        UnBind(extra);
    } else if (command == "mouseunbindall") {
        UnBindAll();
    } else if (command == "mousereload") {
        Reload(DEFAULT_FILENAME);
    } else if (command == "mousebindsave") {
        Save(DEFAULT_FILENAME);
    } else if (command == "mousedebug") {
        if (extra == "1")
            SetDebugEnabled(true);
        else if (extra == "0")
            SetDebugEnabled(false);
        else
            SetDebugEnabled(not GetDebugEnabled());

        
    } else if (command == "mousebindlist") {
        LOG("Mouse Bindings:");
        for (int i = 0; i < bindingList.size(); i++)
            LOG("%i. Keychain: \"%s\", Action: \"%s\", Extra: \"%s\"", i, bindingList[i].rawChain.c_str(), bindingList[i].action.command.c_str(), bindingList[i].action.extra.c_str());
    }

    return false;
}



bool CMouseBindings::Save(const std::string& filename)
{
    if (debugEnabled)
        LOG("[MouseBindings] Saving %i bindings to %s", (int) bindingList.size(), DEFAULT_FILENAME.c_str());

    FILE* out = fopen(filename.c_str(), "wt");
    if (out == nullptr)
        return false;

    fprintf(out, ConfigStringFromCurrent().c_str());
    fclose(out);
    return true;
}

void CMouseBindings::Load(const std::string& filename) {
    CFileHandler fileHandler(filename);
    CSimpleParser parser(fileHandler);

    while (!parser.Eof()) {
		ExecuteCommand(parser.GetCleanLine());
	}

    if (debugEnabled)
        LOG("[MouseBindings] Loaded %i bindings from %s", (int) bindingList.size(), DEFAULT_FILENAME.c_str());
}

void CMouseBindings::Reload(const std::string& filename) {
    if (debugEnabled)
        LOG("[MouseBindings] Reloading bindings!");
    UnBindAll();
    for (std::string defaultBinding : defaultBindings)
        Bind(defaultBinding);
    Load(DEFAULT_FILENAME);
}

void CMouseBindings::Bind(const std::string& rawline) {
    MouseBinding mouseBinding(rawline, aliases, modifiers);
    if (mouseBinding.action.command == "") {
        LOG("[MouseBindings] Missing action from bind command \"%s\"!", rawline.c_str());
        return;
    }
    if (mouseBinding.pressChain.size() == 0) {
        LOG("[MouseBindings] Missing keychain from bind command \"%s\"!", rawline.c_str());
        return;
    }
    for (MousePress mousePress : mouseBinding.pressChain) {
        if (mousePress.mouseButton == 0) {
            LOG(
                "[MouseBindings] Missing/invalid mouse button from mousePress \"%s\" (full keychain: \"%s\", raw line: \"%s\")! Valid mouse buttons are 1-10, or \"Left\", \"Middle\", \"Right\"", 
                mousePress.rawString.c_str(), 
                mouseBinding.rawChain.c_str(), 
                rawline.c_str()
            );
            return;
        }
        if (mousePress.mouseButton > NUM_BUTTONS) {
            LOG("[MouseBindings] Recoil engine does not support mouse buttons 11 and higher! (full keychain: \"%s\", raw line: \"%s\")!", mouseBinding.rawChain.c_str(), rawline.c_str());
            return;
        }
    }

    for (MouseBinding existingBinding : bindingList) {
        if (BindingsAreEqual(mouseBinding, existingBinding)) {
            LOG("[MouseBindings] Binding \"%s\" already exists!", rawline.c_str());
            return;
        }
    }

    bindingList.push_back(mouseBinding);

    if (debugEnabled) {
        LOG(
            "[MouseBindings] Added binding \"%s\" for mouse+modifier combination \"%s\"",
            mouseBinding.action.rawline.c_str(),
            mouseBinding.rawChain.c_str()
        );
    }
}

void CMouseBindings::UnBind(const std::string& rawline) {
    MouseBinding mouseBinding(rawline, aliases, modifiers);
    if (mouseBinding.action.command == "") {
        LOG("[MouseBindings] Missing action from bind command \"%s\"!", rawline.c_str());
        return;
    }
    if (mouseBinding.pressChain.size() == 0) {
        LOG("[MouseBindings] Missing keychain from bind command \"%s\"!", rawline.c_str());
        return;
    }
    for (MousePress mousePress : mouseBinding.pressChain) {
        if (mousePress.mouseButton == 0) {
            LOG(
                "[MouseBindings] Missing/invalid mouse button from mousePress \"%s\" (full keychain: \"%s\", raw line: \"%s\")! Valid mouse buttons are 1-10, or \"Left\", \"Middle\", \"Right\"", 
                mousePress.rawString.c_str(), 
                mouseBinding.rawChain.c_str(), 
                rawline.c_str()
            );
            return;
        }
        if (mousePress.mouseButton > NUM_BUTTONS) {
            LOG("[MouseBindings] Recoil engine does not support mouse buttons 11 and higher! (full keychain: \"%s\", raw line: \"%s\")!", mouseBinding.rawChain.c_str(), rawline.c_str());
            return;
        }
    }

    auto it = bindingList.begin();

    while (it != bindingList.end()) {
        if (BindingsAreEqual(*it, mouseBinding)) {
            it = bindingList.erase(it);
            if (debugEnabled) {
                LOG(
                    "[MouseBindings] Removed binding \"%s\" for mouse+modifier combination \"%s\"",
                    mouseBinding.action.rawline.c_str(),
                    mouseBinding.rawChain.c_str()
                );
            }
		} else {
			++it;
		}
	}
}

void CMouseBindings::UnBindAll() {
    bindingList.clear();

    if (debugEnabled)
        LOG("[MouseBindings] Removed all bindings! %i bindings remaining.", (int) bindingList.size());
}



std::string CMouseBindings::ConfigStringFromCurrent() {
    std::string out;

    out.append("mouseunbindall\n\n");

    for (MouseBinding& binding : bindingList) {
        out += "mousebind ";
        for (MousePress& mousePress : binding.pressChain) {
            // if (mousePress.modifiers.Any())
            //     out += "Any+";
            if (mousePress.modifiers.Ctrl())
                out += "Ctrl+";
            if (mousePress.modifiers.Alt())
                out += "Alt+";
            if (mousePress.modifiers.Shift())
                out += "Shift+";
            if (mousePress.modifiers.Meta())
                out += "Meta+";

            out += std::to_string(mousePress.mouseButton) + ",";
        }

        out.pop_back(); // Get rid of extra comma

        out += " " + binding.action.command;

        if (binding.action.extra != "")
            out += " " + binding.action.extra;

        out += "\n";
    }

    return out;
}

bool CMouseBindings::MouseChainsAreEqual(std::vector<MousePress>& lhs, std::vector<MousePress>& rhs) {
    for (int i = 0; i < lhs.size(); i++) {
        if (!(lhs[i].mouseButton == rhs[i].mouseButton && lhs[i].modifiers.raw == rhs[i].modifiers.raw))
            return false;
    }
    return true;
}

bool CMouseBindings::BindingsAreEqual(CMouseBindings::MouseBinding& lhs, CMouseBindings::MouseBinding& rhs) {
    if (lhs.action.command != rhs.action.command || lhs.action.extra != rhs.action.extra || lhs.pressChain.size() != rhs.pressChain.size()) 
        return false;

    return MouseChainsAreEqual(lhs.pressChain, rhs.pressChain);
}



void CMouseBindings::SetDebugEnabled(bool enabled) {
    debugEnabled = enabled;
    if (debugEnabled) {
        LOG("[MouseBindings] Debug mode enabled!");
    } else {
        LOG("[MouseBindings] Debug mode disabled!");
    }
}