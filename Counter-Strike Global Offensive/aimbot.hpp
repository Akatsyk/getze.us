#pragma once
#include "sdk.hpp"

class c_player_records;

class C_Hitbox
{
public:
	int hitboxID;
	bool isOBB;
	Vector mins;
	Vector maxs;
	float radius;
	int bone;
};

class c_aimbot
{
public:
	virtual Vector get_hitbox(C_BasePlayer * ent, int ihitbox);
	virtual void get_hitbox_data(C_Hitbox * rtn, C_BasePlayer * ent, int ihitbox, matrix3x4_t * matrix);
	virtual bool safe_point(C_BasePlayer * entity, Vector eye_pos, Vector aim_point, int hitboxIdx, bool maxdamage);
	virtual bool safe_static_point(C_BasePlayer * entity, Vector eye_pos, Vector aim_point, int hitboxIdx, bool maxdamage);
	virtual void draw_capsule(C_BasePlayer * ent, int ihitbox);
	virtual Vector get_hitbox(C_BasePlayer * ent, int ihitbox, matrix3x4_t mat[]);
	virtual bool delay_shot(C_BasePlayer * player, CUserCmd * cmd, bool & send_packet, c_player_records record, C_WeaponCSBaseGun * local_weapon, bool did_shot_backtrack);
	virtual float can_hit(int hitbox, C_BasePlayer * Entity, Vector position, matrix3x4_t mx[], bool check_center = false, bool predict = false);
	virtual void build_seed_table();
	virtual bool hit_chance(QAngle angle, C_BasePlayer * ent, float chance, int hitbox, float damage, int &hc);
	virtual bool prefer_baim(C_BasePlayer * player, bool & send_packet, C_WeaponCSBaseGun * local_weapon, bool is_lethal_shot);
	virtual void visualise_hitboxes(C_BasePlayer * entity, matrix3x4_t * mx, Color color, float time);
	virtual void autostop(CUserCmd * cmd, bool & send_packet, C_WeaponCSBaseGun * local_weapon/*, C_BasePlayer * best_player, float dmg, bool hitchanced*/);
	virtual float check_wall(C_WeaponCSBaseGun * local_weapon, Vector startpoint, Vector direction, C_BasePlayer* entity);
	virtual bool work(CUserCmd* cmd, bool &send_packet);
	virtual bool knifebot_work(CUserCmd * cmd, bool & send_packet);

	virtual bool knifebot_target();

	float r8cock_time = 0.f;
	bool unk_3CCA9A51 = 0;
	bool lastAttacked = false;
	int historytick = -1;
	int m_nBestIndex = -1;
	float m_nBestDist = -1;
	Vector m_nAngle;
	C_BasePlayer* pBestEntity;
	float last_pitch;
	std::vector<std::tuple<float, float, float>> precomputed_seeds = {};
};