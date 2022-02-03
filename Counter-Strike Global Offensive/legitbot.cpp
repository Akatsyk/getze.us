#include "legitbot.hpp"
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
#include "misc.hpp"
#include "menu.h"
#include "aimbot.hpp"

float c_legitbot::get_fov(QAngle view_angle, QAngle aim_angle)
{
	auto delta = aim_angle - view_angle;
	//delta.Normalize();
	return fabs(sqrtf(powf(delta.x, 2.0f) + powf(delta.y, 2.0f)));
}

bool c_legitbot::is_viable(Vector start_position, Vector end_position, C_BasePlayer* m_player)
{
	auto is_visible = [m_player,start_position,end_position]() -> bool
	{
		if (!m_player) return false;
		CGameTrace tr;
		Ray_t ray;
		static CTraceFilter traceFilter;
		traceFilter.pSkip = cheat::main::local();

		ray.Init(start_position, end_position);

		Source::m_pEngineTrace->TraceRay(ray, 0x4600400B, &traceFilter, &tr);

		return (tr.m_pEnt == m_player || tr.fraction >= 0.99f);
	};

	if (!cheat::Cvars.LegitBot_IgnoreSmoke.GetValue() && cheat::features::misc.goes_thru_smoke(start_position, end_position))
		return false;
	if (!is_visible())
		return false;
	// autowall check there (if we need autowall checkbox in legitbot?)

	return true;
}

bool c_legitbot::find_target(float &fov)
{
	if (_target_id != -1)
		return false; // how did this happen?

	auto eye_pos = cheat::main::local()->GetEyePosition();
	float best_fov = FLT_MAX;

	for (auto k = 1; k < 64; k++)
	{
		auto ent = Source::m_pEntList->GetClientEntity(k);

		if (!ent ||
			!ent->IsPlayer() ||
			!ent->GetClientClass())
			continue;

		if (ent->IsDormant() || 
			ent->IsDead() ||
			(ent->m_iTeamNum() == cheat::main::local()->m_iTeamNum() && !cheat::Cvars.LegitBot_FriendlyFire.GetValue()))
		{
			continue;
		}

		//int cur_bone = nearest ? 8 : menu_hitbox;

		Vector vec_bone = ent->get_bone_pos(8);

		if (!is_viable(eye_pos, vec_bone, ent))
			continue;

		float cur_fov = get_fov(_cur_angles, Math::CalcAngle(eye_pos, vec_bone));

		if (cur_fov < best_fov) {
			best_fov = cur_fov;
			_target_id = k;
		}
	}

	return _target_id != -1;
}

Vector c_legitbot::find_best_bone(C_BasePlayer* m_player)
{
	auto eye_pos = cheat::main::local()->GetEyePosition();

	if (/*nearest_lock && */_hitbox_id != -1)
	{
		auto vec = cheat::features::aimbot.get_hitbox(m_player, _hitbox_id);

		if (!vec.IsZero()) {
			if (is_viable(eye_pos, vec, m_player))
				return vec;
		}
	}

	auto best_fov = FLT_MAX;
	auto best_bone_pos = Vector::Zero;

	for (auto i = 0; i < 7; i++)
	{
		auto vec = cheat::features::aimbot.get_hitbox(m_player, i)/*m_player->get_bone_pos(i)*/;

		if (vec.IsZero()) continue;

		if (!cheat::Cvars.Legit_head[cheat::main::local()->get_weapon()->GetWeaponType() - 1].GetValue() && i <= 1 ||
			!cheat::Cvars.Legit_chest[cheat::main::local()->get_weapon()->GetWeaponType() - 1].GetValue() && (i >= HITBOX_THORAX && i <= HITBOX_UPPER_CHEST) ||
			!cheat::Cvars.Legit_stomach[cheat::main::local()->get_weapon()->GetWeaponType() - 1].GetValue() && (i >= HITBOX_PELVIS && i <= HITBOX_BODY))
			continue;

		auto flFov = get_fov(_cur_angles, Math::CalcAngle(eye_pos, vec));

		if (flFov < best_fov) 
		{
			if (!is_viable(eye_pos, vec, m_player))
				continue;

			best_fov = flFov;
			best_bone_pos = vec;
			_hitbox_id = i;
		}
	}

	return best_bone_pos;
}

bool c_legitbot::find_best_aim_spot(float fov, C_BasePlayer* m_player)
{
	auto best_spot = Vector::Zero;
	auto eye_pos = cheat::main::local()->GetEyePosition();

	//if (nearest)
		best_spot = find_best_bone(m_player);
	//else
	//	vecBestTarget = m_player->get_bone_pos(menu_hitbox);

	//if (!nearest)
	//	_hitbox_id = menu_hitbox;

	if (best_spot.IsZero())
		return false;

	QAngle best_aim_angle = Math::CalcAngle(eye_pos, best_spot);

	auto final_fov = get_fov(_cur_angles, best_aim_angle);

	if (final_fov > fov)
		return false;

	if (_hitbox_id == -1 || !is_viable(eye_pos, best_spot, m_player))
		return false;

	//Source::m_pDebugOverlay->AddBoxOverlay(best_spot, Vector(-3, -3, -3), Vector(3, 3, 3), Vector(0, 0, 0), 255, 0, 0, 255, 2.f * Source::m_pGlobalVars->interval_per_tick);

	_hitbox_pos = best_spot;
	_aim_angles = best_aim_angle;

	return true;
}

QAngle c_legitbot::do_smooth(QAngle viewangles, QAngle target, float factor, bool randomize)
{
	QAngle delta = target - viewangles;
	//delta.Normalized();

	float smooth = powf(factor, 0.4f);
	if (randomize) {
		RandomSeed(Source::m_pGlobalVars->tickcount);
		smooth += RandomFloat(-0.11f, 0.11f);
	}
	smooth = min(0.99f, smooth);

	float coeff = fabsf(smooth - 1.f) / sqrt((delta.x * delta.x + delta.y * delta.y)) * 4.f;
	coeff = min(1.f, coeff);
	QAngle toChange = (delta * coeff);

	return (viewangles + toChange);
}

void c_legitbot::backtracking(CUserCmd* cmd, C_BasePlayer* m_player, int &time)
{
	if (!cheat::Cvars.LegitBot_PositionAdjustment.GetValue())
		return;

	auto player_index = m_player->entindex() - 1;
	auto player_record = &cheat::features::lagcomp.records[player_index];

	float best_fov = 999.f;
	int best_tick = -1;

	cheat::features::lagcomp.records[player_index].restore_record.data_filled = false;
	cheat::features::lagcomp.store_record_data(m_player, &cheat::features::lagcomp.records[player_index].restore_record);

	for (size_t index = 0; index < player_record->m_Tickrecords.size(); index++)
	{
		if ((index + 1) >(player_record->m_Tickrecords.size() - 1) || index >= player_record->m_Tickrecords.size())
			break;

		auto current_record = &player_record->m_Tickrecords.at(index);

		C_Tickrecord* prev_record = nullptr;

		if (player_record->m_Tickrecords.size() > 2 && index < (player_record->m_Tickrecords.size() - 1)) {
			if (!cheat::features::lagcomp.is_time_delta_too_large(current_record))
				prev_record = &player_record->m_Tickrecords.at(index + 1);
			else
				prev_record = nullptr;
		}

		if (cheat::features::lagcomp.is_time_delta_too_large(current_record))
			continue;

		C_Tickrecord corrected_record;
		auto tick_count = cheat::features::lagcomp.start_lag_compensation(m_player, index, &corrected_record);

		if (tick_count == -1 || !corrected_record.data_filled)
			continue;

		cheat::features::lagcomp.apply_record_data(m_player, &corrected_record);

		Vector vec_bone = m_player->get_bone_pos(8);

		if (!is_viable(cheat::main::local()->GetEyePosition(), vec_bone, m_player))
			continue;

		float cur_fov = get_fov(_cur_angles, Math::CalcAngle(cheat::main::local()->GetEyePosition(), vec_bone));

		if (cur_fov < best_fov) {
			best_fov = cur_fov;
			best_tick = index;
			time = tick_count;
		}
	}

	if (best_tick == -1)
		cheat::features::lagcomp.apply_record_data(m_player, &cheat::features::lagcomp.records[player_index].restore_record);
}

bool c_legitbot::triggerbot(CUserCmd* cmd)
{
	if (cheat::main::local() == nullptr || cheat::main::local()->IsDead())
		return false;

	static auto CustomDelay = 0.f;

	auto weapon = cheat::main::local()->get_weapon();

	if (weapon == nullptr || !cheat::main::local()->get_weapon()->IsGun() || !cheat::Cvars.LegitBot_Trigger_Enabled.GetValue())
		return false;

	if (cheat::Cvars.LegitBot_Trigger_OnKey.GetValue() && !cheat::game::pressed_keys[int(cheat::Cvars.LegitBot_Trigger_Key.GetValue())]) {
		CustomDelay = Source::m_pGlobalVars->realtime;
		return false;
	}
	else
	{
		if (cheat::Cvars.Legit_TDelay[weapon->GetWeaponType() - 1].GetValue() > 0.f)
		{
			if ((Source::m_pGlobalVars->realtime - cheat::Cvars.Legit_TDelay[weapon->GetWeaponType() - 1].GetValue()) >= CustomDelay)
				return false;
		}
	}

	Vector traceStart, traceEnd;
	trace_t tr;

	QAngle viewAngles;
	Source::m_pEngine->GetViewAngles(viewAngles);
	QAngle viewAngles_rcs = viewAngles + cheat::main::local()->m_aimPunchAngle() * 2.0f;

	Math::AngleVectors(viewAngles_rcs, &traceEnd);

	traceStart = cheat::main::local()->GetEyePosition();
	traceEnd = traceStart + (traceEnd * 8192.0f);

	/*cheat::Cvars.Legit_Triggerbot.PlaceControl("enabled", &cheat::Cvars.LegitBot_Trigger_Enabled);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("on key", &cheat::Cvars.LegitBot_Trigger_OnKey);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("key", &cheat::Cvars.LegitBot_Trigger_Key);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("through smoke", &cheat::Cvars.LegitBot_Trigger_IgnoreSmoke);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("auto pistol", &cheat::Cvars.LegitBot_Trigger_AutoPistol);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("adjust positions", &cheat::Cvars.LegitBot_Trigger_PositionAdjustment);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("trigger on teammates", &cheat::Cvars.LegitBot_Trigger_FriendlyFire);*/

	/*
	cheat::Cvars.Legit_TDelay[i - 1].Setup(0, 1000, "%.0fms");
		cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("shot delay", &cheat::Cvars.Legit_TDelay[i - 1], i);

		cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("account for hit chance", &cheat::Cvars.LegitBot_Trigger_account_hitchance[i - 1], i);
		cheat::Cvars.Legit_THitchance[i - 1].Setup(0, 100, "%.0f%%");
		cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("hit chance", &cheat::Cvars.Legit_THitchance[i - 1], i);
		cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("through walls", &cheat::Cvars.LegitBot_Trigger_IgnoreWall[i-1], i);
		cheat::Cvars.Legit_TMinDamage[i - 1].Setup(0, 150, "%.0fhp");
		cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("min damage", &cheat::Cvars.Legit_TMinDamage[i - 1], i);
	*/

	/*cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("head", &cheat::Cvars.Legit_thead[i - 1], i);
		cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("chest", &cheat::Cvars.Legit_tchest[i - 1], i);
		cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("stomach", &cheat::Cvars.Legit_tstomach[i - 1], i);
		cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("legs", &cheat::Cvars.Legit_tlegs[i - 1], i);
		cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("arms", &cheat::Cvars.Legit_tarms[i - 1], i);*/

	/*if (cheat::Cvars.LegitBot_Trigger_IgnoreWall[weapon->GetWeaponType() - 1].GetValue())
	{
		if (cheat::features::autowall.CanHit(cheat::main::local()->GetEyePosition(), traceEnd) <= 0.1f)
			return false;
	}
	else
	{*/
		Ray_t ray;
		ray.Init(traceStart, traceEnd);
		CTraceFilter traceFilter;
		traceFilter.pSkip = cheat::main::local();
		Source::m_pEngineTrace->TraceRay(ray, 0x46004003, &traceFilter, &tr);
	//}

	if (!tr.m_pEnt)
		return false;

	auto player = (C_BasePlayer*)tr.m_pEnt;

	if (!player
		|| !player->GetClientClass()
		|| player->GetClientClass()->m_ClassID != class_ids::CCSPlayer
		|| player == cheat::main::local()
		|| player->IsDormant()
		|| player->IsDead()
		|| player->m_bGunGameImmunity())
		return false;

	if (player->m_iTeamNum() == cheat::main::local()->m_iTeamNum() && !cheat::Cvars.LegitBot_Trigger_FriendlyFire.GetValue())
		return false;

	if (cheat::Cvars.LegitBot_Trigger_IgnoreSmoke.GetValue() && cheat::features::misc.goes_thru_smoke(traceStart, traceEnd))
		return false;

	int hitgroup = tr.hitgroup;

	bool didHit = false;

	if (cheat::Cvars.Legit_thead[weapon->GetWeaponType() - 1].GetValue() && hitgroup == HITGROUP_HEAD)
		didHit = true;
	if (cheat::Cvars.Legit_tchest[weapon->GetWeaponType() - 1].GetValue() && hitgroup == HITGROUP_CHEST)
		didHit = true;
	if (cheat::Cvars.Legit_tstomach[weapon->GetWeaponType() - 1].GetValue() && hitgroup == HITGROUP_STOMACH)
		didHit = true;
	if (cheat::Cvars.Legit_tarms[weapon->GetWeaponType() - 1].GetValue() && (hitgroup == HITGROUP_LEFTARM || hitgroup == HITGROUP_RIGHTARM))
		didHit = true;
	if (cheat::Cvars.Legit_tlegs[weapon->GetWeaponType() - 1].GetValue() && (hitgroup == HITGROUP_LEFTLEG || hitgroup == HITGROUP_RIGHTLEG))
		didHit = true;

	bool returnvalue;

	if (!didHit)
		return false;

	int hc;

	if (!cheat::Cvars.LegitBot_Trigger_account_hitchance[weapon->GetWeaponType() - 1].GetValue() || cheat::features::aimbot.hit_chance(Math::CalcAngle(cheat::main::local()->GetEyePosition(), traceEnd), player, cheat::Cvars.Legit_THitchance[weapon->GetWeaponType() - 1].GetValue(), tr.hitbox, 0.f, hc)) {
		if (weapon->m_iItemDefinitionIndex() == 64) {
			cmd->buttons |= IN_ATTACK2;
			returnvalue = true;
		}
		else
		{
			auto distance = player->m_vecOrigin().Distance(cheat::main::local()->m_vecOrigin());

			if ((weapon->m_iItemDefinitionIndex() == weapon_taser)) {
				if (distance < (200.0f - player->m_iHealth() / 10)) {
					cmd->buttons |= IN_ATTACK;
					returnvalue = true;
				}
			}
			else {
				cmd->buttons |= IN_ATTACK;
				returnvalue = true;
			}

		}
	}

	if (cheat::Cvars.LegitBot_Trigger_account_hitchance[weapon->GetWeaponType() - 1].GetValue())
	{
		if (weapon && (weapon->m_iItemDefinitionIndex() == weapon_awp || weapon->m_iItemDefinitionIndex() == weapon_ssg08 || weapon->m_iItemDefinitionIndex() == weapon_sg556 || weapon->m_iItemDefinitionIndex() == weapon_aug))
		{
			if (!cheat::main::local()->m_bIsScoped() && weapon->m_zoomLevel() <= 0)
			{
				cmd->buttons &= ~IN_ATTACK;
				cmd->buttons |= IN_ATTACK2;
				returnvalue = false;
			}
		}
	}
	return returnvalue;
}

bool c_legitbot::work(CUserCmd* cmd, bool& send_packet)
{
	_target_id = -1;
	_hitbox_pos.clear();
	//_hitbox_id = -1; // nearest lock require us to have this non-null if mouse1 key is hold
	_cur_angles = cmd->viewangles;
	_aim_angles.clear();

	/*
	cheat::Cvars.Legit_Accuracy[i - 1].PlaceControl("smooth", &cheat::Cvars.Legit_smooth[i-1], i);
	cheat::Cvars.Legit_Accuracy[i - 1].PlaceControl("randomize smooth", &cheat::Cvars.Legit_SmoothRandomize[i - 1], i);
	cheat::Cvars.Legit_Accuracy[i - 1].PlaceControl("rcs vertical", &cheat::Cvars.Legit_rcsY[i-1], i);
	cheat::Cvars.Legit_Accuracy[i - 1].PlaceControl("rcs horizontal", &cheat::Cvars.Legit_rcsX[i-1], i);//LegitBot_Nearest

	cheat::Cvars.Legit_fov[i - 1].Setup(0, 55, "%.0f°");
	cheat::Cvars.Legit_Targetting[i - 1].PlaceControl("fov", &cheat::Cvars.Legit_fov[i - 1], i);
	cheat::Cvars.Legit_firstshotdelay[i - 1].Setup(0, 1000, "%.0fms");
	cheat::Cvars.Legit_Targetting[i - 1].PlaceControl("first shot delay", &cheat::Cvars.Legit_firstshotdelay[i - 1], i);
	cheat::Cvars.Legit_killdelay[i - 1].Setup(0, 1000, "%.0fms");
	cheat::Cvars.Legit_Targetting[i - 1].PlaceControl("kill switch delay", &cheat::Cvars.Legit_killdelay[i - 1], i);

	cheat::Cvars.Legit_Filter[i - 1].PlaceControl("head", &cheat::Cvars.Legit_head[i-1], i);
	cheat::Cvars.Legit_Filter[i - 1].PlaceControl("chest", &cheat::Cvars.Legit_chest[i-1], i);
	cheat::Cvars.Legit_Filter[i - 1].PlaceControl("stomach", &cheat::Cvars.Legit_stomach[i-1], i);
	cheat::Cvars.Legit_Filter[i - 1].PlaceControl("legs", &cheat::Cvars.Legit_legs[i-1], i);
	cheat::Cvars.Legit_Filter[i - 1].PlaceControl("arms", &cheat::Cvars.Legit_arms[i-1], i);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("head", &cheat::Cvars.Legit_thead[i - 1], i);
	cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("chest", &cheat::Cvars.Legit_tchest[i - 1], i);
	cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("stomach", &cheat::Cvars.Legit_tstomach[i - 1], i);
	cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("legs", &cheat::Cvars.Legit_tlegs[i - 1], i);
	cheat::Cvars.Legit_TFilter[i - 1].PlaceControl("arms", &cheat::Cvars.Legit_tarms[i - 1], i);

	cheat::Cvars.Legit_TAccuracy[i - 1].SetupGroupbox(10, 230, 200, 150, &legitbot, trg_acc_s[i - 1], i);

	cheat::Cvars.Legit_TDelay[i - 1].Setup(0, 1000, "%.0fms");
	cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("shot delay", &cheat::Cvars.Legit_TDelay[i - 1], i);

	cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("account for hit chance", &cheat::Cvars.LegitBot_Trigger_account_hitchance[i - 1], i);
	cheat::Cvars.Legit_THitchance[i - 1].Setup(0, 100, "%.0f%%");
	cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("hit chance", &cheat::Cvars.Legit_THitchance[i - 1], i);
	cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("through walls", &cheat::Cvars.LegitBot_Trigger_IgnoreWall[i - 1], i);
	cheat::Cvars.Legit_TMinDamage[i - 1].Setup(0, 150, "%.0fhp");
	cheat::Cvars.Legit_TAccuracy[i - 1].PlaceControl("min damage", &cheat::Cvars.Legit_TMinDamage[i - 1], i);
	*/

	/*
	cheat::Cvars.Legit_GMain.PlaceControl("enabled", &cheat::Cvars.LegitBot_Enabled);
	cheat::Cvars.Legit_GMain.PlaceControl("on key", &cheat::Cvars.LegitBot_OnKey);
	cheat::Cvars.Legit_GMain.PlaceControl("key", &cheat::Cvars.LegitBot_Key);
	cheat::Cvars.Legit_GMain.PlaceControl("through smoke", &cheat::Cvars.LegitBot_IgnoreSmoke);
	cheat::Cvars.Legit_GMain.PlaceControl("auto pistol", &cheat::Cvars.LegitBot_AutoPistol);
	cheat::Cvars.Legit_GMain.PlaceControl("adjust positions", &cheat::Cvars.LegitBot_PositionAdjustment);
	cheat::Cvars.Legit_GMain.PlaceControl("aim at teammates", &cheat::Cvars.LegitBot_FriendlyFire);
	*/

	if (cheat::Cvars.RageBot_Enable.GetValue())
		return false;

	if (cheat::features::antiaimbot.drop)
	{
		Source::m_pEngine->ClientCmd_Unrestricted("drop");
		cheat::features::antiaimbot.drop = false;
	}

	if (triggerbot(cmd))
		return false;

	if (!cheat::Cvars.LegitBot_Enabled.GetValue())
		return false;

	if (cheat::main::local() == nullptr || cheat::main::local()->IsDead())
		return false;

	auto wpn = cheat::main::local()->get_weapon();

	if (wpn == nullptr || !wpn->IsGun())
		return false;

	static float first_shot	= 0.f;
	auto id					= wpn->m_iItemDefinitionIndex();
	auto fov				= cheat::Cvars.Legit_fov[wpn->GetWeaponType() - 1].GetValue();
	auto first_shot_delay	= cheat::Cvars.Legit_firstshotdelay[wpn->GetWeaponType() - 1].GetValue();
	auto kill_delay			= cheat::Cvars.Legit_killdelay[wpn->GetWeaponType() - 1].GetValue();
	auto smooth				= cheat::Cvars.Legit_smooth[wpn->GetWeaponType() - 1].GetValue() / 100.f;
	auto smooth_randomize	= cheat::Cvars.Legit_SmoothRandomize[wpn->GetWeaponType() - 1].GetValue() == 1.f;
	auto rcsy				= cheat::Cvars.Legit_rcsY[wpn->GetWeaponType() - 1].GetValue();
	auto rcsx				= cheat::Cvars.Legit_rcsX[wpn->GetWeaponType() - 1].GetValue();

	auto m_angRcsAngle = QAngle(0, 0, 0);

	if (id != weapon_awp && id != weapon_ssg08) {
		m_angRcsAngle = cheat::main::local()->m_aimPunchAngle();


		m_angRcsAngle.x *= (rcsx / 50.f);
		m_angRcsAngle.y *= (rcsy / 50.f);
	}

	int backtrack = -1;

	if ((cheat::game::pressed_keys[(int)cheat::Cvars.LegitBot_Key.GetValue()] || cheat::game::pressed_keys[2] && id == 64 || !cheat::Cvars.LegitBot_OnKey.GetValue()) && !cheat::features::menu.menu_opened)
	{
		_cur_angles = cmd->viewangles + m_angRcsAngle;

		if ((cheat::main::last_kill_time + (kill_delay / 1000.f)) > Source::m_pGlobalVars->realtime)
			return false;

		if (!find_target(fov))
			return false;

		if (_target_id == -1)
			return false;

		auto m_player = Source::m_pEntList->GetClientEntity(_target_id);
		
		if (m_player == nullptr)
			return false;

		backtracking(cmd, m_player, backtrack);

		if (!find_best_aim_spot(fov, m_player))
			return false;

		cheat::features::lagcomp.apply_record_data(m_player, &cheat::features::lagcomp.records[_target_id-1].restore_record);

		auto dest_angle = do_smooth(_cur_angles, _aim_angles, smooth, smooth_randomize) - m_angRcsAngle;

		dest_angle.Clamp();

		if (first_shot > Source::m_pGlobalVars->realtime && cheat::main::local()->m_iShotsFired() < 1)
			cmd->buttons &= ~IN_ATTACK;
		//else
		//	cmd->buttons |= IN_ATTACK;

		if (backtrack > 0)
			cmd->tick_count = backtrack;

		cmd->viewangles = dest_angle;
		Source::m_pEngine->SetViewAngles(dest_angle);

		return true;
	}
	else {
		_hitbox_id = -1;
		first_shot = (Source::m_pGlobalVars->realtime + (first_shot_delay / 1000.f));
	}

	/*
	cheat::Cvars.Legit_Triggerbot.PlaceControl("enabled", &cheat::Cvars.LegitBot_Trigger_Enabled);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("on key", &cheat::Cvars.LegitBot_Trigger_OnKey);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("key", &cheat::Cvars.LegitBot_Trigger_Key);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("through smoke", &cheat::Cvars.LegitBot_Trigger_IgnoreSmoke);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("auto pistol", &cheat::Cvars.LegitBot_Trigger_AutoPistol);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("adjust positions", &cheat::Cvars.LegitBot_Trigger_PositionAdjustment);
	cheat::Cvars.Legit_Triggerbot.PlaceControl("trigger on teammates", &cheat::Cvars.LegitBot_Trigger_FriendlyFire);
	*/



	return false;
}