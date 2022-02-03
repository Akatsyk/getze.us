#include "hooked.hpp"
#include "prediction.hpp"
#include "player.hpp"
#include "lag_compensation.hpp"
#include "anti_aimbot.hpp"
#include "angle_resolver.hpp"
#include <chrono>
#include "visuals.hpp"
#include "prop_manager.hpp"
#include "rmenu.hpp"
#include "menu.h"
#include "weapon.hpp"
#include "movement.hpp"
#include "misc.hpp"
#include "thread/threading.h"
#include "thread/shared_mutex.h"

#include <thread>

const unsigned int FCLIENTANIM_SEQUENCE_CYCLE = 0x00000001;

void fix_local_anims() {
	auto animstate = cheat::main::local()->get_animation_state();
	if (!animstate)
		return;

	const auto local = cheat::main::local();
	if (!cheat::game::last_cmd || !local || local->IsDead())
		return;

	animstate->abs_yaw = cheat::main::real_yaw;

	static std::array<float, 24> sent_pose_params = cheat::main::local()->m_flPoseParameter();
	static int last_tick = -1;
	static float angle = animstate->abs_yaw;

	int current_tick = cheat::main::unpred_tickcount;
	if (current_tick != cheat::main::unpred_tickcount)
		current_tick = Source::m_pGlobalVars->tickcount;

	if (last_tick != current_tick)
	{
		last_tick = current_tick;

		cheat::main::should_update_local_anims = cheat::main::local()->client_side_animation() = true;
		cheat::main::local()->LastBoneSetupFrame() = -FLT_MAX; // invalidate bone cache for proper matrix
		*(std::uint32_t*)(uintptr_t(cheat::main::local()) + 0x2690) = 0; // invalidate bone cache for proper matrix
		cheat::main::local()->update_client_anims();
		if (cheat::main::send_packet)
		{
			angle = animstate->abs_yaw;
			sent_pose_params = cheat::main::local()->m_flPoseParameter();
		}
	}
	animstate->abs_yaw = angle;
	cheat::main::should_update_local_anims = cheat::main::local()->client_side_animation() = false;
	cheat::main::local()->set_abs_angles({ 0.f, angle, 0.f });
	std::memcpy(cheat::main::local()->animation_layers_ptr(), cheat::main::local_server_anim_layers, sizeof(C_AnimationLayer) * 13);
	cheat::main::local()->m_flPoseParameter() = sent_pose_params;

	if (!Source::m_pClientState->m_iChockedCommands) {
		local->force_bone_rebuild();
		local->SetupBonesEx();
		memcpy(cheat::features::antiaimbot.last_sent_matrix, cheat::main::local()->m_CachedBoneData().Base(), cheat::main::local()->GetBoneCount() * sizeof(matrix3x4_t));

		for (int i = 0; i < 128; ++i)
			cheat::main::local_bone_origin_delta[i] = cheat::main::local()->m_vecOrigin() - cheat::features::antiaimbot.last_sent_matrix[i].GetOrigin();
	}

	//const auto cmd = cheat::game::last_cmd;
	//const auto local = cheat::main::local();
	//if (!cmd || !local || local->IsDead()) return;

	//const auto old_curtime = Source::m_pGlobalVars->curtime, old_frametime = Source::m_pGlobalVars->frametime;

	//static int old_cmd_nr = 0;
	//if (old_cmd_nr != cheat::game::last_cmd->command_number) {
	//	old_cmd_nr = cheat::game::last_cmd->command_number;

	//	Source::m_pGlobalVars->curtime = local->m_flSimulationTime(), Source::m_pGlobalVars->frametime = Source::m_pGlobalVars->interval_per_tick;

	//	local->client_side_animation() = true;

	//	cheat::main::should_update_local_anims = true;
	//	local->update_client_anims();
	//	cheat::main::should_update_local_anims = false;

	//	std::memcpy(local->animation_layers_ptr(), cheat::main::local_server_anim_layers, sizeof(C_AnimationLayer) * 13);

	//	if (!Source::m_pClientState->m_iChockedCommands) {
	//		local->m_flPoseParameter().at(6) = 0; // magic trick ;^)
	//		local->force_bone_rebuild();
	//		local->SetupBonesEx();
	//		memcpy(cheat::features::antiaimbot.last_sent_matrix, local->m_CachedBoneData().Base(), local->GetBoneCount() * sizeof(matrix3x4_t));

	//		for (int i = 0; i < 128; ++i)
	//			cheat::main::local_bone_origin_delta[i] = local->m_vecOrigin() - cheat::features::antiaimbot.last_sent_matrix[i].GetOrigin();
	//	}

	//	Source::m_pGlobalVars->curtime = old_curtime, Source::m_pGlobalVars->frametime = old_frametime;
	//}
}

class CFlashLightEffect
{
public:
	bool m_bIsOn; //0x0000 
	char pad_0x0001[0x3]; //0x0001
	int m_nEntIndex; //0x0004 
	WORD m_FlashLightHandle; //0x0008 
	char pad_0x000A[0x2]; //0x000A
	float m_flMuzzleFlashBrightness; //0x000C 
	float m_flFov; //0x0010 
	float m_flFarZ; //0x0014 
	float m_flLinearAtten; //0x0018 
	bool m_bCastsShadows; //0x001C 
	char pad_0x001D[0x3]; //0x001D
	float m_flCurrentPullBackDist; //0x0020 
	DWORD m_MuzzleFlashTexture; //0x0024 
	DWORD m_FlashLightTexture; //0x0028 
	char m_szTextureName[64]; //0x1559888 
}; //Size=0x006C

void UpdateFlashLight(CFlashLightEffect *pFlashLight, const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp)
{
	// here we tell to the compiler wich calling convention we will use to call the function and the paramaters needed (note the __thiscall*)
	typedef void(__thiscall* UpdateLight_t)(void*, int, const Vector&, const Vector&, const Vector&, const Vector&, float, float, float, bool, const char*);

	static UpdateLight_t oUpdateLight = NULL;

	if (!oUpdateLight)
	{
		// here we have to deal with a call instruction (E8 rel32)
		DWORD callInstruction = Memory::Scan(cheat::main::clientdll, "E8 ? ? ? ? 8B 06 F3 0F 10 46"); // get the instruction address // CFlashlightEffect::UpdateLight
		DWORD relativeAddress = *(DWORD*)(callInstruction + 1); // read the rel32
		DWORD nextInstruction = callInstruction + 5; // get the address of next instruction
		oUpdateLight = (UpdateLight_t)(nextInstruction + relativeAddress); // our function address will be nextInstruction + relativeAddress
	}

	oUpdateLight(pFlashLight, pFlashLight->m_nEntIndex, vecPos, vecForward, vecRight, vecUp, pFlashLight->m_flFov, pFlashLight->m_flFarZ, pFlashLight->m_flLinearAtten, pFlashLight->m_bCastsShadows, pFlashLight->m_szTextureName);
}

CFlashLightEffect *CreateFlashLight(int nEntIndex, const char *pszTextureName, float flFov, float flFarZ, float flLinearAtten)
{
	static DWORD oConstructor = Memory::Scan(cheat::main::clientdll, "55 8B EC F3 0F 10 45 ? B8");

	/* // effects/flashlight_freezecam
	 push    ebp
	.text:1027F701                 mov     ebp, esp
	.text:1027F703                 movss   xmm0, [ebp+arg_8]
	.text:1027F708                 mov     eax, 0FFFFh
	.text:1027F70D                 push    esi
	.text:1027F70E                 push    [ebp+arg_4]
	.text:1027F711                 mov     esi, ecx
	*/

	// we need to use the engine memory management if we are calling the destructor later
	CFlashLightEffect *pFlashLight = (CFlashLightEffect*)Source::m_pMemAlloc->Alloc(sizeof(CFlashLightEffect));

	if (!pFlashLight) {
		//std::cout << "Mem Alloc for FL failed" << std::endl;
		return NULL;
	}

	//std::cout << "pFlashLight: " << reinterpret_cast<DWORD*>(pFlashLight) << std::endl;

	// we need to call this function on a non standard way
	__asm
	{
		movss xmm3, flFov
		mov ecx, pFlashLight
		push flLinearAtten
		push flFarZ
		push pszTextureName
		push nEntIndex
		call oConstructor
	}

	return pFlashLight;
}

void DestroyFlashLight(CFlashLightEffect *pFlashLight)
{
	/*
	.text:101E4080                 push    esi
	.text:101E4081                 mov     esi, ecx
	.text:101E4083                 call    sub_10281AE0
	.text:101E4088                 mov     ecx, [esi+28h]
	.text:101E408B                 test    ecx, ecx
	.text:101E408D                 jz      short loc_101E40A4
	.text:101E408F                 cmp     dword_15232F1C, 0
	.text:101E4096                 jz      short loc_101E40A4
	.text:101E4098                 mov     eax, [ecx]
	.text:101E409A                 call    dword ptr [eax+2Ch]
	.text:101E409D                 mov     dword ptr [esi+28h], 0
	*/

	static auto oDestroyFlashLight = reinterpret_cast<void(__thiscall*)(void*, void*)>(Memory::Scan(cheat::main::clientdll, "56 8B F1 E8 ? ? ? ? 8B 4E 28"));

	//std::cout << "oDestroyFlashLight: " << reinterpret_cast<DWORD*>(oDestroyFlashLight) << std::endl;

	// the flash light destructor handles the memory management, so we dont need to free the allocated memory
	// call the destructor of the flashlight
	oDestroyFlashLight(pFlashLight, pFlashLight);
}

void RunFrame()
{
	static CFlashLightEffect *pFlashLight = NULL;

	if (cheat::game::get_key_press((int)cheat::Cvars.RageBot_flashlightkey.GetValue()) && cheat::Cvars.RageBot_flashlightkey.GetValue())
	{
		if (!pFlashLight)
			pFlashLight = CreateFlashLight(cheat::main::local()->entindex(), "effects/flashlight001", 40, 1000, 2000);
		else
		{
			DestroyFlashLight(pFlashLight);
			pFlashLight = NULL;
		}

		Source::m_pSurface->PlaySound_("items\\flashlight1.wav");
	}

	if (pFlashLight && !cheat::main::local()->IsDead() && Source::m_pEngine->IsConnected() && cheat::Cvars.Visuals_flashlight.GetValue())
	{
		Vector f, r, u;
		auto viewAngles = Engine::Movement::Instance()->m_qRealAngles;
		Math::AngleVectors(viewAngles, &f, &r, &u);

		pFlashLight->m_bIsOn = true;
		pFlashLight->m_bCastsShadows = false;
		pFlashLight->m_flFov = 40;/*fabsf(13 + 37 * cosf(x += 0.002f))*/;
		pFlashLight->m_flLinearAtten = 3000;
		UpdateFlashLight(pFlashLight, cheat::main::local()->GetEyePosition(), f, r, u);

		//std::cout << "UpdateFlashLight pFlashLight: " << reinterpret_cast<DWORD*>(pFlashLight) << std::endl;
	}
	else if (pFlashLight && (!Source::m_pEngine->IsConnected() || cheat::main::local()->IsDead() || !cheat::Cvars.Visuals_flashlight.GetValue()))
	{
		DestroyFlashLight(pFlashLight);
		pFlashLight = NULL;
	}
}

using CNETMsg_File_constructor_fn = void(__thiscall *)(void*);
using CNETMsg_File_destructor_fn = void(__thiscall *)(void*);
using CNETMsg_File_proto_fn = void(__thiscall *)(void*, void*);

template<typename t> t follow_rel32(uintptr_t address, size_t offset) {

	if (!address)
		return t{};

	auto offsetAddr = address + offset;
	auto relative = *(uint32_t *)offsetAddr;
	if (!relative)
		return t{};

	return (t)(offsetAddr + relative + sizeof(uint32_t));
}

//static auto render_nightmode = []() -> void
//{
//	if (!Source::m_pEngine->IsConnected() || cheat::main::local() == nullptr)
//		return;
//
//	//RunFrame();
//
//	static auto sv_skyname = Source::m_pCvar->FindVar("sv_skyname");
//	static auto ccvar = Source::m_pCvar->FindVar("r_modelAmbientMin");
//	sv_skyname->m_nFlags &= ~FCVAR_CHEAT;
//
//	//static auto asus_value_static = 100;
//	static float asus_value[2] = { 1.f, 1.f };
//
//	//static auto prev_skybox_clr = 100;
//	static float skybox_color[2] = { 20.f, 1.f };
//
//	//static float time = Source::m_pGlobalVars->curtime + 5.f;
//
//	/*if (time < Source::m_pGlobalVars->curtime)
//	{
//	skybox_color = asus_value = 100;
//	PlaySoundA((RandomInt(0,1) == 1 ? rawData1 : rawData2), NULL, SND_ASYNC | SND_MEMORY);
//
//	if (ccvar)
//	ccvar->SetValue(20);
//	}*/
//
//	Color color = Color().FromHSB(cheat::settings.map_color[0], cheat::settings.map_color[2], 1.f).alpha(cheat::settings.map_color[1] * 255.f);
//	Color skycolor = Color().FromHSB(cheat::settings.sky_color[0], cheat::settings.sky_color[2], 1.f).alpha(cheat::settings.sky_color[1] * 255.f);
//
//	if (color.Hue() != asus_value[0] || cheat::settings.map_color[1] != asus_value[1] || skycolor.Hue() != skybox_color[0] || cheat::settings.sky_color[1] != skybox_color[1])
//	{
//		for (auto handle = Source::m_pMaterialSystem->FirstMaterial(); handle != Source::m_pMaterialSystem->InvalidMaterial(); handle = Source::m_pMaterialSystem->NextMaterial(handle))
//		{
//			auto material = Source::m_pMaterialSystem->GetMaterial(handle);
//
//			if (material == nullptr)
//				continue;
//
//			if (std::string(material->GetTextureGroupName()).compare("World textures") == 0 || std::string(material->GetTextureGroupName()).compare("StaticProp") == 0)//StaticProp
//			{
//				//auto asus_float_value = asus_value / 100.f;
//
//				auto r = float(color.r() / 255.f) * cheat::settings.map_color[1],
//					g = float(color.g() / 255.f) * cheat::settings.map_color[1],
//					b = float(color.b() / 255.f) * cheat::settings.map_color[1];
//
//				material->ColorModulate(r, g, b);
//
//				//if (std::string(material->get_name()).compare("dev/dev_measuregeneric01b") != 0)
//				//{
//				//	material->set_material_var_flag(MATERIAL_VAR_TRANSLUCENT, asus_value < 100);
//
//				//	material->alpha_modulate(static_cast<float>(asus_value * 0.01f));
//				//}
//
//				//material->SetShaderAndParams();
//
//
//				material->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, false);
//			}
//			else if (std::string(material->GetTextureGroupName()).compare("SkyBox textures") == 0)
//			{
//				sv_skyname->SetValue("sky_day02_10");
//
//				//if (asus_value < 100)
//				//	material->colour_modulate(0.f, 0.f, 0.f);
//				//else
//				//	material->colour_modulate(1.f, 1.f, 1.f);
//				//auto asus_float_value = skybox_color / 100.f;
//
//				material->ColorModulate(float(skycolor.r() / 255.f) * cheat::settings.sky_color[1], float(skycolor.g() / 255.f) * cheat::settings.sky_color[1], float(skycolor.b() / 255.f) * cheat::settings.sky_color[1]);
//			}
//		}
//
//		asus_value[0] = color.Hue();
//		skybox_color[0] = skycolor.Hue();
//		asus_value[1] = cheat::settings.map_color[1];
//		skybox_color[1] = cheat::settings.sky_color[1];
//		//if (asus_value_static == 100)
//		//	G::interfaces.engine->client_cmd("mat_reloadallmaterials");
//	}
//
//	/*if (time < Source::m_pGlobalVars->curtime) {
//
//	if (ccvar)
//	ccvar->SetValue(0);
//	time = Source::m_pGlobalVars->curtime + 50.f;
//	}
//	else
//	skybox_color = 20, asus_value = 1;*/
//};

namespace Hooked
{
	static char mapname[128] = "";
	static bool old_map_light = false;
	bool svchts = false;
	static bool checked_fakelag = false;

	void __fastcall FrameStageNotify(void* ecx, void* edx, ClientFrameStage_t stage)
	{
		static float old_viewoffs = -1.f; 
		static auto cycle = 0.f;
		static auto anim_time = 0.f;

		auto clantag_changer = []() -> void
		{
			static std::string cur_clantag = " cheat ";
			static float oldTime = -1.f;

			auto setclantag = [](const char* tag, const char* lol = "Piska") -> void
			{
				typedef void(__fastcall* SetClanTagFn)(const char*, const char*);
				static auto set_clan_tag = reinterpret_cast<SetClanTagFn>(Memory::Scan(cheat::main::enginedll, "53 56 57 8B DA 8B F9 FF 15"));

				if (!set_clan_tag)
					return;

				set_clan_tag(tag, lol);
			};

			auto Marquee = [](std::string& clantag) -> void
			{
				std::string temp = clantag;
				clantag.erase(0, 1);
				clantag += temp[0];
			};

			static float old_tick = 0;
			float latency = 0;
			auto tickrate = (1.0f / Source::m_pGlobalVars->interval_per_tick);
			INetChannelInfo* netchan = Source::m_pEngine->GetNetChannelInfo();

			if (netchan)
				latency = TIME_TO_TICKS(netchan->GetAvgLatency(0));

			auto current_tick = int((latency + Source::m_pGlobalVars->tickcount) / (tickrate / 2)) % 16;

			if (!cheat::Cvars.Visuals_misc_tag.GetValue()) {
				if (oldTime > -1.f) {
					setclantag("");
					oldTime = -1.f;
				}

				return;
			}

			if (Source::m_pEngine->IsInGame() && Source::m_pClientState && Source::m_pClientState->m_iChockedCommands <= 0) {
				/*switch (int(Source::m_pGlobalVars->realtime) % 5)
				{
				case 0: setclantag("\u2708__\u258C_\u258C"); break;
				case 1: setclantag("_\u2708_\u258C_\u258C"); break;
				case 2: setclantag("__\u2708\u258C_\u258C"); break;
				case 3: setclantag("___\u2620\u2708\u258C"); break;
				case 4: setclantag("___\u2620_\u2620"); break;
				}*/

				if (current_tick != old_tick)
				{
					oldTime = Source::m_pGlobalVars->realtime;
					old_tick = current_tick;

					Marquee(cur_clantag);

					setclantag(cur_clantag.c_str());
				}
			}
		};

		auto crash_server = [](int stage)->void
		{
			auto sendnetmsg_rebuild = [](uint32_t netchan, void* msg, bool force_reliable = false, bool voice = false) -> bool
			{
				bool is_reliable = force_reliable;
				if (!is_reliable) is_reliable = Memory::VCall<bool(__thiscall*)(void*)>(msg, 6)(msg);

				void* stream = nullptr;
				if (is_reliable)
					stream = (void*)((DWORD)netchan + 0x30);
				else
					stream = (void*)((DWORD)netchan + 0x54);

				if (voice)
					stream = (void*)((DWORD)netchan + 0x78);

				if (stream == nullptr)
					return false;

				return Memory::VCall<bool(__thiscall*)(void*, void*)>(msg, 5)(msg, stream);
			};

			if (stage != FRAME_RENDER_START || !Source::m_pEngine->IsInGame())
				return;
			if (!Source::m_pClientState->m_ptrNetChannel)
				return;

			static auto cnetmsg_file_ctor_offset = follow_rel32<uintptr_t>(Memory::Scan(cheat::main::enginedll, "E8 ? ? ? ? FF 75 08 83 4D E0 01"), 1);
			static auto cnetmsg_file_dtor_offset = follow_rel32<uintptr_t>(Memory::Scan(cheat::main::enginedll, "E8 ? ? ? ? 8B 4D E0 8A 45 0C 83 C9 04 83 C9 08 88 45 D8 8D 46 30 89 4D E0") + 0x30, 1);
			static auto cnetmsg_file_proto_offset = follow_rel32<uintptr_t>(Memory::Scan(cheat::main::enginedll, "E8 ? ? ? ? 8B 4D E0 8A 45 0C 83 C9 04 83 C9 08 88 45 D8 8D 46 30 89 4D E0"), 1);

			if (cnetmsg_file_ctor_offset == 0 ||
				cnetmsg_file_dtor_offset == 0 ||
				cnetmsg_file_proto_offset == 0)
				return;

			static auto constructor = CNETMsg_File_constructor_fn(cnetmsg_file_ctor_offset);
			static auto destructor = CNETMsg_File_destructor_fn(cnetmsg_file_dtor_offset);
			static auto protobuf = CNETMsg_File_proto_fn(cnetmsg_file_proto_offset);

			byte msg[80] = { 0 };
			constructor(msg);
			protobuf(&msg[4], "asdasdasd.mp3");

			auto tStart = std::chrono::high_resolution_clock::now();

			int commands_count = 500;

			for (auto i = 0; i < commands_count; ++i) {
				sendnetmsg_rebuild((uint32_t)Source::m_pClientState->m_ptrNetChannel, msg, false, true);
			}

			auto time_delta = static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds, long long>(std::chrono::high_resolution_clock::now() - tStart).count()) / 1000000000.f;
			char buf[512]; sprintf(buf, "hit server with %d commands. it took %.5f seconds.", commands_count, time_delta);
			_events.push_back(_event(Source::m_pGlobalVars->curtime + 2.f, std::string(buf)));

			bool hasProblem = Source::m_pEngine->GetNetChannelInfo()->IsTimingOut();

			//// Request non delta compression if high packet loss, show warning message
			if (hasProblem)
				_events.push_back(_event(Source::m_pGlobalVars->curtime + 3.f, "server hit succeed! timeout triggered."));
			else
				_events.push_back(_event(Source::m_pGlobalVars->curtime + 1.f, "waiting server response."));

			destructor(msg);
		};

		auto apply_gloves = [](int xuidlow)
		{
			if (!Source::m_pEntList->GetClientEntityFromHandle((CBaseHandle)cheat::main::local()->m_hMyWearables()[0]))
			{
				static ClientClass* pClass;

				if (!pClass)
					pClass = Source::m_pClient->GetAllClasses();
				while (pClass)
				{
					if (pClass->m_ClassID == class_ids::CEconWearable)
						break;
					pClass = pClass->m_pNext;
				}

				int iEntry = Source::m_pEntList->GetHighestEntityIndex() + 1;
				int iSerial = RandomInt(0x0, 0xFFF);

				pClass->m_pCreateFn(iEntry, iSerial);
				cheat::main::local()->m_hMyWearables()[0] = iEntry | (iSerial << 16);

				auto pEnt = Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hMyWearables()[0]);

				const auto glove_type = cheat::Cvars.Misc_skins_glove_type.GetValue();

				if (pEnt && glove_type != 0.f)
				{
					static int modelindex = 0;

					if (glove_type == 1)
						modelindex = Source::m_pModelInfo->GetModelIndex(_("models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound.mdl"));
					else if (glove_type == 2)
						modelindex = Source::m_pModelInfo->GetModelIndex(_("models/weapons/v_models/arms/glove_sporty/v_glove_sporty.mdl"));
					else if (glove_type == 3)
						modelindex = Source::m_pModelInfo->GetModelIndex(_("models/weapons/v_models/arms/glove_slick/v_glove_slick.mdl"));
					else if (glove_type == 4)
						modelindex = Source::m_pModelInfo->GetModelIndex(_("models/weapons/v_models/arms/glove_handwrap_leathery/v_glove_handwrap_leathery.mdl"));
					else if (glove_type == 5)
						modelindex = Source::m_pModelInfo->GetModelIndex(_("models/weapons/v_models/arms/glove_motorcycle/v_glove_motorcycle.mdl"));
					else if (glove_type == 6)
						modelindex = Source::m_pModelInfo->GetModelIndex(_("models/weapons/v_models/arms/glove_specialist/v_glove_specialist.mdl"));

					static int ItemDefinitionIndex = 0;

					if (glove_type == 1)
						ItemDefinitionIndex = 5027;
					else if (glove_type == 2)
						ItemDefinitionIndex = 5030;
					else if (glove_type == 3)
						ItemDefinitionIndex = 5031;
					else if (glove_type == 4)
						ItemDefinitionIndex = 5032;
					else if (glove_type == 5)
						ItemDefinitionIndex = 5033;
					else if (glove_type == 6)
						ItemDefinitionIndex = 5034;
					else if (glove_type == 7)
						ItemDefinitionIndex = 5035;
					else
						ItemDefinitionIndex = 0;

					static int paintkit = 0;

					const auto skin_idx = (int)cheat::Cvars.Misc_skins_glove_skin.GetValue();

					if (glove_type == 1)
					{
						switch (skin_idx)
						{
						case 0:
							paintkit = 10006;
							break;
						case 1:
							paintkit = 10007;
							break;
						case 2:
							paintkit = 10008;
							break;
						case 3:
							paintkit = 10039;
							break;
						}
					}
					else if (glove_type == 2)
					{
						switch (skin_idx)
						{
						case 0:
							paintkit = 10018;
							break;
						case 1:
							paintkit = 10037;
							break;
						case 2:
							paintkit = 10038;
							break;
						case 3:
							paintkit = 10019;
							break;
						case 4:
							paintkit = 10048;
							break;
						case 5:
							paintkit = 10047;
							break;
						case 6:
							paintkit = 10045;
							break;
						case 7:
							paintkit = 10046;
							break;
						}
					}
					else if (glove_type == 3)
					{
						switch (skin_idx)
						{
						case 0:
							paintkit = 10013;
							break;
						case 1:
							paintkit = 10015;
							break;
						case 2:
							paintkit = 10016;
							break;
						case 3:
							paintkit = 10040;
							break;
						case 4:
							paintkit = 10043;
							break;
						case 5:
							paintkit = 10044;
							break;
						case 6:
							paintkit = 10041;
							break;
						case 7:
							paintkit = 10042;
							break;
						}
					}
					else if (glove_type == 4)
					{
						switch (skin_idx)
						{
						case 0:
							paintkit = 10009;
							break;
						case 1:
							paintkit = 10010;
							break;
						case 2:
							paintkit = 10021;
							break;
						case 3:
							paintkit = 10036;
							break;
						case 4:
							paintkit = 10053;
							break;
						case 5:
							paintkit = 10054;
							break;
						case 6:
							paintkit = 10055;
							break;
						case 7:
							paintkit = 10056;
							break;
						}
					}
					else if (glove_type == 5)
					{
						switch (skin_idx)
						{
						case 0:
							paintkit = 10024;
							break;
						case 1:
							paintkit = 10026;
							break;
						case 2:
							paintkit = 10027;
							break;
						case 3:
							paintkit = 10028;
							break;
						case 4:
							paintkit = 10050;
							break;
						case 5:
							paintkit = 10051;
							break;
						case 6:
							paintkit = 10052;
							break;
						case 7:
							paintkit = 10049;
							break;
						}
					}
					else if (glove_type == 6)
					{
						switch (skin_idx)
						{
						case 0:
							paintkit = 10030;
							break;
						case 1:
							paintkit = 10033;
							break;
						case 2:
							paintkit = 10034;
							break;
						case 3:
							paintkit = 10035;
							break;
						case 4:
							paintkit = 10061;
							break;
						case 5:
							paintkit = 10062;
							break;
						case 6:
							paintkit = 10063;
							break;
						case 7:
							paintkit = 10064;
							break;
						}
					}
					else if (glove_type == 7)
					{
						switch (skin_idx)
						{
						case 0:
							paintkit = 10057;
							break;
						case 1:
							paintkit = 10058;
							break;
						case 2:
							paintkit = 10059;
							break;
						case 3:
							paintkit = 10060;
							break;
						}
					}

					auto weapon_ent = (C_WeaponCSBaseGun*)pEnt;

					weapon_ent->m_iItemDefinitionIndex() = ItemDefinitionIndex;
					weapon_ent->m_iItemIDHigh() = -1;
					weapon_ent->m_iEntityQuality() = 4;
					weapon_ent->m_iAccountID() = xuidlow;
					weapon_ent->m_flFallbackWear() = 0.00000001f;
					weapon_ent->m_nFallbackSeed() = 0;
					weapon_ent->m_nFallbackStatTrak() = -1;
					weapon_ent->m_nFallbackPaintKit() = paintkit;

					pEnt->set_model_index(modelindex);
					pEnt->pre_data_update(DATA_UPDATE_CREATED);
				}
			}
		};

		//auto change_skins = [apply_gloves](int stage)->void
		//{
		//	if (!Source::m_pEngine->IsInGame())
		//		return;
		//	if (stage != FRAME_NET_UPDATE_POSTDATAUPDATE_START)
		//		return;
		//	if (!cheat::main::local())
		//		return;
		//	if (cheat::main::local()->m_iHealth() <= 0)
		//		return;

		//	cheat::main::updating_skins = false;

		//	player_info local_info;
		//	if (!Source::m_pEngine->GetPlayerInfo(Source::m_pEngine->GetLocalPlayer(), &local_info))
		//		return;

		//	auto weapons = cheat::main::local()->m_hMyWeapons();
		//	if (!weapons) return;

		//	auto i_weapon = (C_WeaponCSBaseGun*)Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon());

		//	if (!i_weapon) return;

		//	auto viewmodel = Source::m_pEntList->GetClientEntityFromHandle((CBaseHandle)cheat::main::local()->m_hViewModel());
		//	if (!viewmodel) return;

		//	auto worldmodel = Source::m_pEntList->GetClientEntityFromHandle(i_weapon->m_hWeaponWorldModel());
		//	if (!worldmodel) return;

		//	apply_gloves(local_info.xuidlow);

		//	if (!parser::knifes.list.empty() && i_weapon->is_knife() && cheat::Cvars.Skins_Knife.GetValue()/* && parser::default_knifes.get_by_id(m_iItemDefinitionIndex).has_value() && parser::knifes.get_by_id(m_iItemDefinitionIndex).has_value()*/)
		//	{
		//		const auto new_knife_id = parser::knifes.list[((int)cheat::Cvars.Skins_Knife.GetValue() - 1)].id;

		//		auto knife = get_knife_by_id(new_knife_id);

		//		if (knife.has_value())
		//		{
		//			viewmodel->set_model_index(knife->model_player);
		//			worldmodel->set_model_index(knife->model_world);
		//		}
		//	}

		//	bool lol = false;

		//	for (int i = 0; weapons[i] != 0xFFFFFFFF; i++)
		//	{
		//		if (!weapons[i])
		//			continue;

		//		auto weapon = (C_WeaponCSBaseGun*)Source::m_pEntList->GetClientEntityFromHandle(weapons[i]);

		//		if (!weapon)
		//			continue;

		//		auto& m_iItemIDHigh = weapon->m_iItemIDHigh();
		//		auto& m_iItemDefinitionIndex = weapon->m_iItemDefinitionIndex();
		//		auto& m_nFallbackPaintKit = weapon->m_nFallbackPaintKit();
		//		auto& m_iEntityQuality = weapon->m_iEntityQuality();
		//		auto& m_iAccountID = weapon->m_iAccountID();
		//		auto& m_OriginalOwnerXuidLow = weapon->m_OriginalOwnerXuidLow();
		//		auto& m_OriginalOwnerXuidHigh = weapon->m_OriginalOwnerXuidHigh();
		//		//auto& m_szCustomName = weapon->m_szCustomName();

		//		weapon->m_iItemIDHigh() = -1;

		//		weapon->m_iAccountID() = local_info.xuidlow;

		//		if (local_info.xuidhigh != m_OriginalOwnerXuidHigh ||
		//			local_info.xuidlow != m_OriginalOwnerXuidLow)
		//			continue; // not OUR weapon

		//		if (!parser::knifes.list.empty() && weapon->is_knife() && weapon->GetCSWeaponData()->max_clip == -1 && cheat::Cvars.Skins_Knife.GetValue()/* && parser::default_knifes.get_by_id(m_iItemDefinitionIndex).has_value() && parser::knifes.get_by_id(m_iItemDefinitionIndex).has_value()*/)
		//		{
		//			const auto new_knife_id = parser::knifes.list[((int)cheat::Cvars.Skins_Knife.GetValue() - 1)].id;

		//			auto knife = get_knife_by_id(new_knife_id);

		//			if (knife.has_value())
		//			{
		//				if ((int)weapon->m_iItemDefinitionIndex() != (int)new_knife_id) {
		//					weapon->m_iItemDefinitionIndex() = (int)new_knife_id;
		//					weapon->m_iEntityQuality() = 3;
		//				}

		//				if (knife->model_player == -1 || knife->model_world == -1) {
		//					knife->model_world = Source::m_pModelInfo->GetModelIndex(knife->model_world_path.c_str());
		//					knife->model_player = Source::m_pModelInfo->GetModelIndex(knife->model_player_path.c_str());
		//				}

		//				weapon->set_model_index(knife->model_player);
		//			}
		//		}

		//		auto skin = cheat::settings.paint[m_iItemDefinitionIndex];
		//		if (skin != m_nFallbackPaintKit)
		//		{
		//			m_nFallbackPaintKit = skin;
		//			//m_nFallbackSeed = item.Seed;
		//			//m_nFallbackStatTrak = item.StatTrak;
		//			//if (item.StatTrak >= 0) m_iEntityQuality = 9;
		//			//if (item.Name) strcpy(m_szCustomName, item.Name);
		//			lol = true;
		//		}

		//		//weapon->m_nFallbackStatTrak() = cheat::settings.stattrak[cheat::main::local()->get_weapon()->m_iItemDefinitionIndex()];
		//	}

		//	if (lol && cheat::game::update_hud_weapons) 
		//	{
		//		cheat::main::updating_skins = true;

		//		auto ClearHudWeaponIcon = reinterpret_cast<std::int32_t(__thiscall*)(void*, std::int32_t)>(cheat::game::update_hud_weapons);

		//		if (ClearHudWeaponIcon) {
		//			auto dw_hud_weapon_selection = cheat::game::find_hud_element<DWORD>("CCSGO_HudWeaponSelection");
		//			if (dw_hud_weapon_selection)
		//			{
		//				auto hud_weapons = reinterpret_cast<int*>(dw_hud_weapon_selection - 0xA0);
		//				if (hud_weapons && *hud_weapons)
		//				{
		//					auto hud_weapons_count = reinterpret_cast<std::int32_t*>(std::uintptr_t(hud_weapons) + 0x80);

		//					if (hud_weapons_count) {
		//						for (int i = 0; i < *hud_weapons_count; i++)
		//							i = ClearHudWeaponIcon(hud_weapons, i);
		//					}
		//					//hud_weapons = 0;
		//				}
		//			}
		//		}
		//	}
		//};

		auto extend_fakelag_packets = []() -> void
		{
			static bool noob = false;

			if (noob)
				return;

			if (!noob) {
				//DWORD wowfast = 0x100d10bc + 1, old;
				static DWORD lol = Memory::Scan(cheat::main::enginedll, "55 8B EC A1 ? ? ? ? 81 EC ? ? ? ? B9 ? ? ? ? 53 8B 98") + 0xBC + 1;
				DWORD old;

				//printf("%d\n", *(int*)lol);
				VirtualProtect((LPVOID)lol, 1, PAGE_READWRITE, &old);
				*(int*)lol = 62;
				VirtualProtect((LPVOID)lol, 1, old, &old);
				//printf("%d\n", *(int*)lol);

				//BYTE rarefast = 0x3E; // 62
				//VirtualProtect((LPVOID)wowfast, 1, PAGE_READWRITE, &old);
				//WriteProcessMemory(GetCurrentProcess(), (LPVOID)wowfast, &rarefast, sizeof(rarefast), 0);
				//VirtualProtect((LPVOID)wowfast, 1, old, &old);
				noob = true;
			}
		};

		auto get_convars = []() -> void
		{
			static auto tick = -1.f;
			static bool was_connected = false;

			if (Source::m_pEngine->IsInGame() != was_connected) {
				tick = -1.f;
				was_connected = Source::m_pEngine->IsInGame();
			}

			if (Source::m_pGlobalVars->realtime >= tick && was_connected) {
				cheat::convars::weapon_recoil_scale = Source::m_pCvar->FindVar("weapon_recoil_scale")->GetFloat();
				cheat::convars::sv_usercmd_custom_random_seed = Source::m_pCvar->FindVar("sv_usercmd_custom_random_seed")->GetInt();
				cheat::convars::weapon_accuracy_nospread = Source::m_pCvar->FindVar("weapon_accuracy_nospread")->GetInt();

				cheat::convars::sv_clip_penetration_traces_to_players = Source::m_pCvar->FindVar("sv_clip_penetration_traces_to_players")->GetInt();
				cheat::convars::ff_damage_bullet_penetration = Source::m_pCvar->FindVar("ff_damage_bullet_penetration")->GetFloat();
				cheat::convars::ff_damage_reduction_bullets = Source::m_pCvar->FindVar("ff_damage_reduction_bullets")->GetFloat();
				cheat::convars::sv_penetration_type = Source::m_pCvar->FindVar("sv_penetration_type")->GetInt();
				tick = Source::m_pGlobalVars->realtime + 7.f;
			}
		};

		auto render_nightmode = []() -> void
		{
			if (!Source::m_pEngine->IsInGame() || cheat::main::local() == nullptr)
				return;

			static ConVar* mat_force_tonemap_scale = Source::m_pCvar->FindVar("mat_force_tonemap_scale");
			const auto modifier = cheat::Cvars.Visuals_wrld_nightmode.GetValue() ? 0.24f : 1.f;

			if (mat_force_tonemap_scale && mat_force_tonemap_scale->GetFloat() != modifier/*old_map_light != cheat::Cvars.Visuals_wrld_nightmode.GetValue()*/)
			{
				//ConVar* r_drawspecificstaticprop = Source::m_pCvar->FindVar("r_drawspecificstaticprop");
				//
				//if (r_drawspecificstaticprop) {
				//	r_drawspecificstaticprop->m_nFlags &= ~FCVAR_CHEAT;
				//	r_drawspecificstaticprop->SetValue(0);
				//}
				//
				//ConVar* cl_modelfastpath = Source::m_pCvar->FindVar("cl_modelfastpath");
				//if (cl_modelfastpath)
				//	cl_modelfastpath->SetValue(1);
				//
				//float modifier = cheat::Cvars.Visuals_wrld_nightmode.GetValue() ? 0.2f : 1.f;
				//
				///*static auto m_bUseCustomAutoExposureMin = Engine::PropManager::Instance()->GetOffset("DT_EnvTonemapController", "m_bUseCustomAutoExposureMin");
				//static auto m_bUseCustomAutoExposureMax = Engine::PropManager::Instance()->GetOffset("DT_EnvTonemapController", "m_bUseCustomAutoExposureMax");
				//static auto m_flCustomAutoExposureMin = Engine::PropManager::Instance()->GetOffset("DT_EnvTonemapController", "m_flCustomAutoExposureMin");
				//static auto m_flCustomAutoExposureMax = Engine::PropManager::Instance()->GetOffset("DT_EnvTonemapController", "m_flCustomAutoExposureMax");
				//
				//for (auto i = 0; i < 2000; i++)
				//{
				//	auto entity = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(i);
				//
				//	if (!entity ||
				//		entity->IsPlayer() ||
				//		entity->GetClientClass() == nullptr ||
				//		entity->GetClientClass()->m_ClassID != 67
				//		) continue;
				//
				//	*(int*)(uintptr_t(entity) + m_bUseCustomAutoExposureMin) = *(int*)(uintptr_t(entity) + m_bUseCustomAutoExposureMax) = 1;
				//	*(float*)(uintptr_t(entity) + m_flCustomAutoExposureMax) = cheat::settings.visuals_misc_map_modifier;
				//	*(float*)(uintptr_t(entity) + m_flCustomAutoExposureMin) = cheat::settings.visuals_misc_map_modifier;
				//}*/
				//
				//for (auto mathandle = Source::m_pMaterialSystem->FirstMaterial(); mathandle != Source::m_pMaterialSystem->InvalidMaterial(); mathandle = Source::m_pMaterialSystem->NextMaterial(mathandle))
				//{
				//	if (!mathandle)
				//		continue;
				//
				//	auto mat = Source::m_pMaterialSystem->GetMaterial(mathandle);
				//
				//	if (!mat)
				//		continue;
				//
				//	if (std::strstr(mat->GetTextureGroupName(), "StaticProp")) {
				//		mat->ColorModulate(modifier, modifier, modifier);
				//	}
				//
				//	if (std::strstr(mat->GetTextureGroupName(), "World")) {
				//		mat->ColorModulate(modifier, modifier, modifier);
				//		if (!std::strstr(mat->GetName(), "glass"))
				//			mat->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, false);
				//	}
				//
				//	if (std::strstr(mat->GetName(), "glass") || std::strstr(mat->GetName(), "decals") || std::strstr(mat->GetName(), "door"))
				//		mat->ColorModulate(modifier, modifier, modifier);
				//}

				//if (mat_force_tonemap_scale)
				//{
					mat_force_tonemap_scale->SetValue(modifier);
				//}

				//old_map_light = cheat::Cvars.Visuals_wrld_nightmode.GetValue();
			}

			static int lol = 0;

			if (lol != cheat::Cvars.Visuals_world_sky.GetValue())
			{
				static auto sv_skyname = Source::m_pCvar->FindVar("sv_skyname");

				static auto original = sv_skyname->GetString();

				static const char* skyboxes[] = {
					"cs_tibet",
					"cs_baggage_skybox_",
					"embassy",
					"italy",
					"jungle",
					"office",
					"sky_cs15_daylight01_hdr",
					"vertigoblue_hdr",
					"sky_cs15_daylight02_hdr",
					"vertigo",
					"sky_day02_05_hdr",
					"nukeblank",
					"sky_venice",
					"sky_cs15_daylight03_hdr",
					"sky_cs15_daylight04_hdr",
					"sky_csgo_cloudy01",
					"sky_csgo_night02",
					"sky_csgo_night02b",
					"sky_csgo_night_flat",
					"sky_dust",
					"vietnam",
					"amethyst",
					"sky_descent",
					"clear_night_sky",
					"otherworld",
					"cloudynight",
					"dreamyocean",
					"grimmnight",
					"sky051",
					"sky081",
					"sky091",
					"sky561",
				};

				sv_skyname->m_nFlags &= ~FCVAR_CHEAT;

				if (cheat::Cvars.Visuals_world_sky.GetValue())
					sv_skyname->SetValue(skyboxes[(int)cheat::Cvars.Visuals_world_sky.GetValue() - 1]);
				else
					sv_skyname->SetValue(original);
			}

			lol = cheat::Cvars.Visuals_world_sky.GetValue();
		};

		auto render_asus_props = []() -> void
		{
			static bool asuswalls_performed = false;

			static float asus_last_float;

			float asusval = cheat::Cvars.Visuals_wrld_prop_alpha.GetValue();

			if (asusval < 1.f)
				asusval = 1.f;

			if (asus_last_float != asusval)
				asuswalls_performed = false;

			if (!asuswalls_performed) {
				for (auto mathandle = Source::m_pMaterialSystem->FirstMaterial(); mathandle != Source::m_pMaterialSystem->InvalidMaterial(); mathandle = Source::m_pMaterialSystem->NextMaterial(mathandle))
				{
					if (!mathandle)
						continue;

					auto pmat = Source::m_pMaterialSystem->GetMaterial(mathandle);

					if (!pmat)
						continue;

					if (strstr(pmat->GetName(), "crate") ||
						strstr(pmat->GetName(), "box") ||
						strstr(pmat->GetName(), "door")) {
						if (asusval < 100.f) {
							pmat->AlphaModulate((asusval * 0.01f));
							pmat->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, true);
						}
						else {
							pmat->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, false);
							pmat->AlphaModulate(1.0f);
						}
					}

				}

				asus_last_float = asusval;
				asuswalls_performed = true;
			}
		}; 

		QAngle aim, view;

		if (Source::m_pClientState != nullptr) {
			if (Source::m_pNetChannelSwap && Source::m_pNetChannelSwap->hooked) {
				if (Source::m_pClientState->m_ptrNetChannel != nullptr) {
					uintptr_t* vtable = *(uintptr_t**)Source::m_pClientState->m_ptrNetChannel;

					if (vtable != Source::m_pNetChannelSwap->m_pRestore) {
						Source::m_pNetChannelSwap.reset();
					}
				}
				else
					Source::m_pNetChannelSwap.reset();
			}
		}

		/*if (Source::m_pClientState != nullptr && (CClientState*)(uint32_t(Source::m_pClientState) + 8) != nullptr) {
			if (Source::m_pClientStateSwap != nullptr && Source::m_pClientStateSwap->hooked) {
				uintptr_t* vtable = *(uintptr_t**)(uint32_t(Source::m_pClientState) + 8);

				if (vtable != Source::m_pClientStateSwap->m_pRestore) {
					Source::m_pClientStateSwap.reset();
				}
			}
		}*/

		if (!Source::m_pEngine->IsInGame() && strcmp(mapname, ""))
		{
			_events.clear();
			std::memset(cheat::main::history_hit, 0, sizeof(int) * 128);
			cheat::game::hud_death_notice = nullptr;
			cheat::game::update_hud_weapons = nullptr;
			old_map_light = !cheat::Cvars.Visuals_wrld_nightmode.GetValue();
			checked_fakelag = false;
			cheat::features::antiaimbot.enable_delay = 0.f;
			cheat::main::command_numbers.clear();

			cheat::main::shots_hit = 0;
			cheat::main::shots_missed = 0;
			cheat::main::shots = 0;

			for (auto i = 0; i < 64; i++)
			{
				cheat::main::shots_fired[i] = 0;
				cheat::main::shots_total[i] = 0;

				auto resolve_info = &cheat::features::aaa.player_resolver_records[i];

				cheat::features::lagcomp.records[i].reset(true);

				resolve_info->Reset();
				resolve_info->leftrec.reset();
				resolve_info->rightrec.reset();
			}

			//cheat::game::log("reset 1");
			strcpy(mapname, "");
		}

		if (Source::m_pEngine->IsInGame() && stage == FRAME_NET_UPDATE_START && strcmp(mapname, Source::m_pEngine->GetLevelName()) && strlen(Source::m_pEngine->GetLevelName()) > 0)
		{
			_events.clear();
			std::memset(cheat::main::history_hit, 0, sizeof(int) * 128);
			cheat::game::hud_death_notice = nullptr;
			cheat::game::update_hud_weapons = nullptr;
			old_map_light = !cheat::Cvars.Visuals_wrld_nightmode.GetValue();
			checked_fakelag = false;
			cheat::features::antiaimbot.enable_delay = 0.f;
			cheat::main::command_numbers.clear();

			cheat::main::shots_hit = 0;
			cheat::main::shots_missed = 0;
			cheat::main::shots = 0;

			for (auto i = 0; i < 64; i++)
			{
				cheat::main::shots_fired[i] = 0;
				cheat::main::shots_total[i] = 0;

				auto resolve_info = &cheat::features::aaa.player_resolver_records[i];

				cheat::features::lagcomp.records[i].reset(true);

				resolve_info->Reset();
				resolve_info->leftrec.reset();
				resolve_info->rightrec.reset();
			}

			//cheat::game::log("reset 2");
			strcpy(mapname, Source::m_pEngine->GetLevelName());
		}

		auto is_valid = ((stage != FRAME_NET_UPDATE_POSTDATAUPDATE_START && stage >= 0 && stage <= 6) && Source::m_pEngine->IsInGame() && !cheat::main::updating_skins && Source::m_pClientState->m_iDeltaTick != -1 && cheat::main::local() != nullptr && ((int*)(uintptr_t(cheat::main::local()) + 0x64)) != nullptr && *(int*)(uintptr_t(cheat::main::local()) + 0x64) >= 0 && *(int*)(uintptr_t(cheat::main::local()) + 0x64) < 64 && cheat::main::local()->GetClientClass() && !cheat::main::local()->IsDead() && (C_WeaponCSBaseGun*)(Source::m_pEntList->GetClientEntityFromHandle(cheat::main::local()->m_hActiveWeapon())) != nullptr && cheat::main::local()->m_iHealth() > 0);

		cheat::game::last_frame = stage;

		if (cheat::main::local() && Source::m_pEngine->IsConnected() && !cheat::main::updating_skins)
			RunFrame();

		get_convars();
		//cheat::features::music.run();

		if (stage == FRAME_RENDER_START && Source::m_pClientState->m_iDeltaTick > 0 && !cheat::main::updating_skins)
		{
			if (cheat::main::local() != nullptr && Source::m_pEngine->IsInGame())
			{

				if (cheat::game::hud_death_notice == nullptr)
					cheat::game::hud_death_notice = cheat::game::find_hud_element<CCSGO_HudDeathNotice*>("CCSGO_HudDeathNotice");
				if (cheat::game::update_hud_weapons == nullptr)
					cheat::game::update_hud_weapons = (void*)Memory::Scan(cheat::main::clientdll, "55 8B EC 51 53 56 8B 75 08 8B D9 57 6B FE 2C");

				//extend_fakelag_packets();
				//cheat::game::log("extended");
				get_convars();
			}

			clantag_changer();
		}

		//if (Source::m_pClientState->m_iDeltaTick > 0) {

#ifdef NOAUTH
			if (cheat::game::pressed_keys[VK_END])
				crash_server(stage);
#endif // !NOAUTH

		//}

		if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
		{
			if (cheat::main::local() != nullptr)
			{
				//cheat::game::log("flash");
				if (cheat::Cvars.Visuals_rem_flash.GetValue())
					cheat::main::local()->m_flFlashDuration() = 0;
			}
		}


		if (stage == FRAME_RENDER_START && is_valid && !cheat::main::updating_skins)
		{
			//cheat::game::log("fsn local fine");

			//if (cheat::main::local() && !cheat::main::local()->IsDead()) {
			//	cheat::main::local()->set_abs_angles(QAngle(0, cheat::main::real_angle, 0));

				/*auto sixth_overlay = &cheat::main::local()->get_animation_layer(6);

				if (sixth_overlay && cheat::main::fakewalking) {
					sixth_overlay->m_flWeight = sixth_overlay->m_flPlaybackRate = 0.0f;
					cheat::main::local()->m_flPoseParameter()[3] = 0.f;
					cheat::main::local()->m_flPoseParameter()[4] = 0.f;
				}*/
			//}
			
			//cheat::game::log("visibility");

			fix_local_anims();

			if (Source::m_pInput->m_fCameraInThirdPerson)
				cheat::main::local()->UpdateVisibilityAllEntities();

			Source::m_pInput->m_fCameraInThirdPerson = false;

			if (cheat::Cvars.Visuals_rem_punch.GetValue()) {
				aim = cheat::main::local()->m_aimPunchAngle();
				view = cheat::main::local()->m_viewPunchAngle();

				cheat::main::local()->m_aimPunchAngle().Set();
				cheat::main::local()->m_viewPunchAngle().Set();
			}

			//cheat::game::log("tp");

			if (cheat::game::get_key_press((int)cheat::Cvars.Visuals_lp_toggletp.GetValue()) && cheat::Cvars.Visuals_lp_toggletp.GetValue())
				cheat::main::thirdperson = !cheat::main::thirdperson;

			//cheat::game::log("ocall");

			if (Source::m_pClientSwap)
				Source::m_pClientSwap->VCall<void(__thiscall*)(void*, ClientFrameStage_t)>(Index::IBaseClientDLL::FrameStageNotify)(ecx, stage);

			static auto cl_wpn_sway_interp = Source::m_pCvar->FindVar(_("cl_wpn_sway_interp"));
			static auto cl_bob_lower_amt = Source::m_pCvar->FindVar(_("cl_bob_lower_amt"));

			if (cheat::Cvars.RageBot_Enable.GetValue())
			{
				cl_wpn_sway_interp->SetValue(0.f);
				cl_bob_lower_amt->SetValue(0.f);
			}
			/*else
			{
				cl_wpn_sway_interp->SetValue(5.f);
				cl_bob_lower_amt->SetValue(1.6f);
			}*/

			/*for (auto i = 0; i < 64; i++)
			{
				auto entity = (C_BasePlayer*)Source::m_pEntList->GetClientEntity(i);

				if (!entity || !entity->IsPlayer())
					continue;

				if (entity->IsDormant() ||
					entity->m_iHealth() <= 0 ||
					entity->IsDead() ||
					entity->m_iTeamNum() == cheat::main::local()->m_iTeamNum()
					)
					continue;

				if (TIME_TO_TICKS(Source::m_pGlobalVars->curtime - entity->m_flSimulationTime()) >= 6)
					entity->set_abs_origin(entity->m_vecOrigin());
			}*/

			//if (old_viewoffs != -1.f)
			//	cheat::main::local()->m_vecViewOffset().z = old_viewoffs;

			//cheat::game::log("baboom");

			cheat::main::local()->get_render_angles() = QAngle(cheat::features::antiaimbot.visual_real_angle.x, cheat::main::real_angle, cheat::features::antiaimbot.visual_real_angle.z);

			if (cheat::Cvars.Visuals_rem_punch.GetValue()) {
				cheat::main::local()->m_aimPunchAngle() = aim;
				cheat::main::local()->m_viewPunchAngle() = view;
			}

			render_nightmode();
			//cheat::game::log("end nightmode");
		}
		else if (Source::m_pClientSwap)
			Source::m_pClientSwap->VCall<void(__thiscall*)(void*, ClientFrameStage_t)>(Index::IBaseClientDLL::FrameStageNotify)(ecx, stage);

	
		if (cheat::main::local()  && stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
			if (!cheat::main::local()->IsDead())
				std::memcpy(cheat::main::local_server_anim_layers, cheat::main::local()->animation_layers_ptr(), sizeof(C_AnimationLayer) * 13);

		if (cheat::main::local() && Source::m_pEngine->IsInGame() && Source::m_pClientState->m_iDeltaTick > 0)
		{
			if (stage == FRAME_NET_UPDATE_END) {
				/*if (!cheat::main::local()->IsDead()) {
					const auto animstate = cheat::main::local()->get_animation_state();
					if (animstate) {
						cheat::main::local()->set_abs_angles({0.f, animstate->abs_yaw, 0.f});
					}
				}*/
				cheat::features::lagcomp.store_records();
			}
		}
	}
}