/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobEngine.h"
#include "CobFile.h"
#include "CobFileHandler.h"
#include "CobInstance.h"
#include "CobThread.h"

#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/BubbleProjectile.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/MuzzleFlame.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Rendering/Env/Particles/Classes/WakeProjectile.h"
#include "Rendering/Env/Particles/Classes/WreckProjectile.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/BeamLaser.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/StringUtil.h"
#include "System/SpringMath.h"
#include "System/Sound/ISoundChannels.h"

#include "System/Misc/TracyDefs.h"

/******************************************************************************/
/******************************************************************************/

CR_BIND_DERIVED(CCobInstance, CUnitScript, )

CR_REG_METADATA(CCobInstance, (
	// loaded from cobFileHandler
	CR_IGNORED(cobFile),

	CR_MEMBER(staticVars),
	CR_MEMBER(threadIDs),

	CR_POSTLOAD(PostLoad),
	CR_PREALLOC(GetUnit)
))


inline bool CCobInstance::HasFunction(int id) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (cobFile->scriptIndex.size() > id && cobFile->scriptIndex[id] >= 0);
}



void CCobInstance::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	InitCommon();

	staticVars.clear();
	staticVars.resize(cobFile->numStaticVars, 0);
}

void CCobInstance::PostLoad()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unit != nullptr);
	assert(cobFile == nullptr);

	cobFile = cobFileHandler->GetCobFile(unit->unitDef->scriptName);

	for (int threadID: threadIDs) {
		CCobThread* t = cobEngine->GetThread(threadID);

		t->cobInst = this;
		t->cobFile = cobFile;
	}

	InitCommon();
}


CCobInstance::~CCobInstance()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// this may be dangerous, is it really desired?
	// Destroy();

	// delete our threads, make sure callbacks do not run
	while (!threadIDs.empty()) {
		CCobThread* t = cobEngine->GetThread(threadIDs.back());

		t->MakeGarbage();
		cobEngine->RemoveThread(t->GetID());

		threadIDs.pop_back();
	}

	cobEngine->SanityCheckThreads(this);
}


void CCobInstance::InitCommon()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(cobFile != nullptr);

	MapScriptToModelPieces(&unit->localModel);

	hasSetSFXOccupy  = HasFunction(COBFN_SetSFXOccupy);
	hasRockUnit      = HasFunction(COBFN_RockUnit);
	hasStartBuilding = HasFunction(COBFN_StartBuilding);
}


void CCobInstance::MapScriptToModelPieces(LocalModel* lmodel)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::vector<std::string>& pieceNames = cobFile->pieceNames; // already in lowercase!
	std::vector<LocalModelPiece>& lmodelPieces = lmodel->pieces;

	pieces.clear();
	pieces.reserve(pieceNames.size());

	// clear the default assumed 1:1 mapping
	for (size_t lmodelPieceNum = 0; lmodelPieceNum < lmodelPieces.size(); lmodelPieceNum++) {
		lmodelPieces[lmodelPieceNum].SetScriptPieceIndex(-1);
	}
	for (size_t scriptPieceNum = 0; scriptPieceNum < pieceNames.size(); scriptPieceNum++) {
		unsigned int lmodelPieceNum;

		// Map this piecename to an index in the script's pieceinfo
		for (lmodelPieceNum = 0; lmodelPieceNum < lmodelPieces.size(); lmodelPieceNum++) {
			if (lmodelPieces[lmodelPieceNum].original->name.compare(pieceNames[scriptPieceNum]) == 0) {
				break;
			}
		}

		// Not found? Try lowercase
		if (lmodelPieceNum == lmodelPieces.size()) {
			for (lmodelPieceNum = 0; lmodelPieceNum < lmodelPieces.size(); lmodelPieceNum++) {
				if (StringToLower(lmodelPieces[lmodelPieceNum].original->name).compare(pieceNames[scriptPieceNum]) == 0) {
					break;
				}
			}
		}

		// Did we find it?
		if (lmodelPieceNum < lmodelPieces.size()) {
			lmodelPieces[lmodelPieceNum].SetScriptPieceIndex(scriptPieceNum);
			pieces.push_back(&lmodelPieces[lmodelPieceNum]);
		} else {
			pieces.push_back(nullptr);

			/* Note, scripts can be reused across multiple unit types,
			 * so the COB script name alone is not sufficient */
			const char* fmtString = "[%s] could not find piece named \"%s\" (referenced by COB script \"%s\" used by unit \"%s\")";
			const char* pieceName = pieceNames[scriptPieceNum].c_str();
			const char* scriptName = cobFile->name.c_str();

			LOG_L(L_WARNING, fmtString, __FUNCTION__, pieceName, scriptName, unit->unitDef->name.c_str());
		}
	}
}


int CCobInstance::GetFunctionId(const std::string& fname) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return cobFile->GetFunctionId(fname);
}


bool CCobInstance::HasBlockShot(int weaponNum) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return HasFunction(COBFN_BlockShot + COBFN_Weapon_Funcs * weaponNum);
}


bool CCobInstance::HasTargetWeight(int weaponNum) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return HasFunction(COBFN_TargetWeight + COBFN_Weapon_Funcs * weaponNum);
}


/******************************************************************************/
/******************************************************************************/


void CCobInstance::Create()
{
	ZoneScoped;

	int maxReloadFrames = 0;
	for (const auto* w: unit->weapons)
		maxReloadFrames = std::max(maxReloadFrames, w->reloadTime);

	const int maxReloadMs = 1000 * maxReloadFrames / GAME_SPEED;

	Call(COBFN_Create);
	Call(COBFN_SetMaxReloadTime, maxReloadMs);
}



void CCobInstance::Killed()
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = int(unit->recentDamage / unit->maxHealth * 100);
	callinArgs[2] = 0;

	Call(COBFN_Killed, callinArgs, CBKilled, 0, nullptr);
}


void CCobInstance::WindChanged(float heading, float speed)
{
	ZoneScoped;
	Call(COBFN_SetSpeed, int(speed * 3000.0f));
	Call(COBFN_SetDirection, short(heading * RAD2TAANG));
}


void CCobInstance::ExtractionRateChanged(float speed)
{
	ZoneScoped;
	Call(COBFN_SetSpeed, int(speed * 500.0f));

	if (!unit->activated)
		return;

	Call(COBFN_Go);
}

void CCobInstance::WorldRockUnit(const float3& rockDir) 
{
	RECOIL_DETAILED_TRACY_ZONE;
	RockUnit(unit->GetObjectSpaceVec(rockDir) * 500.0f);
}

void CCobInstance::RockUnit(const float3& rockDir)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = int(rockDir.z);
	callinArgs[2] = int(rockDir.x);

	Call(COBFN_RockUnit, callinArgs);
}

void CCobInstance::WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage)
{
	RECOIL_DETAILED_TRACY_ZONE;
	HitByWeapon(unit->GetObjectSpaceVec(hitDir) * 500.0f, weaponDefId, inoutDamage);
}

void CCobInstance::HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = int(hitDir.z);
	callinArgs[2] = int(hitDir.x);

	if (HasFunction(COBFN_HitByWeaponId)) {
		const WeaponDef* wd = weaponDefHandler->GetWeaponDefByID(weaponDefId);

		callinArgs[0] = 4;
		callinArgs[3] = ((wd != nullptr)? wd->id : -1);
		callinArgs[4] = int(100 * inoutDamage);
		// weaponHitMod, not an actual arg
		callinArgs[MAX_COB_ARGS] = 1;

		Call(COBFN_HitByWeaponId, callinArgs, CBNone, 0, &callinArgs[MAX_COB_ARGS]);

		inoutDamage *= (callinArgs[MAX_COB_ARGS] * 0.01f);
	} else {
		Call(COBFN_HitByWeapon, callinArgs);
	}
}


void CCobInstance::SetSFXOccupy(int curTerrainType)
{
	ZoneScoped;
	Call(COBFN_SetSFXOccupy, curTerrainType);
}


void CCobInstance::QueryLandingPads(std::vector<int>& outPieces)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 1;
	callinArgs[1] = MAX_COB_ARGS; // default max pad count

	if (HasFunction(COBFN_QueryLandingPadCount)) {
		// query the count
		Call(COBFN_QueryLandingPadCount, callinArgs);

		// setup count as first arg for next query
		callinArgs[1] = callinArgs[0];
		callinArgs[0] = 1;
	}

	const int maxPadCount = std::min(callinArgs[1], int(MAX_COB_ARGS));

	// query the pieces
	Call(COBFN_QueryLandingPad, callinArgs);

	outPieces.clear();
	outPieces.resize(maxPadCount);
	outPieces.assign(callinArgs.begin(), callinArgs.begin() + maxPadCount);
}


void CCobInstance::BeginTransport(const CUnit* unit)
{
	ZoneScoped;
	// COB uses model height to identify units
	Call(COBFN_BeginTransport, int(unit->model->height * 65536));
}


int CCobInstance::QueryTransport(const CUnit* unit)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = 0;
	callinArgs[2] = int(unit->model->height * 65536);

	Call(COBFN_QueryTransport, callinArgs);
	return callinArgs[0];
}


void CCobInstance::TransportPickup(const CUnit* unit)
{
	ZoneScoped;
	// here COB uses unitIDs instead of model height
	Call(COBFN_TransportPickup, unit->id);
}


void CCobInstance::TransportDrop(const CUnit* unit, const float3& pos)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = unit->id;
	callinArgs[2] = PACKXZ(pos.x, pos.z);

	Call(COBFN_TransportDrop, callinArgs);
}


void CCobInstance::StartBuilding(float heading, float pitch)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = short(heading * RAD2TAANG);
	callinArgs[2] = short(  pitch * RAD2TAANG);

	Call(COBFN_StartBuilding, callinArgs);
}


int CCobInstance::QueryNanoPiece()
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_QueryNanoPiece, callinArgs);
	return callinArgs[0];
}


int CCobInstance::QueryBuildInfo()
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_QueryBuildInfo, callinArgs);
	return callinArgs[0];
}


int CCobInstance::QueryWeapon(int weaponNum)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_QueryPrimary + COBFN_Weapon_Funcs * weaponNum, callinArgs);
	return callinArgs[0];
}


void CCobInstance::AimWeapon(int weaponNum, float heading, float pitch)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = short(heading * RAD2TAANG);
	callinArgs[2] = short(  pitch * RAD2TAANG);

	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weaponNum, callinArgs, CBAimWeapon, weaponNum, nullptr);
}


void CCobInstance::AimShieldWeapon(CPlasmaRepulser* weapon)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = 0; // compat with AimWeapon (same script is called)

	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weapon->weaponNum, callinArgs, CBAimShield, weapon->weaponNum, nullptr);
}


int CCobInstance::AimFromWeapon(int weaponNum)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_AimFromPrimary + COBFN_Weapon_Funcs * weaponNum, callinArgs);
	return callinArgs[0];
}


void CCobInstance::Shot(int weaponNum)
{
	ZoneScoped;
	Call(COBFN_Shot + COBFN_Weapon_Funcs * weaponNum, 0); // why the 0 argument?
}


bool CCobInstance::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 3;
	callinArgs[1] = (targetUnit != nullptr)? targetUnit->id : 0;
	callinArgs[2] = 0; // return value; default is to not block the shot
	callinArgs[3] = userTarget;

	Call(COBFN_BlockShot + COBFN_Weapon_Funcs * weaponNum, callinArgs);

	return (callinArgs[1] != 0);
}


float CCobInstance::TargetWeight(int weaponNum, const CUnit* targetUnit)
{
	ZoneScoped;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = (targetUnit != nullptr)? targetUnit->id : 0;
	callinArgs[2] = COBSCALE; // return value; default is 1.0

	Call(COBFN_TargetWeight + COBFN_Weapon_Funcs * weaponNum, callinArgs);

	return (callinArgs[1] * 1.0f / COBSCALE);
}

void CCobInstance::AnimFinished(AnimType type, int piece, int axis)
{
	ZoneScoped;
	for (int threadID: threadIDs) {
		CCobThread* t = cobEngine->GetThread(threadID);
		t->AnimFinished(type, piece, axis);
	}
}


void CCobInstance::Destroy() { ZoneScoped; Call(COBFN_Destroy); }
void CCobInstance::StartMoving(bool reversing) { ZoneScoped; Call(COBFN_StartMoving, reversing); }
void CCobInstance::StopMoving() { ZoneScoped; Call(COBFN_StopMoving); }
void CCobInstance::StartUnload() { ZoneScoped; Call(COBFN_StartUnload); }
void CCobInstance::EndTransport() { ZoneScoped; Call(COBFN_EndTransport); }
void CCobInstance::StartBuilding() { ZoneScoped; Call(COBFN_StartBuilding); }
void CCobInstance::StopBuilding() { ZoneScoped; Call(COBFN_StopBuilding); }
void CCobInstance::Falling() { ZoneScoped; Call(COBFN_Falling); }
void CCobInstance::Landed() { ZoneScoped; Call(COBFN_Landed); }
void CCobInstance::Activate() { ZoneScoped; Call(COBFN_Activate); }
void CCobInstance::Deactivate() { ZoneScoped; Call(COBFN_Deactivate); }
void CCobInstance::MoveRate(int curRate) { ZoneScoped; Call(COBFN_MoveRate0 + curRate); }
void CCobInstance::FireWeapon(int weaponNum) { ZoneScoped; Call(COBFN_FirePrimary + COBFN_Weapon_Funcs * weaponNum); }
void CCobInstance::EndBurst(int weaponNum) { ZoneScoped; Call(COBFN_EndBurst + COBFN_Weapon_Funcs * weaponNum); }


/******************************************************************************/


/**
 * @brief Calls a cob script function
 * @param functionId int cob script function id
 * @param args vector<int> function arguments
 * @param cb ThreadCallbackType Callback action
 * @param cbParam int callback argument
 * @return 0 if the call terminated. If the caller provides a callback and the thread does not terminate,
 *  it will continue to run. Otherwise it will be killed. Returns 1 in this case.
 */
int CCobInstance::RealCall(int functionId, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	int ret = -1;

	if (size_t(functionId) >= cobFile->scriptNames.size()) {
		if (retCode != nullptr)
			*retCode = ret;

		// in case the function does not exist the callback should
		// still be called; -1 is the default CobThread return code
		if (cb != CBNone)
			ThreadCallback(cb, -1, cbParam);

		return ret;
	}

	// LOG_L(L_DEBUG, "Calling %s:%s", cobFile->name.c_str(), cobFile->scriptNames[functionId].c_str());

	// tick the thread locally in case we're recursively running this function and then the threads may reallocate
	CCobThread newThread(this);
	newThread.SetID(cobEngine->GenThreadID());

	// make sure this is run even if the call terminates instantly
	if (cb != CBNone)
		newThread.SetCallback(cb, cbParam);

	newThread.Start(functionId, 0, args, false);

	if ((ret = newThread.Tick()) == 0) {
		// thread died already after one tick
		// NOTE:
		//   ticking can trigger recursion, for example FireWeapon ->
		//   Call(COBFN_FirePrimary + N*) -> EmitSFX -> FireWeapon ->
		//   ...
		//
		//   the StartMoving callin now takes an argument which means
		//   there will be a mismatch between the number of arguments
		//   passed in (1) and the number returned (0) as of 95.0, so
		//   prevent error-spam by checking functionId
		//
		//   args[0] holds the number of input args
		const unsigned int numArgs = args[0];
		const unsigned int retArgs = newThread.CheckStack(numArgs, functionId != cobFile->scriptIndex[COBFN_StartMoving]);

		// retrieve output parameter values from stack
		for (unsigned int i = 0, n = std::min(retArgs, MAX_COB_ARGS); i < n; ++i)
			args[i] = newThread.GetStackVal(i);

		// set erroneous parameters to 0
		for (unsigned int i = std::min(retArgs, MAX_COB_ARGS); i < numArgs; ++i)
			args[i] = 0;

		// dtor runs the callback
		if (retCode != nullptr)
			*retCode = newThread.GetRetCode();
	} else {
		cobEngine->AddThread(std::move(newThread));
	}

	// handle any spawned threads
	cobEngine->ProcessQueuedThreads();
	return ret;
}


/******************************************************************************/


int CCobInstance::Call(const std::string& fname)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs = {{0}};

	return Call(fname, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::Call(const std::string& fname, std::array<int, 1 + MAX_COB_ARGS>& args)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return Call(fname, args, CBNone, 0, nullptr);
}

int CCobInstance::Call(const std::string& fname, int arg1)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 1;
	callinArgs[1] = arg1;

	return Call(fname, callinArgs, CBNone, 0, nullptr);
}



int CCobInstance::Call(const std::string& fname, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//TODO: Check that new behaviour of actually calling cb when the function is not defined is right?
	//      (the callback has always been called [when the function is not defined]
	//       in the id-based Call()s, but never in the string based calls.)
	return RealCall(GetFunctionId(fname), args, cb, cbParam, retCode);
}



int CCobInstance::Call(int id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs = {{0}};

	return Call(id, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, int arg1)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 1;
	callinArgs[1] = arg1;

	return Call(id, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, std::array<int, 1 + MAX_COB_ARGS>& args)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return Call(id, args, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	return RealCall(cobFile->scriptIndex[id], args, cb, cbParam, retCode);
}


void CCobInstance::RawCall(int fn)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::array<int, 1 + MAX_COB_ARGS> callinArgs = {{0}};

	RealCall(fn, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::RawCall(int fn, std::array<int, 1 + MAX_COB_ARGS>& args)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return RealCall(fn, args, CBNone, 0, nullptr);
}


void CCobInstance::ThreadCallback(ThreadCallbackType type, int retCode, int cbParam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (type) {
		// note: this callback is always called, even if Killed does not exist
		// however, retCode is only set if the function has a return statement
		// (otherwise its value is -1 regardless of Killed being present which
		// means *no* wreck will be spawned)
		case CBKilled: {
			unit->KilledScriptFinished(retCode);
		} break;
		case CBAimWeapon: {
			unit->weapons[cbParam]->AimScriptFinished(retCode == 1);
		} break;
		case CBAimShield: {
			static_cast<CPlasmaRepulser*>(unit->weapons[cbParam])->SetEnabled(retCode != 0);
		} break;
		default: {
			assert(false);
		} break;
	}
}

/******************************************************************************/
/******************************************************************************/


void CCobInstance::Signal(int signal)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (int threadID: threadIDs) {
		CCobThread* t = cobEngine->GetThread(threadID);

		if ((signal & t->GetSignalMask()) == 0)
			continue;

		t->SetState(CCobThread::Dead);
		cobEngine->QueueRemoveThread(t->GetID());
	}
}


void CCobInstance::PlayUnitSound(int snr, int attr)
{
	RECOIL_DETAILED_TRACY_ZONE;
	Channels::UnitReply->PlaySample(cobFile->sounds[snr], unit->pos, unit->speed, attr);
}


void CCobInstance::ShowScriptError(const std::string& msg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	cobEngine->ShowScriptError(msg);
}
