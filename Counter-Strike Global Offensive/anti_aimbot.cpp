#include "aimbot.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "math.hpp"
#include "displacement.hpp"
#include "anti_aimbot.hpp"
#include "lag_compensation.hpp"
#include "prediction.hpp"
#include "autowall.hpp"
#include <algorithm>
#include "angle_resolver.hpp"
#include <iostream>
#include "rmenu.hpp"
#include "movement.hpp"

using SendNetMsg = bool(__thiscall*)(INetChannel*, INetMessage&, bool, bool);

float GetRotatedYaw(float lby, float yaw)
{
	float delta = Math::NormalizeYaw(yaw - lby);
	if (fabs(delta) < 25.f)
		return lby;

	if (delta > 0.f)
		return yaw + 25.f;

	return yaw;
}

bool IsPressingMovementKeys(CUserCmd* cmd)
{
	if (!cmd)
		return false;

	if (cmd->buttons & IN_FORWARD ||
		cmd->buttons & IN_BACK || cmd->buttons & IN_LEFT || cmd->buttons & IN_RIGHT ||
		cmd->buttons & IN_MOVELEFT || cmd->buttons & IN_MOVERIGHT)
		return true;

	return false;
}

Vector TraceEnd(Vector start, Vector end)
{
	trace_t trace;
	CTraceFilterWorldOnly filter;
	Ray_t ray;

	ray.Init(start, end);
	Source::m_pEngineTrace->TraceRay(ray, MASK_ALL, &filter, &trace);

	return trace.endpos;
}

struct angle_data {
	float angle;
	float thickness;
	angle_data(const float angle, const float thickness) : angle(angle), thickness(thickness) {}
};

float quick_normalize(float degree, const float min, const float max) {
	while (degree < min)
		degree += max - min;
	while (degree > max)
		degree -= max - min;

	return degree;
}

bool trace_to_exit_short(Vector &point, Vector &dir, const float step_size, float max_distance)
{
	float flDistance = 0;

	while (flDistance <= max_distance)
	{
		flDistance += step_size;

		point += dir * flDistance;

		auto contents = Source::m_pEngineTrace->GetPointContents(point);

		if ((contents & MASK_SHOT_HULL && contents & CONTENTS_HITBOX) == 0)
		{
			// found first free point
			return true;
		}
	}

	return false;
}

float get_thickness(Vector& start, Vector& end, C_BaseEntity* efilter) {
	Vector dir = end - start;
	Vector step = start;
	dir /= dir.Length();
	CTraceFilter filter;
	filter.pSkip = efilter;
	trace_t trace;
	Ray_t ray;
	float thickness = 0;
	while (true) {
		ray.Init(step, end);
		Source::m_pEngineTrace->TraceRay(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

		if (!trace.DidHit())
			break;

		const Vector lastStep = trace.endpos;
		step = trace.endpos;

		if ((end - start).Length() <= (step - start).Length())
			break;

		if (!trace_to_exit_short(step, dir, 5, 90))
			return FLT_MAX;

		thickness += (step - lastStep).Length();
	}
	return thickness;
}

void c_antiaimbot::freestand(C_BasePlayer *ent, float& new_angle) {
	std::vector<angle_data> points;

	if (!ent)
		return;

	if (ent->IsDead())
		return;

	const auto local_position = ent->GetEyePosition();
	std::vector<float> scanned = {};

	for (auto i = 0; i <= 64; i++)
	{
		auto p_entity = (C_BasePlayer*)(Source::m_pEntList->GetClientEntity(i));
		if (p_entity == nullptr) continue;
		if (p_entity == ent) continue;
		if (p_entity->IsDead()) continue;
		if (p_entity->m_iTeamNum() == ent->m_iTeamNum()) continue;
		if (p_entity->IsDormant()) continue;
		if (!p_entity->IsPlayer()) continue;

		const auto view = Math::CalcAngle(local_position, p_entity->GetEyePosition());

		std::vector<angle_data> angs;

		for (auto y = 1; y < 5; y++)
		{
			auto ang = quick_normalize((y * 90) + view.y, -180.f, 180.f);
			auto found = false; // check if we already have a similar angle

			for (auto i2 : scanned)
				if (abs(quick_normalize(i2 - ang, -180.f, 180.f)) < 20.f)
					found = true;

			if (found)
				continue;

			points.emplace_back(ang, -1.f);
			scanned.push_back(ang);
		}
		//points.push_back(base_angle_data(view.y, angs)); // base yaws and angle data (base yaw needed for lby breaking etc)
	}

	for (auto i = 0; i <= 64; i++)
	{
		auto p_entity = (C_BasePlayer*)(Source::m_pEntList->GetClientEntity(i));
		if (p_entity == nullptr) continue;
		if (p_entity == ent) continue;
		if (p_entity->IsDead()) continue;
		if (p_entity->m_iTeamNum() == ent->m_iTeamNum()) continue;
		if (p_entity->IsDormant()) continue;
		if (!p_entity->IsPlayer()) continue;

		auto found = false;
		auto points_copy = points; // copy data so that we compair it to the original later to find the lowest thickness
		auto enemy_eyes = p_entity->GetEyePosition();

		for (auto &z : points_copy) // now we get the thickness for all of the data
		{
			const Vector tmp(10, z.angle, 0.0f);
			Vector head;
			Math::AngleVectors(tmp, &head);
			head *= ((16.0f + 3.0f) + ((16.0f + 3.0f) * sin(DEG2RAD(10.0f)))) + 7.0f;
			head += local_position;
			head.z = local_position.z;
			const auto local_thickness = get_thickness(head, enemy_eyes, ent);	//i really need my source for this bit, i forgot how it works entirely Autowall::GetThickness1(head, hacks.m_local_player, p_entity);
			z.thickness = local_thickness;

			if (local_thickness != 0.f) // if theres a thickness of 0 dont use this data
				found = true;
		}

		if (!found) // dont use
			continue;

		for (size_t z = 0; points_copy.size() > z; z++)
			if (points_copy[z].thickness < points[z].thickness || points[z].thickness == -1) // find the lowest thickness so that we can hide our head best for all entities
				points[z].thickness = points_copy[z].thickness;

	}
	float best = -1.f;
	for (auto &i : points)
		if ((i.thickness > best || i.thickness == -1) && i.thickness != 0) // find the best hiding spot (highest thickness)
		{
			best = i.thickness;
			new_angle = i.angle;
		}

	if (best < 0.f)
		new_angle = FLT_MAX;
}

void c_antiaimbot::DoLBY(CUserCmd* cmd, bool *send_packet)
{
	if (!cheat::main::local())
		return;

	auto curtime = Source::m_pGlobalVars->curtime;
	auto animstate = cheat::main::local()->get_animation_state();
	static float prev_angles = 0.f;

	if (!animstate)
		return;

	//if (*send_packet)

	//cheat::features::antiaimbot.m_will_lby_update = false;

	//if (animstate->speed_2d > 0.1f || fabs(animstate->speed_up) > 100.f)
	//	cheat::features::antiaimbot.m_next_lby_update_time = curtime + 0.22f;
	//else
	//{
	//	//auto v60 = Math::AngleDiff(/*cheat::main::local()->m_flLowerBodyYawTarget()*/animstate->abs_yaw, animstate->eye_yaw);

	//	//if (fabs(v60) > 35.f) 
	//	{
	//		/*if (static auto lby = cheat::main::local()->m_flLowerBodyYawTarget(); Math::AngleDiff(lby, cheat::main::local()->m_flLowerBodyYawTarget()) >= 35.f)
	//		{
	//			cheat::features::antiaimbot.m_next_lby_update_time = curtime + 1.1f;
	//			lby = cheat::main::local()->m_flLowerBodyYawTarget();
	//		}*/

	//		auto parasha = fabs(Math::AngleDiff(pasta.y, prev_angles));

	//		if ((cheat::features::antiaimbot.m_next_lby_update_time - curtime) < 0.0f) {

	//			if (parasha > 35.f) {
	//				cheat::features::antiaimbot.m_next_lby_update_time = curtime + 1.1f;
	//				cheat::features::antiaimbot.m_last_lby_update = curtime;
	//				cheat::features::antiaimbot.m_will_lby_update = true;
	//			}
	//		}
	//	}
	//}

	//prev_angles = pasta.y;
}

void target_aim(CUserCmd* cmd,bool& pass) {
	auto angle = cmd->viewangles;
	float lowest = 99999999.f;
	Vector EyePos = cheat::main::local()->m_vecOrigin();

	for (int i = 1; i < 65; i++) {
		if (i == Source::m_pEngine->GetLocalPlayer()) continue;
		C_BasePlayer* pEnt = Source::m_pEntList->GetClientEntity(i);
		if (!pEnt) continue;
		if (pEnt->IsDormant()) continue;
		if (pEnt->IsDead()) continue;
		if (pEnt->m_iTeamNum() == cheat::main::local()->m_iTeamNum()) continue;

		//if (Vars.Ragebot.Antiaim.no_enemy)
		pass = true;

		Vector CurPos = pEnt->m_vecOrigin();

		auto dist = CurPos.Distance(EyePos);

		if (dist < lowest) {
			lowest = dist;
			angle = Math::CalcAngle(EyePos, CurPos);
		}
	}

	cmd->viewangles = angle;
}

void c_antiaimbot::change_angles(CUserCmd * cmd, bool &send_packet)
{
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

	static float desync_angle = 0.f;
	static float prev_desync_state = 0.f;
	float speed_fraction_clamped = 0.f, v56 = 0.f;

	float last_freestand_angle = 0.f, last_freestand_change_time = 0.f;

	auto animstate = cheat::main::local()->get_animation_state();

	if (!animstate || !cheat::Cvars.Antiaim_enable.GetValue() || cheat::features::antiaimbot.enable_delay > Source::m_pGlobalVars->curtime  /* || fabs(last_drop_time - Source::m_pGlobalVars->realtime) < 0.2f*/) {
		visual_exploit_angle = last_real_angle = visual_fake_angle = last_sent_angle = cmd->viewangles;
		no_aas = true;
		return;
	}

	/*if (cheat::Cvars.RageBot_exploits.GetValue() == 3 && !(cmd->command_number & 1) && !(cheat::main::fakeducking && cheat::main::local()->m_fFlags() & FL_ONGROUND))
	{
		switch ((int)cheat::Cvars.anti_aim_ex_pitch.GetValue())
		{
		case 1:
			cmd->viewangles.x = -89.f;
			break;
		case 2:
			cmd->viewangles.x = 89.f;
			break;
		case 3:
			cmd->viewangles.x = 0.f;
			break;
		default:
			break;
		}

		switch ((int)cheat::Cvars.anti_aim_ex_yaw.GetValue())
		{
		case 1:
			cmd->viewangles.y += 180.f;
			break;
		case 2:
			cmd->viewangles.y = last_real_angle.y + 180.f;
			break;
		case 3:
			cmd->viewangles.y = cheat::main::local()->m_flLowerBodyYawTarget() - 90.f * cheat::main::fside;
			break;
		case 4:
			cmd->viewangles.y += Math::normalize_angle(Source::m_pGlobalVars->curtime * 250.f);
			break;
		default:
			break;
		}

		if (fabs(cheat::Cvars.anti_aim_ex_addyaw.GetValue()) > 0.0f)
			cmd->viewangles.y += cheat::Cvars.anti_aim_ex_addyaw.GetValue();

		cheat::features::antiaimbot.shift_ticks = 6;
		cheat::features::antiaimbot.do_tickbase = true;
		visual_fake_angle = visual_exploit_angle = cmd->viewangles;
		last_sent_origin = cheat::main::local()->m_vecOrigin();

		return;
	}*/

	auto max_rotation = cheat::features::aaa.sub_59B13C30(animstate);

	auto is_moving = ((fabs(cheat::main::local()->m_vecVelocity().Length2D()) > 9.f && animstate->t_since_started_moving > 0.5f && cheat::main::local()->m_fFlags() & FL_ONGROUND && !cheat::main::fakewalking));

	int aa_mode = int(is_moving ? cheat::Cvars.anti_aim_m_yaw.GetValue() : cheat::Cvars.anti_aim_s_yaw.GetValue());
	float aa_add = float(is_moving ? cheat::Cvars.anti_aim_m_addyaw.GetValue() : cheat::Cvars.anti_aim_s_addyaw.GetValue());

	int aa_switch = int(is_moving ? cheat::Cvars.anti_aim_m_switchmode.GetValue() : cheat::Cvars.anti_aim_s_switchmode.GetValue());
	int aa_switchspeed = float(is_moving ? cheat::Cvars.anti_aim_m_switchspeed.GetValue() : cheat::Cvars.anti_aim_s_switchspeed.GetValue());
	int aa_switchang = int(is_moving ? cheat::Cvars.anti_aim_m_switchangle.GetValue() : cheat::Cvars.anti_aim_s_switchangle.GetValue());

	bool nreturn = false;
	if (cheat::Cvars.Antiaim_attargets.GetValue() && cheat::main::side == -1)
	{
		target_aim(cmd, nreturn);

		/*if (!nreturn) {
			last_real_angle = last_sent_angle = cmd->viewangles;
			return;
		}*/
	}

	/*if ((!(Engine::Prediction::Instance()->GetFlags() & FL_ONGROUND) && cheat::main::local()->m_fFlags() & FL_ONGROUND) && cheat::Cvars.Misc_AntiUT.GetValue() <= 0)
		cmd->viewangles.x = -539.9885f;
	else */
	//{
		switch ((int)cheat::Cvars.anti_aim_pitch.GetValue())
		{
		case 1:
			cmd->viewangles.x = 179.0f;
			break;
		case 2:
			cmd->viewangles.x = 0.f;
			break;
		default:
			break;
		}
	//}

	static float lby = 0;

	if (cheat::main::side != -1)
	{
		switch (cheat::main::side)
		{
		case 0:
			cmd->viewangles.y += 90.f;
			break;
		case 1:
			cmd->viewangles.y -= 90.f;
			break;
		default:
			cmd->viewangles.y += 178.f;
			break;
		}
	}
	else {
		switch (aa_mode)
		{
		case 1:
			cmd->viewangles.y += 178.f;
			break;
		case 2:
			break;
		case 3:
			cmd->viewangles.y = cheat::main::local()->m_flLowerBodyYawTarget() + 90.f;
			break;
		case 4:
			cmd->viewangles.y += Math::normalize_angle(Source::m_pGlobalVars->curtime * 250.f);
			break;
		default:
			break;
		}
	}

	//static auto allow_change = false;

	cmd->viewangles.y = Math::normalize_angle(cmd->viewangles.y);

	//if (cheat::main::side == -1 && cheat::Cvars.anti_aim_freestand.GetValue())
	//{
	//	float new_angle = cmd->viewangles.y;
	//	/*new_*/freestand(cheat::main::local(), new_angle);
	//
	//	if (new_angle != FLT_MAX && cheat::main::side == -1)
	//		cmd->viewangles.y = Math::normalize_angle(new_angle);
	//	/*else
	//	{
	//		if (new_angle != FLT_MAX) {
	//
	//			if (new_angle != last_freestand_angle)
	//			{
	//				if (fabs(last_freestand_change_time - Source::m_pGlobalVars->realtime) < 4.f)
	//					new_angle = last_freestand_angle;
	//				else
	//					last_freestand_change_time = Source::m_pGlobalVars->realtime;
	//			}
	//
	//			cheat::main::fside = (Math::AngleDiff(new_angle, cmd->viewangles.y + 180.f) > 0.f ? -1 : 1);
	//
	//			last_freestand_angle = new_angle;
	//		}
	//	}*/
	//	desync_angle = ;
	//	copysign(max_rotation, desync_angle);
	//}

	//if (allow_change)
	//	allow_change = !cheat::main::fakewalking && animstate->speed_2d > 10.f;

	if (max_rotation != 0.f)
		max_rotation -= 0.5f;

	const bool extend_fake = cheat::features::antiaimbot.extend && cheat::Cvars.anti_aim_desync_extend_limit_on_shot.GetValue();

	if (!extend_fake && cmd->command_number % 4 < 2 && cheat::Cvars.anti_aim_desync_switch_limit.GetValue())
		max_rotation *= 0.6f;

	auto final_rotation = Math::clamp(max_rotation, 0.f, 60.f * ((cheat::main::fside < 0 ? cheat::Cvars.anti_aim_desync_range.GetValue() : cheat::Cvars.anti_aim_desync_range_right.GetValue()) * 0.01f));

	if (extend_fake)
		final_rotation = max_rotation;

	if (fabs(aa_add) > 0.0f)
		cmd->viewangles.y += aa_add;

	static auto is_inverted = false;
	auto flick_lby = false;

	if (fabs(cheat::main::fside) != 1 && Source::m_pClientState->m_iChockedCommands == 1)
	{
		is_inverted = (cheat::main::fside > 0);
		send_packet = 1;

		cheat::main::fside /= 2;
		flick_lby = true;
	}
	else
		is_inverted = (cheat::main::fside > 0);

	if (cheat::Cvars.anti_aim_desync.GetValue() == 1.f) {
		if (!is_inverted)
			cmd->viewangles.y += 26.f;
		else
			cmd->viewangles.y -= 26.f;
	}

	last_real_angle = cmd->viewangles;

	if (aa_switch > 0)
	{
		static float last_switchang = 0.f;
		static float last_switchtime = 0.f;

		switch (aa_switch)
		{
			case 1:
			{
				if (fabs(last_switchtime - Source::m_pGlobalVars->realtime) > (1.f - (aa_switchspeed * 0.01f))) {
					auto lol = abs(aa_switchang);
					RandomSeed(cmd->command_number & 255);
					last_switchang = RandomFloat(-lol, lol);
					last_switchtime = Source::m_pGlobalVars->realtime;
				}

				break;
			}
			case 2:
			{
				if (fabs(cmd->viewangles.y - last_switchang) < fabs(aa_switchang))
					last_switchang += copysign(aa_switchspeed * 0.1f, aa_switchang);
				else
					last_switchang = 0.f;

				//last_switchtime = Source::m_pGlobalVars->realtime;

				break;
			}
		}

		cmd->viewangles.y += last_switchang;
	}

	//final_rotation = Math::clamp(abs(cmd->viewangles.y - last_real_angle.y), 0.f, 60.f);

	//auto lol = ((Source::m_pClientState->m_iChockedCommands < 5 && cheat::Cvars.RageBot_exploits.GetValue() > 0) || cheat::Cvars.RageBot_exploits.GetValue() == 0) && Source::m_pClientState->m_iChockedCommands < 14;

		switch ((int)cheat::Cvars.anti_aim_desync.GetValue())
		{
		case 0:
			break;
		case 1:
		{
			if (send_packet)
			{
				//cmd->viewangles.y = last_real_angle.y + (final_rotation * cheat::main::fside);

				if (is_inverted)
					cmd->viewangles.y = last_real_angle.y + final_rotation;
				else
					cmd->viewangles.y = last_real_angle.y - final_rotation;

				//visual_real_angle = cmd->viewangles;
			}
			else 
			{
				if (is_inverted)
					cmd->viewangles.y = last_real_angle.y - final_rotation;
				else
					cmd->viewangles.y = last_real_angle.y + final_rotation;

				last_sent_angle = visual_fake_angle = cmd->viewangles;
			}
		}
			break;
		case 2:

			////if ((m_will_lby_update && Source::m_pClientState->m_iChockedCommands < 14 && !send_packet && lol) /*|| (cheat::main::local()->m_vecVelocity().Length2D() > 9.f && send_packet)*/)
			////{
			////	cmd->viewangles.y = last_real_angle.y + 120.f * cheat::main::fside;

			////	if (send_packet)
			////		last_sent_angle = visual_fake_angle = cmd->viewangles;
			////	else
			////		visual_real_angle = cmd->viewangles;

			////	m_will_lby_update = false;
			////}
			////else if (!send_packet && lol)
			////{
			////	cmd->viewangles.y = last_real_angle.y - ((max_rotation /*+ 30.f*/) * cheat::main::fside);
			////	visual_real_angle = cmd->viewangles;
			////}
			////else 
			////	last_sent_angle = visual_fake_angle = cmd->viewangles;

			//if (cmd->sidemove == 0.f && fabs(cheat::main::fside) != 1)
			//{
			//	cmd->viewangles.y = last_real_angle.y - 120.f * cheat::main::fside;

			//	cheat::main::fside /= 2;
			//}
			//else if (send_packet && lol)
			//{
			//	cmd->viewangles.y = last_real_angle.y + final_rotation * cheat::main::fside;
			//	visual_real_angle = cmd->viewangles;
			//}
			////if (send_packet) {

			///*if (TICKS_TO_TIME(cheat::main::local()->m_nTickBase()) > cheat::features::antiaimbot.m_next_lby_update_time && Source::m_pClientState->m_iChockedCommands < 14 && !send_packet)
			//{
			//cmd->viewangles.y = last_real_angle.y + (final_rotation + 30.f) * cheat::main::fside;

			//if (cmd->command_number % 2 == 0)
			//cmd->sidemove -= 5;
			//else
			//cmd->sidemove += 5;
			//}
			//else
			//{*/
			////	cmd->viewangles.y = last_real_angle.y + (final_rotation/* + (max_rotation * animstate->speed_norm)*/)* cheat::main::fside;
			////	visual_real_angle = cmd->viewangles;
			////}
			//else
			//	last_sent_angle = visual_fake_angle = cmd->viewangles;

			{
				//int jitter_side = cheat::main::fside;
				//static int m_iRotate = 0;
				//int rotate = m_iRotate;
				////if (g_Options.rage.antiaim.autodirection && m_bAutomaticDir) {
				////	jitter_side = m_iAutoDirection;
				////	g_Context->m_Cmd->viewangles.y += 90.0f * static_cast<float>(m_iAutoDirection);
				////}
				////else {
				//cmd->viewangles.y += 90.0f;
				////}

				//float desync = max_rotation;
				//float inverse_desync = 190.0f - desync;
				//float jitter = 180.0f - inverse_desync * 0.5f;

				//if (jitter_side == 1)
				//	cmd->viewangles.y += jitter;
				//else if (jitter_side == -1)
				//	cmd->viewangles.y -= jitter;

				//rotate--;
				//if (rotate) {
				//	if (rotate == 1) {
				//		if (jitter_side == 1)
				//			cmd->viewangles.y += inverse_desync;
				//		else
				//			cmd->viewangles.y += desync - 190.0f;
				//	}
				//}
				//else {
				//	if (jitter_side == 1)
				//		cmd->viewangles.y += desync - 190.0f;
				//	else
				//		cmd->viewangles.y += inverse_desync;

				//	send_packet = false;
				//	last_sent_angle = visual_fake_angle = cmd->viewangles;
				//}
				//if (++m_iRotate >= 3)
				//	m_iRotate = 0;

				static bool invert_jitter = false;

				if (invert_jitter) {
					cmd->viewangles.y = last_real_angle.y + 180.0f;
					last_real_angle.y = cmd->viewangles.y;
				}
				
				if (send_packet) {	
					invert_jitter = !invert_jitter;

					if (!invert_jitter)
						cmd->viewangles.y = (last_real_angle.y + (max_rotation * 0.5f) * cheat::main::fside);
					else
						cmd->viewangles.y = (last_real_angle.y - (max_rotation * 0.5f) * cheat::main::fside);
				}
				else if (!flick_lby) {
					if (invert_jitter)
						cmd->viewangles.y = (last_real_angle.y - (max_rotation + 5.0f) * cheat::main::fside);
					else
						cmd->viewangles.y = (last_real_angle.y + (max_rotation + 5.0f) * cheat::main::fside);

					last_sent_angle = visual_fake_angle = cmd->viewangles;
				}
				else {
					if (invert_jitter)
						cmd->viewangles.y = (last_real_angle.y + 90.0f * cheat::main::fside);
					else
						cmd->viewangles.y = (last_real_angle.y - 90.0f * cheat::main::fside);
				}


			}

			break;
		}
	//else
	//	visual_real_angle = cmd->viewangles;

	//if ((int)cheat::Cvars.RageBot_exploits.GetValue() == 3)
	//	visual_real_angle = cmd->viewangles;

	//if (cheat::Cvars.anti_aim_desync.GetValue()) {
	//	auto desync_mode = cheat::Cvars.anti_aim_desync_range.GetValue() > 34.f;

	//	if (desync_mode) {
	//		if (cheat::Cvars.anti_aim_desync_range.GetValue() > 1.f && cheat::Cvars.anti_aim_desync_range.GetValue() < 58.f)
	//			Math::clamp(max_rotation, 0.f, cheat::Cvars.anti_aim_desync_range.GetValue());

	//		do_desync(cmd, send_packet, max_rotation);
	//	}
	//	else
	//	{
	//		/*if (desync_angle != 0.f) {
	//			Math::normalize_angle(desync_angle);
	//			desync_angle = Math::clamp(desync_angle, -max_rotation, max_rotation);
	//		}
	//		else {
	//			desync_angle = max_rotation;
	//			Math::normalize_angle(desync_angle);
	//		}*/

	//		//auto should_fake = (cmd->sidemove > 1.f) ? send_packet : !send_packet;

	//		auto static_rotation = max_rotation * 0.72f;

	//		if (static_rotation > 34.f)
	//			static_rotation = 34.f;

	//		if (cheat::Cvars.anti_aim_desync_range.GetValue() > 1.f && cheat::Cvars.anti_aim_desync_range.GetValue() < 35.f)
	//			static_rotation = Math::clamp(max_rotation, 0.f, cheat::Cvars.anti_aim_desync_range.GetValue());

	//		/*if (!send_packet) {
	//			if (animstate->m_flSpeed2D >= 0.1f && animstate->m_flSpeed2D < 10.f)
	//				static_rotation = max_rotation;
	//			else
	//				static_rotation = Math::clamp(max_rotation, 0.f, 34.f);

	//			cmd->viewangles.y = last_real_angle.y - static_rotation;
	//		}*/

	//		cmd->viewangles.y -= static_rotation;

	//		if (!m_will_lby_update) {
	//			if (send_packet)
	//				cmd->viewangles.y += max_rotation;
	//			else
	//				cmd->viewangles.y -= static_rotation;

	//		}
	//		else {
	//			cmd->viewangles.y += max_rotation;
	//			m_will_lby_update = false;
	//		}
	//	}

	//	if (send_packet)
	//		last_sent_angle = cmd->viewangles;
	//}

	//auto desync_mode = cheat::settings.ragebot_anti_aim_settings[2].desync_mode;

	//if (desync_mode == 4) {
	//	if (cheat::main::local()->m_vecVelocity().Length2D() < 0.1f && cheat::features::antiaimbot.packets_choked > 0 && !send_packet)
	//		send_packet = true;

	//	if (send_packets < 2 && send_packets != 0 && !send_packet)
	//		send_packet = true;

	//	if (send_packets > 1 && send_packet) {
	//		send_packet = false;
	//		send_packets = 0;
	//	}
	//}

	//static auto v24 = 0.f;

	//if (desync_mode == 2)
	//	v24 = -1.f;
	//else
	//	v24 = 1.f;

	//if (m_will_lby_update && cheat::settings.ragebot_anti_aim_break_lby && desync_mode != 1)
	//{
	//	cmd->viewangles.y += delta_minus_180;
	//	m_last_attempted_lby = delta_minus_180;
	//	m_will_lby_update = false;
	//}
	//else {
	//	if (send_packet) {
	//		if (desync_mode == 1)
	//			cmd->viewangles.y -= 100.f;
	//		else if (desync_mode != 4)
	//			cmd->viewangles.y += (max_rotation + 30.0f) * v24;
	//		else
	//			cmd->viewangles.y += nigger ? (-100.f) : (delta_minus_180 * -1);
	//		//else
	//		//	cmd->viewangles.y -= nigger ? (max_rotation - RandomInt(0, 20)) : ((max_rotation - RandomInt(0, 20)) * -1);

	//		nigger = !nigger;

	//		//std::cout << "send  | " << Math::normalize_angle(cmd->viewangles.y) << " | " << Math::normalize_angle(cheat::main::local()->get_abs_eye_angles().y) << " | " << Math::normalize_angle(cheat::main::local()->m_flLowerBodyYawTarget()) << " | " << Math::normalize_angle(cmd->viewangles.y - cheat::main::local()->get_abs_eye_angles().y) << std::endl;
	//	}
	//	else {
	//		nigger = false;

	//		//std::cout << "choke | " << Math::normalize_angle(cmd->viewangles.y) << " | " << Math::normalize_angle(cheat::main::local()->get_abs_eye_angles().y) << " | " << Math::normalize_angle(cheat::main::local()->m_flLowerBodyYawTarget()) << " | " << Math::normalize_angle(cmd->viewangles.y - cheat::main::local()->get_abs_eye_angles().y) << std::endl;
	//	}
	//}

	//if (Math::AngleDiff(visual_real_angle.y, visual_fake_angle.y) > max_rotation && (int)cheat::Cvars.RageBot_exploits.GetValue() != 3)
	//	visual_real_angle.y -= (Math::AngleDiff(visual_real_angle.y, visual_fake_angle.y) - max_rotation);

	//if (send_packet)
	//	send_packets++;
	//else
	//	send_packets = 0;

	if (cheat::Cvars.Misc_AntiUT.GetValue())
		cmd->viewangles.Clamp();

	m_will_lby_update = false;

	//prev_desync_state = desync_state;
}

int max_lag_time_after_started_accelerating = 0;

void c_antiaimbot::do_exloits(CUserCmd* cmd, bool& send_packet)
{
	//if (!cheat::Cvars.RageBot_exploits.GetValue() || !cheat::main::local() || cheat::main::local()->IsDead() || (cheat::main::fakeducking && cheat::main::local()->m_fFlags() & FL_ONGROUND)) {
	//	cheat::features::antiaimbot.shift_ticks = 0;
	//	cheat::features::antiaimbot.do_tickbase = false;
	//	return;
	//}

	//if (cheat::Cvars.RageBot_exploits.GetValue() == 3/* && cheat::features::antiaimbot.do_tickbase*/) {
	//	send_packet = true;
	//	return;
	//}

	//auto wpn = cheat::main::local()->get_weapon();

	//if (cheat::Cvars.RageBot_exploits.GetValue() == 1)
	//{
	//	if (cmd->buttons & 1 && wpn && wpn->IsFireTime())// && weapon->ready_to_shift())
	//		cheat::features::antiaimbot.shift_ticks = 9;
	//}
	//else if (cheat::Cvars.RageBot_exploits.GetValue() == 2)
	//	cheat::features::antiaimbot.shift_ticks = 12;

	//auto v13 = Source::m_pClientState->m_iChockedCommands - 1;

	//if (v13 < 5) {
	//	if (cheat::Cvars.RageBot_exploits.GetValue() == 2)
	//		cheat::features::antiaimbot.shift_ticks = v13;
	//}
	//else
	//	if (Source::m_pClientState->m_iChockedCommands >= 5)
	//	send_packet = true;
}

float accel_time;
bool was_moving = false;

void c_antiaimbot::simulate_lags(CUserCmd * cmd, bool * send_packet)
{
	cheat::features::antiaimbot.unchoke = false;

	static auto onpeek_timer = 0.f;
	static auto onpeek = false;

	auto exploit = false;// cheat::Cvars.RageBot_exploits.GetValue() > 0.f;

	//auto is_fakeducking = (cheat::main::fakeducking && cheat::main::local()->m_fFlags() & FL_ONGROUND);

	if (cheat::main::fakeducking)
		return;

	if (!cheat::Cvars.Misc_FakeLag.GetValue()) {
		if (cheat::Cvars.anti_aim_desync.GetValue() > 0.f && !exploit) {
			cheat::features::antiaimbot.unchoke = *send_packet = (cmd->command_number % 2 == 1);
		}

		return;
	}

	/*if (exploit) {
		cheat::features::antiaimbot.unchoke = *send_packet = (Source::m_pClientState->m_iChockedCommands >= max((int)cheat::Cvars.Misc_fakelag_value.GetValue(), 6));

		if (cheat::Cvars.RageBot_exploits.GetValue() == 1 && Source::m_pClientState->m_iChockedCommands > 5 && !cheat::main::fakeducking)
			cheat::features::antiaimbot.unchoke = *send_packet = true;

		return;
	}*/

	auto net = Source::m_pEngine->GetNetChannelInfo();

	if (!net)
		return;

	auto fakelag_value = Math::clamp((int)cheat::Cvars.Misc_fakelag_value.GetValue(), 0, 17);

	int choke_value = fakelag_value;
	auto a3 = int((cheat::Cvars.Misc_fakelag_variance.GetValue() / 100.f) * float(fakelag_value));
	/*cheat::game::pressed_keys[88]*/
	//if (v3->Fakelag.b_should_unchoke)
	//	v4 = 0;

	auto pressing_move_keys = (cmd->buttons & IN_MOVELEFT
		|| cmd->buttons & IN_MOVERIGHT
		|| cmd->buttons & IN_BACK
		|| cmd->buttons & IN_FORWARD);

	//auto pressed_move_key = pressing_move_keys || /*!cheat::Cvars.RageBot_ScaledmgOnHp.GetValue() && */cheat::main::local()->m_vecVelocity().Length2D() > 0.1f && (cheat::main::local()->m_fFlags() & FL_ONGROUND);

	if (pressing_move_keys && !was_moving)
		accel_time = Source::m_pGlobalVars->realtime + 1.f;

	static bool m_bFakeDuck = false;
	auto new_buttons = 0;
	int v10 = cmd->buttons;

	if (a3 > 0 && !cheat::main::fakeducking)
		choke_value = (rand() % a3 * (2 * (Source::m_pGlobalVars->tickcount & 1) - 1));
	else if (cheat::main::fakeducking)
		choke_value = 16;

	const auto choke_randomize = RandInt(0, 2);
	const auto should_decrease = (choke_value >= 13 && RandInt(0, 1) == 1);

	choke_value = Math::clamp(choke_value + (should_decrease ? -choke_randomize : choke_randomize), 1, 15);

	// are we in on peek?
	if (onpeek_timer > Source::m_pGlobalVars->curtime + TICKS_TO_TIME(16))
	{
		onpeek_timer = 0.f;
		onpeek = false;
	}

	//auto should_choke = false; 
	if (!cheat::features::antiaimbot.skip_lags_this_tick) 
	{
		auto is_moving = cheat::main::local()->m_vecVelocity().Length2D() > 0.01f && pressing_move_keys && cheat::Cvars.Misc_fakelag_triggers.Has(0);
		auto on_ground = !(cheat::main::local()->m_fFlags() & FL_ONGROUND) && cheat::Cvars.Misc_fakelag_triggers.Has(1);
		auto origin_delta = (last_sent_origin - cheat::main::local()->get_abs_origin()).LengthSquared();
		auto did_stop = /*Engine::Prediction::Instance()->GetVelocity().Length2D() > cheat::main::local()->m_vecVelocity().Length2D()*/!pressing_move_keys && cheat::main::local()->m_vecVelocity().Length2D() > 2.f && cheat::Cvars.Misc_fakelag_triggers.Has(2);

		auto should_fakelag = false;

		if (is_moving || on_ground || did_stop)
			should_fakelag = true;

		if (cheat::main::fakeducking)
			should_fakelag = true;

		if (accel_time > Source::m_pGlobalVars->realtime && cheat::Cvars.Misc_fakelag_triggers.Has(3) && !cheat::main::fakewalking)
			should_fakelag = true;

		if (cheat::main::fakewalking)
			should_fakelag = true;

		// extend fake lag to maximum.
		if (onpeek_timer >= Source::m_pGlobalVars->curtime) {
			choke_value = max(12, choke_value);
			should_fakelag = true;
		}

		if (should_fakelag) 
		{
			switch ((int)cheat::Cvars.Misc_fakelag_baseFactor.GetValue())
			{
			case 0:
				cheat::features::antiaimbot.unchoke = *send_packet = (Source::m_pClientState->m_iChockedCommands >= choke_value);
				break;
			case 1:
				cheat::features::antiaimbot.unchoke = *send_packet = (origin_delta > 4096.0f || Source::m_pClientState->m_iChockedCommands >= choke_value);
				break;
			case 2:
				if (Source::m_pClientState->m_iChockedCommands > (choke_value - 2))
					cheat::features::antiaimbot.unchoke = *send_packet = (Source::m_pClientState->m_iChockedCommands >= choke_value);
				else
					cheat::features::antiaimbot.unchoke = *send_packet = (origin_delta > 4096.0f || Source::m_pClientState->m_iChockedCommands >= choke_value);
				break;
			case 3:
				if (Source::m_pClientState->m_iChockedCommands > (choke_value / 2))
					cheat::features::antiaimbot.unchoke = *send_packet = (Source::m_pClientState->m_iChockedCommands >= (choke_value / 2 - 2));
				else
					cheat::features::antiaimbot.unchoke = *send_packet = (Source::m_pClientState->m_iChockedCommands >= choke_value);
				break;
			default:
				break;
			}

			if (pressing_move_keys && cheat::Cvars.Misc_fakelag_triggers.Has(3) && accel_time <= Source::m_pGlobalVars->realtime)
			{
				trace_t tr;
				Ray_t ray;
				CTraceFilter traceFilter;
				traceFilter.pSkip = cheat::main::local();
				auto origin = cheat::main::local()->GetEyePosition();

				origin += cheat::main::local()->m_vecVelocity() * Source::m_pGlobalVars->interval_per_tick;

				Vector forward = Vector::Zero;
				auto start_angle = Engine::Movement::Instance()->m_qRealAngles;

				start_angle.x = 0;

				auto sidemove = fabs(cmd->sidemove) > 20.f;

				if (sidemove) 
				{
					if (cmd->sidemove > 0)
						start_angle.y -= 30.f;
					else
						start_angle.y += 30.f;
				}
				else
				{
					if (cmd->forwardmove > 0)
						start_angle.y -= 120.f;
					else
						start_angle.y += 120.f;
				}

				Math::AngleVectors(start_angle, &forward);
				forward.z = 0;
				auto lol = origin + forward * 200.f;

				ray.Init(origin, lol);
				Source::m_pEngineTrace->TraceRay(ray, 0x201400B, &traceFilter, &tr);

				static bool first_time = false;

				if (!tr.DidHit()) {

					/*if (first_time) {
						*send_packet = true;
						first_time = false;
					}

					cheat::features::antiaimbot.unchoke = *send_packet = (Source::m_pClientState->m_iChockedCommands >= max(14, choke_value));*/
					if (!onpeek && onpeek_timer < Source::m_pGlobalVars->curtime)
					{
						onpeek_timer = Source::m_pGlobalVars->curtime + TICKS_TO_TIME(16);

						if (Source::m_pClientState->m_iChockedCommands > 1)
							cheat::features::antiaimbot.unchoke = *send_packet = (Source::m_pClientState->m_iChockedCommands >= 1);
					}

					onpeek = true;
				}
				else
					onpeek = false;
			}
		}
		else
			cheat::features::antiaimbot.unchoke = *send_packet = Source::m_pClientState->m_iChockedCommands > 0;

		//if (!(Engine::Prediction::Instance()->GetFlags() & FL_ONGROUND) && cheat::main::local()->m_fFlags() & FL_ONGROUND /*&& (fabs(Math::NormalizeFloat(cheat::features::antiaimbot.last_real_angle.y - cheat::main::local()->m_flLowerBodyYawTarget())) / 180.f) > 0.0*/)
		//	*send_packet = false;

		if (Source::m_pClientState->m_iChockedCommands >= ((int)cheat::Cvars.Misc_fakelag_value.GetValue() - choke_randomize) && !cheat::main::fakeducking)
			cheat::features::antiaimbot.unchoke = *send_packet = true;

		if ((Source::m_pEngine->IsVoiceRecording() || cheat::features::music.m_playing) && Source::m_pClientState->m_iChockedCommands >= 3)
			cheat::features::antiaimbot.unchoke = *send_packet = true;

		if (Source::m_pClientState->m_iChockedCommands > 0 && cheat::main::local()->m_vecVelocity().Length2D() <= 1.f && !pressing_move_keys && !cheat::main::fakeducking)
			cheat::features::antiaimbot.unchoke = *send_packet = true;

		if (!*send_packet)
			cheat::main::prev_fakelag_value = Source::m_pClientState->m_iChockedCommands;
		//if (cheat::main::local()->m_fFlags() & FL_ONGROUND && cheat::main::local()->m_vecVelocity().Length() > 0.1f) {
		//	//for (int i = 0; i < 64; i++)
		//	//{
		//	//	auto entity = Source::m_pEntList->GetClientEntity(i);

		//	//	if (entity == nullptr || entity->IsDormant() || entity->m_iHealth() <= 0 || entity->m_iTeamNum() == cheat::main::local()->m_iTeamNum() || entity->IsDead())
		//	//		continue;

		//	//	const auto damage_1 = cheat::features::autowall.CanHit(entity->m_vecOrigin() + Vector(0, 0, 64.f), end_position_1, entity, cheat::main::local()),
		//	//		damage_2 = cheat::features::autowall.CanHit(entity->m_vecOrigin() + Vector(0, 0, 64.f), end_position_2, entity, cheat::main::local());



		//	//	if (max(damage_1, damage_2) > 0)
		//	//	{
		//	//		//printf("peeklag!\n");
		//	//		should_choke = true;
		//	//		break;
		//	//	}
		//	//} 

		//	if ((cmd->buttons & IN_MOVELEFT
		//		|| cmd->buttons & IN_MOVERIGHT
		//		|| cmd->buttons & IN_BACK
		//		|| cmd->buttons & IN_FORWARD) &&
		//		!(Engine::Prediction::Instance()->GetPrevCmd().buttons & IN_MOVELEFT
		//			|| Engine::Prediction::Instance()->GetPrevCmd().buttons & IN_MOVERIGHT
		//			|| Engine::Prediction::Instance()->GetPrevCmd().buttons & IN_BACK
		//			|| Engine::Prediction::Instance()->GetPrevCmd().buttons & IN_FORWARD) &&
		//		max_lag_time_after_started_accelerating <= Source::m_pGlobalVars->curtime)
		//		max_lag_time_after_started_accelerating = Source::m_pGlobalVars->curtime + 1.f;

		//	should_choke = (max_lag_time_after_started_accelerating >= Source::m_pGlobalVars->curtime) && !cheat::game::pressed_keys[88];
		//}

		was_moving = pressing_move_keys;

		//if (!should_choke)
		//{
		//	if (cheat::game::pressed_keys[88] && cheat::main::local()->m_fFlags() & FL_ONGROUND)
		//		*send_packet = (packets_choked >= 14);
		//	else if (cheat::main::local()->m_fFlags() & FL_ONGROUND)
		//		*send_packet = packets_choked > (cheat::settings.misc_fakelag_value / 3);
		//	else
		//		*send_packet = int(ceil(64.f / (cheat::main::local()->m_vecVelocity().Length2D() * Source::m_pGlobalVars->interval_per_tick)) + 1) <= 0 && packets_choked >= 16;
		//}
		//else {
		//	*send_packet = (packets_choked >= cheat::settings.misc_fakelag_value || !should_choke);
		//	//printf("peeklag!\n");
		//}

		//bool is_doing_fakelag_activity = (cheat::game::pressed_keys[88] || (cheat::main::local()->m_vecVelocity().Length() > 0.1f && !cheat::main::fakewalking) || !(cheat::main::local()->m_fFlags() & FL_ONGROUND));

		//if (!is_doing_fakelag_activity && packets_choked > 0)
		//	*send_packet = true;

		//if ((Source::m_pEngine->IsVoiceRecording() || cheat::features::music.m_playing) && Source::m_pClientState->m_iChockedCommands >= 4)
		//	*send_packet = true;

		// Lag Jump
		//if (!(Engine::Prediction::Instance()->GetFlags() & FL_ONGROUND) && cheat::main::local()->m_fFlags() & FL_ONGROUND /*&& (fabs(Math::NormalizeFloat(cheat::features::antiaimbot.last_real_angle.y - cheat::main::local()->m_flLowerBodyYawTarget())) / 180.f) > 0.0*/)
		//	cheat::features::antiaimbot.unchoke = *send_packet = false;

		if (cheat::Cvars.Misc_fakelag_triggers.Has(4) && cheat::main::local() != nullptr && cheat::main::local()->GetCollideable() && Source::m_pClientState->m_iDeltaTick != -1)
		{
			auto origin = cheat::main::local()->m_vecOrigin();
			auto velocity = cheat::main::local()->m_vecVelocity();
			auto flags = cheat::main::local()->m_fFlags();

			cheat::features::lagcomp.extrapolate(cheat::main::local(), origin, velocity, flags,
				Engine::Prediction::Instance()->get_flags() & FL_ONGROUND);

			auto fakeland = false;

			if (flags & FL_ONGROUND
				&& !(cheat::main::local()->m_fFlags() & FL_ONGROUND))
			{
				fakeland = true;//fabsf(Math::normalize_angle(cheat::main::local()->m_flLowerBodyYawTarget()
					//- animation_system->local_animation.eye_angles.y)) > 35.f;
			}
			else if (cheat::main::local()->m_fFlags() & FL_ONGROUND && fakeland)
				fakeland = false;

			if (fakeland)
				cheat::features::antiaimbot.unchoke = *send_packet = false;
		}

		//if ((packets_choked >= cheat::settings.misc_fakelag_value && !*send_packet) || packets_choked >= (cheat::settings.misc_fakelag_value + 1) || (packets_choked > 0 && skip_lags_this_tick && !cheat::main::fakewalking))
		//	*send_packet = true;

		/*if (!*send_packet)
			++packets_choked;
		else
			packets_choked = 0;*/
	}
}

void c_antiaimbot::work(CUserCmd * cmd, bool * send_packet)
{
	no_aas = false;

	static bool wait = false;

	auto state = cheat::main::local()->get_animation_state();

	if (cmd == nullptr || cheat::main::local() == nullptr || !state)
		return;

	if (cheat::main::fside == 0) {
		previous_side = cheat::main::fside = 1;
	}

	if (cheat::game::get_key_press(cheat::Cvars.anti_aim_desync_side_switch.GetValue()) && cheat::Cvars.anti_aim_desync_side_switch.GetValue()) {
		cheat::main::fside *= -1;

		if (cheat::main::local()->m_vecVelocity().Length2D() < 9.f) {
			cheat::main::fside *= 2;
			cmd->sidemove = 0;
		}

		previous_side = cheat::main::fside;
		wait = true;
	}

	if (cheat::Cvars.RageBot_SilentAim.GetValue() == 3.f && previous_side != 0) {
		if (flip_side)
			cheat::main::fside *= -1;
		else
			cheat::main::fside = previous_side;
	}

	auto pressing_move_keys = (cmd->buttons & IN_MOVELEFT
		|| cmd->buttons & IN_MOVERIGHT
		/*|| cmd->buttons & IN_BACK
		|| cmd->buttons & IN_FORWARD*/);

	//auto current_velocity_angle = Math::normalize_angle(RAD2DEG(atan2(cheat::main::local()->m_vecVelocity().y, cheat::main::local()->m_vecVelocity().x)));

	static float old_move_time = 0.f;

	if (cheat::Cvars.RageBot_AntiAim_FakeAutoDirection.Has(0) && pressing_move_keys && (Source::m_pGlobalVars->realtime - old_move_time) > 0.7f && !wait)
	{
		if (cmd->buttons & IN_MOVERIGHT)
			cheat::main::fside = 1;
		else
			cheat::main::fside = -1;

		if (cheat::Cvars.anti_aim_desync.GetValue() == 2.f)
			cheat::main::fside *= -1;

		old_move_time = Source::m_pGlobalVars->realtime;
	}

	if (!pressing_move_keys && wait)
		wait = false;

	auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

	if (!cheat::main::local() || !local_weapon) return;

	if (cheat::game::get_key_press(cheat::Cvars.Misc_aaleft.GetValue()) && cheat::Cvars.Misc_aaleft.GetValue())
		cheat::main::side = (cheat::main::side == 0 ? -1 : 0);

	if (cheat::game::get_key_press(cheat::Cvars.Misc_aaright.GetValue()) && cheat::Cvars.Misc_aaright.GetValue())
		cheat::main::side = (cheat::main::side == 1 ? -1 : 1);

	if (cheat::game::get_key_press(cheat::Cvars.Misc_aaback.GetValue()) && cheat::Cvars.Misc_aaback.GetValue())
		cheat::main::side = (cheat::main::side == 2 ? -1 : 2);

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

	if (drop)
	{
		Source::m_pEngine->ClientCmd_Unrestricted("drop");
		visual_exploit_angle = last_real_angle = visual_fake_angle = last_sent_angle = cmd->viewangles;
		*send_packet = true;
		drop = false;
		no_aas = true;

		return;
	}

	if (!cheat::Cvars.RageBot_Enable.GetValue() || (local_weapon && IsGrenade(local_weapon->m_iItemDefinitionIndex()) && local_weapon->IsBeingThrowed(cmd))) {
		visual_exploit_angle = last_real_angle = visual_fake_angle = last_sent_angle = cmd->viewangles;
		max_lag_time_after_started_accelerating = 0;
		no_aas = true;
		return;
	}

	if (cheat::main::local()->m_MoveType() == 9 || cheat::main::local()->m_MoveType() == 8) {
		visual_exploit_angle = last_real_angle = visual_fake_angle = last_sent_angle = cmd->viewangles;
		max_lag_time_after_started_accelerating = 0;
		no_aas = true;
		return;
	}

	if (cmd->buttons & IN_USE && local_weapon->m_iItemDefinitionIndex() != weapon_c4)
	{
		//last_real_angle = cmd->viewangles;

		//auto delta = cheat::features::aaa.sub_59B13C30(state) * (cheat::Cvars.anti_aim_desync_range.GetValue() / 100.f);

		//static auto is_inverted = false;

		//if (fabs(cheat::main::fside) != 1 && Source::m_pClientState->m_iChockedCommands == 1)
		//{
		//	is_inverted = (cheat::main::fside > 0);
		//	*send_packet = 1;

		//	cheat::main::fside /= 2;
		//}
		//else
		//	is_inverted = (cheat::main::fside > 0);

		//if (*send_packet)
		//{
		//	//cmd->viewangles.y = last_real_angle.y + (final_rotation * cheat::main::fside);

		//	if (is_inverted)
		//		cmd->viewangles.y += delta;
		//	else
		//		cmd->viewangles.y -= delta;

		//	//visual_real_angle = cmd->viewangles;
		//}
		//else
		//	last_sent_angle = visual_fake_angle = cmd->viewangles;
		//

		//if (cheat::Cvars.Misc_AntiUT.GetValue())
		//	cmd->viewangles.Clamp();

		return;
	}

	//DoLBY(cmd, send_packet);

	change_angles(cmd, *send_packet);

	//const auto old_delta = cheat::settings.ragebot_anti_aim_lby_delta;
	//cheat::settings.ragebot_anti_aim_lby_delta = cheat::features::aaa.sub_59B13C30(cheat::main::local()->get_animation_state());

	//cheat::settings.ragebot_anti_aim_lby_delta = old_delta;
}

inline float anglemod(float a)
{
	a = (360.f / 65536) * ((int)(a*(65536.f / 360.0f)) & 65535);
	return a;
}

float c_antiaimbot::some_func(float target, float value, float speed)
{
	//target = (target * 182.04445f) * 0.0054931641f;
	//value = (value * 182.04445f) * 0.0054931641f;

	//float delta = target - value;

	//// Speed is assumed to be positive
	//if (speed < 0)
	//	speed = -speed;

	//if (delta < -180.0f)
	//	delta += 360.0f;
	//else if (delta > 180.0f)
	//	delta -= 360.0f;

	//if (delta > speed)
	//	value += speed;
	//else if (delta < -speed)
	//	value -= speed;
	//else
	//	value = target;

	//return value;
	target = anglemod(target);
	value = anglemod(value);

	float delta = target - value;

	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}