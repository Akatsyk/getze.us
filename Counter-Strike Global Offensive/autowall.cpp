#include "sdk.hpp"
#include "autowall.hpp"
#include "aimbot.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "math.hpp"
#include "displacement.hpp"

#include "thread/threading.h"
#include "thread/shared_mutex.h"

#include <thread>

////////////////////////////////////// Traces //////////////////////////////////////
void c_autowall::TraceLine(Vector& absStart, Vector& absEnd, unsigned int mask, C_BasePlayer* ignore, CGameTrace* ptr)
{
	Ray_t ray;
	ray.Init(absStart, absEnd);
	CTraceFilter filter;
	filter.pSkip = ignore;

	Source::m_pEngineTrace->TraceRay(ray, mask, &filter, ptr);
}

//void c_autowall::ClipTraceToPlayers(Vector& absStart, Vector absEnd, unsigned int mask, ITraceFilter* filter, CGameTrace* tr)
//{
//	//TODO: Un-ASM this
//#ifdef _WIN32
//	_asm
//	{
//		mov eax, filter
//		lea ecx, tr
//		push ecx
//		push eax
//		push mask
//		lea edx, absEnd
//		lea ecx, absStart
//		call Engine::Displacement::Data::m_uClipTracePlayers
//		add esp, 0xC
//	}
//#else
//	UTIL_ClipTraceToPlayers(absStart, absEnd, mask, filter, tr, 60.f, 0.f);
//#endif
//}

////////////////////////////////////// Legacy Functions //////////////////////////////////////
void c_autowall::GetBulletTypeParameters(float& maxRange, float& maxDistance, char* bulletType, bool sv_penetration_type)
{
	if (sv_penetration_type)
	{
		maxRange = 35.0;
		maxDistance = 3000.0;
	}
	else
	{
		//Play tribune to framerate. Thanks, stringcompare
		//Regardless I doubt anyone will use the old penetration system anyway; so it won't matter much.
		if (!strcmp(bulletType, ("BULLET_PLAYER_338MAG")))
		{
			maxRange = 45.0;
			maxDistance = 8000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_762MM")))
		{
			maxRange = 39.0;
			maxDistance = 5000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_556MM")) || !strcmp(bulletType, ("BULLET_PLAYER_556MM_SMALL")) || !strcmp(bulletType, ("BULLET_PLAYER_556MM_BOX")))
		{
			maxRange = 35.0;
			maxDistance = 4000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_57MM")))
		{
			maxRange = 30.0;
			maxDistance = 2000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_50AE")))
		{
			maxRange = 30.0;
			maxDistance = 1000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_357SIG")) || !strcmp(bulletType, ("BULLET_PLAYER_357SIG_SMALL")) || !strcmp(bulletType, ("BULLET_PLAYER_357SIG_P250")) || !strcmp(bulletType, ("BULLET_PLAYER_357SIG_MIN")))
		{
			maxRange = 25.0;
			maxDistance = 800.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_9MM")))
		{
			maxRange = 21.0;
			maxDistance = 800.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_45ACP")))
		{
			maxRange = 15.0;
			maxDistance = 500.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_BUCKSHOT")))
		{
			maxRange = 0.0;
			maxDistance = 0.0;
		}
	}
}

////////////////////////////////////// Misc Functions //////////////////////////////////////
bool c_autowall::IsBreakableEntity(C_BasePlayer* entity)
{
	if (!entity)
		return false;

	static auto is_breakable_ent = Memory::Scan(cheat::main::clientdll, "55 8B EC 51 56 8B F1 85 F6 74 68");

	int m_take_damage = *(int*)((uintptr_t)is_breakable_ent + 38);
	int backup = *(int*)((uintptr_t)entity + m_take_damage);

	auto client_class = entity->GetClientClass();

	if (!client_class)
		return false;

	if (entity->entindex() > 0 && client_class->m_ClassID != class_ids::CFuncTrackTrain &&
		client_class->m_ClassID != class_ids::CBaseDoor)
	//if (!(strstr(client_class->m_pNetworkName, "FuncBrush")
	//	|| strstr(client_class->m_pNetworkName, "BaseDoor") // fuck doors
	//	|| strstr(client_class->m_pNetworkName, "CSPlayer")
	//	|| strstr(client_class->m_pNetworkName, "BaseEntity")))
	//{
		*(int*)((uintptr_t)entity + m_take_damage) = DAMAGE_YES;
	

	using fn_t = bool(__thiscall*)(C_BaseEntity*);
	auto result = ((fn_t)is_breakable_ent)(entity);
	*(int*)((uintptr_t)entity + m_take_damage) = backup;

	return result;
}

void c_autowall::ScaleDamage(CGameTrace &enterTrace, weapon_info *weaponData, float& currentDamage)
{
	////Cred. to N0xius for reversing this.
	////TODO: _xAE^; look into reversing this yourself sometime

	if (!enterTrace.m_pEnt || !enterTrace.m_pEnt->GetClientClass() || enterTrace.m_pEnt->GetClientClass()->m_ClassID != class_ids::CCSPlayer)
		return;

	auto target = ((C_BasePlayer*)enterTrace.m_pEnt);
	//int armorValue = ((C_BasePlayer*)enterTrace.m_pEnt)->m_ArmorValue();
	int hitgroup = enterTrace.hitgroup;
	auto is_zeus = cheat::main::local()->get_weapon()->m_iItemDefinitionIndex() == weapon_taser;

	////Fuck making a new function, lambda beste. ~ Does the person have armor on for the hitbox checked?
	//auto IsArmored = [&enterTrace]()->bool
	//{
	//	C_BasePlayer* targetEntity = (C_BasePlayer*)enterTrace.m_pEnt;

	//	if ((int)targetEntity < 50977)
	//		return false;

	//	switch (enterTrace.hitgroup)
	//	{
	//	case HITGROUP_HEAD:
	//		return targetEntity->m_bHasHelmet(); //Fuck compiler errors - force-convert it to a bool via (!!)
	//	case HITGROUP_GENERIC:
	//	case HITGROUP_CHEST:
	//	case HITGROUP_STOMACH:
	//	case HITGROUP_LEFTARM:
	//	case HITGROUP_RIGHTARM:
	//		return true;
	//	default:
	//		return false;
	//	}
	//};

	//switch (hitGroup)
	//{
	//case HITGROUP_HEAD:
	//	currentDamage *= hasHeavyArmor ? 2.f : 4.f; //Heavy Armor does 1/2 damage
	//	break;
	//case HITGROUP_STOMACH:
	//	currentDamage *= 1.25f;
	//	break;
	//case HITGROUP_LEFTLEG:
	//case HITGROUP_RIGHTLEG:
	//	currentDamage *= 0.75f;
	//	break;
	//default:
	//	break;
	//}

	//if (armorValue > 0 && IsArmored())
	//{
	//	float bonusValue = 1.f, armorBonusRatio = 0.5f, armorRatio = weaponData->armor_ratio / 2.f;

	//	//Damage gets modified for heavy armor users
	//	if (hasHeavyArmor)
	//	{
	//		armorBonusRatio = 0.33f;
	//		armorRatio *= 0.5f;
	//		bonusValue = 0.33f;
	//	}

	//	auto NewDamage = currentDamage * armorRatio;

	//	if (hasHeavyArmor)
	//		NewDamage *= 0.85f;

	//	if (((currentDamage - (currentDamage * armorRatio)) * (bonusValue * armorBonusRatio)) > armorValue)
	//		NewDamage = currentDamage - (armorValue / armorBonusRatio);

	//	currentDamage = NewDamage;
	//}

	const auto is_armored = [&]() -> bool
	{
		if (target->m_ArmorValue() > 0.f)
		{
			switch (hitgroup)
			{
			case HITGROUP_GENERIC:
			case HITGROUP_CHEST:
			case HITGROUP_STOMACH:
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
				return true;
			case HITGROUP_HEAD:
				return target->m_bHasHelmet();
			default:
				break;
			}
		}

		return false;
	};

	if (!is_zeus) {
		switch (hitgroup)
		{
		case HITGROUP_HEAD:
			if (target->m_bHasHeavyArmor())
				currentDamage = (currentDamage * 4.f) * .5f;
			else
				currentDamage *= 4.f;
			break;
		case HITGROUP_STOMACH:
			currentDamage *= 1.25f;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			currentDamage *= .75f;
			break;
		default:
			break;
		}
	}

	if (is_armored())
	{
		auto modifier = 1.f, armor_bonus_ratio = .5f, armor_ratio = weaponData->armor_ratio * .5f;

		if (target->m_bHasHeavyArmor())
		{
			armor_bonus_ratio = 0.33f;
			armor_ratio = (weaponData->armor_ratio * 0.5f) * 0.5f;
			modifier = 0.33f;
		}

		auto new_damage = currentDamage * armor_ratio;

		if (target->m_bHasHeavyArmor())
			new_damage *= 0.85f;

		if ((currentDamage - currentDamage * armor_ratio) * (modifier * armor_bonus_ratio) > target->m_ArmorValue())
			new_damage = currentDamage - target->m_ArmorValue() / armor_bonus_ratio;

		currentDamage = new_damage;
	}
}

////////////////////////////////////// Main Autowall Functions //////////////////////////////////////
//bool c_autowall::TraceToExit(trace_t& enterTrace, trace_t& exitTrace, Vector startPosition, Vector direction)
//{
//	/*
//	Masks used:
//	MASK_SHOT_HULL					 = 0x600400B
//	CONTENTS_HITBOX					 = 0x40000000
//	MASK_SHOT_HULL | CONTENTS_HITBOX = 0x4600400B
//	*/
//
//	Vector start, end;
//	float maxDistance = 90.f, rayExtension = 4.f, currentDistance = 0;
//	int firstContents = 0;
//
//	while (currentDistance <= maxDistance)
//	{
//		//Add extra distance to our ray
//		currentDistance += rayExtension;
//
//		//Multiply the direction vector to the distance so we go outwards, add our position to it.
//		start = startPosition + direction * currentDistance;
//
//		//if (!firstContents) {
//		auto pointContents = Source::m_pEngineTrace->GetPointContents(start, 0x4600400B, nullptr);
//			//pointContents = firstContents;
//		//}
//		//else {
//		//	pointContents = Source::m_pEngineTrace->GetPointContents(start, 0x4600400B, nullptr);
//		//}
//
//		if (pointContents & MASK_SHOT_HULL && !(pointContents & CONTENTS_HITBOX))
//			continue;
//
//		//if (pointContents & 0x600400B || (!(pointContents & 0x40000000) && pointContents != firstContents))
//		//{
//
//		//if (!(pointContents & MASK_SHOT_HULL) || pointContents & CONTENTS_HITBOX && pointContents != firstContents) /*0x600400B, *0x40000000*/
//		//{
//			//Let's setup our end position by deducting the direction by the extra added distance
//			end = start - (direction * rayExtension);
//
//			//Let's cast a ray from our start pos to the end pos
//			TraceLine(start, end, 0x4600400B, nullptr, &exitTrace);
//
//			//Let's check if a hitbox is in-front of our enemy and if they are behind of a solid wall
//			if (exitTrace.startsolid && exitTrace.surface.flags & SURF_HITBOX)
//			{
//				TraceLine(start, startPosition, MASK_SHOT_HULL, (C_BasePlayer*)exitTrace.m_pEnt, &exitTrace);
//
//				if (exitTrace.DidHit() && !exitTrace.startsolid)
//				{
//					start = exitTrace.endpos;
//					return true;
//				}
//
//				continue;
//			}
//
//			//Can we hit? Is the wall solid?
//			if (exitTrace.DidHit() && !exitTrace.startsolid)
//			{
//
//				//Is the wall a breakable? If so, let's shoot through it.
//				if (c_autowall::IsBreakableEntity((C_BasePlayer*)enterTrace.m_pEnt) && c_autowall::IsBreakableEntity((C_BasePlayer*)exitTrace.m_pEnt))
//					return true;
//
//				if (enterTrace.surface.flags & SURF_NODRAW || !(exitTrace.surface.flags & SURF_NODRAW) && (exitTrace.plane.normal.Dot(direction) <= 1.f))
//				{
//					float multAmount = exitTrace.fraction * 4.f;
//					start -= direction * multAmount;
//					return true;
//				}
//
//				continue;
//			}
//
//			if (!exitTrace.DidHit() || exitTrace.startsolid)
//			{
//				if (enterTrace.DidHitNonWorldEntity() && c_autowall::IsBreakableEntity((C_BasePlayer*)enterTrace.m_pEnt))
//				{
//					exitTrace = enterTrace;
//					exitTrace.endpos = start + direction;
//					return true;
//				}
//
//				continue;
//		//	}
//		}
//	}
//	return false;
//}

uint32_t get_filter_simple_vtable()
{
	static const auto filter_simple = *reinterpret_cast<uint32_t*>(Memory::Scan("client_panorama.dll", "55 8B EC 83 E4 F0 83 EC 7C 56 52") + 0x3d);
	return filter_simple;
}

bool c_autowall::TraceToExit(trace_t& enter_trace, trace_t& exit_trace, const Vector start_position,
	const Vector direction, const bool is_local)
{
	const auto max_distance = is_local ? 200.f : 90.f;
	const auto ray_extension = is_local ? 8.f : 4.f;

	float current_distance = 0;
	auto first_contents = 0;

	while (current_distance <= max_distance)
	{
		current_distance += ray_extension;

		auto start = start_position + direction * current_distance;

		if (!first_contents)
			first_contents = Source::m_pEngineTrace->GetPointContents(start, MASK_SHOT_HULL | CONTENTS_HITBOX);

		const auto point_contents = Source::m_pEngineTrace->GetPointContents(start, MASK_SHOT_HULL | CONTENTS_HITBOX);

		if (!(point_contents & MASK_SHOT_HULL) || (point_contents & CONTENTS_HITBOX && point_contents != first_contents))
		{
			const auto end = start - direction * ray_extension;

			Ray_t r{};
			r.Init(start, end);
			uint32_t filter[4] = { get_filter_simple_vtable(),
				uint32_t(cheat::main::local()), 0, 0 };
			Source::m_pEngineTrace->TraceRay(r, MASK_SHOT_HULL | CONTENTS_HITBOX, reinterpret_cast<CTraceFilter*>(filter), &exit_trace);

			if (exit_trace.startsolid && exit_trace.surface.flags & SURF_HITBOX)
			{
				r.Init(start, start_position);
				filter[1] = reinterpret_cast<uint32_t>(exit_trace.m_pEnt);
				Source::m_pEngineTrace->TraceRay(r, MASK_SHOT_HULL | CONTENTS_HITBOX, reinterpret_cast<CTraceFilter*>(filter), &exit_trace);

				if (exit_trace.DidHit() && !exit_trace.startsolid)
				{
					start = exit_trace.endpos;
					return true;
				}

				continue;
			}

			if (exit_trace.DidHit() && !exit_trace.startsolid)
			{
				if (enter_trace.m_pEnt->is_breakable() && exit_trace.m_pEnt != nullptr && exit_trace.m_pEnt->is_breakable())
					return true;

				if (enter_trace.surface.flags & SURF_NODRAW
					|| (!(exit_trace.surface.flags & SURF_NODRAW)
						&& exit_trace.plane.normal.Dot(direction) <= 1.f))
				{
					const auto mult_amount = exit_trace.fraction * 4.f;
					start -= direction * mult_amount;
					return true;
				}

				continue;
			}

			if (!exit_trace.DidHit() || exit_trace.startsolid)
			{
				if (enter_trace.DidHitNonWorldEntity() && exit_trace.m_pEnt != nullptr && enter_trace.m_pEnt->is_breakable())
				{
					exit_trace = enter_trace;
					exit_trace.endpos = start;
					return true;
				}
			}
		}
	}

	return false;
}

bool c_autowall::HandleBulletPenetration(weapon_info* weaponData, trace_t& enterTrace, Vector& eyePosition, Vector direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, float ff_damage_bullet_penetration, bool pskip)
{
	//Because there's been issues regarding this- putting this here.
	if (&currentDamage == nullptr)
		return 0;

	//SafeLocalPlayer() false;
	CGameTrace exitTrace;
	C_BasePlayer* pEnemy = (C_BasePlayer*)enterTrace.m_pEnt;
	auto enterSurfaceData = Source::m_pPhysProps->GetSurfaceData(enterTrace.surface.surfaceProps);
	int enter_material = enterSurfaceData->game.material;

	if (!enterSurfaceData || enterSurfaceData->game.penetrationmodifier < 0.1f) {
		//possibleHitsRemaining = 0;
		return false;
	}

	auto enter_penetration_modifier = enterSurfaceData->game.penetrationmodifier;
	float enterDamageModifier = enterSurfaceData->game.damagemodifier;// , modifier, finalDamageModifier, combinedPenetrationModifier;
	bool isSolidSurf = (enterTrace.contents & CONTENTS_GRATE);
	bool isLightSurf = (enterTrace.surface.flags & SURF_NODRAW);

	//Test for "DE_CACHE/DE_CACHE_TELA_03" as the entering surface and "CS_ITALY/CR_MISCWOOD2B" as the exiting surface.
	//Fixes a wall in de_cache which seems to be broken in some way. Although bullet penetration is not recorded to go through this wall
	//Decals can be seen of bullets within the glass behind of the enemy. Hacky method, but works.
	if (enterTrace.surface.name == (const char*)0x2227c261 && exitTrace.surface.name == (const char*)0x2227c868)
		return false;

	if ((!possibleHitsRemaining && !isLightSurf && !isSolidSurf && enter_material != CHAR_TEX_GRATE && enter_material != CHAR_TEX_GLASS)
		|| weaponData->penetration <= 0.f)
		return false;

	if (!TraceToExit(enterTrace, exitTrace, enterTrace.endpos, direction, weaponData->damage > 10000.f)) {
		if (!(Source::m_pEngineTrace->GetPointContents(enterTrace.endpos, 0x600400B, nullptr) & 0x600400B))
			return false;
	}

	auto exitSurfaceData = Source::m_pPhysProps->GetSurfaceData(exitTrace.surface.surfaceProps);
	int exitMaterial = exitSurfaceData->game.material;
	float exitSurfPenetrationModifier = exitSurfaceData->game.penetrationmodifier;
	float exitDamageModifier = exitSurfaceData->game.damagemodifier;

	auto combined_damage_modifier = 0.16f;
	auto combined_penetration_modifier = 0.f;

	//Are we using the newer penetration system?
	if (enter_material == CHAR_TEX_GLASS || enter_material == CHAR_TEX_GRATE) {
		combined_damage_modifier = 0.05f;
		combined_penetration_modifier = 3.f;
	}
	else if (isSolidSurf || isLightSurf) {
		combined_penetration_modifier = 1.f;
		combined_damage_modifier = 0.16f;
	}
	else if (enter_material == CHAR_TEX_FLESH && (pEnemy != nullptr && pEnemy->m_iTeamNum() == cheat::main::local()->m_iTeamNum())) { // TODO: Team check config
		combined_penetration_modifier = ff_damage_bullet_penetration;
		combined_damage_modifier = 0.16f;
	}
	else {
		combined_damage_modifier = 0.16f;
		combined_penetration_modifier = ((enter_penetration_modifier + exitSurfPenetrationModifier) * 0.5f);
	}

	if (enter_material == exitMaterial) {
		if (exitMaterial == CHAR_TEX_WOOD || exitMaterial == CHAR_TEX_CARDBOARD)
			combined_penetration_modifier = 3.f;
		else if (exitMaterial == CHAR_TEX_PLASTIC)
			combined_penetration_modifier = 2.f;
	}

	/*auto v22 = fmaxf(1.0f / combined_penetration_modifier, 0.0f);
	auto v23 = fmaxf(3.0f / penetrationPower, 0.0f);

	auto penetration_modifier = fmaxf(0.f, 1.f / combined_penetration_modifier);
	auto penetration_distance = (exitTrace.endpos - enterTrace.endpos).Length();

	auto damage_lost = ((currentDamage * combined_damage_modifier) + ((v23 * v22) * 3.0f)) + (((penetration_distance * penetration_distance) * v22) * 0.041666668f);

	auto new_damage = currentDamage - damage_lost;

	currentDamage = new_damage;

	if (new_damage > 0.0f)
	{
		*eyePosition = exitTrace.endpos;
		--possibleHitsRemaining;
		return true;
	}*/

	auto thickness = (exitTrace.endpos - enterTrace.endpos).Length();
	thickness *= thickness;
	thickness *= fmaxf(0.f, 1.0f / combined_penetration_modifier);
	thickness /= 24.0f;

	const auto lost_damage = fmaxf(0.0f, currentDamage * combined_damage_modifier + fmaxf(0.f, 1.0f / combined_penetration_modifier)
		* 3.0f * fmaxf(0.0f, 3.0f / penetrationPower) * 1.25f + thickness);

	if (lost_damage > currentDamage)
		return false;

	if (lost_damage > 0.f)
		currentDamage -= lost_damage;

	if (currentDamage < 1.f)
		return false;

	eyePosition = exitTrace.endpos;
	--possibleHitsRemaining;

	return true;
}

void c_autowall::FixTraceRay(Vector end, Vector start, trace_t* oldtrace, C_BasePlayer* ent) {
	//auto mins = ent->OBBMins();
	//auto maxs = ent->OBBMaxs();

	//auto CenterOfBBOX = (maxs + mins) * 0.5f;
	//auto origin = ent->get_abs_origin();

	//Vector CenterOfEntityInWorld = (CenterOfBBOX + origin);

	//Vector delta(end - start);
	//delta.Normalize();

	//auto v20 = (CenterOfEntityInWorld - start);
	//auto ForwardDot = delta.Dot(v20);

	//float v23 = 0.f;

	//if (ForwardDot >= 0.0f) {
	//	//Player is at or in front
	//	if (ForwardDot <= delta.Length())
	//	{
	//		auto fuck(CenterOfEntityInWorld - ((delta * ForwardDot) + start));
	//		v23 = fuck.Length();
	//	}
	//	else
	//	{
	//		auto lol(CenterOfEntityInWorld - end);
	//		v23 = -lol.Length();
	//	}
	//}

	//else
	//	v23 = -v20.Length();

	//if (v23 > 60.0 || v23 < 0.0)
	//	return;

	//Ray_t ray;
	//ray.Init(start, end);

	//trace_t trace;
	//Source::m_pEngineTrace->ClipRayToEntity(ray, 0x4600400B | 0x40000000, (IHandleEntity*)ent, &trace);

	//if (oldtrace->fraction > trace.fraction)
	//	*oldtrace = trace;

	if (!ent)
		return;

	const auto mins = ent->OBBMins();
	const auto maxs = ent->OBBMaxs();

	auto dir(end - start);
	dir.Normalized();

	const auto center = (maxs + mins) / 2;
	const auto pos(center + ent->m_vecOrigin());

	auto to = pos - start;
	const float range_along = dir.Dot(to);

	float range;
	if (range_along < 0.f) {
		range = -to.Length();
	}
	else if (range_along > dir.Length()) {
		range = -(pos - end).Length();
	}
	else {
		auto ray(pos - (dir * range_along + start));
		range = ray.Length();
	}

	if (range <= 60.f) {

		Ray_t ray;
		ray.Init(start, end);

		trace_t trace;
		Source::m_pEngineTrace->ClipRayToEntity(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, ent, &trace);

		if (oldtrace->fraction > trace.fraction)
			*oldtrace = trace;
	}
}

void c_autowall::ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, uint32_t mask, ITraceFilter* filter, trace_t* tr) {
	float smallestFraction = tr->fraction;
	constexpr float maxRange = 60.0f;

	Vector delta(vecAbsEnd - vecAbsStart);
	const float delta_length = delta.Normalize();

	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);

	for (int i = 1; i <= 64; ++i) {
		auto ent = Source::m_pEntList->GetClientEntity(i);
		if (!ent || ent->IsDormant() || ent->IsDead())
			continue;

		if (filter && !filter->ShouldHitEntity(ent, mask))
			continue;

		matrix3x4_t coordinate_frame;
		Math::AngleMatrix(ent->get_abs_eye_angles(), ent->m_vecOrigin(), coordinate_frame);

		auto collideble = ent->GetCollideable();
		auto mins = collideble->OBBMins();
		auto maxs = collideble->OBBMaxs();

		auto obb_center = (maxs + mins) * 0.5f;
		Math::VectorTransform(obb_center, coordinate_frame, obb_center);

		auto extend = (obb_center - vecAbsStart);
		auto rangeAlong = delta.Dot(extend);

		float range;
		if (rangeAlong >= 0.0f) {
			if (rangeAlong <= delta_length)
				range = Vector(obb_center - ((delta * rangeAlong) + vecAbsStart)).Length();
			else
				range = -(obb_center - vecAbsEnd).Length();
		}
		else {
			range = -extend.Length();
		}

		if (range > 0.0f && range <= maxRange) {
			trace_t playerTrace;
			Source::m_pEngineTrace->ClipRayToEntity(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, ent, &playerTrace);
			if (playerTrace.fraction < smallestFraction) {
				// we shortened the ray - save off the trace
				*tr = playerTrace;
				smallestFraction = playerTrace.fraction;
			}
		}
	}
}

int HitboxToHitgroup(int Hitbox)
{
	switch (Hitbox)
	{
	case HITBOX_HEAD:
	case HITBOX_NECK:
		return HITGROUP_HEAD;
	case HITBOX_UPPER_CHEST:
	case HITBOX_CHEST:
	case HITBOX_THORAX:
	case HITBOX_LEFT_UPPER_ARM:
	case HITBOX_RIGHT_UPPER_ARM:
		return HITGROUP_CHEST;
	case HITBOX_PELVIS:
	case HITBOX_LEFT_THIGH:
	case HITBOX_RIGHT_THIGH:
	case HITBOX_BODY:
		return HITGROUP_STOMACH;
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
		return HITGROUP_LEFTLEG;
	case HITBOX_RIGHT_CALF:
	case HITBOX_RIGHT_FOOT:
		return HITGROUP_RIGHTLEG;
	case HITBOX_LEFT_FOREARM:
	case HITBOX_LEFT_HAND:
		return HITGROUP_LEFTARM;
	case HITBOX_RIGHT_FOREARM:
	case HITBOX_RIGHT_HAND:
		return HITGROUP_RIGHTARM;
	default:
		return HITGROUP_STOMACH;
	}
}

bool c_autowall::FireBullet(Vector eyepos, C_WeaponCSBaseGun* pWeapon, Vector& direction, float& currentDamage, C_BasePlayer* ignore, C_BasePlayer * to_who, int target_hitbox)
{
	if (!pWeapon || !ignore)
		return false;

	//SafeLocalPlayer() false;
	//bool sv_penetration_type;
	//	  Current bullet travel Power to penetrate Distance to penetrate Range               Player bullet reduction convars			  Amount to extend ray by
	float currentDistance = 0.f, penetrationPower, penetrationDistance, maxRange, ff_damage_bullet_penetration;

	static ConVar* damageBulletPenetration = Source::m_pCvar->FindVar("ff_damage_bullet_penetration");

	ff_damage_bullet_penetration = damageBulletPenetration->GetFloat();

	weapon_info* weaponData = pWeapon->GetCSWeaponData();
	CGameTrace enterTrace;

	//We should be skipping localplayer when casting a ray to players.
	CTraceFilter filter;
	filter.pSkip = ignore;

	if (!weaponData)
		return false;

	maxRange = weaponData->range;
	penetrationDistance = weaponData->range;
	penetrationPower = weaponData->penetration;

	//This gets set in FX_Firebullets to 4 as a pass-through value.
	//CS:GO has a maximum of 4 surfaces a bullet can pass-through before it 100% stops.
	//Excerpt from Valve: https://steamcommunity.com/sharedfiles/filedetails/?id=275573090
	//"The total number of surfaces any bullet can penetrate in a single flight is capped at 4." -CS:GO Official
	cheat::main::last_penetrated_count = 4;

	//Set our current damage to what our gun's initial damage reports it will do
	currentDamage = weaponData->damage;

	//If our damage is greater than (or equal to) 1, and we can shoot, let's shoot.
	while (cheat::main::last_penetrated_count > 0 && currentDamage >= 1.0f)
	{
		//Calculate max bullet range
		maxRange -= currentDistance;

		//Create endpoint of bullet
		Vector end = eyepos + direction * maxRange;

		TraceLine(eyepos, end, MASK_SHOT_HULL | CONTENTS_HITBOX, ignore, &enterTrace);

		if (to_who/* && target_hitbox == HITBOX_HEAD*/) {
			//Pycache/aimware traceray fix for head while players are jumping
			FixTraceRay(eyepos + direction * 40.f, eyepos, &enterTrace, to_who);
		}
		else
			ClipTraceToPlayers(eyepos, end + direction * 40.f, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &enterTrace);

		if (enterTrace.fraction == 1.0f)
			break;

		//We have to do this *after* tracing to the player.
		auto enterSurfaceData = Source::m_pPhysProps->GetSurfaceData(enterTrace.surface.surfaceProps);
		float enterSurfPenetrationModifier = enterSurfaceData->game.penetrationmodifier;
		int enterMaterial = enterSurfaceData->game.material;

		//calculate the damage based on the distance the bullet traveled.
		currentDistance += enterTrace.fraction * maxRange;

		//Let's make our damage drops off the further away the bullet is.
		currentDamage *= powf(weaponData->range_modifier, (currentDistance / 500.f));

		//Sanity checking / Can we actually shoot through?
		if (currentDistance > maxRange || currentDistance > 3000.0 && weaponData->penetration > 0.f || enterSurfPenetrationModifier < 0.1f)
			cheat::main::last_penetrated_count = 0;

		//This looks gay as fuck if we put it into 1 long line of code.
		bool canDoDamage = (enterTrace.hitgroup > 0 && enterTrace.hitgroup <= 8);
		bool isPlayer = (enterTrace.m_pEnt->GetClientClass() && enterTrace.m_pEnt->GetClientClass()->m_ClassID == class_ids::CCSPlayer);
		bool isEnemy = (ignore->m_iTeamNum() != ((C_BasePlayer*)enterTrace.m_pEnt)->m_iTeamNum());
		bool onTeam = (((C_BasePlayer*)enterTrace.m_pEnt)->m_iTeamNum() == 2 || ((C_BasePlayer*)enterTrace.m_pEnt)->m_iTeamNum() == 3);

		//TODO: Team check config
		if ((canDoDamage && isPlayer && isEnemy) && onTeam || (to_who == enterTrace.m_pEnt))
		{
			ScaleDamage(enterTrace, weaponData, currentDamage);
			return true;
		}

		//Calling HandleBulletPenetration here reduces our penetrationCounter, and if it returns true, we can't shoot through it.
		if (!HandleBulletPenetration(weaponData, enterTrace, eyepos, direction, cheat::main::last_penetrated_count, currentDamage, penetrationPower, ff_damage_bullet_penetration))
			break;
	}

	return false;
}

////////////////////////////////////// Usage Calls //////////////////////////////////////
float c_autowall::CanHit(Vector &vecEyePos, Vector &point)
{
	Vector angles, direction;
	Vector tmp = point - cheat::main::local()->GetEyePosition();
	float currentDamage = 0;

	Math::VectorAngles(tmp, angles);
	Math::AngleVectors(angles, &direction);
	direction.Normalize();

	auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

	if (local_weapon != nullptr && FireBullet(vecEyePos, local_weapon, direction, currentDamage, cheat::main::local()))
		return currentDamage;

	return -1; //That wall is just a bit too thick buddy
}

float c_autowall::CanHit(Vector &vecEyePos, Vector &point, C_BasePlayer* ignore_ent, C_BasePlayer* to_who, int target_hitbox)
{
	if (ignore_ent == nullptr || to_who == nullptr)
		return 0;

	Vector angles, direction;
	Vector tmp = point - vecEyePos;
	float currentDamage = 0;

	Math::VectorAngles(tmp, angles);
	Math::AngleVectors(angles, &direction);
	direction.Normalize();

	auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(ignore_ent->m_hActiveWeapon()));

	if (local_weapon != nullptr)
	{
		if (FireBullet(vecEyePos, local_weapon, direction, currentDamage, ignore_ent, to_who, target_hitbox))
			return currentDamage;
		else
			return -1;
	}

	return -1; //That wall is just a bit too thick buddy
}

struct autowall_mt
{
	autowall_mt() {  };

	Vector start;
	Vector direction;
	C_WeaponCSBaseGun* weapon;
	float* damage;
	C_BasePlayer* ignore_ent;
	C_BasePlayer* to_who;
	int target_hitbox;
	bool result;
};

/*
struct traced_rays
{
	CGameTrace trace;
	Ray_t ray;
	CTraceFilter filter;
	Vector w2s;
	bool success_w2s;
};

static traced_rays rays[720] = {};

static auto mt_traceray_lambda = [](void* _data) {
	traced_rays *data = (traced_rays*)_data;

	Source::m_pEngineTrace->TraceRay(data->ray, MASK_SOLID, &data->filter, &data->trace);
	data->success_w2s = Drawing::WorldToScreen(data->trace.endpos, data->w2s);
};

Vector eye_pos = cheat::main::local()->GetEyePosition();

for (int i = 0; i < 720; i++) {
	Vector end = Vector(cos(DEG2RAD(float(i)* 0.5f)) * 550.0f + eye_pos.x,
		sin(DEG2RAD(float(i) * 0.5f)) * 550.0f + eye_pos.y,
		eye_pos.z
	);

	rays[i].filter = CTraceFilter();
	rays[i].filter.pSkip = cheat::main::local();
	rays[i].ray.Init(eye_pos, end);
	Threading::QueueJobRef(mt_traceray_lambda, &rays[i]);
}

Threading::FinishQueue();

Vector w2s_eye;
if (Drawing::WorldToScreen(eye_pos, w2s_eye)) {
	for (int i = 0; i < 720; i++) {
		if (rays[i].success_w2s)
			Drawing::DrawLine(w2s_eye.x, w2s_eye.y, rays[i].w2s.x, rays[i].w2s.y, Color::White(200));
	}
}
*/