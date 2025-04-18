/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "KeySet.h"
#include "KeyCodes.h"
#include "ScanCodes.h"
#include "KeyBindings.h"

#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Input/KeyInput.h"

#include <SDL_keycode.h>

#include "System/Misc/TracyDefs.h"


/******************************************************************************/
//
// CKeySet
//

void CKeySet::Reset()
{
	RECOIL_DETAILED_TRACY_ZONE;
	key = -1;
	modifiers = 0;
	type = KSKeyCode;
}


void CKeySet::ClearModifiers()
{
	RECOIL_DETAILED_TRACY_ZONE;
	modifiers &= ~(KS_ALT | KS_CTRL | KS_META | KS_SHIFT);
}


void CKeySet::SetAnyBit()
{
	RECOIL_DETAILED_TRACY_ZONE;
	modifiers |= KS_ANYMOD;
}


bool CKeySet::IsModifier() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return key == CKeyCodes::NONE && modifiers > 0;
}

bool CKeySet::IsKeyCode() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return type == KSKeyCode;
}

IKeys* CKeySet::GetKeys() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (IsKeyCode()) {
		return &keyCodes;
	} else {
		return &scanCodes;
	}
}

CKeySet::CKeySet(int k)
	: CKeySet(k, KSKeyCode) { }
CKeySet::CKeySet(int k, CKeySetType keyType)
	: CKeySet(k, GetCurrentModifiers(), keyType) { }
CKeySet::CKeySet(int k, unsigned char mods, CKeySetType keyType)
{
	type = keyType;
	key = k;
	modifiers = mods;

	SanitizeModifiers();
}


void CKeySet::SanitizeModifiers() {
	if (!GetKeys()->IsModifier(key)) {
		return;
	}

	// Disambiguate pure modifier keysets
	// (mod)Alt+(key)ctrl == (mod)Ctrl+(key)alt -> (mod)Alt+(mod)Ctrl+(key)none
	// This optimizes matching keysets that are combination of multiple pure
	// modifiers
	unsigned char modifier = IsKeyCode() ? CKeyCodes::ToModifier(key) : CScanCodes::ToModifier(key);
	modifiers |= modifier;

	key = CKeyCodes::NONE;
}


unsigned char CKeySet::GetCurrentModifiers()
{
	RECOIL_DETAILED_TRACY_ZONE;
	unsigned char modifiers = 0;

	if (KeyInput::GetKeyModState(KMOD_ALT))   { modifiers |= KS_ALT; }
	if (KeyInput::GetKeyModState(KMOD_CTRL))  { modifiers |= KS_CTRL; }
	if (KeyInput::GetKeyModState(KMOD_GUI))   { modifiers |= KS_META; }
	if (KeyInput::GetKeyModState(KMOD_SHIFT)) { modifiers |= KS_SHIFT; }

	return modifiers;
}


std::string CKeySet::GetHumanModifiers(unsigned char modifiers)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::string modstr;

	if (modifiers & KS_ANYMOD)  { modstr += "Any+"; }
	if (modifiers & KS_ALT)     { modstr += "Alt+"; }
	if (modifiers & KS_CTRL)    { modstr += "Ctrl+"; }
	if (modifiers & KS_META)    { modstr += "Meta+"; }
	if (modifiers & KS_SHIFT)   { modstr += "Shift+"; }

	return modstr;
}


std::string CKeySet::GetString(bool useDefaultKeysym) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::string name;

	const IKeys* keys = GetKeys();
	name = useDefaultKeysym ? keys->GetDefaultName(key) : keys->GetName(key);
	
	return (GetHumanModifiers(modifiers) + name);
}


std::string CKeySet::GetCodeString() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return IsKeyCode() ? CKeyCodes::GetCodeString(key) : CScanCodes::GetCodeString(key);
}


bool CKeySet::ParseModifier(std::string& s, const std::string& token, const std::string& abbr)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (s.starts_with(token)) {
		s.erase(0, token.size());
		return true;
	}
	if (s.starts_with(abbr)) {
		s.erase(0, abbr.size());
		return true;
	}
	return false;
}


bool CKeySet::Parse(const std::string& token, bool showerror)
{
	RECOIL_DETAILED_TRACY_ZONE;
	Reset();

	std::string s = StringToLower(token);

	// parse the modifiers
	while (!s.empty()) {
		if (ParseModifier(s, "up+",    "u+")) { LOG_L(L_DEPRECATED, "KeySet: Up modifier is deprecated"); } else
		if (ParseModifier(s, "any+",   "*+")) { modifiers |= KS_ANYMOD; } else
		if (ParseModifier(s, "alt+",   "a+")) { modifiers |= KS_ALT; } else
		if (ParseModifier(s, "ctrl+",  "c+")) { modifiers |= KS_CTRL; } else
		if (ParseModifier(s, "meta+",  "m+")) { modifiers |= KS_META; } else
		if (ParseModifier(s, "shift+", "s+")) { modifiers |= KS_SHIFT; } else {
			break;
		}
	}

	if (s.empty()) {
		Reset();
		if (showerror) LOG_L(L_ERROR, "KeySet: Missing key");
		return false;
	}

	// remove ''s, if present
	if ((s.size() >= 2) && (s[0] == '\'') && (s[s.size() - 1] == '\''))
		s = s.substr(1, s.size() - 2);

	if (s.find("0x") == 0) {
		type = KSKeyCode;

		const char* start = (s.c_str() + 2);
		char* end;
		key = strtol(start, &end, 16);
		if (end == start) {
			Reset();
			if (showerror) LOG_L(L_ERROR, "KeySet: Bad hex value: %s", s.c_str());
			return false;
		}
	} else if (s.find("sc_0x") == 0) {
		type = KSScanCode;

		const char* start = (s.c_str() + 5);
		char* end;
		key = strtol(start, &end, 16);
		if (end == start) {
			Reset();
			if (showerror) LOG_L(L_ERROR, "KeySet: Bad hex value: %s", s.c_str());
			return false;
		}
	} else if (((key = scanCodes.GetCode(s)) > 0)) {
		type = KSScanCode;
	} else if (((key = keyCodes.GetCode(s)) > 0) || key == CKeyCodes::NONE) {
		type = KSKeyCode;
	} else {
		Reset();
		if (showerror) LOG_L(L_ERROR, "KeySet: Bad keysym: %s", s.c_str());
		return false;
	}

	SanitizeModifiers();
	
	return true;
}


/******************************************************************************/
//
// CTimedKeyChain
//

void CTimedKeyChain::push_back(const CKeySet& ks, const spring_time t, const bool isRepeat)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// clear chain on timeout
	const auto dropTime = t - spring_msecs(keyBindings.GetKeyChainTimeout());

	if (!empty() && times.back() < dropTime)
		clear();

	// append repeating keystrokes only when they differ from the last
	if (isRepeat && (!empty() && ks == back()))
		return;

	// When you want to press c,alt+b you will press the c(down),c(up),alt(down),b(down)
	// -> the chain will be: c,Alt+alt,Alt+b
	// this should now match "c,alt+b", so we need to get rid of the redundant Alt+alt .
	// Still we want to support "Alt+alt,Alt+alt" (double click alt), so only override e.g. the last in chain
	// when the new keypress is _not_ a modifier.
	if (!empty() && !ks.IsModifier() && back().IsModifier() && (back().Mod() == ks.Mod())) {
		times.back() = t;
		back() = ks;
		return;
	}

	// push new to front
	CKeyChain::emplace_back(ks);
	times.emplace_back(t);
}
