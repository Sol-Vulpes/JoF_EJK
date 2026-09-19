/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// Any dedicated force oriented effects

#include "cg_local.h"
#include "fx_local.h"

// A shared time-window budget bounds work even at very high frame rates.
#define LIGHTNING_INTERVAL 40
#define LIGHTNING_EMIT_INTERVAL 10
#define LIGHTNING_TRACE_BUDGET 192
#define LIGHTNING_EFFECT_BUDGET 256
#define LIGHTNING_SOUND_INTERVAL 200
#define LIGHTNING_SOUND_BUDGET 6
static int lightningBudgetTime;
static int lightningTraces;
static int lightningEffects;
static int lightningSoundBudgetTime;
static int lightningSounds;

static qboolean FX_LightningTrace(trace_t *tr, vec3_t start, vec3_t end, int owner) {
	if (lightningTraces >= LIGHTNING_TRACE_BUDGET)
		return qfalse;
	lightningTraces++;
	CG_Trace(tr, start, NULL, NULL, end, owner, MASK_SHOT);
	return !tr->startsolid && !tr->allsolid;
}

static qboolean FX_LightningSurface(const trace_t *tr) {
	if (tr->fraction == 1.0f || tr->startsolid || tr->allsolid ||
		(tr->surfaceFlags & (SURF_SKY | SURF_NOIMPACT | SURF_NODRAW)))
		return qfalse;
	return tr->entityNum == ENTITYNUM_WORLD ||
		(tr->entityNum >= 0 && tr->entityNum < ENTITYNUM_WORLD &&
		 cg_entities[tr->entityNum].currentState.solid == SOLID_BMODEL);
}

static void FX_LightningArc(vec3_t start, vec3_t end, float width, float chaos, qboolean mainBolt) {
	addElectricityArgStruct_t arc;
	vec3_t delta;
	VectorSubtract(end, start, delta);
	if (lightningEffects >= LIGHTNING_EFFECT_BUDGET || VectorLengthSquared(delta) < 1.0f)
		return;
	lightningEffects++;
	memset(&arc, 0, sizeof(arc));
	VectorCopy(start, arc.start);
	VectorCopy(end, arc.end);
	VectorSet(arc.sRGB, 1.0f, 1.0f, 1.0f);
	VectorCopy(arc.sRGB, arc.eRGB);
	arc.size1 = width;
	arc.size2 = mainBolt ? 1.0f : 0.5f;
	arc.alpha1 = 1.0f;
	// Both ends stay opaque. Fading the far end to 0 for non-main bolts
	// made every branch/link arc invisible exactly at the wall/nest it was
	// supposed to be visibly connecting to - that, not the chaos value, was
	// the main reason connections looked broken/absent rather than like a
	// real bolt.
	arc.alpha2 = 1.0f;
	arc.chaos = chaos;
	arc.killTime = mainBolt ? 65 : 55;
	arc.shader = cgs.media.forceLightningArcShader;
	arc.flags = FX_ALPHA_LINEAR | FX_SIZE_LINEAR;
	if (mainBolt) {
		// Stock electricity's TAPER / BRANCH / GROW flags provide its familiar
		// dense, animated silhouette. Surface arcs stay small and unbranched.
		arc.flags |= 0x01000000 | 0x02000000 | 0x04000000;
	}
	trap->FX_AddElectricity(&arc);
}

static void FX_LightningFlash(vec3_t origin, float size) {
	addspriteArgStruct_t flash;
	if (lightningEffects >= LIGHTNING_EFFECT_BUDGET)
		return;
	lightningEffects++;
	memset(&flash, 0, sizeof(flash));
	VectorCopy(origin, flash.origin);
	flash.scale = size;
	flash.sAlpha = 0.7f;
	flash.life = LIGHTNING_INTERVAL;
	flash.shader = cgs.media.forceLightningFlashShader;
	flash.flags = FX_ALPHA_LINEAR;
	trap->FX_AddSprite(&flash);
}

// --- Nests --------------------------------------------------------------
// A nest is a point out in the environment - a wall, floor or ceiling -
// that the main beam periodically strikes. Nests do NOT connect to each
// other and do NOT spawn their own sub-branches - they're purely a
// destination a branch reaches out and hits. A thin arc is redrawn to each
// active nest every so often so the connection reads as continuous, while
// a stronger "strike" (impact visual + sound, the real DEMP2 wall-impact
// effect) fires on its own slower per-nest timer. A nest lives until its
// expireTime, refilling its strike queue as needed rather than dying when
// strikes run out.
#define LIGHTNING_MAX_NESTS 8
#define LIGHTNING_NESTS_PER_OWNER 3
#define LIGHTNING_NEST_MIN_LIFE 900
#define LIGHTNING_NEST_MAX_LIFE 2000
// Nests are now searched for in random 3D directions from the caster's
// hand (see FX_LightningScatterNest) rather than scattered laterally near
// wherever the aim beam happens to hit - that's what lets a strike land on
// the floor or ceiling even while you're aiming levelly at a wall. This is
// the max probe distance from the hand, not a lateral offset.
#define LIGHTNING_NEST_SCATTER_RADIUS 280.0f
// Random directions that miss all geometry are common (aiming out into
// open space along that particular ray) - try a few per spawn attempt
// instead of relying on a single probe.
#define LIGHTNING_NEST_SCATTER_ATTEMPTS 4
// Ignore hits closer than this to the caster - a "strike" essentially on
// top of the player's own model doesn't read as a branch reaching out.
#define LIGHTNING_NEST_SCATTER_MIN_DIST 40.0f
// Reject candidate directions pointing this far behind the caster's facing
// (dot product against the beam's forward direction). A full random sphere
// let nests land directly behind the player, which reads as broken/out of
// place since you never see it happen. This still allows floor/ceiling
// (roughly perpendicular to forward, dot near 0) - only the rearmost cone
// is cut off.
#define LIGHTNING_NEST_FORWARD_MIN_DOT -0.15f
// Chance per cycle that a new nest attempts to spawn at all - the main
// knob on how often fresh nests appear.
#define LIGHTNING_NEST_SPAWN_CHANCE 0.35f
// Each nest gets a random number of individual strikes queued up at spawn
// time (and refilled whenever it runs out - see FX_LightningUpdateNests),
// then fires them one at a time (not as a simultaneous burst).
#define LIGHTNING_NEST_MIN_STRIKES 10
#define LIGHTNING_NEST_MAX_STRIKES 18
#define LIGHTNING_NEST_STRIKE_MIN_GAP 15
#define LIGHTNING_NEST_STRIKE_MAX_GAP 45
// Width/chaos of the thin, continuously-redrawn arc that keeps a nest
// visibly connected to the main beam between strikes. Chaos kept low - the
// electricity renderer already adds its own wave noise along the arc's
// length, and stacking manual jitter on top of that is what made links
// look like tangled "spaghetti" rather than a bolt.
#define LIGHTNING_NEST_LINK_WIDTH 2.0f
// Wider chaos range than before (was a flat 0.08, then a mild 0.3-0.65) - a
// low value drew an identical, gentle sine-like curve every redraw, which
// read as a uniform wavy line rather than lightning. This range now
// matches the same order of magnitude the main bolt itself uses.
#define LIGHTNING_NEST_LINK_CHAOS_MIN 1.8f
#define LIGHTNING_NEST_LINK_CHAOS_MAX 3.2f
// How often the link arc is redrawn. Must be close to (not much shorter
// than) the non-mainBolt killTime used in FX_LightningArc (55ms) - drawing
// a fresh, independently-random link far more often than the old one dies
// leaves several different random shapes alive on screen at once, which is
// what actually caused the "spaghetti" look, not the chaos value alone.
#define LIGHTNING_NEST_LINK_INTERVAL 55
// How far the link's origin is allowed to wander around its branch point
// each redraw (perpendicular to the link direction).
#define LIGHTNING_NEST_LINK_ORIGIN_JITTER 6.0f
// A branch off a real lightning channel doesn't only ever leave from the
// exact tip - it leaves from various points along the channel. Picking the
// branch point as a random fraction along the main beam's own segment
// (hand -> beam tip) breaks the "everything meets at one exact end point"
// look, and works regardless of which direction the nest itself is in
// (floor, ceiling or a side wall).
#define LIGHTNING_NEST_BRANCH_FRAC_MIN 0.05f
#define LIGHTNING_NEST_BRANCH_FRAC_MAX 0.35f
// A nest slowly slides across the surface it's stuck to instead of sitting
// dead still for its whole life. Kept infrequent/slow since each crawl step
// spends from the same shared trace budget as everything else this frame.
#define LIGHTNING_NEST_CRAWL_SPEED 12.0f
#define LIGHTNING_NEST_CRAWL_INTERVAL 90
// Max radians the crawl direction drifts per step - gives an organic
// wander instead of a dead-straight line across the wall.
#define LIGHTNING_NEST_CRAWL_TURN 0.5f
// How far out/in (along the old normal) to re-probe for the surface after
// each crawl step, to stay stuck to it (and to pick up a new normal on a
// curved or angled surface) rather than drifting off into open air.
#define LIGHTNING_NEST_CRAWL_PROBE 24.0f

typedef struct {
	qboolean active;
	int owner;
	vec3_t pos;
	vec3_t normal;
	vec3_t crawlDir;
	int expireTime;
	int strikesLeft;
	int nextStrikeTime;
	int nextLinkTime;
	int nextCrawlTime;
} lightningNest_t;

static lightningNest_t lightningNests[LIGHTNING_MAX_NESTS];

static int FX_LightningCountOwnerNests(int owner) {
	int i, n = 0;
	for (i = 0; i < LIGHTNING_MAX_NESTS; i++)
		if (lightningNests[i].active && lightningNests[i].owner == owner)
			n++;
	return n;
}

// Nests die purely from their expireTime - running out of queued strikes
// doesn't end a nest (FX_LightningUpdateNests refills the queue instead),
// so this only needs to check the clock.
static void FX_LightningExpireNests(int owner) {
	int i;
	for (i = 0; i < LIGHTNING_MAX_NESTS; i++)
		if (lightningNests[i].active && lightningNests[i].owner == owner &&
			cg.time >= lightningNests[i].expireTime)
			lightningNests[i].active = qfalse;
}

// Finds a fresh nest point by probing random directions from the caster's
// hand out to LIGHTNING_NEST_SCATTER_RADIUS. Directions are still fully
// random in the hemisphere/floor/ceiling sense, but candidates pointing too
// far behind the caster's facing (see LIGHTNING_NEST_FORWARD_MIN_DOT) are
// rejected, so a nest never lands somewhere the player can't see it strike.
static qboolean FX_LightningScatterNest(vec3_t origin, vec3_t forward, int owner, vec3_t outPos, vec3_t outNormal) {
	int attempt;
	vec3_t dir, end;
	trace_t tr;

	for (attempt = 0; attempt < LIGHTNING_NEST_SCATTER_ATTEMPTS; attempt++) {
		dir[0] = Q_flrand(-1.0f, 1.0f);
		dir[1] = Q_flrand(-1.0f, 1.0f);
		dir[2] = Q_flrand(-1.0f, 1.0f);
		if (VectorNormalize(dir) < 0.1f)
			continue;
		if (DotProduct(dir, forward) < LIGHTNING_NEST_FORWARD_MIN_DOT)
			continue;

		VectorMA(origin, LIGHTNING_NEST_SCATTER_RADIUS, dir, end);
		if (!FX_LightningTrace(&tr, origin, end, owner) || !FX_LightningSurface(&tr))
			continue;
		if (tr.fraction * LIGHTNING_NEST_SCATTER_RADIUS < LIGHTNING_NEST_SCATTER_MIN_DIST)
			continue;

		VectorMA(tr.endpos, 2.0f, tr.plane.normal, outPos);
		VectorCopy(tr.plane.normal, outNormal);
		return qtrue;
	}
	return qfalse;
}

// Tries to add one new nest this cycle (subject to the per-owner cap).
// No "always keep at least one alive" guarantee here on purpose - unlike
// a persistent-contact system, it's fine for a caster to occasionally have
// zero nests active; they'll reappear as chance allows.
static void FX_LightningSpawnNest(vec3_t origin, vec3_t forward, int owner) {
	int slot, s;
	vec3_t pos, nrm;

	if (FX_LightningCountOwnerNests(owner) >= LIGHTNING_NESTS_PER_OWNER)
		return;
	if (Q_flrand(0.0f, 1.0f) > LIGHTNING_NEST_SPAWN_CHANCE)
		return;
	if (!FX_LightningScatterNest(origin, forward, owner, pos, nrm))
		return;

	slot = -1;
	for (s = 0; s < LIGHTNING_MAX_NESTS; s++)
		if (!lightningNests[s].active) { slot = s; break; }
	if (slot < 0)
		return;

	lightningNests[slot].active = qtrue;
	lightningNests[slot].owner = owner;
	VectorCopy(pos, lightningNests[slot].pos);
	VectorCopy(nrm, lightningNests[slot].normal);
	// Random initial crawl direction, tangent to the surface (built from two
	// perpendicular basis vectors so it can point any way across the wall,
	// not just along one axis).
	{
		vec3_t tangent, side;
		PerpendicularVector(tangent, nrm);
		CrossProduct(nrm, tangent, side);
		VectorScale(tangent, Q_flrand(-1.0f, 1.0f), lightningNests[slot].crawlDir);
		VectorMA(lightningNests[slot].crawlDir, Q_flrand(-1.0f, 1.0f), side, lightningNests[slot].crawlDir);
		if (VectorNormalize(lightningNests[slot].crawlDir) < 0.01f)
			VectorCopy(tangent, lightningNests[slot].crawlDir);
	}
	lightningNests[slot].expireTime = cg.time + (int)Q_flrand(LIGHTNING_NEST_MIN_LIFE, LIGHTNING_NEST_MAX_LIFE);
	lightningNests[slot].strikesLeft = LIGHTNING_NEST_MIN_STRIKES +
		(rand() % (LIGHTNING_NEST_MAX_STRIKES - LIGHTNING_NEST_MIN_STRIKES + 1));
	// first strike goes out almost immediately, subsequent ones use the gap
	lightningNests[slot].nextStrikeTime = cg.time;
	lightningNests[slot].nextLinkTime = cg.time;
	lightningNests[slot].nextCrawlTime = cg.time + LIGHTNING_NEST_CRAWL_INTERVAL;
}

// Slides a nest a small step across the surface it's stuck to, then
// re-probes along its (old) normal to snap back onto that surface - this
// is what lets it follow a curved wall or stop cleanly at an edge rather
// than floating off into open air. If the probe fails (walked off the
// edge of the surface, or the shared trace budget is exhausted this
// window), the crawl direction is reversed so it wanders back rather than
// getting stuck pointed at empty space.
static void FX_LightningCrawlNest(lightningNest_t *nest, int owner) {
	vec3_t tangent, side, candidate, probeStart, probeEnd;
	trace_t tr;
	float angle, c, s, dt, ds;

	if (lightningTraces >= LIGHTNING_TRACE_BUDGET)
		return; // no budget left this window - hold position, try again next tick

	// Drift the crawl direction a little each step instead of a dead-straight line.
	PerpendicularVector(tangent, nest->normal);
	CrossProduct(nest->normal, tangent, side);
	angle = Q_flrand(-LIGHTNING_NEST_CRAWL_TURN, LIGHTNING_NEST_CRAWL_TURN);
	c = cosf(angle);
	s = sinf(angle);
	dt = DotProduct(nest->crawlDir, tangent);
	ds = DotProduct(nest->crawlDir, side);
	VectorScale(tangent, dt * c - ds * s, nest->crawlDir);
	VectorMA(nest->crawlDir, dt * s + ds * c, side, nest->crawlDir);
	VectorNormalize(nest->crawlDir);

	VectorMA(nest->pos, LIGHTNING_NEST_CRAWL_SPEED * (LIGHTNING_NEST_CRAWL_INTERVAL * 0.001f),
		nest->crawlDir, candidate);

	VectorMA(candidate, LIGHTNING_NEST_CRAWL_PROBE, nest->normal, probeStart);
	VectorMA(candidate, -LIGHTNING_NEST_CRAWL_PROBE, nest->normal, probeEnd);
	if (!FX_LightningTrace(&tr, probeStart, probeEnd, owner) || !FX_LightningSurface(&tr)) {
		VectorScale(nest->crawlDir, -1.0f, nest->crawlDir);
		return;
	}

	VectorMA(tr.endpos, 2.0f, tr.plane.normal, nest->pos);
	VectorCopy(tr.plane.normal, nest->normal);
}

// Real DEMP2 wall-impact visual, matching the WP_DEMP2 case in
// CG_MissileHitWall (cg_weapons.c): normal (non-alt) DEMP2 missiles call
// FX_DEMP2_HitWall directly rather than going through FX_PlayEffectID -
// it's a normal function, already declared via fx_local.h (same header
// this file includes), not an effect ID. Using that exact call (instead
// of a custom sprite pair) makes a struck nest read as the regular DEMP2
// bolt impact rather than the bigger alt-fire detonation. This plays the
// scaled-down variant (demp2/wall_impact_small.efx), registered alongside
// the full-size one in CG_RegisterWeapon, so base assets stay untouched.
static void FX_LightningNestVisual(vec3_t origin, vec3_t normal) {
	trap->FX_PlayEffectID( cgs.effects.demp2WallImpactEffectSmall, origin, normal, -1, -1, qfalse );
}

// Impact sound for a nest getting struck - reuses cgs.media.crackleSound,
// a real field already used for electrical/disintegration crackle in
// cg_ents.c, so it compiles safely and fits the electric theme. Budgeted
// per LIGHTNING_SOUND_INTERVAL so several nests striking in quick succession
// don't stack into a wall of noise.
static void FX_LightningNestImpactSound(vec3_t pos) {
	if (cg.time < lightningSoundBudgetTime ||
		cg.time - lightningSoundBudgetTime >= LIGHTNING_SOUND_INTERVAL) {
		lightningSoundBudgetTime = cg.time;
		lightningSounds = 0;
	}
	if (lightningSounds >= LIGHTNING_SOUND_BUDGET)
		return;
	lightningSounds++;
	trap->S_StartSound(pos, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.crackleSound);
}

// Picks a point along the main beam's own segment (hand -> beam tip) to
// branch off from. Using a random fraction of the whole segment - rather
// than always the tip, or always a fixed offset from wherever the beam's
// own hit point is - means this works the same whether the nest ends up
// on a wall, the floor or the ceiling.
static void FX_LightningBranchBase(vec3_t beamStart, vec3_t beamEnd, vec3_t outBase) {
	vec3_t seg;
	VectorSubtract(beamEnd, beamStart, seg);
	VectorMA(beamStart, Q_flrand(LIGHTNING_NEST_BRANCH_FRAC_MIN, LIGHTNING_NEST_BRANCH_FRAC_MAX), seg, outBase);
}

// Draws ONE jittered arc from a point along the main beam into the nest.
// Real lightning striking a target repeatedly doesn't fire every strike at
// once - it flickers, pauses, flickers again. That per-strike pacing lives
// in FX_LightningUpdateNests below (via nextStrikeTime); this function just
// draws a single strike when called. This is the stronger periodic "hit",
// separate from the thin always-on link arc drawn every update.
static void FX_LightningStrikeNest(vec3_t beamStart, vec3_t beamEnd, lightningNest_t *nest) {
	vec3_t tangent, side, jitterStart, jitterEnd, linkDir, branchBase;

	FX_LightningBranchBase(beamStart, beamEnd, branchBase);

	// Jitter basis is built from the actual branchBase->nest line, so the
	// offset stays perpendicular to the two points it's really connecting,
	// whichever direction that line happens to point in.
	VectorSubtract(nest->pos, branchBase, linkDir);
	VectorNormalize(linkDir);
	PerpendicularVector(tangent, linkDir);
	CrossProduct(linkDir, tangent, side);

	// Small manual jitter - the electricity renderer's own chaos parameter
	// already adds waviness along the arc, and stacking a wide manual
	// offset on top of that is what made bolts look like curly "spaghetti"
	// instead of a taut strike.
	VectorMA(branchBase, Q_flrand(-3.0f, 3.0f), tangent, jitterStart);
	VectorMA(jitterStart, Q_flrand(-3.0f, 3.0f), side, jitterStart);

	VectorMA(nest->pos, Q_flrand(-2.0f, 2.0f), tangent, jitterEnd);
	VectorMA(jitterEnd, Q_flrand(-2.0f, 2.0f), side, jitterEnd);
	VectorMA(jitterEnd, 1.0f, nest->normal, jitterEnd);

	// Strike chaos bumped to match the link arc's range (was a near-straight
	// 0.12-0.22) - strikes fire far more often than the link redraws, so a
	// low chaos here was the bigger reason bolts still read as too linear.
	FX_LightningArc(jitterStart, jitterEnd, Q_flrand(2.0f, 3.0f),
		Q_flrand(LIGHTNING_NEST_LINK_CHAOS_MIN, LIGHTNING_NEST_LINK_CHAOS_MAX), qfalse);
	FX_LightningNestImpactSound(nest->pos);
	FX_LightningNestVisual(nest->pos, nest->normal);
}

// Called every update - not just while the aim beam currently hits a
// surface - so nests keep animating (links redrawn, strikes fired,
// expiry checked) even during a frame where you're aiming at open air or
// another player. beamStart/beamEnd describe the main beam's own segment
// this update, used purely to pick each branch's origin point (see
// FX_LightningBranchBase); they don't need to represent a surface hit.
static void FX_LightningUpdateNests(vec3_t beamStart, vec3_t beamEnd, int owner) {
	int slot;
	for (slot = 0; slot < LIGHTNING_MAX_NESTS; slot++) {
		lightningNest_t *nest = &lightningNests[slot];
		if (!nest->active || nest->owner != owner)
			continue;

		if (cg.time >= nest->nextCrawlTime) {
			nest->nextCrawlTime = cg.time + LIGHTNING_NEST_CRAWL_INTERVAL;
			FX_LightningCrawlNest(nest, owner);
		}

		// Connecting arc: redrawn on its own cadence (LIGHTNING_NEST_LINK_INTERVAL,
		// matched to its killTime) rather than on every call. Redrawing far more
		// often than each drawn arc's own lifetime meant several independently-
		// random copies were alive on screen simultaneously, which is what
		// actually produced the tangled "spaghetti" look - not the chaos value.
		if (cg.time >= nest->nextLinkTime) {
			vec3_t linkDir, linkTangent, linkSide, linkOrigin, branchBase;
			nest->nextLinkTime = cg.time + LIGHTNING_NEST_LINK_INTERVAL;

			FX_LightningBranchBase(beamStart, beamEnd, branchBase);

			VectorSubtract(nest->pos, branchBase, linkDir);
			VectorNormalize(linkDir);
			PerpendicularVector(linkTangent, linkDir);
			CrossProduct(linkDir, linkTangent, linkSide);
			VectorMA(branchBase, Q_flrand(-LIGHTNING_NEST_LINK_ORIGIN_JITTER, LIGHTNING_NEST_LINK_ORIGIN_JITTER), linkTangent, linkOrigin);
			VectorMA(linkOrigin, Q_flrand(-LIGHTNING_NEST_LINK_ORIGIN_JITTER, LIGHTNING_NEST_LINK_ORIGIN_JITTER), linkSide, linkOrigin);
			FX_LightningArc(linkOrigin, nest->pos, LIGHTNING_NEST_LINK_WIDTH,
				Q_flrand(LIGHTNING_NEST_LINK_CHAOS_MIN, LIGHTNING_NEST_LINK_CHAOS_MAX), qfalse);
		}

		if (nest->strikesLeft <= 0 || cg.time < nest->nextStrikeTime)
			continue;

		FX_LightningStrikeNest(beamStart, beamEnd, nest);
		nest->strikesLeft--;
		if (nest->strikesLeft <= 0) {
			// Refill instead of letting the nest go idle - it now only
			// dies from expireTime, not from running out of strikes.
			nest->strikesLeft = LIGHTNING_NEST_MIN_STRIKES +
				(rand() % (LIGHTNING_NEST_MAX_STRIKES - LIGHTNING_NEST_MIN_STRIKES + 1));
		}
		nest->nextStrikeTime = cg.time +
			LIGHTNING_NEST_STRIKE_MIN_GAP +
			(rand() % (LIGHTNING_NEST_STRIKE_MAX_GAP - LIGHTNING_NEST_STRIKE_MIN_GAP + 1));
	}
}
// ------------------------------------------------------------------------

// Emit a stock-like dense spray independently of the slower surface response.
// If the shared budget is exhausted, the caller falls back to vanilla lightning.
qboolean FX_ForceLightningEnvironment(centity_t *cent, vec3_t origin, matrix3_t axis, qboolean wide) {
	int ray, rays, surfaceRay = -1;
	float phase;
	vec3_t end, beamEnd;
	trace_t hits[4];
	vec3_t directions[4];
	qboolean valid[4];

	if (!cg_lightningEnvironment.integer)
		return qfalse;
	if (cent->lightningEnvironmentTime > cg.time &&
		cent->lightningEnvironmentTime <= cg.time + LIGHTNING_EMIT_INTERVAL)
		return qtrue;
	if (cg.time < lightningBudgetTime || cg.time - lightningBudgetTime >= LIGHTNING_INTERVAL) {
		lightningBudgetTime = cg.time;
		lightningTraces = lightningEffects = 0;
	}
	rays = wide ? 2 + ((cg.time / 10 + cent->currentState.number) % 3) : 1;
	// Reserve the entire hand spray before drawing anything. Falling back
	// midway would double up custom and stock bolts in the same frame.
	if (lightningTraces + rays > LIGHTNING_TRACE_BUDGET ||
		lightningEffects + rays + 1 > LIGHTNING_EFFECT_BUDGET)
		return qfalse;
	cent->lightningEnvironmentTime = cg.time + LIGHTNING_EMIT_INTERVAL;
	phase = cg.time * 0.017f + cent->currentState.number * 2.39996f;
	for (ray = 0; ray < rays; ray++) {
		VectorCopy(axis[0], directions[ray]);
		if (wide) {
			float spread = (ray / (float)(rays - 1) - 0.5f) * 1.4f;
			VectorMA(directions[ray], spread + sinf(phase + ray * 2.4f) * 0.16f, axis[1], directions[ray]);
			VectorMA(directions[ray], sinf(phase * 0.7f + ray) * 0.08f, axis[2], directions[ray]);
		}
		VectorNormalize(directions[ray]);
		VectorMA(origin, wide ? 512.0f : 2048.0f, directions[ray], end);
		valid[ray] = FX_LightningTrace(&hits[ray], origin, end, cent->currentState.number);
	}
	for (ray = 0; ray < rays; ray++) {
		if (!valid[ray])
			continue;
		VectorCopy(hits[ray].endpos, end);
		if (FX_LightningSurface(&hits[ray])) {
			VectorMA(end, 2.0f, hits[ray].plane.normal, end);
			// At most one impact is used as the beam's "tip" reference below.
			if (surfaceRay < 0 || hits[ray].fraction < hits[surfaceRay].fraction)
				surfaceRay = ray;
		}
		FX_LightningArc(origin, end, 5.0f + sinf(phase + ray) * 2.0f,
			1.4f + sinf(phase * 1.3f + ray) * 0.6f, qtrue);
	}
	// Keep the small hand flash which was absent in the first prototype.
	FX_LightningFlash(origin, 18.0f);

	// A representative tip for the main beam, used only to pick branch
	// points along it (FX_LightningBranchBase) - falls back to whatever
	// ray 0 actually reached (surface or not) so nests keep animating even
	// when the beam itself isn't currently hitting anything solid.
	if (surfaceRay >= 0)
		VectorMA(hits[surfaceRay].endpos, 2.0f, hits[surfaceRay].plane.normal, beamEnd);
	else if (valid[0])
		VectorCopy(hits[0].endpos, beamEnd);
	else
		VectorCopy(origin, beamEnd);

	{
		int owner = cent->currentState.number;

		// Nests are updated/expired every call regardless of whether the
		// beam currently hits a surface, so they don't freeze mid-life
		// just because you glanced away from a wall for a moment.
		FX_LightningExpireNests(owner);
		FX_LightningUpdateNests(origin, beamEnd, owner);

		if (cent->lightningSurfaceTime <= cg.time) {
			cent->lightningSurfaceTime = cg.time + LIGHTNING_INTERVAL;
			// Scatters around the caster's hand in random 3D directions,
			// biased away from directly behind (see LIGHTNING_NEST_FORWARD_MIN_DOT),
			// so this can land on a wall, the floor or the ceiling roughly
			// in front of/around the player, never somewhere unseen behind them.
			FX_LightningSpawnNest(origin, axis[0], owner);
		}
	}

	return qtrue;
}

/*
-------------------------
FX_ForceDrained
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_ForceDrained(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap->FX_PlayEffectID(cgs.effects.forceDrained, origin, dir, -1, -1, qfalse);
}