#pragma once
#include "sdk.hpp"
#include <deque>
#include <array>
#include "lag_compensation.hpp"

class C_Tickrecord;

class resolver_records
{
public:
	resolver_records() { /*Reset();*/ }

	//~PlayerResolverRecord() { Reset(); }

	//PlayerResolverRecord(CBaseEntity* entity) { Record(entity); }

	void Reset()
	{
		/*last_balance_adjust_trigger_time = 0.f;
		just_stopped_delta = 0.f;
		last_time_down_pitch = 0.f;
		last_time_moving = 0.f;
		last_moving_lby = 0.0000001337f;*/

		is_dormant = false;
		/*is_last_moving_lby_valid = false, is_fakewalking = false, is_just_stopped = false, is_breaking_lby = false;*/
		//is_balance_adjust_triggered = false, is_balance_adjust_playing = false, last_moving_lby_can_be_invalid = false;
		//did_lby_flick = false;
		resolved = false;
		fakeangles = false;
		//resolving_method = 0;
		//last_time_lby_updated = 0;
		//lby_update_tick = 0;
		previous_angles = QAngle::Zero;
		last_real_angles = QAngle::Zero;
		latest_angles = QAngle::Zero;
		latest_angles_when_faked = QAngle::Zero;
		latest_fake = QAngle::Zero;

		m_flRate = 0.0f;
		m_flServerTorso = 0.0f;

		m_flClientRate = 0.0f;
		m_flLastFlickTime = FLT_MAX;
		m_iSide = -1;
		m_iCurrentSide = -1;
		last_time_balanced = 0.0f;
		last_cycle_desync = 0.0f;
		last_cycle = 0.f;
		b_bRateCheck = false;
		had_fake = false;
		last_time_standed = 0.f;
		last_time_choked = 0.f;
		last_time_three = 0.f;
		last_time_moved = 0.f;
		last_shot_time = 0.f;
		previous_rotation = 0.f;
		//for (auto i = 0; i < 10; i++)
		//	missed_shots[i] = 0;

		current_tick_max_delta = 0.f;
		tick_delta = 0.0f;
		lby_delta = 0.0f;
		abs_yaw_delta = 0.0f;
		latest_delta_used = 0.0f;
		last_speed = 0.0f;
		last_lby = 0.0f;

		is_jittering = false;
		is_using_static = false;
		is_using_balance = false;
		did_shot_this_tick = false;
		did_lby_update_fail = false;
		force_resolving = false;
		is_shifting = false;
		prev_pose = 0.f;
		last_abs_yaw_delta = 0.f;
		last_abs_yaw_delta_change = 0.f;
		prev_delta = FLT_MAX;
		last_abs_yaw_delta_60 = 0.0f;
		/*did_hit_low_delta = false;
		did_hit_max_delta = false;
		did_hit_no_delta = false;*/
		inverse_lby = false;
		did_hit_inversed_lby = false;
		was_shifting = false;
		memset(missed_shots, 0, sizeof(int) * 10);
		//spawntime = 0.0f;
		animations_updated = false;
		preserver_animlayers_saved = false;

		cfirstmisses = 0;

		leftrec.reset();
		rightrec.reset();
		norec.reset();
		previous_side = 0.f;
		cur_side = 0.f;
		breaking_lc = false;
		freestand_fixed = false;


		force_velocity = false;
		new_velocity.clear();
		old_velocity.clear();
		did_force_velocity = false;

		memset(leftmx, 0, sizeof(matrix3x4_t) * 128);
		memset(rightmx, 0, sizeof(matrix3x4_t) * 128);
		memset(nodesmx, 0, sizeof(matrix3x4_t) * 128);
	}

	struct AntiFreestandingRecord
	{
		int right_damage = 0, left_damage = 0;
		float right_fraction = 0.f, left_fraction = 0.f;

		void reset()
		{
			right_damage = 0;
			left_damage = 0;
			right_fraction = 0.f;
			left_fraction = 0.f;
		}
	};

	QAngle previous_angles;
	float prev_pose;
	QAngle last_real_angles;
	QAngle latest_angles;
	QAngle latest_fake;
	QAngle latest_angles_when_faked;
	float previous_rotation;
	float last_time_balanced;
	float last_time_choked;
	float last_time_three;
	float last_cycle_desync;
	float last_cycle;
	bool  had_fake;
	float last_time_standed;
	float last_time_moved;
	float last_abs_yaw_delta;
	float last_abs_yaw_delta_change;
	float last_abs_yaw_delta_60;
	float last_simtime;
	float resolved_yaw;

	float previous_side;
	float cur_side;

	bool force_velocity;
	bool did_force_velocity;
	Vector new_velocity;
	Vector old_velocity;

	bool freestand_fixed = false;

	float latest_delta_used;

	C_AnimationLayer client_anim_layers[15];

	C_AnimationLayer resolver_anim_layers[3][15];

	C_AnimationLayer server_anim_layers[15];
	C_AnimationLayer preserver_anim_layers[15];
	bool preserver_animlayers_saved = false;
	AntiFreestandingRecord freestanding_record;
	float last_shot_time = 0.0f;
	float spawntime = 0.0f;
	float m_flServerTorso = 0.0f;

	float current_tick_max_delta = 0.0f;
	float previous_jitter_lby = 0.0f;
	int previous_resolving = 0;
	float tick_delta = 0.0f;
	float lby_delta = 0.0f;
	float abs_yaw_delta = 0.0f;
	float last_velocity_angle = 0.0f;
	float last_speed = 0.0f;
	float last_lby = 0.0f;

	bool simtime_updated = false;
	bool inverse_lby = false;
	bool did_hit_inversed_lby = false;

	float left_side = FLT_MAX;
	float right_side = FLT_MAX;
	float left_lside = FLT_MAX;
	float lby_side = FLT_MAX;
	float right_lside = FLT_MAX;
	float no_side = FLT_MAX;

	//float next_predicted_lby_update;
	//float last_lby_update;
	//bool did_predicted_lby_flick;

	/*float last_time_down_pitch;
	float last_time_lby_updated;
	int lby_update_tick;*/
	bool skeet_fakes = false;
	bool is_shifting = false;
	bool was_shifting = false;
	bool did_shot_this_tick = false;
	bool breaking_lc = false;
	bool is_jittering = false;
	bool is_using_static = false;
	bool is_using_balance = false;
	bool did_lby_update_fail = false;

	bool force_resolving = false;

	float m_flRate = 0.0f;

	float m_flClientRate = 0.0f;
	float prev_delta = FLT_MAX;
	float m_flLastFlickTime = FLT_MAX;
	int m_iSide = -1;
	int m_iCurrentSide = -1;
	bool b_bRateCheck = false;

	bool animations_updated = false;

	bool invalidate_resolving = false;
	int resolving_method;
	int resolving_way;
	int cfirstmisses = 0;
	int missed_shots[12];

	float last_anims_update_time;

	matrix3x4_t leftmx[128];
	matrix3x4_t rightmx[128];
	matrix3x4_t leftlmx[128];
	matrix3x4_t rightlmx[128];
	matrix3x4_t LBYmx[128];
	matrix3x4_t nodesmx[128];

	C_Tickrecord leftrec;
	C_Tickrecord rightrec;
	C_Tickrecord leftlrec;
	C_Tickrecord LBYrec;
	C_Tickrecord rightlrec;
	C_Tickrecord norec;

	/*bool lby_update_locked;*/
	bool is_dormant, resolved;
	//bool /*is_last_moving_lby_valid, last_moving_lby_can_be_invalid, is_just_stopped, is_getting_right_delta,*/is_breaking_lby;
	/*bool is_fakewalking;
	bool is_balance_adjust_triggered, is_balance_adjust_playing;
	bool did_lby_flick;*/
	bool fakeangles;
	//bool did_hit_low_delta;
	//bool did_hit_max_delta;
	//bool did_hit_no_delta;
};

class c_resolver
{
public:
	class CLBYRECORD;
	//virtual bool freestanding_resolver(C_BasePlayer * entity, float & yaw);
	/*virtual*/ bool has_fake(C_BasePlayer * entity);
	/*virtual*/ float get_average_moving_lby(const int& entindex, const float& accuracy = 30.f);
	/*virtual*/ float get_max_desync_delta(C_BasePlayer * ent);
	/*virtual*/ float sub_59B13C30(CCSGOPlayerAnimState * nn);
	/*virtual*/ bool compare_delta(float v1, float v2, float Tolerance);
	/*virtual*/ void on_real_angle_arrive(C_BasePlayer * m_player, resolver_records * resolve_data, float real_yaw);
	/*virtual*/ void on_new_data_arrive(C_BasePlayer * m_player, resolver_records * resolve_data, CCSGOPlayerAnimState * state);
	/*virtual*/ bool resolve_freestand(C_BasePlayer * m_player, resolver_records * resolve, CCSGOPlayerAnimState * state, float &step);
	/*virtual*/	void store_deltas(C_BasePlayer * m_player, resolver_records * resolve_data);
	//virtual C_AnimationLayer get_animation_layer(C_AnimationLayer * animlayers, int number);
	/*virtual*/ void resolve(C_BasePlayer * entity, int &history);
	void resolve(C_BasePlayer * m_player);
	//virtual void resolve_moving_entities(CCSGOPlayerAnimState * state, C_BasePlayer * m_player);
	/*virtual*/ void resolve_moving_entity(C_BasePlayer * player);
	/*virtual*/ void work();

	resolver_records player_resolver_records[128];

	class CLBYRECORD {
	public:
		CLBYRECORD(C_BasePlayer* entity) { store(entity); }
		CLBYRECORD() { speed = lby = -1.f; }
		float speed;
		float lby;
		void store(C_BasePlayer* entity);
	};

	std::deque<CLBYRECORD> move_logs[128];
};