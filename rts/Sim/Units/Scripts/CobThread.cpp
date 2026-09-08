/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobThread.h"

#include "CobDeferredCallin.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "CobOpCodes.h"
#include "CobEngine.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

#include "System/Misc/TracyDefs.h"

CR_BIND(CCobThread, )

CR_REG_METADATA(CCobThread, (
	CR_MEMBER(cobInst),
	CR_IGNORED(cobFile),

	CR_MEMBER(id),
	CR_MEMBER(pc),

	CR_MEMBER(wakeTime),
	CR_MEMBER(paramCount),
	CR_MEMBER(retCode),
	CR_MEMBER(cbParam),
	CR_MEMBER(signalMask),

	CR_MEMBER(waitAxis),
	CR_MEMBER(waitPiece),

	CR_IGNORED(errorCounter),

	CR_MEMBER(cbType),
	CR_MEMBER(state),

	CR_MEMBER(luaArgs),
	CR_MEMBER(callStack),
	CR_MEMBER(dataStack)
))

CR_BIND(CCobThread::CallInfo,)

CR_REG_METADATA_SUB(CCobThread, CallInfo,(
	CR_MEMBER(functionId),
	CR_MEMBER(returnAddr),
	CR_MEMBER(stackTop)
))

std::vector<decltype(CCobThread::dataStack)> CCobThread::freeDataStacks;
std::vector<decltype(CCobThread::callStack)> CCobThread::freeCallStacks;

CCobThread::CCobThread(CCobInstance* _cobInst)
	: cobInst(_cobInst)
	, cobFile(_cobInst->cobFile)
{
	// If there are any free data and call stacks available, reuse them by 
	// moving them to the current thread's data and call stack variables to
	// amortize memory allocations.
	if (!freeDataStacks.empty()) {
		assert(freeDataStacks.size() == freeCallStacks.size());
		dataStack = std::move(freeDataStacks.back());
		freeDataStacks.pop_back();
		callStack = std::move(freeCallStacks.back());
		freeCallStacks.pop_back();
	} else {
    	// These reservation sizes were experimentally obtained from a few
    	// games in BAR, but regardless of the game being played, the size of
    	// all stacks in use will over time converge to the max size because we
    	// are reusing vectors from older threads.
		dataStack.reserve(16);
		callStack.reserve(4);
	}
	memset(&luaArgs[0], 0, MAX_LUA_COB_ARGS * sizeof(luaArgs[0]));
}

CCobThread::~CCobThread()
{
	RECOIL_DETAILED_TRACY_ZONE;
	Stop();

	if (dataStack.capacity() > 0) {
		dataStack.clear();
		freeDataStacks.emplace_back(std::move(dataStack));
		callStack.clear();
		freeCallStacks.emplace_back(std::move(callStack));
	}
}

CCobThread& CCobThread::operator = (CCobThread&& t) {
	id = t.id;
	pc = t.pc;

	wakeTime = t.wakeTime;
	paramCount = t.paramCount;
	retCode = t.retCode;
	cbParam = t.cbParam;
	signalMask = t.signalMask;

	waitAxis = t.waitAxis;
	waitPiece = t.waitPiece;

	std::memcpy(luaArgs, t.luaArgs, sizeof(luaArgs));

	callStack = std::move(t.callStack);
	dataStack = std::move(t.dataStack);
	// execTrace = std::move(t.execTrace);

	state = t.state;
	cbType = t.cbType;

	cobInst = t.cobInst; t.cobInst = nullptr;
	cobFile = t.cobFile; t.cobFile = nullptr;
	return *this;
}

CCobThread& CCobThread::operator = (const CCobThread& t) {
	id = t.id;
	pc = t.pc;

	wakeTime = t.wakeTime;
	paramCount = t.paramCount;
	retCode = t.retCode;
	cbParam = t.cbParam;
	signalMask = t.signalMask;

	waitAxis = t.waitAxis;
	waitPiece = t.waitPiece;

	std::memcpy(luaArgs, t.luaArgs, sizeof(luaArgs));

	callStack = t.callStack;
	dataStack = t.dataStack;
	// execTrace = t.execTrace;

	state = t.state;
	cbType = t.cbType;

	cobInst = t.cobInst;
	cobFile = t.cobFile;
	return *this;
}


void CCobThread::Start(int functionId, int sigMask, const std::array<int, 1 + MAX_COB_ARGS>& args, bool schedule)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(callStack.size() == 0);

	state = Run;
	pc = cobFile->scriptOffsets[functionId];

	paramCount = args[0];
	signalMask = sigMask;

	CallInfo& ci = PushCallStackRef();
	ci.functionId = functionId;
	ci.returnAddr = -1;
	ci.stackTop   = 0;

	// copy arguments; args[0] holds the count
	// handled by InitStack if thread has a parent that STARTs it,
	// in which case args[0] is 0 and stack already contains data
	if (paramCount > 0) {
		dataStack.resize(paramCount);
		dataStack.assign(args.begin() + 1, args.begin() + 1 + paramCount);
	}

	// add to scheduler
	if (schedule)
		cobEngine->ScheduleThread(this);
}

void CCobThread::Stop()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (cobInst == nullptr)
		return;

	if (cbType != CCobInstance::CBNone)
		cobInst->ThreadCallback(cbType, retCode, cbParam);

	cobInst->RemoveThreadID(id);
	SetState(Dead);

	cobInst = nullptr;
	cobFile = nullptr;
}


const std::string& CCobThread::GetName()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return cobFile->scriptNames[callStack[0].functionId];
}


int CCobThread::CheckStack(unsigned int size, bool warn)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (size <= dataStack.size())
		return size;

	if (warn) {
		char msg[512];
		const char* fmt =
			"stack-size mismatch: need %u but have %d arguments "
			"(too many passed to function or too few returned?)";

		SNPRINTF(msg, sizeof(msg), fmt, size, dataStack.size());
		ShowError(msg);
	}

	return dataStack.size();
}

void CCobThread::InitStack(unsigned int n, CCobThread* t)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(dataStack.size() == 0);

	// move n arguments from caller's stack onto our own
	for (unsigned int i = 0; i < n; ++i) {
		PushDataStack(t->PopDataStack());
	}
}

#if 0
#define GET_LONG_PC() (cobFile->code[pc++])
#else
// mantis #5981
#define GET_LONG_PC() (cobFile->code.at(pc++))
#endif

bool CCobThread::Tick()
{
	assert(state != Sleep);
	assert(cobInst != nullptr);

	if (IsDead())
		return false;

	ZoneScoped;

	state = Run;

	int r1, r2, r3, r4, r5, r6;

	while (state == Run) {
		const int opcode = GET_LONG_PC();

		switch (opcode) {
			case PUSH_CONSTANT: {
				r1 = GET_LONG_PC();
				PushDataStack(r1);
			} break;
			case SLEEP: {
				r1 = PopDataStack();
				wakeTime = cobEngine->GetCurrTime() + r1;
				state = Sleep;

				cobEngine->ScheduleThread(this);
				return true;
			} break;
			case SPIN: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack();         // speed
				r4 = PopDataStack();         // accel
				cobInst->Spin(r1, r2, r3, r4);
			} break;
			case STOP_SPIN: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack();         // decel

				cobInst->StopSpin(r1, r2, r3);
			} break;
			case RETURN: {
				retCode = PopDataStack();

				if (LocalReturnAddr() == -1) {
					state = Dead;

					// leave values intact on stack in case caller wants to check them
					// callStackSize -= 1;
					return false;
				}

				// return to caller
				pc = LocalReturnAddr();
				if (dataStack.size() > LocalStackFrame())
					dataStack.resize(LocalStackFrame());

				callStack.pop_back();
			} break;


			case SHADE: {
				r1 = GET_LONG_PC();
			} break;
			case DONT_SHADE: {
				r1 = GET_LONG_PC();
			} break;
			case CACHE: {
				r1 = GET_LONG_PC();
			} break;
			case DONT_CACHE: {
				r1 = GET_LONG_PC();
			} break;

			case SIGNATURE_LUA: {
				LOG_L(L_ERROR, "BAD ACCESS: Entered a lua method reference.");
				state = Dead;
				return false;
			} break;

			case BATCH_LUA: {
				DeferredCall(false);
			} break;

			case CALL: {
				r1 = GET_LONG_PC();
				pc--;

				if (cobFile->scriptNames[r1].find("lua_") == 0) {
					cobFile->code[pc - 1] = LUA_CALL;
					LuaCall();
					break;
				}

				cobFile->code[pc - 1] = REAL_CALL;

			} [[fallthrough]];
			case REAL_CALL: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				// do not call zero-length functions
				if (cobFile->scriptLengths[r1] == 0)
					break;

				CallInfo& ci = PushCallStackRef();
				ci.functionId = r1;
				ci.returnAddr = pc;
				ci.stackTop = dataStack.size() - r2;

				paramCount = r2;

				// call cobFile->scriptNames[r1]
				pc = cobFile->scriptOffsets[r1];
			} break;
			case LUA_CALL: {
				LuaCall();
			} break;


			case POP_STATIC: {
				r1 = GET_LONG_PC();
				r2 = PopDataStack();

				if (static_cast<size_t>(r1) < cobInst->staticVars.size())
					cobInst->staticVars[r1] = r2;
			} break;
			case POP_STACK: {
				PopDataStack();
			} break;


			case START: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (cobFile->scriptLengths[r1] == 0)
					break;


				CCobThread t(cobInst);

				t.SetID(cobEngine->AllocateThreadID());
				t.InitStack(r2, this);
				t.Start(r1, signalMask, {{0}}, true);

				// calling AddThread directly might move <this>, defer it
				cobEngine->QueueAddThread(std::move(t));
			} break;

			case CREATE_LOCAL_VAR: {
				if (paramCount == 0) {
					PushDataStack(0);
				} else {
					paramCount--;
				}
			} break;
			case GET_UNIT_VALUE: {
				r1 = PopDataStack();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					PushDataStack(luaArgs[r1 - LUA0]);
					break;
				}
				r1 = cobInst->GetUnitVal(r1, 0, 0, 0, 0);
				PushDataStack(r1);
			} break;


			case JUMP_NOT_EQUAL: {
				r1 = GET_LONG_PC();
				r2 = PopDataStack();

				if (r2 == 0)
					pc = r1;

			} break;
			case JUMP: {
				r1 = GET_LONG_PC();
				// this seem to be an error in the docs..
				//r2 = cobFile->scriptOffsets[LocalFunctionID()] + r1;
				pc = r1;
			} break;


			case POP_LOCAL_VAR: {
				r1 = GET_LONG_PC();
				r2 = PopDataStack();
				dataStack[LocalStackFrame() + r1] = r2;
			} break;
			case PUSH_LOCAL_VAR: {
				r1 = GET_LONG_PC();
				r2 = dataStack[LocalStackFrame() + r1];
				PushDataStack(r2);
			} break;


			case BITWISE_AND: {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 & r2);
			} break;
			case BITWISE_OR: {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 | r2);
			} break;
			case BITWISE_XOR: {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 ^ r2);
			} break;
			case BITWISE_NOT: {
				r1 = PopDataStack();
				PushDataStack(~r1);
			} break;

			case EXPLODE: {
				r1 = GET_LONG_PC();
				r2 = PopDataStack();
				cobInst->Explode(r1, r2);
			} break;

			case PLAY_SOUND: {
				r1 = GET_LONG_PC();
				r2 = PopDataStack();
				cobInst->PlayUnitSound(r1, r2);
			} break;

			case PUSH_STATIC: {
				r1 = GET_LONG_PC();

				if (static_cast<size_t>(r1) < cobInst->staticVars.size())
					PushDataStack(cobInst->staticVars[r1]);
			} break;

			case SET_NOT_EQUAL: {
				r1 = PopDataStack();
				r2 = PopDataStack();

				PushDataStack(int(r1 != r2));
			} break;
			case SET_EQUAL: {
				r1 = PopDataStack();
				r2 = PopDataStack();

				PushDataStack(int(r1 == r2));
			} break;

			case SET_LESS: {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 < r2));
			} break;
			case SET_LESS_OR_EQUAL: {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 <= r2));
			} break;

			case SET_GREATER: {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 > r2));
			} break;
			case SET_GREATER_OR_EQUAL: {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 >= r2));
			} break;

			case RAND: {
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = gsRNG.NextInt(r2 - r1 + 1) + r1;
				PushDataStack(r3);
			} break;
			case EMIT_SFX: {
				r1 = PopDataStack();
				r2 = GET_LONG_PC();
				cobInst->EmitSfx(r1, r2);
			} break;
			case MUL: {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 * r2);
			} break;


			case SIGNAL: {
				r1 = PopDataStack();
				cobInst->Signal(r1);
			} break;
			case SET_SIGNAL_MASK: {
				r1 = PopDataStack();
				signalMask = r1;
			} break;


			case TURN: {
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = GET_LONG_PC(); // piece
				r4 = GET_LONG_PC(); // axis

				cobInst->Turn(r3, r4, r1, r2);
			} break;
			case GET: {
				r5 = PopDataStack();
				r4 = PopDataStack();
				r3 = PopDataStack();
				r2 = PopDataStack();
				r1 = PopDataStack();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					PushDataStack(luaArgs[r1 - LUA0]);
					break;
				}
				r6 = cobInst->GetUnitVal(r1, r2, r3, r4, r5);
				PushDataStack(r6);
			} break;
			case ADD: {
				r2 = PopDataStack();
				r1 = PopDataStack();
				PushDataStack(r1 + r2);
			} break;
			case SUB: {
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = r1 - r2;
				PushDataStack(r3);
			} break;

			case DIV: {
				r2 = PopDataStack();
				r1 = PopDataStack();

				if (r2 != 0) {
					r3 = r1 / r2;
				} else {
					r3 = 1000; // infinity!
					ShowError("division by zero");
				}
				PushDataStack(r3);
			} break;
			case MOD: {
				r2 = PopDataStack();
				r1 = PopDataStack();

				if (r2 != 0) {
					PushDataStack(r1 % r2);
				} else {
					PushDataStack(0);
					ShowError("modulo division by zero");
				}
			} break;


			case MOVE: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r4 = PopDataStack();
				r3 = PopDataStack();
				cobInst->Move(r1, r2, r3, r4);
			} break;
			case MOVE_NOW: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack();
				cobInst->MoveNow(r1, r2, r3);
			} break;
			case TURN_NOW: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack();
				cobInst->TurnNow(r1, r2, r3);
			} break;
			case SCALE: {
				r1 = GET_LONG_PC();
				r3 = PopDataStack();
				r2 = PopDataStack();
				cobInst->Scale(r1, r2, r3);
			} break;
			case SCALE_NOW: {
				r1 = GET_LONG_PC();
				r2 = PopDataStack();
				cobInst->ScaleNow(r1, r2);
			} break;

			case WAIT_TURN: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (cobInst->NeedsWait(CCobInstance::ATurn, r1, r2)) {
					state = WaitTurn;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			} break;
			case WAIT_MOVE: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (cobInst->NeedsWait(CCobInstance::AMove, r1, r2)) {
					state = WaitMove;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			} break;
			case WAIT_SCALE: {
				r1 = GET_LONG_PC();

				if (cobInst->NeedsWait(CCobInstance::AScale, r1, -1)) {
					state = WaitScale;
					waitPiece = r1;
					waitAxis = -1;
					return true;
				}
			} break;

			case SET: {
				r2 = PopDataStack();
				r1 = PopDataStack();

				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					luaArgs[r1 - LUA0] = r2;
					break;
				}

				cobInst->SetUnitVal(r1, r2);
			} break;


			case ATTACH: {
				r3 = PopDataStack();
				r2 = PopDataStack();
				r1 = PopDataStack();
				cobInst->AttachUnit(r2, r1);
			} break;
			case DROP: {
				r1 = PopDataStack();
				cobInst->DropUnit(r1);
			} break;

			// like bitwise ops, but only on values 1 and 0
			case LOGICAL_NOT: {
				r1 = PopDataStack();
				PushDataStack(int(r1 == 0));
			} break;
			case LOGICAL_AND: {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int(r1 && r2));
			} break;
			case LOGICAL_OR: {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int(r1 || r2));
			} break;
			case LOGICAL_XOR: {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int((!!r1) ^ (!!r2)));
			} break;


			case HIDE: {
				r1 = GET_LONG_PC();
				cobInst->SetVisibility(r1, false);
			} break;

			case SHOW: {
				r1 = GET_LONG_PC();

				int i;
				for (i = 0; i < MAX_WEAPONS_PER_UNIT; ++i)
					if (LocalFunctionID() == cobFile->scriptIndex[COBFN_FirePrimary + COBFN_Weapon_Funcs * i])
						break;

				// if true, we are in a Fire-script and should show a special flare effect
				if (i < MAX_WEAPONS_PER_UNIT) {
					cobInst->ShowFlare(r1);
				} else {
					cobInst->SetVisibility(r1, true);
				}
			} break;

			default: {
				const char* name = cobFile->name.c_str();
				const char* func = cobFile->scriptNames[LocalFunctionID()].c_str();

				LOG_L(L_ERROR, "[COBThread::%s] unknown opcode %x (in %s:%s at %x)", __func__, opcode, name, func, pc - 1);

				#if 0
				auto ei = execTrace.begin();
				while (ei != execTrace.end()) {
					LOG_L(L_ERROR, "\tprogctr: %3x  opcode: %s", __func__, *ei, GetOpcodeName(cobFile->code[*ei]));
					++ei;
				}
				#endif

				state = Dead;
				return false;
			} break;
		}
	}

	// can arrive here as dead, through CCobInstance::Signal()
	return (state != Dead);
}

void CCobThread::ShowError(const char* msg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if ((errorCounter = std::max(errorCounter - 1, 0)) == 0)
		return;

	if (callStack.size() == 0) {
		LOG_L(L_ERROR, "[COBThread::%s] %s outside script execution (?)", __func__, msg);
		return;
	}

	const char* name = cobFile->name.c_str();
	const char* func = cobFile->scriptNames[LocalFunctionID()].c_str();

	LOG_L(L_ERROR, "[COBThread::%s] %s (in %s:%s at %x)", __func__, msg, name, func, pc - 1);
}


void CCobThread::DeferredCall(bool synced)
{
	const int r1 = GET_LONG_PC(); // script id
	const int r2 = GET_LONG_PC(); // arg count

	// Make sure to clean args from stack on exit
	CCobStackGuard guard{&dataStack, r2};

	// sanity checks
	if (!luaRules) {
		retCode = 0;
		return;
	}

	// check script index validity
	if (static_cast<size_t>(r1) >= cobFile->luaScripts.size()) {
		retCode = 0;
		return;
	}

	// setup the parameter array
	auto d = CCobDeferredCallin(cobInst->GetUnit(), cobFile->luaScripts[r1], dataStack, r2);

	cobEngine->AddDeferredCallin(std::move(d));

	// always succeeds
	retCode = 1;
}


void CCobThread::LuaCall()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int r1 = GET_LONG_PC(); // script id
	const int r2 = GET_LONG_PC(); // arg count

	// Make sure to clean args from stack on exit
	CCobStackGuard guard{&dataStack, r2};

	// setup the parameter array
	const int size = static_cast<int>(dataStack.size());
	const int argCount = std::min(r2, MAX_LUA_COB_ARGS);
	const int start = std::max(0, size - r2);
	const int end = std::min(size, start + argCount);

	for (int a = 0, i = start; i < end; i++) {
		luaArgs[a++] = dataStack[i];
	}

	if (!luaRules) {
		luaArgs[0] = 0; // failure
		return;
	}

	// check script index validity
	if (static_cast<size_t>(r1) >= cobFile->luaScripts.size()) {
		luaArgs[0] = 0; // failure
		return;
	}

	int argsCount = argCount;
	luaRules->syncedLuaHandle.Cob2Lua(cobFile->luaScripts[r1], cobInst->GetUnit(), argsCount, luaArgs);
	retCode = luaArgs[0];
}

void CCobThread::AnimFinished(CUnitScript::AnimType type, int piece, int axis)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (piece != waitPiece || axis != waitAxis)
		return;

	if (!Reschedule(type))
		return;

	state = Run;
	waitPiece = -1;
	waitAxis = -1;

	cobEngine->ScheduleThread(this);
}

