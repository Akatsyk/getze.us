#pragma once
#include "sdk.hpp"

class c_antiaimbot
{
public:
	/*virtual*/ void freestand(C_BasePlayer * ent, float & new_angle);
	//virtual void skeet_lby(CUserCmd * cmd);
	//virtual bool freestand(CUserCmd* cmd, float &lby);
	//virtual void new_freestand(CUserCmd * cmd, float& desync_angle);
	//virtual void /*new_*/freestand(C_BasePlayer * ent, float & new_angle);
	//virtual bool freestand(CUserCmd * cmd, float & new_ang);
	/*virtual*/ void DoLBY(CUserCmd * cmd, bool * send_packet);
	/*virtual*/ void change_angles(CUserCmd* cmd, bool &send_packet);
	/*virtual*/ void do_exloits(CUserCmd * cmd, bool& send_packet);
	/*virtual*/ void simulate_lags(CUserCmd* cmd, bool *send_packet);
	/*virtual*/ void work(CUserCmd * cmd, bool * send_packet);
	/*virtual*/ float some_func(float target, float value, float speed);

	bool skip_lags_this_tick;
	bool unlag_next_tick;
	Vector last_sent_origin;
	matrix3x4_t last_sent_matrix[128];
	Vector last_pre_origin;
	float last_sent_simtime;
	QAngle last_real_angle;
	QAngle last_sent_angle;
	float enable_delay;
	bool is_fakelagging;

	bool no_aas = false;

	bool drop = false;

	QAngle visual_real_angle;
	QAngle visual_fake_angle;
	QAngle visual_exploit_angle;

	int shift_ticks = 0;
	int piska = 0;
	bool do_tickbase = 0;
	bool did_tickbase = 0;

	bool flip_side = false;
	int previous_side = 0;
	bool extend = false;

	float m_next_lby_update_time = 0.f, m_last_lby_update = 0.f, m_last_attempted_lby = 0.f;
	bool m_will_lby_update = false;
	int packets_choked = 0;
	bool unchoke = false;

	float min_delta = 0.f, max_delta = 0.f,
		stop_to_full_running_fraction = 0.f,
		feet_speed_stand = 0.f, feet_speed_ducked = 0.f;
	bool is_standing = false;
};