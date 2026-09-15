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
static int lightningBudgetTime;
static int lightningTraces;
static int lightningEffects;

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
	arc.alpha2 = mainBolt ? 1.0f : 0.0f;
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

// Emit a stock-like dense spray independently of the slower surface response.
// If the shared budget is exhausted, the caller falls back to vanilla lightning.
qboolean FX_ForceLightningEnvironment(centity_t *cent, vec3_t origin, matrix3_t axis, qboolean wide) {
	int ray, rays, branch, hop, surfaceRay = -1;
	float phase;
	vec3_t end, direction, contact, tangent, side, start;
	trace_t hits[4], bounce;
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
			// At most one impact produces surface branches per update.
			if (surfaceRay < 0 || hits[ray].fraction < hits[surfaceRay].fraction)
				surfaceRay = ray;
		}
		FX_LightningArc(origin, end, 5.0f + sinf(phase + ray) * 2.0f,
			1.4f + sinf(phase * 1.3f + ray) * 0.6f, qtrue);
	}
	// Keep the small hand flash which was absent in the first prototype.
	FX_LightningFlash(origin, 18.0f);

	if (surfaceRay < 0 || (cent->lightningSurfaceTime > cg.time &&
		cent->lightningSurfaceTime <= cg.time + LIGHTNING_INTERVAL))
		return qtrue;
	cent->lightningSurfaceTime = cg.time + LIGHTNING_INTERVAL;
	phase = cg.time * 0.004f + cent->currentState.number * 2.39996f;
	VectorCopy(directions[surfaceRay], direction);
	VectorMA(hits[surfaceRay].endpos, 2.0f, hits[surfaceRay].plane.normal, contact);
	FX_LightningFlash(contact, 4.0f);
	for (branch = 0; branch < 2; branch++) {
		vec3_t normal;
		VectorCopy(hits[surfaceRay].plane.normal, normal);
		VectorMA(direction, -DotProduct(direction, normal), normal, tangent);
		if (VectorNormalize(tangent) < 0.1f)
			PerpendicularVector(tangent, normal);
		CrossProduct(normal, tangent, side);
		VectorMA(tangent, sinf(phase) * 0.5f, side, tangent);
		if (branch)
			VectorScale(tangent, -1.0f, tangent);
		VectorMA(tangent, 0.03f, normal, tangent);
		VectorNormalize(tangent);
		VectorCopy(contact, start);
		for (hop = 0; hop < 2; hop++) {
			VectorMA(start, 80.0f / (hop + 1), tangent, end);
			if (!FX_LightningTrace(&bounce, start, end, cent->currentState.number))
				break;
			VectorCopy(bounce.endpos, end);
			if (FX_LightningSurface(&bounce))
				VectorMA(end, 2.0f, bounce.plane.normal, end);
			FX_LightningArc(start, end, 1.5f / (hop + 1), 0.2f, qfalse);
			if (bounce.fraction < 1.0f) {
				if (!FX_LightningSurface(&bounce))
					break;
				VectorMA(tangent, -2.0f * DotProduct(tangent, bounce.plane.normal),
					bounce.plane.normal, tangent);
				VectorNormalize(tangent);
			}
			VectorCopy(end, start);
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

