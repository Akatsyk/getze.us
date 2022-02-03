#include "hooked.hpp"
#include "prediction.hpp"
#include "player.hpp"
#include "angle_resolver.hpp"
#include "lag_compensation.hpp"
#include "anti_aimbot.hpp"
#include <intrin.h>
#include "weapon.hpp"
#include "aimbot.hpp"
#include "rmenu.hpp"
#include <map>

namespace Hooked
{

	void __fastcall RunCommand( void* ecx, void* edx, C_BasePlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper )
	{
		using Fn = void ( __thiscall* )( void*, C_BasePlayer*, CUserCmd*, IMoveHelper* );

		float m_flVelocityModifier = FLT_MAX;
		static int old_tickbase = 0;
		//CBaseHandle prev_weapon;

		if (cheat::main::local() != nullptr && player == cheat::main::local())
		{
			/*auto weapon = cheat::main::local()->get_weapon();
			if (weapon && weapon->m_iItemDefinitionIndex() == weapon_revolver)
				weapon->m_flPostponeFireReadyTime() = cheat::features::aimbot.r8cock_time;*/

			m_flVelocityModifier = cheat::main::local()->m_flVelocityModifier();
		}

		const auto prev_weapon = player->m_hActiveWeapon();
		//const auto weapon = player->get_weapon();

		if (Source::m_pPredictionSwap)
			Source::m_pPredictionSwap->VCall<Fn>( Index::IPrediction::RunCommand )( ecx, player, ucmd, moveHelper );

		if (cheat::main::local() != nullptr && !cheat::main::local()->IsDead() && player == cheat::main::local()) {

			const auto weapon = cheat::main::local()->get_weapon();

			if (m_flVelocityModifier != FLT_MAX)
				cheat::main::local()->m_flVelocityModifier() = m_flVelocityModifier;

			if (weapon != nullptr) {
				static int old_activity = weapon->m_Activity();
				const auto tickbase = player->m_nTickBase() - 1;
				auto activity = weapon->m_Activity();

				if (weapon->m_iItemDefinitionIndex() == 64 && !ucmd->hasbeenpredicted) {

					if (old_activity != activity && weapon->m_Activity() == 208)
						old_tickbase = tickbase + 2;

					if (weapon->m_Activity() == 208 && old_tickbase == tickbase)
						weapon->m_flPostponeFireReadyTime() = TICKS_TO_TIME(tickbase) + 0.2f;
				}

				old_activity = activity;
			}
		}

		if (Source::m_pMoveHelper != moveHelper)
			Source::m_pMoveHelper = moveHelper;

		auto& prediction = Engine::Prediction::Instance();
		prediction->on_run_command(player);
		//cheat::game::log("runcmd");
	}

	void __fastcall SetupMove(void* ecx, void* edx, C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper, void* pMoveData)
	{
		using Fn = void(__thiscall*)(void*, C_BasePlayer*, CUserCmd*, IMoveHelper*, void*);

		auto& prediction = Engine::Prediction::Instance();

		if (&prediction.move_data != pMoveData && pMoveData != nullptr)
			memcpy(&prediction.move_data, pMoveData, 0x564);

		if (Source::m_pPredictionSwap)
			Source::m_pPredictionSwap->VCall<Fn>(Index::IPrediction::SetupMove)(ecx, player, ucmd, moveHelper, pMoveData);
	}

	bool __stdcall InPrediction()
	{
		using Fn =  bool(__thiscall*)(void*);

		//static auto CalC_BasePlayerView = (void*)Memory::Scan("client_panorama.dll", "84 C0 75 08 57 8B CE E8 ? ? ? ? 8B 06");

		//if (_ReturnAddress() == CalC_BasePlayerView && cheat::main::local() && !cheat::main::local()->IsDead())
		//{
		//	for (int i = 1; i < 65; i++)
		//	{
		//		auto ent = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(i);

		//		if (!ent || ent->IsDead())
		//			continue;

		//		// if ( ent == ctx.m_local && !csgo.m_input( )->m_fCameraInThirdPerson )
		//		// continue;

		//		//by setting last bone setup time to float NaN here, all setupbones calls (even from game) will break
		//		//only way to setupbones then is to invalidate bone cache before running setupbones
		//		//this boosts performance but also make visuals a bit more polished as u dont see any fighting happening between cheat and game

		//		if (!ent->IsDormant() && !ent->IsDead())
		//			ent->break_setup_bones();
		//		else if (ent->setup_bones_broken())
		//			ent->force_bone_rebuild();
		//	}
		//}

		static auto MaintainSequenceTransitions = (void*)Memory::Scan(cheat::main::clientdll, "84 C0 74 17 8B 87"); //C_BaseAnimating::MaintainSequenceTransitions
		static auto SetupBones_Timing = (void*)Memory::Scan(cheat::main::clientdll, "84 C0 74 0A F3 0F 10 05 ? ? ? ? EB 05"); //C_BaseAnimating::SetupBones
																															  /*
																															  if (GetPredictable() && Interfaces::Prediction->InPrediction())
																															  currentTime = Interfaces::Prediction->GetSavedTime();
																															  */

		if (cheat::Cvars.RageBot_Enable.GetValue()) {
			if (MaintainSequenceTransitions && _ReturnAddress() == MaintainSequenceTransitions)
				return true;
			if (SetupBones_Timing && cheat::main::updating_resolver_anims && _ReturnAddress() == SetupBones_Timing)
				return false;
		}

		if (Source::m_pPredictionSwap)
			return Source::m_pPredictionSwap->VCall<Fn>(Index::IPrediction::InPrediction)(Source::m_pPrediction);

		return false;
	}

}