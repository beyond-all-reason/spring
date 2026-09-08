/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Path.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/Threading/ThreadPool.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"

#include <bit>
#include <cstring>


// what the texture holds while nothing is selected
static constexpr SColor CLEAR_COLOR = SColor(1.0f, 0.0f, 0.0f, 1.0f);

// texels (re)computed per update; the MoveDef view is cheap enough per
// texel to sweep the texture (and thus refresh) faster than the build view
static constexpr int MOVEDEF_TEXELS_PER_UPDATE = 256 * 256;
// bigger bands for the first sweep after a view change, so the old picture goes away quickly
static constexpr int FIRST_SWEEP_TEXELS_PER_UPDATE = 512 * 512;
static constexpr int   BUILD_TEXELS_PER_UPDATE = 128 * 128;

// updates over which a changed row of the MoveDef view fades into the visible
// texture; also the number of rest updates between two sweeps
static constexpr int FADE_STEPS = 4;


CPathTexture::CPathTexture()
: CModernInfoTexture("path")
, isCleared(true)
//, updateFrame(0)
, updateProcess(0)
, restUpdates(0)
, fadeAllRows(true)
, lastSelectedPathType(0)
, forcedPathType(-1)
, forcedUnitDef(-1)
, lastUsage(spring_gettime())
{
	texSize = int2(mapDims.hmapx, mapDims.hmapy);

	shownTexels.assign(texSize.x * texSize.y, CLEAR_COLOR);
	sweepTexels.assign(texSize.x * texSize.y, CLEAR_COLOR);
	rowFadeSteps.assign(texSize.y, 0);
	rowTouched.assign(texSize.y, 0);

	GL::TextureCreationParams tcp{
		.reqNumLevels = 1,
		.wrapMirror = false,
		.minFilter = GL_NEAREST,
		.magFilter = GL_LINEAR
	};

	texture = GL::Texture2D(texSize, GL_RGBA8, tcp, false);

	CreateFBO("CPathTexture");

	if (!fbo.IsValid()) {
		throw opengl_error("");
	}

	// start out cleared, matching the CPU mirror initialized above
	fbo.Bind();
	glClearColor(CLEAR_COLOR.r / 255.0f, CLEAR_COLOR.g / 255.0f, CLEAR_COLOR.b / 255.0f, CLEAR_COLOR.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	FBO::Unbind();
}


enum BuildSquareStatus {
	NOLOS          = 0,
	FREE           = 1,
	OBJECTBLOCKED  = 2,
	TERRAINBLOCKED = 3,
};


static constexpr SColor buildColors[] = {
	SColor(  0,   0,   0), // nolos
	SColor(  0, 255,   0), // free
	SColor(  0,   0, 255), // objblocked
	SColor(254,   0,   0), // terrainblocked
};


static inline const SColor& GetBuildColor(const BuildSquareStatus& status) {
	RECOIL_DETAILED_TRACY_ZONE;
	return buildColors[status];
}


static SColor GetSpeedModColor(const float sm) {
	RECOIL_DETAILED_TRACY_ZONE;
	SColor col(255, 0, 0);

	if (sm > 0.0f) {
		col.r = 255 - std::min(sm * 255.0f, 255.0f);
		col.g = 255 - col.r;
	} else {
		col.b = 255;
	}

	return col;
}


const MoveDef* CPathTexture::GetSelectedMoveDef()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (forcedPathType >= 0)
		return moveDefHandler.GetMoveDefByPathType(forcedPathType);

	const auto& unitSet = selectedUnitsHandler.selectedUnits;

	if (unitSet.empty())
		return nullptr;

	const auto iter = unitSet.begin();
	const CUnit* unit = unitHandler.GetUnit(*iter);
	return unit->moveDef;
}


const UnitDef* CPathTexture::GetCurrentBuildCmdUnitDef()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (forcedUnitDef >= 0)
		return unitDefHandler->GetUnitDefByID(forcedUnitDef);

	if ((unsigned)guihandler->inCommand > guihandler->commands.size())
		return nullptr;

	if (guihandler->commands[guihandler->inCommand].type != CMDTYPE_ICON_BUILDING)
		return nullptr;

	return unitDefHandler->GetUnitDefByID(-guihandler->commands[guihandler->inCommand].id);
}


GLuint CPathTexture::GetTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	lastUsage = spring_gettime();
	return texture.GetId();
}


bool CPathTexture::ShowMoveDef(const int pathType)
{
	RECOIL_DETAILED_TRACY_ZONE;
	forcedUnitDef  = -1;
	forcedPathType = pathType;
	RestartSweep();
	lastUsage = spring_gettime(); // otherwise IsUpdateNeeded() drops the forced view before its first draw
	return true; // TODO: unused
}


bool CPathTexture::ShowUnitDef(const int udefid)
{
	RECOIL_DETAILED_TRACY_ZONE;
	forcedUnitDef  = udefid;
	forcedPathType = -1;
	RestartSweep();
	lastUsage = spring_gettime(); // otherwise IsUpdateNeeded() drops the forced view before its first draw
	return true; // TODO: unused
}


bool CPathTexture::IsUpdateNeeded()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// don't update when not rendered/used
	if ((spring_gettime() - lastUsage).toSecsi() > 2) {
		forcedUnitDef = forcedPathType = -1;
		RestartSweep();
		return false;
	}

	// newly build cmd active?
	const UnitDef* ud = GetCurrentBuildCmdUnitDef();

	if (ud != nullptr) {
		const unsigned int buildDefID = -(ud->id + 1);

		if (buildDefID != lastSelectedPathType) {
			lastSelectedPathType = buildDefID;
			RestartSweep();
			return true;
		}
	} else {
		// newly unit/moveType active?
		const MoveDef* md = GetSelectedMoveDef();

		if (md != nullptr) {
			const unsigned int pathType = md->pathType + 1;

			if (pathType != lastSelectedPathType) {
				lastSelectedPathType = pathType;
				RestartSweep();
				return true;
			}
		}
	}

	// nothing selected nor any build cmd active -> don't update
	return (lastSelectedPathType != 0 || !isCleared);
}


// advances the sweep by one band of texel rows, returns the end row
int CPathTexture::AdvanceSweep(int texelsPerUpdate)
{
	// the build view wraps around here, the MoveDef view restarts via BeginSweep
	if (updateProcess >= texSize.y)
		updateProcess = 0;

	const int updateLines = std::max(texelsPerUpdate / texSize.x, ThreadPool::GetNumThreads());

	updateProcess = std::min(updateProcess + updateLines, texSize.y);
	return updateProcess;
}


/*
 * <shownTexels> always mirrors the GL texture. The build view is computed
 * straight into it, one band of rows per update, and uploaded band by band
 * (as it always was).
 *
 * The MoveDef view instead computes its frame into <sweepTexels>, one band
 * per update, and marks the rows whose texels changed. FadeRows() then blends
 * each marked row over FADE_STEPS updates into <shownTexels> and uploads only
 * those rows: a band fades in as soon as it is computed, so there is neither
 * a visible sweep front nor a pop when the map changes, and on a quiet map a
 * refresh costs just the sweep. After a view change every row is marked
 * (fadeAllRows) and the sweep uses bigger bands to get rid of the old picture
 * quickly. Clearing fades to CLEAR_COLOR through the same path.
 *
 * One refresh cycle is the sweep plus FADE_STEPS rest updates, so the last
 * band has settled before the next sweep starts.
 */
void CPathTexture::Update()
{
	ZoneScopedN("CPathTexture::Update");
	const MoveDef* md = GetSelectedMoveDef();
	const UnitDef* ud = GetCurrentBuildCmdUnitDef();

	// nothing selected: fade the last view out to the clear color
	if (ud == nullptr && md == nullptr) {
		if (!isCleared) {
			isCleared = true;
			RestartSweep();
			std::fill(sweepTexels.begin(), sweepTexels.end(), CLEAR_COLOR);
			std::fill(rowFadeSteps.begin(), rowFadeSteps.end(), FADE_STEPS);
		}

		FadeRows();
		return;
	}

	if (ud != nullptr) {
		// build view: spread across updates, written straight into the visible texture
		const int start = updateProcess;
		const int end = AdvanceSweep(BUILD_TEXELS_PER_UPDATE);

		UpdateBuildView(ud, start, end);

		auto binding = texture.ScopedBind();
		texture.UploadSubImage(&shownTexels[start * texSize.x], 0, start, texSize.x, end - start);

		isCleared = false;
		return;
	}

	// MoveDef view, see above
	if (updateProcess < texSize.y) {
		const int start = updateProcess;
		const int end = AdvanceSweep(fadeAllRows ? FIRST_SWEEP_TEXELS_PER_UPDATE : MOVEDEF_TEXELS_PER_UPDATE);

		UpdateMoveDefView(*md, start, end);
	} else {
		restUpdates += 1;
	}

	const bool rowsPending = FadeRows();

	// the next sweep starts once every row has settled and the cycle has had
	// its FADE_STEPS rest updates, which keeps the refresh rate (and cost) of
	// a quiet map the same as that of a changing one
	if (updateProcess >= texSize.y && !rowsPending && restUpdates >= FADE_STEPS)
		BeginSweep(false);

	isCleared = false;
}


// blends every pending row one step closer to <sweepTexels> and uploads it;
// returns whether any row still has steps left afterwards
bool CPathTexture::FadeRows()
{
	ZoneScopedN("CPathTexture::FadeRows");

	bool anyPending = false;

	for (int y = 0; y < texSize.y && !anyPending; ++y) {
		anyPending = (rowFadeSteps[y] != 0);
	}

	if (!anyPending)
		return false;

	const size_t rowBytes = texSize.x * sizeof(SColor);

	for_mt(0, texSize.y, [&](const int y) {
		const int stepsLeft = rowFadeSteps[y];

		rowTouched[y] = (stepsLeft != 0);

		if (stepsLeft == 0)
			return;

		      uint8_t* shown = reinterpret_cast<      uint8_t*>(&shownTexels[y * texSize.x]);
		const uint8_t* sweep = reinterpret_cast<const uint8_t*>(&sweepTexels[y * texSize.x]);

		if (stepsLeft == 1) {
			// the last step is an exact copy, so a settled texture equals the
			// sweep buffer bit for bit regardless of the rounding of the blends
			std::memcpy(shown, sweep, rowBytes);
		} else {
			// linear fade: cover a stepsLeft-th of the remaining distance
			// (fixed-point 1/stepsLeft, vectorizes unlike a division)
			const int weight = 256 / stepsLeft;

			for (size_t i = 0; i < rowBytes; ++i) {
				shown[i] += (((int(sweep[i]) - int(shown[i])) * weight) >> 8);
			}
		}

		rowFadeSteps[y] = stepsLeft - 1;
	});

	// upload the runs of rows touched this step
	bool stillPending = false;
	int runStart = -1;

	{
		auto binding = texture.ScopedBind();

		for (int y = 0; y <= texSize.y; ++y) {
			const bool touched = (y < texSize.y) && (rowTouched[y] != 0);

			if (touched) {
				stillPending |= (rowFadeSteps[y] != 0);

				if (runStart < 0)
					runStart = y;

				continue;
			}

			if (runStart >= 0) {
				texture.UploadSubImage(&shownTexels[runStart * texSize.x], 0, runStart, texSize.x, y - runStart);
				runStart = -1;
			}
		}
	}

	return stillPending;
}


void CPathTexture::UpdateBuildView(const UnitDef* ud, int texStart, int texEnd)
{
	ZoneScopedN("CPathTexture::UpdateBuildView");

	for_mt(texStart, texEnd, [&](const int y) {
		const int currentThread = ThreadPool::GetThreadNum();

		SColor* texels = &shownTexels[y * texSize.x];

		for (int x = 0; x < texSize.x; ++x) {
			const float3 pos = float3(x << 1, 0.0f, y << 1) * SQUARE_SIZE;

			BuildSquareStatus status = FREE;
			BuildInfo bi(ud, pos, guihandler->buildFacing);
			bi.pos = CGameHelper::Pos2BuildPos(bi, false);

			CFeature* f = nullptr;

			if (CGameHelper::TestUnitBuildSquare(
					bi, f, gu->myAllyTeam, false, nullptr, nullptr, currentThread
				)) {
				if (f != nullptr) {
					status = OBJECTBLOCKED;
				}
			} else {
				status = TERRAINBLOCKED;
			}

			texels[x] = GetBuildColor(status);
		}
	});
}


/*
 * Each texel covers a 2x2 block of map squares. Its color is the terrain
 * speed-modifier of the top-left square, darkened by a quarter for every
 * square of the block that CMoveMath::IsBlocked reports as BLOCK_STRUCTURE
 * (zero speed-modifier, or a structure inside the MoveDef footprint centered
 * on the square). Structures only count inside the local player's LOS.
 *
 * IsBlocked visits O(footprint area) blocking-map cells per square, and
 * neighbouring squares revisit the same cells. Instead the band of map rows
 * updated this frame, plus a footprint-sized margin, is scanned once for
 * squares holding a structure that blocks this MoveDef (skipping the empty
 * cells via the blocking-map bitmap). That mask is then dilated by the
 * footprint, horizontally then vertically, sampling every other square
 * exactly like RangeIsBlocked does, so both passes are plain byte-ORs over
 * rows and the result matches IsBlocked square for square.
 *
 * The only part of ObjectBlockType that depends on the querying square is
 * the in-water exemption of IsNonBlocking, which looks at the ground height
 * there. Structures standing in water therefore go into a second mask, and
 * the (few) squares whose footprint reaches one of them fall back to the
 * per-square test.
 */
void CPathTexture::UpdateMoveDefView(const MoveDef& md, int texStart, int texEnd)
{
	ZoneScopedN("CPathTexture::UpdateMoveDefView");

	const int mapx = mapDims.mapx;
	const int mapy = mapDims.mapy;

	// map-square rows covered by the texel rows [texStart, texEnd)
	const int sqStart = texStart * 2;
	const int sqEnd   = std::min(texEnd * 2, mapy);

	const int xsh = md.xsizeh;
	const int zsh = md.zsizeh;

	// rows whose structures reach into the band through the footprint
	const int padStart = std::max(sqStart - zsh, 0);
	const int padEnd   = std::min(sqEnd   + zsh, mapy);
	const int numPadRows = padEnd - padStart;

	// zero margins of xsh squares on both sides, so the horizontal dilation
	// needs no clamping (RangeIsBlocked clamps squares outside the map away)
	const int rowStride = mapx + 2 * xsh;

	// CheckCollisionQuery::UpdateElevationForPos puts the collider at
	// max(groundHeight, -waterline), so only a waterline below the surface
	// can ever put it in water
	const bool canBeInWater = (md.waterline > 0.0f);

	// per padded map row: which of the two masks have anything set, so the
	// dilation passes can skip the (many) rows without structures
	enum { ROW_HAS_STRUCT = 1, ROW_HAS_WATER = 2 };

	// *Squares: one flag per map square, *Spans: the same after horizontal
	// dilation, *Footprints: after vertical dilation, only for the band rows;
	// no assign() here, the rows are cleared by the tasks that fill them
	structSquares.resize(numPadRows * rowStride);
	structSpans.resize(numPadRows * rowStride);
	waterSquares.resize(numPadRows * rowStride);
	waterSpans.resize(numPadRows * rowStride);
	rowFlags.resize(numPadRows);
	structFootprints.resize((sqEnd - sqStart) * mapx);
	waterFootprints.resize((sqEnd - sqStart) * mapx);

	// horizontal footprint dilation of one row with the sampling of
	// RangeIsBlocked: every other square from max(x - xsh, 0) to
	// min(x + xsh, mapx - 1); <src> and <dst> point at column 0 of rows
	// that have <xsh> zeroed squares on either side
	const auto dilateRow = [&](const uint8_t* src, uint8_t* dst) {
		std::memset(dst - xsh, 0, rowStride);

		// interior: reading past either end lands in the zero margins,
		// which is what clamping the range end to the map does
		for (int dx = -xsh; dx <= xsh; dx += 2) {
			const uint8_t* s = src + dx;

			for (int x = xsh; x < mapx; ++x) {
				dst[x] |= s[x];
			}
		}

		// left border: clamping the range *start* to 0 changes which
		// squares get sampled, so these columns are done explicitly
		for (int x = 0, xn = std::min(xsh, mapx); x < xn; ++x) {
			const int xmax = std::min(x + xsh, mapx - 1);
			uint8_t v = 0;

			for (int sx = 0; sx <= xmax; sx += 2) {
				v |= src[sx];
			}

			dst[x] = v;
		}
	};

	// pass 1: per map row, flag the squares holding a structure that blocks
	// this MoveDef and dilate the flags horizontally
	{
		ZoneScopedN("CPathTexture::UpdateMoveDefView::Squares");

		const std::vector<uint32_t>& cellBits = groundBlockingObjectMap.GetCellBits();

		for_mt(padStart, padEnd, [&](const int z) {
			// ObjectBlockType only looks at the collider position through the
			// in-water exemptions of IsNonBlocking; a collider that is not in
			// water (pos.y = 0 also enables the height checks, the query would
			// otherwise take the pathfinder-estimator branch) gets the
			// height-independent answer for every object
			MoveTypes::CheckCollisionQuery dryCollider(&md);
			dryCollider.pos.y = 0.0f;

			const int rowOffset = (z - padStart) * rowStride + xsh;

			uint8_t* structRow = &structSquares[rowOffset];
			uint8_t* waterRow  = &waterSquares[rowOffset];
			uint8_t  flags = 0;

			std::memset(structRow - xsh, 0, rowStride);
			std::memset(waterRow  - xsh, 0, rowStride);

			// only visit the non-empty cells, skipping over the empty
			// ones 32 at a time through the bitmap
			const unsigned int rowStart = z * mapx;
			const unsigned int rowEnd   = rowStart + mapx;

			for (unsigned int sqr = rowStart; sqr < rowEnd; ) {
				const uint32_t bits = cellBits[sqr >> 5] >> (sqr & 31);

				if (bits == 0) {
					sqr += (32 - (sqr & 31));
					continue;
				}

				if ((sqr += std::countr_zero(bits)) >= rowEnd)
					break;

				const int x = sqr - rowStart;
				const auto cell = groundBlockingObjectMap.GetCellUnsafeConst(sqr);

				for (size_t i = 0, n = cell.size(); i < n; i++) {
					const CSolidObject* collidee = cell[i];

					if ((CMoveMath::ObjectBlockType(collidee, &dryCollider) & CMoveMath::BLOCK_STRUCTURE) == 0)
						continue;

					if (canBeInWater && collidee->IsInWater()) {
						waterRow[x] = 1;
						flags |= ROW_HAS_WATER;
					} else {
						structRow[x] = 1;
						flags |= ROW_HAS_STRUCT;
					}
				}

				sqr += 1;
			}

			rowFlags[z - padStart] = flags;

			if (flags & ROW_HAS_STRUCT)
				dilateRow(structRow, &structSpans[rowOffset]);
			if (flags & ROW_HAS_WATER)
				dilateRow(waterRow, &waterSpans[rowOffset]);
		});
	}

	// pass 2: per texel row, dilate vertically and color the texels
	{
		ZoneScopedN("CPathTexture::UpdateMoveDefView::Texels");

		//FIXME make global func
		const bool losFullView = ((gu->spectating && gu->spectatingFullView) || losHandler->GetGlobalLOS(gu->myAllyTeam));

		for_mt(texStart, texEnd, [&](const int y) {
			const int thread = ThreadPool::GetThreadNum();
			const int sqz = y * 2;

			// the two map rows covered by this texel row; vertical dilation
			// samples every other row like RangeIsBlocked (see dilateRow)
			uint8_t* structRows[2];
			uint8_t* waterRows[2];

			for (int i = 0; i < 2; ++i) {
				const int z = sqz + i;

				structRows[i] = &structFootprints[(z - sqStart) * mapx];
				waterRows[i]  = &waterFootprints[(z - sqStart) * mapx];

				std::memset(structRows[i], 0, mapx);
				std::memset(waterRows[i],  0, mapx);

				const int zFirst = (z >= zsh) ? (z - zsh) : 0;
				const int zLast  = std::min(z + zsh, mapy - 1);

				for (int sz = zFirst; sz <= zLast; sz += 2) {
					const int rowOffset = (sz - padStart) * rowStride + xsh;
					const uint8_t flags = rowFlags[sz - padStart];

					if (flags & ROW_HAS_STRUCT) {
						const uint8_t* spans = &structSpans[rowOffset];

						for (int x = 0; x < mapx; ++x) {
							structRows[i][x] |= spans[x];
						}
					}
					if (flags & ROW_HAS_WATER) {
						const uint8_t* spans = &waterSpans[rowOffset];

						for (int x = 0; x < mapx; ++x) {
							waterRows[i][x] |= spans[x];
						}
					}
				}
			}

			// same result as (CMoveMath::IsBlocked(md, x, z, nullptr, thread) & BLOCK_STRUCTURE)
			const auto isBlocked = [&](int x, int z, float speedMod) -> bool {
				if (speedMod == 0.0f)
					return true; // BLOCK_IMPASSABLE

				const int i = z - sqz;

				if (structRows[i][x])
					return true;
				if (!waterRows[i][x])
					return false;

				// a structure standing in water is inside the footprint, whether
				// IsNonBlocking exempts it depends on the height at <x, z>
				return ((CMoveMath::IsBlockedNoSpeedModCheck(md, x, z, nullptr, thread) & CMoveMath::BLOCK_STRUCTURE) != 0);
			};

			SColor* texels = &sweepTexels[y * texSize.x];
			bool changed = false;

			for (int x = 0; x < texSize.x; ++x) {
				const int2 sq = int2(x << 1, y << 1);

				// NOTE: raw speedmods are not necessarily clamped to [0, 1]
				float sm;
				float scale = 1.0f;

				if (losFullView || losHandler->InLos(SquareToFloat3(sq), gu->myAllyTeam)) {
					float sms[4];
					CMoveMath::GetPosSpeedMod2x2(md, sq.x, sq.y, sms);

					sm = sms[0];

					if (isBlocked(sq.x,     sq.y    , sms[0])) { scale -= 0.25f; }
					if (isBlocked(sq.x + 1, sq.y    , sms[1])) { scale -= 0.25f; }
					if (isBlocked(sq.x,     sq.y + 1, sms[2])) { scale -= 0.25f; }
					if (isBlocked(sq.x + 1, sq.y + 1, sms[3])) { scale -= 0.25f; }
				} else {
					sm = CMoveMath::GetPosSpeedMod(md, sq.x, sq.y);
				}

				const SColor texel = GetSpeedModColor(sm * scale);

				changed |= (texels[x].i != texel.i);
				texels[x] = texel;
			}

			if (changed || fadeAllRows)
				rowFadeSteps[y] = FADE_STEPS;
		});
	}
}
