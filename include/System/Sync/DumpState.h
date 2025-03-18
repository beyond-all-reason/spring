/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DUMPSTATE_H
#define DUMPSTATE_H

#include <optional>

extern void DumpState(int startFrameNum, int endFrameNum, int newFramePeriod, std::optional<bool> outputFloats, bool serverRequest = false);
extern void DumpRNG(int startFrameNum, int endFrameNum);

#endif /* DUMPSTATE_H */
