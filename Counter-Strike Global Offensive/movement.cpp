#include "movement.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "source.hpp"
#include "prediction.hpp"
#include "lag_compensation.hpp"
#include "rmenu.hpp"
#include "anti_aimbot.hpp"
#include <cmath>
#include "aimbot.hpp"

#define CheckIfNonValidNumber(x) (fpclassify(x) == FP_INFINITE || fpclassify(x) == FP_NAN || fpclassify(x) == FP_SUBNORMAL)

namespace Engine
{

	/*
	* get_move_angle
	* Returns the movement angle from the current speed
	*/
	float Movement::get_move_angle(float speed)
	{
		auto move_angle = RAD2DEG(std::atan2(15.f, speed));
		//auto move_angle = RAD2DEG(std::atan2(30.f / speed));

		if (!isfinite(move_angle) || move_angle > 90.f)
			move_angle = 90.f;
		else if (move_angle < 0.f)
			move_angle = 0.f;

		return move_angle;
	}

	void Movement::predict_velocity(Vector* velocity) {
		static auto sv_friction = Source::m_pCvar->FindVar("sv_friction");
		static auto sv_stopspeed = Source::m_pCvar->FindVar("sv_stopspeed");

		float speed = velocity->Length();
		if (speed >= 0.1f) {
			float friction = sv_friction->GetFloat();
			float stop_speed = std::max< float >(speed, sv_stopspeed->GetFloat());
			float time = std::max< float >(Source::m_pGlobalVars->interval_per_tick, Source::m_pGlobalVars->frametime);
			*velocity *= std::max< float >(0.f, speed - friction * stop_speed * time / speed);
		}
	};

	void Movement::RotateMovement(CUserCmd* cmd, float yaw)
	{
		float rotation = DEG2RAD(m_qRealAngles.y - yaw);

		float cos_rot = cos(rotation);
		float sin_rot = sin(rotation);

		float new_forwardmove = (cos_rot * cmd->forwardmove) - (sin_rot * cmd->sidemove);
		float new_sidemove = (sin_rot * cmd->forwardmove) + (cos_rot * cmd->sidemove);

		cmd->forwardmove = new_forwardmove;
		cmd->sidemove = new_sidemove;
	}

	void Movement::quick_stop(CUserCmd* cmd) {
		/*auto vel = cheat::main::local()->m_vecVelocity();
		float speed = vel.Length2D();
		if (speed > 13.f) {
			QAngle direction = QAngle::Zero;
			Math::VectorAngles({ 0.f, 0.f, 0.f }, vel, direction);
			direction.y = cmd->viewangles.y - direction.y;

			Vector new_move = Vector::Zero;
			Math::AngleVectors(direction, &new_move);
			new_move *= -450.f;

			cmd->forwardmove = new_move.x;
			cmd->sidemove = new_move.y;
		}
		else {
			cmd->forwardmove = 0.f;
			cmd->sidemove = 0.f;
		}*/

		/*cmd->sidemove = 0;
		cmd->forwardmove = 450;

		RotateMovement(cmd, Math::CalcAngle(Vector(0, 0, 0), cheat::main::local()->m_vecVelocity()).y + 180.f);*/

		Vector hvel = cheat::main::local()->m_vecVelocity();
		hvel.z = 0;
		float speed = hvel.Length2D();

		if (speed < 1.f) // Will be clipped to zero anyways
		{
			cmd->forwardmove = 0.f;
			cmd->sidemove = 0.f;
			return;
		}

		// Homework: Get these dynamically
		static float accel = Source::m_pCvar->FindVar("sv_accelerate")->GetFloat();
		static float maxSpeed = Source::m_pCvar->FindVar("sv_maxspeed")->GetFloat();
		float playerSurfaceFriction = cheat::main::local()->m_surfaceFriction(); // I'm a slimy boi
		float max_accelspeed = accel * Source::m_pGlobalVars->interval_per_tick * maxSpeed * playerSurfaceFriction;

		float wishspeed{};

		// Only do custom deceleration if it won't end at zero when applying max_accel
		// Gamemovement truncates speed < 1 to 0
		if (speed - max_accelspeed <= -1.f)
		{
			// We try to solve for speed being zero after acceleration:
			// speed - accelspeed = 0
			// speed - accel*frametime*wishspeed = 0
			// accel*frametime*wishspeed = speed
			// wishspeed = speed / (accel*frametime)
			// ^ Theoretically, that's the right equation, but it doesn't work as nice as 
			//   doing the reciprocal of that times max_accelspeed, so I'm doing that :shrug:
			wishspeed = max_accelspeed / (speed / (accel * Source::m_pGlobalVars->interval_per_tick));
		}
		else // Full deceleration, since it won't overshoot
		{
			// Or use max_accelspeed, doesn't matter
			wishspeed = max_accelspeed;
		}

		// Calculate the negative movement of our velocity, relative to our viewangles
		Vector ndir = (hvel * -1.f); Math::VectorAngles(ndir, ndir);
		ndir.y = cmd->viewangles.y - ndir.y; // Relative to local view
		Math::AngleVectors(ndir, &ndir);

		cmd->forwardmove = ndir.x * wishspeed;
		cmd->sidemove = ndir.y * wishspeed;
	};

	inline void VectorMultiply(const Vector &a, float b, Vector &c)
	{
		CHECK_VALID(a);
		Assert(IsFinite(b));
		c.x = a.x * b;
		c.y = a.y * b;
		c.z = a.z * b;
	}

	inline void VectorAdd(const Vector &a, const Vector &b, Vector &c)
	{
		CHECK_VALID(a);
		CHECK_VALID(b);
		c.x = a.x + b.x;
		c.y = a.y + b.y;
		c.z = a.z + b.z;
	}

	inline void VectorSubtract(const Vector &a, const Vector &b, Vector &c)
	{
		CHECK_VALID(a);
		CHECK_VALID(b);
		c.x = a.x - b.x;
		c.y = a.y - b.y;
		c.z = a.z - b.z;
	}

	void Movement::Begin(CUserCmd* cmd, bool& send_packet)
	{
		static auto sidespeed = Source::m_pCvar->FindVar("cl_sidespeed");

		if (!cheat::main::local() || !cmd || !cheat::main::local()->GetClientClass() || cheat::main::local()->IsDead() || cheat::main::local()->m_MoveType() == 10)
			return;

		m_pCmd = cmd;

		auto local_weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

		if (!m_pCmd || !local_weapon)
			return;

		m_pPlayer = C_CSPlayer::GetLocalPlayer();

		if (!m_pPlayer || m_pPlayer->IsDead())
			return;

		Source::m_pEngine->GetViewAngles(m_qRealAngles);

		static auto circle_yaw = 0.f;
		m_qAngles = m_pCmd->viewangles;
		m_qAnglesView = m_pCmd->viewangles;

		auto& prediction = Prediction::Instance();

		cheat::features::aimbot.autostop(cmd, send_packet, local_weapon);

		if (!(cheat::main::local()->m_fFlags() & FL_ONGROUND))
		{
			if (cheat::Cvars.Misc_AutoJump.GetValue())
				cmd->buttons &= ~IN_JUMP;

			if (cheat::Cvars.Misc_AutoStrafe.GetValue() && !cheat::main::fakeducking && !(cheat::game::pressed_keys[16] && local_weapon->m_iItemDefinitionIndex() == weapon_ssg08))
			{
				static float old_yaw = m_qAnglesView.y;

				auto get_velocity_degree = [](float length_2d)
				{
					auto tmp = RAD2DEG(atan(30.f / length_2d));

					if (CheckIfNonValidNumber(tmp) || tmp > 90.f)
						return 90.f;

					else if (tmp < 0.f)
						return 0.f;
					else
						return tmp;
				};

				if (cheat::main::local()->m_MoveType() != MOVETYPE_WALK)
					return;

				auto velocity = cheat::main::local()->m_vecVelocity();
				velocity.z = 0;

				static auto flip = false;
				auto turn_direction_modifier = (flip) ? 1.f : -1.f;
				flip = !flip;

				float forwardmove = 0.f;

				auto sidemove = cmd->sidemove;

				if (((forwardmove = cmd->forwardmove, forwardmove != 0.0) || sidemove != 0.0) && cheat::Cvars.Misc_AutoStrafeWASD.GetValue())
				{
					m_pCmd->forwardmove = 0.0;
					m_pCmd->sidemove = 0.0;
					auto v26 = atan2(sidemove * -1, forwardmove);
					m_qAnglesView.y += (v26 * 57.295776);
				}
				else if (cmd->forwardmove > 0.f)
					cmd->forwardmove = 0.f;

				auto velocity_length_2d = velocity.Length2D();

				auto strafe_angle = RAD2DEG(atan(30.f / velocity_length_2d));

				if (strafe_angle > 45.f)
					strafe_angle = 45.f;
				else if (strafe_angle < 0.f)
					strafe_angle = 0.f;

				Vector Buffer(0, m_qAnglesView.y - old_yaw, 0);
				Buffer.y = Math::normalize_angle(Buffer.y);

				int yaw_delta = Buffer.y;
				old_yaw = m_qAnglesView.y;

				auto abs_yaw_delta = abs(yaw_delta);

				if (abs_yaw_delta <= strafe_angle || abs_yaw_delta >= 30.f)
				{
					Vector velocity_angles;
					Math::VectorAngles(velocity, velocity_angles);

					Buffer = Vector(0, m_qAnglesView.y - velocity_angles.y, 0);
					Buffer.y = Math::normalize_angle(Buffer.y);
					int velocityangle_yawdelta = Buffer.y;

					auto velocity_degree = get_velocity_degree(velocity_length_2d) * 2.f; // retrack value, for teleporters

					if (velocityangle_yawdelta <= velocity_degree || velocity_length_2d <= 15.f)
					{
						if (-(velocity_degree) <= velocityangle_yawdelta || velocity_length_2d <= 15.f)
						{
							m_qAnglesView.y += (strafe_angle * turn_direction_modifier);
							cmd->sidemove = sidespeed->GetFloat() * turn_direction_modifier;
						}
						else
						{
							m_qAnglesView.y = velocity_angles.y - velocity_degree;
							cmd->sidemove = sidespeed->GetFloat();
						}
					}
					else
					{
						m_qAnglesView.y = velocity_angles.y + velocity_degree;
						cmd->sidemove = -sidespeed->GetFloat();
					}
				}
				else if (yaw_delta > 0.0f) {
					cmd->sidemove = -sidespeed->GetFloat();
				}
				else if (yaw_delta < 0.0f) {
					cmd->sidemove = sidespeed->GetFloat();
				}

				circle_yaw = m_qAnglesView.y;
			}
		}
		else
		{
			cheat::main::fakewalking = false;

			auto velocity = cheat::main::local()->m_vecVelocity();

			if (cheat::game::pressed_keys[int(cheat::Cvars.anti_aim_slowwalk_key.GetValue())] && cheat::Cvars.anti_aim_slowwalk_key.GetValue()) {
				cheat::main::fakewalking = true;

				cmd->buttons |= IN_SPEED;

				if (cheat::Cvars.anti_aim_slow_walk_accurate.GetValue()) {
					float maxspeed = 260.f;

					if (cheat::main::local()->m_bIsScoped())
						maxspeed = local_weapon->GetCSWeaponData()->max_speed_alt;
					else
						maxspeed = local_weapon->GetCSWeaponData()->max_speed;

					maxspeed *= 0.33f;

					auto v22 = (velocity.x * velocity.x) + (velocity.y * velocity.y);
					auto a1 = (cmd->sidemove * cmd->sidemove) + (cmd->forwardmove * cmd->forwardmove);
					a1 = sqrt(a1);
					v22 = sqrt(v22);
					auto v23 = v22;
					auto v15 = a1;
					auto v25 = cmd->forwardmove / v15;
					auto v27 = cmd->sidemove / v15;

					if (v15 > maxspeed) 
					{
						if ((maxspeed + 1.0f) <= v23)
						{
							cmd->forwardmove = 0.0f;
							cmd->sidemove = 0.0f;
						}
						else
						{
							cmd->sidemove = maxspeed * v27;
							cmd->forwardmove = maxspeed * v25;
						}
					}
				}
				else 
				{
					auto v36 = cmd->sidemove;
					auto v34 = cmd->forwardmove;
					auto a1 = (cmd->sidemove * cmd->sidemove) + (cmd->forwardmove * cmd->forwardmove);
					a1 = sqrt(a1);
					auto v14 = a1;

					if (a1 > 110.0f)
					{
						cmd->forwardmove = (v34 / v14) * 110.0f;
						a1 = (v36 / v14) * 110.0f;
						cmd->sidemove = (v36 / v14) * 110.0f;
					}
				}
			}
		}

		if (cheat::Cvars.RageBot_scopeminwalk.GetValue() && cheat::main::local()->m_bIsScoped() && local_weapon->m_zoomLevel() > 0)
		{
			if (cheat::main::local()->m_vecVelocity().Length2D() > 80.0f)
				quick_stop(cmd);

			cmd->buttons |= IN_SPEED;
		}

		//End();
	}

	void Movement::End()
	{
		if (!m_pCmd || !m_pPlayer)
			return;

		/*auto move = Vector(m_pCmd->forwardmove, m_pCmd->sidemove, m_pCmd->upmove);
		auto speed = move.ToVector2D().Length();

		auto yaw_move = ToDegrees(std::atan2(move.y, move.x));

		if (yaw_move < 0.0f)
			yaw_move += 360.0f;

		auto yaw_distance = m_pCmd->viewangles.y - m_qAnglesView.y + yaw_move;

		if (yaw_distance < 0.0f)
			yaw_distance += 360.0f;

		auto yaw_radians = ToRadians(yaw_distance);

		m_pCmd->forwardmove = std::cos(yaw_radians) * speed;
		m_pCmd->sidemove = std::sin(yaw_radians) * speed;

		m_qAnglesLast = m_qAngles;*/

		Vector wish_forward, wish_right, wish_up, cmd_forward, cmd_right, cmd_up;

		auto viewangles = m_pCmd->viewangles;
		auto movedata = Vector(m_pCmd->forwardmove, m_pCmd->sidemove, m_pCmd->upmove);
		viewangles.Normalize();

		if (!(m_pPlayer->m_fFlags() & FL_ONGROUND) && viewangles.z != 0.f)
			movedata.y = 0.f;

		Math::AngleVectors(m_qAnglesView, &wish_forward, &wish_right, &wish_up);
		Math::AngleVectors(viewangles, &cmd_forward, &cmd_right, &cmd_up);

		auto v8 = sqrt(wish_forward.x * wish_forward.x + wish_forward.y * wish_forward.y), v10 = sqrt(wish_right.x * wish_right.x + wish_right.y * wish_right.y), v12 = sqrt(wish_up.z * wish_up.z);

		Vector wish_forward_norm(1.0f / v8 * wish_forward.x, 1.0f / v8 * wish_forward.y, 0.f),
			wish_right_norm(1.0f / v10 * wish_right.x, 1.0f / v10 * wish_right.y, 0.f),
			wish_up_norm(0.f, 0.f, 1.0f / v12 * wish_up.z);

		auto v14 = sqrt(cmd_forward.x * cmd_forward.x + cmd_forward.y * cmd_forward.y), v16 = sqrt(cmd_right.x * cmd_right.x + cmd_right.y * cmd_right.y), v18 = sqrt(cmd_up.z * cmd_up.z);

		Vector cmd_forward_norm(1.0f / v14 * cmd_forward.x, 1.0f / v14 * cmd_forward.y, 1.0f / v14 * 0.0f),
			cmd_right_norm(1.0f / v16 * cmd_right.x, 1.0f / v16 * cmd_right.y, 1.0f / v16 * 0.0f),
			cmd_up_norm(0.f, 0.f, 1.0f / v18 * cmd_up.z);

		auto v22 = wish_forward_norm.x * movedata.x, v26 = wish_forward_norm.y * movedata.x, v28 = wish_forward_norm.z * movedata.x, v24 = wish_right_norm.x * movedata.y, v23 = wish_right_norm.y * movedata.y, v25 = wish_right_norm.z * movedata.y, v30 = wish_up_norm.x * movedata.z, v27 = wish_up_norm.z * movedata.z, v29 = wish_up_norm.y * movedata.z;

		Vector correct_movement;
		correct_movement.x = cmd_forward_norm.x * v24 + cmd_forward_norm.y * v23 + cmd_forward_norm.z * v25
			+ (cmd_forward_norm.x * v22 + cmd_forward_norm.y * v26 + cmd_forward_norm.z * v28)
			+ (cmd_forward_norm.y * v30 + cmd_forward_norm.x * v29 + cmd_forward_norm.z * v27);
		correct_movement.y = cmd_right_norm.x * v24 + cmd_right_norm.y * v23 + cmd_right_norm.z * v25
			+ (cmd_right_norm.x * v22 + cmd_right_norm.y * v26 + cmd_right_norm.z * v28)
			+ (cmd_right_norm.x * v29 + cmd_right_norm.y * v30 + cmd_right_norm.z * v27);
		correct_movement.z = cmd_up_norm.x * v23 + cmd_up_norm.y * v24 + cmd_up_norm.z * v25
			+ (cmd_up_norm.x * v26 + cmd_up_norm.y * v22 + cmd_up_norm.z * v28)
			+ (cmd_up_norm.x * v30 + cmd_up_norm.y * v29 + cmd_up_norm.z * v27);

		correct_movement.x = std::clamp(correct_movement.x, -450.f, 450.f);
		correct_movement.y = std::clamp(correct_movement.y, -450.f, 450.f);
		correct_movement.z = std::clamp(correct_movement.z, -320.f, 320.f);

		m_pCmd->forwardmove = correct_movement.x;
		m_pCmd->sidemove = correct_movement.y;
		m_pCmd->upmove = correct_movement.z;
	}

}