/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYSET_H
#define KEYSET_H

#include <string>
#include <deque>
#include "IKeys.h"
#include "System/Misc/SpringTime.h"


class CKeySet {
	public:

		enum CKeySetType { KSScanCode, KSKeyCode };

		CKeySet() { Reset(); }
		CKeySet(int key);
		CKeySet(int key, CKeySetType codeType);
		CKeySet(int key, unsigned char modifiers, CKeySetType codeType);

		static unsigned char GetCurrentModifiers();
		static std::string GetHumanModifiers(unsigned char modifiers);

		void Reset();
		void SetAnyBit();
		void ClearModifiers();
		void SanitizeModifiers();
		bool Parse(const std::string& token, bool showerror = true);

		std::string GetString(bool useDefaultKeysym) const;
		std::string GetCodeString() const;

		enum CKeySetModifiers {
			KS_ALT     = (1 << 0),
			KS_CTRL    = (1 << 1),
			KS_META    = (1 << 2),
			KS_SHIFT   = (1 << 3),
			KS_ANYMOD  = (1 << 4),
			//KS_RELEASE = (1 << 5) Deprecated, need rework for enabling separate release bindings
		};

		int  Key()     const { return key; }
		unsigned char Mod() const { return modifiers; }
		bool Alt()     const { return !!(modifiers & KS_ALT); }
		bool Ctrl()    const { return !!(modifiers & KS_CTRL); }
		bool Meta()    const { return !!(modifiers & KS_META); }
		bool Shift()   const { return !!(modifiers & KS_SHIFT); }
		bool AnyMod()  const { return !!(modifiers & KS_ANYMOD); }

		bool IsModifier() const;
		bool IsKeyCode() const;
		IKeys* GetKeys() const;

		bool operator<(const CKeySet& ks) const
		{
			if (key < ks.key) { return true; }
			if (key > ks.key) { return false; }
			if (modifiers < ks.modifiers) { return true; }
			if (modifiers > ks.modifiers) { return false; }
			return false;
		}

		bool fit(const CKeySet& ks) const
		{
			if (type != ks.type || key != ks.key)
				return false;

			// If any of either keyset being checked has the any bit set we have to check
			// that all bits in the any keyset match on the other one. E.g.:
			//
			//   shift+ctrl fits any+ctrl (ctrl is present on the first ks)
			//   alt+shift does not fit any+ctrl (ctrl is not present on the first ks)
			if (ks.AnyMod()) {
				return ((ks.modifiers & ~KS_ANYMOD) & modifiers) == (ks.modifiers & ~KS_ANYMOD);
			}
			else if (AnyMod()) {
				return ((modifiers & ~KS_ANYMOD) & ks.modifiers) == (modifiers & ~KS_ANYMOD);
			}
			else {
				return modifiers == ks.modifiers;
			}
		}

		bool operator==(const CKeySet& ks) const
		{
			return (type == ks.type) && (key == ks.key) && (modifiers == ks.modifiers);
		}

		bool operator!=(const CKeySet& ks) const
		{
			return (type != ks.type) || (key != ks.key) || (modifiers != ks.modifiers);
		}

	protected:
		bool ParseModifier(std::string& s, const std::string& token, const std::string& abbr);

	protected:
		int key;
		CKeySetType type;
		unsigned char modifiers;
};


class CKeyChain : public std::deque<CKeySet>
{
	public:
		bool operator<(const CKeyChain& kc) const
		{
			if (size() < kc.size()) { return true;  }
			if (size() > kc.size()) { return false; }
			if (empty())            { return false; }
			return (back() < kc.back());
		}

		bool operator==(const CKeyChain& needle) const
		{
			if (size() != needle.size())
				return false;

			return std::equal(needle.rbegin(), needle.rend(), rbegin());
		}

		bool fit(const CKeyChain& needle) const
		{
			// Scans the chain (*this) for a needle from the _back_
			// e.g. chain=a,b,c,d will fit needle=b,c,d but won't fit needle=a,b,c!

			if (size() < needle.size())
				return false;

			return std::equal(needle.rbegin(), needle.rend(), rbegin(), [](const CKeySet& a, const CKeySet& b) { return b.fit(a); });
		}

		std::string GetString() const
		{
			std::string s;
			for (const CKeySet& ks: *this) {
				if (!s.empty()) s += ",";
				s += ks.GetString(true);
			}
			return s;
		}
};


class CTimedKeyChain : public CKeyChain
{
	public:
		std::deque<spring_time> times;

		void clear()
		{
			CKeyChain::clear();
			times.clear();
		}

		void push_back(const CKeySet& ks, const spring_time t, const bool isRepeat);
		void emplace_back(const CKeySet& ks, const spring_time t) { assert(false); }
};


#endif /* KEYSET_H */
