#include "prediction.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "source.hpp"
#include "md5_shit.h"
#include "lag_compensation.hpp"
#include "aimbot.hpp"
#include "displacement.hpp"
#include "rmenu.hpp"

namespace Engine
{

	void Prediction::begin(CUserCmd* cmd)
	{
		/*cl_showerror 2*/

		if (C_CSPlayer::GetLocalPlayer())
			m_nTickBase = C_CSPlayer::GetLocalPlayer()->m_nTickBase();

		//if (cheat::Cvars.Exploits_fakeduck.GetValue())
		//	cmd->buttons &= ~4u;

		// proper tickbase/prediction fix
		// runcommand is only called on frames, when ur framerate dips, ur cmds wont be predicted (in time), so lots of things desync and fuck up
		// think like, cmd #1 has fire, and while calculating cmd #2, in_attack will still be true, as prediction never ran on cmd #1 and NextPrimaryAttack was never incremented
		// by simply forcing prediction on our cmds in CM, we circumvent all problems caused by this flawed setup where prediction is based on framerate

		//static auto host_frameticks = (int*)(Memory::Scan("engine.dll", "03 05 ? ? ? ? 83 CF 10") + 2);
		auto delta_tick = Source::m_pClientState->m_iDeltaTick;

		cheat::main::unpred_tickcount = Source::m_pGlobalVars->tickcount;

		Source::m_pPrediction->Update(delta_tick, true, Source::m_pClientState->m_iLastCommandAck,
			Source::m_pClientState->m_iLastOutgoingCommand + Source::m_pClientState->m_iChockedCommands);

		if (!cmd)
			return;

		m_pWeapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

		if (!m_pWeapon)
			return;

		m_fFlags = cheat::main::local()->m_fFlags();
		m_vecVelocity = cheat::main::local()->m_vecVelocity();

		m_flCurrentTime = Source::m_pGlobalVars->curtime;
		m_flFrameTime = Source::m_pGlobalVars->frametime;

		//if (!cheat::main::fakeducking) 
		fix_netvar_compression();

		//auto tickbase = m_pPlayer->m_nTickBase();
		//auto seq_diff = 0;
		//static CUserCmd* last_cmd = nullptr;
		//
		// correct tickbase.
		//if (!last_cmd || last_cmd->hasbeenpredicted)
		//	seq_diff = cmd->command_number - m_pPlayer->m_nTickBase();
		//
		//last_cmd = cmd;
		//
		//m_pPlayer->SetCurrentCommand(cmd);
		//
		//static auto m_nImpulse = m_pPlayer->FindInDataMap(m_pPlayer->GetPredDescMap(), "m_nImpulse");
		//static auto m_nButtons = m_pPlayer->FindInDataMap(m_pPlayer->GetPredDescMap(), "m_nButtons");
		//static auto m_afButtonLast = m_pPlayer->FindInDataMap(m_pPlayer->GetPredDescMap(), "m_afButtonLast");
		//static auto m_afButtonPressed = m_pPlayer->FindInDataMap(m_pPlayer->GetPredDescMap(), "m_afButtonPressed");
		//static auto m_afButtonReleased = m_pPlayer->FindInDataMap(m_pPlayer->GetPredDescMap(), "m_afButtonReleased");
		//
		//// not in datamap :(
		//static auto m_afButtonForced = m_nButtons + 0x138; // 0x3330
		//
		//cmd->buttons |= *reinterpret_cast< uint32_t * >(uint32_t(m_pPlayer) + m_afButtonForced);
		//
		//if (cmd->impulse)
		//	*reinterpret_cast< uint32_t * >(uint32_t(m_pPlayer) + m_nImpulse) = cmd->impulse;
		//
		//// CBasePlayer::UpdateButtonState
		//int *buttons = reinterpret_cast< int * >(uint32_t(m_pPlayer) + m_nButtons);
		//const int buttonsChanged = cmd->buttons ^ *buttons;
		//
		//// Track button info so we can detect 'pressed' and 'released' buttons next frame
		//*reinterpret_cast< int * >(uint32_t(m_pPlayer) + m_afButtonLast) = *buttons;
		//*reinterpret_cast< int * >(uint32_t(m_pPlayer) + m_nButtons) = cmd->buttons;
		//*reinterpret_cast< int * >(uint32_t(m_pPlayer) + m_afButtonPressed) = buttonsChanged & cmd->buttons;  // The changed ones still down are "pressed"
		//*reinterpret_cast< int * >(uint32_t(m_pPlayer) + m_afButtonReleased) = buttonsChanged & ~cmd->buttons; // The ones not down are "released"

		//memset(&m_MoveData, 0, sizeof(CMoveData));

		m_nServerCommandsAcknowledged = *(int*)(uintptr_t(Source::m_pPrediction) + 0x20);
		m_bInPrediction = *(bool*)(uintptr_t(Source::m_pPrediction) + 8);

		*(int*)(uintptr_t(Source::m_pPrediction) + 0x20) = 0; // m_nServerCommandsAcknowledged
		*(bool*)(uintptr_t(Source::m_pPrediction) + 8) = 1; // m_bInPrediction

		C_BaseEntity::SetPredictionRandomSeed(cmd);
		C_BaseEntity::SetPredictionPlayer(cheat::main::local());
		cheat::main::local()->SetCurrentCommand(cmd);

		Source::m_pGlobalVars->curtime = static_cast<float>(cheat::main::local()->m_nTickBase()) * Source::m_pGlobalVars->interval_per_tick;
		Source::m_pGlobalVars->frametime = *(bool*)(uintptr_t(Source::m_pPrediction) + 0xA) /*m_bEnginePaused*/ ? 0.f : Source::m_pGlobalVars->interval_per_tick;

		Source::m_pGameMovement->StartTrackPredictionErrors(cheat::main::local());

		Source::m_pMoveHelper->SetHost(cheat::main::local());
		Source::m_pPrediction->SetupMove(cheat::main::local(), cmd, Source::m_pMoveHelper, &move_data);
		Source::m_pGameMovement->ProcessMovement(cheat::main::local(), &move_data);
		Source::m_pPrediction->FinishMove(cheat::main::local(), cmd, &move_data);
		Source::m_pGameMovement->FinishTrackPredictionErrors(cheat::main::local());

		Source::m_pMoveHelper->SetHost(nullptr);

		C_BaseEntity::SetPredictionRandomSeed(nullptr);
		C_BaseEntity::SetPredictionPlayer(nullptr);
		cheat::main::local()->SetCurrentCommand(nullptr);

		cheat::main::local()->set_abs_angles(QAngle(0,cheat::main::real_yaw,0));
		cheat::main::local()->force_bone_rebuild();
		//cheat::main::updating_resolver_anims = true;
		cheat::main::local()->SetupBonesEx();
		//cheat::main::updating_resolver_anims = false;
	}

	void Prediction::end()
	{
		if (!cheat::main::local() || !m_pWeapon)
			return;

		//Source::m_pGameMovement->StartTrackPredictionErrors(m_pPlayer);

		Source::m_pGlobalVars->curtime = m_flCurrentTime;
		Source::m_pGlobalVars->frametime = m_flFrameTime;

		*(int*)(uintptr_t(Source::m_pPrediction) + 0x20) = m_nServerCommandsAcknowledged; //m_bPreviousAckHadErrors
		*(bool*)(uintptr_t(Source::m_pPrediction) + 8) = m_bInPrediction; // m_bInPrediction

		//Source::m_pMoveHelper->SetHost(nullptr);
	}

	int Prediction::get_flags()
	{
		return m_fFlags;
	}

	CUserCmd Prediction::get_prev_cmd() { return *m_pPrevCmd; }

	Vector Prediction::get_velocity() { return m_vecVelocity; }

	void Prediction::fix_netvar_compression()
	{
		auto local = cheat::main::local();
		if (local)
		{
			if (!local->IsDead())
			{
				const auto &data = m_Data[local->m_nTickBase() % 150];

				if (fabsf(local->m_aimPunchAngle().x - data.m_aimPunchAngle.x) < 0.03125f &&
					fabsf(local->m_aimPunchAngle().y - data.m_aimPunchAngle.y) < 0.03125f &&
					fabsf(local->m_aimPunchAngle().z - data.m_aimPunchAngle.z) < 0.03125f)
					local->m_aimPunchAngle() = data.m_aimPunchAngle;

				if (fabsf(local->m_aimPunchAngleVel().x - data.m_aimPunchAngleVel.x) < 0.03125f &&
					fabsf(local->m_aimPunchAngleVel().y - data.m_aimPunchAngleVel.y) < 0.03125f &&
					fabsf(local->m_aimPunchAngleVel().z - data.m_aimPunchAngleVel.z) < 0.03125f)
					local->m_aimPunchAngleVel() = data.m_aimPunchAngleVel;

				if (fabsf(local->m_vecViewOffset().z - data.m_vecViewOffset.z) < 0.03125f)
					local->m_vecViewOffset().z = data.m_vecViewOffset.z;

				if (fabsf(local->m_flDuckAmount() - data.m_flDuckAmount) < 0.03125f)
					local->m_flDuckAmount() = data.m_flDuckAmount;

				local->m_flDuckSpeed() = data.m_flDuckSpeed;

				/*QAngle m_aimPunchAngle = {};
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
				int m_flRecoilIndex = 0;*/

				/*local->m_viewPunchAngle() = data->m_viewPunchAngle;
				local->m_vecBaseVelocity() = data->m_vecBaseVelocity;
				local->m_vecVelocity() = data->m_vecVelocity;
				local->m_vecOrigin() = data->m_vecOrigin;
				local->m_flFallVelocity() = data->m_flFallVelocity;
				local->m_flVelocityModifier() = data->m_flVelocityModifier;
				local->m_hGroundEntity() = data->m_hGroundEntity;
				local->m_MoveType() = data->m_nMoveType;
				local->m_fFlags() = data->m_nFlags;

				if (auto wpn = local->get_weapon(); wpn != nullptr)
				{
					wpn->m_fAccuracyPenalty() = data->m_fAccuracyPenalty;
					wpn->m_flRecoilIndex() = data->m_flRecoilIndex;
				}*/
			}
		}
	}

	void Prediction::on_run_command(C_BasePlayer* player)
	{
		//static auto prev_tickbase = 0;
		auto local = C_CSPlayer::GetLocalPlayer();

		if (!local || local != player)
			return;

		auto data = &m_Data[local->m_nTickBase() % 150];

		data->m_aimPunchAngle = local->m_aimPunchAngle();
		data->m_aimPunchAngleVel = local->m_aimPunchAngleVel();
		data->m_vecViewOffset = local->m_vecViewOffset();
		data->m_flDuckAmount = local->m_flDuckAmount();
		data->m_flDuckSpeed = local->m_flDuckSpeed();
		//prev_tickbase = local->m_nTickBase();

		data->m_viewPunchAngle = local->m_viewPunchAngle();
		data->m_vecBaseVelocity = local->m_vecBaseVelocity();
		data->m_vecVelocity = local->m_vecVelocity();
		data->m_vecOrigin = local->m_vecOrigin();

		data->m_flFallVelocity = local->m_flFallVelocity();
		data->m_flVelocityModifier = local->m_flVelocityModifier();
		data->m_hGroundEntity = local->m_hGroundEntity();
		data->m_nMoveType = local->m_MoveType();
		data->m_nFlags = local->m_fFlags();

		if (auto wpn = local->get_weapon(); wpn != nullptr)
		{
			data->m_fAccuracyPenalty = wpn->m_fAccuracyPenalty();
			data->m_flRecoilIndex = wpn->m_flRecoilIndex();
		}

		data->is_filled = true;
	}
}