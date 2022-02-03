#include "aimbot.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "autowall.hpp"
#include "math.hpp"
#include "displacement.hpp"
#include "lag_compensation.hpp"
#include "anti_aimbot.hpp"
#include "angle_resolver.hpp"
#include "visuals.hpp"
#include "rmenu.hpp"
#include "movement.hpp"
#include "prediction.hpp"

#include "thread/threading.h"
#include "thread/shared_mutex.h"

#include <thread>

static constexpr auto total_seeds = 256;

struct TargetListing_t
{
	TargetListing_t(C_BasePlayer* ent)
	{
		entity = ent;

		hp = entity->m_iHealth();
		distance = cheat::main::local()->m_vecOrigin().Distance(entity->get_abs_origin());
		Vector lol;
		if (Drawing::WorldToScreen(entity->get_abs_origin(), lol)) 
			fov = Vector(cheat::game::screen_size.x * 0.5f, cheat::game::screen_size.y * 0.5f, 0).DistanceSquared(lol);
		idx = entity->entindex();
	}

	TargetListing_t()
	{
	}

	int hp;
	int idx;
	int distance;
	int fov;
	bool strange;
	C_BasePlayer* entity;
};

std::vector<TargetListing_t> m_entities;

Vector c_aimbot::get_hitbox(C_BasePlayer* ent, int ihitbox)
{
	if (ihitbox < 0 || ihitbox > 19) return Vector::Zero;

	if (!ent) return Vector::Zero;

	const auto model = ent->GetModel();

	if (!model)
		return Vector::Zero;

	auto pStudioHdr = Source::m_pModelInfo->GetStudioModel(model);

	if (!pStudioHdr)
		return Vector::Zero;

	auto hitbox = pStudioHdr->pHitbox(ihitbox, 0);

	if (!hitbox)
		return Vector::Zero;

	Vector min, max;
	Math::VectorTransform(hitbox->bbmin, ent->m_CachedBoneData().Base()[hitbox->bone], min);
	Math::VectorTransform(hitbox->bbmax, ent->m_CachedBoneData().Base()[hitbox->bone], max);

	auto center = (min + max) / 2.f;

	return center;
}

void c_aimbot::get_hitbox_data(C_Hitbox* rtn, C_BasePlayer* ent, int ihitbox, matrix3x4_t* matrix)
{
	if (ihitbox < 0 || ihitbox > 19) return;

	if (!ent) return;

	const auto model = ent->GetModel();

	if (!model)
		return;

	auto pStudioHdr = Source::m_pModelInfo->GetStudioModel(model);

	if (!pStudioHdr)
		return;

	auto hitbox = pStudioHdr->pHitbox(ihitbox, 0);

	if (!hitbox)
		return;

	const auto is_capsule = hitbox->radius != -1.f;

	Vector min, max;
	if (is_capsule) {
		Math::VectorTransform(hitbox->bbmin, matrix[hitbox->bone], min);
		Math::VectorTransform(hitbox->bbmax, matrix[hitbox->bone], max);
	}
	else
	{
		Math::VectorTransform(Math::VectorRotate(hitbox->bbmin, hitbox->rotation), matrix[hitbox->bone], min);
		Math::VectorTransform(Math::VectorRotate(hitbox->bbmax, hitbox->rotation), matrix[hitbox->bone], max);
	}

	rtn->hitboxID = ihitbox;
	rtn->isOBB = !is_capsule;
	rtn->radius = hitbox->radius;
	rtn->mins = min;
	rtn->maxs = max;
	rtn->bone = hitbox->bone;
}

bool c_aimbot::safe_point(C_BasePlayer* entity,Vector eye_pos, Vector aim_point, int hitboxIdx, bool shot)
{
	if (shot)
		return true;

	auto resolve_info = &cheat::features::aaa.player_resolver_records[entity->entindex() - 1];

	auto baimkey_shit = (cheat::Cvars.RageBot_SafePointsBaimKey.GetValue() && (cheat::game::pressed_keys[(int)cheat::Cvars.RageBot_BaimKey.GetValue()] && cheat::Cvars.RageBot_BaimKey.GetValue() > 0.f));
	auto can_safepoint = (resolve_info->did_shot_this_tick && cheat::Cvars.RageBot_SafePoints.GetValue() == 1) || cheat::Cvars.RageBot_SafePoints.GetValue() == 2;

	//if (!can_safepoint && !(baimkey_shit && cheat::Cvars.RageBot_SafePointsBaimKey.GetValue() != 0.f))
	//	return true;

	if (!baimkey_shit || cheat::Cvars.RageBot_SafePointsBaimKey.GetValue() == 0.f)
	{
		auto can_safepoint = (resolve_info->did_shot_this_tick && cheat::Cvars.RageBot_SafePoints.GetValue() == 1) || cheat::Cvars.RageBot_SafePoints.GetValue() == 2;

		if (!can_safepoint)
			return true;
	}
	/*const auto is_colliding = [entity, hitboxIdx, maxdamage](Vector start, Vector end, C_Tickrecord *rec, C_Tickrecord* orgtc) -> bool
	{
		Ray_t ray;
		trace_t tr;
		ray.Init(start, end);

		cheat::features::lagcomp.apply_record_data(entity, rec);
		Source::m_pEngineTrace->ClipRayToEntity(ray, MASK_SHOT | CONTENTS_GRATE, entity, &tr);
		const auto walldamage = cheat::features::autowall.CanHit(start, end, cheat::main::local(), entity, hitboxIdx);
		cheat::features::lagcomp.apply_record_data(entity, orgtc);

		return (tr.m_pEnt == entity && walldamage > 1.f);
	};*/

	const auto angle = Math::CalcAngle(eye_pos, aim_point);
	Vector forward;
	Math::AngleVectors(angle, &forward);
	auto end = eye_pos + forward * 8092.f;

	auto cur_rec = &cheat::features::lagcomp.records[entity->entindex() - 1].m_Current;

	if (!cur_rec->data_filled)
		return false;

	C_Hitbox box1; get_hitbox_data(&box1, entity, hitboxIdx, cur_rec->leftmatrixes);
	C_Hitbox box2; get_hitbox_data(&box2, entity, hitboxIdx, cur_rec->rightmatrixes);
	C_Hitbox box3; get_hitbox_data(&box3, entity, hitboxIdx, cur_rec->matrixes);

	auto pizdets = box1.isOBB || box2.isOBB || box3.isOBB;

	if (pizdets) {
		Math::VectorITransform(eye_pos, cur_rec->leftmatrixes[box1.bone], box1.mins);
		Math::VectorIRotate(end, cur_rec->leftmatrixes[box1.bone], box1.maxs);

		Math::VectorITransform(eye_pos, cur_rec->rightmatrixes[box2.bone], box2.mins);
		Math::VectorIRotate(end, cur_rec->rightmatrixes[box2.bone], box2.maxs);

		Math::VectorITransform(eye_pos, cur_rec->matrixes[box3.bone], box3.mins);
		Math::VectorIRotate(end, cur_rec->matrixes[box3.bone], box3.maxs);
	}

	//C_Tickrecord rec;
	//cheat::features::lagcomp.store_record_data(entity, &rec);

	auto hits = 0;

	if (pizdets ? Math::IntersectBB(eye_pos, end, box1.mins, box1.maxs) : Math::Intersect(eye_pos, end, box1.mins, box1.maxs, box1.radius))//if (is_colliding(eye_pos, end_point, &resolve_info->leftrec, &rec))
		++hits;
	if (pizdets ? Math::IntersectBB(eye_pos, end, box2.mins, box2.maxs) : Math::Intersect(eye_pos, end, box2.mins, box2.maxs, box2.radius))//if (is_colliding(eye_pos, end_point, &resolve_info->rightrec, &rec))
		++hits;
	if (pizdets ? Math::IntersectBB(eye_pos, end, box3.mins, box3.maxs) : Math::Intersect(eye_pos, end, box3.mins, box3.maxs, box3.radius))//if (is_colliding(eye_pos, end_point, &resolve_info->norec, &rec))
		++hits;

	/*bool ok = false;
	if (box2.isOBB)
	{
		if (Math::IntersectionBoundingBox(eye_pos, end_point, box1.mins, box1.maxs))
			hits = 1;
		if (Math::IntersectionBoundingBox(eye_pos, end_point, box2.mins, box2.maxs))
			++hits;
		if (Math::IntersectionBoundingBox(eye_pos, end_point, box3.mins, box3.maxs))
			ok = true;
	}
	else
	{
		if (Math::IntersectSegmentCapsule(eye_pos, end_point, box1.mins, box1.maxs, box1.radius))
			hits = 1;
		if (Math::IntersectSegmentCapsule(eye_pos, end_point, box2.mins, box2.maxs, box2.radius))
			++hits;
		if (Math::IntersectSegmentCapsule(eye_pos, end_point, box3.mins, box3.maxs, box3.radius))
			ok = true;
	}

	if (ok)
		++hits;*/

	return (hits >= 3);
}

bool c_aimbot::safe_static_point(C_BasePlayer* entity, Vector eye_pos, Vector aim_point, int hitboxIdx, bool shot)
{
	auto resolve_info = &cheat::features::aaa.player_resolver_records[entity->entindex() - 1];

	if (cheat::Cvars.Ragebot_safe_pointselection.GetValue() == 0.f || resolve_info->resolving_method == 0)
		return true;

	const auto is_colliding = [entity, hitboxIdx](Vector start, Vector end, C_Hitbox box) -> bool
	{
		//cheat::features::lagcomp.apply_record_data(entity, rec);
		const auto is_intersecting = box.isOBB ? Math::IntersectBB(start, end, box.mins, box.maxs) : Math::Intersect(start, end, box.mins, box.maxs, box.radius);
		//Source::m_pEngineTrace->ClipRayToEntity(ray, MASK_SHOT | CONTENTS_GRATE, entity, &tr);
		//const auto walldamage = cheat::features::autowall.CanHit(start, end, cheat::main::local(), entity, hitboxIdx);
		//cheat::features::lagcomp.apply_record_data(entity, orgtc);

		return is_intersecting;
	};

	/*Vector angles, direction;
	auto v6 = aim_point - eye_pos;
	Math::VectorAngles(v6, angles);
	Math::AngleVectors(angles, &direction);
	direction.Normalize();
	const auto end_point = direction * 8092.f + eye_pos;*/
	const auto angle = Math::CalcAngle(eye_pos, aim_point);
	Vector forward;
	Math::AngleVectors(angle, &forward);
	auto end = eye_pos + forward * 8092.f;

	//Source::m_pDebugOverlay->AddLineOverlay(eye_pos, end, 255, 0, 0, false, Source::m_pGlobalVars->interval_per_tick * 2.f);

	auto cur_rec = &cheat::features::lagcomp.records[entity->entindex() - 1].m_Current;

	if (!cur_rec->data_filled)
		return false;

	C_Hitbox box1; get_hitbox_data(&box1, entity, hitboxIdx, cur_rec->leftlmatrixes);
	C_Hitbox box2; get_hitbox_data(&box2, entity, hitboxIdx, cur_rec->rightlmatrixes);

	if (box1.isOBB || box2.isOBB) {
		Math::VectorITransform(eye_pos, cur_rec->leftlmatrixes[box1.bone], box1.mins);
		Math::VectorIRotate(end, cur_rec->leftlmatrixes[box1.bone], box1.maxs);

		Math::VectorITransform(eye_pos, cur_rec->rightlmatrixes[box2.bone], box2.mins);
		Math::VectorIRotate(end, cur_rec->rightlmatrixes[box2.bone], box2.maxs);
	}

	//C_Tickrecord rec;
	//cheat::features::lagcomp.store_record_data(entity, &rec);

	if (is_colliding(eye_pos, end, box2))
		return true;
	if (is_colliding(eye_pos, end, box1))//is_colliding(eye_pos, end_point, &resolve_info->leftlrec, &rec))
		return true;

	return false;
}

void c_aimbot::draw_capsule(C_BasePlayer* ent, int ihitbox)
{
	if (ihitbox < 0 || ihitbox > 19) return;

	if (!ent) return;

	const auto model = ent->GetModel();

	if (!model)
		return;

	auto pStudioHdr = Source::m_pModelInfo->GetStudioModel(model);

	if (!pStudioHdr)
		return;

	auto hitbox = pStudioHdr->pHitbox(ihitbox, 0);

	if (!hitbox)
		return;

	//Vector min, max;
	//Math::VectorTransform(hitbox->bbmin, mat[hitbox->bone], min);
	//Math::VectorTransform(hitbox->bbmax, mat[hitbox->bone], max);

	//cheat::features::visuals.draw_capsule(hitbox->bbmin, hitbox->bbmax, hitbox->radius, mat[hitbox->bone], Color::Red(200));
}

Vector c_aimbot::get_hitbox(C_BasePlayer* ent, int ihitbox, matrix3x4_t mat[])
{
	if (ihitbox < 0 || ihitbox > 19) return Vector::Zero;

	if (!ent) return Vector::Zero;

	/*matrix3x4_t mat[128];

	ent->force_bone_rebuild();

	const auto _backup = ent->get_animation_state()->m_bOnGround;
	ent->get_animation_state()->m_bOnGround = 0;
	if (!ent->SetupBones(mat, 128, 0x00000100, ent->m_flSimulationTime())) {
	ent->get_animation_state()->m_bOnGround = _backup;
	return Vector::Zero;
	}
	ent->get_animation_state()->m_bOnGround = _backup;*/

	if (!ent->GetClientRenderable())
		return Vector::Zero;

	const auto model = ent->GetModel();

	if (!model)
		return Vector::Zero;

	auto pStudioHdr = Source::m_pModelInfo->GetStudioModel(model);

	if (!pStudioHdr)
		return Vector::Zero;

	auto hitbox = pStudioHdr->pHitbox(ihitbox, ent->m_nHitboxSet());

	if (!hitbox)
		return Vector::Zero;

	if (hitbox->bone > 128 || hitbox->bone < 0)
		return Vector::Zero;

	Vector min, max;
	Math::VectorTransform(hitbox->bbmin, mat[hitbox->bone], min);
	Math::VectorTransform(hitbox->bbmax, mat[hitbox->bone], max);

	auto center = (min + max) / 2.f;

	return center;
}

bool c_aimbot::knifebot_work(CUserCmd* cmd, bool&send_packet)
{
	auto return_value = false;

	m_nBestIndex = -1;

	auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

	if (!local_weapon || !local_weapon->is_knife())
		return return_value;

	if (!knifebot_target() || pBestEntity == nullptr) {
		cheat::features::lagcomp.finish_position_adjustment();
		return return_value;
	}

	Vector tempBestAngle = get_hitbox(pBestEntity, HITBOX_PELVIS);
	auto entAng = Math::CalcAngle(cheat::main::local()->GetEyePosition(), tempBestAngle);

	if (!lastAttacked)
	{
		bool stab = false;
		int health = pBestEntity->m_iHealth();
		if (pBestEntity->m_ArmorValue())
		{
			if (health <= 55 &&
				health > 34)
				stab = true;
		}
		else
		{
			if (health <= 65 &&
				health > 40)
				stab = true;
		}

		stab = stab && m_nBestDist <= 32.0f;

		if (cheat::features::aaa.compare_delta(pBestEntity->m_angEyeAngles().y, Math::CalcAngle(cheat::main::local()->m_vecOrigin(), pBestEntity->get_abs_origin()).y, 75) + 180.f) {
			if (cheat::features::lagcomp.records[pBestEntity->entindex() - 1].backtrack_ticks > 0) {
				if (m_nBestDist < 32.0f)
					stab = true;
				else
				{
					cheat::features::lagcomp.finish_position_adjustment();
					return return_value;
				}
			}
			else
			{
				if (m_nBestDist < 32.0f)
					stab = true;
				else
					stab = false;
			}
		}

		if (m_nBestDist < 32.0f && health >= 90)
			stab = true;

		if (stab)
			cmd->buttons |= IN_ATTACK2;
		else
			cmd->buttons |= IN_ATTACK;

		if (m_nBestDist > 48.0f)
		{
			cheat::features::lagcomp.finish_position_adjustment();
			return return_value;
		}

		float next_shot = (stab ? local_weapon->m_flNextPrimaryAttack() : local_weapon->m_flNextPrimaryAttack()) - Source::m_pGlobalVars->curtime;

		if (!(next_shot > 0))
		{
			cmd->viewangles = entAng;
			cmd->tick_count = cheat::features::lagcomp.records[pBestEntity->entindex() - 1].tick_count;
			send_packet = false;
			return_value = true;
		}
	}

	cheat::features::lagcomp.finish_position_adjustment();

	lastAttacked = !lastAttacked;

	return return_value;
}

bool c_aimbot::knifebot_target()
{
	static auto is_visible = [](C_BasePlayer* thisptr, Vector& Start, Vector& End) -> bool
	{
		if (!thisptr) return NULL;

		CGameTrace tr;
		Ray_t ray;
		static CTraceFilter traceFilter;
		traceFilter.pSkip = cheat::main::local();

		ray.Init(Start, End);

		Source::m_pEngineTrace->TraceRay(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &traceFilter, &tr);
		cheat::features::autowall.FixTraceRay(Start, End, &tr, thisptr);

		return (tr.m_pEnt == thisptr || tr.fraction > 0.99f);
	};

	cheat::features::lagcomp.start_position_adjustment();

	float bestDist = 75;
	for (int i = 1; i < 64; i++)
	{
		auto pBaseEntity = Source::m_pEntList->GetClientEntity(i);
		if (!pBaseEntity)
			continue;
		if (!pBaseEntity->GetClientClass())
			continue;
		if (pBaseEntity->GetClientClass()->m_ClassID != class_ids::CCSPlayer)
			continue;
		if (pBaseEntity == cheat::main::local())
			continue;
		if (pBaseEntity->m_bGunGameImmunity())
			continue;
		if (pBaseEntity->IsDead())
			continue;
		if (pBaseEntity->IsDormant())
			continue;
		if (pBaseEntity->m_iTeamNum() == cheat::main::local()->m_iTeamNum())
			continue;

		Vector localPos = cheat::main::local()->m_vecOrigin() + Vector(0, 0, 60);
		Vector basePos = pBaseEntity->get_abs_origin() + Vector(0, 0, 60);

		if (!is_visible(pBaseEntity, localPos, basePos))
			continue;

		float curDist = localPos.Distance(basePos);
		if (curDist < bestDist)
		{
			bestDist = curDist;
			m_nBestIndex = i;
			pBestEntity = pBaseEntity;
		}
	}

	m_nBestDist = bestDist;
	return m_nBestIndex != -1;
}

float c_aimbot::can_hit(int hitbox, C_BasePlayer* Entity, Vector position, matrix3x4_t mx[], bool check_center, bool predict)
{
	static auto is_visible = [](C_BasePlayer* thisptr, Vector& Start, Vector& End) -> bool
	{
		if (!thisptr) return NULL;

		CGameTrace tr;
		Ray_t ray;
		static CTraceFilter traceFilter;
		traceFilter.pSkip = cheat::main::local();

		ray.Init(Start, End);

		Source::m_pEngineTrace->TraceRay(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &traceFilter, &tr);
		cheat::features::autowall.FixTraceRay(Start, End, &tr, thisptr);

		return (tr.m_pEnt == thisptr || tr.fraction > 0.99f);
	};

	auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

	if (!local_weapon->GetCSWeaponData())
		return 0;

	/*static auto is_visible = [](C_BasePlayer* thisptr, Vector& Start, Vector& End) -> bool
	{
	if (!thisptr) return NULL;

	CGameTrace tr;
	Ray_t ray;
	static CTraceFilter traceFilter;
	traceFilter.pSkip = cheat::main::local();

	ray.Init(Start, End);

	Source::m_pEngineTrace->TraceRay(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &traceFilter, &tr);
	cheat::features::aimbot.ClipTraceToPlayers(Start, End, MASK_SHOT_HULL | CONTENTS_HITBOX, &traceFilter, &tr);

	return (tr.m_pEnt == thisptr || tr.fraction > 0.99f);
	};*/

	auto eyepos = cheat::main::local()->GetEyePosition();

	//if (predict)
	//	eyepos += (cheat::main::local()->m_vecVelocity() * (Source::m_pGlobalVars->interval_per_tick * 2));

	auto cdmg = cheat::features::autowall.CanHit(eyepos, position, cheat::main::local(), Entity, hitbox);

	auto nob = (Entity->m_iHealth() < cheat::Cvars.RageBot_MinDamage.GetValue() && cheat::Cvars.RageBot_ScaledmgOnHp.GetValue() ? Entity->m_iHealth() : ((cheat::Cvars.RageBot_MinDmgKey.GetValue() && cheat::game::pressed_keys[(int)cheat::Cvars.RageBot_MinDmgKey.GetValue()]) ? cheat::Cvars.RageBot_MinDamage.GetValue() : cheat::Cvars.RageBot_MinDmgKey_val.GetValue()));

	//if (cdmg >= nob)
	//	return cdmg;

	if (check_center) {
		if (cdmg >= nob)
			return cdmg;

		return 0.f;
	}

	static std::vector<Vector> points;

	if (Entity->get_multipoints(hitbox, points, mx) && !points.empty())
	{
		cheat::main::points[Entity->entindex() - 1][hitbox] = points;

		//if (cdmg >= nob)
		//	return cdmg;

		if (hitbox != 11 && hitbox != 12) 
		{

			//auto dmg = cheat::features::autowall.CanHit(eyepos, points.at(1), cheat::main::local(), Entity, hitbox);
			//auto dmg2 = cheat::features::autowall.CanHit(eyepos, points.at(2), cheat::main::local(), Entity, hitbox);
			//auto dmg3 = cheat::features::autowall.CanHit(eyepos, points.at(3), cheat::main::local(), Entity, hitbox);

			if (hitbox == 0) {
				auto dmg = cheat::features::autowall.CanHit(eyepos, points.at(1), cheat::main::local(), Entity, hitbox);
				auto dmg2 = cheat::features::autowall.CanHit(eyepos, points.at(4), cheat::main::local(), Entity, hitbox);
				auto dmg3 = cheat::features::autowall.CanHit(eyepos, points.at(3), cheat::main::local(), Entity, hitbox);

				if (max(dmg, max(dmg2, dmg3)) >= nob)
					return dmg;
			}
			else
			{
				auto dmg = cheat::features::autowall.CanHit(eyepos, points.at(1), cheat::main::local(), Entity, hitbox);
				auto dmg2 = cheat::features::autowall.CanHit(eyepos, points.at(6), cheat::main::local(), Entity, hitbox);
				auto dmg3 = cheat::features::autowall.CanHit(eyepos, points.at(4), cheat::main::local(), Entity, hitbox);

				//auto dmg = cheat::features::autowall.CanHit(eyepos, points.at(1), cheat::main::local(), Entity, hitbox);
				//auto dmg2 = cheat::features::autowall.CanHit(eyepos, points.at(2), cheat::main::local(), Entity, hitbox);
				//auto dmg3 = cheat::features::autowall.CanHit(eyepos, points.at(3), cheat::main::local(), Entity, hitbox);

				if (max(dmg, max(dmg2, dmg3)) >= nob)
					return dmg;
			}
		}
		else
		{
			auto dmg1 = cheat::features::autowall.CanHit(eyepos, points.at(0), cheat::main::local(), Entity, hitbox);
			auto dmg2 = cheat::features::autowall.CanHit(eyepos, points.at(1), cheat::main::local(), Entity, hitbox);

			if (max(dmg1, dmg2) >= nob)
				return max(dmg1, dmg2);
		}
	}

	if (cdmg >= nob)
		return cdmg;

	return 0;
}

void c_aimbot::build_seed_table()
{
	if (precomputed_seeds.size() >= total_seeds)
		return;

	for (auto i = 0; i < total_seeds; i++) {
		RandomSeed(i + 1);

		const auto pi_seed = RandomFloat(0.f, 6.2831855f);

		precomputed_seeds.emplace_back(RandomFloat(0.f, 1.f),
			sin(pi_seed), cos(pi_seed));
	}
}

bool c_aimbot::hit_chance(QAngle angle, C_BasePlayer *ent, float chance, int hitbox, float damage, int &hc)
{
	//auto aechseVektor = [](Vector aechsen, float *forward, float *right, float *up) -> void
	//{
	//	float aechse;
	//	float sp, sy, cp, cy;
	//
	//	aechse = aechsen[0] * (3.1415f / 180);
	//	sp = sin(aechse);
	//	cp = cos(aechse);
	//
	//	aechse = aechsen[1] * (3.1415f / 180);
	//	sy = sin(aechse);
	//	cy = cos(aechse);
	//
	//	if (forward)
	//	{
	//		forward[0] = cp * cy;
	//		forward[1] = cp * sy;
	//		forward[2] = -sp;
	//	}
	//
	//	if (right || up)
	//	{
	//		float sr, cr;
	//
	//		aechse = aechsen[2] * (3.1415f / 180);
	//		sr = sin(aechse);
	//		cr = cos(aechse);
	//
	//		if (right)
	//		{
	//			right[0] = -1 * sr * sp * cy + -1 * cr * -sy;
	//			right[1] = -1 * sr * sp * sy + -1 * cr *cy;
	//			right[2] = -1 * sr * cp;
	//		}
	//
	//		if (up)
	//		{
	//			up[0] = cr * sp *cy + -sr * -sy;
	//			up[1] = cr * sp *sy + -sr * cy;
	//			up[2] = cr * cp;
	//		}
	//	}
	//};
	//
	//auto praviVektor = [](Vector src, Vector &dst) -> void
	//{
	//	float sp, sy, cp, cy;
	//
	//	Math::SinCos(DEG2RAD(src[1]), &sy, &cy);
	//	Math::SinCos(DEG2RAD(src[0]), &sp, &cp);
	//
	//	dst.x = cp * cy;
	//	dst.y = cp * sy;
	//	dst.z = -sp;
	//};
	//
	//auto vekAechse = [this](Vector &forward, Vector &up, Vector &aechse) -> void
	//{
	//	auto CrossProduct = [this](const Vector& a, const Vector& b) -> Vector
	//	{
	//		return Vector(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
	//	};
	//
	//	Vector left = CrossProduct(up, forward);
	//	left.NormalizeInPlace();
	//
	//	float forwardDist = forward.Length2D();
	//
	//	if (forwardDist > 0.001f)
	//	{
	//		aechse.x = atan2f(-forward.z, forwardDist) * 180 / M_PI_F;
	//		aechse.y = atan2f(forward.y, forward.x) * 180 / M_PI_F;
	//
	//		float upZ = (left.y * forward.x) - (left.x * forward.y);
	//		aechse.z = atan2f(left.z, upZ) * 180 / M_PI_F;
	//	}
	//	else
	//	{
	//		aechse.x = atan2f(-forward.z, forwardDist) * 180 / M_PI_F;
	//		aechse.y = atan2f(-left.x, left.y) * 180 / M_PI_F;
	//		aechse.z = 0;
	//	}
	//};
	//
	//auto weap = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));
	//
	//if (!weap)
	//	return false;
	//
	//if (chance < 2.f)
	//	return true;
	//
	//auto weap_data = weap->GetCSWeaponData();
	//
	//if (!weap_data)
	//	return false;
	//
	//Vector forward, right, up;
	//Vector neu = cheat::main::local()->GetEyePosition();
	//Math::AngleVectors(aechse, &forward, &right, &up);//aechseVektor(aechse, (float*)&forward, (float*)&right, (float*)&up);
	//
	//int cHits = 0;
	//int cNeededHits = static_cast<int>(150.f * (chance / 100.f));
	//
	//weap->UpdateAccuracyPenalty();
	//float weap_sir = weap->GetSpread();
	//float weap_inac = weap->GetInaccuracy() + weap->m_fAccuracyPenalty();
	//auto recoil_index = weap->m_flRecoilIndex();
	//
	//if (weap_sir <= 0.f)
	//	return true;
	//
	//for (int i = 0; i < 150; i++)
	//{
	//	float a = RandomFloat(0.f, 1.f);
	//	float b = RandomFloat(0.f, 6.2831855f);
	//	float c = RandomFloat(0.f, 1.f);
	//	float d = RandomFloat(0.f, 6.2831855f);
	//
	//	float inac = a * weap_inac;
	//	float sir = c * weap_sir;
	//
	//	if (weap->m_iItemDefinitionIndex() == 64)
	//	{
	//		a = 1.f - a * a;
	//		a = 1.f - c * c;
	//	}
	//	else if (weap->m_iItemDefinitionIndex() == 28 && recoil_index < 3.0f)
	//	{
	//		for (int i = 3; i > recoil_index; i--)
	//		{
	//			a *= a;
	//			c *= c;
	//		}
	//
	//		a = 1.0f - a;
	//		c = 1.0f - c;
	//	}
	//
	//	Vector sirVec((cos(b) * inac) + (cos(d) * sir), (sin(b) * inac) + (sin(d) * sir), 0), direction;
	//
	//	direction.x = forward.x + (sirVec.x * right.x) + (sirVec.y * up.x);
	//	direction.y = forward.y + (sirVec.x * right.y) + (sirVec.y * up.y);
	//	direction.z = forward.z + (sirVec.x * right.z) + (sirVec.y * up.z);
	//	direction.Normalize();
	//
	//	QAngle viewAnglesSpread;
	//	Math::VectorAngles(direction, up, viewAnglesSpread);
	//	viewAnglesSpread.Normalize();
	//
	//	Vector viewForward;
	//	Math::AngleVectors(viewAnglesSpread, &viewForward);
	//	viewForward.NormalizeInPlace();
	//
	//	viewForward = neu + (viewForward * weap_data->range);
	//
	//	trace_t tr;
	//	Ray_t ray;
	//
	//	ray.Init(neu, viewForward);
	//
	//	Source::m_pEngineTrace->ClipRayToEntity(ray, MASK_SHOT | CONTENTS_GRATE | CONTENTS_WINDOW, ent, &tr);
	//
	//	//Source::m_pDebugOverlay->AddLineOverlay(neu, tr.endpos, 255, 0, 0, false, Source::m_pGlobalVars->interval_per_tick * 2);
	//
	//	if (tr.m_pEnt == ent)
	//		++cHits;
	//
	//	if (static_cast<int>((static_cast<float>(cHits) / 150.f) * 100.f) >= chance)
	//		return true;
	//
	//	if ((150 - i + cHits) < cNeededHits)
	//		return false;
	//}
	//return false;

	cheat::features::aimbot.build_seed_table();

	C_Hitbox ht;
	get_hitbox_data(&ht, ent, hitbox, ent->m_CachedBoneData().Base());

	int traces_hit = 0;
	int awalls_hit = 0;

	Vector forward, right, up;
	auto eye_position = cheat::main::local()->GetEyePosition();
	Math::AngleVectors(angle/* + cheat::main::local()->m_aimPunchAngle() * 2.f*/, &forward, &right, &up); // maybe add an option to not account for punch.

	auto weapon = cheat::main::local()->get_weapon();

	if (!weapon || !weapon->GetCSWeaponData())
		return false;

	weapon->UpdateAccuracyPenalty();
	float weap_spread = weapon->GetSpread();
	float weap_inaccuracy = weapon->GetInaccuracy();

	// setup calculation parameters.
	//const auto round_acc = [](const float accuracy) { return roundf(accuracy * 1000.f) / 1000.f; };
	//const auto sniper = weapon->m_iItemDefinitionIndex() == weapon_awp || weapon->m_iItemDefinitionIndex() == weapon_g3sg1|| weapon->m_iItemDefinitionIndex() == weapon_scar20 || weapon->m_iItemDefinitionIndex() == weapon_ssg08;
	//const auto crouched = cheat::main::local()->m_fFlags() & FL_DUCKING;

	// no need for hitchance, if we can't increase it anyway.
	/*if (crouched)
	{
		if (round_acc(weap_inaccuracy) == round_acc(sniper ? weapon->GetCSWeaponData()->flInaccuracyCrouchAlt : weapon->GetCSWeaponData()->flInaccuracyCrouch))
			return true;
	}
	else
	{
		if (round_acc(weap_inaccuracy) == round_acc(sniper ? weapon->GetCSWeaponData()->flInaccuracyStandAlt : weapon->GetCSWeaponData()->flInaccuracyStand))
			return true;
	}*/

	if (weap_inaccuracy == 0.f)
		return true;

	//if (weapon->m_iItemDefinitionIndex() == weapon_revolver && cheat::main::local()->m_fFlags() & FL_ONGROUND)
	//	return weap_inaccuracy < (cheat::main::local()->m_fFlags() & FL_DUCKING ? .0020f : .0055f);

	//float max_damage = cheat::Cvars.RageBot_MinDamage.GetValue();

	//if (max_damage > ent->m_iHealth())
	//	max_damage = ent->m_iHealth();

	//cheat::Cvars.RageBot_accuracy_boost

	if (precomputed_seeds.empty())
		return false;

	Vector total_spread, spread_angle, end;
	float inaccuracy, spread_x, spread_y;
	std::tuple<float, float, float>* seed;

	for (int i = 0; i < total_seeds; i++)
	{
		////if (cheat::game::last_cmd)
		//RandomSeed(i + 1);

		//auto check_wall = (RandomFloat(1.f, 100.f) <= cheat::Cvars.RageBot_accuracy_boost.GetValue());

		//float a = RandomFloat(0.f, 1.f);
		//float b = RandomFloat(0.f, 6.2831855f);
		//float c = RandomFloat(0.f, 1.f);
		//float d = RandomFloat(0.f, 6.2831855f);

		//float inaccuracy = a * weap_inaccuracy;
		//float spread = c * weap_spread;

		///*if (weapon->m_iItemDefinitionIndex() == 64 && cmd->buttons & IN_ATTACK2)
		//{
		//	a = 1.f - a * a;
		//	a = 1.f - c * c;
		//}*/

		//Vector spreadView((cos(b) * inaccuracy) + (cos(d) * spread), (sin(b) * inaccuracy) + (sin(d) * spread), 0), direction;

		//direction.x = forward.x + (spreadView.x * right.x) + (spreadView.y * up.x);
		//direction.y = forward.y + (spreadView.x * right.y) + (spreadView.y * up.y);
		//direction.z = forward.z + (spreadView.x * right.z) + (spreadView.y * up.z);

		//QAngle viewAnglesSpread;
		//Math::VectorAngles(direction, up, viewAnglesSpread);
		//viewAnglesSpread.Clamp();

		//Vector viewForward;
		//Math::AngleVectors(viewAnglesSpread, &viewForward);

		//viewForward = eye_position + (viewForward * weapon->GetCSWeaponData()->range);

		//trace_t tr;
		//Ray_t ray;

		//ray.Init(eye_position, viewForward);

		// get seed.
		seed = &precomputed_seeds[i];

		// calculate spread.
		inaccuracy = std::get<0>(*seed) * weap_inaccuracy;
		spread_x = std::get<2>(*seed) * inaccuracy;
		spread_y = std::get<1>(*seed) * inaccuracy;
		total_spread = (forward + right * spread_x + up * spread_y);

		// calculate angle with spread applied.
		Math::VectorAngles(total_spread, spread_angle);

		// calculate end point of trace.
		Math::AngleVectors(spread_angle, &end);
		end = eye_position + end * 8092.f;

		bool intersect = false;

		if (ht.isOBB) {
			Math::VectorITransform(eye_position, ent->m_CachedBoneData().Base()[ht.bone], ht.mins);
			Math::VectorIRotate(end, ent->m_CachedBoneData().Base()[ht.bone], ht.maxs);

			intersect = Math::IntersectBB(eye_position, end, ht.mins, ht.maxs);
		}
		else
		{
			intersect = Math::Intersect(eye_position, end, ht.mins, ht.maxs, ht.radius);
		}

		/*trace_t tr;
		Ray_t ray;

		ray.Init(eye_position, end);

		Source::m_pEngineTrace->ClipRayToEntity(ray, 0x46004003, ent, &tr);*/

		if (intersect) {
			++traces_hit;

			if (cheat::Cvars.RageBot_accuracy_boost.GetValue() > 1.f && cheat::features::autowall.CanHit(eye_position, end, cheat::main::local(), ent, hitbox) > 1.f)
				++awalls_hit;
		}

		if (((static_cast<float>(traces_hit) / static_cast<float>(total_seeds)) >= (chance / 100.f))) 
		{
			if (((static_cast<float>(awalls_hit) / static_cast<float>(total_seeds)) >= (cheat::Cvars.RageBot_accuracy_boost.GetValue() / 100.f)) || cheat::Cvars.RageBot_accuracy_boost.GetValue() <= 1.f)
				return true;
		}

		// abort if we can no longer reach hitchance.
		if (((static_cast<float>(traces_hit + total_seeds - i) / static_cast<float>(total_seeds)) < (chance / 100.f)))
			return false;

		//if (float(traces_hit * 0.392156863f/*0.78125f*/) >= chance)
		//	return true;
	}

	hc = int(traces_hit * 0.392156863f);

	//if (float(traces_hit * 0.78125f) >= chance)
	//	return true;

	return ((static_cast<float>(traces_hit) / static_cast<float>(total_seeds)) >= (chance / 100.f));
}

bool c_aimbot::delay_shot(C_BasePlayer * player, CUserCmd* cmd, bool & send_packet, c_player_records record, C_WeaponCSBaseGun * local_weapon, bool did_shot_backtrack)
{
	if (cheat::Cvars.RageBot_FixLag.GetValue() == 0 || player == nullptr)
		return false;

	if (cheat::features::aaa.player_resolver_records[player->entindex()-1].is_shifting && cheat::features::lagcomp.is_time_delta_too_large(&record.m_NewTick))
		return true;

	//auto lul = record.m_Tickrecords;
	auto animstate = player->get_animation_state();
	auto player_weapon = player->get_weapon();
	auto net = Source::m_pEngine->GetNetChannelInfo();

	if (/*lul.size() <= 0 || */net == nullptr || animstate == nullptr || player_weapon == nullptr || !local_weapon->IsGun() || local_weapon->m_iItemDefinitionIndex() == weapon_taser)
		return false;

	auto unlag = (((((cmd->tick_count + 1) * Source::m_pGlobalVars->interval_per_tick) + net->GetLatency(FLOW_OUTGOING)) - net->GetLatency(FLOW_OUTGOING)) <= player->m_flSimulationTime());
	auto unlag2 = (((((cmd->tick_count + 2) * Source::m_pGlobalVars->interval_per_tick) + net->GetLatency(FLOW_OUTGOING)) - net->GetLatency(FLOW_OUTGOING)) <= player->m_flSimulationTime());

	//auto already_delays = false;

	//if (record.backtrack_ticks == 0 && record.type == RECORD_NORMAL)
	return (unlag2 && unlag);//already_delays = !(TIME_TO_TICKS(player->m_flSimulationTime() - player->m_flOldSimulationTime()) >= 1);
}

bool c_aimbot::prefer_baim(C_BasePlayer * player, bool & send_packet, C_WeaponCSBaseGun * local_weapon, bool is_lethal_shot)
{
	if (cheat::Cvars.RageBot_Baim_PreferBaim.GetValue() == 0 || player == nullptr || !local_weapon->GetCSWeaponData() || !player->get_animation_state())
		return false;

	if (cheat::Cvars.RageBot_Baim_PreferBaim.GetValue() == 1)
		return is_lethal_shot;
	else if (cheat::Cvars.RageBot_Baim_PreferBaim.GetValue() == 2)
	{
		if (cheat::main::local()->m_vecVelocity().Length2D() > (local_weapon->GetCSWeaponData()->max_speed / 3) && local_weapon->m_iItemDefinitionIndex() != weapon_ssg08)
			return true;
		else if (is_lethal_shot)
			return true;
		else if (cheat::features::aaa.sub_59B13C30(player->get_animation_state()) < 20)
			return false;
		else if (local_weapon->GetInaccuracy() >= 0.015f && player->get_abs_origin().Distance(player->m_vecOrigin()) > 2000)
			return true;
		//else if (local_weapon->m_iItemDefinitionIndex() == weapon_ssg08 || local_weapon->m_iItemDefinitionIndex() == weapon_awp)
		//	return true;
	}

	return false;
}

void c_aimbot::visualise_hitboxes(C_BasePlayer* entity, matrix3x4_t *mx, Color color, float time)
{
	auto model = entity->GetModel();

	if (!model)
		return;

	auto studioHdr = Source::m_pModelInfo->GetStudioModel(model);

	if (!studioHdr
		/*|| !entity->SetupBones(matrix, 128, 0x100, 0)*/)
		return;

	auto set = studioHdr->pHitboxSet(entity->m_nHitboxSet());

	if (!set)
		return;

	for (int i = 0; i < set->numhitboxes; i++)
	{
		auto hitbox = set->pHitbox(i);

		if (!hitbox)
			continue;

		Vector min, max, center;
		Math::VectorTransform(hitbox->bbmin, mx[hitbox->bone], min);
		Math::VectorTransform(hitbox->bbmax, mx[hitbox->bone], max);



		if (hitbox->radius != -1) {
			Source::m_pDebugOverlay->AddCapsuleOverlay(min, max, hitbox->radius, color.r(), color.g(), color.b(), 255, time, 0, 1);
		}
		/*else
		{
		Math::VectorTransform((hitbox->bbmin + hitbox->bbmax) * 0.5f, mx[hitbox->bone], center);

		Vector v59 = Vector::Zero;
		Vector v76 = Vector::Zero;

		Math::MatrixAngles(mx[hitbox->bone], &v59, &v76);


		v59.x = 0;

		Source::m_pDebugOverlay->AddBoxOverlay(center, hitbox->bbmin, hitbox->bbmax, v59, color.r(), color.g(), color.b(), 200, time);
		}*/
	}
};

void c_aimbot::autostop(CUserCmd* cmd, bool &send_packet, C_WeaponCSBaseGun* local_weapon/*, C_BasePlayer* best_player, float dmg, bool hitchanced*/)
{
	if (!cheat::Cvars.RageBot_AutoStop.GetValue())
		return;

	//auto should_stop = (!best_player && !hitchanced && dmg == 1337.f) || ((int)cheat::Cvars.RageBot_AutoStop.GetValue() == 3 || (best_player && dmg >= best_player->m_iHealth() && (int)cheat::Cvars.RageBot_AutoStop.GetValue() == 1) || (!hitchanced && (int)cheat::Cvars.RageBot_AutoStop.GetValue() == 2));

	//auto quickstop = cheat::game::pressed_keys[(int)cheat::Cvars.Misc_quickstop_key.GetValue()] && cheat::Cvars.Misc_quickstop_key.GetValue() > 0.f;
		
	/*if ((fmaxf(cheat::main::local()->m_flNextAttack(), local_weapon->m_flNextPrimaryAttack()))
		- Source::m_pGlobalVars->curtime > 0.1f)
	{
		return;
	}*/

	static bool was_onground = cheat::main::local()->m_fFlags() & FL_ONGROUND;

	if (cheat::main::fast_autostop && local_weapon && local_weapon->GetCSWeaponData() && was_onground && cheat::main::local()->m_fFlags() & FL_ONGROUND)
	{
		auto speed = ((cmd->sidemove * cmd->sidemove) + (cmd->forwardmove * cmd->forwardmove));
		auto lol = sqrt(speed);

		auto velocity = cheat::main::local()->m_vecVelocity();
		float maxspeed = 30.f;

		if (cheat::main::local()->m_bIsScoped())
			maxspeed = local_weapon->GetCSWeaponData()->max_speed_alt;
		else
			maxspeed = local_weapon->GetCSWeaponData()->max_speed;

		maxspeed *= 0.31f;

		switch ((int)cheat::Cvars.RageBot_AutoStopMethod.GetValue())
		{
		case 0:
		{
			cmd->buttons |= IN_SPEED;

			if (velocity.Length2D() > maxspeed)
			{
				//	cmd->buttons |= IN_WALK;
				//Engine::Movement::Instance()->quick_stop(cmd);

				if ((maxspeed + 1.0f) <= velocity.Length2D())
				{
					cmd->forwardmove = 0.0f;
					cmd->sidemove = 0.0f;
				}
				else
				{
					cmd->sidemove = (maxspeed * (cmd->sidemove / lol));
					cmd->forwardmove = (maxspeed * (cmd->forwardmove / lol));
				}
			}
		}
		break;
		/*case 1:
		{
			cmd->buttons |= IN_SPEED;

			if (velocity.Length2D() > maxspeed)
			{
				if ((maxspeed + 1.0f) <= velocity.Length2D())
				{
					cmd->forwardmove = 0.0f;
					cmd->sidemove = 0.0f;
				}
				else
				{
					cmd->sidemove = (maxspeed * (cmd->sidemove / lol));
					cmd->forwardmove = (maxspeed * (cmd->forwardmove / lol));
				}
			}

			if (!cheat::main::fakeducking)
				cmd->buttons |= IN_DUCK;
		}
		break;*/
		case 1:
		{
			Engine::Movement::Instance()->quick_stop(cmd);
		}
		break;
		}

		cheat::main::fast_autostop = false;
		//Engine::Prediction::Instance()->Begin(cmd);
	}

	was_onground = (cheat::main::local()->m_fFlags() & FL_ONGROUND);
}

float c_aimbot::check_wall(C_WeaponCSBaseGun* local_weapon, Vector startpoint, Vector direction, C_BasePlayer* entity)
{
	static float prev_dmg = -1.f;

	if (!local_weapon)
		return -1.f;

	auto weapon_info = local_weapon->GetCSWeaponData();

	if (!weapon_info)
		return -1.f;

	float autowall_dmg = 0;

	Vector start = startpoint;

	float maxRange = weapon_info->range;

	autowall_dmg = weapon_info->damage;

	auto max_range = weapon_info->range * 2;

	Vector end = start + (direction * max_range);

	//Source::m_pDebugOverlay->AddLineOverlay(start, end, 255, 0, 0, false, Source::m_pGlobalVars->interval_per_tick * 2.f);

	auto currentDistance = 0.f;

	CGameTrace enterTrace;

	CTraceFilter filter;
	filter.pSkip = cheat::main::local();

	cheat::features::autowall.TraceLine(start, end, MASK_SHOT | CONTENTS_GRATE, cheat::main::local(), &enterTrace);

	if (enterTrace.fraction == 1.0f)
		autowall_dmg = 0.f;
	else
		//calculate the damage based on the distance the bullet traveled.
		currentDistance += enterTrace.fraction * max_range;

	//Let's make our damage drops off the further away the bullet is.
	autowall_dmg *= pow(weapon_info->range_modifier, (currentDistance / 500.f));

	auto enterSurfaceData = Source::m_pPhysProps->GetSurfaceData(enterTrace.surface.surfaceProps);
	float enterSurfPenetrationModifier = enterSurfaceData->game.penetrationmodifier;

	if (currentDistance > 3000.0 && weapon_info->penetration > 0.f || enterSurfPenetrationModifier < 0.1f)
		return -1.f;

	//if (enterTrace.m_pEnt != nullptr)
	//{
	//	//This looks gay as fuck if we put it into 1 long line of code.
	//	bool canDoDamage = (enterTrace.hitgroup != HITGROUP_GEAR && enterTrace.hitgroup != HITGROUP_GENERIC);
	//	bool isPlayer = (enterTrace.m_pEnt->GetClientClass() && enterTrace.m_pEnt->GetClientClass()->m_ClassID == class_ids::CCSPlayer);
	//	bool isEnemy = (cheat::main::local()->m_iTeamNum() != ((C_BasePlayer*)enterTrace.m_pEnt)->m_iTeamNum());
	//	bool onTeam = (((C_BasePlayer*)enterTrace.m_pEnt)->m_iTeamNum() == 2 || ((C_BasePlayer*)enterTrace.m_pEnt)->m_iTeamNum() == 3);

	//	//TODO: Team check config
	//	if ((canDoDamage && isPlayer && isEnemy) && onTeam)
	if (entity)
	{
		int armorValue = entity->m_ArmorValue();
		int hitGroup = enterTrace.hitgroup;

		//Fuck making a new function, lambda beste. ~ Does the person have armor on for the hitbox checked?
		auto IsArmored = [&enterTrace, entity]()->bool
		{
			if ((DWORD)entity < 50977)
				return false;

			switch (enterTrace.hitgroup)
			{
			case HITGROUP_HEAD:
				return entity->m_bHasHelmet(); //Fuck compiler errors - force-convert it to a bool via (!!)
			case HITGROUP_GENERIC:
			case HITGROUP_CHEST:
			case HITGROUP_STOMACH:
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
				return true;
			default:
				return false;
			}
		};

		switch (hitGroup)
		{
		case HITGROUP_HEAD:
			autowall_dmg *= /*hasHeavyArmor ? 2.f :*/ 4.f; //Heavy Armor does 1/2 damage
			break;
		case HITGROUP_STOMACH:
			autowall_dmg *= 1.25f;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			autowall_dmg *= 0.75f;
			break;
		default:
			break;
		}

		if (armorValue > 0 && IsArmored())
		{
			float bonusValue = 1.f, armorBonusRatio = 0.5f, armorRatio = weapon_info->armor_ratio / 2.f;

			////Damage gets modified for heavy armor users
			//if (hasHeavyArmor)
			//{
			//	armorBonusRatio = 0.33f;
			//	armorRatio *= 0.5f;
			//	bonusValue = 0.33f;
			//}

			auto NewDamage = autowall_dmg * armorRatio;

			//if (hasHeavyArmor)
			//	NewDamage *= 0.85f;

			if (((autowall_dmg - (autowall_dmg * armorRatio)) * (bonusValue * armorBonusRatio)) > armorValue)
				NewDamage = autowall_dmg - (armorValue / armorBonusRatio);

			autowall_dmg = NewDamage;
		}
	}

	//	if (!canDoDamage && isPlayer && isEnemy)
	//		return -1.f;

	//	return autowall_dmg;
	//}

	auto penetrate_count = 4;

	if (!cheat::features::autowall.HandleBulletPenetration(weapon_info, enterTrace, start, direction, penetrate_count, autowall_dmg, weapon_info->penetration, cheat::convars::ff_damage_bullet_penetration, true))
		return -1.f;

	if (penetrate_count <= 0)
		return -1.f;

	if (fabs(prev_dmg - autowall_dmg) < 3.f)
		autowall_dmg = prev_dmg;

	prev_dmg = autowall_dmg;

	return autowall_dmg;
}

struct lagcomp_mt
{
	lagcomp_mt() {  };
	C_BasePlayer* entity;
};

struct freestand_mt
{
	freestand_mt() {  };
	C_BasePlayer* entity;
	resolver_records * res;
};

std::string hitbox_to_string(int h)
{
	/*	if (!skip_hitscan && (!cheat::Cvars.RageBot_Hitboxes.Has(0) && hitboxesLoop[i] <= HITBOX_NECK ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(1) && hitboxesLoop[i] >= HITBOX_THORAX && hitboxesLoop[i] <= HITBOX_UPPER_CHEST) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(2) && hitboxesLoop[i] >= HITBOX_PELVIS && hitboxesLoop[i] <= HITBOX_BODY) ||
					//(!cheat::Cvars.RageBot_Hitboxes.Has(2) && hitboxesLoop[i] >= HITBOX_PELVIS && hitboxesLoop[i] <= HITBOX_UPPER_CHEST) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(3) && hitboxesLoop[i] >= HITBOX_RIGHT_HAND && hitboxesLoop[i] <= HITBOX_LEFT_FOREARM) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(4) && hitboxesLoop[i] >= HITBOX_RIGHT_THIGH && hitboxesLoop[i] <= HITBOX_LEFT_CALF) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(5) && hitboxesLoop[i] == HITBOX_RIGHT_FOOT && hitboxesLoop[i] == HITBOX_LEFT_FOOT)))*/

	switch (h)
	{
	case 0:
		return "head";
		break;
	case 1:
		return "neck";
		break;
	case HITBOX_RIGHT_FOOT:
		return "right foot";
		break;
	case HITBOX_LEFT_FOOT:
		return "left foot";
		break;
	case HITBOX_RIGHT_HAND:
		return "right hand";
		break;
	case HITBOX_LEFT_HAND:
		return "left hand";
		break;
	default:
		return "body";
		break;
	}
}

bool c_aimbot::work(CUserCmd* cmd, bool &send_packet)
{
	m_entities.clear();
	C_BasePlayer* best_player = nullptr;
	Vector best_hitbox = Vector::Zero;
	int	best_hitboxid = -1;
	static float last_shoot_time = 0.f;
	last_pitch = FLT_MAX;
	//matrix3x4_t bestmatrix[128];

	if (!cheat::main::local() || cheat::main::local()->IsDead()) return false;

	auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

	if (!local_weapon) return false;

	auto IsGrenade = [](int item)
	{
		if (item == weapon_flashbang
			|| item == weapon_hegrenade
			|| item == weapon_smokegrenade
			|| item == weapon_molotov
			|| item == weapon_decoy
			|| item == weapon_incgrenade
			|| item == weapon_tagrenade)
			return true;
		else
			return false;
	};

	/*auto run_search_backtrack = [](void* _data) {
		lagcomp_mt *data = (lagcomp_mt*)_data;

		auto player_record = &cheat::features::lagcomp.records[data->entity->entindex() - 1];

		if (player_record->m_Tickrecords.size() <= 0)
			return;

		player_record->being_lag_compensated = true;
		cheat::features::lagcomp.start_position_adjustment(data->entity);
		player_record->being_lag_compensated = false;
	};*/

	if ((Source::m_pGlobalVars->realtime - last_shoot_time) > 0.25f) {
		cheat::features::antiaimbot.flip_side = false;

		if (cheat::Cvars.anti_aim_desync_extend_limit_on_shot.GetValue())
			cheat::features::antiaimbot.extend = false;
	}

	auto is_zeus = (local_weapon->m_iItemDefinitionIndex() == weapon_taser);

	if (!cheat::Cvars.RageBot_Enable.GetValue() || !local_weapon->IsGun() || local_weapon->m_iClip1() <= 0 /*|| local_weapon->m_Activity() == 183 || local_weapon->m_Activity() == 481 */|| Source::m_pGlobalVars->curtime < cheat::main::local()->m_flNextAttack()/*|| IsGrenade(local_weapon->m_iItemDefinitionIndex()) || local_weapon->m_iItemDefinitionIndex() == weapon_knife_ct || local_weapon->m_iItemDefinitionIndex() == weapon_knife_t*/)
		return false;

	//if (cheat::Cvars.RageBot_autor8.GetValue())
	//{
	//	if (local_weapon->m_iItemDefinitionIndex() == 64)
	//	{
	//		auto v7 = Source::m_pGlobalVars->curtime;
	//		if (r8cock_time <= (Source::m_pGlobalVars->frametime + v7))
	//			r8cock_time = v7 + 0.249f;
	//		else
	//			cmd->buttons |= IN_ATTACK;
	//	}
	//	else
	//	{
	//		r8cock_time = 0.0;
	//	}
	//
	//	//local_weapon->m_flPostponeFireReadyTime() = r8cock_time;
	//}

	auto neyepos = cheat::main::local()->GetEyePosition();

	//Source::m_pDebugOverlay->AddBoxOverlay(neyepos, Vector(-3, -3, -3), Vector(3, 3, 3), Vector::Zero, 255, 0, 0, 255, Source::m_pGlobalVars->interval_per_tick * 2.f);

	if (local_weapon->m_iItemDefinitionIndex() == 64 && cheat::Cvars.RageBot_autor8.GetValue())
	{
		auto curtime = TICKS_TO_TIME(cheat::main::local()->m_nTickBase());
		//	unk_3CCA9A51 = 1;
		//	//cmd->buttons &= ~IN_ATTACK;
		//	cmd->buttons &= ~IN_ATTACK2;
		//
		//	if (unk_3CCA9A51 && local_weapon->IsFireTime(false))
		//	{
		//		if (r8cock_time <= curtime)
		//		{
		//			if (local_weapon->m_flNextSecondaryAttack() <= curtime)
		//				r8cock_time = curtime + 0.234375f;
		//			else
		//				cmd->buttons |= IN_ATTACK2;
		//		}
		//		else
		//			cmd->buttons |= IN_ATTACK;
		//
		//		unk_3CCA9A51 = curtime > r8cock_time;
		//	}
		//	else
		//	{
		//		unk_3CCA9A51 = 0;
		//		r8cock_time = curtime + 0.234375f;
		//		cmd->buttons &= ~IN_ATTACK;
		//	}
		//}

		if (local_weapon->m_flPostponeFireReadyTime() > curtime)
		{
			cmd->buttons |= IN_ATTACK;
		}
		// COCK EXTENDER
		else if (local_weapon->m_flNextSecondaryAttack() > curtime)
		{
			cmd->buttons |= IN_ATTACK2;
		}
	}

	int ppl = 0;
	float avg_distance = 0;

	if (m_entities.empty()) 
	{
		for (auto idx = 1; idx < 64; idx++)
		{
			auto entity = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(idx);
	
			if (!entity ||
				!entity->IsPlayer() ||
				entity->IsDormant() ||
				entity->m_iHealth() <= 0 ||
				entity->m_iTeamNum() == cheat::main::local()->m_iTeamNum() ||
				entity->m_bGunGameImmunity()
				) continue;

			/*cheat::features::lagcomp.records[idx - 1].restore_record.data_filled = false;
			cheat::features::lagcomp.store_record_data(entity, &cheat::features::lagcomp.records[idx - 1].restore_record);*/

			m_entities.push_back(TargetListing_t(entity));
	
			//ppl++;
			//avg_distance += cheat::main::local()->m_vecOrigin().Distance(entity->get_abs_origin());

			//if (ppl > 1)
			//{
				//if (float(avg_distance / ppl) < cheat::main::local()->m_vecOrigin().Distance(entity->m_vecOrigin()))
				//	continue;
			//}

			auto resolve_info = &cheat::features::aaa.player_resolver_records[idx - 1];

			cheat::features::lagcomp.records[idx-1].restore_record.data_filled = false;
			cheat::features::lagcomp.store_record_data(entity, &cheat::features::lagcomp.records[idx - 1].restore_record);

			if (resolve_info->leftrec.data_filled && resolve_info->rightrec.data_filled/* && resolve_info->simtime_updated*/)
			{
				cheat::features::lagcomp.apply_record_data(entity, &resolve_info->leftrec);
				auto lefthitbox = cheat::features::aimbot.get_hitbox(entity, 0);
				resolve_info->freestanding_record.left_damage = cheat::features::autowall.CanHit(neyepos, lefthitbox, cheat::main::local(), entity, 0);//cheat::features::aimbot.can_hit(0, entity, lefthitbox, entity->m_CachedBoneData().Base(), false, false);// 

				cheat::features::lagcomp.apply_record_data(entity, &resolve_info->rightrec);
				auto righthitbox = cheat::features::aimbot.get_hitbox(entity, 0);
				resolve_info->freestanding_record.right_damage = cheat::features::autowall.CanHit(neyepos, righthitbox, cheat::main::local(), entity, 0);//cheat::features::aimbot.can_hit(0, entity, righthitbox, entity->m_CachedBoneData().Base(), false, false);// 
			}

			//if (!cheat::features::lagcomp.is_time_delta_too_large(&cheat::features::lagcomp.records[idx - 1].m_NewTick))
			//	cheat::features::lagcomp.apply_record_data(entity, &cheat::features::lagcomp.records[idx - 1].m_NewTick, true);
			//else
				cheat::features::lagcomp.apply_record_data(entity, &cheat::features::lagcomp.records[idx - 1].restore_record);
			//else
			//	resolve_info->freestanding_record.reset();

			auto done = cheat::features::aaa.resolve_freestand(entity, resolve_info, entity->get_animation_state(), resolve_info->cur_side);

			if (done && resolve_info->cur_side != FLT_MAX) 
			{
				if (resolve_info->previous_side != resolve_info->cur_side)
					resolve_info->previous_side = resolve_info->cur_side;
			}
			else
				resolve_info->cur_side = resolve_info->previous_side;

			if (resolve_info->cur_side != FLT_MAX)
				resolve_info->resolving_method = resolve_info->cur_side;

			if (resolve_info->animations_updated)
				resolve_info->resolving_method = resolve_info->resolving_way;
		}
	
		std::sort(m_entities.begin(), m_entities.end(), [&](const TargetListing_t& a, const TargetListing_t& b)
		{
			if (cheat::Cvars.RageBot_TargetSelection.GetValue() == 0)
				return (a.hp < b.hp);
			else if (cheat::Cvars.RageBot_TargetSelection.GetValue() == 1)
				return (a.distance < b.distance);
			else
				return (a.fov < b.fov);
	
			return false;
		}
		);
	}

	if (cheat::main::updating_skins)
		return false;

	if (m_entities.empty()) {
		//cheat::features::lagcomp.finish_position_adjustment();
		return false;
	}

	auto is_innaccurate = (local_weapon->GetInaccuracy() > 0.012f);

	auto baimkey = (cheat::game::pressed_keys[(int)cheat::Cvars.RageBot_BaimKey.GetValue()] && cheat::Cvars.RageBot_BaimKey.GetValue());

	bool best_player_baim = false;
	//bool did_shot_backtrack = false;
	//int shot_time[65];
	int pisun = 0;

	static int hitboxesLoop[] =
	{
		HITBOX_HEAD,
		HITBOX_NECK,
		HITBOX_PELVIS,
		HITBOX_BODY,
		HITBOX_THORAX,
		HITBOX_CHEST,
		HITBOX_UPPER_CHEST,
		HITBOX_RIGHT_THIGH,
		HITBOX_LEFT_THIGH,
		HITBOX_RIGHT_CALF,
		HITBOX_LEFT_CALF,
		HITBOX_RIGHT_FOOT,
		HITBOX_LEFT_FOOT,
		HITBOX_RIGHT_HAND,
		HITBOX_LEFT_HAND,
		HITBOX_RIGHT_UPPER_ARM,
		HITBOX_RIGHT_FOREARM,
		HITBOX_LEFT_UPPER_ARM,
		HITBOX_LEFT_FOREARM
	};

	auto skip_hitscan = (cheat::Cvars.RageBot_Hitboxes.GetValue() == 0);

	float max_damage = cheat::Cvars.RageBot_MinDamage.GetValue();

	if (cheat::Cvars.RageBot_MinDmgKey.GetValue() != 0.f && cheat::game::pressed_keys[(int)cheat::Cvars.RageBot_MinDmgKey.GetValue()])
		max_damage = cheat::Cvars.RageBot_MinDmgKey_val.GetValue();

	//for (auto k = 1; k < 64; k++)
	for (auto target : m_entities)
	{
		//auto entity = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(k);
		auto entity = target.entity;
		auto k = target.idx;

		if (!entity ||
			entity->IsDormant() ||
			!entity->IsPlayer() ||
			entity->m_bGunGameImmunity() ||
			entity->m_iTeamNum() == cheat::main::local()->m_iTeamNum() ||
			entity->IsDead() ||
			!entity->GetClientClass())
		{
			if (cheat::features::lagcomp.records[k - 1].tick_count != -1)
				cheat::features::lagcomp.records[k - 1].reset(true);

			continue;
		}

		auto idx = entity->entindex();

		auto animstate = entity->get_animation_state();

		if (!animstate)
			continue;

		int loopsize = ARRAYSIZE(hitboxesLoop) - 1;

		cheat::features::lagcomp.records[idx - 1].tick_count = -1;
		cheat::features::lagcomp.records[idx - 1].backtrack_ticks = 0;
		cheat::features::lagcomp.records[idx - 1].type = RECORD_NORMAL;

		float maxRange = local_weapon->GetCSWeaponData()->range;
		auto hp = entity->m_iHealth();

		if (is_zeus)
			max_damage = entity->m_iHealth() + 1.f;

		bool shot = false;
		bool lt = false;
		auto resolve_info = &cheat::features::aaa.player_resolver_records[idx - 1];
		auto history_data = cheat::features::lagcomp.records[idx - 1].m_Tickrecords;

		const auto headh = get_hitbox(entity, HITBOX_HEAD, entity->m_CachedBoneData().Base());
		const auto head_dmg = can_hit(HITBOX_HEAD, entity, headh, entity->m_CachedBoneData().Base());

		if (!history_data.empty() && head_dmg <= 0.f && pisun > 0 && cheat::Cvars.Ragebot_fps_safety.GetValue())
			continue;

		if ((history_data.size() > 1 && fabs(entity->m_angEyeAngles().x) < 55.f && cheat::main::shots_fired[idx - 1] <= 0 && fabs(history_data[1].eye_angles.x) < 55.f && !resolve_info->did_shot_this_tick) || entity->IsBot())
			resolve_info->resolving_method = 0;

		if (history_data.size() > 2 && !resolve_info->did_shot_this_tick)
		{
			auto prepreprev = history_data.at(2);
			auto preprev = history_data.at(1);
			auto prev = history_data.at(0);

			if (fabs(prepreprev.eye_angles.x) > 70.f && fabs(preprev.eye_angles.x) > 70.f && fabs(prev.eye_angles.x) > 70.f && fabs(entity->m_angEyeAngles().x) < 45.f && entity->get_weapon() && !entity->get_weapon()->IsGun() && entity->get_weapon()->m_bPinPulled()) {
				resolve_info->resolving_method = 0;
			}
		}

		pisun++;

		float head_damage = 0.f;

		//const auto try_lower_delta = cheat::main::shots_fired[idx - 1] > 0 && cheat::main::shots_fired[idx - 1] % 2 == 1;

		if (cheat::main::shots_fired[idx - 1] > 0)
		{
			const auto type = ((cheat::main::shots_fired[idx - 1] - 1) % 3);

			switch (type)
			{
			case 0:
				resolve_info->resolving_method = 0;
				break;
			case 1:
				resolve_info->resolving_method = (resolve_info->previous_resolving * -1);
				break;
			default:
				resolve_info->resolving_method = resolve_info->previous_resolving;
				break;
			}
		}
		else
			resolve_info->previous_resolving = resolve_info->resolving_method;

		float shead_damage = 0.f;

		if (!cheat::features::lagcomp.is_time_delta_too_large(&cheat::features::lagcomp.records[idx - 1].m_NewTick)) {
			cheat::features::lagcomp.apply_record_data(entity, &cheat::features::lagcomp.records[idx - 1].m_NewTick, true);

			auto hitbox_head = get_hitbox(entity, HITBOX_HEAD, entity->m_CachedBoneData().Base());
			head_damage = can_hit(HITBOX_HEAD, entity, hitbox_head, entity->m_CachedBoneData().Base());
			cheat::features::lagcomp.records[idx - 1].tick_count = TIME_TO_TICKS(cheat::features::lagcomp.records[idx - 1].m_NewTick.simulation_time + cheat::main::lerp_time);
			cheat::features::lagcomp.records[idx - 1].backtrack_ticks = Source::m_pGlobalVars->tickcount - TIME_TO_TICKS(cheat::features::lagcomp.records[idx - 1].m_NewTick.simulation_time);
			cheat::features::lagcomp.records[idx - 1].m_Current = cheat::features::lagcomp.records[idx - 1].m_NewTick;
		}
		else
			continue;

		if (!cheat::features::lagcomp.is_time_delta_too_large(&cheat::features::lagcomp.records[idx - 1].m_ShotBacktracking) /*&& !resolve_info->breaking_lc*/ && !resolve_info->is_shifting)
		{
			cheat::features::lagcomp.apply_record_data(entity, &cheat::features::lagcomp.records[idx - 1].m_ShotBacktracking, true);
			auto hitbox_head = get_hitbox(entity, HITBOX_HEAD, entity->m_CachedBoneData().Base());
			shead_damage = can_hit(HITBOX_HEAD, entity, hitbox_head, entity->m_CachedBoneData().Base());

			if (shead_damage >= max_damage) {
				cheat::features::lagcomp.records[idx - 1].tick_count = TIME_TO_TICKS(cheat::features::lagcomp.records[idx - 1].m_ShotBacktracking.simulation_time + cheat::main::lerp_time);
				cheat::features::lagcomp.records[idx - 1].backtrack_ticks = Source::m_pGlobalVars->tickcount - TIME_TO_TICKS(cheat::features::lagcomp.records[idx - 1].m_ShotBacktracking.simulation_time);
				cheat::features::lagcomp.records[idx - 1].type = RECORD_SHOT;
				cheat::features::lagcomp.records[idx - 1].m_Current = cheat::features::lagcomp.records[idx - 1].m_ShotBacktracking;
				shot = true;
				head_damage = shead_damage;
			}
		}

		if (!shot)
		{
			if (!cheat::features::lagcomp.is_time_delta_too_large(&cheat::features::lagcomp.records[idx - 1].m_LastTick)/* && cheat::features::lagcomp.records[idx - 1].m_LastTick.origin.DistanceSquared(entity->m_vecOrigin()) > 16 */&& !resolve_info->breaking_lc && !resolve_info->is_shifting)
			{
				cheat::features::lagcomp.apply_record_data(entity, &cheat::features::lagcomp.records[idx - 1].m_LastTick, true);
				auto hitbox_head = get_hitbox(entity, HITBOX_HEAD, entity->m_CachedBoneData().Base());
				auto lhead_damage = can_hit(HITBOX_HEAD, entity, hitbox_head, entity->m_CachedBoneData().Base());

				if (lhead_damage >= head_damage) {
					cheat::features::lagcomp.records[idx - 1].tick_count = TIME_TO_TICKS(cheat::features::lagcomp.records[idx - 1].m_LastTick.simulation_time + cheat::main::lerp_time);
					cheat::features::lagcomp.records[idx - 1].backtrack_ticks = Source::m_pGlobalVars->tickcount - TIME_TO_TICKS(cheat::features::lagcomp.records[idx - 1].m_LastTick.simulation_time);
					cheat::features::lagcomp.records[idx - 1].m_Current = cheat::features::lagcomp.records[idx - 1].m_LastTick;
					lt = true;
					head_damage = lhead_damage;
				}
			}
		}
		
		auto distance = entity->m_vecOrigin().Distance(cheat::main::local()->m_vecOrigin());

		if (distance > maxRange || is_zeus && distance > (180.0f - (entity->m_iHealth() / 10.f))) {
			cheat::features::lagcomp.finish_position_adjustment(entity);
			continue;
		}

		float pelvis_damage = 0.f;
		float stomach_damage = 0.f;

		if (!skip_hitscan)
		{
			auto hitbox_pelvis = get_hitbox(entity, HITBOX_PELVIS, entity->m_CachedBoneData().Base());
			pelvis_damage = can_hit(HITBOX_PELVIS, entity, hitbox_pelvis, entity->m_CachedBoneData().Base());

			auto hitbox_stomach = get_hitbox(entity, HITBOX_BODY, entity->m_CachedBoneData().Base());
			stomach_damage = can_hit(HITBOX_BODY, entity, hitbox_stomach, entity->m_CachedBoneData().Base());
		}
		else
			loopsize = 1;

		if (is_zeus)
			loopsize = 5;

		const float at_target_yaw = Math::CalcAngle(cheat::main::local()->m_vecOrigin(), entity->get_abs_origin()).y + 180.f;

		auto baim = baimkey || ((cheat::Cvars.RageBot_bodyaim.Has(0) && !cheat::features::aaa.compare_delta(fabs(at_target_yaw + 90.f), fabs(animstate->eye_yaw), 80.f)) ||
			(cheat::Cvars.RageBot_bodyaim.Has(1) && animstate->speed_2d > 1.f && animstate->speed_2d < 100.f && (entity->m_fFlags() & FL_ONGROUND)) ||
			(cheat::Cvars.RageBot_bodyaim.Has(2) && (resolve_info->did_shot_this_tick || shot)) ||
			(cheat::Cvars.RageBot_bodyaim.Has(3) && (local_weapon->GetInaccuracy() > 0.18f)) ||
			(cheat::Cvars.RageBot_bodyaim.Has(4) && (pelvis_damage > hp || stomach_damage > hp /*|| hp < 50.f*/)) ||
			(cheat::Cvars.RageBot_bodyaim.Has(5) && !(entity->m_fFlags() & FL_ONGROUND || animstate->on_ground))/* ||
			(cheat::Cvars.RageBot_bodyaim.Has(6) && (local_weapon->GetInaccuracy() > 2.f))*/);

		auto headaim = !baim;//!baimkey && ((cheat::Cvars.RageBot_allow_head.Has(0) && (pelvis_damage <= 0.f || stomach_damage <= 0.f) && head_damage > 0.f) || !cheat::Cvars.RageBot_allow_head.Has(0)) &&
			//((cheat::Cvars.RageBot_allow_head.Has(1) && cheat::features::aaa.compare_delta(fabs(at_target_yaw + 90.f), fabs(animstate->eye_yaw), 60.f)) ||
			//(cheat::Cvars.RageBot_allow_head.Has(2) && resolve_info->did_shot_this_tick) ||
			//	(cheat::Cvars.RageBot_allow_head.Has(3) && animstate->speed_2d > 110.f && (entity->m_fFlags() & FL_ONGROUND)) ||
			//	(cheat::Cvars.RageBot_allow_head.Has(4) && resolve_info->current_tick_max_delta < 30.f));

		auto xshots = (cheat::main::shots_total[idx - 1] >= (int)cheat::Cvars.RageBot_Baim_AfterXshots.GetValue() && cheat::Cvars.RageBot_Baim_AfterXshots.GetValue() > 0.f);

		auto priority = int(baim ? HITBOX_PELVIS : HITBOX_HEAD);

		if (headaim)
			priority = HITBOX_HEAD;

		if (xshots || is_zeus || baimkey || is_innaccurate && distance > 900.f)
			priority = HITBOX_PELVIS;

		if (cheat::Cvars.Ragebot_headaim_only_on_shot.GetValue())
			priority = (shot && head_damage > hp) ? HITBOX_HEAD : HITBOX_PELVIS;

		if (!cheat::Cvars.RageBot_Hitboxes.Has(0))
			priority = HITBOX_PELVIS;

		//if (shot && head_damage > pelvis_damage)
		//	priority = HITBOX_HEAD;

		//if (is_innaccurate && priority == HITBOX_HEAD)
		//	priority = HITBOX_NECK;

		auto phitbox = get_hitbox(entity, priority, entity->m_CachedBoneData().Base());
		auto pdmg = can_hit(priority, entity, phitbox, entity->m_CachedBoneData().Base());

		if (!phitbox.IsZero() && (pdmg > max_damage || (pdmg >= hp && cheat::Cvars.RageBot_ScaledmgOnHp.GetValue())) && safe_point(entity, cheat::main::local()->GetEyePosition(), phitbox, priority, shot))
		{
			max_damage = pdmg;
			best_player = entity;
			best_hitbox = phitbox;
			best_hitboxid = priority;
			best_player_baim = baim;
		}
		else
		{
			for (auto i = loopsize; i >= 0; i--)
			{
				if (i > loopsize || i < 0)
					continue;

				if (cheat::Cvars.Ragebot_headaim_only_on_shot.GetValue() && !shot && hitboxesLoop[i] != HITBOX_BODY && hitboxesLoop[i] != HITBOX_PELVIS)
					continue;

				if (skip_hitscan && hitboxesLoop[i] != 0)
					continue;

				if ((!headaim || !cheat::Cvars.RageBot_Hitboxes.Has(0)) && hitboxesLoop[i] <= 1)
					continue;

				if (!skip_hitscan && (!cheat::Cvars.RageBot_Hitboxes.Has(0) && hitboxesLoop[i] <= HITBOX_NECK ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(1) && (hitboxesLoop[i] >= HITBOX_THORAX && hitboxesLoop[i] <= HITBOX_UPPER_CHEST)) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(2) && (hitboxesLoop[i] >= HITBOX_PELVIS && hitboxesLoop[i] <= HITBOX_BODY)) ||
					//(!cheat::Cvars.RageBot_Hitboxes.Has(2) && hitboxesLoop[i] >= HITBOX_PELVIS && hitboxesLoop[i] <= HITBOX_UPPER_CHEST) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(3) && hitboxesLoop[i] >= HITBOX_RIGHT_HAND) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(4) && (hitboxesLoop[i] >= HITBOX_RIGHT_THIGH && hitboxesLoop[i] <= HITBOX_LEFT_CALF)) ||
					(!cheat::Cvars.RageBot_Hitboxes.Has(5) && (hitboxesLoop[i] == HITBOX_RIGHT_FOOT || hitboxesLoop[i] == HITBOX_LEFT_FOOT))))
					continue;

				if (is_innaccurate && (hitboxesLoop[i] == HITBOX_RIGHT_FOOT
					|| hitboxesLoop[i] == HITBOX_LEFT_FOOT
					|| hitboxesLoop[i] == HITBOX_HEAD
					|| hitboxesLoop[i] >= HITBOX_RIGHT_HAND))
					continue;

				if (is_zeus && (hitboxesLoop[i] <= 1 || ((stomach_damage > 0.f || pelvis_damage > 0.f) && (hitboxesLoop[i] == 11 || hitboxesLoop[i] == 12))))
					continue;

				if ((baim || xshots || baimkey) && !((pelvis_damage <= 0.f || stomach_damage <= 0.f) && head_damage > 0.f) && hitboxesLoop[i] <= 1)
					continue;

				auto hitbox = get_hitbox(entity, hitboxesLoop[i], entity->m_CachedBoneData().Base());

				if (hitbox.IsZero())
					continue;

				if (!safe_point(entity, cheat::main::local()->GetEyePosition(), hitbox, hitboxesLoop[i], shot))
					continue;

				auto dmg = can_hit(hitboxesLoop[i], entity, hitbox, entity->m_CachedBoneData().Base());

				if (dmg > max_damage || (dmg > hp && cheat::Cvars.RageBot_ScaledmgOnHp.GetValue()))
				{
					max_damage = dmg;
					best_player = entity;
					best_hitbox = hitbox;
					best_hitboxid = hitboxesLoop[i];
					best_player_baim = baim;
				}
			}
		}

		if (best_player == nullptr || best_hitboxid == -1)
			cheat::features::lagcomp.finish_position_adjustment(entity);
		else
			break;
	}

	float max_dmg = cheat::Cvars.RageBot_MinDamage.GetValue();

	if (cheat::Cvars.RageBot_MinDmgKey.GetValue() > 0.f && cheat::game::pressed_keys[(int)cheat::Cvars.RageBot_MinDmgKey.GetValue()])
		max_dmg = cheat::Cvars.RageBot_MinDmgKey_val.GetValue();

	if (best_player != nullptr && best_hitboxid != -1) 
	{
		//cheat::features::lagcomp.store_record_data()
		memcpy(&cheat::features::lagcomp.records[best_player->entindex()-1].matrix, best_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * best_player->GetBoneCount());

		if (best_player->get_multipoints(best_hitboxid, cheat::main::points[best_player->entindex() - 1][best_hitboxid], best_player->m_CachedBoneData().Base()))
		{
			best_hitbox.clear();

			auto resolve_info = &cheat::features::aaa.player_resolver_records[best_player->entindex() - 1];

			auto points = cheat::main::points[best_player->entindex() - 1][best_hitboxid];
			auto cwall = cheat::features::autowall.CanHit(cheat::main::local()->GetEyePosition(), points.back(), cheat::main::local(), best_player, best_hitboxid);

			if (((cwall >= best_player->m_iHealth() || (cwall * 2.f) >= best_player->m_iHealth() || best_player_baim) && best_hitboxid > 1) && cheat::main::last_penetrated_count <= 1) {
				best_hitbox = points.back();
				max_dmg = cwall;
			}
			else
			{
				for (size_t i = 0; i < points.size(); i++)
				{
					auto &point = points[i];

					if (!safe_static_point(best_player, cheat::main::local()->GetEyePosition(), point, best_hitboxid, cheat::features::lagcomp.records[best_player->entindex() - 1].type == RECORD_SHOT))
						continue;
					if (!safe_point(best_player, cheat::main::local()->GetEyePosition(), point, best_hitboxid, cheat::features::lagcomp.records[best_player->entindex() - 1].type == RECORD_SHOT))
						continue;

					//Source::m_pDebugOverlay->AddLineOverlay(cheat::main::local()->GetEyePosition(), point, 255, 0, 0, false, Source::m_pGlobalVars->interval_per_tick * 2.f);

					const auto dmg = cheat::features::autowall.CanHit(cheat::main::local()->GetEyePosition(), point, cheat::main::local(), best_player, best_hitboxid);

					if (dmg > max_dmg)
					{
						max_dmg = dmg;
						best_hitbox = point;
					}
				}
			}
		}
	}

	auto hitchance = false;
	auto aim_angles = QAngle(0, 0, 0);
	int hc = 0;

	if (best_player != nullptr && !best_hitbox.IsZero()) {
		aim_angles = Math::CalcAngle(cheat::main::local()->GetEyePosition(), best_hitbox);
		aim_angles.Clamp();

		last_pitch = aim_angles.x;

		hitchance = hit_chance(aim_angles, best_player, cheat::Cvars.RageBot_HitChance.GetValue(), best_hitboxid, max_dmg, hc);
	}

	//cheat::features::lagcomp.finish_position_adjustment();

	if (!best_player || best_hitbox.IsZero() || aim_angles.IsZero() || best_hitboxid == -1)
		return false;

	cheat::features::lagcomp.finish_position_adjustment(best_player);

	//auto tick_record = -1;

	auto entity_index = best_player->entindex() - 1;

	auto is_zoomable_weapon = (local_weapon->m_iItemDefinitionIndex() == weapon_ssg08 || local_weapon->m_iItemDefinitionIndex() == weapon_awp || local_weapon->m_iItemDefinitionIndex() == weapon_scar20 || local_weapon->m_iItemDefinitionIndex() == weapon_g3sg1);
	auto sniper = (local_weapon->m_iItemDefinitionIndex() == weapon_ssg08 || local_weapon->m_iItemDefinitionIndex() == weapon_awp);
	//prevtickdata.did_hitchance = hitchance;
	//prevtickdata.prev_hitbox = best_hitboxid;

	cheat::main::fast_autostop = (cheat::Cvars.RageBot_AutoStopBetweenShots.GetValue() == 1.f ? true : local_weapon->IsFireTime()) && (cheat::Cvars.RageBot_AutoStop.GetValue() == 3.f || (best_player && max_dmg >= best_player->m_iHealth() && cheat::Cvars.RageBot_AutoStop.GetValue() == 1.f) || (!hitchance && cheat::Cvars.RageBot_AutoStop.GetValue() == 2.f));

	if (local_weapon->IsGun() && local_weapon->IsFireTime() && (cheat::Cvars.RageBot_AutoFire.GetValue() || (cheat::game::pressed_keys[(int)cheat::Cvars.RageBot_Key.GetValue()] && cheat::Cvars.RageBot_Key.GetValue())))
	{
		//tick_record = cheat::features::lagcomp.records[entity_index].tick_count;

		//auto netchan = Source::m_pEngine->GetNetChannelInfo();

		//auto can_shoot_if_hide_shots = (cheat::Cvars.RageBot_exploits.GetValue() == 1 && Source::m_pClientState->m_iChockedCommands >= 5 /*&& abs(Source::m_pGlobalVars->tickcount - last_shoot_tick) > 6*/) || cheat::Cvars.RageBot_exploits.GetValue() != 1 || cheat::main::fakeducking;
		//auto can_shoot_if_fakeduck = (netchan && (((cmd->tick_count - Source::m_pClientState->m_iChockedCommands + 14) * Source::m_pGlobalVars->interval_per_tick) + netchan->GetLatency(FLOW_OUTGOING) / Source::m_pGlobalVars->interval_per_tick) > (TIME_TO_TICKS(best_player->m_flSimulationTime() + cheat::main::lerp_time) - TICKS_TO_TIME(cheat::main::lerp_time) - 1)) || !cheat::main::fakeducking;//fabs(Source::m_pClientState->m_iChockedCommands - (int)cheat::Cvars.Misc_fakelag_value.GetValue()) > 9 && abs(Source::m_pGlobalVars->tickcount - last_shoot_tick) > 6;

		//auto target_time = shot_time[entity_index] == -1 ? (cheat::features::lagcomp.records[entity_index].m_NewTick.simulation_time) : (TICKS_TO_TIME(shot_time[entity_index]) - cheat::main::lerp_time);

		auto can_shoot_if_fakeduck = !cheat::main::fakeducking || cheat::main::stand;

		auto delayshot = (cheat::Cvars.RageBot_DelayaShot.GetValue() > 0.f && (Source::m_pGlobalVars->realtime - last_shoot_time) < (cheat::Cvars.RageBot_DelayaShot.GetValue() / 1000.f));// || delay_shot(best_player, cmd, send_packet, cheat::features::lagcomp.records[entity_index], local_weapon, cheat::features::lagcomp.records[entity_index].type == RECORD_SHOT);// || cheat::features::aaa.player_resolver_records[entity_index].is_shifting && tick_record <= 0;

		if (cheat::Cvars.RageBot_AutoScope.GetValue() && local_weapon->m_zoomLevel() == 0 && is_zoomable_weapon && !(cmd->buttons & IN_JUMP)) 
		{
			cmd->buttons |= IN_ATTACK2/*0x800u*/;
			return false;
		}

		auto is_tick_valid = cheat::features::lagcomp.records[entity_index].tick_count != -1;

		if (hitchance && can_shoot_if_fakeduck && !delayshot && is_tick_valid/* && ((fakeduck && can_shoot_if_fakeduck) || !fakeduck) && can_shoot_if_hide_shots*/)
		{
			cmd->tick_count = cheat::features::lagcomp.records[entity_index].tick_count;

			cmd->viewangles = aim_angles;
			cmd->buttons |= IN_ATTACK;

			if (cmd->buttons & IN_ATTACK && !is_zeus)
			{
				cheat::main::shots_fired[entity_index] += 1;
				cheat::main::shots_total[entity_index] += 1;
			}

			if (cmd->buttons & IN_ATTACK)
				cheat::main::shots++;

			if (!cheat::main::fakeducking && cheat::Cvars.RageBot_SilentAim.GetValue() >= 2.f && !cheat::features::antiaimbot.unchoke /*&& !send_packet*/) {
				send_packet = true;

				/* u can intersect fake and real here, if it is aligning -> do magic */

				//if (cheat::Cvars.RageBot_exploits.GetValue())
				//	cheat::features::antiaimbot.shift_ticks = (Source::m_pClientState->m_iChockedCommands + 4);
			}

			if (cmd->buttons & IN_ATTACK && cheat::Cvars.RageBot_SilentAim.GetValue() == 3.f)
				cheat::features::antiaimbot.flip_side = !cheat::features::antiaimbot.flip_side;

			cheat::features::antiaimbot.extend = true;
			cheat::features::antiaimbot.visual_real_angle = cmd->viewangles;

			//visualise_hitboxes(best_player, bestmatrix, Color::Red(), 4);
			if (cmd->buttons & IN_ATTACK) {
				if (auto net = Source::m_pEngine->GetNetChannelInfo(); net != nullptr) {
					auto impact_time = Source::m_pGlobalVars->curtime /*- net->GetLatency(FLOW_INCOMING) - net->GetLatency(FLOW_OUTGOING)*/ + /*Source::m_pGlobalVars->interval_per_tick*/net->GetLatency(MAX_FLOWS);
					cheat::main::fired_shot = _shotinfo(best_player, cheat::features::lagcomp.records[entity_index].matrix, cheat::main::local()->GetEyePosition(), best_hitbox, &cheat::features::lagcomp.records[entity_index]/*, cheat::features::lagcomp.records[entity_index].m_CurrentApplied*/, impact_time, hc, best_hitboxid);

					if (cheat::Cvars.Visuals_misc_logthings.Has(4))
						_events.push_back(_event(Source::m_pGlobalVars->curtime + 4.f, std::string("fired shot at " + best_player->m_szNickName() + "'s " + hitbox_to_string(best_hitboxid) + " [" + std::to_string(best_hitboxid) + "] | bt: " + std::to_string(cheat::features::lagcomp.records[entity_index].backtrack_ticks) + "t | dmg: " + std::to_string(max_dmg) + " | R:" + std::to_string(cheat::features::aaa.player_resolver_records[entity_index].resolving_method) + " [" + std::to_string(cheat::features::aaa.player_resolver_records[entity_index].freestanding_record.left_damage) + "]:[" + std::to_string(cheat::features::aaa.player_resolver_records[entity_index].freestanding_record.right_damage) + "] | shots: " + std::to_string(cheat::main::shots_fired[entity_index]))));
				}
			}

			if (!cheat::Cvars.RageBot_SilentAim.GetValue())
				Source::m_pEngine->SetViewAngles(aim_angles);

			if (cmd->buttons & IN_ATTACK)
				last_shoot_time = Source::m_pGlobalVars->realtime;//cheat::Cvars.RageBot_DelayaShot

			//Engine::Prediction::Instance()->Begin(cmd);

			return true;
		}
	}

	return false;
}