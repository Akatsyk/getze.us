#include "hooked.hpp"
#include "displacement.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "prediction.hpp"
#include "movement.hpp"
#include "aimbot.hpp"
#include "anti_aimbot.hpp"
#include "menu.h"
#include "game_movement.h"
#include "lag_compensation.hpp"
#include <intrin.h>
#include "rmenu.hpp"
#include "legitbot.hpp"
#include "md5_shit.h"

auto prev_choked = 0;
auto prev_seq = 0;

//inline auto ResolveRelativeAddress(uint8_t* address) {
//	//�������� �� ���� ������, ����� ����� ����� ������� �������  
//	ptrdiff_t offset = (address[3] << 24) | (address[2] << 16) | (address[1] << 8) | (address[0]);
//	return (address + 5 + offset);
//}

using SendNetMsg_t = bool(__thiscall*)(INetChannel*, INetMessage&, bool, bool);

namespace Hooked
{

	//void __fastcall CreateMove(void* ecx, void* edx, int sequence_number, float input_sample_frametime, bool active)
	bool __stdcall CreateMove(float flInputSampleTime, CUserCmd* cmd)
	{
		using Fn = bool(__stdcall*)(float, CUserCmd*);

		//if (!C_CSPlayer::GetLocalPlayer())
		bool original =	Source::m_pClientModeSwap->VCall<Fn>(Index::IBaseClientDLL::CreateMove)(flInputSampleTime, cmd);

		cheat::features::antiaimbot.did_tickbase = cheat::features::antiaimbot.do_tickbase;
		cheat::features::antiaimbot.do_tickbase = false;
		cheat::main::fix = cmd->viewangles;
		
		if (!cmd || cmd->command_number == 0 || !Source::m_pEngine->IsInGame())
			return false;

		if (original)
			Source::m_pEngine->SetViewAngles(cmd->viewangles);

		auto ff = cmd->viewangles;
		cheat::game::last_cmd = cmd;

		cmd->random_seed = MD5_PseudoRandom(cmd->command_number) & 0x7fffffff;

		auto& prediction = Engine::Prediction::Instance();
		auto& movement = Engine::Movement::Instance();

		static bool was_jumping = false;
		bool send_packet = true;
		//cheat::main::jittering = false;
		cheat::features::antiaimbot.skip_lags_this_tick = false;
		cheat::features::antiaimbot.unchoke = false;

		cheat::main::fakeducking = (cheat::game::pressed_keys[(int)cheat::Cvars.Exploits_fakeduck.GetValue()] && cheat::Cvars.Exploits_fakeduck.GetValue());

		if (cheat::Cvars.Exploits_air_desync.GetValue() && !((cmd->buttons & IN_DUCK)) && cheat::main::local() != nullptr)
		{
			if (!(cmd->buttons & IN_JUMP) || (cheat::main::local()->m_fFlags() & FL_ONGROUND) || cheat::main::local()->m_vecVelocity().z >= -140.0)
				cmd->buttons &= ~IN_DUCK;
			else
				cmd->buttons |= IN_DUCK;
		}

		//if (cheat::game::pressed_keys[(int)cheat::Cvars.Misc_quickstop_key.GetValue()] && cheat::Cvars.Misc_quickstop_key.GetValue() > 0.f)
		//	cheat::main::fast_autostop = true;

		if (auto local = C_CSPlayer::GetLocalPlayer(); local != nullptr && !local->IsDead() && local->m_MoveType() != 10 && !(cmd->buttons & IN_JUMP) && cheat::main::local()->m_fFlags() & FL_ONGROUND)
		{
			auto weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(local->m_hActiveWeapon()));

			auto IsGrenade = [](int item)
			{
				if (item == weapon_flashbang
					|| item == weapon_hegrenade
					|| item == weapon_smokegrenade
					|| item == weapon_molotov
					|| item == weapon_decoy
					|| item == weapon_incgrenade
					|| item == weapon_tagrenade)
					return true;
				else
					return false;
			};

			auto pressed_move_key = (cmd->buttons & IN_MOVELEFT
				|| cmd->buttons & IN_MOVERIGHT
				|| cmd->buttons & IN_BACK
				|| cmd->buttons & IN_FORWARD);
			
			if (auto state = local->get_animation_state(); state != nullptr && !pressed_move_key && cheat::Cvars.Misc_AutoFStop.GetValue() && state->speed_2d > 5.f)
				movement->quick_stop(cmd);

		
			cmd->buttons |= IN_BULLRUSH;

			if (cheat::Cvars.Antiaim_enable.GetValue() && cheat::Cvars.anti_aim_desync.GetValue() && fabs(cmd->sidemove) < 5.f /*&& cheat::Cvars.Antiaim_attargets.GetValue()*/) {
				if (auto state = local->get_animation_state(); state != nullptr && weapon && !IsGrenade(weapon->m_iItemDefinitionIndex()) && state->on_ground && state->time_since_inair == 0.f /*&& fabs(state->speed_up) > 100.f */)
				{
					if (cmd->command_number % 4 < 2/*cheat::main::jittering*/) {
						cmd->sidemove -= (cmd->buttons & IN_DUCK || cheat::main::fakeducking ? 3.941177f : 1.01f);
						cheat::main::jittering = false;
					}
					else {
						cmd->sidemove += (cmd->buttons & IN_DUCK || cheat::main::fakeducking ? 3.941177f : 1.01f);
						cheat::main::jittering = true;
					}
				}
			}
		}

		movement->Begin(cmd, send_packet);

		prediction->begin(cmd);

		if (cheat::main::local() != nullptr && cheat::main::local()->GetClientClass() && !cheat::main::local()->IsDead() && cheat::main::local()->m_MoveType() != 10)
		{
			//cheat::game::log("working");

			if (cheat::main::fakeducking)
			{
				//int fakelag_limit = Math::clamp((int)cheat::Cvars.Misc_fakelag_value.GetValue(), 0, 14);
				//int choked_goal = 16 / 2;
				//bool should_crouch = (Source::m_pClientState->m_iChockedCommands >= choked_goal);
				//
				//if (should_crouch)
				//	m_pCmd->buttons |= IN_DUCK;
				//else 
				//	m_pCmd->buttons &= ~IN_DUCK;

				if (Source::m_pClientState->m_iChockedCommands <= 6) 
				{
					cmd->buttons &= ~IN_DUCK;
					
					if (Source::m_pClientState->m_iChockedCommands > 3)
						cheat::main::stand = true;
				}
				else
				{
					cmd->buttons |= IN_DUCK;
				}

				if (Source::m_pClientState->m_iChockedCommands < 14)
					send_packet = false;								// choke
				else
					send_packet = true;									// send packet
			}

			auto weapon = (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon()));

			if (weapon != nullptr)
			{
				cmd->tick_count += TIME_TO_TICKS(cheat::main::lerp_time);

				if (weapon->IsFireTime())
					cheat::main::known_cmd_nr = cmd->command_number;

				//cheat::game::log("weap is fine");

				cheat::features::antiaimbot.simulate_lags(cmd, &send_packet);

				//cheat::game::log("flag");

				if (cheat::features::antiaimbot.unlag_next_tick) {
					send_packet = true;
					cheat::features::antiaimbot.unlag_next_tick = false;
				}

				if (!cheat::Cvars.LegitBot_Enabled.GetValue()) {

					//cheat::game::log("starting shit");

					auto aimbot_worked = cheat::features::aimbot.work(cmd, send_packet) /*|| cheat::features::aimbot.knifebot_work(cmd, send_packet)*/;

					if (!cheat::main::fakeducking && ((Source::m_pClientState->m_iChockedCommands >= 14 && !send_packet) || Source::m_pClientState->m_iChockedCommands >= 15) /*|| cheat::features::antiaimbot.unchoke*/) {
						cheat::features::antiaimbot.unchoke = send_packet = true;
					}

					//if (!cheat::main::fakeducking && ((cheat::features::antiaimbot.packets_choked >= 14 && !send_packet) || cheat::features::antiaimbot.packets_choked >= 15) /*|| cheat::features::antiaimbot.unchoke*/) {
					//	cheat::features::antiaimbot.unchoke = send_packet = true;
					//}

					if ((aimbot_worked || (cmd->buttons & IN_ATTACK && weapon->m_iItemDefinitionIndex() != 64) || (weapon->m_iItemDefinitionIndex() == 64 && cheat::game::pressed_keys[1] && !cheat::features::menu.menu_opened) || (cmd->buttons & IN_ATTACK2 && weapon->m_iItemDefinitionIndex() == 64)) && (weapon->IsFireTime() || weapon->IsSecondaryFireTime()))
					{
						weapon->UpdateAccuracyPenalty();

						//auto sv_usercmd_custom_random_seed = Source::m_pCvar->FindVar("sv_usercmd_custom_random_seed");

						if (cheat::Cvars.RageBot_NoSpread.GetValue() && cheat::convars::sv_usercmd_custom_random_seed == 0 && !cheat::Cvars.Misc_AntiUT.GetValue())
						{
							auto weapon_inaccuracy = weapon->GetInaccuracy();
							auto weapon_spread = weapon->GetSpread();

							RandomSeed((cmd->random_seed & 255) + 1);

							auto rand1 = RandomFloat(0.0f, 1.0f);
							auto rand_pi1 = RandomFloat(0.0f, 6.2831855f/*2.0f * RadPi*/);
							auto rand2 = RandomFloat(0.0f, 1.0f);
							auto rand_pi2 = RandomFloat(0.0f, 6.2831855f/*2.0f * RadPi*/);

							int id = weapon->m_iItemDefinitionIndex();
							auto recoil_index = weapon->m_flRecoilIndex();

							if (id == 64 && cmd->buttons & IN_ATTACK2)
							{
								rand1 = 1.0f - rand1 * rand1;
								rand2 = 1.0f - rand2 * rand2;
							}
							else if (id == 28 && recoil_index < 3.0f)
							{
								for (int i = 3; i > recoil_index; i--)
								{
									rand1 *= rand1;
									rand2 *= rand2;
								}

								rand1 = 1.0f - rand1;
								rand2 = 1.0f - rand2;
							}

							auto rand_inaccuracy = rand1 * weapon_inaccuracy;
							auto rand_spread = rand2 * weapon_spread;

							Vector2D spread =
							{
								std::cos(rand_pi1) * rand_inaccuracy + std::cos(rand_pi2) * rand_spread,
								std::sin(rand_pi1) * rand_inaccuracy + std::sin(rand_pi2) * rand_spread,
							};

							spread *= -1;

							// 
							// pitch/yaw/roll
							// 
							Vector side, up;
							Vector forward = QAngle::Zero.ToVectors(&side, &up);

							Vector direction = forward + (side * spread.x) + (up * spread.y);
							direction.Normalize();

							QAngle angles_spread = direction.ToEulerAngles();

							angles_spread.x -= cmd->viewangles.x;
							angles_spread.Normalize();

							forward = angles_spread.ToVectorsTranspose(&side, &up);

							angles_spread = forward.ToEulerAngles(&up);
							angles_spread.Normalize();

							angles_spread.y += cmd->viewangles.y;
							angles_spread.Normalize();

							cmd->viewangles = angles_spread;

							// 
							// pitch/roll
							// 
							// cmd->viewangles.x += ToDegrees( std::atan( spread.Length() ) );
							// cmd->viewangles.z = -ToDegrees( std::atan2( spread.x, spread.y ) );
							//
							// 
							// yaw/roll
							// 
							// cmd->viewangles.y += ToDegrees( std::atan( spread.Length() ) );
							// cmd->viewangles.z = -( ToDegrees( std::atan2( spread.x, spread.y ) ) - 90.0f );
						}

						static auto weapon_recoil_scale = Source::m_pCvar->FindVar("weapon_recoil_scale")->GetFloat();

						if (weapon_recoil_scale > 0.f && cheat::Cvars.RageBot_NoRecoil.GetValue())
							cmd->viewangles -= cheat::main::local()->m_aimPunchAngle() * 2.f;

						cmd->viewangles.Normalize();
					}
					else
					{
						cheat::features::antiaimbot.work(cmd, &send_packet);
						
						if (!send_packet)
							cheat::features::lagcomp.m_local_chocked_angles.push_back(local_data{ cheat::main::local()->m_fFlags(),
								cmd->viewangles,
								cheat::main::local()->m_flDuckAmount(),
								cheat::main::local()->m_flSimulationTime(),
								cheat::main::local()->m_flLowerBodyYawTarget()
								});
					}
				}
				else
					cheat::features::legitbot.work(cmd, send_packet);
			}

			//if (!send_packet)
			if (send_packet) {
				cheat::features::antiaimbot.last_sent_origin = cheat::main::local()->m_vecOrigin();
				cheat::features::antiaimbot.last_sent_simtime = cheat::main::local()->m_flSimulationTime();

				cheat::features::antiaimbot.visual_real_angle = cmd->viewangles;

				/*if (cheat::main::local()->m_vecVelocity().Length() > 0.f && fabs(cmd->viewangles.x) > 45.f)
				{
					if (cheat::main::fside > 0)
						cheat::features::antiaimbot.visual_real_angle.y -= 20.f * (cheat::Cvars.anti_aim_desync_range_right.GetValue() / 100.f);
					else
						cheat::features::antiaimbot.visual_real_angle.y += (10.f * (cheat::Cvars.anti_aim_desync_range_right.GetValue() / 100.f));
				}*/
			}

			if (cheat::main::local()->get_animation_state() != nullptr
				&& cheat::main::local()->get_animation_state()->hitgr_anim
				&& !(cmd->buttons & IN_JUMP)
				&& !was_jumping)
				cheat::features::antiaimbot.visual_real_angle.x = -10.f;

			was_jumping = cmd->buttons & IN_JUMP;

			// cheat::features::lagcomp.update_local_animations_data(cmd, send_packet);

			//else
			//	cheat::features::lagcomp.m_local_chocked_angles.emplace_back(local_data{ cheat::main::local()->m_fFlags(), cmd->viewangles, cheat::main::local()->m_flDuckAmount() });
			//
			//if (send_packet) {
			//	cheat::features::antiaimbot.do_exloits(cmd, send_packet);
			//
			//	if (cheat::game::pressed_keys[67])
			//		cheat::features::antiaimbot.shift_ticks = 12;
			//}

			movement->End();

			if (cheat::Cvars.Misc_AntiUT.GetValue())
				cmd->viewangles.Clamp();

			//old_velocity = velocity;
		}

		prediction->end();

		if ((cmd->buttons & IN_SCORE) != 0 && cheat::main::local() != nullptr)
			Source::m_pClient->DispatchUserMessage(CS_UM_ServerRankRevealAll, 0, 0, nullptr);

		//	cheat::features::antiaimbot.last_sent_angle = cmd->viewangles;

		//if (!cmd->buttons & IN_ATTACK)

		//if (cheat::Cvars.Exploits_lc_break.GetValue() && cheat::main::local()->m_vecVelocity().Length2D() > 0.1f && !wasAttacking && !send_packet && cheat::features::antiaimbot.unchoke) {
		//	static void(__thiscall * clc_move_construct)(CCLCMsg_Move_t*);
		//	if (!clc_move_construct) {
		//		auto vtable = *(uint8_t***)(Memory::Scan("engine.dll", "83 C6 01 C7 45 ? ? ? ? ?") + 6);
		//		auto CreateCLCMoveMsg = vtable[13];
		//
		//		auto relative_call = CreateCLCMoveMsg + 0x20;
		//		auto offset = *(uintptr_t*)(relative_call + 0x1);
		//		clc_move_construct = (void(__thiscall*)(CCLCMsg_Move_t*))(relative_call + 5 + offset);
		//	}
		//
		//	static void(__thiscall * clc_move_destruct)(CCLCMsg_Move_t*);
		//	if (!clc_move_destruct) {
		//		auto scan = Memory::Scan("engine.dll", "FF 90 A0 00 00 00 8D 4D A8 E8 ? ? ? ? 5F 5E 5B 8B E5 5D C3");
		//
		//		auto relative_call = scan + 0x9;
		//		auto offset = *(uintptr_t*)(relative_call + 0x1);
		//		clc_move_destruct = (void(__thiscall*)(CCLCMsg_Move_t*))(relative_call + 5 + offset);
		//	}
		//
		//	auto WriteUsercmd = [](bf_write* buf, CUserCmd* incmd, CUserCmd* outcmd) {
		//		using WriteUsercmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
		//		static WriteUsercmd_t WriteUsercmdF = (WriteUsercmd_t)Memory::Scan("client_panorama.dll", ("55 8B EC 83 E4 F8 51 53 56 8B D9"));
		//
		//		__asm
		//		{
		//			mov     ecx, buf
		//			mov     edx, incmd
		//			push    outcmd
		//			call    WriteUsercmdF
		//			add     esp, 4
		//		}
		//	};
		//
		//	auto ReadUsercmd = [](bf_read* buf, CUserCmd* incmd, CUserCmd* outcmd) {
		//		static uintptr_t ReadUsercmd = (uintptr_t)Memory::Scan("client_panorama.dll", ("55 8B EC 83 EC 08 53 8B 5D 08 8B C2"));
		//
		//		__asm
		//		{
		//			mov     ecx, buf
		//			mov     edx, incmd
		//			push    outcmd
		//			call		ReadUsercmd
		//			add		esp, 4
		//		}
		//	};
		//
		//	if (clc_move_construct && clc_move_destruct && Source::m_pClientState->m_iChockedCommands > 1) {
		//		auto base_cmd = *cmd;
		//
		//		int last_command_number = base_cmd.command_number + 1;
		//		int last_tick_count = base_cmd.tick_count + 100; // anything bigger than 9 + latency
		//
		//		uint8_t buffer[4000];
		//		uint8_t message[0x100] = { 0 };
		//		CCLCMsg_Move_t* CL_Move = (CCLCMsg_Move_t*)&message[0];
		//		clc_move_construct(CL_Move);
		//
		//		CL_Move->m_DataIn.m_pDebugName = 0;
		//		CL_Move->m_DataIn.m_pData = (const unsigned int*)15;
		//		CL_Move->m_DataIn.m_pBufferEnd = 0;
		//		CL_Move->m_DataOut.m_pData = (unsigned char*)&buffer[0];
		//		CL_Move->m_DataOut.m_nDataBytes = 4000;
		//		CL_Move->m_DataOut.m_nDataBits = 32000;
		//		CL_Move->m_DataOut.m_iCurBit = 0;
		//		CL_Move->m_DataOut.m_bOverflow = 0;
		//		CL_Move->m_DataOut.m_pDebugName = nullptr;
		//		*(uintptr_t*)&CL_Move->m_DataIn.m_bOverflow = 3 | 4;
		//
		//		auto v8 = Source::m_pClientState->m_iChockedCommands;
		//		auto v9 = v8 + Source::m_pClientState->m_iLastOutgoingCommand + 1;
		//		auto v10 = v8 + 1;
		//		auto v36 = v8 + Source::m_pClientState->m_iLastOutgoingCommand + 1;
		//
		//		if (v8 + 1 > 62)
		//			v10 = 62;
		//
		//		auto v4 = true;
		//		auto v11 = -1;
		//		auto v35 = v10;
		//		auto v12 = (v9 - v10 + 1);
		//		auto v13 = 1;
		//		auto v14 = v12;
		//		auto v34 = v12;
		//		auto result = v36;
		//		if (v14 > v36)
		//			goto LABEL_17;
		//		auto v15 = v14;
		//		do
		//		{
		//			v13 = v13 && (v4 = Source::m_pClient->WriteUsercmdDeltaToBuffer(0, &CL_Move->m_DataOut, v11, v14, v14 >= v15) == 0, result = v36, !v4);
		//			v11 = v14++;
		//		} while (v14 <= result);
		//
		//		if (v13) 
		//		{
		//			LABEL_17:
		//			CUserCmd from = CUserCmd(), to = base_cmd;
		//			to.command_number = last_command_number;
		//			to.tick_count = last_tick_count;
		//			for (int cmd_count = 0; cmd_count < Source::m_pClientState->m_iChockedCommands - 1; cmd_count++)
		//			{
		//				WriteUsercmd(&CL_Move->m_DataOut, &to, &from);
		//				from = to;
		//				to.command_number += 1;
		//				to.tick_count += 1;
		//
		//				last_command_number++;
		//				last_tick_count++;
		//			}
		//		}
		//
		//		CL_Move->m_nNewCommands = Source::m_pClientState->m_iChockedCommands - 1;
		//		CL_Move->m_nBackupCommands = 0;
		//
		//		int v19 = (CL_Move->m_DataOut.m_iCurBit + 7) >> 3;
		//
		//		using assign_lol = std::string&(__thiscall*)(int, const char*, size_t);
		//		static auto assign_std_autistic_string = (assign_lol)Memory::Scan("engine.dll", "55 8B EC 53 8B 5D 08 56 8B F1 85 DB 74 57 8B 4E 14 83 F9 10 72 04 8B 06 EB 02");
		//		assign_std_autistic_string(CL_Move->m_data, (const char*)CL_Move->m_DataOut.m_pData, v19);
		//
		//		//CallVFunction< SendNetMsg >(g_ClientState->m_NetChannel, index::SendNetMsg)(g_ClientState->m_NetChannel, *CL_Move, false, false);
		//		Memory::VCall< SendNetMsg_t >(Source::m_pClientState->m_ptrNetChannel, 40)(Source::m_pClientState->m_ptrNetChannel, *CL_Move, false, false);
		//		Memory::VCall<int(__thiscall*)(INetChannel*, bf_write*)>(Source::m_pClientState->m_ptrNetChannel, 46)(Source::m_pClientState->m_ptrNetChannel, 0);
		//		//g_ClientState->m_NetChannel->SendDatagram(nullptr);
		//
		//		clc_move_destruct(CL_Move);
		//	}
		//}

		if (!Source::m_pNetChannelSwap && Source::m_pClientState != nullptr && Source::m_pClientState->m_ptrNetChannel != nullptr && cheat::main::local() != nullptr && !cheat::main::local()->IsDead())
		{
			Source::m_pNetChannelSwap = std::make_shared<Memory::VmtSwap>((DWORD**)Source::m_pClientState->m_ptrNetChannel);
			Source::m_pNetChannelSwap->Hook(&Hooked::SendNetMsg, 40);
		}

		cmd->buttons &= ~(IN_MOVERIGHT | IN_MOVELEFT | IN_FORWARD | IN_BACK);

		if (cmd->forwardmove > 0.0f)
			cmd->buttons |= IN_FORWARD;
		else if (cmd->forwardmove < 0.0f)
			cmd->buttons |= IN_BACK;

		if (cmd->sidemove > 0.0f)
			cmd->buttons |= IN_MOVERIGHT;
		else if (cmd->sidemove < 0.0f)
			cmd->buttons |= IN_MOVELEFT;

		/*if (!Source::m_pClientStateSwap && Source::m_pClientState != nullptr && (CClientState*)(uint32_t(Source::m_pClientState) + 8) != nullptr && cheat::main::local() != nullptr && !cheat::main::local()->IsDead())
		{
			Source::m_pClientStateSwap = std::make_shared<Memory::VmtSwap>((CClientState*)(uint32_t(Source::m_pClientState) + 8));
			Source::m_pClientStateSwap->Hook(&Hooked::PacketStart, 5);
		}*/

		//if (send_packet)
		//	cheat::main::command_numbers.push_back(cmd->command_number);
		//else
		//{
		//	//cheat::features::lagcomp.m_local_chocked_angles.push_back(local_data{cheat::main::local()->m_fFlags(), cmd->viewangles, cheat::main::local()->m_flDuckAmount(), cheat::main::local()->m_nTickBase()});
		//	if (Source::m_pClientState != nullptr) {
		//		auto net_channel = (INetChannel*)Source::m_pClientState->m_ptrNetChannel;
		//		if (net_channel != nullptr && net_channel->choked_packets > 0 && !(net_channel->choked_packets % 4)) {
		//			const auto current_choke = net_channel->choked_packets;
		//			net_channel->choked_packets = 0;
		//			net_channel->send_datagram();
		//			--net_channel->out_sequence_nr;
		//			net_channel->choked_packets = current_choke;
		//		}
		//	}
		//}

		uintptr_t* ctx;
		__asm { mov ctx, ebp };
		*reinterpret_cast<uint8_t*>(*ctx - 0x1c) = send_packet;

		cheat::main::send_packet = send_packet;

		if (cheat::main::thirdperson)
			cheat::main::local_eye_angles = cmd->viewangles;
		else
			cheat::main::local_eye_angles = cheat::main::fix;

		if (!send_packet)
			cheat::main::real_yaw = cheat::main::local_eye_angles.y;
		return false;
	}
}