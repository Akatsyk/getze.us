#include "aimbot.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "math.hpp"
#include "displacement.hpp"
#include "lag_compensation.hpp"
#include "autowall.hpp"
#include "angle_resolver.hpp"
#include "game_movement.h"
#include "anti_aimbot.hpp"
#include <unordered_map>
#include <algorithm>
#include "rmenu.hpp"
#include "visuals.hpp"

#include "thread/threading.h"
#include "thread/shared_mutex.h"

#include <thread>

#define LAG_COMPENSATION_TICKS 18
#define EFL_DIRTY_ABSVELOCITY (1<<12)

void c_lagcomp::get_interpolation()
{
	static auto cl_interp					= Source::m_pCvar->FindVar(_("cl_interp"));
	static auto cl_interp_ratio				= Source::m_pCvar->FindVar(_("cl_interp_ratio"));
	static auto sv_client_min_interp_ratio	= Source::m_pCvar->FindVar(_("sv_client_min_interp_ratio"));
	static auto sv_client_max_interp_ratio	= Source::m_pCvar->FindVar(_("sv_client_max_interp_ratio"));
	static auto cl_updaterate				= Source::m_pCvar->FindVar(_("cl_updaterate"));
	static auto sv_minupdaterate			= Source::m_pCvar->FindVar(_("sv_minupdaterate"));
	static auto sv_maxupdaterate			= Source::m_pCvar->FindVar(_("sv_maxupdaterate"));

	auto updaterate = std::clamp(cl_updaterate->GetFloat(), sv_minupdaterate->GetFloat(), sv_maxupdaterate->GetFloat());
	auto lerp_ratio = std::clamp(cl_interp_ratio->GetFloat(), sv_client_min_interp_ratio->GetFloat(), sv_client_max_interp_ratio->GetFloat());

	cheat::main::lerp_time = std::clamp(lerp_ratio / updaterate, cl_interp->GetFloat(), 1.0f);
}

void c_lagcomp::store_record_data(C_BasePlayer* entity, C_Tickrecord* record_data)
{
	if (entity == nullptr || entity->get_animation_state() == nullptr)
		return;

	auto activity = entity->get_sec_activity(entity->get_animation_layer(1).m_nSequence);
	//auto shot_bt = ((activity >= ACT_CSGO_FIRE_PRIMARY && activity <= ACT_CSGO_FIRE_SECONDARY_OPT_2) && entity->get_animation_layer(1).m_flWeight > 0.01f && entity->get_animation_layer(1).m_flCycle < 0.05f) || (entity->get_weapon() && entity->get_weapon()->m_Activity() == 208);

	record_data->type = RECORD_NORMAL;

	auto priority = ((activity == ACT_CSGO_RELOAD) && entity->get_animation_layer(1).m_flWeight > 0.001f && entity->get_animation_layer(1).m_flCycle < 1.f);

	if (cheat::features::aaa.player_resolver_records[entity->entindex() - 1].did_shot_this_tick) 
	{
		record_data->shot_this_tick = true;
		record_data->type = RECORD_SHOT;
		record_data->shot_time = cheat::features::aaa.player_resolver_records[entity->entindex() - 1].last_shot_time;
	}//ACT_CSGO_FLASHBANG_REACTION
	else if (cheat::features::aaa.player_resolver_records[entity->entindex() - 1].animations_updated)
		record_data->type = RECORD_PRIORITY;
	else if (priority)
		record_data->type = RECORD_PRIORITY;

	record_data->bones_count = entity->GetBoneCount();
	record_data->bones_count = Math::clamp(record_data->bones_count, 0, 128);
	memcpy(record_data->matrixes, cheat::features::aaa.player_resolver_records[entity->entindex() - 1].nodesmx, record_data->bones_count * sizeof(matrix3x4_t));

	memcpy(record_data->leftmatrixes, cheat::features::aaa.player_resolver_records[entity->entindex() - 1].leftmx, record_data->bones_count * sizeof(matrix3x4_t));
	memcpy(record_data->rightmatrixes, cheat::features::aaa.player_resolver_records[entity->entindex() - 1].rightmx, record_data->bones_count * sizeof(matrix3x4_t));

	memcpy(record_data->leftlmatrixes, cheat::features::aaa.player_resolver_records[entity->entindex() - 1].leftlmx, record_data->bones_count * sizeof(matrix3x4_t));
	memcpy(record_data->rightlmatrixes, cheat::features::aaa.player_resolver_records[entity->entindex() - 1].rightlmx, record_data->bones_count * sizeof(matrix3x4_t));

	record_data->left_side = cheat::features::aaa.player_resolver_records[entity->entindex() - 1].left_side;
	record_data->right_side = cheat::features::aaa.player_resolver_records[entity->entindex() - 1].right_side;

	record_data->rtype = cheat::features::aaa.player_resolver_records[entity->entindex() - 1].resolving_method;

	record_data->origin = entity->m_vecOrigin();
	record_data->abs_origin = entity->get_abs_origin();
	record_data->velocity = entity->m_vecVelocity();
	record_data->object_mins = entity->OBBMins();
	record_data->object_maxs = entity->OBBMaxs();
	record_data->eye_angles = entity->m_angEyeAngles();
	record_data->abs_eye_angles = QAngle(0, entity->get_animation_state()->abs_yaw, 0);
	record_data->sequence = entity->m_nSequence();
	record_data->entity_flags = entity->m_fFlags();
	record_data->simulation_time = entity->m_flSimulationTime();
	record_data->lower_body_yaw = entity->m_flLowerBodyYawTarget();
	record_data->cycle = entity->m_flCycle();
	record_data->dormant = entity->IsDormant();
	record_data->anim_velocity = entity->m_vecVelocity();
	record_data->ientity_flags = entity->m_iEFlags();
	record_data->duck_amt = entity->m_flDuckAmount();
	record_data->desync_delta = cheat::features::aaa.sub_59B13C30(entity->get_animation_state());
	record_data->skin = entity->m_nSkin();
	record_data->body = entity->m_nBody();

	record_data->lag = TICKS_TO_TIME(entity->m_flSimulationTime() - entity->m_flOldSimulationTime());

	// animations are off when we enter pvs, we do not want to shoot yet.
	record_data->valid = record_data->lag >= 0 && record_data->lag <= 17;

	// clamp it so we don't interpolate too far : )
	record_data->lag = Math::clamp(record_data->lag, 0, 17);

	memcpy(&record_data->animstate, entity->get_animation_state(), 0x334);

	record_data->pose_paramaters = entity->m_flPoseParameter();
	std::memcpy(record_data->anim_layers, entity->animation_layers_ptr(), 0x38 * entity->get_animation_layers_count());

	//record_data->shot_this_tick = (record_data->anim_layers[1].m_nSequence == 91 || record_data->anim_layers[1].m_nSequence == 93) && record_data->anim_layers[1].m_flWeight > 0.00f;

	record_data->tickcount = Source::m_pGlobalVars->tickcount;
	//record_data->orig_bonematrix_ptr = *entity->dwBoneMatrix();

	record_data->data_filled = true;
}

void c_lagcomp::apply_record_data(C_BasePlayer* entity, C_Tickrecord* record_data, bool force, bool lower_delta)
{
	if (entity == nullptr || !record_data->data_filled || !entity->get_animation_state())
		return;

	//*(matrix3x4_t**)((uintptr_t)entity + 0x2910) = &record_data->matrixes[0];

	auto curr = cheat::features::aaa.player_resolver_records[entity->entindex() - 1].resolving_method;
	record_data->bones_count = Math::clamp(record_data->bones_count, 0, 128);

	if (force && curr != 0 && !entity->IsBot()) 
	{
		std::memcpy(entity->m_CachedBoneData().Base(), /*(lower_delta ? (curr < 0 ? record_data->leftlmatrixes : record_data->rightlmatrixes) : */(curr < 0 ? record_data->leftmatrixes : record_data->rightmatrixes)/*)*/, record_data->bones_count * sizeof(matrix3x4_t));
		entity->set_abs_angles(QAngle(0, (curr < 0 ? record_data->left_side : record_data->right_side), 0));
		//memcpy(entity->get_animation_state(), &record_data->animstate, 0x330);
	}
	else {
		std::memcpy(entity->m_CachedBoneData().Base(), record_data->matrixes, record_data->bones_count * sizeof(matrix3x4_t));
		entity->set_abs_angles(QAngle(0, record_data->animstate.abs_yaw, 0));
	}

	entity->GetBoneCount() = record_data->bones_count;
	entity->m_vecOrigin() = record_data->origin;
	entity->m_vecVelocity() = record_data->velocity;
	entity->OBBMins() = record_data->object_mins;
	entity->OBBMaxs() = record_data->object_maxs;
	entity->m_angEyeAngles() = record_data->eye_angles;
	//entity->get_abs_eye_angles() = record_data->abs_eye_angles;
	//entity->m_nSequence() = record_data->sequence;
	entity->m_fFlags() = record_data->entity_flags;
	entity->m_flSimulationTime() = record_data->simulation_time;
	//entity->get_animation_state()->m_flDuckAmount = entity->m_flDuckAmount() = record_data->duck_amt;
	//entity->m_iEFlags() = record_data->ientity_flags;
	//entity->m_flLowerBodyYawTarget() = record_data->lower_body_yaw;
	
	//entity->m_flCycle() = record_data->cycle;
	entity->m_flDuckAmount() = record_data->duck_amt;

	memcpy(entity->get_animation_state(), &record_data->animstate, 0x334);

	//if (force) {
		//entity->m_flPoseParameter() = record_data->pose_paramaters;
		//std::memcpy(entity->animation_layers_ptr(), record_data->anim_layers, 0x38 * entity->get_animation_layers_count());
	//}

	//entity->set_abs_angles(QAngle(0, record_data->animstate.abs_yaw,0));
	entity->set_abs_origin(/*(backup ? record_data->abs_origin : */record_data->origin/*)*/);

	//if (backup)
	//	*entity->dwBoneMatrix() = (matrix3x4_t*)record_data->orig_bonematrix_ptr;
	//else
	//	*entity->dwBoneMatrix() = (matrix3x4_t*)&record_data->matrixes;
}

void c_lagcomp::extrapolate(C_BasePlayer* player, Vector& origin, Vector& velocity, int& flags, bool on_ground)
{
	static const auto sv_gravity = Source::m_pCvar->FindVar(_("sv_gravity"));
	static const auto sv_jump_impulse = Source::m_pCvar->FindVar(_("sv_jump_impulse"));

	if (!(flags & FL_ONGROUND))
		velocity.z -= TICKS_TO_TIME(sv_gravity->GetFloat());
	else if (player->m_fFlags() & FL_ONGROUND && !on_ground)
		velocity.z = sv_jump_impulse->GetFloat();

	const auto src = origin;
	auto end = src + velocity * Source::m_pGlobalVars->interval_per_tick;

	Ray_t r;
	r.Init(src, end, player->OBBMins(), player->OBBMaxs());

	trace_t t;
	CTraceFilter filter;
	filter.pSkip = player;

	Source::m_pEngineTrace->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

	if (t.fraction != 1.f)
	{
		for (auto i = 0; i < 2; i++)
		{
			velocity -= t.plane.normal * velocity.Dot(t.plane.normal);

			const auto dot = velocity.Dot(t.plane.normal);
			if (dot < 0.f)
				velocity -= Vector(dot * t.plane.normal.x,
					dot * t.plane.normal.y, dot * t.plane.normal.z);

			end = t.endpos + velocity * TICKS_TO_TIME(1.f - t.fraction);

			r.Init(t.endpos, end, player->OBBMins(), player->OBBMaxs());
			Source::m_pEngineTrace->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

			if (t.fraction == 1.f)
				break;
		}
	}

	origin = end = t.endpos;
	end.z -= 2.f;

	r.Init(origin, end, player->OBBMins(), player->OBBMaxs());
	Source::m_pEngineTrace->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

	flags &= FL_ONGROUND;

	if (t.DidHit() && t.plane.normal.z > .7f)
		flags |= FL_ONGROUND;
}

void c_lagcomp::predict_player(C_BasePlayer* entity, C_Tickrecord* current_record, C_Tickrecord* next_record)
{
	C_Simulationdata simulation_data;

	simulation_data.entity = entity;
	simulation_data.origin = current_record->origin;
	simulation_data.velocity = current_record->velocity;
	simulation_data.on_ground = current_record->entity_flags & FL_ONGROUND;
	simulation_data.data_filled = true;

	auto simulation_time_delta = current_record->simulation_time - next_record->simulation_time;
	auto simulation_time_clamped_delta = Math::clamp(simulation_time_delta, Source::m_pGlobalVars->interval_per_tick, 1.0f);
	auto delta_ticks_clamped = Math::clamp(TIME_TO_TICKS(simulation_time_clamped_delta), 1, 16);
	auto simulation_tick_delta = std::clamp(TIME_TO_TICKS(simulation_time_delta), 1, 16);
	auto delta_ticks = Math::clamp(TIME_TO_TICKS(simulation_time_delta), 1, 16);
	//auto delta_ticks = (Math::clamp(TIME_TO_TICKS(Source::m_pEngine->GetNetChannelInfo()->GetLatency(1) + Source::m_pEngine->GetNetChannelInfo()->GetLatency(0)) + Source::m_pGlobalVars->tickcount - TIME_TO_TICKS(current_record->simulation_time + cheat::main::lerp_time), 0, 100)) - simulation_tick_delta;

	// movement delta
	auto current_velocity_angle = RAD2DEG(atan2(current_record->velocity.y, current_record->velocity.x));
	auto next_velocity_angle = RAD2DEG(atan2(next_record->velocity.y, next_record->velocity.x));
	auto velocity_angle_delta = Math::NormalizeYaw(current_velocity_angle - next_velocity_angle);
	auto velocity_movement_delta = velocity_angle_delta / simulation_time_delta;

	bool inair = false;

	if (!(entity->m_fFlags() & 1) || !(current_record->entity_flags & 1))
		inair = 1;

	if (delta_ticks > TIME_TO_TICKS(Source::m_pEngine->GetNetChannelInfo()->GetLatency(FLOW_INCOMING)) && simulation_data.data_filled)
	{
		for (; delta_ticks >= 0; delta_ticks -= delta_ticks_clamped)
		{
			auto ticks_left = delta_ticks_clamped;

			do
			{
				extrapolate(entity, simulation_data.origin, simulation_data.velocity, entity->m_fFlags(), inair);
				current_record->simulation_time += Source::m_pGlobalVars->interval_per_tick;

				--ticks_left;
			} while (ticks_left);
		}

		current_record->origin = simulation_data.origin;
		current_record->abs_origin = simulation_data.origin;
	}
}

bool c_lagcomp::is_time_delta_too_large(C_Tickrecord* wish_record)
{
	const auto net = Source::m_pEngine->GetNetChannelInfo();
	static auto sv_maxunlag = Source::m_pCvar->FindVar("sv_maxunlag");

	const auto outgoing = net->GetLatency(FLOW_OUTGOING);
	const auto incoming = net->GetLatency(FLOW_INCOMING);

	float correct = 0.f;
	correct += outgoing;
	correct += incoming;
	correct += cheat::main::lerp_time;

	if (cheat::main::fakeducking)
		correct += TICKS_TO_TIME(14 - Source::m_pClientState->m_iChockedCommands);

	correct = std::clamp(correct, 0.f, sv_maxunlag->GetFloat());

	float deltaTime = std::abs(correct - (Source::m_pGlobalVars->curtime - wish_record->simulation_time));

	return deltaTime > 0.2f;
}

bool c_lagcomp::is_time_delta_too_large(const float &simulation_time)
{
	/*float correct = 0;

	correct += Source::m_pEngine->GetNetChannelInfo()->GetLatency(0);
	correct += Source::m_pEngine->GetNetChannelInfo()->GetLatency(1);
	correct += cheat::main::lerp_time;

	if (cheat::main::fakeducking)
	correct += TICKS_TO_TIME(14 - Source::m_pClientState->m_iChockedCommands);

	static auto sv_maxunlag = Source::m_pCvar->FindVar("sv_maxunlag");

	correct = std::clamp(correct, 0.f, sv_maxunlag->GetFloat());

	float deltaTime = std::abs(correct - (Source::m_pGlobalVars->curtime - wish_record->simulation_time));

	if (deltaTime < TICKS_TO_TIME(14))
	return false;

	return true;*/

	float correct = 0.f;

	correct += Source::m_pEngine->GetNetChannelInfo()->GetLatency(0);
	correct += Source::m_pEngine->GetNetChannelInfo()->GetLatency(1);
	correct += cheat::main::lerp_time;

	if (cheat::main::fakeducking)
		correct += TICKS_TO_TIME(14 - Source::m_pClientState->m_iChockedCommands);

	static auto sv_maxunlag = Source::m_pCvar->FindVar("sv_maxunlag");

	correct = std::clamp(correct, 0.f, sv_maxunlag->GetFloat());

	float deltaTime = std::abs(correct - (Source::m_pGlobalVars->curtime - simulation_time));

	return deltaTime > 0.2f;

	/*const auto net_channel = Source::m_pEngine->GetNetChannelInfo();

	if (!net_channel || !sv_maxunlag)
		return true;

	const auto lerp = cheat::main::lerp_time;
	auto correct = std::clamp(net_channel->GetLatency(0)
		+ net_channel->GetLatency(1)
		+ lerp, 0.f, sv_maxunlag->GetFloat());

	if (cheat::main::fakeducking)
		correct += TICKS_TO_TIME(14 - Source::m_pClientState->m_iChockedCommands);

	return (fabsf(correct - (Source::m_pGlobalVars->curtime - simulation_time)) > 0.2f);*/
}

int c_lagcomp::start_lag_compensation(C_BasePlayer* entity, size_t wish_tick, C_Tickrecord* output_record)
{
	//if (!should_lag_compensate(entity))
	//	return -1;

	auto player_index = entity->entindex() - 1;
	auto player_record = &records[player_index];

	if (player_record->m_Tickrecords.empty())// || (wish_tick + 1 > player_record->m_Tickrecords.size() - 1))
		return -1;

	auto current_record = player_record->m_Tickrecords.at(wish_tick);
	//auto next_record = player_record->m_Tickrecords.at(wish_tick + 1);

	if (!current_record.data_filled /*|| !next_record.data_filled*/ || wish_tick > 0 && is_time_delta_too_large(&current_record))
		return -1;

	if ((player_record->m_Tickrecords.size() > (wish_tick + 1)) && wish_tick == 0 && (current_record.origin - player_record->m_Tickrecords.at(wish_tick + 1).origin).LengthSquared() > 4096.f && !current_record.origin.IsZero() && !player_record->m_Tickrecords.at(wish_tick + 1).origin.IsZero() && current_record.tickcount != Source::m_pGlobalVars->tickcount && entity->m_vecVelocity().Length2D() > 270.f)
	{
		predict_player(entity, &current_record, &player_record->m_Tickrecords.at(wish_tick + 1));
		apply_record_data(entity, &current_record);
		entity->force_bone_rebuild();
		entity->SetupBonesEx();
		store_record_data(entity, &current_record);
	}

	if (output_record != nullptr && current_record.data_filled)
		*output_record = current_record;

	return TIME_TO_TICKS(current_record.simulation_time + cheat::main::lerp_time);
}

bool got_update[65];
float flick_time[65];

void c_lagcomp::update_player_record_data(C_BasePlayer* entity)
{
	//const auto net = Source::m_pEngine->GetNetChannelInfo();

	if (entity == nullptr /*|| net == nullptr*//* || cheat::game::last_cmd == nullptr*/)
		return;

	auto player_index = entity->entindex() - 1;
	auto player_record = &records[player_index];

	auto resolve_info = &cheat::features::aaa.player_resolver_records[player_index];

	if (!player_record->m_Tickrecords.empty() && player_record->m_Tickrecords.front().simulation_time >= entity->m_flSimulationTime())
		resolve_info->is_shifting = (player_record->m_Tickrecords.front().eye_angles != entity->m_angEyeAngles() || player_record->m_Tickrecords.front().origin != entity->m_vecOrigin());

	//auto lol = int(resolve_info->server_anim_layers[2].m_flWeight);

	const float tickrate = (1 / Source::m_pGlobalVars->interval_per_tick) / 4.f;

	const auto wtf = abs(Source::m_pGlobalVars->tickcount - TIME_TO_TICKS(entity->m_flSimulationTime()));
	auto unlag = wtf <= tickrate;

	resolve_info->simtime_updated = false;
	if (entity->m_flSimulationTime() > 0.f && player_record->m_Tickrecords.empty() && unlag || !player_record->m_Tickrecords.empty() && player_record->m_Tickrecords.front().simulation_time != entity->m_flSimulationTime() && unlag)
	{
		resolve_info->simtime_updated = true;

		auto pasti = player_record->m_Tickrecords;
		
		//if (!player_record->m_Tickrecords.empty())
			update_animations_data(entity);

		if (entity->m_flSimulationTime() > 0.f && player_record->m_Tickrecords.empty() || player_record->m_Tickrecords.front().simulation_time < entity->m_flSimulationTime())
		{
			C_Tickrecord new_record;
			store_record_data(entity, &new_record);

			if (new_record.data_filled) {
				player_record->m_Tickrecords.push_front(new_record);

				if (cheat::features::aaa.player_resolver_records[player_index].did_shot_this_tick)
					player_record->m_ShotBacktracking = new_record;
				if (!player_record->m_LastTick.data_filled || is_time_delta_too_large(&player_record->m_LastTick))
					player_record->m_LastTick = new_record;

				player_record->m_NewTick = new_record;
			}
		}
	}

	if (player_record->m_Tickrecords.size() > LAG_COMPENSATION_TICKS)
		player_record->m_Tickrecords.resize(LAG_COMPENSATION_TICKS);

	while (!player_record->m_Tickrecords.empty() && is_time_delta_too_large(&player_record->m_Tickrecords.back()))
		player_record->m_Tickrecords.pop_back();
	//else
	//	entity->client_side_animation() = true;
}

//struct lagcomp_mt
//{
//	lagcomp_mt() {  };
//
//	C_BasePlayer* entity;
//};

void c_lagcomp::start_position_adjustment()
{
	if (!cheat::Cvars.RageBot_AdjustPositions.GetValue())
		return;

	//cheat::main::fast_autostop = false;

	/*auto run_search_backtrack = [](void* _data) {
		lagcomp_mt *data = (lagcomp_mt*)_data;

		auto player_record = &cheat::features::lagcomp.records[data->entity->entindex() - 1];

		if (player_record->m_Tickrecords.size() <= 0)
			return;

		player_record->being_lag_compensated = true;
		cheat::features::lagcomp.start_position_adjustment(data->entity);
		player_record->being_lag_compensated = false;
	};*/

	for (auto index = 1; index < 64; index++)
	{
		auto entity = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(index);

		if (!entity || !entity->IsPlayer())
			continue;

		if (entity->IsDead() && entity->m_iTeamNum() != cheat::main::local()->m_iTeamNum())
		{
			cheat::main::shots_fired[index - 1] = 0;
			cheat::main::shots_total[index - 1] = 0;
			memset(cheat::features::aaa.player_resolver_records[index - 1].missed_shots, 0, sizeof(int) * 11);
		}

		if (entity->IsDormant() ||
			entity->m_iHealth() <= 0 ||
			entity->IsDead() ||
			entity->m_iTeamNum() == cheat::main::local()->m_iTeamNum()
			)
		{
			continue;
		}

		//if (!should_lag_compensate(entity))
		//	continue;

		/*lagcomp_mt adata;

		adata.entity = entity;

		Threading::QueueJobRef(run_search_backtrack, &adata);

		Threading::FinishQueue();

		auto player_record = &records[entity->entindex() - 1];

		if (player_record->m_Tickrecords.size() <= 0)
			continue;

		player_record->being_lag_compensated = true;
		start_position_adjustment(entity);
		player_record->being_lag_compensated = false;*/
	}
}

void c_lagcomp::start_position_adjustment(C_BasePlayer* entity)
{
	auto player_index = entity->entindex() - 1;
	auto player_record = &records[player_index];

	auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

	if (player_record->m_Tickrecords.empty() || !local_weapon)
		return;

	auto has_knife = (local_weapon->m_iItemDefinitionIndex() == weapon_knife_ct || local_weapon->m_iItemDefinitionIndex() == weapon_knife_t);
	auto has_zeus = local_weapon->m_iItemDefinitionIndex() == weapon_taser;

	player_record->backtrack_ticks = 0;
	player_record->tick_count = -1;
	player_record->type = 0;
	player_record->hitbox_position.clear();
	player_record->matrix_valid = false;

	store_record_data(entity, &player_record->restore_record);

	float max_dmg = (entity->m_iHealth() < cheat::Cvars.RageBot_MinDamage.GetValue() && cheat::Cvars.RageBot_ScaledmgOnHp.GetValue() ? entity->m_iHealth() : cheat::Cvars.RageBot_MinDamage.GetValue());
	TickrecordType type = RECORD_NORMAL;
	float max_delta = 58.f;
	float max_distance = 8196.f;

	// Just normally loop as we would
	for (size_t index = 1; index < player_record->m_Tickrecords.size(); index++)
	{
		if ((index + 1) > (player_record->m_Tickrecords.size() - 1) || index >= player_record->m_Tickrecords.size())
			break;

		auto current_record = &player_record->m_Tickrecords.at(index);

		C_Tickrecord* prev_record = nullptr;

		if (player_record->m_Tickrecords.size() > 2 && index < (player_record->m_Tickrecords.size() - 1)) {
			if (!is_time_delta_too_large(current_record))
				prev_record = &player_record->m_Tickrecords.at(index + 1);
			else
				prev_record = nullptr;
		}

		if (is_time_delta_too_large(current_record))
			continue;

		C_Tickrecord corrected_record;
		auto tick_count = start_lag_compensation(entity, index, &corrected_record);

		if (tick_count == -1 || !corrected_record.data_filled)
			continue;

		if (index > 0 && prev_record != nullptr && current_record->hitbox_positon == prev_record->hitbox_positon && current_record->origin == prev_record->origin)
			continue;

		apply_record_data(entity, &corrected_record);

		corrected_record.hitbox_positon = cheat::features::aimbot.get_hitbox(entity, 0, corrected_record.matrixes);
		auto left_leg = cheat::features::aimbot.get_hitbox(entity, 11, corrected_record.matrixes);
		auto chest = cheat::features::aimbot.get_hitbox(entity, HITBOX_PELVIS, corrected_record.matrixes);
		auto right_leg = cheat::features::aimbot.get_hitbox(entity, 12, corrected_record.matrixes);

		auto lol = (cheat::Cvars.RageBot_Hitboxes.GetValue() == 1 && cheat::Cvars.RageBot_Hitboxes.Has(0));

		auto hpass = cheat::features::aimbot.can_hit(0, entity, player_record->hitbox_position, corrected_record.matrixes, !lol);

		auto pass = hpass;

		if (pass <= 0.f && !lol)
			pass = cheat::features::aimbot.can_hit(HITBOX_PELVIS, entity, chest, corrected_record.matrixes, !cheat::Cvars.RageBot_Hitboxes.Has(0));
		//if (pass <= 0.f)
		//	pass = cheat::features::aimbot.can_hit(HITBOX_RIGHT_FOOT, entity, left_leg, corrected_record.matrixes, true);
		//if (pass <= 0.f)
		//	pass = cheat::features::aimbot.can_hit(HITBOX_LEFT_FOOT, entity, right_leg, corrected_record.matrixes, true);

		if (corrected_record.hitbox_positon.IsZero() || left_leg.IsZero() || chest.IsZero() || right_leg.IsZero())
			continue;

		player_record->hitbox_position = corrected_record.hitbox_positon;

		if (has_knife || has_zeus) 
		{
			auto chest = cheat::features::aimbot.get_hitbox(entity, HITBOX_PELVIS, corrected_record.matrixes);
			auto dist = cheat::main::local()->m_vecOrigin().Distance(chest);

			if (dist < max_distance)
			{
				max_distance = dist;

				player_record->tick_count = tick_count;
				player_record->type = corrected_record.type;
				std::memcpy(player_record->matrix, corrected_record.matrixes, sizeof(matrix3x4_t) * 128);
				player_record->matrix_valid = true;
				player_record->backtrack_ticks = index;
			}
		}
		else {
			if (pass > max_dmg)
			{
				player_record->tick_count = tick_count;
				player_record->type = corrected_record.type;
				std::memcpy(player_record->matrix, corrected_record.matrixes, sizeof(matrix3x4_t) * 128);
				player_record->matrix_valid = true;

				player_record->backtrack_ticks = index;
				max_dmg = pass;
				max_delta = corrected_record.desync_delta;
				type = (TickrecordType)(corrected_record.type);

				if (hpass > 0.f && current_record->type == RECORD_SHOT)
					break;
			}
		}
	}
}

void c_lagcomp::finish_position_adjustment()
{
	if (!cheat::Cvars.RageBot_AdjustPositions.GetValue())
		return;

	for (auto index = 1; index < 64; index++)
	{
		auto entity = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(index);

		if (!entity || !entity->IsPlayer())
			continue;

		if (entity->IsDormant() ||
			entity->m_iHealth() <= 0 ||
			entity->IsDead() ||
			entity->m_iTeamNum() == cheat::main::local()->m_iTeamNum()
			)
		{
			continue;
		}

		finish_position_adjustment(entity);
	}
}

std::unordered_map <int, float> m_simulation;

void c_lagcomp::fix_netvar_compression(C_BasePlayer* player) {
	static auto sv_gravity = Source::m_pCvar->FindVar(_("sv_gravity"));
	auto& history = records[player->entindex()-1].m_Tickrecords;

	if (history.size() < 2 || player == nullptr || player->get_animation_state() == nullptr || sv_gravity == nullptr)
		return;

	auto& record = history.front();
	auto& pre_record = history.at(1);

	auto v15 = (player->m_fFlags() & FL_ONGROUND) == 0;

	float chokedTime = player->m_flSimulationTime() - record.simulation_time;
	chokedTime = Math::clamp(chokedTime, Source::m_pGlobalVars->interval_per_tick, 1.0f);

	Vector origin = player->m_vecOrigin();
	Vector deltaOrigin = (origin - record.origin) * (1.0f / chokedTime);

	float penultimateChoke = record.simulation_time - pre_record.simulation_time;
	penultimateChoke = Math::clamp(penultimateChoke, Source::m_pGlobalVars->interval_per_tick, 1.0f);

	float delta = RAD2DEG(atan2((record.origin.y - pre_record.origin.y) * (1.0f / penultimateChoke),
		(record.origin.x - pre_record.origin.x) * (1.0f / penultimateChoke)));

	float direction = RAD2DEG(atan2(deltaOrigin.y, deltaOrigin.x));
	float deltaDirection = Math::normalize_angle(direction - delta);

	deltaDirection = DEG2RAD(deltaDirection * 0.5f + direction);

	float dirCos = cos(deltaDirection), dirSin = sin(deltaDirection);
	//DirectX::XMScalarSinCos(&dirSin, &dirCos, deltaDirection);

	float move = deltaOrigin.Length2D();
	Vector velocity;
	player->m_vecVelocity().x = dirCos * move;
	player->m_vecVelocity().y = dirSin * move;
	player->m_vecVelocity().z = deltaOrigin.z;

	if (!(player->m_fFlags() & FL_ONGROUND)) {
		player->m_vecVelocity().z -= sv_gravity->GetFloat() * chokedTime * 0.5f;
	}
	//static auto sv_accelerate = Source::m_pCvar->FindVar("sv_accelerate");

	//float delta = player->m_flSimulationTime() - record.simulation_time;
	//delta = max(delta, Source::m_pGlobalVars->interval_per_tick); // TICK_INTERVAL()

	//bool on_ground = (player->m_fFlags() & FL_ONGROUND || player->get_animation_state()->on_ground);

	//auto origin = player->m_vecOrigin();
	//auto origin_delta = origin - record.origin;

	//auto velocity = origin_delta / delta;
	//auto last_velocity = record.velocity;

	////if (velocity.Length2D() > last_velocity.Length2D())
	////	anim_vel = RebuildGameMovement::Get().Accelerate/*accelerate*/(this, anim_vel, wishdir, 250.f, sv_accelerate->GetFloat())

	//if (on_ground)
	//{

	//}
	//else
	//{
	//	player->m_vecVelocity() = Math::Lerp(Source::m_pGlobalVars->interval_per_tick / delta,
	//		last_velocity,
	//		velocity
	//	);
	//}
}

float spawntime;

void c_lagcomp::update_animations(C_BasePlayer* player, C_Tickrecord* from, int Resolver_Index, float angle)
{
	auto resolver_info = &cheat::features::aaa.player_resolver_records[player->entindex()-1];

	player->set_abs_origin(player->m_vecOrigin());
	auto lag = from == nullptr ? 1 : TICKS_TO_TIME(player->m_flSimulationTime() - from->simulation_time);
	lag = Math::clamp(lag, 1, 17);

	if (!cheat::Cvars.Misc_AntiUT.GetValue() && from)
	{
		resolver_info->prev_delta = player->m_angEyeAngles().x;

		player->m_angEyeAngles().x = std::clamp(Math::normalize_angle(player->m_angEyeAngles().x), -89.f, 89.f);

		if (!(player->m_fFlags() & FL_ONGROUND) && from->entity_flags & FL_ONGROUND && player->m_angEyeAngles().x >= 178.36304f)
			player->m_angEyeAngles().x = -89.f;
		else
		{
			if (abs(player->m_angEyeAngles().x) > 89.f)
				player->m_angEyeAngles().x = cheat::main::shots_fired[player->entindex() - 1] % 4 != 3 ? 89.f : -89.f;
			else
				player->m_angEyeAngles().x = cheat::main::shots_fired[player->entindex() - 1] % 4 != 3 ? 89.f : resolver_info->prev_delta;
		}
	}

	if (from == nullptr || !resolver_info->preserver_animlayers_saved || (lag - 1) <= 1)
	{
		// set velocity and layers.
		resolver_info->new_velocity = player->m_vecVelocity();
		resolver_info->force_velocity = true;

		// fix feet spin.
		player->get_animation_state()->feet_rate = 0.f;

		float yaw = player->m_angEyeAngles().y;

		if (Resolver_Index)
		{
			if (resolver_info->left_side != FLT_MAX && resolver_info->right_side != FLT_MAX)
			{
				if (Resolver_Index <= 0)
					yaw = resolver_info->right_side;
				else
					yaw = resolver_info->left_side;
			}
			else 
			{
				if (Resolver_Index <= 0)
					yaw = player->m_angEyeAngles().y - angle; /*60 a.k.a left*/
				else                                                             // <- those are 60 in the dump and not a custom value inserted from the function argument
					yaw = player->m_angEyeAngles().y + angle; /*60 a.k.a right*/
			}

			yaw = Math::normalize_angle(yaw);
			player->get_animation_state()->abs_yaw = yaw;
		}

		cheat::main::updating_resolver_anims = true;
		player->update_clientside_animations();
		cheat::main::updating_resolver_anims = false;
		resolver_info->force_velocity = false;
		player->invalidate_anims();
		return;
	}

	const auto new_velocity = player->m_vecVelocity();
	const auto new_flags = player->m_fFlags();
	//const auto new_angles = resolver_info->previous_rotation;

	// restore old record.
	//*player->get_animation_layers() = from->layers;
	memcpy(player->animation_layers_ptr(), resolver_info->preserver_anim_layers, 0x38 * player->get_animation_layers_count());
	player->m_vecOrigin() = from->origin;
	player->set_abs_angles(from->abs_eye_angles);
	player->m_vecVelocity() = from->velocity;

	// setup velocity.
	//resolver_info->new_velocity = new_velocity;

	// did the player shoot?
	//const auto shot = resolver_info->last_shot_time > from->simulation_time && resolver_info->last_shot_time <= player->m_flSimulationTime();

	// setup extrapolation parameters.
	auto old_origin = from->origin;
	auto old_flags = from->entity_flags;

	float yaw = resolver_info->previous_rotation;

	for (auto i = 0; i < lag; i++)
	{
		// move time forward.
		const auto time = from->simulation_time + TICKS_TO_TIME(i + 1);
		const auto lerp = 1.f - (player->m_flSimulationTime() - time) / (player->m_flSimulationTime() - from->simulation_time);

		// lerp eye angles.
		//auto eye_angles = Math::interpolate(from->eye_angles, player->m_angEyeAngles(), lerp);
		//eye_angles.y = Math::normalize_angle(eye_angles.y);
		//player->m_angEyeAngles().y = eye_angles.y;
		resolver_info->new_velocity = Math::interpolate(from->velocity, player->m_vecVelocity(), lerp);

		// lerp duck amount.
		player->m_flDuckAmount() = Math::interpolate(from->duck_amt, player->m_flDuckAmount(), lerp);

		// resolve player.
		if ((lag - 1) == i)
		{
			resolver_info->new_velocity = new_velocity;
			player->m_fFlags() = new_flags;

			if ((resolver_info->did_shot_this_tick && resolver_info->last_shot_time > player->m_flSimulationTime() || !resolver_info->did_shot_this_tick) && Resolver_Index != 0)
			{
				if (Resolver_Index <= 0)
					yaw = resolver_info->previous_rotation - angle;
				else
					yaw = resolver_info->previous_rotation + angle;

				yaw = Math::normalize_angle(yaw);
				*(float*)(uintptr_t(player->get_animation_state()) + 0x80) = yaw;
			}
		}
		else // compute velocity and flags.
		{
			extrapolate(player, old_origin, player->m_vecVelocity(), player->m_fFlags(), old_flags & FL_ONGROUND);
			old_flags = player->m_fFlags();
		}

		// fix feet spin.
		player->get_animation_state()->feet_cycle /*= record.feet_cycle*/ = from->anim_layers[6].m_flCycle;
		player->get_animation_state()->feet_rate /*= record.feet_yaw_rate*/ = from->anim_layers[6].m_flWeight;

		// backup simtime.
		const auto backup_simtime = player->m_flSimulationTime();
		resolver_info->force_velocity = true;
		cheat::main::updating_resolver_anims = true;

		// set new simtime.
		player->m_flSimulationTime() = time;

		// run update.
		player->update_clientside_animations();
		resolver_info->force_velocity = false;
		cheat::main::updating_resolver_anims = false;
		// restore old simtime.
		player->m_flSimulationTime() = backup_simtime;
	}

	player->invalidate_anims();
}

void c_lagcomp::update_local_animations_data(CUserCmd* cmd, bool& send_packet)
{
	auto animstate = cheat::main::local()->get_animation_state();

	if (!animstate || animstate->ent != cheat::main::local() || !send_packet)
		return;

	float prev[2];

	if (cheat::main::local()->m_flSpawnTime() != spawntime)
	{
		//using ResetAnimState_t = void(__thiscall*)(CCSGOPlayerAnimState*);
		//static auto ResetAnimState = (ResetAnimState_t)Memory::Scan(cheat::main::clientdll, "56 6A 01 68 ? ? ? ? 8B F1");

		//if (ResetAnimState != nullptr)
		//	ResetAnimState(cheat::main::local()->get_animation_state());

		cheat::main::local()->update_clientside_animations();

		animstate = cheat::main::local()->get_animation_state();
		spawntime = cheat::main::local()->m_flSpawnTime();
	}

	CCSGOPlayerAnimState state;
	C_AnimationLayer backuplayers[15];
	matrix3x4_t backup[128];
	memcpy(backuplayers, cheat::main::local()->animation_layers_ptr(), 0x38 * cheat::main::local()->get_animation_layers_count());

	const auto v63 = Source::m_pGlobalVars->curtime;
	const auto v62 = Source::m_pGlobalVars->frametime;
	Source::m_pGlobalVars->curtime = cheat::main::local()->m_flSimulationTime();
	Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;

	const auto backup_poses = cheat::main::local()->m_flPoseParameter();
	const auto backup_origin = cheat::main::local()->m_vecOrigin();
	const auto backup_absorigin = cheat::main::local()->get_abs_origin();
	const auto backup_vel = cheat::main::local()->m_vecVelocity();
	const auto backup_absvel = cheat::main::local()->m_vecAbsVelocity();
	const auto backup_flags = cheat::main::local()->m_fFlags();
	const auto backup_eflags = cheat::main::local()->m_iEFlags();
	const auto backup_duckamt = cheat::main::local()->m_flDuckAmount();
	const auto backup_lby = cheat::main::local()->m_flLowerBodyYawTarget();
	const auto backup_renderang = cheat::main::local()->get_render_angles();

	cheat::main::local()->get_render_angles() = QAngle(cheat::features::antiaimbot.visual_real_angle.x, cmd->viewangles.y, cmd->viewangles.z);
	memcpy(backup, cheat::main::local()->m_CachedBoneData().Base(), cheat::main::local()->GetBoneCount() * sizeof(matrix3x4_t));

	/* REMOVED BLACK MAGIC HERE TROLOLOL */
	animstate->feet_cycle = prev[0];
	animstate->feet_rate = prev[1];
	animstate->unk_frac = 0.f;

	if (animstate->hitgr_anim)
		animstate->time_since_inair = 0.f;

	{
		memcpy(&state, animstate, 0x334);
		cheat::main::local()->get_animation_state()->abs_yaw = cheat::main::local()->m_flLowerBodyYawTarget();
		//cheat::main::local()->get_animation_state()->last_anim_upd_tick = Source::m_pGlobalVars->framecount - 1;
		//cheat::main::local()->get_animation_state()->last_anim_upd_time = Source::m_pGlobalVars->curtime - TICKS_TO_TIME(1);
		cheat::main::local()->client_side_animation() = true;
		cheat::main::updating_anims = true;
		Memory::VCall<void(__thiscall*)(void*)>(cheat::main::local(), UPDATE_CLIENTSIDE_ANIMATIONS)(cheat::main::local());
		cheat::main::updating_anims = false;

		cheat::main::fake_angle = cheat::main::local()->get_animation_state()->abs_yaw;
		cheat::main::fake_pose = cheat::main::local()->m_flPoseParameter();

		memcpy(cheat::main::local()->animation_layers_ptr(), backuplayers, 0x38 * cheat::main::local()->get_animation_layers_count());

		cheat::main::local()->m_vecOrigin() = backup_origin;
		cheat::main::local()->m_vecVelocity() = backup_vel;
		cheat::main::local()->m_vecAbsVelocity() = backup_absvel;
		cheat::main::local()->m_fFlags() = backup_flags;
		cheat::main::local()->m_iEFlags() = backup_eflags;
		cheat::main::local()->m_flDuckAmount() = backup_duckamt;
		cheat::main::local()->m_flLowerBodyYawTarget() = backup_lby;
		//cheat::main::local()->get_render_angles() = backup_renderang;

		if (animstate->speed_2d < 0.1 && animstate->on_ground && cheat::main::local()->get_sec_activity(backuplayers[3].m_nSequence) == 979)
		{
			cheat::main::local()->get_animation_layer(3).m_flWeight = 0.f;
			cheat::main::local()->get_animation_layer(3).m_flCycle = 0.f;
		}

		cheat::main::local()->force_bone_rebuild();
		auto lol = cheat::main::local()->SetupBonesEx();
		cheat::main::local()->m_flPoseParameter() = backup_poses;
		memcpy(cheat::features::antiaimbot.last_sent_matrix, cheat::main::local()->m_CachedBoneData().Base(), cheat::main::local()->GetBoneCount() * sizeof(matrix3x4_t));
		memcpy(cheat::main::local()->animation_layers_ptr(), backuplayers, 0x38 * cheat::main::local()->get_animation_layers_count());
		memcpy(cheat::main::local()->get_animation_state(), &state, 0x334);
		memcpy(cheat::main::local()->m_CachedBoneData().Base(), backup, cheat::main::local()->GetBoneCount() * sizeof(matrix3x4_t));
	}

	if (!m_local_chocked_angles.empty()) {
		cheat::main::local()->get_animation_state()->abs_yaw = m_local_chocked_angles.back().angles.y;
		//cheat::main::local()->m_fFlags() = m_local_chocked_angles.back().flags;
		//cheat::main::local()->m_flDuckAmount() = m_local_chocked_angles.back().duck_amt;
		//cheat::main::local()->m_flLowerBodyYawTarget() = m_local_chocked_angles.back().lby;
	}

	//cheat::main::local()->get_animation_state()->last_anim_upd_tick = Source::m_pGlobalVars->framecount - 1;
	//cheat::main::local()->get_animation_state()->last_anim_upd_time = Source::m_pGlobalVars->curtime - TICKS_TO_TIME(1);

	cheat::main::local()->get_animation_state()->duck_amt = cheat::main::local()->m_flDuckAmount();

	cheat::main::local()->client_side_animation() = true;
	cheat::main::updating_anims = true;
	Memory::VCall<void(__thiscall*)(void*)>(cheat::main::local(), UPDATE_CLIENTSIDE_ANIMATIONS)(cheat::main::local());
	cheat::main::updating_anims = false;

	cheat::main::real_angle = cheat::main::local()->get_animation_state()->abs_yaw;
	cheat::main::real_pose = cheat::main::local()->m_flPoseParameter();

	memcpy(cheat::main::local()->animation_layers_ptr(), backuplayers, 0x38 * cheat::main::local()->get_animation_layers_count());

	//m_player->m_flPoseParameter() = backup_poses;
	cheat::main::local()->m_vecOrigin() = backup_origin;
	cheat::main::local()->m_vecVelocity() = backup_vel;
	cheat::main::local()->m_vecAbsVelocity() = backup_absvel;
	cheat::main::local()->m_fFlags() = backup_flags;
	cheat::main::local()->m_iEFlags() = backup_eflags;
	cheat::main::local()->m_flDuckAmount() = backup_duckamt;
	cheat::main::local()->m_flLowerBodyYawTarget() = backup_lby;
	cheat::main::local()->get_render_angles() = backup_renderang;

	if (animstate->speed_2d < 0.1 && animstate->on_ground && cheat::main::local()->get_sec_activity(backuplayers[3].m_nSequence) == 979)
	{
		cheat::main::local()->get_animation_layer(3).m_flWeight = 0.f;
		cheat::main::local()->get_animation_layer(3).m_flCycle = 0.f;
	}

	build_local_bones(cheat::main::local());

	cheat::features::antiaimbot.min_delta = animstate->min_yaw;
	cheat::features::antiaimbot.max_delta = animstate->max_yaw;

	float max_speed = 260.f;

	if (cheat::main::local()->get_weapon() != nullptr)
	{
		const auto info = cheat::main::local()->get_weapon()->GetCSWeaponData();

		if (info)
			max_speed = max(.001f, info->max_speed);
	}

	auto velocity = cheat::main::local()->m_vecVelocity();
	const auto abs_velocity_length = powf(velocity.Length(), 2.f);
	const auto fraction = 1.f / (abs_velocity_length + .00000011920929f);

	if (abs_velocity_length > 97344.008f)
		velocity *= velocity * 312.f;

	auto speed = velocity.Length();

	if (speed >= 260.f)
		speed = 260.f;

	cheat::features::antiaimbot.feet_speed_stand = (1.923077f / max_speed) * speed;
	cheat::features::antiaimbot.feet_speed_ducked = (2.9411764f / max_speed) * speed;

	auto feet_speed = (((cheat::features::antiaimbot.stop_to_full_running_fraction * -.3f) - .2f) * std::clamp(cheat::features::antiaimbot.feet_speed_stand, 0.f, 1.f)) + 1.f;

	if (animstate->duck_amt > 0.f)
		feet_speed = feet_speed + ((std::clamp(cheat::features::antiaimbot.feet_speed_ducked, 0.f, 1.f) * animstate->duck_amt) * (.5f - feet_speed));

	cheat::features::antiaimbot.min_delta *= feet_speed;
	cheat::features::antiaimbot.max_delta *= feet_speed;

	if (cheat::features::antiaimbot.stop_to_full_running_fraction > 0.0 && cheat::features::antiaimbot.stop_to_full_running_fraction < 1.0)
	{
		const auto interval = Source::m_pGlobalVars->interval_per_tick * 2.f;

		if (cheat::features::antiaimbot.is_standing)
			cheat::features::antiaimbot.stop_to_full_running_fraction = cheat::features::antiaimbot.stop_to_full_running_fraction - interval;
		else
			cheat::features::antiaimbot.stop_to_full_running_fraction = interval + cheat::features::antiaimbot.stop_to_full_running_fraction;

		cheat::features::antiaimbot.stop_to_full_running_fraction = std::clamp(cheat::features::antiaimbot.stop_to_full_running_fraction, 0.f, 1.f);
	}

	if (speed > 135.2f && cheat::features::antiaimbot.is_standing)
	{
		cheat::features::antiaimbot.stop_to_full_running_fraction = fmaxf(cheat::features::antiaimbot.stop_to_full_running_fraction, .0099999998f);
		cheat::features::antiaimbot.is_standing = false;
	}

	if (speed < 135.2f && !cheat::features::antiaimbot.is_standing)
	{
		cheat::features::antiaimbot.stop_to_full_running_fraction = fminf(cheat::features::antiaimbot.stop_to_full_running_fraction, .99000001f);
		cheat::features::antiaimbot.is_standing = true;
	}

	Source::m_pGlobalVars->curtime = v63;
	Source::m_pGlobalVars->frametime = v62;

	prev[0] = backuplayers[6].m_flWeight;
	prev[1] = backuplayers[6].m_flCycle;
	//cheat::main::local()->DrawServerHitboxes();
}

//void c_lagcomp::update_local_animations_data(CUserCmd* cmd, bool& send_packet)
//{
//	auto animstate = cheat::main::local()->get_animation_state();
//
//	if (!animstate || animstate->ent != cheat::main::local())
//		return;
//
//	float prev[2];
//
//	if (cheat::main::local()->m_flSpawnTime() != spawntime)
//	{
//		cheat::main::local()->update_clientside_animations();
//
//		animstate = cheat::main::local()->get_animation_state();
//		spawntime = cheat::main::local()->m_flSpawnTime();
//	}
//
//	C_AnimationLayer backuplayers[15];
//	memcpy(backuplayers, cheat::main::local()->animation_layers_ptr(), 0x38 * cheat::main::local()->get_animation_layers_count());
//
//	const auto backup_poses = cheat::main::local()->m_flPoseParameter();
//	const auto backup_origin = cheat::main::local()->m_vecOrigin();
//	const auto backup_absorigin = cheat::main::local()->get_abs_origin();
//	const auto backup_vel = cheat::main::local()->m_vecVelocity();
//	const auto backup_absvel = cheat::main::local()->m_vecAbsVelocity();
//	const auto backup_flags = cheat::main::local()->m_fFlags();
//	const auto backup_eflags = cheat::main::local()->m_iEFlags();
//	const auto backup_duckamt = cheat::main::local()->m_flDuckAmount();
//	const auto backup_lby = cheat::main::local()->m_flLowerBodyYawTarget();
//	const auto backup_simtime = cheat::main::local()->m_flSimulationTime();
//	const auto backup_deadflagang = cheat::main::local()->get_render_angles();
//
//	if (!m_local_chocked_angles.empty())
//	{
//		const auto first_simtime = m_local_chocked_angles.front().simtime;
//		int piska = 0;
//
//		for (const auto& v : m_local_chocked_angles)
//		{
//			const auto v196 = Source::m_pGlobalVars->curtime;
//			const auto v197 = Source::m_pGlobalVars->frametime;
//			const auto v198 = Source::m_pGlobalVars->absoluteframetime;
//			const auto v199 = Source::m_pGlobalVars->interpolation_amount;
//			const auto timing = first_simtime + TICKS_TO_TIME(piska++);
//			Source::m_pGlobalVars->curtime = timing;
//			Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;
//			Source::m_pGlobalVars->absoluteframetime = Source::m_pGlobalVars->interval_per_tick;
//			Source::m_pGlobalVars->interpolation_amount = 0.0f;
//
//			cheat::main::local()->m_flLowerBodyYawTarget() = v.lby;
//			cheat::main::local()->get_render_angles() = v.angles;
//			//cheat::main::local()->get_render_angles().x = cheat::features::antiaimbot.visual_real_angle.x;
//			cheat::main::local()->m_flDuckAmount() = v.duck_amt;
//			cheat::main::local()->m_fFlags() = v.flags;
//
//			cheat::main::local()->get_animation_state()->feet_rate = 0.f;
//			cheat::main::local()->m_flSimulationTime() = timing;
//
//			//cheat::main::local()->update_clientside_animations();
//
//			//if (cheat::main::local()->get_animation_state()->last_anim_upd_tick == Source::m_pGlobalVars->framecount)
//			//	cheat::main::local()->get_animation_state()->last_anim_upd_tick -= 1;
//
//			cheat::main::local()->client_side_animation() = true;
//			cheat::main::updating_anims = true;
//			Memory::VCall<void(__thiscall*)(void*)>(cheat::main::local(), UPDATE_CLIENTSIDE_ANIMATIONS)(cheat::main::local());
//			cheat::main::updating_anims = false;
//
//			Source::m_pGlobalVars->curtime = v196;
//			Source::m_pGlobalVars->frametime = v197;
//			Source::m_pGlobalVars->absoluteframetime = v198;
//			Source::m_pGlobalVars->interpolation_amount = v199;
//
//			/*do lby there*/
//		}
//
//		cheat::main::local()->force_bone_rebuild();
//		cheat::main::local()->set_abs_angles(QAngle(0, cheat::main::real_angle, 0));
//		cheat::main::setuped_bones = cheat::main::local()->SetupBonesEx();
//		memcpy(cheat::features::antiaimbot.last_sent_matrix, cheat::main::local()->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * cheat::main::local()->GetBoneCount());
//
//		cheat::main::local()->m_flSimulationTime() = backup_simtime;
//		cheat::main::local()->m_flLowerBodyYawTarget() = backup_lby;
//		cheat::main::local()->get_render_angles() = backup_deadflagang;
//		cheat::main::local()->m_flDuckAmount() = backup_duckamt;
//		cheat::main::local()->m_fFlags() = backup_flags;
//	}
//
//	memcpy(cheat::main::local()->animation_layers_ptr(), backuplayers, 0x38 * cheat::main::local()->get_animation_layers_count());
//
//	if (send_packet) 
//	{
//		m_local_chocked_angles.clear();
//	
//		const auto v196 = Source::m_pGlobalVars->curtime;
//		const auto v197 = Source::m_pGlobalVars->frametime;
//		const auto v199 = Source::m_pGlobalVars->interpolation_amount;
//
//		Source::m_pGlobalVars->curtime = backup_simtime;
//		Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;
//		Source::m_pGlobalVars->interpolation_amount = 0.0f;
//		cheat::main::local()->get_render_angles() = cmd->viewangles;
//		cheat::main::local()->get_render_angles().x = cheat::features::antiaimbot.visual_real_angle.x;
//
//		cheat::main::local()->get_animation_state()->feet_rate = 0.f;
//
//		//if (cheat::main::local()->get_animation_state()->last_anim_upd_tick == Source::m_pGlobalVars->framecount)
//		//	cheat::main::local()->get_animation_state()->last_anim_upd_tick -= 1;
//
//		cheat::main::local()->client_side_animation() = true;
//		cheat::main::updating_anims = true;
//		Memory::VCall<void(__thiscall*)(void*)>(cheat::main::local(), UPDATE_CLIENTSIDE_ANIMATIONS)(cheat::main::local());
//		cheat::main::updating_anims = false;
//
//		Source::m_pGlobalVars->curtime = v196;
//		Source::m_pGlobalVars->frametime = v197;
//		Source::m_pGlobalVars->interpolation_amount = v199;
//
//		//_events.push_back(_event(Source::m_pGlobalVars->curtime + Source::m_pGlobalVars->interval_per_tick * 2, std::string("y: " + std::to_string(int(cmd->viewangles.y)))));
//		
//		cheat::main::real_angle = cheat::main::local()->get_animation_state()->abs_yaw;
//	}
//
//	build_local_bones(cheat::main::local());
//
//	//memcpy(cheat::main::local()->animation_layers_ptr(), backuplayers, 0x38 * cheat::main::local()->get_animation_layers_count());
//
//	cheat::main::local()->DrawServerHitboxes();
//
//	//for (auto i = 0; i < m_local_chocked_angles.size(); i++)
//	//{
//	//	auto v = m_local_chocked_angles[i];
//	//
//	//	int bkp1 = cheat::main::local()->m_fFlags();
//	//	float bkp2 = cheat::main::local()->m_flDuckAmount();
//	//
//	//	cheat::main::local()->get_render_angles() = v.angles;
//	//	//cheat::main::local()->m_angEyeAngles() = v.angles;
//	//	cheat::main::local()->m_fFlags() = v.flags;
//	//	cheat::main::local()->m_flDuckAmount() = v.duck_amt;
//	//	//cheat::main::local()->m_flSimulationTime() = TICKS_TO_TIME(v.tickbase);
//	//
//	//	Source::m_pGlobalVars->curtime = TICKS_TO_TIME(v.tickbase);
//	//	Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;
//	//
//	//	//cheat::main::local()->get_animation_state()->feet_rate = 0;
//	//	//serverAnim[i].Update(v.tickBase*I::pGlobals->interval_per_tick, v.tickBase);
//	//	//cheat::main::local()->set_abs_angles(v.angles);
//	//	cheat::main::local()->update_clientside_animations();
//	//
//	//	cheat::main::local()->m_vecOrigin() = backup_origin;
//	//	cheat::main::local()->m_vecVelocity() = backup_vel;
//	//	cheat::main::local()->m_vecAbsVelocity() = backup_absvel;
//	//	cheat::main::local()->m_fFlags() = backup_flags;
//	//	cheat::main::local()->m_iEFlags() = backup_eflags;
//	//	cheat::main::local()->m_flDuckAmount() = backup_duckamt;
//	//	cheat::main::local()->m_flSimulationTime() = backup_simtime;
//	//	cheat::main::local()->m_flLowerBodyYawTarget() = backup_lby;
//	//	cheat::main::local()->get_render_angles() = backup_deadflagang;
//	//	Source::m_pGlobalVars->frametime = b1;
//	//	Source::m_pGlobalVars->curtime = b2;
//	//
//	//	if (i + 1 == m_local_chocked_angles.size())
//	//	{ 
//	//		cheat::main::local()->set_abs_angles(QAngle(0, cheat::main::local()->get_animation_state()->abs_yaw, 0));
//	//		cheat::main::local()->force_bone_rebuild();
//	//		cheat::main::local()->SetupBonesEx();
//	//	}
//	//}
//}

void c_lagcomp::build_local_bones(C_BasePlayer* local)
{
	local->force_bone_rebuild();
	local->set_abs_angles(QAngle(0, cheat::main::real_angle, 0));
	cheat::main::setuped_bones = local->SetupBonesEx();
}

//void c_lagcomp::update_local_animations_data(CUserCmd* cmd, bool& send_packet)
//{
//	auto animstate = cheat::main::local()->get_animation_state();
//
//	if (!animstate || animstate->ent != cheat::main::local())
//		return;
//
//	if (cheat::main::local()->m_flSpawnTime() != spawntime)
//	{
//		//using ResetAnimState_t = void(__thiscall*)(CCSGOPlayerAnimState*);
//		//static auto ResetAnimState = (ResetAnimState_t)Memory::Scan(cheat::main::clientdll, "56 6A 01 68 ? ? ? ? 8B F1");
//
//		//if (ResetAnimState != nullptr)
//		//	ResetAnimState(m_player->get_animation_state());
//
//		cheat::main::local()->update_clientside_animations();
//
//		animstate = cheat::main::local()->get_animation_state();
//		spawntime = cheat::main::local()->m_flSpawnTime();
//	}
//	//cheat::features::lagcomp.m_local_chocked_angles
//
//	CCSGOPlayerAnimState backupstate;
//	memcpy(&backupstate, animstate, 0x334);
//	C_AnimationLayer backuplayers[15];
//	memcpy(backuplayers, cheat::main::local()->animation_layers_ptr(), 0x38 * cheat::main::local()->get_animation_layers_count());
//	const auto v7 = animstate->origin;
//	const auto v9 = animstate->velocity;
//	const auto v65 = animstate->velocity_norm_2d;
//	*(float *)((DWORD)animstate + 0xE0 + 8) = v9.z;
//
//	const auto v63 = Source::m_pGlobalVars->curtime;
//	const auto v62 = Source::m_pGlobalVars->frametime;
//
//	const auto backup_poses = cheat::main::local()->m_flPoseParameter();
//	const auto backup_origin = cheat::main::local()->m_vecOrigin();
//	const auto backup_absorigin = cheat::main::local()->get_abs_origin();
//	const auto backup_vel = cheat::main::local()->m_vecVelocity();
//	const auto backup_absvel = cheat::main::local()->m_vecAbsVelocity();
//	const auto backup_flags = cheat::main::local()->m_fFlags();
//	const auto backup_eflags = cheat::main::local()->m_iEFlags();
//	const auto backup_duckamt = cheat::main::local()->m_flDuckAmount();
//	const auto backup_lby = cheat::main::local()->m_flLowerBodyYawTarget();
//
//	//animstate->abs_yaw = yaw;
//
//	auto in_tp = cheat::main::thirdperson && cheat::Cvars.Visuals_lp_forcetp.GetValue();
//
//	int ticks = 0;
//
//	if (!m_local_chocked_angles.empty())
//	{
//		auto last = m_local_chocked_angles.at(m_local_chocked_angles.size() - 1);
//
//		float b1 = Source::m_pGlobalVars->frametime;
//		float b2 = Source::m_pGlobalVars->curtime;
//		int b4 = Source::m_pGlobalVars->framecount;
//		float b5 = cheat::main::local()->m_flLowerBodyYawTarget();
//		auto b6 = cheat::main::local()->m_angEyeAngles();
//		Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;
//		Source::m_pGlobalVars->curtime = cheat::main::local()->m_flSimulationTime() + TICKS_TO_TIME(m_local_chocked_angles.size());
//		Source::m_pGlobalVars->framecount = Source::m_pGlobalVars->tickcount;
//
//		//cheat::main::local()->SetLowerBodyYawTarget(last.lby);
//		cheat::main::local()->m_angEyeAngles() = last.angles;
//		cheat::main::local()->update_clientside_animations();
//
//		cheat::main::real_angle = cheat::main::local()->get_animation_state()->abs_yaw;
//
//		Source::m_pGlobalVars->frametime = b1;
//		Source::m_pGlobalVars->curtime = b2;
//		Source::m_pGlobalVars->framecount = b4;
//		cheat::main::local()->m_flLowerBodyYawTarget() = b5;
//		cheat::main::local()->m_angEyeAngles() = b6;
//
//		cheat::main::local()->m_flPoseParameter() = backup_poses;
//		cheat::main::local()->m_vecOrigin() = backup_origin;
//		cheat::main::local()->m_vecVelocity() = backup_vel;
//		cheat::main::local()->m_vecAbsVelocity() = backup_absvel;
//		cheat::main::local()->m_fFlags() = backup_flags;
//		cheat::main::local()->m_iEFlags() = backup_eflags;
//		cheat::main::local()->m_flDuckAmount() = backup_duckamt;
//		cheat::main::local()->m_flLowerBodyYawTarget() = backup_lby;
//		memcpy(animstate, &backupstate, 0x334);
//		memcpy(cheat::main::local()->animation_layers_ptr(), backuplayers, 0x38 * cheat::main::local()->get_animation_layers_count());
//	}
//
//	for (auto &d : m_local_chocked_angles)
//	{
//		Source::m_pGlobalVars->curtime = cheat::main::local()->m_flSimulationTime() + TICKS_TO_TIME(ticks++);
//		Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;
//
//		if (in_tp)
//			cheat::main::local()->get_render_angles() = QAngle(d.angles.x, d.angles.y, d.angles.z);
//
//		cheat::main::local()->set_abs_origin(cheat::main::local()->m_vecOrigin());
//		cheat::main::local()->m_vecAbsVelocity() = cheat::main::local()->m_vecVelocity();
//		cheat::main::local()->m_iEFlags() &= ~EFL_DIRTY_ABSVELOCITY;
//
//		animstate->feet_rate = 0.0f;
//		animstate->unk_frac = 0.f;
//
//		if (animstate->speed_2d < 0.1 && animstate->on_ground && cheat::main::local()->get_sec_activity(backuplayers[3].m_nSequence) == 979)
//		{
//			cheat::main::local()->get_animation_layer(3).m_flWeight = 0.f;
//			cheat::main::local()->get_animation_layer(3).m_flCycle = 0.f;
//		}
//
//		//if (animstate->on_ground || (m_player->m_fFlags() & FL_ONGROUND))
//		//	animstate->time_since_inair = 0.f;
//
//		//if (animstate->hitgr_anim)
//		//	animstate->time_since_inair = 0.f;
//
//		//animstate->abs_yaw = cheat::features::antiaimbot.visual_real_angle.y;
//
//		cheat::main::local()->m_fFlags() = d.flags;
//		cheat::main::local()->m_angEyeAngles() = d.angles;
//		cheat::main::local()->m_flDuckAmount() = d.duck_amt;
//
//		//g_bIsUpdatingAnimations = 1;
//		cheat::main::local()->update_clientside_animations();
//		//g_bIsUpdatingAnimations = 0;
//
//		//auto daun = Math::normalize_angle(cheat::features::antiaimbot.visual_fake_angle.y - cheat::features::antiaimbot.visual_real_angle.y);
//		//cheat::main::local()->m_flPoseParameter()[11] = (daun / 120.0f) + 0.5f;
//		//if (cheat::main::fside > 0)
//		//	cheat::main::local()->m_flPoseParameter()[11] = 0.5f;
//
//		memcpy(cheat::main::local()->animation_layers_ptr(), backuplayers, 0x38 * cheat::main::local()->get_animation_layers_count());
//
//		//m_player->m_flPoseParameter() = backup_poses;
//		cheat::main::local()->m_vecOrigin() = backup_origin;
//		cheat::main::local()->m_vecVelocity() = backup_vel;
//		cheat::main::local()->m_vecAbsVelocity() = backup_absvel;
//		cheat::main::local()->m_fFlags() = backup_flags;
//		cheat::main::local()->m_iEFlags() = backup_eflags;
//		cheat::main::local()->m_flDuckAmount() = backup_duckamt;
//		cheat::main::local()->m_flLowerBodyYawTarget() = backup_lby;
//
//		cheat::main::local()->set_abs_angles(QAngle(0.f, cheat::main::local()->get_animation_state()->abs_yaw, 0.f));
//
//		cheat::main::local()->force_bone_rebuild();
//		cheat::main::local()->SetupBonesEx();
//
//		Source::m_pGlobalVars->curtime = v63;
//		Source::m_pGlobalVars->frametime = v62;
//	}
//
//	cheat::features::lagcomp.m_local_chocked_angles.clear();
//
//	cheat::main::local()->DrawServerHitboxes();
//}

void c_lagcomp::update_animations_data(C_BasePlayer* m_player)
{
	CCSGOPlayerAnimState* state = m_player->get_animation_state();

	auto idx = m_player->entindex() - 1;

	if (!state || m_player == cheat::main::local())
		return;

	//fix_netvar_compression(m_player);

	auto resolver_info = &cheat::features::aaa.player_resolver_records[idx];

	const auto backup_m_origin		 = m_player->m_vecOrigin();
	const auto backup_m_velocity	 = m_player->m_vecVelocity();
	const auto backup_m_abs_velocity = m_player->m_vecAbsVelocity();
	const auto backup_m_flags		 = m_player->m_fFlags();
	const auto backup_m_eflags		 = m_player->m_iEFlags();
	const auto backup_m_duck		 = m_player->m_flDuckAmount();
	const auto backup_m_body		 = m_player->m_flLowerBodyYawTarget();
	const auto backup_m_absorigin	 = m_player->get_abs_origin();
	const auto backup_m_absangles	 = m_player->get_abs_eye_angles();
	const auto backup_m_absyaw		 = state->abs_yaw;

	std::memcpy(resolver_info->server_anim_layers, m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());

	//m_player->set_abs_origin(m_player->m_vecOrigin());

	if (m_player->m_flSpawnTime() != resolver_info->spawntime)
	{
		using ResetAnimState_t = void(__thiscall*)(CCSGOPlayerAnimState*);
		static auto ResetAnimState = (ResetAnimState_t)Memory::Scan(cheat::main::clientdll, "56 6A 01 68 ? ? ? ? 8B F1");

		if (ResetAnimState)
			ResetAnimState(m_player->get_animation_state());

		//m_player->update_clientside_animations();

		//state = m_player->get_animation_state();
		resolver_info->spawntime = m_player->m_flSpawnTime();
	}

	auto m_records = records[idx].m_Tickrecords;

	/*auto v75 = TIME_TO_TICKS(m_player->m_flSimulationTime() - resolver_info->last_simtime);

	if (v75 > 62)
		v75 = 61;*/

	/*const auto curtime = Source::m_pGlobalVars->curtime;
	const auto frametime = Source::m_pGlobalVars->frametime;

	if (v75 >= 20)
		Source::m_pGlobalVars->curtime = m_player->m_flSimulationTime();
	else
		Source::m_pGlobalVars->curtime = Source::m_pGlobalVars->interval_per_tick + m_player->m_flOldSimulationTime();

	Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;

	resolver_info->m_flRate = resolver_info->server_anim_layers[6].m_flPlaybackRate;

	if (!m_player->IsBot() && m_records.size() > 1)
	{
		if (v75 > 2) {
			//if (m_player->m_vecVelocity().Length2D() > 0.1f && m_player->m_vecVelocity().Length2D() > m_records.front().velocity.Length2D())
			//{
			//	const auto new_move_angle = Vector(0, RAD2DEG(std::atan2(m_player->m_vecVelocity().y, m_player->m_vecVelocity().x)), 0);
			//	const auto prev_move_angle = Vector(0, RAD2DEG(std::atan2(m_records.front().velocity.y, m_records.front().velocity.x)), 0);
			//
			//	accelerate_velocity(m_player, m_player->m_vecVelocity(), new_move_angle, prev_move_angle);
			//}
			//else
				m_player->m_vecVelocity() = (m_records.front().origin - m_records.at(1).origin) * (1.0f / TICKS_TO_TIME(v75));
	
				if (resolver_info->server_anim_layers[5].m_flWeight > 0.f)
					m_player->m_fFlags() |= FL_ONGROUND;
		}
	}*/

	resolver_info->did_shot_this_tick = false;
	resolver_info->breaking_lc = false;

	if (m_player->get_weapon() &&  m_player->get_weapon()->m_flLastShotTime() <= m_player->m_flSimulationTime() && m_player->get_weapon()->m_flLastShotTime() > resolver_info->last_simtime/*&& abs(m_player->m_angEyeAngles().x) < 60.f*/)
		resolver_info->did_shot_this_tick = true;

	if (!m_records.empty() && (m_player->m_vecOrigin() - m_records.front().origin).LengthSquared() > 4096.f)
		resolver_info->breaking_lc = true;

	// EFL_DIRTY_ABSVELOCITY
	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	//m_player->m_iEFlags() &= ~0x1000;
	//m_player->m_vecAbsVelocity() = m_player->m_vecVelocity();

	//if (v75 > 2)
	//{
		//	if (m_player->m_fFlags() & FL_ONGROUND && v75 > 2)
		//m_player->m_vecVelocity() = (m_records.front().origin - m_records.at(1).origin) * (1.0f / TICKS_TO_TIME(v75));
		//
		//	auto newduck = ((m_player->m_flDuckAmount() - record->duck_amt)
		//		* (1.0f
		//			- (Source::m_pGlobalVars->interval_per_tick
		//				/ ((float)v75 * Source::m_pGlobalVars->interval_per_tick))))
		//		+ record->duck_amt;
		//
		//	m_player->get_animation_state()->duck_amt = newduck;
		//	m_player->m_flDuckAmount() = newduck;
	//}

	//if (m_player->get_animation_state()->on_ground)
	//	m_player->get_animation_state()->time_since_inair = 0.f;

	//if (m_player->get_animation_state()->duck_amt > 0.0f && !(m_player->m_fFlags() & FL_ONGROUND))
	//	m_player->m_fFlags() |= FL_DUCKING;

	//m_player->get_animation_state()->feet_rate = 0.f;

	//resolver_info->force_velocity = true;
	//resolver_info->new_velocity = m_player->m_vecVelocity();
	//cheat::main::updating_resolver_anims = true;

	resolver_info->resolved_yaw = backup_m_absyaw;

	cheat::features::aaa.resolve(m_player, cheat::main::shots_fired[idx]);

	cheat::main::updating_resolver_anims = false;
	resolver_info->force_velocity = false;

	m_player->get_animation_state()->abs_yaw = backup_m_absyaw;// (resolver_info->resolving_method == 0 ? m_player->m_angEyeAngles().y : (resolver_info->resolving_method > 0 ? resolver_info->left_side : resolver_info->right_side));//(m_player->m_angEyeAngles().y + (60.f * resolver_info->resolving_method));

	m_player->update_clientside_animations();

	std::memcpy(resolver_info->client_anim_layers, m_player->animation_layers_ptr(), 0x38 * m_player->get_animation_layers_count());
	//resolver_info->m_flClientRate = resolver_info->client_anim_layers[6].m_flPlaybackRate;

	m_player->m_vecOrigin() = backup_m_origin;
	m_player->m_vecVelocity() = backup_m_velocity;
	m_player->m_vecAbsVelocity() = backup_m_abs_velocity;
	m_player->m_fFlags() = backup_m_flags;
	m_player->m_iEFlags() = backup_m_eflags;
	m_player->m_flDuckAmount() = backup_m_duck;
	m_player->m_flLowerBodyYawTarget() = backup_m_body;
	m_player->set_abs_origin(backup_m_absorigin);

	std::memcpy(m_player->animation_layers_ptr(), resolver_info->server_anim_layers, 0x38 * m_player->get_animation_layers_count());

	//auto unknownLayer = &m_player->get_animation_layer(12);
	//
	//if (unknownLayer && m_player->get_animation_layers_count() > 12 && m_player->get_animation_state()->speed_2d < 0.1f)
	//{
	//	unknownLayer->m_flPlaybackRate = 0;
	//	unknownLayer->m_flCycle = 0;
	//	unknownLayer->m_flWeight = 0;
	//}

	std::memcpy(resolver_info->preserver_anim_layers, resolver_info->server_anim_layers, 0x38 * m_player->get_animation_layers_count());
	resolver_info->preserver_animlayers_saved = true;
	//m_player->set_abs_angles(QAngle(0, backup_m_absyaw,0));

	m_player->force_bone_rebuild();
	m_player->SetupBonesEx();

	//if (resolver_info->did_shot_this_tick) {
	//	cheat::features::aimbot.visualise_hitboxes(m_player, m_player->m_CachedBoneData().Base(), Color::White(), Source::m_pGlobalVars->interval_per_tick * 2.0f);
	//	m_player->DrawServerHitboxes();
	//
	//	if (resolver_info->did_shot_this_tick)
	//		cheat::features::aimbot.visualise_hitboxes(m_player, m_player->m_CachedBoneData().Base(), Color::Red(), 4.0f);
	//}
}

void c_lagcomp::finish_position_adjustment(C_BasePlayer* entity)
{
	if (!cheat::Cvars.RageBot_AdjustPositions.GetValue())
		return;

	auto player_index = entity->entindex() - 1;
	auto player_record = &records[player_index];

	if (!player_record->restore_record.data_filled)
		return;

	apply_record_data(entity, &player_record->restore_record);

	//player_record->restore_record.data_filled = false; // Set to false so that we dont apply this record again if its not set next time
}

void c_lagcomp::store_records()
{
	get_interpolation();

	if (cheat::main::local() == nullptr)
	{
		reset();
		return;
	}

	for (auto index = 1; index < 64; index++)
	{
		auto entity = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(index);

		if (!entity || !entity->IsPlayer() || entity->m_iTeamNum() == cheat::main::local()->m_iTeamNum())
			continue;

		//auto entidx = entity->entindex();

		if (entity->IsDormant() ||
			entity->m_iHealth() <= 0 ||
			!entity->IsPlayer() ||
			entity->IsDead())
		{
			//cheat::main::shots_fired[index - 1] = 0;
			//cheat::main::shots_total[index - 1] = 0;

			auto resolve_info = &cheat::features::aaa.player_resolver_records[index - 1];

			if (fabs(entity->m_flSimulationTime() - resolve_info->last_simtime) > 2.f) {
				cheat::main::shots_fired[index - 1] = 0;
				cheat::main::shots_total[index - 1] = 0;
			}

			cheat::features::lagcomp.records[index - 1].reset(true);

			resolve_info->leftrec.reset();
			resolve_info->rightrec.reset();
			resolve_info->left_side = FLT_MAX;
			resolve_info->right_side = FLT_MAX;
			resolve_info->no_side = FLT_MAX;
			resolve_info->preserver_animlayers_saved = false;


			//resolver_info->resolving_method = 0;
			//cheat::main::setuping_bones = true;
			entity->client_side_animation() = true;

			continue;
		}

		cheat::main::updating_skins = false;

		if (!cheat::main::local()->IsDead() && !cheat::main::updating_skins) {
			update_player_record_data(entity);
		}
		else {
			cheat::main::updating_anims = true;
			entity->client_side_animation() = true;
			cheat::main::updating_resolver_anims = false;
			//entity->update_clientside_animations();
			//entity->client_side_animation() = false;
			//if (entity->client_side_animation()) {
			//	entity->client_side_animation() = true;
			//	entity->update_clientside_animations();
			//}
		}

	}
}

void c_lagcomp::reset()
{
	for (auto i = 0; i < 64; i++) {
		records[i].reset(true);
		//cheat::features::aaa.move_logs[i].clear();
	}
}