/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "Game/Action.h"
#include "Game/Console.h"

#include "System/FileSystem/SimpleParser.h"
#include "System/UnorderedMap.hpp"

#ifndef MOUSEBINDINGS_H
#define MOUSEBINDINGS_H

class CMouseBindings : public CommandReceiver 
{
    public:
        struct BindingModifiers {
            BindingModifiers(int raw) : raw(raw) {}
            BindingModifiers(bool alt, bool ctrl, bool meta, bool shift, bool any = false);
            int raw = 0;

            bool Alt()     const { return !!(raw & KS_ALT); }
            bool Ctrl()    const { return !!(raw & KS_CTRL); }
            bool Meta()    const { return !!(raw & KS_META); }
            bool Shift()   const { return !!(raw & KS_SHIFT); }
            bool AnyMod()  const { return !!(raw & KS_ANYMOD); }

            enum Modifier {
                KS_ALT     = (1 << 0),
                KS_CTRL    = (1 << 1),
                KS_META    = (1 << 2),
                KS_SHIFT   = (1 << 3),
                KS_ANYMOD  = (1 << 4),
            };
        };

        struct MousePress {
            MousePress(BindingModifiers modifiers, int mouseButton, std::string rawString = "") : modifiers(modifiers), mouseButton(mouseButton), rawString(rawString) {};
            BindingModifiers modifiers;
            int mouseButton;
            std::string rawString;
        };

        struct MouseBinding  {
            MouseBinding() {}
            MouseBinding(const std::string& rawline, spring::unsynced_map<std::string, int> aliases, spring::unsynced_map<std::string, int> modifiers);

            Action action;
            std::vector<MousePress> pressChain;
            std::string rawChain;
        };

        typedef std::vector<MouseBinding> MouseBindingList;

    public:
        void Init();
        void Kill();

        MouseBindingList GetBindingList() { return bindingList; }

        virtual void PushAction(const Action&);
        bool ExecuteCommand(const std::string& line);
        bool ExecuteCommand(const std::string& command, const std::string& extra);

        bool Save(const std::string& filename);
        void Load(const std::string& filename);
        void Reload(const std::string& filename);
        void Bind(const std::string& rawline);
        void UnBind(const std::string& rawline);
        void UnBindAll();

        std::string ConfigStringFromCurrent();
        static bool MouseChainsAreEqual(std::vector<MousePress>& lhs, std::vector<MousePress>& rhs);
        static bool BindingsAreEqual(CMouseBindings::MouseBinding& lhs, CMouseBindings::MouseBinding& rhs);

        bool GetDebugEnabled() { return debugEnabled; }
        void SetDebugEnabled(bool enabled);

    private:
        MouseBindingList bindingList;

        spring::unsynced_map<std::string, int> aliases;
        spring::unsynced_map<std::string, int> modifiers;

        bool debugEnabled = false;
};

extern CMouseBindings mouseBindings;

#endif /* MOUSEBINDINGS_H */
