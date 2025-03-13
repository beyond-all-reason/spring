/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <fstream>
#include <vector>
#include <list>

#include "fmt/format.h"
#include "fmt/printf.h"

#include "DumpState.h"

#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/GameVersion.h"
#include "Net/GameServer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Scripts/CobEngine.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Map/ReadMap.h"
#include "System/StringUtil.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Log/ILog.h"
#include "System/SpringHash.h"

static bool onlyHash = true;

namespace {
	inline std::string TapFloats(const float v) {
		std::ostringstream str;
		if (onlyHash) {
			str << reinterpret_cast<const uint32_t&>(v) << "\n";
		}
		else {
			str << reinterpret_cast<const uint32_t&>(v) << fmt::format(" <{}>\n", v);
		}

		return str.str();
	}
	inline std::string TapFloats(const float3& v) {
		std::ostringstream str;
		if (onlyHash) {
			str << spring::LiteHash(v) << "\n";
		}
		else {
			str << spring::LiteHash(v) << fmt::format(" <{},{},{}>\n", v.x, v.y, v.z);
		}


		return str.str();
	}
	inline std::string TapFloats(const float4& v) {
		std::ostringstream str;
		if (onlyHash) {
			str << spring::LiteHash(v) << "\n";
		}
		else {
			str << spring::LiteHash(v) << fmt::format(" <{},{},{},{}>\n", v.x, v.y, v.z, v.w);
		}


		return str.str();
	}
	inline std::string TapFloats(const CMatrix44f& v) {
		std::ostringstream str;
		if (onlyHash) {
			str << spring::LiteHash(v) << "\n";
		}
		else {
			str << spring::LiteHash(v) << fmt::format(" <{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}>\n",
				v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
		}

		return str.str();
	}

	inline std::string DumpSolidObjectID(const CSolidObject* so) {
		std::string s = so ? std::to_string(so->id) : "(nullptr)";
		s.append("\n");
		return s;
	}

	inline std::string DumpGameID(const uint8_t* p) {
		return fmt::sprintf(
			"%02x%02x%02x%02x%02x%02x%02x%02x"
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			p[0], p[1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
			p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]
		);
	}
}


void DumpState(int newMinFrameNum, int newMaxFrameNum, int newFramePeriod, std::optional<bool> outputFloats, bool serverRequest)
{
	if (outputFloats.has_value())
		onlyHash = !outputFloats.value();

	static std::fstream file;
	static int gMinFrameNum = -1;
	static int gMaxFrameNum = -1;
	static int gFramePeriod =  1;

	const int oldMinFrameNum = gMinFrameNum;
	const int oldMaxFrameNum = gMaxFrameNum;

	if (!gs->cheatEnabled && !serverRequest)
		return;
	// check if the range is valid
	if (newMaxFrameNum < newMinFrameNum)
		return;

	// adjust the bounds if the new values are valid
	if (newMinFrameNum >= 0) gMinFrameNum = newMinFrameNum;
	if (newMaxFrameNum >= 0) gMaxFrameNum = newMaxFrameNum;
	if (newFramePeriod >= 1) gFramePeriod = newFramePeriod;

	if ((gMinFrameNum != oldMinFrameNum) || (gMaxFrameNum != oldMaxFrameNum)) {
		LOG("[%s] dumping state (from %d to %d step %d)", __func__, gMinFrameNum, gMaxFrameNum, gFramePeriod);
		// bounds changed, open a new file
		if (file.is_open()) {
			file.flush();
			file.close();
		}

		std::string name = (gameServer != nullptr)? "Server": "Client";
		name += "GameState-";
		name += IntToString(guRNG.NextInt());
		name += "-[";
		name += IntToString(gMinFrameNum);
		name += "-";
		name += IntToString(gMaxFrameNum);
		name += "].txt";

		file.open(name.c_str(), std::ios::out);

		if (file.is_open()) {
			file << " mapName: " << gameSetup->mapName << "\n";
			file << " modName: " << gameSetup->modName << "\n";
			file << "minFrame: " << gMinFrameNum << "\n";
			file << "maxFrame: " << gMaxFrameNum << "\n";
			file << "randSeed: " << gsRNG.GetLastSeed() << "\n";
			file << "initSeed: " << gsRNG.GetInitSeed() << "\n";
			file << "  gameID: " << DumpGameID(game->gameID) << "\n";
			file << " syncVer: " << SpringVersion::GetSync() << "\n";
		}

		LOG("[%s] using dump-file \"%s\"", __func__, name.c_str());
	}

	if (file.bad() || !file.is_open())
		return;
	// check if the CURRENT frame lies within the bounds
	if (gs->frameNum < gMinFrameNum)
		return;
	if (gs->frameNum > gMaxFrameNum)
		return;
	if ((gs->frameNum % gFramePeriod) != 0)
		return;

	// we only care about the synced projectile data here
	const std::vector<CUnit*>& activeUnits = unitHandler.GetActiveUnits();
	const auto& activeFeatureIDs = featureHandler.GetActiveFeatureIDs();
	const auto& projectiles = projectileHandler.GetActiveProjectiles(true);

	file << "frame: " << gs->frameNum << ", seed: " << gsRNG.GetLastSeed() << "\n";

	#define DUMP_MATH_CONST
	#define DUMP_MODEL_DATA
	#define DUMP_CS_DATA
	#define DUMP_UNIT_DATA
	#define DUMP_UNIT_PIECE_DATA
	#define DUMP_UNIT_WEAPON_DATA
	#define DUMP_UNIT_COMMANDAI_DATA
	#define DUMP_UNIT_MOVETYPE_DATA
	#define DUMP_UNIT_BUILDER_DATA
	#define DUMP_UNIT_SCRIPT_DATA
	#define DUMP_FEATURE_DATA
	#define DUMP_PROJECTILE_DATA
	#define DUMP_TEAM_DATA
	//#define DUMP_ALLYTEAM_DATA
	#define DUMP_ALLYTEAM_DATA_CHECKSUM
	//#define DUMP_HEIGHTMAP
	#define DUMP_HEIGHTMAP_CHECKSUM
	//#define DUMP_SMOOTHMESH
	#define DUMP_SMOOTHMESH_CHECKSUM

	#ifdef DUMP_MATH_CONST
	if (gs->frameNum == gMinFrameNum) { //dump once
		file << "\tmath constants:\n";
		#define TAP_MATH_CONST(name) file << "\t\t" << #name << ": " << TapFloats(math::name)
		TAP_MATH_CONST(PI);
		TAP_MATH_CONST(INVPI);
		TAP_MATH_CONST(INVPI);
		TAP_MATH_CONST(INVPI2);
		TAP_MATH_CONST(TWOPI);
		TAP_MATH_CONST(THREEPI);
		TAP_MATH_CONST(SQRPI);
		TAP_MATH_CONST(PIU4);
		TAP_MATH_CONST(PISUN4);

		TAP_MATH_CONST(HALFPI);
		TAP_MATH_CONST(QUARTERPI);
		TAP_MATH_CONST(NEGHALFPI);

		TAP_MATH_CONST(SQRT2);
		TAP_MATH_CONST(HALFSQRT2);

		TAP_MATH_CONST(RAD_TO_DEG);
		TAP_MATH_CONST(DEG_TO_RAD);
		#undef TAP_MATH_CONST
	}
	#endif

	#ifdef DUMP_CS_DATA
	if (gs->frameNum == gMinFrameNum) { //dump once
		sha512::hex_digest hexDigest;
		{
			hexDigest = { 0 };
			const auto mapCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->mapName);
			sha512::dump_digest(mapCheckSum, hexDigest);
			file << "\tmapCheckSum: " << std::string(hexDigest.data()) << "\n";
		}
		{
			hexDigest = { 0 };
			const auto modCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->modName);
			sha512::dump_digest(modCheckSum, hexDigest);
			file << "\tmodCheckSum: " << std::string(hexDigest.data()) << "\n";
		}
		/*
		for (const auto& ari : archiveScanner->GetAllArchives()) {
			hexDigest = { 0 };
			const auto cs = archiveScanner->GetArchiveCompleteChecksumBytes(ari.GetNameVersioned());
			sha512::dump_digest(cs, hexDigest);
			file << "\tArchive: " << ari.GetNameVersioned() << " checkSum: " << std::string(hexDigest.data()) << "\n";
		}
		*/
	}
	#endif

	#ifdef DUMP_MODEL_DATA
	if (gs->frameNum == gMinFrameNum) { //dump once
		// models no longer have same order and IDs across different runs due to MT preload.
		// Need to sort them to ease comparison
		std::map<std::string, size_t> sortedModelNames;
		const auto& mv = modelLoader.GetModelsVec();
		for (size_t i = 0; i < mv.size(); ++i) {
			if (mv[i].id <= 0)
				continue;

			sortedModelNames.emplace(mv[i].name, i);
		}

		file << "\tloaded models: " << sortedModelNames.size() << "\n";
		for (const auto& smn : sortedModelNames) {
			const auto& m = mv[smn.second];
			file << "\t\tname: " << m.name << "\n";
			file << "\t\tnumPieces: " << m.numPieces << "\n";
			//file << "\t\ttextureType: " << m.textureType << "\n";
			file << "\t\tmodelType: " << m.type << "\n";
			file << "\t\tradius: " << TapFloats(m.radius);
			file << "\t\theight: " << TapFloats(m.height);
			file << "\t\tmins: " << TapFloats(m.mins);
			file << "\t\tmaxs: " << TapFloats(m.maxs);
			file << "\t\trelMidPos: " << TapFloats(m.relMidPos);
			file << "\t\tpieceObjects: " << m.pieceObjects.size() << "\n";
			for (const auto* p : m.pieceObjects) {
				file << "\t\t\tname: " << p->name << "\n";
				file << "\t\t\tchildrenNum: " << p->children.size() << "\n";
				file << "\t\t\tparentName: " << (p->parent ? p->parent->name : "(NULL)") << "\n";
				file << "\t\t\thasBakedMat: " << p->HasBackedTra() << "\n";
				file << "\t\t\tbposeTransform(t): " << TapFloats(p->bposeTransform.t);
				file << "\t\t\tbposeTransform(r): " << TapFloats(float4{ p->bposeTransform.r.x, p->bposeTransform.r.y, p->bposeTransform.r.z, p->bposeTransform.r.r });
				file << "\t\t\tbposeTransform(s): " << TapFloats(p->bposeTransform.s);
				file << "\t\t\tbakedTransform(t): " << TapFloats(p->bakedTransform.t);
				file << "\t\t\tbakedTransform(r): " << TapFloats(float4{ p->bakedTransform.r.x, p->bakedTransform.r.y, p->bakedTransform.r.z, p->bakedTransform.r.r });
				file << "\t\t\tbakedTransform(s): " << TapFloats(p->bakedTransform.s);
				file << "\t\t\toffset: " << TapFloats(p->offset);
				file << "\t\t\tgoffset: " << TapFloats(p->goffset);
				file << "\t\t\tscales: " << TapFloats(p->scale);
				file << "\t\t\tscales: " << TapFloats(p->mins);
				file << "\t\t\tscales: " << TapFloats(p->maxs);

				// GetVerticesVec and GetIndicesVec are now empty after upload to GPu is complete
#if 0
				file << "\t\t\tvertices.size(): " << p->GetVerticesVec().size() << "\n";
				for (const auto& v : p->GetVerticesVec()) { //is it sync significant?
					file << "\t\t\tpos: " << TapFloats(v.pos);
				}

				file << "\t\t\tindices.size(): " << p->GetIndicesVec().size() << "\n";
				file << "\t\t\t";
				for (const auto& i : p->GetIndicesVec()) { //is it sync significant?
					file << i << ", ";
				}
#endif
				file << "\n";
			}
			file << "\n";
			file << "\n";
		}
	}
	#endif

	file << "\tunits: " << activeUnits.size() << "\n";

	#ifdef DUMP_UNIT_DATA
	for (const CUnit* u: activeUnits) {
		const std::vector<CWeapon*>& weapons = u->weapons;
		const LocalModel& lm = u->localModel;
		const std::vector<LocalModelPiece>& pieces = lm.pieces;

		const auto& pos  = u->pos;
		const auto& xdir = u->rightdir;
		const auto& ydir = u->updir;
		const auto& zdir = u->frontdir;
		const auto& speed = u->speed;

		file << "\t\tunitID: " << u->id << " (name: " << u->unitDef->name << ")\n";
		file << "\t\t\tpos: " << TapFloats(pos);
		file << "\t\t\tspeed: " << TapFloats(speed);
		file << "\t\t\txdir: " << TapFloats(xdir);
		file << "\t\t\tydir: " << TapFloats(ydir);
		file << "\t\t\tzdir: " << TapFloats(zdir);
		file << "\t\t\trelMidPos: " << TapFloats(u->relMidPos);
		file << "\t\t\trelAimPos: " << TapFloats(u->relAimPos);
		file << "\t\t\tmidPos: " << TapFloats(u->midPos);
		file << "\t\t\theading: " << int(u->heading) << ", mapSquare: " << u->mapSquare << "\n";
		file << "\t\t\thealth: " << TapFloats(u->health);
		file << "\t\t\texperience: " << TapFloats(u->experience);
		file << "\t\t\tisDead: " << u->isDead << ", activated: " << u->activated << "\n";
		file << "\t\t\tphysicalState: " << u->physicalState << "\n";
		file << "\t\t\tfireState: " << u->fireState << ", moveState: " << u->moveState << "\n";
		file << "\t\t\tpieces: " << pieces.size() << "\n";
		file << "\t\t\tinBuildStance " << u->inBuildStance << "\n";

		#ifdef DUMP_UNIT_PIECE_DATA
		for (const LocalModelPiece& lmp: pieces) {
			const S3DModelPiece* omp = lmp.original;
			const S3DModelPiece* par = omp->parent;
			const float3& ppos = lmp.GetPosition();
			const float3& prot = lmp.GetRotation();

			file << "\t\t\t\tname: " << omp->name << " (parentName: " << ((par != nullptr)? par->name: "[null]") << ")\n";
			file << "\t\t\t\tpos: " << TapFloats(ppos);
			file << "\t\t\t\trot: " << TapFloats(prot);
			file << "\t\t\t\tvisible: " << lmp.GetScriptVisible() << "\n";
			file << "\n";
		}
		#endif

		file << "\t\t\tweapons: " << weapons.size() << "\n";

		#ifdef DUMP_UNIT_WEAPON_DATA
		for (const CWeapon* w: weapons) {
			const float3& awp = w->aimFromPos;
			const float3& rwp = w->relAimFromPos;
			const float3& amp = w->weaponMuzzlePos;
			const float3& rmp = w->relWeaponMuzzlePos;

			file << "\t\t\t\tweaponID: " << w->weaponNum << " (name: " << w->weaponDef->name << ")\n";
			file << "\t\t\t\tweaponDir: " << TapFloats(w->weaponDir);
			file << "\t\t\t\tabsWeaponPos: " << TapFloats(awp);
			file << "\t\t\t\trelAimFromPos: " << TapFloats(rwp);
			file << "\t\t\t\tabsWeaponMuzzlePos: " << TapFloats(amp);
			file << "\t\t\t\trelWeaponMuzzlePos: " << TapFloats(rmp);
			file << "\n";
		}
		#endif

		#ifdef DUMP_UNIT_COMMANDAI_DATA
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		file << "\t\t\tcommandAI:\n";
		file << "\t\t\t\torderTarget->id: " << ((cai->orderTarget != nullptr)? cai->orderTarget->id: -1) << "\n";
		file << "\t\t\t\tcommandQue.size(): " << cq.size() << "\n";

		for (const Command& c: cq) {
			file << "\t\t\t\t\tcommandID: " << c.GetID() << "\n";
			file << "\t\t\t\t\ttag: " << c.GetTag() << ", options: " << +c.GetOpts() << "\n";
			file << "\t\t\t\t\tparams: " << c.GetNumParams() << "\n";

			for (unsigned int n = 0; n < c.GetNumParams(); n++) {
				file << "\t\t\t\t\t\t" << c.GetParam(n) << "\n";
			}
		}
		#endif

		#ifdef DUMP_UNIT_MOVETYPE_DATA
		const AMoveType* amt = u->moveType;
		const float3& goalPos = amt->goalPos;
		const float3& oldUpdatePos = amt->oldPos;
		const float3& oldSlowUpPos = amt->oldSlowUpdatePos;

		file << "\t\t\tmoveType:\n";
		file << "\t\t\t\tgoalPos: " << TapFloats(goalPos);
		file << "\t\t\t\toldUpdatePos: " << TapFloats(oldUpdatePos);
		file << "\t\t\t\toldSlowUpPos: " << TapFloats(oldSlowUpPos);
		file << "\t\t\t\tmaxSpeed: " << TapFloats(amt->GetMaxSpeed());
		file << "\t\t\t\tmaxWantedSpeed: " << TapFloats(amt->GetMaxWantedSpeed());
		file << "\t\t\t\tprogressState: " << amt->progressState << "\n";

		if (const auto* gmt = dynamic_cast<const CGroundMoveType*>(amt)) {
			file << "\t\t\t\tCGroundMoveType:\n";
			file << "\t\t\t\t\tcurrWayPoint: " << TapFloats(gmt->GetCurrWayPoint());
			file << "\t\t\t\t\tnextWayPoint: " << TapFloats(gmt->GetNextWayPoint());
		}
		#endif

		#ifdef DUMP_UNIT_BUILDER_DATA
		if (const CBuilder* b = dynamic_cast<const CBuilder*>(u); b != nullptr) {
			file << "\t\t\tThe unit is CBuilder:\n";
			file << "\t\t\t\tcurResurrect: " << DumpSolidObjectID(b->curResurrect);
			file << "\t\t\t\tlastResurrected: " << b->lastResurrected << "\n";
			file << "\t\t\t\tcurBuild: " << DumpSolidObjectID(b->curBuild);
			file << "\t\t\t\tcurCapture: " << DumpSolidObjectID(b->curCapture);
			file << "\t\t\t\tcurReclaim: " << DumpSolidObjectID(b->curReclaim);
			file << "\t\t\t\treclaimingUnit: " << (b->reclaimingUnit ? 1 : 0) << "\n";
			file << "\t\t\t\thelpTerraform: " << DumpSolidObjectID(b->helpTerraform);
			file << "\t\t\t\tterraforming: " << (b->terraforming ? 1 : 0) << "\n";
			file << "\t\t\t\tterraformHelp: " << TapFloats(b->terraformHelp);
			file << "\t\t\t\tmyTerraformLeft: " << TapFloats(b->myTerraformLeft);
			file << "\t\t\t\tterraformType: " << std::to_string(b->terraformType) << "\n";
			file << "\t\t\t\ttx1,tx2,tz1,tz2: " << b->tx1 << "," << b->tx2 << "," << b->tz1 << "," << b->tz2 << "\n";
			file << "\t\t\t\tterraformCenter: " << TapFloats(b->terraformCenter);
			file << "\t\t\t\tterraformRadius: " << TapFloats(b->terraformRadius);
		}
		#endif
	}
	file << "\tunitsToBeRemoved: " << unitHandler.GetUnitsToBeRemoved().size() << "\n";
	for (auto* u : unitHandler.GetUnitsToBeRemoved()) {
		file << "\t\tunitID: " << u->id << " (name: " << u->unitDef->name << ")\n";
	}
	#endif
	#ifdef DUMP_UNIT_SCRIPT_DATA
	{
		file << "\tCobEngine:\n";
		file << "\t\tcurrentTime: " << cobEngine->GetCurrTime();
		file << "\t\tCobThreads: " << cobEngine->GetThreadInstances().size() << "\n";
		for (const auto& [tid, thread] : cobEngine->GetThreadInstances()) {
			auto ownerID = thread.cobInst->GetUnit() ? thread.cobInst->GetUnit()->id : -1;
			file << "\t\t\tid: " << tid << " t.id " << thread.GetID() << " t.wt " << thread.GetWakeTime()
				 << " owner " << ownerID
				 << " t.state " << +thread.GetState() << " t.sigmask " << thread.GetSignalMask()
				 << " t.retc " << thread.GetRetCode()
				 << " dead|gargage|waiting " << thread.IsDead() << "|" << thread.IsGarbage() << "|" << thread.IsWaiting() << "\n";
		}
		file << "\t\tWaitingThreads: " << cobEngine->GetWaitingThreadIDs().size();
		file << "\t\t\tids:";
		for (const auto id : cobEngine->GetWaitingThreadIDs()) {
			file << " " << id;
		}
		file << "\n";

		auto zzzThreads = cobEngine->GetSleepingThreadIDs(); //copied on purpose
		file << "\t\tSleepingThreads: " << zzzThreads.size();
		file << "\t\t\twts|ids:";
		while (!zzzThreads.empty()) {
			const auto& zt = zzzThreads.top();
			file << " " << zt.wt << "|" << zt.id;
			zzzThreads.pop();
		}
		file << "\n";
	}
	#endif

	file << "\tfeatures: " << activeFeatureIDs.size() << "\n";

	#ifdef DUMP_FEATURE_DATA
	for (const int featureID: activeFeatureIDs) {
		const CFeature* f = featureHandler.GetFeature(featureID);

		const auto& pos  = f->pos;
		const auto& xdir = f->rightdir;
		const auto& ydir = f->updir;
		const auto& zdir = f->frontdir;
		const auto& speed = f->speed;

		file << "\t\tfeatureID: " << f->id << " (name: " << f->def->name << ")\n";
		file << "\t\t\tpos: " << TapFloats(pos);
		file << "\t\t\tspeed: " << TapFloats(speed);
		file << "\t\t\txdir: " << TapFloats(xdir);
		file << "\t\t\tydir: " << TapFloats(ydir);
		file << "\t\t\tzdir: " << TapFloats(zdir);
		file << "\t\t\trelMidPos: " << TapFloats(f->relMidPos);
		file << "\t\t\trelAimPos: " << TapFloats(f->relAimPos);
		file << "\t\t\tmidPos: " << TapFloats(f->midPos);
		file << "\t\t\thealth: " << TapFloats(f->health);
		file << "\t\t\treclaimLeft: " << TapFloats(f->reclaimLeft);
	}
	#endif

	file << "\tprojectiles: " << projectiles.size() << "\n";

	#ifdef DUMP_PROJECTILE_DATA
	for (const CProjectile* p: projectiles) {
		file << "\t\tprojectileID: " << p->id << "\n";
		file << "\t\t\tpos: <" << TapFloats(p->pos);
		file << "\t\t\tdir: <" << TapFloats(p->dir);
		file << "\t\t\tspeed: <" << TapFloats(p->speed);
		file << "\t\t\tweapon: " << p->weapon << ", piece: " << p->piece << "\n";
		file << "\t\t\tcheckCol: " << p->checkCol << ", deleteMe: " << p->deleteMe << "\n";
	}
	#endif

	file << "\tteams: " << teamHandler.ActiveTeams() << "\n";

	#ifdef DUMP_TEAM_DATA
	for (int a = 0; a < teamHandler.ActiveTeams(); ++a) {
		const CTeam* t = teamHandler.Team(a);

		file << "\t\tteamID: " << t->teamNum << " (controller: " << t->GetControllerName() << ")\n";
		file << "\t\t\tmetal: " << TapFloats(t->res.metal);
		file << "\t\t\tenergy: " << TapFloats(t->res.energy);
		file << "\t\t\tmetalPull: " << TapFloats(t->resPull.metal);
		file << "\t\t\tenergyPull: " << TapFloats(t->resPull.energy);
		file << "\t\t\tmetalIncome: " << TapFloats(t->resIncome.metal);
		file << "\t\t\tenergyIncome: " << TapFloats(t->resIncome.energy);
		file << "\t\t\tmetalExpense: " << TapFloats(t->resExpense.metal);
		file << "\t\t\tenergyExpense: " << TapFloats(t->resExpense.energy);
	}
	#endif

	file << "\tallyteams: " << teamHandler.ActiveAllyTeams() << "\n";

	std::array<ILosType*, 7> losTypes = {
		&losHandler->los,
		&losHandler->airLos,
		&losHandler->radar,
		&losHandler->sonar,
		&losHandler->seismic,
		&losHandler->jammer,
		&losHandler->sonarJammer
	};
	#if defined(DUMP_ALLYTEAM_DATA) || defined(DUMP_ALLYTEAM_DATA_CHECKSUM)
	for (int a = 0; a < teamHandler.ActiveAllyTeams(); ++a) {
		file << "\t\tallyteamID: " << a << "\n";

		for (int lti = 0; lti < losTypes.size(); ++lti) {
			file << "\t\t\tLOS-map type:" << lti << "\n";
			const auto lt = losTypes[lti];
			const auto* lm = &lt->losMaps[a].front();

			#ifdef DUMP_ALLYTEAM_DATA
			file << "\t\t\t\t";
			for (unsigned int i = 0; i < (lt->size.x * lt->size.y); i++) {
				file << lm[i] << " ";
			}
			file << "\n";
			#endif

			#ifdef DUMP_ALLYTEAM_DATA_CHECKSUM
			uint32_t adCs = 0;
			for (unsigned int i = 0; i < (lt->size.x * lt->size.y); i++) {
				adCs = spring::LiteHash(lm[i], adCs);
			}
			file << "\t\t\t\thash: " << adCs << "\n";
			#endif
		}
	}
	#endif

	const auto heightmap = readMap->GetCornerHeightMapSynced();
	const auto centerNormals = readMap->GetCenterNormalsSynced();
	const auto faceNormals = readMap->GetFaceNormalsSynced();
	#ifdef DUMP_HEIGHTMAP
	file << "\theightmap as uint32t: " << "\n";
	file << "\t\t";
	for (unsigned int i = 0; i < (mapDims.mapxp1 * mapDims.mapyp1); i++) {
		file << *reinterpret_cast<const uint32_t*>(&heightmap[i]) << " ";
	}
	file << "\n";

	file << "\tcenterNormals as uint32t: " << "\n";
	file << "\t\t";
	for (unsigned int i = 0; i < (mapDims.mapx * mapDims.mapy); i++) {
		file << *reinterpret_cast<const uint32_t*>(&centerNormals[i].x) << " ";
		file << *reinterpret_cast<const uint32_t*>(&centerNormals[i].y) << " ";
		file << *reinterpret_cast<const uint32_t*>(&centerNormals[i].z) << " ";
	}
	file << "\n";

	file << "\tfaceNormals as uint32t: " << "\n";
	file << "\t\t";
	for (unsigned int i = 0; i < (mapDims.mapx * mapDims.mapy); i++) {
		file << *reinterpret_cast<const uint32_t*>(&faceNormals[i + 0].x) << " ";
		file << *reinterpret_cast<const uint32_t*>(&faceNormals[i + 0].y) << " ";
		file << *reinterpret_cast<const uint32_t*>(&faceNormals[i + 0].z) << " ";

		file << *reinterpret_cast<const uint32_t*>(&faceNormals[i + 1].x) << " ";
		file << *reinterpret_cast<const uint32_t*>(&faceNormals[i + 1].y) << " ";
		file << *reinterpret_cast<const uint32_t*>(&faceNormals[i + 1].z) << " ";
	}
	file << "\n";

	#endif

	#ifdef DUMP_HEIGHTMAP_CHECKSUM
	uint32_t hmCs = 0;
	uint32_t cnCs = 0;
	uint32_t fnCs = 0;
	for (unsigned int i = 0; i < (mapDims.mapxp1 * mapDims.mapyp1); i++) {
		hmCs = spring::LiteHash(heightmap[i], hmCs);
	}
	for (unsigned int i = 0; i < (mapDims.mapx * mapDims.mapy); i++) {
		cnCs = spring::LiteHash(centerNormals[i], cnCs);
	}
	for (unsigned int i = 0; i < (mapDims.mapx * mapDims.mapy); i++) {
		fnCs = spring::LiteHash(faceNormals[i + 0], fnCs);
		fnCs = spring::LiteHash(faceNormals[i + 1], fnCs);
	}

	file << "\theightmap checksum as uint32t: " << hmCs << "\n";
	file << "\tcenterNormals checksum as uint32t: " << cnCs << "\n";
	file << "\tfaceNormals checksum as uint32t: " << fnCs << "\n";
	#endif

	const auto smoothMesh = smoothGround.GetMeshData();
	#ifdef DUMP_SMOOTHMESH
	file << "\tsmoothMesh as uint32t: " << "\n";
	file << "\t\t";
	for (unsigned int i = 0; i < (smoothGround.GetMaxX() * smoothGround.GetMaxY()); i++) {
		file << *reinterpret_cast<const uint32_t*>(&smoothMesh[i]) << " ";
	}
	file << "\n";
	#endif

	#ifdef DUMP_SMOOTHMESH_CHECKSUM
	uint32_t smCs = 0;
	for (unsigned int i = 0; i < (smoothGround.GetMaxX() * smoothGround.GetMaxY()); i++) {
		smCs = spring::LiteHash(smoothMesh[i], smCs);
	}
	file << "\tsmoothMesh checksum as uint32t: " << smCs << "\n";
	#endif

	file.flush();
	if (gs->frameNum == gMaxFrameNum)
		file.close();

	gMinFrameNum = -1;
	gMaxFrameNum = -1;
	gFramePeriod =  1;
}

void DumpRNG(int newMinFrameNum, int newMaxFrameNum)
{
	static std::fstream file;
	static int gMinFrameNum = -1;
	static int gMaxFrameNum = -1;

	const int oldMinFrameNum = gMinFrameNum;
	const int oldMaxFrameNum = gMaxFrameNum;

	if (!gs->cheatEnabled)
		return;
	// check if the range is valid
	if (newMaxFrameNum < newMinFrameNum)
		return;


	// adjust the bounds if the new values are valid
	if (newMinFrameNum >= 0) gMinFrameNum = newMinFrameNum;
	if (newMaxFrameNum >= 0) gMaxFrameNum = newMaxFrameNum;

	if ((gMinFrameNum != oldMinFrameNum) || (gMaxFrameNum != oldMaxFrameNum)) {
		LOG("[%s] dumping RNG state (from %d to %d)", __func__, gMinFrameNum, gMaxFrameNum);
		// bounds changed, open a new file
		if (file.is_open()) {
			file.flush();
			file.close();
		}

		std::string name = (gameServer != nullptr) ? "Server" : "Client";
		name += "SyncedRNG-";
		name += IntToString(guRNG.NextInt());
		name += "-[";
		name += IntToString(gMinFrameNum);
		name += "-";
		name += IntToString(gMaxFrameNum);
		name += "].txt";

		file.open(name.c_str(), std::ios::out);

		if (file.is_open()) {
			file << " mapName: " << gameSetup->mapName << "\n";
			file << " modName: " << gameSetup->modName << "\n";
			file << "minFrame: " << gMinFrameNum << "\n";
			file << "maxFrame: " << gMaxFrameNum << "\n";
			file << "randSeed: " << gsRNG.GetLastSeed() << "\n";
			file << "initSeed: " << gsRNG.GetInitSeed() << "\n";
		}

		LOG("[%s] using RNG dump-file \"%s\"", __func__, name.c_str());
	}

	if (file.bad() || !file.is_open())
		return;

	if (gs->frameNum == gMaxFrameNum + 1) { //close the file and remove debug callback early next frame after gMaxFrameNum
		gsRNG.SetDebug();
		file.close();
		gMinFrameNum = -1;
		gMaxFrameNum = -1;
	}
	// check if the CURRENT frame lies within the bounds
	if (gs->frameNum < gMinFrameNum)
		return;
	if (gs->frameNum > gMaxFrameNum)
		return;

	//must be static to not get destroyed when the function scope is lost
	static auto fcb = [](auto N, auto R) {
		file << "N=" << N << ", R=" << R << "\n";
	};

	if (gs->frameNum == gMinFrameNum)
		gsRNG.SetDebug(fcb);

	file.flush(); //before the next frame begins
	file << "frame: " << gs->frameNum << ", seed: " << gsRNG.GetLastSeed() << "\n";
}
