#pragma once
#include "sdk.hpp"

class c_legitbot
{
public:
	virtual float get_fov(QAngle viewAngle, QAngle aimAngle);
	virtual bool is_viable(Vector start_position, Vector end_position, C_BasePlayer * m_player);
	virtual	bool find_target(float &fov);
	virtual Vector find_best_bone(C_BasePlayer * pBaseEntity);
	virtual bool find_best_aim_spot(float fov, C_BasePlayer * m_player);
	virtual QAngle do_smooth(QAngle viewangles, QAngle target, float factor, bool p);
	virtual void backtracking(CUserCmd * cmd, C_BasePlayer * m_player, int&time);
	virtual bool triggerbot(CUserCmd * cmd);
	virtual bool work(CUserCmd * cmd, bool & send_packet);

	int		_target_id;
	int		_hitbox_id;
	QAngle	_cur_angles;
	QAngle	_aim_angles;
	Vector	_hitbox_pos;
};