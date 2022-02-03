#include "source.hpp"
#include "hooked.hpp"

#include "prop_manager.hpp"
#include "displacement.hpp"
#include <fstream>

#include "player.hpp"
#include "menu.h"
#include "visuals.hpp"
#include "aimbot.hpp"
#include "lag_compensation.hpp"
#include "anti_aimbot.hpp"
#include "angle_resolver.hpp"
#include "autowall.hpp"
#include "detours.h"
#include <intrin.h>
#include <time.h>
#include "game_events.h"
#include <iostream>
#include "sdk.hpp"
#include "rmenu.hpp"
#include "sound_parser.hpp"
#include "misc.hpp"
#include "legitbot.hpp"

constexpr auto s_enginesnd = LIT(("IEngineSoundClient"));
constexpr auto s_sig_clientsideanims = LIT(("A1 ?? ?? ?? ?? F6 44 F0 04 01 74 0B"));
constexpr auto s_sig_effectshead = LIT(("8B 35 ? ? ? ? 85 F6 0F 84 ? ? ? ? BB FF FF ? ? 8B 0E"));
constexpr auto s_prop_smoketick = LIT(("m_nSmokeEffectTickBegin"));
constexpr auto s_prop_simtime = LIT(("m_flSimulationTime"));
constexpr auto s_wndproc_valve = LIT(("Valve001"));
constexpr auto s_hook_impact = LIT(("Impact"));

constexpr auto enginedll = LIT(("engine.dll"));
constexpr auto clientdll = LIT(("client_panorama.dll"));

typedef void(__thiscall *LockCursor_t)(void*);
typedef bool(__thiscall *isHLTV_t)(IVEngineClient*);
typedef void(__thiscall *FireEvents_t)(IVEngineClient*);
typedef void(__thiscall *UpdateClientSideAnimations_t)(C_BasePlayer*);
typedef void(__thiscall *SetupVelocity_t)(CCSGOPlayerAnimState*);
typedef void(__thiscall *dangerzoneCheck_t)(void*);
typedef const char *(__thiscall *GetForeignFallbackFontName_t)(void*);
typedef void(__thiscall *DoExtraBonesProcessing_t)(uintptr_t* ecx, CStudioHdr* hdr, Vector* pos, Quaternion* rotations, const matrix3x4_t&, uint8_t*, void*);
typedef void(__thiscall *lolz)(void*, float, bool);
typedef int(__thiscall *StandardBlendingRules_t)(C_BasePlayer*, CStudioHdr*, Vector*, Quaternion*, float_t, int32_t);
typedef bool(__thiscall *SendNetMsg_t)(INetChannel* pNetChan, INetMessage& msg, bool bForceReliable, bool bVoice);
typedef int(__stdcall* IsBoxVisible_t)(const Vector&, const Vector&);
typedef QAngle*(__thiscall* EyeAngles_t)(C_BasePlayer*);

DWORD OriginalUpdateClientSideAnimations;
DWORD OriginalSetupVelocity;
DWORD OriginalCalcAbsoluteVelocity;
DWORD OriginalStandardBlendingRules;
DWORD OriginalDoExtraBonesProcessing;
DWORD OriginalGetForeignFallbackFontName;
DWORD OriginalEyeAngles;

typedef bool(__thiscall *TempEntities_t)(void*, void* /*SVC_TempEntities*/);
TempEntities_t oTempEntities;

typedef bool(__thiscall *SetupBones_t)(void*, matrix3x4_t*, int, int, float);
SetupBones_t oSetupBones;

typedef bool(__thiscall* sv_impacts_int)(void*);

//typedef void(__thiscall* FireBullets_t)(C_TEFireBullets*, int);
//FireBullets_t oFireBullets;

//void __stdcall createmove_thread()
//{
//	auto m_pClientMode = **(void***)((*(DWORD**)Source::m_pClient)[10] + 5);
//
//	if (!m_pClientMode)
//	{
//		Win32::Error("ClientMode is nullptr (Source::%s)", __FUNCTION__);
//		Source::Destroy();
//		return;
//	}
//
//	Source::m_pClientModeSwap = std::make_shared<Memory::VmtSwap>(m_pClientMode);
//	Source::m_pClientModeSwap->Hook(&Hooked::CreateMove, Index::IBaseClientDLL::CreateMove);
//	Source::m_pClientModeSwap->Hook(&Hooked::GetViewModelFOV, Index::IBaseClientDLL::GetViewModelFOV);
//	Source::m_pClientModeSwap->Hook(&Hooked::OverrideView, Index::IBaseClientDLL::OverrideView);
//}

RecvVarProxyFn m_nSmokeEffectTickBegin;
RecvVarProxyFn m_flSimulationTime;
RecvVarProxyFn m_flAbsYaw;
RecvVarProxyFn m_bClientSideAnimation;
ClientEffectCallback oImpact;

namespace Hooked
{
	void __stdcall LockCursor()
	{
		static auto ofc = Source::m_pSurfaceSwap->VCall<LockCursor_t>(67);

		if (cheat::features::menu.menu_opened) {
			Source::m_pSurface->UnlockCursor();
			return;
		}

		ofc(Source::m_pSurface);
	}

	/*void __fastcall C_TEFireBullets__PostDataUpdate(C_TEFireBullets *thisptr, int a2)
	{
		if (Source::m_pEngine->GetNetChannelInfo() && cheat::main::local())
		{
			auto ent = Source::m_pEntList->GetClientEntity(thisptr->m_iPlayer + 1);
			if (ent)
			{

			}
		}

		oFireBullets(thisptr, a2);
	}

	__declspec (naked) void __stdcall CTEFireBullets__PostDataUpdate(int updateType)
	{
		__asm
		{
			push[esp + 4]
			push ecx
			call C_TEFireBullets__PostDataUpdate
			retn 4
		}
	}*/

	/*bool __fastcall TempEntities(void* ECX, void* EDX, void* msg)
	{
		if (oTempEntities == nullptr)
			return false;

		if (!cheat::main::local() || !Source::m_pEngine->IsInGame())
			return oTempEntities(ECX, msg);

		bool ret = oTempEntities(ECX, msg);

		if (!cheat::Cvars.RageBot_AdjustPositions.GetValue() || cheat::main::local()->IsDead())
			return ret;

		for (auto i = Source::m_pClientState->m_ptrEvents; i != nullptr && i->next != nullptr; i = i->next)
			i->fire_delay = 0.0f;

		Source::m_pEngine->FireEvents();

		return ret;
	}*/

	void __fastcall FireEvents(IVEngineClient *_this, void* EDX)
	{
		/*if (cheat::main::local() && !cheat::main::local()->IsDead()) {
			auto events = (CEventInfo*)(Source::m_pClientState + 0x4E64);

			if (events) {
				for (auto i = events; i != nullptr; i = i->next)
					i->fire_delay = 0.f;
			}
		}*/

		Source::m_pEngineSwap->VCall<FireEvents_t>(59)(_this);
	}

	bool __fastcall SetupBones(void* ECX, void* EDX, matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
	{
		//const auto client_unknown = Memory::VCall<void*(__thiscall*)(void*)>(ECX, 0)(ECX);
		//
		//auto entity = Memory::VCall<C_BasePlayer*(__thiscall*)(void*)>(client_unknown, 7)(client_unknown);
		//
		//auto networkable = entity->GetClientNetworkable();
		//
		//if (entity && networkable)
		//{
		//	if (entity && entity->GetClientClass() && entity->GetClientClass()->m_ClassID == 38)
		//	{
		//		auto hdr = entity->GetModelPtr();
		//
		//		if (!hdr)
		//			return false;
		//
		//		auto boneAccessor = entity->GetBoneAccessor();
		//
		//		if (!boneAccessor)
		//			return false;
		//
		//		auto backup_matrix = boneAccessor->m_pBones;
		//
		//		if (!backup_matrix)
		//			return false;
		//
		//		auto rend = entity->GetClientRenderable();
		//
		//		if (!rend)
		//			return false;
		//
		//		alignas(16) matrix3x4_t tmpAligned[128];
		//		alignas(16) matrix3x4_t parentTransform;
		//		alignas(16) Quaternion q[128];
		//		alignas(16) Vector pos[128];
		//		byte computed[0x1000] = { 0 }; //0x20?
		//
		//		boneAccessor->m_pBones = tmpAligned;
		//		boneAccessor->m_WritableBones = boneMask;
		//		boneAccessor->m_ReadableBones = boneMask;
		//
		//		auto origin = entity->m_vecOrigin();
		//		auto angles = entity->get_abs_eye_angles();
		//
		//		const auto backup_poses = entity->m_flPoseParameter();
		//		entity->m_flPoseParameter().at(11) = (angles.y + 60) / 180;
		//
		//		Math::AngleMatrix(angles, origin, parentTransform);
		//		entity->StandardBlendingRules(hdr, &pos[0], &q[0], currentTime, boneMask);
		//		entity->BuildTransformations(hdr, &pos[0], &q[0], parentTransform, boneMask, &computed[0]);
		//
		//		entity->m_flPoseParameter() = backup_poses;
		//		boneAccessor->m_pBones = backup_matrix;
		//
		//
		//		if (pBoneToWorldOut) {
		//			memcpy((void*)pBoneToWorldOut, tmpAligned, sizeof(matrix3x4_t) * hdr->m_pStudioHdr->numbones);
		//			*entity->dwBoneMatrix() = (matrix3x4_t*)pBoneToWorldOut;
		//		}
		//
		//		return true;
		//	}
		//}

		if (!ECX)
			return oSetupBones(ECX, pBoneToWorldOut, nMaxBones, boneMask, currentTime);

		auto v9 = (C_BasePlayer*)(uintptr_t(ECX) - 4);

		if ((DWORD)ECX == 0x4 || (*(int*)(uintptr_t(v9) + 0x64)) > 64)
			return oSetupBones(ECX, pBoneToWorldOut, nMaxBones, boneMask, currentTime);

		const auto is_local = v9 == cheat::main::local() && v9 != nullptr;

		if (cheat::main::setuping_bones || !cheat::main::local() || !is_local && v9->m_iTeamNum() == cheat::main::local()->m_iTeamNum() || cheat::main::local()->IsDead())
		{
			/*v10 = dword_3CB140F8;
			v11 = sub_3CA28E20(a2);
			v11();
			v12 = sub_3CA296A0(v10);
			v14 = v12(v13);
			v15 = v14;
			if (v14)
			{
				v16 = 0;
				if (*(v14 + 156) > 0)
				{
					v17 = 0;
					do
					{
						++v16;
						v18 = v17 + *(v15 + 160);
						v17 += 216;
						*(v18 + v15 + 164) &= 0xFFFFFFFA;
					} while (v16 < *(v15 + 156));
				}
			}*/
			return oSetupBones(ECX, pBoneToWorldOut, nMaxBones, boneMask, currentTime);
		}
		else {

			auto v25 = 128;

			if (nMaxBones <= 128)
				v25 = nMaxBones;

			if (pBoneToWorldOut) 
			{
				if (is_local)
				{
					//cheat::main::setuped_bones
					//if (cheat::main::setuped_bones)
					//{
						if (v25 > 0)
							memcpy(pBoneToWorldOut, v9->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * v25);

						return true;
					/*}
					else
						return false;*/
				}
				else
				{
					if (v25 > 0)
						memcpy(pBoneToWorldOut, v9->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * v25);

					return true;
				}
			}
			/*else if (is_local)
				return cheat::main::setuped_bones;
			else*/
				return true;
		}

		//return oSetupBones(ECX, pBoneToWorldOut, nMaxBones, boneMask, currentTime);
	}

	_declspec(noinline)void CalcAbsoluteVelocity_Detour(C_BasePlayer *ent, void*retaddr)
	{
		static const auto setup_velocity = (void*)(Memory::Scan(cheat::main::clientdll, "8B CE E8 ? ? ? ? F3 0F 10 A6") + 7);
		
		if (retaddr == setup_velocity)
		{
			if (ent != nullptr && cheat::main::local() != nullptr)
			{
				if (ent != cheat::main::local())
				{
					auto v7 = *(int*)(uintptr_t(ent) + 0x64);
					if (v7 <= 64)
					{
						auto lag_data = &cheat::features::aaa.player_resolver_records[v7 - 1];
						if (lag_data)
						{
							if (lag_data->force_velocity)
							{
								lag_data->did_force_velocity = true;
								lag_data->old_velocity = ent->m_vecAbsVelocity();
								ent->m_vecAbsVelocity() = lag_data->new_velocity;
							}
						}
					}
				}
			}
		}
	}

	void __fastcall CalcAbsoluteVelocity(C_BasePlayer* ecx, void* edx)
	{
		CalcAbsoluteVelocity_Detour(ecx, _ReturnAddress());
	}

	_declspec(noinline)void SetupVelocity_Detour(CCSGOPlayerAnimState *state)
	{
		auto ent = (C_BasePlayer*)state->ent;

		if (ent == nullptr) 
		{
			((SetupVelocity_t)OriginalSetupVelocity)(state);
		}
		else {
			if (ent != cheat::main::local())
			{
				((SetupVelocity_t)OriginalSetupVelocity)(state);

				auto v7 = *(int*)(uintptr_t(ent) + 0x64);
				if (v7 <= 64)
				{
					auto lag_data = &cheat::features::aaa.player_resolver_records[v7 - 1];
					if (lag_data)
					{
						if (lag_data->force_velocity)
						{
							lag_data->did_force_velocity = false;
							ent->m_vecAbsVelocity() = lag_data->old_velocity;
						}
					}
				}
			}
			else
			{
				((SetupVelocity_t)OriginalSetupVelocity)(state);
			}
		}
	}

	void __fastcall SetupVelocity(CCSGOPlayerAnimState *a1, void* edx)
	{
		SetupVelocity_Detour(a1);
	}

	_declspec(noinline)void DoExtraBonesProcessing_Detour(uintptr_t* ecx, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_computed, void* context)
	{
		// if (cheat::main::local() && !cheat::main::local()->IsDead())
			return;

		/*auto v7 = (C_BasePlayer*)(ecx);

		if (v7 == nullptr || v7->get_animation_state() == nullptr)
			return ((DoExtraBonesProcessing_t)OriginalDoExtraBonesProcessing)(ecx, hdr, pos, q, matrix, bone_computed, context);

		const auto state = uint32_t(v7->get_animation_state());

		const auto backup_tickcount = *reinterpret_cast<int32_t*>(state + 8);
		*reinterpret_cast<int32_t*>(state + 8) = 0;
		((DoExtraBonesProcessing_t)OriginalDoExtraBonesProcessing)(ecx, hdr, pos, q, matrix, bone_computed, context);
		*reinterpret_cast<int32_t*>(state + 8) = backup_tickcount;*/

		//auto v7 = (void*)(uintptr_t(ecx) - 4);
		//void* v8 = nullptr;
		//int v9;
		//if (ecx != 4)
		//{
		//	//v8 = *(_DWORD *)&v7[v10 - 10];
		//	v8 = *(int**)(uintptr_t(v7) + Engine::Displacement::DT_CSPlayer::m_bIsScoped - 10);
		//	if (v8)
		//	{
		//		v9 = *(DWORD *)(uintptr_t(v8) + 8);
		//		*(DWORD *)(uintptr_t(v8) + 8) = 0;
		//	}
		//}
		//((DoExtraBonesProcessing_t)OriginalDoExtraBonesProcessing)(ecx, hdr, pos, rotations, transforma, bone_list, ik_context);
		//if (v7)
		//{
		//	if (v8)
		//		*(DWORD *)(uintptr_t(v8) + 8) = v9;
		//}
	}

	void __fastcall DoExtraBonesProcessing(uintptr_t* ecx, uint32_t, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_computed, void* context)
	{
		DoExtraBonesProcessing_Detour(ecx, hdr, pos, q, matrix, bone_computed, context);
	}

	_declspec(noinline)const char * GetForeignFallbackFontName_Detour(uintptr_t* ecx)
	{
		if (strlen(Drawing::LastFontName) > 1)
			return Drawing::LastFontName;
		else
			return GetForeignFallbackFontName_t(OriginalGetForeignFallbackFontName)(ecx);
	}

	const char * __fastcall GetForeignFallbackFontName(uintptr_t* ecx, uint32_t)
	{
		return GetForeignFallbackFontName_Detour(ecx);
	}
	auto return_to_fire_bullet = (void*)Memory::Scan("client_panorama.dll", "8B 0D ? ? ? ? F3 0F 7E 00 8B 40 08 89 45 E4");
	auto return_to_set_first_person_viewangles = (void*)Memory::Scan("client_panorama.dll", "8B 5D 0C 8B 08 89 0B 8B 48 04 89 4B 04 B9");
	_declspec(noinline)QAngle* EyeAngles_Detour(C_BasePlayer* player)
	{
		if (player == cheat::main::local()
			&& _ReturnAddress() != return_to_fire_bullet
			&& _ReturnAddress() != return_to_set_first_person_viewangles
			&& Source::m_pEngine->IsInGame())
		{
			auto& angle = cheat::main::local_eye_angles;
			return &angle;
		}

		return EyeAngles_t(OriginalEyeAngles)(player);
	}

	QAngle* __fastcall EyeAngles(C_BasePlayer* player, uint32_t)
	{
		return EyeAngles_Detour(player);
	}

	_declspec(noinline)void StandardBlendingRules_Detour(C_BasePlayer* ent, CStudioHdr *hdr, Vector *pos, Quaternion *q, float curtime, int32_t bonemask)
	{
		if (ent != nullptr && *(int*)(uintptr_t(ent) + 0x64) <= 64 && ent != cheat::main::local())
		{
			auto boneMask = 0;

			/*if (cheat::Cvars.RageBot_Enable.GetValue()
				&& ent != cheat::main::local())
			{*/
				boneMask = bonemask;

			//	if (cheat::Cvars.RageBot_AdjustPositions.GetValue())
			//		boneMask = (0x100 | 0x200);
			//}
			//else
			//{
			//	boneMask = bonemask;
			//}

			((StandardBlendingRules_t)OriginalStandardBlendingRules)(ent, hdr, pos, q, curtime, boneMask);

			if (*(BYTE*)(uintptr_t(ent) + 0xF0) & 8)
				*(BYTE*)(uintptr_t(ent) + 0xF0) &= ~8;
		}
		else
		{
			((StandardBlendingRules_t)OriginalStandardBlendingRules)(ent, hdr, pos, q, curtime, bonemask);
		}
	}

	void __fastcall StandardBlendingRules(C_BasePlayer* a1, int a2, CStudioHdr *hdr, Vector *pos, Quaternion *q, float curtime, int32_t boneMask)
	{
		StandardBlendingRules_Detour(a1, hdr, pos, q, curtime, boneMask);
	}

	bool __fastcall IsHLTV(IVEngineClient *_this, void* EDX)
	{
		/*
		C_CSPlayer::AccumulateLayers
		sub_1037A9B0+6    8B 0D B4 13 15 15                 mov     ecx, dword_151513B4
		sub_1037A9B0+C    8B 01                             mov     eax, [ecx]
		sub_1037A9B0+E    8B 80 74 01 00 00                 mov     eax, [eax+174h]
		sub_1037A9B0+14   FF D0                             call    eax
		sub_1037A9B0+16   84 C0                             test    al, al
		sub_1037A9B0+18   75 0D                             jnz     short loc_1037A9D7
		sub_1037A9B0+1A   F6 87 28 0A 00 00+                test    byte ptr [edi+0A28h], 0Ah ; ent check here, whatever
		sub_1037A9B0+21   0F 85 F1 00 00 00                 jnz     loc_1037AAC8
		*/

		static auto ofc = Source::m_pEngineSwap->VCall<isHLTV_t>(93);

		static const auto accumulate_layers_call = (void*)Memory::Scan(cheat::main::clientdll, "84 C0 75 0D F6 87");
		static const auto setup_velocity = (void*)Memory::Scan(cheat::main::clientdll, "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80");

		if (_ReturnAddress() == accumulate_layers_call || cheat::main::updating_resolver_anims && _ReturnAddress() == setup_velocity/* && Source::m_pEngine->IsInGame() && cheat::main::local() && Source::m_pClientState->m_iDeltaTick > 0*/)
			return true;

		return ofc(_this);
	}

	static float spawntime = 0.f;

	

	/*_declspec(noinline)void anti_crash(void* epb)
	{
		static auto piska = *(void**)(Memory::Scan("client_panorama.dll", "75 3B 8B 0D ? ? ? ? 57") + 4);

		if (!piska || !Source::m_pEngine->IsInGame())
			return;

		((dangerzoneCheck_t)OriginalLol)(epb);
	}*/

	/*void __fastcall DangerZoneSmth(void* epb, void* edx)
	{
		anti_crash(epb);
	}*/

	//void manipulate_tickbase(INetChannel* pNetChan, INetMessage& msg, bool bshit, bool bshit2)
	//{
	//	auto WriteUsercmd = [](bf_write* buf, CUserCmd* incmd, CUserCmd* outcmd) {
	//		using WriteUsercmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	//		static WriteUsercmd_t WriteUsercmdF = (WriteUsercmd_t)Memory::Scan("client_panorama.dll", ("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));
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
	//	auto v4 = (cheat::features::antiaimbot.shift_ticks == 0);
	//
	//	if (v4 && (pNetChan->m_nChokedPackets != 15 || Source::m_pClientState->m_iChockedCommands <= 14))
	//		return;
	//
	//	bf_write buf;
	//
	//	auto pmsg = (CCLCMsg_Move_t*)(&msg);
	//
	//	uint8_t buffer[4000];
	//	buf.m_nDataBytes = 4000;
	//	buf.m_pData = (unsigned char*)&buffer[0];
	//	buf.m_nDataBits = 32000;
	//	buf.m_iCurBit = 0;
	//	*(WORD *)&buf.m_bOverflow = 0;
	//	buf.m_pDebugName = 0;
	//
	//	auto v8 = Source::m_pClientState->m_iChockedCommands;
	//	auto v9 = v8 + Source::m_pClientState->m_iLastOutgoingCommand + 1;
	//	auto v10 = v8 + 1;
	//	auto v36 = v8 + Source::m_pClientState->m_iLastOutgoingCommand + 1;
	//
	//	if (v8 + 1 > 62)
	//		v10 = 62;
	//
	//	auto v11 = -1;
	//	auto v35 = v10;
	//	auto v12 = (v9 - v10 + 1);
	//	auto v13 = 1;
	//	auto v14 = v12;
	//	auto v34 = v12;
	//	auto result = v36;
	//	if (v14 > v36)
	//		goto LABEL_17;
	//	auto v15 = v14;
	//	do
	//	{
	//		v13 = v13 && (v4 = Source::m_pClient->WriteUsercmdDeltaToBuffer(0, &buf, v11, v14, v14 >= v15) == 0, result = v36, !v4);
	//		v11 = v14++;
	//	} while (v14 <= result);
	//	v10 = v35;
	//	if (v13)
	//	{
	//	LABEL_17:
	//
	//		if (cheat::features::antiaimbot.shift_ticks > 0)
	//		{
	//			v36 = 0;
	//			do
	//			{
	//				if (v10 >= 62)
	//					break;
	//				auto v17 = (CUserCmd*)(((DWORD)Source::m_pInput + 0xF4) + 100 * (v11 % 150));
	//				CUserCmd v24 = CUserCmd();
	//				CUserCmd v22 = CUserCmd();
	//				memcpy(&v24, v17, 0x64u);
	//
	//				memcpy(&v22, &v24, 0x64u);
	//				v22.tick_count = 0x7FFFFFFF;
	//
	//				//sub_44DDD6D0(&v24);                     // CLAMP and shit
	//				//sub_44DDD6D0(&v22);
	//				WriteUsercmd(&buf, &v22, &v24);
	//
	//				v10 = v35 + 1;
	//				++v36;
	//				++v35;
	//			} while (v36 < cheat::features::antiaimbot.shift_ticks/**((AWContextXor1 ^ AWContextXor2) - 1425092251)*/);
	//			cheat::features::antiaimbot.shift_ticks = 0;
	//		}
	//	}
	//
	//
	//	auto v19 = (buf.m_iCurBit + 7) >> 3;
	//	//pmsg.m_data = v9;
	//	//pmsg.m_nNewCommands = 0;
	//	pmsg->m_nNewCommands = v10;
	//	pmsg->m_nBackupCommands = 0;
	//
	//	uintptr_t uiMsg = (uintptr_t)(&pmsg);
	//
	//	using assign_lol = std::string&(__thiscall*)(int*, unsigned char*, size_t);
	//	static auto assign_std_autistic_string = (assign_lol)Memory::Scan("engine.dll", "55 8B EC 53 8B 5D 08 56 8B F1 85 DB 74 57 8B 4E 14 83 F9 10 72 04 8B 06 EB 02");
	//	assign_std_autistic_string(*(int**)(uiMsg + 0x14), buf.m_pData, v19);
	//
	//	//}
	//	//return;
	//
	//	//auto from = -1;
	//
	//	//auto lol = (nextCmdNr - Source::m_pClientState->m_iChockedCommands + 1);
	//	//auto chk = Source::m_pClientState->m_iChockedCommands + 1;
	//
	//	//bool bOK = true;
	//
	//	//// Write real commands
	//
	//	//auto total_new = (nextCmdNr - Source::m_pClientState->m_iChockedCommands + 1);
	//
	//	//for (auto to = total_new; to <= nextCmdNr; to++)
	//	//{
	//	//	bOK = (bOK && Source::m_pClient->WriteUsercmdDeltaToBuffer(0, &pmsg.m_DataOut, from, to, from >= lol), total_new = nextCmdNr);
	//
	//	//	from = to;
	//	//}
	//
	//	//// Write fake commands
	//	//if (bOK) {
	//
	//	//	auto lastRealCmd = (CUserCmd*)(((DWORD)Source::m_pInput + 0xF4) + 100 * (from % 150));
	//	//	CUserCmd fromCmd = CUserCmd();
	//
	//	//	if (lastRealCmd) {
	//	//		fromCmd = *lastRealCmd;
	//	//	}
	//
	//	//	CUserCmd toCmd = fromCmd;
	//	//	toCmd.command_number++;
	//	//	toCmd.tick_count += 200;
	//
	//	//	for (int i = 0; i <= 6; i++) {
	//
	//	//		CUserCmd new_cmd;
	//	//		CUserCmd new_pcmd;
	//
	//	//		if (chk > 62)
	//	//			break;
	//
	//	//		memcpy(&new_cmd, &lastRealCmd, 0x64u);
	//	//		memcpy(&new_pcmd, &new_cmd, 0x64u);
	//
	//
	//	//		WriteUsercmd(&pmsg.m_DataOut, &new_cmd, &new_pcmd);
	//
	//	//		fromCmd = toCmd;
	//	//		toCmd.command_number++;
	//	//		toCmd.tick_count++;
	//
	//	//		chk++;
	//	//	}
	//	//}
	//}

	//void TickbaseManipulation(CCLCMsg_Move_t* CL_Move) {
	//	int nextCmdNr2; // eax
	//	int TotalCmds; // edi
	//	signed int from; // ebx
	//	bool bOk; // cl
	//	int v13; // esi
	//	int v15; // edi
	//	int ctx_xor2; // ecx
	//	CUserCmd *cmd_from_slot; // esi
	//	int curbit; // ST10_4
	//	unsigned __int8 *buf_data; // ST0C_4
	//	char v21; // [esp+10h] [ebp-10A0h]
	//	CUserCmd cmd2; // [esp+FB0h] [ebp-100h]
	//	CUserCmd cmd1; // [esp+1018h] [ebp-98h]
	//	bf_write *buffer; // [esp+10A4h] [ebp-Ch]
	//	int iterator; // [esp+10A8h] [ebp-8h]
	//	int nextCmdNr; // [esp+10ACh] [ebp-4h]

	//	if (cheat::features::antiaimbot.shift_ticks == 0 && (CL_Move->m_nNewCommands != 15 || Source::m_pClientState->m_iChockedCommands <= 14))
	//		return;

	//	using assign_lol = std::string&(__thiscall*)(void*, const char*, size_t);
	//	static auto assign_std_autistic_string = (assign_lol)Memory::Scan(cheat::main::enginedll,
	//		"55 8B EC 53 8B 5D 08 56 8B F1 85 DB 74 57 8B 4E 14 83 F9 10 72 04 8B 06 EB 02");

	//	const auto dumbass = CL_Move->m_nNewCommands;

	//	uint8_t data[4000];
	//	bf_write buf;
	//	buf.m_nDataBytes = 4000;
	//	buf.m_nDataBits = 32000;
	//	buf.m_pData = data;
	//	buf.m_iCurBit = false;
	//	buf.m_bOverflow = false;
	//	buf.m_bAssertOnOverflow = false;
	//	buf.m_pDebugName = false;

	//	TotalCmds = Source::m_pClientState->m_iChockedCommands + 1;
	//	nextCmdNr = Source::m_pClientState->m_iLastOutgoingCommand + TotalCmds;

	//	if (TotalCmds > 62)
	//		TotalCmds = 62;

	//	bOk = true;

	//	from = -1;
	//	{
	//		auto to = nextCmdNr - TotalCmds + 1;
	//		if (to <= nextCmdNr) {
	//			int newcmdnr = to >= (nextCmdNr - TotalCmds + 1);
	//			do {
	//				bOk = bOk && Source::m_pClient->WriteUsercmdDeltaToBuffer(0, &buf, from, to, 1/*(to >= newcmdnr)*/);
	//				from = to++;
	//			} while (to <= nextCmdNr);
	//		}
	//	}

	//	iterator = 0;

	//	auto WriteUsercmd = [](bf_write* buf, CUserCmd* incmd, CUserCmd* outcmd) {
	//		using WriteUsercmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	//		static WriteUsercmd_t WriteUsercmdF = (WriteUsercmd_t)Memory::Scan(cheat::main::clientdll, ("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));

	//		__asm
	//		{
	//			mov     ecx, buf
	//			mov     edx, incmd
	//			push    outcmd
	//			call    WriteUsercmdF
	//			add     esp, 4
	//		}
	//	};

	//	if (bOk) {

	//		cheat::features::antiaimbot.shift_ticks += dumbass;

	//		if (cheat::features::antiaimbot.shift_ticks > 0) {

	//			cmd_from_slot = (CUserCmd*)(((DWORD)Source::m_pInput + 0xF4) + 100 * (from % 150));
	//			memcpy(&cmd1, cmd_from_slot, sizeof(CUserCmd));

	//			cmd1.viewangles.Clamp();

	//			do {
	//				if (TotalCmds >= 62)
	//					break;

	//				memcpy(&cmd2, &cmd1, sizeof(CUserCmd));
	//				cmd2.viewangles.Clamp();
	//				cmd2.tick_count = INT_MAX;
	//				cmd2.command_number++;
	//				WriteUsercmd(&buf, &cmd2, &cmd1);

	//				++TotalCmds;
	//				++iterator;

	//			} while (iterator < cheat::features::antiaimbot.shift_ticks);

	//			cheat::features::antiaimbot.shift_ticks = 0;
	//		}
	//	}

	//	curbit = (buf.m_iCurBit + 7) >> 3;
	//	CL_Move->m_nNewCommands = TotalCmds;
	//	CL_Move->m_nBackupCommands = 0;
	//	assign_std_autistic_string(CL_Move->m_data, (const char*)buf.m_pData, curbit);
	//}

	bool __fastcall SendNetMsg(INetChannel* pNetChan, void* edx, INetMessage& msg, bool bForceReliable, bool bVoice)
	{
		using Fn = bool(__thiscall*)(INetChannel* pNetChan, INetMessage& msg, bool bForceReliable, bool bVoice);
		static auto ofc = Source::m_pNetChannelSwap->VCall<Fn>(40);

		if (cheat::main::local() && Source::m_pEngine->IsInGame())
		{
			//if (msg.GetGroup() == 11) {
			//	if (cheat::game::pressed_keys[67])
			//		cheat::features::antiaimbot.shift_ticks = 12;

			//	uintptr_t uiMsg = (uintptr_t)(&msg);

			//	TickbaseManipulation((CCLCMsg_Move_t*)&msg);
			//}

			if (msg.GetType() == 14) // Return and don't send messsage if its FileCRCCheck
				return true;

			if (msg.GetGroup() == 9) // Fix lag when transmitting voice and fakelagging
				bVoice = true;
		}
		//static auto pasta = (void*)((DWORD)GetModuleHandleA("engine.dll") +0xCCE19);

		return ofc(pNetChan, msg, bForceReliable, bVoice);
	}

	/*_declspec(noinline)void detour_my_csgo(void* ecx, float accumulated_extra_samples, bool bFinalTick)
	{
		const auto current_choke = Source::m_pClientState->m_ptrNetChannel->m_nChokedPackets;
		const auto outcmd = Source::m_pClientState->m_iLastOutgoingCommand;
		const auto chk = Source::m_pClientState->m_iChockedCommands;
		const auto cmdtime = Source::m_pClientState->m_flNextCmdTime;
		Source::m_pClientState->m_ptrNetChannel->m_nChokedPackets = 0;
		((lolz)OriginalLol)(ecx, accumulated_extra_samples, bFinalTick);
		if (chk > 0) {
			Source::m_pClientState->m_iLastOutgoingCommand = outcmd;
			--Source::m_pClientState->m_ptrNetChannel->m_nOutSequenceNr;
			Source::m_pClientState->m_flNextCmdTime = cmdtime;
			Source::m_pClientState->m_iChockedCommands = chk;
			Source::m_pClientState->m_ptrNetChannel->m_nChokedPackets = current_choke;
		}
	}

	void __fastcall CL_Move(void* ecx, void* edx, float accumulated_extra_samples, bool bFinalTick)
	{
		detour_my_csgo(ecx, accumulated_extra_samples, bFinalTick);
	}*/

	void m_nSmokeEffectTickBeginHook(const CRecvProxyData* pData, void* pStruct, void* pOut) 
	{
		auto begin = (BYTE*)((DWORD)pStruct + pData->m_pRecvProp->m_Offset + 5);

		if (begin &&  cheat::Cvars.Visuals_rem_smoke.GetValue())
			*begin = 1;

		m_nSmokeEffectTickBegin(pData, pStruct, pOut);
	}

	float last_time_got_impact = 0;

	void impact_callback(const CEffectData& data)
	{
		auto org = data.origin;

		if (cheat::Cvars.Visuals_wrld_impact.GetValue() && !org.IsZero() && cheat::main::fired_shot._avg_impact_time > 0.f && (cheat::main::fired_shot._avg_impact_time - Source::m_pGlobalVars->curtime) <= (Source::m_pGlobalVars->interval_per_tick + TICKS_TO_TIME(2)))
			Source::m_pDebugOverlay->AddBoxOverlay(data.origin, Vector(-2.f, -2.f, -2.f), Vector(2.f, 2.f, 2.f), Vector(0.f, 0.f, 0.f), 255, 0, 0, 127, cheat::Cvars.Visuals_wrld_impact_duration.GetValue());

		//last_time_got_impact = Source::m_pGlobalVars->realtime;

		oImpact(data);
	}

	typedef int(__thiscall* oKeyEvent)(void*,int, ButtonCode_t, const char*);
	int __fastcall key_event(void* thisptr, void*, int eventcode, ButtonCode_t KeyNum, const char* pszCurrentBinding)
	{
		static auto ofunc = Source::m_pClientModeSwap->VCall< oKeyEvent >(20);

		if (pszCurrentBinding) {
			if (strstr(pszCurrentBinding, "drop") && eventcode) {
				cheat::features::antiaimbot.drop = true;
				return 0;
			}
		}

		return ofunc(thisptr, eventcode, KeyNum, pszCurrentBinding);
	}

	void m_flSimulationTimeHook(const CRecvProxyData* pData, void* pStruct, void* pOut) 
	{
		//C_BasePlayer *pEntity = (C_BasePlayer *)pStruct;

		if (pData->m_Value.m_Int)
			m_flSimulationTime(pData, pStruct, pOut);
	}

	int	__stdcall IsBoxVisible(const Vector& mins, const Vector& maxs)
	{
		static auto ofc = Source::m_pEngineSwap->VCall<IsBoxVisible_t>(32);

		if (!memcmp(_ReturnAddress(), "\x85\xC0\x74\x2D\x83\x7D\x10\x00\x75\x1C", 10))
			return 1;

		return ofc(mins, maxs);
	}

	void m_bClientSideAnimationHook(const CRecvProxyData* pData, void* pStruct, void* pOut) 
	{
		m_bClientSideAnimation(pData, pStruct, pOut);

		auto ent = (C_BasePlayer*)pStruct;

		if (ent != nullptr && Source::m_pClientState->m_iDeltaTick < 0 && cheat::main::local() && (ent->m_iTeamNum() != cheat::main::local()->m_iTeamNum() || ent == cheat::main::local()))
			*(int*)pOut = (cheat::main::updating_anims ? 1 : 0);
	}

	//bool exploit(int amount, CCLCMsg_Move_t* msg, void* ecx, int nSlot, void * buf, int from)
	//{
	//	auto WriteUsercmd = [](void* buf, CUserCmd* incmd, CUserCmd* outcmd) {
	//		using WriteUsercmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	//		static WriteUsercmd_t WriteUsercmdF = (WriteUsercmd_t)Memory::Scan(cheat::main::clientdll, ("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));

	//		__asm
	//		{
	//			mov     ecx, buf
	//			mov     edx, incmd
	//			push    outcmd
	//			call    WriteUsercmdF
	//			add     esp, 4
	//		}
	//	};

	//	bool result = 0;

	//	CUserCmd cmd2; // [esp+FB0h] [ebp-100h]
	//	CUserCmd cmd1; // [esp+1018h] [ebp-98h]

	//	int v28 = *(int *)((DWORD)msg + 16) + amount;
	//	int v39 = *(int *)((DWORD)msg + 16);
	//	*(int *)((DWORD)msg + 12) = 0;

	//	if (v28 > 62)
	//		v28 = 62;

	//	int v29 = -1;
	//	*(int *)((DWORD)msg + 16) = v28;
	//	int v30 = Source::m_pClientState->m_iChockedCommands + Source::m_pClientState->m_iLastOutgoingCommand + 1;
	//	int v31 = v30 - v39 + 1;
	//	if (v31 > v30)
	//	{
	//		auto v34 = (CUserCmd*)(((DWORD)Source::m_pInput + 0xF4) + 100 * (from % 150));
	//		if (v34)
	//		{
	//			memcpy(&cmd1, v34, 0x64);
	//			memcpy(&cmd2, v34, 0x64);

	//			cmd2.command_number++;
	//			cmd2.tick_count += 200;

	//			if (v39 <= v28)
	//			{
	//				auto v36 = v28 - v39 + 1;
	//				do
	//				{
	//					WriteUsercmd(buf, &cmd2, &cmd1);

	//					cmd1.viewangles = cmd2.viewangles;
	//					cmd1.command_number = cmd2.command_number;
	//					cmd1.tick_count = cmd2.tick_count;

	//					++cmd2.command_number;
	//					++cmd2.tick_count;

	//					--v36;
	//				} while (v36 > 0);
	//			}
	//		}
	//		result = 1;
	//	}
	//	else
	//	{
	//		using Fn = bool(__thiscall*)(void* ecx, int nSlot, void * buf, int from, int to, bool isNewCmd);

	//		while (Source::m_pClientSwap->VCall<Fn>(24)(ecx, nSlot, buf, v29, v31, 1))
	//		{
	//			v29 = v31++;
	//			result = 0;
	//			if (v31 > v30)
	//			{
	//				auto v34 = (CUserCmd*)(((DWORD)Source::m_pInput + 0xF4) + 100 * (from % 150));
	//				if (v34)
	//				{
	//					memcpy(&cmd1, v34, 0x64);
	//					memcpy(&cmd2, v34, 0x64);

	//					cmd2.command_number++;
	//					cmd2.tick_count += 200;

	//					if (v39 <= v28)
	//					{
	//						auto v36 = v28 - v39 + 1;
	//						do
	//						{
	//							WriteUsercmd(buf, &cmd2, &cmd1);

	//							cmd1.viewangles = cmd2.viewangles;
	//							cmd1.command_number = cmd2.command_number;
	//							cmd1.tick_count = cmd2.tick_count;

	//							++cmd2.command_number;
	//							++cmd2.tick_count;

	//							--v36;
	//						} while (v36 > 0);
	//					}
	//				}
	//				result = 1;
	//			}
	//		}
	//	}
	//	return result;
	//}

	bool __fastcall WriteUsercmdDeltaToBuffer(void* ecx, void* edx, int slot, void* buffer, int from, int to, bool new_cmd)
	{
		/*
		auto v69 = cheat::features::antiaimbot.shift_ticks;

		if (!cheat::features::antiaimbot.shift_ticks)
			return ofc(ecx, nSlot, buf, from, to, isNewCmd);

		if (from != -1)
			return 1;

		uintptr_t framePtr;
		__asm mov framePtr, ebp;
		auto msg = reinterpret_cast<CCLCMsg_Move_t*>(framePtr + 0xFCC);
		int a67 = *reinterpret_cast<int*>(framePtr + 0xFDC);

		cheat::features::antiaimbot.shift_ticks = 0;
		if (v69 < 0)
			return exploit(abs(v69), msg, ecx, nSlot, buf, from);

		auto a66 = 0;
		auto v76 = a67;
		auto v71 = a67 - abs(v69);
		if (v71 < 1)
			v71 = 1;
		a67 = v71;
		auto v77 = v71;
		auto v72 = -1;
		auto v75 = Source::m_pClientState->m_iChockedCommands + 1 + Source::m_pClientState->m_iLastOutgoingCommand;
		auto v73 = v75 - v71 + 1;
		if (v73 > v75)
		{
			auto v74 = v76 + cheat::features::antiaimbot.piska - v71;
			if (v74 > 17)
			{
				cheat::features::antiaimbot.piska = 17;
				return 1;
			}

			if (v74 < 0)
				v74 = 0;

			cheat::features::antiaimbot.piska = v74;
			return 1;
		}
		while (ofc(ecx, nSlot, buf, from, to, isNewCmd))
		{
			v72 = v73++;
			if (v73 > v75)
			{
				v71 = v77;
				auto v74 = v76 + cheat::features::antiaimbot.piska - v71;
				if (v74 > 17)
				{
					cheat::features::antiaimbot.piska = 17;
					return 1;
				}

				if (v74 < 0)
					v74 = 0;

				cheat::features::antiaimbot.piska = v74;
				return 1;
			}
		}

		return 0;*/

		//auto WriteUsercmd = [](void* buf, CUserCmd* incmd, CUserCmd* outcmd) {
		//	using WriteUsercmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
		//	static WriteUsercmd_t WriteUsercmdF = (WriteUsercmd_t)Memory::Scan(cheat::main::clientdll, ("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));

		//	__asm
		//	{
		//		mov     ecx, buf
		//		mov     edx, incmd
		//		push    outcmd
		//		call    WriteUsercmdF
		//		add     esp, 4
		//	}
		//};

		//using Fn = bool(__thiscall*)(void* ecx, int nSlot, void * buf, int from, int to, bool isNewCmd);
		//static auto ofc = Source::m_pClientSwap->VCall<Fn>(24);

		//uintptr_t stackbase;
		//__asm mov stackbase, ebp;

		////we arent trying to adjust tickbase, return original
		//if (cheat::features::antiaimbot.shift_ticks <= 0)
		//	return ofc(ecx, slot, buffer, from, to, new_cmd);

		////do some fun tickbase stuff here
		//if (from != -1)
		//	return true; // Skip next calls

		////get needed objects
		//CCLCMsg_Move_t* msg = reinterpret_cast<CCLCMsg_Move_t*>(stackbase + 0xFCC);
		//INetChannel* net_channel = Source::m_pClientState->m_ptrNetChannel;

		//const int newCommands = msg->m_nNewCommands;

		//// Manipulate CLC_Move
		//const int nextCmdNr = Source::m_pClientState->m_iLastOutgoingCommand + Source::m_pClientState->m_iChockedCommands + 1;
		//const int totalNewCommands = min(cheat::features::antiaimbot.shift_ticks, MAX_USERCMDS_SEND);
		//cheat::features::antiaimbot.shift_ticks -= totalNewCommands;

		//from = -1;
		//msg->m_nNewCommands = totalNewCommands;
		//msg->m_nBackupCommands = 0;

		//// Write real commands	
		//for (to = nextCmdNr - newCommands + 1; to <= nextCmdNr; to++)
		//{
		//	if (!ofc(ecx, slot, buffer, from, to, true)) //maybe not?
		//		return false;

		//	from = to;
		//}

		//// Write fake commands
		//CUserCmd* lastRealCmd = (CUserCmd*)(((DWORD)Source::m_pInput + 0xF4) + 100 * (from % 150));
		//CUserCmd fromCmd;

		//if (lastRealCmd)
		//	fromCmd = *lastRealCmd;

		//CUserCmd toCmd = fromCmd;

		//toCmd.command_number++;
		//toCmd.tick_count += 200; // Prevent server from executing fake commands sometimes

		//for (int i = newCommands; i <= totalNewCommands; i++)
		//{
		//	WriteUsercmd(buffer, &toCmd, &fromCmd);
		//	fromCmd = toCmd;
		//	toCmd.command_number++;
		//	toCmd.tick_count++;
		//}

		//return true;

		////get stack base
		//uintptr_t stackbase;
		//__asm mov stackbase, ebp;

		//if (cheat::features::antiaimbot.shift_ticks == 0)
		//	return ofc(ecx, slot, buffer, from, to, new_cmd);

		////do some fun tickbase stuff here
		//if (from != -1)
		//	return true; // Skip next calls

		//const auto shift = cheat::features::antiaimbot.shift_ticks;

		//cheat::features::antiaimbot.shift_ticks = 0;
		// 
		////get needed objects
		//CCLCMsg_Move_t* msg = reinterpret_cast<CCLCMsg_Move_t*>(stackbase + 0xFCC);

		//if (shift != 0)
		//	return shift_tickbase(abs(shift), msg, slot, buffer);

		return false;
	}

	bool __fastcall IsConnected(void* ecx, void* edx)
	{
		/*string: "IsLoadoutAllowed" 
		- follow up v8::FunctionTemplate::New function
		- inside it go to second function that is being called after "if" statement.
		- after that u need to open first function that is inside it. [before (*(int (**)(void))(*(_DWORD *)dword_152350E4 + 516))();]
		*/
		/*
		.text:103A2110 57                          push    edi
		.text:103A2111 8B F9                       mov     edi, ecx
		.text:103A2113 8B 0D AC E5+                mov     ecx, dword_14F8E5AC
		.text:103A2119 8B 01                       mov     eax, [ecx]
		.text:103A211B 8B 40 6C                    mov     eax, [eax+6Ch]
		.text:103A211E FF D0                       call    eax             ; Indirect Call Near Procedure
		.text:103A2120 84 C0                       test    al, al          ; Logical Compare <-
		.text:103A2122 75 04                       jnz     short loc_103A2128 ; Jump if Not Zero (ZF=0) 
		.text:103A2124 B0 01                       mov     al, 1
		.text:103A2126 5F                          pop     edi
		*/

		using Fn = bool(__thiscall*)(void* ecx);
		static auto ofc = Source::m_pEngineSwap->VCall<Fn>(27);

		static void* is_loadout_allowed = (void*)(Memory::Scan("client_panorama.dll", "84 C0 75 04 B0 01 5F"));

		if (_ReturnAddress() == is_loadout_allowed && cheat::Cvars.Visuals_unlock_invertory.GetValue() && Source::m_pEngine->IsInGame())
			return false;

		return ofc(ecx);
	}
}

struct InterfaceLinkedList {
	void*(*func)();
	const char *name;
	InterfaceLinkedList *next;
};

//#define NOAUTH

namespace Source
{
	IBaseClientDLL* m_pClient = nullptr;
	ISurface* m_pSurface = nullptr;
	InputSystem* m_pInputSystem = nullptr;
	ICvar* m_pCvar = nullptr;
	IClientEntityList* m_pEntList = nullptr;
	IGameMovement* m_pGameMovement = nullptr;
	IPrediction* m_pPrediction = nullptr;
	IMoveHelper* m_pMoveHelper = nullptr;
	IInput* m_pInput = nullptr;
	CClientState* m_pClientState = nullptr;
	IVModelInfo* m_pModelInfo = nullptr;
	CGlobalVars* m_pGlobalVars = nullptr;
	IVEngineClient* m_pEngine = nullptr;
	IPanel* m_pPanel = nullptr;
	IVRenderView* m_pRenderView = nullptr;
	IEngineTrace* m_pEngineTrace = nullptr;
	IVDebugOverlay* m_pDebugOverlay = nullptr;
	IMemAlloc* m_pMemAlloc = nullptr;
	IPhysicsSurfaceProps* m_pPhysProps = nullptr;
	IMaterialSystem* m_pMaterialSystem = nullptr;
	IGameEventManager* m_pGameEvents = nullptr;
	IVModelRender* m_pModelRender = nullptr;
	IEngineVGui* m_pEngineVGUI = nullptr;
	ILocalize* m_pLocalize = nullptr;
	IEngineSound* m_pEngineSound = nullptr;

	Memory::VmtSwap::Shared m_pClientSwap = nullptr;
	Memory::VmtSwap::Shared m_pClientStateSwap = nullptr;
	Memory::VmtSwap::Shared m_pClientModeSwap = nullptr;
	Memory::VmtSwap::Shared m_pSurfaceSwap = nullptr;
	Memory::VmtSwap::Shared m_pPredictionSwap = nullptr;
	Memory::VmtSwap::Shared m_pPanelSwap = nullptr;
	Memory::VmtSwap::Shared m_pRenderViewSwap = nullptr;
	Memory::VmtSwap::Shared m_pEngineSwap = nullptr;

	Memory::VmtSwap::Shared m_pFireBulletsSwap = nullptr;
	Memory::VmtSwap::Shared m_pEngineVGUISwap = nullptr;
	Memory::VmtSwap::Shared m_pModelRenderSwap = nullptr;
	Memory::VmtSwap::Shared m_pNetChannelSwap = nullptr;
	Memory::VmtSwap::Shared m_pShowImpactsSwap = nullptr;

	Memory::VmtSwap::Shared m_pDeviceSwap = nullptr;

	HWND Window = nullptr;
	HANDLE m_pCreateMoveThread = nullptr;
	static DWORD dwCCSPlayerRenderablevftable = NULL;

	//void DynAppSysFactory(const char *module) {
	//	void *create_interface = GetProcAddress(GetModuleHandleA(module), "CreateInterface");
	//
	//	size_t jmp_instruction = (size_t)(create_interface)+4;
	//	//rva from end of instruction
	//	size_t jmp_target = jmp_instruction + *(size_t*)(jmp_instruction + 1) + 5;
	//
	//	auto interface_list = **(InterfaceLinkedList***)(jmp_target + 6);
	//
	//	auto list_ptr = interface_list->next;
	//
	//	//std::vector<std::string> ifaces;
	//
	//	std::ofstream myfile("C:\\MorphEngine\\ifaces.log", std::ios::app);
	//	if (myfile.is_open())
	//	{
	//		while (list_ptr) {
	//
	//			myfile << module << " : " << list_ptr->name << std::endl;
	//
	//			list_ptr = list_ptr->next;
	//		}
	//
	//		myfile.close();
	//	}
	//
	//	/*while (list_ptr) {
	//		
	//		ifaces.emplace_back(list_ptr->name);
	//
	//		list_ptr = list_ptr->next;
	//	}*/
	//
	//	printf("%s iface list ptr: 0x%X\n", module, interface_list);
	//}

	bool Create()
	{
		auto& pPropManager = Engine::PropManager::Instance();

		cheat::main::enginedll = enginedll;
		cheat::main::clientdll = clientdll;

		m_pClient = (IBaseClientDLL*)CreateInterface("client_panorama.dll", "VClient");

		if (!m_pClient)
		{
#ifdef DEBUG
			Win32::Error("IBaseClientDLL is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
	}

		if (!pPropManager->Create(m_pClient))
		{
#ifdef DEBUG
			Win32::Error("Engine::PropManager::Create failed (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		if (!Engine::Displacement::Create())
		{
#ifdef DEBUG
			Win32::Error("Engine::Displacement::Create failed (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		Engine::Displacement::Data::m_pGlowManager = *reinterpret_cast<CGlowObjectManager**>(Memory::Scan("client_panorama.dll", "0F 11 05 ? ? ? ? 83 C8 01") + 3);
		Engine::Displacement::Data::m_uTraceLineIgnoreTwoEntities = *(DWORD**)(Memory::Scan("client_panorama.dll", "53 8B DC 83 EC ? 83 E4 ? 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC 8C"));
		//Data::m_uClipTracePlayers = (DWORD)(Memory::Scan("client_panorama.dll", "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? 8B 43 10"));
		Engine::Displacement::Data::m_uClientState = **(std::uintptr_t**)(Memory::Scan("engine.dll", "A1 ? ? ? ? 8B 80 ? ? ? ? C3") + 1);//8B F1 E8 ? ? ? ? 8B 8E ? ? ? ? 3B 8E ? ? ? ?
		Engine::Displacement::Data::m_uMoveHelper = **(std::uintptr_t**)(Memory::Scan("client_panorama.dll", "8B 0D ?? ?? ?? ?? 8B 45 ?? 51 8B D4 89 02 8B 01") + 2);
		Engine::Displacement::Data::m_uInput = *(std::uintptr_t*)(Memory::Scan("client_panorama.dll", "B9 ?? ?? ?? ?? F3 0F 11 04 24 FF 50 10") + 1);
		//Data::m_uGlobalVars = **( std::uintptr_t** )( Memory::Scan( "client_panorama.dll", "A3 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 6A" ) + 1 );
		Engine::Displacement::Data::m_uPredictionRandomSeed = *(std::uintptr_t*)(Memory::Scan("client_panorama.dll", "8B 0D ?? ?? ?? ?? BA ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04") + 2); // 8B 0D ? ? ? ? BA ? ? ?? E8 ? ? ? ? 83 C4 04
		Engine::Displacement::Data::m_uUpdateClientSideAnimation = (DWORD)(Memory::Scan("client_panorama.dll", "55 8B EC 51 56 8B F1 80 BE ? ? 00 00 00 74 36"));
		Engine::Displacement::Data::m_uPredictionPlayer = *(std::uintptr_t*)(Memory::Scan("client_panorama.dll", "89 ?? ?? ?? ?? ?? F3 0F 10 48 20") + 2);

		m_pSurface = (ISurface*)CreateInterface("vguimatsurface.dll", "VGUI_Surface");//"vguimatsurface.dll", "VGUI_Surface031", true);

		if (!m_pSurface)
		{
#ifdef DEBUG
			Win32::Error("ISurface is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pInputSystem = (InputSystem*)CreateInterface("inputsystem.dll", "InputSystemVersion001", true);

		if (!m_pInputSystem)
		{
#ifdef DEBUG
			Win32::Error("IInputSystem is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pCvar = (ICvar*)CreateInterface("vstdlib.dll", "VEngineCvar");

		if (!m_pCvar)
		{
#ifdef DEBUG
			Win32::Error("IInputSystem is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pEntList = (IClientEntityList*)CreateInterface("client_panorama.dll", "VClientEntityList");

		if (!m_pEntList)
		{
#ifdef DEBUG
			Win32::Error("IClientEntityList is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pGameMovement = (IGameMovement*)CreateInterface("client_panorama.dll", "GameMovement");

		if (!m_pGameMovement)
		{
#ifdef DEBUG
			Win32::Error("IGameMovement is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pPrediction = (IPrediction*)CreateInterface("client_panorama.dll", "VClientPrediction");

		if (!m_pPrediction)
		{
#ifdef DEBUG
			Win32::Error("IPrediction is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pMaterialSystem = (IMaterialSystem*)CreateInterface("materialsystem.dll", "VMaterialSystem080", true);

		if (!m_pMaterialSystem)
		{
#ifdef DEBUG
			Win32::Error("IMaterialSystem is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pMoveHelper = (IMoveHelper*)(Engine::Displacement::Data::m_uMoveHelper);

		if (!m_pMoveHelper)
		{
#ifdef DEBUG
			Win32::Error("IMoveHelper is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pInput = (IInput*)(Engine::Displacement::Data::m_uInput);

		if (!m_pInput)
		{
#ifdef DEBUG
			Win32::Error("IInput is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pEngineTrace = (IEngineTrace*)CreateInterface("engine.dll", "EngineTraceClient004", true);

		if (!m_pEngineTrace)
		{
#ifdef DEBUG
			Win32::Error("IInput is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pClientState = (CClientState*)(Engine::Displacement::Data::m_uClientState);

		if (!m_pClientState)
		{
#ifdef DEBUG
			Win32::Error("CClientState is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pGlobalVars = **(CGlobalVars***)((*(DWORD**)m_pClient)[0] + 0x1B);

		if (!m_pGlobalVars)
		{
#ifdef DEBUG
			Win32::Error("CGlobalVars is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pModelInfo = (IVModelInfo*)CreateInterface("engine.dll", "VModelInfoClient004", true);

		if (!m_pModelInfo)
		{
#ifdef DEBUG
			Win32::Error("IVModelInfo is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pEngineVGUI = (IEngineVGui*)CreateInterface("engine.dll", "VEngineVGui001", true);

		if (!m_pEngineVGUI)
		{
#ifdef DEBUG
			Win32::Error("IEngineVGui is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pPhysProps = (IPhysicsSurfaceProps*)CreateInterface("vphysics.dll", "VPhysicsSurfaceProps001", true);

		if (!m_pPhysProps)
		{
#ifdef DEBUG
			Win32::Error("IPhysicsSurfaceProps is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pEngine = (IVEngineClient*)CreateInterface("engine.dll", "VEngineClient");

		if (!m_pEngine)
		{
#ifdef DEBUG
			Win32::Error("IVEngineClient is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pPanel = (IPanel*)CreateInterface("vgui2.dll", "VGUI_Panel");

		if (!m_pPanel)
		{
#ifdef DEBUG
			Win32::Error("IPanel is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pRenderView = (IVRenderView*)CreateInterface("engine.dll", "VEngineRenderView014", true);

		if (!m_pRenderView)
		{
#ifdef DEBUG
			Win32::Error("IVRenderView is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pDebugOverlay = (IVDebugOverlay*)CreateInterface("engine.dll", "VDebugOverlay004", true);

		if (!m_pDebugOverlay)
		{
#ifdef DEBUG
			Win32::Error("IVDebugOverlay is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pMemAlloc = *(IMemAlloc**)(GetProcAddress(GetModuleHandleA("tier0.dll"), "g_pMemAlloc"));

		if (!m_pMemAlloc)
		{
#ifdef DEBUG
			Win32::Error("IMemAlloc is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pGameEvents = (IGameEventManager*)(CreateInterface("engine.dll", "GAMEEVENTSMANAGER002", true));

		if (!m_pGameEvents)
		{
#ifdef DEBUG
			Win32::Error("IGameEventManager is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pModelRender = (IVModelRender*)(CreateInterface("engine.dll", "VEngineModel016", true));

		if (!m_pModelRender)
		{
#ifdef DEBUG
			Win32::Error("IVModelRender is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		m_pLocalize = (ILocalize*)(CreateInterface("localize.dll", "Localize_001", true));

		if (!m_pLocalize)
		{
#ifdef DEBUG
			Win32::Error("ILocalize is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}

		if (!Engine::Displacement::Data::m_uDirectX)
		{
#ifdef DEBUG
			Win32::Error("IDevice is nullptr (Source::%s)", __FUNCTION__);
#endif // DEBUG
			return false;
		}



		std::ofstream output;
		output.open("C:\\zeus\\CSGOclassids.txt", std::ofstream::out | std::ofstream::app);
		output << "enum class_ids" << std::endl << "{" << std::endl;

		static auto entry = Source::m_pClient->GetAllClasses();

		while (entry)
		{
			output << "\t" << entry->m_pNetworkName << " = " << entry->m_ClassID << "," << std::endl;
			entry = entry->m_pNext;
		}

		output << "};";
		output.close();

		m_pEngineSound = (IEngineSound*)(CreateInterface(cheat::main::enginedll, s_enginesnd));

		//Engine::PropManager::Instance()->DumpProps();

		/*DynAppSysFactory("engine.dll");
		DynAppSysFactory("vgui2.dll");
		DynAppSysFactory("vphysics.dll");
		DynAppSysFactory("materialsystem.dll");
		DynAppSysFactory("client_panorama.dll");
		DynAppSysFactory("vstdlib.dll");
		DynAppSysFactory("inputsystem.dll");
		DynAppSysFactory("matchmaking.dll");
		DynAppSysFactory("serverbrowser.dll");
		DynAppSysFactory("server.dll");

		DynAppSysFactory("datacache.dll");
		DynAppSysFactory("vguimatsurface.dll");

		DynAppSysFactory("filesystem_stdio.dll");*/

		cheat::main::localstate.last_anim_upd_tick = 0;
		cheat::main::fired_shot._avg_impact_time = 0.f;
		cheat::main::command_numbers.clear();

		cheat::main::side = -1;
		cheat::main::fside = -1;
		cheat::main::shots = 0;
		cheat::main::shots_hit = 0;
		cheat::main::shots_missed = 0;

		cheat::features::clientside_animlist = *(CUtlVector<clientanimating_t>**)(Memory::Scan(cheat::main::clientdll, s_sig_clientsideanims) + 1);
		cheat::features::effects_head = **reinterpret_cast<CClientEffectRegistration***>(Memory::Scan(cheat::main::clientdll, s_sig_effectshead) + 2);

		m_pClientSwap = std::make_shared<Memory::VmtSwap>(m_pClient);
		m_pSurfaceSwap = std::make_shared<Memory::VmtSwap>(m_pSurface);
		m_pPredictionSwap = std::make_shared<Memory::VmtSwap>(m_pPrediction);
		m_pPanelSwap = std::make_shared<Memory::VmtSwap>(m_pPanel);
		m_pRenderViewSwap = std::make_shared<Memory::VmtSwap>(m_pRenderView);
		m_pEngineSwap = std::make_shared<Memory::VmtSwap>(m_pEngine);
		m_pEngineVGUISwap = std::make_shared<Memory::VmtSwap>(m_pEngineVGUI);
		m_pModelRenderSwap = std::make_shared<Memory::VmtSwap>(m_pModelRender);
		m_pDeviceSwap = std::make_shared<Memory::VmtSwap>((void*)Engine::Displacement::Data::m_uDirectX);
		//auto device_ptr = (**(uintptr_t**)(offsets::directx));
		//m_pClientStateSwap = std::make_shared<Memory::VmtSwap>((uint32_t*)(uint32_t(m_pClientState) + 0x8));
	
		m_pEngineSwap->Hook(&Hooked::IsHLTV, 93);
		m_pEngineSwap->Hook(&Hooked::GetScreenAspectRatio, 101);
		//m_pEngineSwap->Hook(&Hooked::FireEvents, 59);
		//m_pEngineSwap->Hook(&Hooked::IsBoxVisible, 32);

		//m_pDeviceSwap->Hook(&Hooked::EndScene, Index::IDirectX::EndScene);
		//m_pDeviceSwap->Hook(&Hooked::Reset, Index::IDirectX::Reset);

		//m_pEngineSwap->Hook(&Hooked::IsConnected, 27);
		//m_pClientModeSwap->Hook(&Hooked::CreateMove, Index::IBaseClientDLL::CreateMove);
		m_pEngineVGUISwap->Hook(&Hooked::EngineVGUI_Paint, 14);
		//m_pCreateMoveThread = CreateThread(nullptr, 0u, (LPTHREAD_START_ROUTINE)createmove_thread, hModule, 0u, nullptr);

		auto m_pClientMode = **(void***)((*(DWORD**)Source::m_pClient)[10] + 5);

		if (m_pClientMode)
		{
			m_pClientModeSwap = std::make_shared<Memory::VmtSwap>(m_pClientMode);
			m_pClientModeSwap->Hook(&Hooked::CreateMove, Index::IBaseClientDLL::CreateMove);
			m_pClientModeSwap->Hook(&Hooked::GetViewModelFOV, Index::IBaseClientDLL::GetViewModelFOV);
			m_pClientModeSwap->Hook(&Hooked::OverrideView, Index::IBaseClientDLL::OverrideView);
			m_pClientModeSwap->Hook(&Hooked::DoPostScreenEffects, 44);
			m_pClientModeSwap->Hook(&Hooked::key_event, 20);
		}

		const std::string loli = s_hook_impact;

		for (auto head = cheat::features::effects_head; head; head = head->next)
		{
			if (strstr(head->effectName, loli.c_str()) && strlen(head->effectName) <= 8) {
				oImpact = head->function;
				head->function = &Hooked::impact_callback;
				break;
			}
		}

		m_pClientSwap->Hook(&Hooked::FrameStageNotify, Index::IBaseClientDLL::FrameStageNotify);
		//m_pClientSwap->Hook(&Hooked::WriteUsercmdDeltaToBuffer, 24);

		//key_event

		cheat::main::setuping_bones = true;
		cheat::main::updating_resolver_anims = false;

		/*static auto SetupVelocity = (DWORD)(Memory::Scan("client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 30 56 57 8B 3D"));
		OriginalSetupVelocity = (DWORD)DetourFunction((byte*)SetupVelocity, (byte*)Hooked::SetupVelocity);

		static auto CalcAbsoluteVelocity = (DWORD)(Memory::Scan(cheat::main::clientdll, "55 8B EC 83 E4 F8 83 EC 1C 53 56 57 8B F9 F7"));
		OriginalCalcAbsoluteVelocity = (DWORD)DetourFunction((byte*)CalcAbsoluteVelocity, (byte*)Hooked::CalcAbsoluteVelocity);*/

		static auto StandardBlendingRules = (DWORD)(Memory::Scan(cheat::main::clientdll, "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6"));
		OriginalStandardBlendingRules = (DWORD)DetourFunction((byte*)StandardBlendingRules, (byte*)Hooked::StandardBlendingRules);

		static auto DoExtraBonesProcessing = (DWORD)(Memory::Scan(cheat::main::clientdll, "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 8B F1 57 89 74 24 1C"));
		OriginalDoExtraBonesProcessing = (DWORD)DetourFunction((byte*)DoExtraBonesProcessing, (byte*)Hooked::DoExtraBonesProcessing);

		static auto GetForeignFallbackFontName = (DWORD)(Memory::Scan("vguimatsurface.dll", "80 3D ? ? ? ? ? 74 06 B8"));
		OriginalGetForeignFallbackFontName = (DWORD)DetourFunction((byte*)GetForeignFallbackFontName, (byte*)Hooked::GetForeignFallbackFontName);

		static auto EyeAngles = (DWORD)(Memory::Scan("client_panorama.dll", "56 8B F1 85 F6 74 32"));
		OriginalEyeAngles = (DWORD)DetourFunction((byte*)EyeAngles, (byte*)Hooked::EyeAngles);


		//static auto slol = (DWORD)Memory::Scan("client_panorama.dll", "55 8B EC 81 EC ? ? ? ? 80 3D ? ? ? ? ? 56 57 8B F9");
		//OriginalLol = (DWORD)DetourFunction((byte*)slol, (byte*)Hooked::DangerZoneSmth);
		dwCCSPlayerRenderablevftable = *(DWORD*)(Memory::Scan("client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 89 7C 24 0C") + 0x4E);

		//auto sv_impacts_bool = *(DWORD*)(Memory::Scan("client_panorama.dll", "24 01 8B 0D ? ? ? ?") + 0x1);

		//auto sv_showimpacts = Source::m_pCvar->FindVar("sv_showimpacts");
		//m_pShowImpactsSwap = std::make_shared<Memory::VmtSwap>(reinterpret_cast<DWORD**>(sv_showimpacts));
		//m_pShowImpactsSwap->Hook(&Hooked::get_int, 13);//original_get_bool = reinterpret_cast<SvCheatsGetBoolFn>(get_bool_manager.HookFunction<SvCheatsGetBoolFn>(13, HookedGetBool));

		//if (dwCCSPlayerRenderablevftable != NULL)
		//	oSetupBones = Memory::VFTableHook::HookManual<SetupBones_t>((uintptr_t*)dwCCSPlayerRenderablevftable, 13, (SetupBones_t)Hooked::SetupBones);

		//if (o_SetupBones == nullptr)
		//	Win32::Error("oSetupBones is nullptr (Source::%s)", __FUNCTION__);

		//oTempEntities = Memory::VFTableHook::HookManual<TempEntities_t>(*(uintptr_t**)((int)Source::m_pClientState + 0x8), 36, (TempEntities_t)Hooked::TempEntities);

		//m_pClientStateSwap->Hook(&Hooked::PacketStart, 5);
		
		//auto m_pFireBullets = *(DWORD**)(Memory::Scan("client_panorama.dll", "55 8B EC 51 53 56 8B F1 BB ? ? ? ? B8") + 0x131);

		//m_pFireBulletsSwap = std::make_shared<Memory::VmtSwap>(m_pFireBullets);

		//oFireBullets = Memory::VFTableHook::HookManual<FireBullets_t>(*(uintptr_t**)m_pFireBullets, 7, (FireBullets_t)&Hooked::CTEFireBullets__PostDataUpdate); //(FireBullets_t)m_pFireBulletsSwap->Hook(&Hooked::CTEFireBullets__PostDataUpdate, 7);
		//static auto lol = (DWORD)Memory::Scan("engine.dll", "55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D ? ? ? ? ");
		//OriginalLol = (DWORD)DetourFunction((byte*)lol, (byte*)Hooked::CL_Move);
		//if (!oFireBullets || !m_pFireBullets)
		//	Win32::Error("C_TEFireBullets::PostDataUpdate is nullptr (Source::%s)", __FUNCTION__);

		//m_nSmokeEffectTickBegin = pPropManager->Hook(Hooked::m_nSmokeEffectTickBeginHook, "DT_SmokeGrenadeProjectile", "m_nSmokeEffectTickBegin");
		m_flSimulationTime = pPropManager->Hook(Hooked::m_flSimulationTimeHook, "DT_BaseEntity", "m_flSimulationTime");
		//m_flAbsYaw = pPropManager->Hook(Hooked::m_flAbsYawHook, "DT_CSRagdoll", "m_flAbsYaw");
		//m_bClientSideAnimation = pPropManager->Hook(Hooked::m_bClientSideAnimationHook, "DT_BaseAnimating", "m_bClientSideAnimation");

		m_pPredictionSwap->Hook(&Hooked::InPrediction, Index::IPrediction::InPrediction);
		m_pPredictionSwap->Hook(&Hooked::SetupMove, Index::IPrediction::SetupMove);
		m_pSurfaceSwap->Hook(&Hooked::LockCursor, Index::ISurface::LockCursor);
		m_pPredictionSwap->Hook(&Hooked::RunCommand, Index::IPrediction::RunCommand);
		m_pPanelSwap->Hook(&Hooked::PaintTraverse, Index::IPanel::PaintTraverse);
		
		m_pRenderViewSwap->Hook(&Hooked::SceneEnd, 9);
		m_pModelRenderSwap->Hook(&Hooked::DrawModelExecute, 21);

		const std::string lol = s_wndproc_valve;

		Window = FindWindowA(lol.c_str(), NULL);

		Hooked::oldWindowProc = (WNDPROC)SetWindowLongPtr(Window, GWL_WNDPROC, (LONG_PTR)Hooked::WndProc);

		Drawing::CreateFonts();

		game_events::init();

		cheat::features::music.init();
		
		_events.clear();

		

		//_events.push_back(_event(Source::m_pGlobalVars->curtime + 2.f, "BETA Injected"));

		cheat::menu.Setup();

		//cheat::game::log("injected");

		//auto base = **reinterpret_cast<ConCommandBase***>(Source::m_pCvar + 0x44);

		//int count = 0;

		//// set all flags to 0
		//for (auto c = base->m_pNext; c != nullptr; c = c->m_pNext)
		//{
		//	// TODO fix masking
		//	//maskCvar("tf", (ConVar *)base);

		//	c->m_nFlags = 0;

		//	count++;
		//}

		//Source::m_pCvar->ConsolePrintf("Unlocked %d cvars\n", count);

		cheat::Cvars.GradientFrom.SetColor(Color(66, 140, 244));
		cheat::Cvars.GradientTo.SetColor(Color(75, 180, 242));
		cheat::Cvars.MenuTheme.SetColor(Color(66, 140, 244));

		cheat::Cvars.EnemyBoxCol.SetColor(Color(0, 129, 255));
		cheat::Cvars.Visuals_glow_color.SetColor(Color(0, 129, 255));
		cheat::Cvars.Visuals_lglow_color.SetColor(Color(0, 129, 255));
		cheat::Cvars.Visuals_SnaplinesColor.SetColor(Color(128, 0, 128));
		cheat::Cvars.Visuals_skeletonColor.SetColor(Color(105, 105, 105));
		cheat::Cvars.Visuals_chams_color.SetColor(Color(0, 129, 255));
		cheat::Cvars.Visuals_chams_hidden_color.SetColor(Color(0, 255, 129));
		cheat::Cvars.Visuals_chams_history_color.SetColor(Color(0, 255, 129));
		cheat::Cvars.Visuals_wrld_entities_color.SetColor(Color(0, 255, 129));
		cheat::Cvars.Misc_AntiUT.SetValue(1.f);

		cheat::features::antiaimbot.enable_delay = 0.f;

		cheat::main::updating_anims = false;
		cheat::features::antiaimbot.flip_side = false;

		return true;
	}

	void Destroy()
	{
		
		DetourRemove((byte*)OriginalCalcAbsoluteVelocity, (byte*)Hooked::CalcAbsoluteVelocity);
		DetourRemove((byte*)OriginalSetupVelocity, (byte*)Hooked::SetupVelocity);
		DetourRemove((byte*)OriginalStandardBlendingRules, (byte*)Hooked::StandardBlendingRules);
		DetourRemove((byte*)OriginalDoExtraBonesProcessing, (byte*)Hooked::DoExtraBonesProcessing);
		DetourRemove((byte*)OriginalGetForeignFallbackFontName, (byte*)Hooked::GetForeignFallbackFontName);
		DetourRemove((byte*)OriginalEyeAngles, (byte*)Hooked::EyeAngles);

		//DetourRemove((byte*)OriginalLol, (byte*)Hooked::DangerZoneSmth);
		//CloseHandle(m_pCreateMoveThread);

		const std::string lol = s_hook_impact;

		for (auto head = cheat::features::effects_head; head; head = head->next)
		{
			if (strstr(head->effectName, lol.c_str()) && strlen(head->effectName) <= 8) {
				head->function = oImpact;
				break;
			}
		}

		//if (cheat::main::localstate) {
		//	Source::m_pMemAlloc->Free(cheat::main::localstate);
		//	cheat::main::localstate = nullptr;
		//}

		//Engine::PropManager::Instance()->Hook(m_nSmokeEffectTickBegin, "DT_SmokeGrenadeProjectile", "m_nSmokeEffectTickBegin");
		Engine::PropManager::Instance()->Hook(m_flSimulationTime, "DT_BaseEntity", "m_flSimulationTime");
		//Engine::PropManager::Instance()->Hook(m_flAbsYaw, "DT_CSRagdoll", "m_flAbsYaw");
		//Engine::PropManager::Instance()->Hook(m_bClientSideAnimation, "DT_BaseAnimating", "m_bClientSideAnimation");

		m_pClientSwap.reset();
		m_pPredictionSwap.reset();
		m_pPanelSwap.reset();
		m_pClientModeSwap.reset();
		m_pSurfaceSwap.reset();
		m_pRenderViewSwap.reset();
		m_pEngineSwap.reset();
		m_pClientStateSwap.reset();
		m_pEngineVGUISwap.reset();
		m_pModelRenderSwap.reset();
		m_pNetChannelSwap.reset();
		m_pDeviceSwap.reset();
		
		//m_pShowImpactsSwap.reset();

		//cheat::convars::spoofed::viewmodel_offset_x->Restore();
		//cheat::convars::spoofed::viewmodel_offset_y->Restore();
		//cheat::convars::spoofed::viewmodel_offset_z->Restore();
		//cheat::convars::spoofed::sv_cheats->Restore();

		//DetourRemove((byte*)OriginalLol, (byte*)Hooked::CL_Move);
		//Memory::VFTableHook::HookManual<TempEntities_t>(*(uintptr_t**)((int)Source::m_pClientState + 0x8), 36, oTempEntities);
		//if (dwCCSPlayerRenderablevftable)
		//	Memory::VFTableHook::HookManual<SetupBones_t>((uintptr_t*)dwCCSPlayerRenderablevftable, 13, oSetupBones);
		//DetourRemove((byte*)OriginalLol, (byte*)Hooked::CL_SendMove);
		SetWindowLongPtr(Window, GWL_WNDPROC, (LONG_PTR)Hooked::oldWindowProc);
	}

	void* CreateInterface(const std::string& image_name, const std::string& name, bool force /*= false */)
	{
		auto image = GetModuleHandleA(image_name.c_str());

		if (!image)
			return nullptr;

		auto fn = (CreateInterfaceFn)(GetProcAddress(image, "CreateInterface"));

		if (!fn)
			return nullptr;

		if (force)
			return fn(name.c_str(), nullptr);

		char format[1024] = { };

		for (auto i = 0u; i < 1000u; i++)
		{
			sprintf_s(format, "%s%03u", name.c_str(), i);

			auto factory = fn(format, nullptr);

			if (factory)
				return factory;
		}

		return nullptr;
	}
}

namespace cheat
{
	namespace game
	{
		bool pressed_keys[256] = {};
		int last_frame = -1;
		inline bool get_key_press(int key, int zticks)
		{
			static int ticks[256];
			bool returnvalue = false;

			if (pressed_keys[key])
			{
				ticks[key]++;

				if (ticks[key] <= zticks)
				{
					returnvalue = true;
				}
			}
			else
				ticks[key] = 0;

			return returnvalue;
		}
		void log(std::string text)
		{
			std::ofstream code("C:\\zeus\\cheat.log", std::ios::app);
			if (code.is_open())
			{
				code << text << std::endl;
				code.close();
			}
		}

		CUserCmd* last_cmd = nullptr;
		void* update_hud_weapons = nullptr;
		CCSGO_HudDeathNotice* hud_death_notice = nullptr;
		Vector2D screen_size = Vector2D(0,0);
	}
	namespace features {
		c_drawhack menu;
		c_visuals visuals;
		c_aimbot aimbot;
		c_lagcomp lagcomp;
		c_antiaimbot antiaimbot;
		c_resolver aaa;
		c_autowall autowall;
		c_music_player music;
		CUtlVector<clientanimating_t> *clientside_animlist;
		CClientEffectRegistration *effects_head;
		c_dormant_esp dormant;
		c_misc misc;
		c_legitbot legitbot;
	}
	namespace main {
		FORCEINLINE C_BasePlayer* local()
		{
			auto index = Source::m_pEngine->GetLocalPlayer();

			if (!index)
				return nullptr;

			auto client = Source::m_pEntList->GetClientEntity(index);

			if (!client)
				return nullptr;

			return client/*ToCSPlayer(client->GetBaseEntity())*/;
		}
		CCSGOPlayerAnimState localstate;
		QAngle fix;
		bool reset_local_animstate = false;
		float lerp_time;
		bool fakewalking;
		bool updating_skins;
		int prev_fakelag_value;
		int side;
		int fside;
		int shots_fired[128];
		int shots_total[128];
		int history_hit[128];
		bool thirdperson;
		bool send_packet;
		bool updating_anims;
		bool called_chams_render;
		std::vector<Vector> points[64][19];
		_shotinfo fired_shot;
		std::vector<int> command_numbers;
		matrix3x4_t localplaya_matrix[128];
		float hit_time;
		bool jittering;
		std::string enginedll;
		std::string clientdll;
		float fov;
		bool fakeducking;
		int last_penetrated_count;
		int known_cmd_nr;
		bool setuping_bones;
		bool setuped_bones;
		std::wstring nickname;
		std::array<float, 24> fake_pose;
		int shots;
		int shots_hit;
		int shots_missed;
		float real_angle;
		float real_yaw;
		int fps;
		float last_kill_time;
		bool updating_resolver_anims;
		bool fast_autostop;
		float fake_angle;
		bool stand;
		int unpred_tickcount;
		std::array<float, 24> real_pose;
		bool should_update_local_anims = false;
		C_AnimationLayer local_server_anim_layers[13];
		QAngle local_eye_angles;
		Vector local_bone_origin_delta[128];
	}
	namespace convars {
		int sv_usercmd_custom_random_seed;
		float weapon_recoil_scale;
		int weapon_accuracy_nospread;
		int sv_clip_penetration_traces_to_players;
		float ff_damage_reduction_bullets;
		float ff_damage_bullet_penetration;
		int sv_penetration_type;

		//namespace spoofed {
			/*SpoofedConvar *viewmodel_offset_x = nullptr;
			SpoofedConvar *viewmodel_offset_y = nullptr;
			SpoofedConvar *viewmodel_offset_z = nullptr;
			SpoofedConvar *sv_cheats = nullptr;*/
		//}
	}
	c_variables settings;
	CVars Cvars;
	CMenu menu;
}