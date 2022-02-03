#pragma once

#include "sdk.hpp"

namespace Engine
{

	struct player_data
	{
		void reset()
		{
			m_aimPunchAngle.clear();
			m_aimPunchAngleVel.clear();
			m_viewPunchAngle.clear();

			m_vecViewOffset.clear();
			m_vecBaseVelocity.clear();
			m_vecVelocity.clear();
			m_vecOrigin.clear();

			m_flFallVelocity = 0.0f;
			m_flVelocityModifier = 0.0f;
			m_flDuckAmount = 0.0f;
			m_flDuckSpeed = 0.0f;
			m_fAccuracyPenalty = 0.0f;

			m_hGroundEntity = 0;
			m_nMoveType = 0;
			m_nFlags = 0;
			m_nTickBase = 0;
			m_flRecoilIndex = 0;
		}

		QAngle m_aimPunchAngle = {};
		QAngle m_aimPunchAngleVel = {};
		QAngle m_viewPunchAngle = {};

		Vector m_vecViewOffset = {};
		Vector m_vecBaseVelocity = {};
		Vector m_vecVelocity = {};
		Vector m_vecOrigin = {};

		float m_flFallVelocity = 0.0f;
		float m_flVelocityModifier = 0.0f;
		float m_flDuckAmount = 0.0f;
		float m_flDuckSpeed = 0.0f;
		float m_fAccuracyPenalty = 0.0f;

		int m_hGroundEntity = 0;
		int m_nMoveType = 0;
		int m_nFlags = 0;
		int m_nTickBase = 0;
		int m_flRecoilIndex = 0;

		bool is_filled = false;
	};

	class Prediction : public Core::Singleton<Prediction>
	{
	public:
		void begin(CUserCmd* cmd);
		void end();

		int get_flags();
		CUserCmd get_prev_cmd();
		Vector get_velocity();

		void fix_netvar_compression();
		void on_run_command(C_BasePlayer* player);

		CMoveData move_data;

	private:
		CUserCmd* m_pPrevCmd = nullptr;
		C_WeaponCSBaseGun* m_pWeapon = nullptr;

		int m_fFlags = 0;
		Vector m_vecVelocity = Vector::Zero;

		float m_flCurrentTime = 0.0f;
		float m_flFrameTime = 0.0f;
		int m_nTickBase = 0;

		int m_nServerCommandsAcknowledged;
		bool m_bInPrediction;

		player_data m_Data[150] = {};
	};

}