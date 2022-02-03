#include "aimbot.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "math.hpp"
#include "displacement.hpp"
#include "angle_resolver.hpp"
#include "lag_compensation.hpp"
#include "anti_aimbot.hpp"
#include <algorithm>
#include <numeric>
#include "rmenu.hpp"

bool c_resolver::has_fake(C_BasePlayer* entity)
{
	auto index = entity->entindex() - 1;
	auto player_lag_record = &cheat::features::lagcomp.records[index];

	if (player_lag_record->m_Tickrecords.size() < 2)
		return true;

	if (fabs(player_lag_record->m_Tickrecords.front().simulation_time - player_lag_record->m_Tickrecords.at(1).simulation_time) == Source::m_pGlobalVars->interval_per_tick)
		return false;

	return true;
}

void c_resolver::CLBYRECORD::store(C_BasePlayer* entity)
{
	lby = entity->m_flLowerBodyYawTarget();
	speed = entity->m_vecVelocity().Length();
}

float c_resolver::get_average_moving_lby(const int& entindex, const float& accuracy)
{
	auto logs = move_logs[entindex];

	if (logs.empty())
		return 1337.f;

	auto ilogs = logs.size() - 1;

	if (ilogs > 12)
		logs.resize(12);

	float piska = 0.f;

	for (auto siski : logs)
		piska += siski.lby;

	return Math::normalize_angle(piska / ilogs);
}

float c_resolver::get_max_desync_delta(C_BasePlayer* ent) {

	auto animstate = ent->get_animation_state();
	float duckamount = animstate->duck_amt;// + 0xA4;

	float speedfraction = max(0, min(animstate->feet_speed, 1));
	float speedfactor = max(0, min(animstate->feet_shit, 1));

	float unk1 = ((*reinterpret_cast< float* > ((uintptr_t)animstate + 0x11C) * -0.30000001) - 0.19999999) * speedfraction;
	float unk2 = unk1 + 1.f;

	if (duckamount > 0.0)
		unk2 += ((duckamount * speedfactor) * (0.5f - unk2));

	return (*(float*)((uintptr_t)animstate + 0x334)) * unk2;
}

float c_resolver::sub_59B13C30(CCSGOPlayerAnimState *nn)
{
	if (nn->ent != cheat::main::local()) {
		float v1; // xmm0_4
		float v2; // xmm1_4
		float v3; // xmm0_4
		int v4; // eax
		float v5; // xmm4_4
		float v6; // xmm2_4
		float v7; // xmm0_4
		int v8; // eax
		float v10; // [esp+0h] [ebp-Ch]
		float v11; // [esp+4h] [ebp-8h]

		v1 = nn->feet_speed;
		v2 = 1.0;
		v10 = 0.0;
		v11 = v1;
		if (v1 <= 1.f)
		{
			v4 = v10;
			if (v1 >= 0.0)
				v4 = v11;
			v3 = v4;
		}
		else
			v3 = 1.f;

		v5 = nn->duck_amt;
		v6 = ((nn->stop_to_full_run_frac * -0.30000001f) - 0.19999999f) * v3 + 1.0f;
		if (v5 > 0.0)
		{
			v7 = nn->feet_shit;
			v11 = 0.0;
			v10 = v7;
			if (v7 <= 1.0)
			{
				v8 = v11;
				if (v7 >= 0.0)
					v8 = v10;
				v2 = v8;
			}
			v6 = v6 + (float)((float)(v2 * v5) * (float)(0.5f - v6));
		}

		return (nn->max_yaw * v6);
	}
	else
		return (cheat::features::antiaimbot.max_delta - .1f);//((*(float*)((uintptr_t)nn + 0x334)) * v6);
}

bool c_resolver::compare_delta(float v1, float v2, float Tolerance)
{
	v1 = Math::normalize_angle(v1);
	v2 = Math::normalize_angle(v2);

	if (fabs(Math::AngleDiff(v1, v2)) <= Tolerance)
		return true;

	return false;
}

std::unordered_map <int, float> old_static_yaw = {};
std::unordered_map <int, int> prev_rotation = {};

void c_resolver::on_real_angle_arrive(C_BasePlayer* m_player, resolver_records* resolve_data, float real_yaw)
{
	if (fabs(resolve_data->last_shot_time - Source::m_pGlobalVars->realtime) > 0.1f)
		resolve_data->latest_delta_used = Math::AngleDiff(resolve_data->latest_fake.y, real_yaw);

	//resolve_data->is_velocity_angle_fine	= (resolve_data->last_speed > 0.1f && fabs(Math::AngleDiff(real_yaw, resolve_data->last_velocity_angle)) < 45.f);
	resolve_data->is_using_static = fabs(fabs(resolve_data->latest_delta_used) - resolve_data->current_tick_max_delta) > 10.f;// && fabs(Math::AngleDiff(real_yaw, resolve_data->last_lby)) < 70.f;
	resolve_data->had_fake =
		resolve_data->fakeangles = (fabs(Math::AngleDiff(resolve_data->latest_fake.y, real_yaw)) > 5.f || fabs(Math::AngleDiff(resolve_data->latest_angles.y, real_yaw)) > 5.f);
}

void c_resolver::on_new_data_arrive(C_BasePlayer* m_player, resolver_records* resolve_data, CCSGOPlayerAnimState* state)
{
	auto history = cheat::main::shots_fired[m_player->entindex() - 1];

	if (fabs(resolve_data->server_anim_layers[6].m_flCycle - resolve_data->client_anim_layers[6].m_flCycle) > 0.000f && state->speed_2d < 1.f)
		resolve_data->last_cycle_desync = Source::m_pGlobalVars->realtime;

	auto pitch = m_player->m_angEyeAngles().x;

	if (pitch == 88.9947510f) // fix netvar compression
		m_player->m_angEyeAngles().x = 89.f;

	resolve_data->current_tick_max_delta = fabs(cheat::features::aaa.sub_59B13C30(state));
	m_player->m_angEyeAngles().x = Math::clamp(pitch, -89.f, 89.f);

	auto current_velocity_angle = RAD2DEG(atan2(m_player->m_vecVelocity().y, m_player->m_vecVelocity().x));
	auto balance_adjust = (resolve_data->server_anim_layers[3].m_flWeight > 0.01f && m_player->get_sec_activity(resolve_data->server_anim_layers[3].m_nSequence) == 979 && resolve_data->server_anim_layers[3].m_flCycle < 1.f);
	auto activity = m_player->get_sec_activity(resolve_data->server_anim_layers[1].m_nSequence);

	if (balance_adjust)
		resolve_data->last_time_balanced = Source::m_pGlobalVars->realtime;

	resolve_data->did_shot_this_tick = (m_player->get_weapon() != nullptr && m_player->get_weapon()->m_flLastShotTime() != resolve_data->last_shot_time && (m_player->m_angEyeAngles().x != resolve_data->previous_angles.x || fabs(m_player->m_angEyeAngles().x) < 65));//((activity >= ACT_CSGO_FIRE_PRIMARY && activity <= ACT_CSGO_FIRE_SECONDARY_OPT_2) && resolve_data->server_anim_layers[1].m_flWeight >= 0.01f && m_player->get_animation_layer(1).m_flCycle < 0.05f) || (m_player->get_weapon() && m_player->get_weapon()->m_Activity() == 208);
	resolve_data->tick_delta = Math::normalize_angle(m_player->m_angEyeAngles().y - resolve_data->latest_angles.y);
	resolve_data->abs_yaw_delta = Math::normalize_angle(m_player->m_angEyeAngles().y - m_player->get_abs_eye_angles().y);
	resolve_data->lby_delta = Math::normalize_angle(m_player->m_flLowerBodyYawTarget() - m_player->m_angEyeAngles().y);
	resolve_data->is_jittering = (/*fabs(resolve_data->tick_delta) > 90 && */fabs(resolve_data->last_abs_yaw_delta_change - Source::m_pGlobalVars->realtime) < 1.f && (fabs(Source::m_pGlobalVars->realtime - resolve_data->last_time_balanced) < 0.6f) && fabs(resolve_data->last_abs_yaw_delta_60 - Source::m_pGlobalVars->realtime) < 2.f);
	resolve_data->last_velocity_angle = current_velocity_angle;
	resolve_data->last_speed = state->speed_2d;
	resolve_data->last_lby = m_player->m_flLowerBodyYawTarget();

	if (state->speed_2d > 0.1f)
		resolve_data->last_time_moved = Source::m_pGlobalVars->realtime;
	if (TIME_TO_TICKS(m_player->m_flSimulationTime() - m_player->m_flOldSimulationTime()) > 1)
		resolve_data->last_time_choked = Source::m_pGlobalVars->realtime;
	if (fabs(resolve_data->m_flClientRate - resolve_data->m_flRate) == 0.003f)
		resolve_data->last_time_three = Source::m_pGlobalVars->realtime;

	if (resolve_data->abs_yaw_delta != resolve_data->last_abs_yaw_delta)
	{
		if (resolve_data->last_abs_yaw_delta != 0.0f)
			resolve_data->last_abs_yaw_delta_change = Source::m_pGlobalVars->realtime;

		if (resolve_data->abs_yaw_delta > resolve_data->current_tick_max_delta)
			resolve_data->last_abs_yaw_delta_60 = Source::m_pGlobalVars->realtime;

		resolve_data->last_abs_yaw_delta = resolve_data->abs_yaw_delta;
	}

	resolve_data->is_using_static = ((fabs(resolve_data->last_cycle_desync - Source::m_pGlobalVars->realtime) < 1.f || fabs(resolve_data->last_time_three - Source::m_pGlobalVars->realtime) > 1.f && state->speed_2d > 1.f) && resolve_data->current_tick_max_delta > 40 && !(fabs(Source::m_pGlobalVars->realtime - resolve_data->last_time_balanced) < 0.6f)); /*(fabs(lag_data->last_time_moved - lag_data->last_move_cycle_update) > 0.1f && fabs(Source::m_pGlobalVars->realtime - lag_data->last_feet_cycle_update) < 0.25f) || */ //(fabs(resolve_data->m_flClientRate - resolve_data->m_flRate) > 0.00020f && resolve_data->m_flClientRate == 0.f));
	resolve_data->is_using_balance = (fabs(Source::m_pGlobalVars->realtime - resolve_data->last_time_balanced) < 0.6f && fabs(resolve_data->lby_delta) > 0.0001f);
	resolve_data->did_lby_update_fail = (fabs(resolve_data->abs_yaw_delta) > 0.0001f && fabs(resolve_data->lby_delta) > 0.0001f);
	//	resolve_data->is_using_static	 = (fabs(fabs(resolve_data->latest_delta_used) - resolve_data->current_tick_max_delta) > 10.f);

	resolve_data->fakeangles = (resolve_data->is_using_balance
		|| resolve_data->is_jittering
		|| fabs(resolve_data->abs_yaw_delta) > 0.0001f && fabs(resolve_data->tick_delta) < 1.f
		|| resolve_data->is_using_static
		|| (fabs(m_player->m_angEyeAngles().x) > 45.f && (history > 0 && history % 3 == 0 || resolve_data->had_fake || resolve_data->force_resolving)));
	/*|| (fabs(m_player->get_pose(7) - 0.5f) * 116.f) > 10.f*/;

	if (resolve_data->is_shifting)
		resolve_data->fakeangles = resolve_data->is_using_balance = (fabs(Source::m_pGlobalVars->realtime - resolve_data->last_time_balanced) < 0.1f && history % 2 == 1);//(fabs(Source::m_pGlobalVars->realtime - resolve_data->last_time_balanced) < 0.1f && fabs(resolve_data->lby_delta) > 0.0000f);

	if (resolve_data->fakeangles)
		resolve_data->latest_angles_when_faked = resolve_data->latest_angles;

	if (!resolve_data->fakeangles)
		resolve_data->fakeangles = (fabs(resolve_data->latest_angles.x) > 45.f && (history > 0 && history % 3 < 2));
}

bool c_resolver::resolve_freestand(C_BasePlayer* m_player, resolver_records* resolve, CCSGOPlayerAnimState* state, float &step)
{
	const auto freestanding_record = resolve->freestanding_record;

	//const float at_target_yaw = Math::CalcAngle(cheat::main::local()->m_vecOrigin(), m_player->m_vecOrigin()).y;

	if ((freestanding_record.left_damage >= 1.0f && freestanding_record.right_damage >= 1.0f) || resolve->did_shot_this_tick/* || (int(freestanding_record.left_damage * 1000.f) == int(freestanding_record.right_damage * 1000.f))*/)
		return false;

	//if (cheat::features::aaa.compare_delta(fabs(at_target_yaw + 90.f), fabs(state->eye_yaw), 40.f))
	//{
	//	step = -1.f;
	//}
	//else 
	//{
	//	//if (int(freestanding_record.left_damage * 1000.f) > int(freestanding_record.right_damage * 1000.f))
	//	//	step = -1.f;
	//	//else
	//	//	step = 1.f;

	//if (freestanding_record.right_damage < 1.0f)
	//	step = 1.f;
	//if (freestanding_record.left_damage < 1.0f)
	//	step = -1.f;
	if (freestanding_record.right_damage < freestanding_record.left_damage)
		step = 1.f;
	else
		step = -1.f;
	//}

	return true;
}

void c_resolver::store_deltas(C_BasePlayer* m_player, resolver_records* resolve_data)
{
	CCSGOPlayerAnimState animstate_backup;
	memcpy(&animstate_backup, m_player->get_animation_state(), 0x344);

	//const auto abs_origin = m_player->get_abs_origin();
	const auto poses = m_player->m_flPoseParameter();
	const auto backup_angles = m_player->m_angEyeAngles();
	const auto backup_velocity = m_player->m_vecVelocity();
	const auto backup_origin = m_player->m_vecOrigin();
	const auto backup_duckamt = m_player->m_flDuckAmount();
	const auto backup_simtime = m_player->m_flSimulationTime();
	const auto backup_flags = m_player->m_fFlags();

	auto m_records = cheat::features::lagcomp.records[m_player->entindex() - 1].m_Tickrecords;

	const auto animlayers = (/*resolve_data->preserver_animlayers_saved ? resolve_data->preserver_anim_layers :*/ resolve_data->server_anim_layers);

	resolve_data->previous_rotation = backup_angles.y;

	auto lag = m_records.empty() ? 1 : TICKS_TO_TIME(m_player->m_flSimulationTime() - m_records[0].simulation_time);

	if (lag <= 1)
	{
		{
			std::memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

			cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), -1);

			memcpy(resolve_data->resolver_anim_layers[1], m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
			std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			m_player->force_bone_rebuild();
			m_player->SetupBonesEx();
			memcpy(resolve_data->rightmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			resolve_data->right_side = m_player->get_animation_state()->abs_yaw;
			cheat::features::lagcomp.store_record_data(m_player, &resolve_data->rightrec);

			m_player->m_flPoseParameter() = poses;
			memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
			memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());

			m_player->m_vecVelocity() = backup_velocity;
			m_player->m_vecOrigin() = backup_origin;
			m_player->m_flDuckAmount() = backup_duckamt;
			m_player->m_flSimulationTime() = backup_simtime;
			m_player->m_angEyeAngles() = backup_angles;
			m_player->m_fFlags() = backup_flags;
			m_player->m_vecAbsVelocity() = backup_velocity;
		}

		{
			cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), 1);

			std::memcpy(resolve_data->resolver_anim_layers[2], m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
			std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			m_player->force_bone_rebuild();
			m_player->SetupBonesEx();
			memcpy(resolve_data->leftmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			cheat::features::lagcomp.store_record_data(m_player, &resolve_data->leftrec);
			resolve_data->left_side = m_player->get_animation_state()->abs_yaw;

			m_player->m_flPoseParameter() = poses;
			memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
			memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

			m_player->m_vecVelocity() = backup_velocity;
			m_player->m_vecOrigin() = backup_origin;
			m_player->m_flDuckAmount() = backup_duckamt;
			m_player->m_flSimulationTime() = backup_simtime;
			m_player->m_angEyeAngles() = backup_angles;
			m_player->m_fFlags() = backup_flags;
			m_player->m_vecAbsVelocity() = backup_velocity;
		}

		{
			cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), -1, resolve_data->current_tick_max_delta * 0.66f);

			std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			m_player->force_bone_rebuild();
			m_player->SetupBonesEx();
			memcpy(resolve_data->rightlmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			cheat::features::lagcomp.store_record_data(m_player, &resolve_data->rightlrec);
			resolve_data->right_lside = m_player->get_animation_state()->abs_yaw;

			m_player->m_flPoseParameter() = poses;
			memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
			memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

			m_player->m_vecVelocity() = backup_velocity;
			m_player->m_vecOrigin() = backup_origin;
			m_player->m_flDuckAmount() = backup_duckamt;
			m_player->m_flSimulationTime() = backup_simtime;
			m_player->m_angEyeAngles() = backup_angles;
			m_player->m_fFlags() = backup_flags;
			m_player->m_vecAbsVelocity() = backup_velocity;
		}

		{
			cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), 1, resolve_data->current_tick_max_delta * 0.66f);

			std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			m_player->force_bone_rebuild();
			m_player->SetupBonesEx();
			memcpy(resolve_data->leftlmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			cheat::features::lagcomp.store_record_data(m_player, &resolve_data->leftlrec);
			resolve_data->left_lside = m_player->get_animation_state()->abs_yaw;

			m_player->m_flPoseParameter() = poses;
			memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
			memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

			m_player->m_vecVelocity() = backup_velocity;
			m_player->m_vecOrigin() = backup_origin;
			m_player->m_flDuckAmount() = backup_duckamt;
			m_player->m_flSimulationTime() = backup_simtime;
			m_player->m_angEyeAngles() = backup_angles;
			m_player->m_fFlags() = backup_flags;
			m_player->m_vecAbsVelocity() = backup_velocity;
		}
	}
	else
	{
		cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), 1);

		auto v38 = Math::AngleDiff(m_player->m_angEyeAngles().y, m_player->get_animation_state()->abs_yaw);
		auto is_inverted = v38 < 0.0f;

		if (is_inverted)
		{
			std::memcpy(resolve_data->resolver_anim_layers[1], m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
			std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			m_player->force_bone_rebuild();
			m_player->SetupBonesEx();
			memcpy(resolve_data->rightmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			cheat::features::lagcomp.store_record_data(m_player, &resolve_data->rightrec);
			resolve_data->right_side = m_player->get_animation_state()->abs_yaw;
		}
		else
		{
			std::memcpy(resolve_data->resolver_anim_layers[2], m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
			std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			m_player->force_bone_rebuild();
			m_player->SetupBonesEx();
			memcpy(resolve_data->leftmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			cheat::features::lagcomp.store_record_data(m_player, &resolve_data->leftrec);
			resolve_data->left_side = m_player->get_animation_state()->abs_yaw;
		}

		m_player->m_flPoseParameter() = poses;
		memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
		memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

		m_player->m_vecVelocity() = backup_velocity;
		m_player->m_vecOrigin() = backup_origin;
		m_player->m_flDuckAmount() = backup_duckamt;
		m_player->m_flSimulationTime() = backup_simtime;
		m_player->m_angEyeAngles() = backup_angles;
		m_player->m_fFlags() = backup_flags;
		m_player->m_vecAbsVelocity() = backup_velocity;

		{
			std::memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

			cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), -1);

			if (is_inverted)
			{
				std::memcpy(resolve_data->resolver_anim_layers[2], m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
				std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				m_player->force_bone_rebuild();
				m_player->SetupBonesEx();
				memcpy(resolve_data->leftmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
				cheat::features::lagcomp.store_record_data(m_player, &resolve_data->leftrec);
				resolve_data->left_side = m_player->get_animation_state()->abs_yaw;
			}
			else
			{
				memcpy(resolve_data->resolver_anim_layers[1], m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
				std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				m_player->force_bone_rebuild();
				m_player->SetupBonesEx();
				memcpy(resolve_data->rightmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
				resolve_data->right_side = m_player->get_animation_state()->abs_yaw;
				cheat::features::lagcomp.store_record_data(m_player, &resolve_data->rightrec);
			}

			m_player->m_flPoseParameter() = poses;
			memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
			memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());

			m_player->m_vecVelocity() = backup_velocity;
			m_player->m_vecOrigin() = backup_origin;
			m_player->m_flDuckAmount() = backup_duckamt;
			m_player->m_flSimulationTime() = backup_simtime;
			m_player->m_angEyeAngles() = backup_angles;
			m_player->m_fFlags() = backup_flags;
			m_player->m_vecAbsVelocity() = backup_velocity;
		}

		{
			cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), -1, resolve_data->current_tick_max_delta * 0.66f);

			if (is_inverted)
			{
				std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				m_player->force_bone_rebuild();
				m_player->SetupBonesEx();
				memcpy(resolve_data->leftlmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
				cheat::features::lagcomp.store_record_data(m_player, &resolve_data->leftlrec);
				resolve_data->left_lside = m_player->get_animation_state()->abs_yaw;
			}
			else
			{
				std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				m_player->force_bone_rebuild();
				m_player->SetupBonesEx();
				memcpy(resolve_data->rightlmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
				cheat::features::lagcomp.store_record_data(m_player, &resolve_data->rightlrec);
				resolve_data->right_lside = m_player->get_animation_state()->abs_yaw;
			}

			m_player->m_flPoseParameter() = poses;
			memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
			memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

			m_player->m_vecVelocity() = backup_velocity;
			m_player->m_vecOrigin() = backup_origin;
			m_player->m_flDuckAmount() = backup_duckamt;
			m_player->m_flSimulationTime() = backup_simtime;
			m_player->m_angEyeAngles() = backup_angles;
			m_player->m_fFlags() = backup_flags;
			m_player->m_vecAbsVelocity() = backup_velocity;
		}

		{
			cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), 1, resolve_data->current_tick_max_delta * 0.66f);

			if (is_inverted)
			{
				std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				m_player->force_bone_rebuild();
				m_player->SetupBonesEx();
				memcpy(resolve_data->rightlmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
				cheat::features::lagcomp.store_record_data(m_player, &resolve_data->rightlrec);
				resolve_data->right_lside = m_player->get_animation_state()->abs_yaw;
			}
			else
			{
				std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				m_player->force_bone_rebuild();
				m_player->SetupBonesEx();
				memcpy(resolve_data->leftlmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
				cheat::features::lagcomp.store_record_data(m_player, &resolve_data->leftlrec);
				resolve_data->left_lside = m_player->get_animation_state()->abs_yaw;
			}

			m_player->m_flPoseParameter() = poses;
			memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
			memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

			m_player->m_vecVelocity() = backup_velocity;
			m_player->m_vecOrigin() = backup_origin;
			m_player->m_flDuckAmount() = backup_duckamt;
			m_player->m_flSimulationTime() = backup_simtime;
			m_player->m_angEyeAngles() = backup_angles;
			m_player->m_fFlags() = backup_flags;
			m_player->m_vecAbsVelocity() = backup_velocity;
		}
	}

	{
		cheat::features::lagcomp.update_animations(m_player, (m_records.empty() ? nullptr : &m_records[0]), 0);
		//*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80) = Math::normalize_angle(/*resolve_data->preserver_animlayers_saved ? resolve_data->no_side : */animstate_backup.abs_yaw);


		memcpy(resolve_data->resolver_anim_layers[0], m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
		std::memcpy(m_player->animation_layers_ptr(), resolve_data->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
		//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
		m_player->force_bone_rebuild();
		m_player->SetupBonesEx();
		cheat::features::lagcomp.store_record_data(m_player, &resolve_data->norec);
		memcpy(resolve_data->nodesmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
		resolve_data->no_side = m_player->get_animation_state()->abs_yaw;

		m_player->m_flPoseParameter() = poses;
		memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);
		memcpy(m_player->animation_layers_ptr(), animlayers, 0x38 * m_player->get_animation_layers_count());

		m_player->m_vecVelocity() = backup_velocity;
		m_player->m_vecOrigin() = backup_origin;
		m_player->m_flDuckAmount() = backup_duckamt;
		m_player->m_flSimulationTime() = backup_simtime;
		m_player->m_angEyeAngles() = backup_angles;
		m_player->m_fFlags() = backup_flags;
		m_player->m_vecAbsVelocity() = backup_velocity;
	}

	//m_player->set_abs_origin(abs_origin);
}

//C_AnimationLayer c_resolver::get_animation_layer(C_AnimationLayer* animlayers, int number)
//{
//	auto cur_layer = animlayers[0];
//	auto cnumber = 0;
//	while (cur_layer.m_nOrder != number)
//	{
//		++cnumber;
//
//		cur_layer = animlayers[cnumber];
//
//		if (cnumber >= 13)
//			return animlayers[number];
//	}
//
//	return animlayers[cnumber];
//}

void c_resolver::resolve(C_BasePlayer* m_player, int &history)
{
	auto idx = m_player->entindex() - 1;

	if (cheat::main::shots_fired[idx] < 0)
		cheat::main::shots_fired[idx] = 0;

	if (history < 0)
		history = 0;

	auto lag_data = &player_resolver_records[idx];
	auto history_data = &cheat::features::lagcomp.records[idx].m_Tickrecords;

	lag_data->resolved_yaw = m_player->get_animation_state()->abs_yaw;
	lag_data->animations_updated = false;

	//if (!history_data || history_data->empty())
	//	return;

	lag_data->current_tick_max_delta = fabs(cheat::features::aaa.sub_59B13C30(m_player->get_animation_state()));

	CCSGOPlayerAnimState animstate_backup;

	if (lag_data->did_shot_this_tick && m_player->get_weapon())
		lag_data->last_shot_time = m_player->get_weapon()->m_flLastShotTime();

	memcpy(&animstate_backup, m_player->get_animation_state(), 0x344);

	/*if (lag_data->last_simtime == m_player->m_flSimulationTime() && lag_data->previous_angles != m_player->m_angEyeAngles())
	{
	if (lag_data->previous_angles.x > 0)
	m_player->m_angEyeAngles() = lag_data->previous_angles;
	}*/

	/*auto lol = Math::NormalizePitch(m_player->m_angEyeAngles().x - lag_data->previous_angles.x);

	if (fabs(lol) > 79.f)
	{
	if (lag_data->previous_angles.x > m_player->m_angEyeAngles().x)
	m_player->m_angEyeAngles() = lag_data->previous_angles;
	}*/

	//m_player->m_angEyeAngles().x = std::clamp(m_player->m_angEyeAngles().x, -89.f, 89.f);
	//m_player->m_angEyeAngles().x = 89.f;

	//auto delta = Math::AngleDiff(m_player->m_angEyeAngles().y, state->abs_yaw);

	store_deltas(m_player, lag_data);

	memcpy(m_player->get_animation_state(), &animstate_backup, 0x344);

	//on_new_data_arrive(m_player, lag_data, state);

	//if (cheat::settings.ragebot_hitchance <= 0 && player_lag_record->m_Tickrecords.size() > 1)
	//{
	//	auto record1 = player_lag_record->m_Tickrecords.at(0), record2 = player_lag_record->m_Tickrecords.at(1);
	//
	//	eye_angles->x = std::clamp(Math::normalize_angle(eye_angles->x), -89.f, 89.f);
	//
	//	if (!(entity->m_fFlags() & FL_ONGROUND) && record2.entity_flags & FL_ONGROUND && eye_angles->x >= 178.36304f)
	//		eye_angles->x = -89.f;
	//	else
	//	{
	//		if (abs(eye_angles->x) > 89.f)
	//			eye_angles->x = cheat::main::shots_fired[idx] % 4 != 3 ? 89.f : -89.f;
	//		else
	//			eye_angles->x = cheat::main::shots_fired[idx] % 4 != 3 ? 89.f : eye_angles->x;
	//	}
	//}

	auto current_velocity_angle = RAD2DEG(atan2(m_player->m_vecVelocity().y, m_player->m_vecVelocity().x));

	//if (state->m_flSpeed2D <= 0.1f || ((history > 0 && ((history % 5) > 3 || (history % 5) == 0))))
	//	lag_data->max_delta = ((((Source::m_pGlobalVars->realtime - lag_data->last_time_balanced) < 1.5f) || !isUsingStatic) && isDesyncing);

	float lby = m_player->m_flLowerBodyYawTarget();

	//float step = 60.f;//(history % 3 == 1 ? (lag_data->current_tick_max_delta * 0.67f) : lag_data->current_tick_max_delta);

	//lag_data->resolving_method = 0;
	auto at_targ = Math::CalcAngle(cheat::main::local()->m_vecOrigin(), m_player->m_vecOrigin()).y;
	auto delta_from_bw = Math::AngleDiff(at_targ + 180.f, m_player->get_animation_state()->abs_yaw);
	auto vdelta_from_bw = Math::AngleDiff(at_targ + 180.f, current_velocity_angle);
	auto is_backwards = fabs(delta_from_bw) < 70.f;
	float start_angle = m_player->m_angEyeAngles().y;

	//auto eye_tick_delta = fabs(Math::normalize_angle(m_player->m_angEyeAngles().y - history_data->front().eye_angles.y));

	//float ResolvedYaw = state->abs_yaw;

	lag_data->abs_yaw_delta = Math::normalize_angle(m_player->m_angEyeAngles().y - m_player->get_abs_eye_angles().y);
	lag_data->lby_delta = Math::normalize_angle(m_player->m_flLowerBodyYawTarget() - m_player->m_angEyeAngles().y);

	if (lag_data->abs_yaw_delta != lag_data->last_abs_yaw_delta)
	{
		if (lag_data->last_abs_yaw_delta != 0.0f)
			lag_data->last_abs_yaw_delta_change = Source::m_pGlobalVars->realtime;

		if (lag_data->abs_yaw_delta > lag_data->current_tick_max_delta)
			lag_data->last_abs_yaw_delta_60 = Source::m_pGlobalVars->realtime;

		lag_data->last_abs_yaw_delta = lag_data->abs_yaw_delta;
	}

	lag_data->is_jittering = (/*fabs(resolve_data->tick_delta) > 90 && */fabs(lag_data->last_abs_yaw_delta_change - Source::m_pGlobalVars->realtime) < 1.f && (fabs(lag_data->lby_delta) > 1.f) && fabs(lag_data->last_abs_yaw_delta_60 - Source::m_pGlobalVars->realtime) < 2.f);

	if (/*lag_data->fakeangles && */cheat::Cvars.RageBot_Resolver.GetValue()/*&& lag_data->current_tick_max_delta > 10.f*/ && !m_player->IsBot())
	{
		auto speed = m_player->m_vecVelocity().Length2D();

		if (m_player->m_fFlags() & FL_ONGROUND && !history_data->empty() && history_data->front().entity_flags & FL_ONGROUND)
		{
			if (speed < 0.1f)
			{
				auto delta = Math::AngleDiff(m_player->m_angEyeAngles().y, lag_data->no_side);

				if (lag_data->server_anim_layers[3].m_flWeight == 0.0f && lag_data->server_anim_layers[3].m_flCycle == 0.0f) {
					lag_data->resolving_way = Math::clamp((2 * (delta <= 0.f) - 1), -1, 1);// copysign(1, delta);
					lag_data->animations_updated = true;
				}
			}
			else if (!int(lag_data->server_anim_layers[12].m_flWeight * 1000.f))//(lag_data->server_anim_layers[6].m_flWeight * 1000.f) == (history_data->at(0).anim_layers[6].m_flWeight * 1000.f))
			{
				//2 = -1; 3 = 1; 1 = fake;
				if (int(lag_data->server_anim_layers[6].m_flWeight * 1000.f) == int(lag_data->preserver_anim_layers[6].m_flWeight * 1000.f))
				{
					float delta1 = abs(lag_data->server_anim_layers[6].m_flPlaybackRate - lag_data->resolver_anim_layers[0][6].m_flPlaybackRate);
					float delta2 = abs(lag_data->server_anim_layers[6].m_flPlaybackRate - lag_data->resolver_anim_layers[1][6].m_flPlaybackRate);
					float delta3 = abs(lag_data->server_anim_layers[6].m_flPlaybackRate - lag_data->resolver_anim_layers[2][6].m_flPlaybackRate);

					if (delta1 < delta3 || delta2 <= delta3 || (int)(float)(delta3 * 1000.0f)) {
						if (delta1 >= delta2 && delta3 > delta2 && !(int)(float)(delta2 * 1000.0f))
						{
							//lag_data->resolving_method = 1;
							lag_data->resolving_way = 1;
							lag_data->animations_updated = true;
							lag_data->resolved = true;
							lag_data->last_anims_update_time = Source::m_pGlobalVars->realtime;
						}
					}
					else
					{
						//lag_data->resolving_method = -1;
						lag_data->resolving_way = -1;
						lag_data->animations_updated = true;
						lag_data->resolved = true;
						lag_data->last_anims_update_time = Source::m_pGlobalVars->realtime;
					}
				}
			}
		}
	}

	auto ResolvedYaw = Math::normalize_angle(m_player->m_angEyeAngles().y + Math::normalize_angle(60.f * lag_data->resolving_method));//(lag_data->resolving_method == 0 ? lag_data->no_side : (lag_data->resolving_method > 0 ? lag_data->left_side : lag_data->right_side));//

	lag_data->latest_angles = m_player->m_angEyeAngles();
	//ResolvedYaw = Math::normalize_angle(ResolvedYaw);

	lag_data->latest_fake.y = m_player->get_animation_state()->abs_yaw;
	lag_data->previous_rotation = ResolvedYaw;
	lag_data->previous_angles.y = ResolvedYaw;
	lag_data->previous_angles.x = m_player->m_angEyeAngles().x;

	lag_data->was_shifting = lag_data->is_shifting;
	lag_data->last_simtime = m_player->m_flSimulationTime();
}