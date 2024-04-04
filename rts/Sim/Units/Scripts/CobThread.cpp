/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CobThread.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "CobEngine.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

#include <tracy/Tracy.hpp>

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
	CR_MEMBER(dataStack),
	CR_MEMBER(cobVersion)
))

CR_BIND(CCobThread::CallInfo,)

CR_REG_METADATA_SUB(CCobThread, CallInfo,(
	CR_MEMBER(functionId),
	CR_MEMBER(returnAddr),
	CR_MEMBER(stackTop)
))

std::vector<decltype(CCobThread::dataStack)> CCobThread::freeDataStacks;
std::vector<decltype(CCobThread::callStack)> CCobThread::freeCallStacks;

CCobThread::CCobThread(CCobInstance *_cobInst)
	: cobInst(_cobInst)
	, cobFile(_cobInst->cobFile)
{
       // If there are any free data and call stacks available, reuse them by
       // moving them to the current thread's data and call stack variables to
       // amortize memory allocations.
    if (!freeDataStacks.empty())
    {
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
	cobVersion = cobInst->cobVersion;
	//const char *name = cobFile->name.c_str();
	//LOG_L(L_ERROR, "[COBThread::%s] CCobThread::CCobThread(CCobInstance *_cobInst) %d", name, cobVersion);
}

CCobThread::~CCobThread()
{
	Stop();

	if (dataStack.capacity() > 0)
	{
		dataStack.clear();
		freeDataStacks.emplace_back(std::move(dataStack));
		callStack.clear();
		freeCallStacks.emplace_back(std::move(callStack));
	}
}

CCobThread &CCobThread::operator=(CCobThread &&t)
{
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
	//if (cobInst != nullptr)
	//	cobVersion = cobInst->cobVersion;
	//else{
		cobVersion = t.cobVersion;
		//LOG_L(L_ERROR, "[COBThread::] CCobThread::operator=(CCobThread &&t) cobInst nullptr!");
	//}
	cobInst = t.cobInst;
	t.cobInst = nullptr;
	cobFile = t.cobFile;
	t.cobFile = nullptr;

	//const char *name = cobFile->name.c_str();
	//LOG_L(L_ERROR, "[COBThread::] CCobThread::operator=(CCobThread &&t) %d", cobVersion);
	return *this;
}

CCobThread &CCobThread::operator=(const CCobThread &t)
{
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

	//if (cobInst != nullptr)
		//cobVersion = cobInst->cobVersion;
	//else{
		cobVersion = t.cobVersion;
		//LOG_L(L_ERROR, "[COBThread::] CCobThread::operator=(const CCobThread &t) cobInst nullptr!");
	//}


	//const char *name = cobFile->name.c_str();
	//LOG_L(L_ERROR, "[COBThread::] CCobThread::operator=(const CCobThread &t) %d", cobVersion);
	return *this;
}

void CCobThread::Start(int functionId, int sigMask, const std::array<int, 1 + MAX_COB_ARGS> &args, bool schedule)
{
	assert(callStack.size() == 0);

	state = Run;
	pc = cobFile->scriptOffsets[functionId];

	paramCount = args[0];
	signalMask = sigMask;

	CallInfo &ci = PushCallStackRef();
	ci.functionId = functionId;
	ci.returnAddr = -1;
	ci.stackTop = 0;

	// copy arguments; args[0] holds the count
	// handled by InitStack if thread has a parent that STARTs it,
	// in which case args[0] is 0 and stack already contains data
	if (paramCount > 0)
	{
		dataStack.resize(paramCount);
		dataStack.assign(args.begin() + 1, args.begin() + 1 + paramCount);
	}

	// add to scheduler
	if (schedule)
		cobEngine->ScheduleThread(this);
}

void CCobThread::Stop()
{
	if (cobInst == nullptr)
		return;

	if (cbType != CCobInstance::CBNone)
		cobInst->ThreadCallback(cbType, retCode, cbParam);

	cobInst->RemoveThreadID(id);
	SetState(Dead);

	cobInst = nullptr;
	cobFile = nullptr;
}

const std::string &CCobThread::GetName()
{
	return cobFile->scriptNames[callStack[0].functionId];
}

int CCobThread::CheckStack(unsigned int size, bool warn)
{
	if (size <= dataStack.size())
		return size;

	if (warn)
	{
		char msg[512];
		const char *fmt =
			"stack-size mismatch: need %u but have %d arguments "
			"(too many passed to function or too few returned?)";

		SNPRINTF(msg, sizeof(msg), fmt, size, dataStack.size());
		ShowError(msg);
	}

	return dataStack.size();
}

void CCobThread::InitStack(unsigned int n, CCobThread *t)
{
	assert(dataStack.size() == 0);

	// move n arguments from caller's stack onto our own
	for (unsigned int i = 0; i < n; ++i)
	{
		PushDataStack(t->PopDataStack());
	}
}

inline int CCobThread::PopDataStack()
{
	if (dataStack.empty())
	{
		const char *name = cobFile->name.c_str();
		const char *func = cobFile->scriptNames[LocalFunctionID()].c_str();
		LOG_L(L_ERROR, "[COBThread::%s] empty data stack (in %s at %x)", name, func, pc - 1);
		return 0;
	}
	int ret = dataStack.back();
	dataStack.pop_back();
	return ret;
}

// Command documentation from http://visualta.tauniverse.com/Downloads/cob-commands.txt
// And some information from basm0.8 source (basm ops.txt)
//
#define SHORTOPCODES

// Model interaction
static constexpr int MOVE = 0x10001000;
static constexpr int TURN = 0x10002000;
static constexpr int SPIN = 0x10003000;
static constexpr int STOP_SPIN = 0x10004000;
static constexpr int SHOW = 0x10005000;
static constexpr int HIDE = 0x10006000;
static constexpr int CACHE = 0x10007000;
static constexpr int DONT_CACHE = 0x10008000;
static constexpr int MOVE_NOW = 0x1000B000;
static constexpr int TURN_NOW = 0x1000C000;
static constexpr int SHADE = 0x1000D000;
static constexpr int DONT_SHADE = 0x1000E000;
static constexpr int EMIT_SFX = 0x1000F000;

// Blocking operations
static constexpr int WAIT_TURN = 0x10011000;
static constexpr int WAIT_MOVE = 0x10012000;
static constexpr int SLEEP = 0x10013000;

// Stack manipulation
static constexpr int PUSH_CONSTANT = 0x10021001;
static constexpr int PUSH_LOCAL_VAR = 0x10021002;
static constexpr int PUSH_STATIC = 0x10021004;
static constexpr int CREATE_LOCAL_VAR = 0x10022000;
static constexpr int POP_LOCAL_VAR = 0x10023002;
static constexpr int POP_STATIC = 0x10023004;
static constexpr int POP_STACK = 0x10024000; ///< Not sure what this is supposed to do

// Arithmetic operations
static constexpr int ADD = 0x10031000;
static constexpr int SUB = 0x10032000;
static constexpr int MUL = 0x10033000;
static constexpr int DIV = 0x10034000;
static constexpr int MOD = 0x10034001; ///< spring specific
static constexpr int BITWISE_AND = 0x10035000;
static constexpr int BITWISE_OR = 0x10036000;
static constexpr int BITWISE_XOR = 0x10037000;
static constexpr int BITWISE_NOT = 0x10038000;

// Native function calls
static constexpr int RAND = 0x10041000;
static constexpr int GET_UNIT_VALUE = 0x10042000;
static constexpr int GET = 0x10043000;

// Comparison
static constexpr int SET_LESS = 0x10051000;
static constexpr int SET_LESS_OR_EQUAL = 0x10052000;
static constexpr int SET_GREATER = 0x10053000;
static constexpr int SET_GREATER_OR_EQUAL = 0x10054000;
static constexpr int SET_EQUAL = 0x10055000;
static constexpr int SET_NOT_EQUAL = 0x10056000;
static constexpr int LOGICAL_AND = 0x10057000;
static constexpr int LOGICAL_OR = 0x10058000;
static constexpr int LOGICAL_XOR = 0x10059000;
static constexpr int LOGICAL_NOT = 0x1005A000;

// Flow control
static constexpr int START = 0x10061000;
static constexpr int CALL = 0x10062000;		 ///< converted when executed
static constexpr int REAL_CALL = 0x10062001; ///< spring custom
static constexpr int LUA_CALL = 0x10062002;	 ///< spring custom
static constexpr int JUMP = 0x10064000;
static constexpr int RETURN = 0x10065000;
static constexpr int JUMP_NOT_EQUAL = 0x10066000;
static constexpr int SIGNAL = 0x10067000;
static constexpr int SET_SIGNAL_MASK = 0x10068000;

// Piece destruction
static constexpr int EXPLODE = 0x10071000;
static constexpr int PLAY_SOUND = 0x10072000;

// Special functions
static constexpr int SET = 0x10082000;
static constexpr int ATTACH = 0x10083000;
static constexpr int DROP = 0x10084000;

static constexpr uint8_t RAS_BADOPCODE = 0x00;

static constexpr uint8_t RAS_MOVE = 0x01;
static constexpr uint8_t RAS_TURN = 0x02;
static constexpr uint8_t RAS_SPIN = 0x03;
static constexpr uint8_t RAS_STOP_SPIN = 0x04;
static constexpr uint8_t RAS_SHOW = 0x05;
static constexpr uint8_t RAS_HIDE = 0x06;
static constexpr uint8_t RAS_CACHE = 0x07; // TODO REMOVE
static constexpr uint8_t RAS_DONT_CACHE = 0x08;
static constexpr uint8_t RAS_MOVE_NOW = 0x0B;
static constexpr uint8_t RAS_TURN_NOW = 0x0C;
static constexpr uint8_t RAS_SHADE = 0x0D;
static constexpr uint8_t RAS_DONT_SHADE = 0x0E;
static constexpr uint8_t RAS_EMIT_SFX = 0x0F;

// Blocking operations
static constexpr uint8_t RAS_WAIT_TURN = 0x11;
static constexpr uint8_t RAS_WAIT_MOVE = 0x12;
static constexpr uint8_t RAS_SLEEP = 0x13;

// Stack manipulation
static constexpr uint8_t RAS_PUSH_CONSTANT = 0x21;
static constexpr uint8_t RAS_PUSH_LOCAL_VAR = 0x22;
static constexpr uint8_t RAS_PUSH_STATIC = 0x23;
static constexpr uint8_t RAS_CREATE_LOCAL_VAR = 0x24;
static constexpr uint8_t RAS_POP_LOCAL_VAR = 0x25;
static constexpr uint8_t RAS_POP_STATIC = 0x26;
static constexpr uint8_t RAS_POP_STACK = 0x27;

// Arithmetic operations
static constexpr uint8_t RAS_MOD = 0x30;
static constexpr uint8_t RAS_ADD = 0x31;
static constexpr uint8_t RAS_SUB = 0x32;
static constexpr uint8_t RAS_MUL = 0x33;
static constexpr uint8_t RAS_DIV = 0x34;
static constexpr uint8_t RAS_BITWISE_AND = 0x35;
static constexpr uint8_t RAS_BITWISE_OR = 0x36;
static constexpr uint8_t RAS_BITWISE_XOR = 0x37;
static constexpr uint8_t RAS_BITWISE_NOT = 0x38;

static constexpr uint8_t RAS_ABSOLUTE = 0x39;
static constexpr uint8_t RAS_MINIMUM = 0x3A;
static constexpr uint8_t RAS_MAXIMUM = 0x3B;
static constexpr uint8_t RAS_SIGN = 0x3C;
static constexpr uint8_t RAS_CLAMP = 0x3D;
static constexpr uint8_t RAS_DELTAHEADING = 0x3E;
static constexpr uint8_t RAS_MSINE = 0x3F;
static constexpr uint8_t RAS_MCOSINE = 0x40;

// Native function calls
static constexpr uint8_t RAS_RAND = 0x41;
static constexpr uint8_t RAS_GET_UNIT_VALUE = 0x42;
static constexpr uint8_t RAS_GET = 0x43;

// Comparison
static constexpr uint8_t RAS_SET_LESS = 0x51;
static constexpr uint8_t RAS_SET_LESS_OR_EQUAL = 0x52;
static constexpr uint8_t RAS_SET_GREATER = 0x53;
static constexpr uint8_t RAS_SET_GREATER_OR_EQUAL = 0x54;
static constexpr uint8_t RAS_SET_EQUAL = 0x55;
static constexpr uint8_t RAS_SET_NOT_EQUAL = 0x56;
static constexpr uint8_t RAS_LOGICAL_AND = 0x57;
static constexpr uint8_t RAS_LOGICAL_OR = 0x58;
static constexpr uint8_t RAS_LOGICAL_XOR = 0x59;
static constexpr uint8_t RAS_LOGICAL_NOT = 0x5A;

// Flow control
static constexpr uint8_t RAS_START = 0x61;
static constexpr uint8_t RAS_CALL = 0x62;
static constexpr uint8_t RAS_REAL_CALL = 0x63;
static constexpr uint8_t RAS_JUMP = 0x64;
static constexpr uint8_t RAS_RETURN = 0x65;
static constexpr uint8_t RAS_JUMP_NOT_EQUAL = 0x66;
static constexpr uint8_t RAS_SIGNAL = 0x67;
static constexpr uint8_t RAS_SET_SIGNAL_MASK = 0x68;
static constexpr uint8_t RAS_LUA_CALL = 0x69;

// Piece destruction
static constexpr uint8_t RAS_EXPLODE = 0x71;
static constexpr uint8_t RAS_PLAY_SOUND = 0x72;

// Special functions
static constexpr uint8_t RAS_SET = 0x82;
static constexpr uint8_t RAS_ATTACH = 0x83;
static constexpr uint8_t RAS_DROP = 0x84;


// Indices for SET, GET, and GET_UNIT_VALUE for LUA return values
static constexpr int LUA0 = 110; // (LUA0 returns the lua call status, 0 or 1)
static constexpr int LUA1 = 111;
static constexpr int LUA2 = 112;
static constexpr int LUA3 = 113;
static constexpr int LUA4 = 114;
static constexpr int LUA5 = 115;
static constexpr int LUA6 = 116;
static constexpr int LUA7 = 117;
static constexpr int LUA8 = 118;
static constexpr int LUA9 = 119;

#if 0
#define GET_LONG_PC() (cobFile->code[pc++])
#else
// mantis #5981
#define GET_LONG_PC() (cobFile->code.at(pc++))
#endif

#if 1
static const char* GetOpcodeName(int opcode)
{
	switch (opcode) {
		case MOVE: return "move";
		case TURN: return "turn";
		case SPIN: return "spin";
		case STOP_SPIN: return "stop-spin";
		case SHOW: return "show";
		case HIDE: return "hide";
		case CACHE: return "cache";
		case DONT_CACHE: return "dont-cache";
		case TURN_NOW: return "turn-now";
		case MOVE_NOW: return "move-now";
		case SHADE: return "shade";
		case DONT_SHADE: return "dont-shade";
		case EMIT_SFX: return "sfx";

		case WAIT_TURN: return "wait-for-turn";
		case WAIT_MOVE: return "wait-for-move";
		case SLEEP: return "sleep";

		case PUSH_CONSTANT: return "pushc";
		case PUSH_LOCAL_VAR: return "pushl";
		case PUSH_STATIC: return "pushs";
		case CREATE_LOCAL_VAR: return "clv";
		case POP_LOCAL_VAR: return "popl";
		case POP_STATIC: return "pops";
		case POP_STACK: return "pop-stack";

		case ADD: return "add";
		case SUB: return "sub";
		case MUL: return "mul";
		case DIV: return "div";
		case MOD: return "mod";
		case BITWISE_AND: return "and";
		case BITWISE_OR: return "or";
		case BITWISE_XOR: return "xor";
		case BITWISE_NOT: return "not";

		case RAND: return "rand";
		case GET_UNIT_VALUE: return "getuv";
		case GET: return "get";

		case SET_LESS: return "setl";
		case SET_LESS_OR_EQUAL: return "setle";
		case SET_GREATER: return "setg";
		case SET_GREATER_OR_EQUAL: return "setge";
		case SET_EQUAL: return "sete";
		case SET_NOT_EQUAL: return "setne";
		case LOGICAL_AND: return "land";
		case LOGICAL_OR: return "lor";
		case LOGICAL_XOR: return "lxor";
		case LOGICAL_NOT: return "neg";

		case START: return "start";
		case CALL: return "call";
		case REAL_CALL: return "call";
		case LUA_CALL: return "lua_call";
		case JUMP: return "jmp";
		case RETURN: return "return";
		case JUMP_NOT_EQUAL: return "jne";
		case SIGNAL: return "signal";
		case SET_SIGNAL_MASK: return "mask";

		case EXPLODE: return "explode";
		case PLAY_SOUND: return "play-sound";

		case SET: return "set";
		case ATTACH: return "attach";
		case DROP: return "drop";

		case RAS_MOVE: return "move";
		case RAS_TURN: return "turn";
		case RAS_SPIN: return "spin";
		case RAS_STOP_SPIN: return "stop-spin";
		case RAS_SHOW: return "show";
		case RAS_HIDE: return "hide";
		case RAS_CACHE: return "cache";
		case RAS_DONT_CACHE: return "dont-cache";
		case RAS_TURN_NOW: return "turn-now";
		case RAS_MOVE_NOW: return "move-now";
		case RAS_SHADE: return "shade";
		case RAS_DONT_SHADE: return "dont-shade";
		case RAS_EMIT_SFX: return "sfx";

		case RAS_WAIT_TURN: return "wait-for-turn";
		case RAS_WAIT_MOVE: return "wait-for-move";
		case RAS_SLEEP: return "sleep";

		case RAS_PUSH_CONSTANT: return "pushc";
		case RAS_PUSH_LOCAL_VAR: return "pushl";
		case RAS_PUSH_STATIC: return "pushs";
		case RAS_CREATE_LOCAL_VAR: return "clv";
		case RAS_POP_LOCAL_VAR: return "popl";
		case RAS_POP_STATIC: return "pops";
		case RAS_POP_STACK: return "pop-stack";

		case RAS_ADD: return "add";
		case RAS_SUB: return "sub";
		case RAS_MUL: return "mul";
		case RAS_DIV: return "div";
		case RAS_MOD: return "mod";
		case RAS_BITWISE_AND: return "and";
		case RAS_BITWISE_OR: return "or";
		case RAS_BITWISE_XOR: return "xor";
		case RAS_BITWISE_NOT: return "not";

		case RAS_RAND: return "rand";
		case RAS_GET_UNIT_VALUE: return "getuv";
		case RAS_GET: return "get";

		case RAS_SET_LESS: return "setl";
		case RAS_SET_LESS_OR_EQUAL: return "setle";
		case RAS_SET_GREATER: return "setg";
		case RAS_SET_GREATER_OR_EQUAL: return "setge";
		case RAS_SET_EQUAL: return "sete";
		case RAS_SET_NOT_EQUAL: return "setne";
		case RAS_LOGICAL_AND: return "land";
		case RAS_LOGICAL_OR: return "lor";
		case RAS_LOGICAL_XOR: return "lxor";
		case RAS_LOGICAL_NOT: return "neg";

		case RAS_START: return "start";
		case RAS_CALL: return "call";
		case RAS_REAL_CALL: return "call";
		case RAS_LUA_CALL: return "lua_call";
		case RAS_JUMP: return "jmp";
		case RAS_RETURN: return "return";
		case RAS_JUMP_NOT_EQUAL: return "jne";
		case RAS_SIGNAL: return "signal";
		case RAS_SET_SIGNAL_MASK: return "mask";

		case RAS_EXPLODE: return "explode";
		case RAS_PLAY_SOUND: return "play-sound";

		case RAS_SET: return "set";
		case RAS_ATTACH: return "attach";
		case RAS_DROP: return "drop";
	}

	return "unknown";
}
#endif
/*
int interp_cgoto(unsigned char* code, int initval) {
	// The indices of labels in the dispatch_table are the relevant opcodes

	static void* dispatch_table[] = {
		&&do_halt, &&do_inc, &&do_dec, &&do_mul2,
		&&do_div2, &&do_add7, &&do_neg };
	#define DISPATCH() goto *dispatch_table[code[pc++]]

	int pc = 0;
	int val = initval;

	DISPATCH();
	while (1) {
	do_halt:
		return val;
	do_inc:
		val++;
		DISPATCH();
	do_dec:
		val--;
		DISPATCH();
	do_mul2:
		val *= 2;
		DISPATCH();
	do_div2:
		val /= 2;
		DISPATCH();
	do_add7:
		val += 7;
		DISPATCH();
	do_neg:
		val = -val;
		DISPATCH();
	}
}*/

// Use computed goto for gcc

#ifdef _MSC_VER

#else

#endif

bool CCobThread::Tick()
{
	assert(state != Sleep);
	assert(cobInst != nullptr);

	if (IsDead())
		return false;

	ZoneScoped;
	// ZoneScopedN(GetName().c_str());

	state = Run;

	const char *name = cobFile->name.c_str();
	//LOG_L(L_ERROR, "[COBThread::%s] Tick %d", name, cobVersion);
	if (cobVersion < 8 )
	//if ( 1 == 1)
	{

		int r1, r2, r3, r4, r5, r6;
		while (true)
		{

			const int opcode = GET_LONG_PC();

			switch (opcode)
			{
			case PUSH_CONSTANT:
			{
				r1 = GET_LONG_PC();
				PushDataStack(r1);
			}
			break;
			case SLEEP:
			{
				r1 = PopDataStack();
				wakeTime = cobEngine->GetCurrTime() + r1;
				state = Sleep;

				cobEngine->ScheduleThread(this);
				return true;
			}
			break;
			case SPIN:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack(); // speed
				r4 = PopDataStack(); // accel
				cobInst->Spin(r1, r2, r3, r4);
			}
			break;
			case STOP_SPIN:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack(); // decel

				cobInst->StopSpin(r1, r2, r3);
			}
			break;
			case RETURN:
			{
				retCode = PopDataStack();

				if (LocalReturnAddr() == -1)
				{
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
			}
			break;

			case SHADE:
			{
				r1 = GET_LONG_PC();
			}
			break;
			case DONT_SHADE:
			{
				r1 = GET_LONG_PC();
			}
			break;
			case CACHE:
			{
				r1 = GET_LONG_PC();
			}
			break;
			case DONT_CACHE:
			{
				r1 = GET_LONG_PC();
			}
			break;

			case CALL:
			{
				r1 = GET_LONG_PC();
				pc--;

				if (cobFile->scriptNames[r1].find("lua_") == 0)
				{
					cobFile->code[pc - 1] = LUA_CALL;
					LuaCall();
					break;
				}

				cobFile->code[pc - 1] = REAL_CALL;

				// fall-through
			}
			case REAL_CALL:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				// do not call zero-length functions
				if (cobFile->scriptLengths[r1] == 0)
					break;

				CallInfo &ci = PushCallStackRef();
				ci.functionId = r1;
				ci.returnAddr = pc;
				ci.stackTop = dataStack.size() - r2;

				paramCount = r2;

				// call cobFile->scriptNames[r1]
				pc = cobFile->scriptOffsets[r1];
			}
			break;
			case LUA_CALL:
			{
				LuaCall();
			}
			break;

			case POP_STATIC:
			{
				r1 = GET_LONG_PC();
				r2 = PopDataStack();

				if (static_cast<size_t>(r1) < cobInst->staticVars.size())
					cobInst->staticVars[r1] = r2;
			}
			break;
			case POP_STACK:
			{
				PopDataStack();
			}
			break;

			case START:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (cobFile->scriptLengths[r1] == 0)
					break;

				CCobThread t(cobInst);

				t.SetID(cobEngine->GenThreadID());
				t.InitStack(r2, this);
				t.Start(r1, signalMask, {{0}}, true);

				// calling AddThread directly might move <this>, defer it
				cobEngine->QueueAddThread(std::move(t));
			}
			break;

			case CREATE_LOCAL_VAR:
			{
				if (paramCount == 0)
				{
					PushDataStack(0);
				}
				else
				{
					paramCount--;
				}
			}
			break;
			case GET_UNIT_VALUE:
			{
				r1 = PopDataStack();
				if ((r1 >= LUA0) && (r1 <= LUA9))
				{
					PushDataStack(luaArgs[r1 - LUA0]);
					break;
				}
				r1 = cobInst->GetUnitVal(r1, 0, 0, 0, 0);
				PushDataStack(r1);
			}
			break;

			case JUMP_NOT_EQUAL:
			{
				r1 = GET_LONG_PC();
				r2 = PopDataStack();

				if (r2 == 0)
					pc = r1;
			}
			break;
			case JUMP:
			{
				r1 = GET_LONG_PC();
				// this seem to be an error in the docs..
				// r2 = cobFile->scriptOffsets[LocalFunctionID()] + r1;
				pc = r1;
			}
			break;

			case POP_LOCAL_VAR:
			{
				r1 = GET_LONG_PC();
				r2 = PopDataStack();
				dataStack[LocalStackFrame() + r1] = r2;
			}
			break;
			case PUSH_LOCAL_VAR:
			{
				r1 = GET_LONG_PC();
				r2 = dataStack[LocalStackFrame() + r1];
				PushDataStack(r2);
			}
			break;

			case BITWISE_AND:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 & r2);
			}
			break;
			case BITWISE_OR:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 | r2);
			}
			break;
			case BITWISE_XOR:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 ^ r2);
			}
			break;
			case BITWISE_NOT:
			{
				r1 = PopDataStack();
				PushDataStack(~r1);
			}
			break;

			case EXPLODE:
			{
				r1 = GET_LONG_PC();
				r2 = PopDataStack();
				cobInst->Explode(r1, r2);
			}
			break;

			case PLAY_SOUND:
			{
				r1 = GET_LONG_PC();
				r2 = PopDataStack();
				cobInst->PlayUnitSound(r1, r2);
			}
			break;

			case PUSH_STATIC:
			{
				r1 = GET_LONG_PC();

				if (static_cast<size_t>(r1) < cobInst->staticVars.size())
					PushDataStack(cobInst->staticVars[r1]);
			}
			break;

			case SET_NOT_EQUAL:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();

				PushDataStack(int(r1 != r2));
			}
			break;
			case SET_EQUAL:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();

				PushDataStack(int(r1 == r2));
			}
			break;

			case SET_LESS:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 < r2));
			}
			break;
			case SET_LESS_OR_EQUAL:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 <= r2));
			}
			break;

			case SET_GREATER:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 > r2));
			}
			break;
			case SET_GREATER_OR_EQUAL:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 >= r2));
			}
			break;

			case RAND:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = gsRNG.NextInt(r2 - r1 + 1) + r1;
				PushDataStack(r3);
			}
			break;
			case EMIT_SFX:
			{
				r1 = PopDataStack();
				r2 = GET_LONG_PC();
				cobInst->EmitSfx(r1, r2);
			}
			break;
			case MUL:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 * r2);
			}
			break;

			case SIGNAL:
			{
				r1 = PopDataStack();
				cobInst->Signal(r1);
			}
			break;
			case SET_SIGNAL_MASK:
			{
				r1 = PopDataStack();
				signalMask = r1;
			}
			break;

			case TURN:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = GET_LONG_PC(); // piece
				r4 = GET_LONG_PC(); // axis

				cobInst->Turn(r3, r4, r1, r2);
			}
			break;
			case GET:
			{
				r5 = PopDataStack();
				r4 = PopDataStack();
				r3 = PopDataStack();
				r2 = PopDataStack();
				r1 = PopDataStack();
				if ((r1 >= LUA0) && (r1 <= LUA9))
				{
					PushDataStack(luaArgs[r1 - LUA0]);
					break;
				}
				r6 = cobInst->GetUnitVal(r1, r2, r3, r4, r5);
				PushDataStack(r6);
			}
			break;
			case ADD:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();
				PushDataStack(r1 + r2);
			}
			break;
			case SUB:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = r1 - r2;
				PushDataStack(r3);
			}
			break;

			case DIV:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();

				if (r2 != 0)
				{
					r3 = r1 / r2;
				}
				else
				{
					r3 = 1000; // infinity!
					ShowError("division by zero");
				}
				PushDataStack(r3);
			}
			break;
			case MOD:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();

				if (r2 != 0)
				{
					PushDataStack(r1 % r2);
				}
				else
				{
					PushDataStack(0);
					ShowError("modulo division by zero");
				}
			}
			break;

			case MOVE:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r4 = PopDataStack();
				r3 = PopDataStack();
				cobInst->Move(r1, r2, r3, r4);
			}
			break;
			case MOVE_NOW:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack();
				cobInst->MoveNow(r1, r2, r3);
			}
			break;
			case TURN_NOW:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = PopDataStack();
				cobInst->TurnNow(r1, r2, r3);
			}
			break;

			case WAIT_TURN:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (cobInst->NeedsWait(CCobInstance::ATurn, r1, r2))
				{
					state = WaitTurn;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			}
			break;
			case WAIT_MOVE:
			{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (cobInst->NeedsWait(CCobInstance::AMove, r1, r2))
				{
					state = WaitMove;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			}
			break;

			case SET:
			{
				r2 = PopDataStack();
				r1 = PopDataStack();

				if ((r1 >= LUA0) && (r1 <= LUA9))
				{
					luaArgs[r1 - LUA0] = r2;
					break;
				}

				cobInst->SetUnitVal(r1, r2);
			}
			break;

			case ATTACH:
			{
				r3 = PopDataStack();
				r2 = PopDataStack();
				r1 = PopDataStack();
				cobInst->AttachUnit(r2, r1);
			}
			break;
			case DROP:
			{
				r1 = PopDataStack();
				cobInst->DropUnit(r1);
			}
			break;

				// like bitwise ops, but only on values 1 and 0
			case LOGICAL_NOT:
			{
				r1 = PopDataStack();
				PushDataStack(int(r1 == 0));
			}
			break;
			case LOGICAL_AND:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int(r1 && r2));
			}
			break;
			case LOGICAL_OR:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int(r1 || r2));
			}
			break;
			case LOGICAL_XOR:
			{
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int((!!r1) ^ (!!r2)));
			}
			break;

			case HIDE:
			{
				r1 = GET_LONG_PC();
				cobInst->SetVisibility(r1, false);
			}
			break;

			case SHOW:
			{
				r1 = GET_LONG_PC();

				int i;
				for (i = 0; i < MAX_WEAPONS_PER_UNIT; ++i)
					if (LocalFunctionID() == cobFile->scriptIndex[COBFN_FirePrimary + COBFN_Weapon_Funcs * i])
						break;

				// if true, we are in a Fire-script and should show a special flare effect
				if (i < MAX_WEAPONS_PER_UNIT)
				{
					cobInst->ShowFlare(r1);
				}
				else
				{
					cobInst->SetVisibility(r1, true);
				}
			}
			break;

			default:
			{
				const char *name = cobFile->name.c_str();
				const char *func = cobFile->scriptNames[LocalFunctionID()].c_str();

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
			}
			break;
			}
		}
	}
	else // RAS (Recoil Animation Script), with a version signature of 8
	{
		/*
		int interp_cgoto(unsigned char* code, int initval) {
			// The indices of labels in the dispatch_table are the relevant opcodes

			static void* dispatch_table[] = {
				&&do_halt, &&do_inc, &&do_dec, &&do_mul2,
				&&do_div2, &&do_add7, &&do_neg };
			#define DISPATCH() goto *dispatch_table[code[pc++]]

			int pc = 0;
			int val = initval;

			DISPATCH();
			while (1) {
			do_halt:
				return val;
			do_inc:
				val++;
				DISPATCH();
			do_dec:
				val--;
				DISPATCH();
			do_mul2:
				val *= 2;
				DISPATCH();
			do_div2:
				val /= 2;
				DISPATCH();
			do_add7:
				val += 7;
				DISPATCH();
			do_neg:
				val = -val;
				DISPATCH();
			}
		}*/
#ifndef _MSC_VER
		static void *ras_dispatch_table[] = {
			// 0x00
			&&DO_RAS_BADOPCODE, &&DO_RAS_MOVE, &&DO_RAS_TURN, &&DO_RAS_SPIN,
			&&DO_RAS_STOP_SPIN, &&DO_RAS_SHOW, &&DO_RAS_HIDE, &&DO_RAS_BADOPCODE,
			// 0x08
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_MOVE_NOW,
			&&DO_RAS_TURN_NOW, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_EMIT_SFX,
			// 0x10
			&&DO_RAS_BADOPCODE, &&DO_RAS_WAIT_TURN, &&DO_RAS_WAIT_MOVE, &&DO_RAS_SLEEP,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x18
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x20
			&&DO_RAS_BADOPCODE, &&DO_RAS_PUSH_CONSTANT, &&DO_RAS_PUSH_LOCAL_VAR, &&DO_RAS_PUSH_STATIC,
			&&DO_RAS_CREATE_LOCAL_VAR, &&DO_RAS_POP_LOCAL_VAR, &&DO_RAS_POP_STATIC, &&DO_RAS_POP_STACK,
			// 0x28
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x30
			&&DO_RAS_MOD, &&DO_RAS_ADD, &&DO_RAS_SUB, &&DO_RAS_MUL, 
			&&DO_RAS_DIV, &&DO_RAS_BITWISE_AND, &&DO_RAS_BITWISE_OR, &&DO_RAS_BITWISE_XOR,
			// 0x38
			&&DO_RAS_BITWISE_NOT, &&DO_RAS_ABSOLUTE, &&DO_RAS_MINIMUM, &&DO_RAS_MAXIMUM,
			&&DO_RAS_SIGN, &&DO_RAS_CLAMP, &&DO_RAS_DELTAHEADING, &&DO_RAS_MSINE,
			// 0x40
			&&DO_RAS_MCOSINE, &&DO_RAS_RAND, &&DO_RAS_GET_UNIT_VALUE, &&DO_RAS_GET,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x48
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x50
			&&DO_RAS_BADOPCODE, &&DO_RAS_SET_LESS, &&DO_RAS_SET_LESS_OR_EQUAL, &&DO_RAS_SET_GREATER,
			&&DO_RAS_SET_GREATER_OR_EQUAL, &&DO_RAS_SET_EQUAL, &&DO_RAS_SET_NOT_EQUAL, &&DO_RAS_LOGICAL_AND,
			// 0x58
			&&DO_RAS_LOGICAL_OR, &&DO_RAS_LOGICAL_XOR, &&DO_RAS_LOGICAL_NOT, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x60
			&&DO_RAS_BADOPCODE, &&DO_RAS_START, &&DO_RAS_CALL, &&DO_RAS_REAL_CALL,
			&&DO_RAS_JUMP, &&DO_RAS_RETURN, &&DO_RAS_JUMP_NOT_EQUAL, &&DO_RAS_SIGNAL,
			// 0x68
			&&DO_RAS_SET_SIGNAL_MASK, &&DO_RAS_LUA_CALL, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x70
			&&DO_RAS_BADOPCODE, &&DO_RAS_EXPLODE, &&DO_RAS_PLAY_SOUND, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x78
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x80
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_SET, &&DO_RAS_ATTACH,
			&&DO_RAS_DROP, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x88
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x90
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0x98
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0xA0
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0xB0
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0xC0
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0xD0
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0xE0
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			// 0xF0
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE,
			&&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE, &&DO_RAS_BADOPCODE
			};
#define RAS_DISPATCH() \
	opcode = GET_LONG_PC(); \
	/*LOG_L(L_ERROR, "[COBThread] opcode %s %x (in %s:%s at %x) r1=%d r2=%d",GetOpcodeName(opcode),  opcode, name, func, pc - 1, r1, r2);*/ \
	goto *ras_dispatch_table[opcode] ;
#endif

//	LOG_L(L_ERROR, "[COBThread::%s] unknown opcode %x (in %s:%s at %x)", __func__, opcode, name, func, pc - 1);


#ifdef _MSC_VER
#define BREAK_OR_RAS_DISPATCH \
	}                         \
	break;
// we have to use the RAS_ naming here cause otherwise it wont pick up whitespace and merges with case
#define CASE_OR_RAS_LABEL(name) \
	case RAS_##name:            \
	{
#else
#define BREAK_OR_RAS_DISPATCH RAS_DISPATCH();
// we have to use the RAS_ naming here cause otherwise it wont pick up whitespace and merges with for MSC
#define CASE_OR_RAS_LABEL(name) DO_RAS_##name:
#endif

		const char *name = cobFile->name.c_str();
		const char *func = cobFile->scriptNames[LocalFunctionID()].c_str();

		//LOG_L(L_ERROR, "[COBThread::%s] RUNNING AS RAS(in %s) Cobversion = %d", name, func, cobVersion);

		int r1, r2, r3, r4, r5, r6;

		while (true)
		{
			uint8_t opcode;
#ifdef _MSC_VER
			const int longopcode = GET_LONG_PC();
			opcode = (uint8_t)longopcode; // TODO : const

			switch (opcode)
			{
#else
				// We start with a dispatch operation
				RAS_DISPATCH();
#endif

				CASE_OR_RAS_LABEL(PUSH_CONSTANT)

					r1 = GET_LONG_PC();
					PushDataStack(r1);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SLEEP)
					r1 = PopDataStack();
					wakeTime = cobEngine->GetCurrTime() + r1;
					state = Sleep;

					cobEngine->ScheduleThread(this);
					return true;
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SPIN)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();
					r3 = PopDataStack(); // speed
					r4 = PopDataStack(); // accel
					cobInst->Spin(r1, r2, r3, r4);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(STOP_SPIN)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();
					r3 = PopDataStack(); // decel

					cobInst->StopSpin(r1, r2, r3);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(RETURN)
					retCode = PopDataStack();

					if (LocalReturnAddr() == -1)
					{
						state = Dead;

						// leave values intact on stack in case RAS_caller wants to check them
						// callStackSize -= 1;
						return false;
					}

					// return to caller
					pc = LocalReturnAddr();
					if (dataStack.size() > LocalStackFrame())
						dataStack.resize(LocalStackFrame());

					callStack.pop_back();
				BREAK_OR_RAS_DISPATCH

				/*
				CASE_OR_RAS_LABEL(SHADE)
					r1 = GET_LONG_PC();
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(DONT_SHADE)
					r1 = GET_LONG_PC();
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(CACHE)
					r1 = GET_LONG_PC();
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(DONT_CACHE)
					r1 = GET_LONG_PC();
				BREAK_OR_RAS_DISPATCH
				*/

				CASE_OR_RAS_LABEL(CALL)		
					//LOG_L(L_ERROR, "[COBThread::CALL] opcode %s %x (in %s:%s at %x) r1=%d r2=%d",GetOpcodeName(opcode),  opcode, name, func, pc - 1, r1, r2);\

					r1 = GET_LONG_PC();
					pc--;

					if (cobFile->scriptNames[r1].find("lua_") == 0)
					{
						cobFile->code[pc - 1] = RAS_LUA_CALL;
						LuaCall();
#ifdef _MSC_VER
						break;
#else
					RAS_DISPATCH();
#endif
					}

					cobFile->code[pc - 1] = RAS_REAL_CALL;

					// fall-through
					// NOTE NO BREAK HERE!
	#ifndef _MSC_VER
					//goto DO_RAS_REAL_CALL;
	#else
				}
	#endif
				CASE_OR_RAS_LABEL(REAL_CALL)
					//LOG_L(L_ERROR, "[COBThread::REAL_CALL] opcode %s %x (in %s:%s at %x) r1=%d r2=%d",GetOpcodeName(opcode),  opcode, name, func, pc - 1, r1, r2);\

					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();

					// do not call zero-length functions
					// TODO: NOTE THIS BREAK!
					if (cobFile->scriptLengths[r1] == 0)
					{
	#ifdef _MSC_VER
						break;
	#else
					RAS_DISPATCH();
	#endif
					}
					CallInfo &ci = PushCallStackRef();
					ci.functionId = r1;
					ci.returnAddr = pc;
					ci.stackTop = dataStack.size() - r2;

					paramCount = r2;

					// call cobFile->scriptNames[r1]
					pc = cobFile->scriptOffsets[r1];		
					//LOG_L(L_ERROR, "[COBThread::REAL_CALL End] opcode %s %x (in %s:%s at %x) r1=%d r2=%d",GetOpcodeName(opcode),  opcode, name, func, pc - 1, r1, r2);\

				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(LUA_CALL)
					LuaCall();
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(POP_STATIC)
					r1 = GET_LONG_PC();
					r2 = PopDataStack();

					if (static_cast<size_t>(r1) < cobInst->staticVars.size())
						cobInst->staticVars[r1] = r2;
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(POP_STACK)
					PopDataStack();
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(START)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();

					if (cobFile->scriptLengths[r1] == 0)
					{
#ifdef _MSC_VER
						break;
#else
						RAS_DISPATCH();
#endif
					}
					//LOG_L(L_ERROR, "[COBThread::START] opcode %s %x (in %s:%s at %x) r1=%d r2=%d",GetOpcodeName(opcode),  opcode, name, func, pc - 1, r1, r2);\

					CCobThread t(cobInst);

					t.SetID(cobEngine->GenThreadID());
					t.InitStack(r2, this);
					t.Start(r1, signalMask, {{0}}, true);
					//LOG_L(L_ERROR, "[COBThread::POSTSTART] opcode %s %x (in %s:%s at %x) r1=%d r2=%d",GetOpcodeName(opcode),  opcode, name, func, pc - 1, r1, r2);\

					// calling AddThread directly might move <this>, defer it
					cobEngine->QueueAddThread(std::move(t));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(CREATE_LOCAL_VAR)
					if (paramCount == 0)
					{
						PushDataStack(0);
					}
					else
					{
						paramCount--;
					}
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(GET_UNIT_VALUE)
					r1 = PopDataStack();
					if ((r1 >= LUA0) && (r1 <= LUA9))
					{
						PushDataStack(luaArgs[r1 - LUA0]);
#ifdef _MSC_VER
						break;
#else
						RAS_DISPATCH();
#endif
					}
					r1 = cobInst->GetUnitVal(r1, 0, 0, 0, 0);
					PushDataStack(r1);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(JUMP_NOT_EQUAL)
					r1 = GET_LONG_PC();
					r2 = PopDataStack();

					if (r2 == 0)
						pc = r1;

				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(JUMP)
					r1 = GET_LONG_PC();
					// this seem to be an error in the docs..
					// r2 = cobFile->scriptOffsets[LocalFunctionID()] + r1;
					pc = r1;
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(POP_LOCAL_VAR)
					r1 = GET_LONG_PC();
					r2 = PopDataStack();
					dataStack[LocalStackFrame() + r1] = r2;
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(PUSH_LOCAL_VAR)
					r1 = GET_LONG_PC();
					r2 = dataStack[LocalStackFrame() + r1];
					PushDataStack(r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(BITWISE_AND)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(r1 & r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(BITWISE_OR)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(r1 | r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(BITWISE_XOR)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(r1 ^ r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(BITWISE_NOT)
					r1 = PopDataStack();
					PushDataStack(~r1);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(ABSOLUTE)
					r1 = PopDataStack();
					PushDataStack(abs(r1));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MINIMUM)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(std::min(r1, r2));

				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MAXIMUM)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(std::max(r1, r2));

				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SIGN)
					r1 = PopDataStack();
					PushDataStack(((0 < r1) - (r1 < 0)));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(CLAMP)
					r1 = PopDataStack();
					r2 = PopDataStack();
					r3 = PopDataStack();
					PushDataStack(std::min(std::max(r1, r2), r3));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(DELTAHEADING)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack((r1 - r2 + COBSCALE_HALF * 3) % (COBSCALE_HALF * 2) - COBSCALE_HALF);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MSINE)
					r1 = PopDataStack();
					PushDataStack(int(1024 * 1024 * math::sinf(TAANG2RAD * (float)r1)));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MCOSINE)
					r1 = PopDataStack();
					PushDataStack(int(1024 * 1024 * math::sinf(TAANG2RAD * (float)r1)));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(EXPLODE)
					r1 = GET_LONG_PC();
					r2 = PopDataStack();
					cobInst->Explode(r1, r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(PLAY_SOUND)
					r1 = GET_LONG_PC();
					r2 = PopDataStack();
					cobInst->PlayUnitSound(r1, r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(PUSH_STATIC)
					r1 = GET_LONG_PC();
					if (static_cast<size_t>(r1) < cobInst->staticVars.size())
						PushDataStack(cobInst->staticVars[r1]);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET_NOT_EQUAL)
					r1 = PopDataStack();
					r2 = PopDataStack();

					PushDataStack(int(r1 != r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET_EQUAL)
					r1 = PopDataStack();
					r2 = PopDataStack();

					PushDataStack(int(r1 == r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET_LESS)
					r2 = PopDataStack();
					r1 = PopDataStack();

					PushDataStack(int(r1 < r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET_LESS_OR_EQUAL)
					r2 = PopDataStack();
					r1 = PopDataStack();

					PushDataStack(int(r1 <= r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET_GREATER)
					r2 = PopDataStack();
					r1 = PopDataStack();

					PushDataStack(int(r1 > r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET_GREATER_OR_EQUAL)
					r2 = PopDataStack();
					r1 = PopDataStack();

					PushDataStack(int(r1 >= r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(RAND)
					r2 = PopDataStack();
					r1 = PopDataStack();
					r3 = gsRNG.NextInt(r2 - r1 + 1) + r1;
					PushDataStack(r3);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(EMIT_SFX)
					r1 = PopDataStack();
					r2 = GET_LONG_PC();
					cobInst->EmitSfx(r1, r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MUL)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(r1 * r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SIGNAL)
					r1 = PopDataStack();
					cobInst->Signal(r1);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET_SIGNAL_MASK)
					r1 = PopDataStack();
					signalMask = r1;
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(TURN)
					r2 = PopDataStack();
					r1 = PopDataStack();
					r3 = GET_LONG_PC(); // piece
					r4 = GET_LONG_PC(); // axis

					cobInst->Turn(r3, r4, r1, r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(GET)
					r5 = PopDataStack();
					r4 = PopDataStack();
					r3 = PopDataStack();
					r2 = PopDataStack();
					r1 = PopDataStack();
					if ((r1 >= LUA0) && (r1 <= LUA9))
					{
						PushDataStack(luaArgs[r1 - LUA0]);
#ifdef _MSC_VER
						break;
#else
						RAS_DISPATCH();
#endif
					}
					r6 = cobInst->GetUnitVal(r1, r2, r3, r4, r5);
					PushDataStack(r6);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(ADD)
					r2 = PopDataStack();
					r1 = PopDataStack();
					PushDataStack(r1 + r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SUB)
					r2 = PopDataStack();
					r1 = PopDataStack();
					r3 = r1 - r2;
					PushDataStack(r3);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(DIV)
					r2 = PopDataStack();
					r1 = PopDataStack();

					if (r2 != 0)
					{
						r3 = r1 / r2;
					}
					else
					{
						r3 = 1000; // infinity!
						ShowError("division by zero");
					}
					PushDataStack(r3);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MOD)
					r2 = PopDataStack();
					r1 = PopDataStack();

					if (r2 != 0)
					{
						PushDataStack(r1 % r2);
					}
					else
					{
						PushDataStack(0);
						ShowError("modulo division by zero");
					}
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MOVE)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();
					r4 = PopDataStack();
					r3 = PopDataStack();
					cobInst->Move(r1, r2, r3, r4);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(MOVE_NOW)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();
					r3 = PopDataStack();
					cobInst->MoveNow(r1, r2, r3);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(TURN_NOW)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();
					r3 = PopDataStack();
					cobInst->TurnNow(r1, r2, r3);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(WAIT_TURN)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();

					if (cobInst->NeedsWait(CCobInstance::ATurn, r1, r2))
					{
						state = WaitTurn;
						waitPiece = r1;
						waitAxis = r2;
						return true;
					}
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(WAIT_MOVE)
					r1 = GET_LONG_PC();
					r2 = GET_LONG_PC();

					if (cobInst->NeedsWait(CCobInstance::AMove, r1, r2))
					{
						state = WaitMove;
						waitPiece = r1;
						waitAxis = r2;
						return true;
					}
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SET)
					r2 = PopDataStack();
					r1 = PopDataStack();

					if ((r1 >= LUA0) && (r1 <= LUA9))
					{
						luaArgs[r1 - LUA0] = r2;
						// This could just be BREAK_OR_RAS_DISPATCH ?
#ifdef _MSC_VER
						break;
#else
						RAS_DISPATCH();
#endif
					}

					cobInst->SetUnitVal(r1, r2);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(ATTACH)
					r3 = PopDataStack();
					r2 = PopDataStack();
					r1 = PopDataStack();
					cobInst->AttachUnit(r2, r1);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(DROP)
					r1 = PopDataStack();
					cobInst->DropUnit(r1);
				BREAK_OR_RAS_DISPATCH

				// like bitwise ops, but only on values 1 and 0
				CASE_OR_RAS_LABEL(LOGICAL_NOT)
					r1 = PopDataStack();
					PushDataStack(int(r1 == 0));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(LOGICAL_AND)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(int(r1 && r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(LOGICAL_OR)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(int(r1 || r2));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(LOGICAL_XOR)
					r1 = PopDataStack();
					r2 = PopDataStack();
					PushDataStack(int((!!r1) ^ (!!r2)));
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(HIDE)
					r1 = GET_LONG_PC();
					cobInst->SetVisibility(r1, false);
				BREAK_OR_RAS_DISPATCH

				CASE_OR_RAS_LABEL(SHOW)
					r1 = GET_LONG_PC();

					int i;
					for (i = 0; i < MAX_WEAPONS_PER_UNIT; ++i)
						if (LocalFunctionID() == cobFile->scriptIndex[COBFN_FirePrimary + COBFN_Weapon_Funcs * i])
							break;
	

					// if true, we are in a Fire-script and should show a special flare effect
					if (i < MAX_WEAPONS_PER_UNIT)
					{
						cobInst->ShowFlare(r1);
					}
					else
					{
						cobInst->SetVisibility(r1, true);
					}
				BREAK_OR_RAS_DISPATCH

				// Note that Python developers found that switches becomes essentially a computed goto in MSVC if all cases are filled:
				// https://github.com/python/cpython/pull/91718

#ifndef _MSC_VER
				CASE_OR_RAS_LABEL(BADOPCODE)
					const char *name = cobFile->name.c_str();
					const char *func = cobFile->scriptNames[LocalFunctionID()].c_str();

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
#endif

#ifdef _MSC_VER
			case 0x0:
			case 0x9:
			case 0xa:
			case 0x10:
			case 0x14:
			case 0x15:
			case 0x16:
			case 0x17:
			case 0x18:
			case 0x19:
			case 0x1a:
			case 0x1b:
			case 0x1c:
			case 0x1d:
			case 0x1e:
			case 0x1f:
			case 0x20:
			case 0x28:
			case 0x29:
			case 0x2a:
			case 0x2b:
			case 0x2c:
			case 0x2d:
			case 0x2e:
			case 0x2f:

			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
			case 0x48:
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4e:
			case 0x4f:
			case 0x50:
			case 0x5b:
			case 0x5c:
			case 0x5d:
			case 0x5e:
			case 0x5f:
			case 0x60:
			case 0x6a:
			case 0x6b:
			case 0x6c:
			case 0x6d:
			case 0x6e:
			case 0x6f:
			case 0x70:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x76:
			case 0x77:
			case 0x78:
			case 0x79:
			case 0x7a:
			case 0x7b:
			case 0x7c:
			case 0x7d:
			case 0x7e:
			case 0x7f:
			case 0x80:
			case 0x81:
			case 0x85:
			case 0x86:
			case 0x87:
			case 0x88:
			case 0x89:
			case 0x8a:
			case 0x8b:
			case 0x8c:
			case 0x8d:
			case 0x8e:
			case 0x8f:
			case 0x90:
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x96:
			case 0x97:
			case 0x98:
			case 0x99:
			case 0x9a:
			case 0x9b:
			case 0x9c:
			case 0x9d:
			case 0x9e:
			case 0x9f:
			case 0xa0:
			case 0xa1:
			case 0xa2:
			case 0xa3:
			case 0xa4:
			case 0xa5:
			case 0xa6:
			case 0xa7:
			case 0xa8:
			case 0xa9:
			case 0xaa:
			case 0xab:
			case 0xac:
			case 0xad:
			case 0xae:
			case 0xaf:
			case 0xb0:
			case 0xb1:
			case 0xb2:
			case 0xb3:
			case 0xb4:
			case 0xb5:
			case 0xb6:
			case 0xb7:
			case 0xb8:
			case 0xb9:
			case 0xba:
			case 0xbb:
			case 0xbc:
			case 0xbd:
			case 0xbe:
			case 0xbf:
			case 0xc0:
			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc4:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcc:
			case 0xcd:
			case 0xce:
			case 0xcf:
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			case 0xd4:
			case 0xd5:
			case 0xd6:
			case 0xd7:
			case 0xd8:
			case 0xd9:
			case 0xda:
			case 0xdb:
			case 0xdc:
			case 0xdd:
			case 0xde:
			case 0xdf:
			case 0xe0:
			case 0xe1:
			case 0xe2:
			case 0xe3:
			case 0xe4:
			case 0xe5:
			case 0xe6:
			case 0xe7:
			case 0xe8:
			case 0xe9:
			case 0xea:
			case 0xeb:
			case 0xec:
			case 0xed:
			case 0xee:
			case 0xef:
			case 0xf0:
			case 0xf1:
			case 0xf2:
			case 0xf3:
			case 0xf4:
			case 0xf5:
			case 0xf6:
			case 0xf7:
			case 0xf8:
			case 0xf9:
			case 0xfa:
			case 0xfb:
			case 0xfc:
			case 0xfd:
			case 0xfe:
			case 0xff:
			default:
			{
				const char *name = cobFile->name.c_str();
				const char *func = cobFile->scriptNames[LocalFunctionID()].c_str();

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
			}
			break;
			}
#endif
		}
	}

	// can arrive here as dead, through CCobInstance::Signal()
	return (state != Dead);
}

void CCobThread::ShowError(const char *msg)
{
	if ((errorCounter = std::max(errorCounter - 1, 0)) == 0)
		return;

	if (callStack.size() == 0)
	{
		LOG_L(L_ERROR, "[COBThread::%s] %s outside script execution (?)", __func__, msg);
		return;
	}

	const char *name = cobFile->name.c_str();
	const char *func = cobFile->scriptNames[LocalFunctionID()].c_str();

	LOG_L(L_ERROR, "[COBThread::%s] %s (in %s:%s at %x)", __func__, msg, name, func, pc - 1);
}

void CCobThread::LuaCall()
{
	const int r1 = GET_LONG_PC(); // script id
	const int r2 = GET_LONG_PC(); // arg count

	// setup the parameter array
	const int size = static_cast<int>(dataStack.size());
	const int argCount = std::min(r2, MAX_LUA_COB_ARGS);
	const int start = std::max(0, size - r2);
	const int end = std::min(size, start + argCount);

	for (int a = 0, i = start; i < end; i++)
	{
		luaArgs[a++] = dataStack[i];
	}

	if (r2 >= size)
	{
		dataStack.clear();
	}
	else
	{
		dataStack.resize(size - r2);
	}

	if (!luaRules)
	{
		luaArgs[0] = 0; // failure
		return;
	}

	// check script index validity
	if (static_cast<size_t>(r1) >= cobFile->luaScripts.size())
	{
		luaArgs[0] = 0; // failure
		return;
	}

	int argsCount = argCount;
	luaRules->Cob2Lua(cobFile->luaScripts[r1], cobInst->GetUnit(), argsCount, luaArgs);
	retCode = luaArgs[0];
}

void CCobThread::AnimFinished(CUnitScript::AnimType type, int piece, int axis)
{
	if (piece != waitPiece || axis != waitAxis)
		return;

	if (!Reschedule(type))
		return;

	state = Run;
	waitPiece = -1;
	waitAxis = -1;

	cobEngine->ScheduleThread(this);
}
