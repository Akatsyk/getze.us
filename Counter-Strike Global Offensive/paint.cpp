#include "hooked.hpp"
#include "menu.h"
#include "visuals.hpp"
#include "displacement.hpp"
#include "player.hpp"
#include "anti_aimbot.hpp"
#include "lag_compensation.hpp"
#include "rmenu.hpp"
#include "sound_parser.hpp"
#include "angle_resolver.hpp"

using FnPT = void(__thiscall*)(void*, unsigned int, bool, bool);
using FnFOV = float(__stdcall *)();

float shit = 0.f;
bool ss = false;
bool glow_call = false;

namespace Hooked
{
	int ScreenSize2W, ScreenSize2H;

	void __fastcall PaintTraverse(void* ecx, void* edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce)
	{
		if ((std::string(Source::m_pPanel->GetName(vguiPanel)).find("HudZoom") == std::string::npos || !cheat::Cvars.Visuals_rem_scope.GetValue() || !cheat::main::local() || cheat::main::local()->IsDead() || cheat::main::updating_skins) && Source::m_pPanelSwap)
			Source::m_pPanelSwap->VCall<FnPT>(Index::IPanel::PaintTraverse)(ecx, vguiPanel, forceRepaint, allowForce);

		static unsigned int drawPanel = 0;
		if (!drawPanel)
		{
			const char* panelname = Source::m_pPanel->GetName(vguiPanel);

			if (panelname[0] == 'M' && panelname[2] == 't')
				drawPanel = vguiPanel;
		}

		if (vguiPanel != drawPanel || cheat::main::updating_skins)
			return;

		static bool bResChange = false;

		Source::m_pEngine->GetScreenSize(ScreenSize2W, ScreenSize2H);

		if (!bResChange && (ScreenSize2W != cheat::game::screen_size.x
			|| ScreenSize2H != cheat::game::screen_size.y))
		{
			cheat::game::screen_size.x = ScreenSize2W;
			cheat::game::screen_size.y = ScreenSize2H;
			bResChange = true;
		}

		if (bResChange)
		{
			Drawing::CreateFonts();
			bResChange = false;
		}

		if (Source::m_pEngine->IsInGame() && cheat::main::local() && !cheat::main::local()->IsDead() && cheat::Cvars.Visuals_misc_preserve_kills.GetValue()) {
			if (cheat::game::hud_death_notice) {
				float* localDeathNotice = &cheat::game::hud_death_notice->localplayer_lifetime_mod;
				//static auto penis = (Memory::Scan("client_panorama.dll", "E8 ? ? ? ? 66 83 3E"));
				if (localDeathNotice) *localDeathNotice = /*Vars.Visuals.PreserveKillfeed ? */FLT_MAX/*: 1.5f*/;
			}
		}

		//Drawing::DrawPixel(1, 1, Color::Black());
		//Drawing::DrawString(F::ESP, 5, 2, Color::White(), FONT_LEFT, "csgo hvh hack");
		//cheat::features::menu.render();
		//cheat::features::visuals.render();

		static bool* s_bOverridePostProcessingDisable = *(bool**)(Memory::Scan(cheat::main::clientdll, "80 3D ? ? ? ? ? 53 56 57 0F 85") + 0x2);
		*s_bOverridePostProcessingDisable = (Source::m_pEngine->IsInGame() && cheat::main::local() && cheat::Cvars.Visuals_wrld_postprocess.GetValue());
	}

	float __fastcall GetScreenAspectRatio(void *pEcx, void *pEdx, int32_t iWidth, int32_t iHeight)
	{
		static bool lel = true;
		using FnAR = float(__thiscall*)(void*, void*, int32_t, int32_t);
		auto original = Source::m_pEngineSwap->VCall<FnAR>(101)(pEcx, pEdx, iWidth, iHeight);

		if (lel)
		{
			cheat::Cvars.Visuals_misc_screen_aspr.SetValue(((float)cheat::game::screen_size.x / (float)cheat::game::screen_size.y));
			lel = !(((float)cheat::game::screen_size.x / (float)cheat::game::screen_size.y) > 0.f);
		}

		auto mm = cheat::Cvars.Visuals_misc_screen_aspr.GetValue();

		if (mm != 0.f)
			return mm;
		else
			return ((float)cheat::game::screen_size.x / (float)cheat::game::screen_size.y);
	}

	float __stdcall GetViewModelFOV() {
		float fov = Source::m_pClientModeSwap->VCall<FnFOV>(Index::IBaseClientDLL::GetViewModelFOV)();

		if (Source::m_pEngine->IsConnected() && Source::m_pEngine->IsInGame() && cheat::main::local() && !cheat::main::updating_skins) {
			fov = fov + cheat::Cvars.Visuals_misc_fov.GetValue();
		}

		return fov;
	}

	void __stdcall EngineVGUI_Paint(int mode)
	{
		typedef void(__thiscall* Paint_t)(IEngineVGui*, int);
		Source::m_pEngineVGUISwap->VCall<Paint_t>(14)(Source::m_pEngineVGUI, mode);

		typedef void(__thiscall *start_drawing)(void *);
		typedef void(__thiscall *finish_drawing)(void *);

		static start_drawing start_draw = (start_drawing)Memory::Scan("vguimatsurface.dll", "55 8B EC 83 E4 C0 83 EC 38");
		static finish_drawing end_draw = (finish_drawing)Memory::Scan("vguimatsurface.dll", "8B 0D ? ? ? ? 56 C6 05");

		if (mode & 1)
		{
			start_draw(Source::m_pSurface);

			static bool bResChange = false;

			Source::m_pEngine->GetScreenSize(ScreenSize2W, ScreenSize2H);

			if (!bResChange && (ScreenSize2W != cheat::game::screen_size.x
				|| ScreenSize2H != cheat::game::screen_size.y))
			{
				cheat::game::screen_size.x = ScreenSize2W;
				cheat::game::screen_size.y = ScreenSize2H;
				bResChange = true;
			}

			if (bResChange)
				Drawing::CreateFonts();
			
			cheat::features::dormant.start();
			cheat::features::menu.render();
			if (!cheat::main::updating_skins)
				cheat::features::visuals.render(bResChange);
			cheat::menu.UpdateMenu();
			cheat::menu.HandleTopMost();
			cheat::features::dormant.finish();

			if (bResChange)
				bResChange = false;

			end_draw(Source::m_pSurface);
		}
	}

	void __fastcall SceneEnd(void* ecx, void* edx)
	{
		Source::m_pRenderViewSwap->VCall<void(__thiscall*)(void*)>(9)(ecx);

		if (Source::m_pClientState->m_iDeltaTick == -1 || cheat::main::updating_skins)
			return;

		if (cheat::Cvars.Visuals_Enable.GetValue() && (cheat::Cvars.Visuals_glow.GetValue() || cheat::Cvars.Visuals_lglow.GetValue()) && Source::m_pEngine->IsConnected() && Source::m_pEngine->IsInGame() && cheat::main::local() && Engine::Displacement::Data::m_pGlowManager)
		{
			for (auto i = 0; i < Engine::Displacement::Data::m_pGlowManager->GetSize(); i++)
			{
				auto &glowObject = Engine::Displacement::Data::m_pGlowManager->m_GlowObjectDefinitions[i];
				auto entity = reinterpret_cast<C_BasePlayer*>(glowObject.m_pEntity);

				if (!entity || entity->IsDead() || entity->IsDormant())
					continue;

				if (glowObject.IsUnused())
					continue;

				if (!entity->GetClientClass() || entity->GetClientClass()->m_ClassID != class_ids::CCSPlayer)
					continue;

				bool is_local_player = entity == cheat::main::local();
				//bool is_teammate = cheat::main::local()->m_iTeamNum() == entity->m_iTeamNum() && !is_local_player;
				bool is_enemy = cheat::main::local()->m_iTeamNum() != entity->m_iTeamNum() || is_local_player;

				if (!is_local_player && !cheat::Cvars.Visuals_glow.GetValue())
					continue;
				else
					if (is_local_player && (!cheat::Cvars.Visuals_lglow.GetValue() || cheat::features::antiaimbot.no_aas))
						continue;

				if (!is_enemy)
					continue;

				Color color = /*Color().FromHSB(cheat::settings.glow_color[0], cheat::settings.glow_color[2], 1.f)*/cheat::Cvars.Visuals_glow_color.GetColor();

				//if (is_local_player && SETTINGS::main_configs.glow_local_enabled)
				//{
				//	should_draw_glow = true;
				//	color = SETTINGS::main_configs.glow_local_color;
				//	style = SETTINGS::main_configs.glow_local_style;
				//	full_bloom = SETTINGS::main_configs.glow_local_fullbloom_enabled;
				//}

				//if (is_teammate && SETTINGS::main_configs.glow_team_enabled)
				//{
				//	should_draw_glow = true;
				//	color = SETTINGS::main_configs.glow_team_color;
				//	style = SETTINGS::main_configs.glow_team_style;
				//	full_bloom = SETTINGS::main_configs.glow_team_fullbloom_enabled;
				//}

				if (is_local_player)
				{
					if (!cheat::Cvars.Visuals_lglow.GetValue())
						continue;

					glowObject.m_nGlowStyle = (cheat::Cvars.Visuals_lglow.GetValue() - 1);
					color = cheat::Cvars.Visuals_lglow_color.GetColor();
				}
				else
					glowObject.m_nGlowStyle = (cheat::Cvars.Visuals_glow.GetValue() - 1);

				glowObject.m_bFullBloomRender = false;
				glowObject.m_flRed = color.r() / 255.0f;
				glowObject.m_flGreen = color.g() / 255.0f;
				glowObject.m_flBlue = color.b() / 255.0f;
				glowObject.m_flAlpha = color.a() / 255.0f;
				glowObject.m_bRenderWhenOccluded = true;
				glowObject.m_bRenderWhenUnoccluded = false;
			}
		}

		//--- Wireframe Smoke ---//
		static std::vector<const char*> vistasmoke_wireframe =
		{
			"particle/vistasmokev1/vistasmokev1_smokegrenade",
		};

		static std::vector<const char*> vistasmoke_nodraw =
		{
			"particle/vistasmokev1/vistasmokev1_fire",
			"particle/vistasmokev1/vistasmokev1_emods",
			"particle/vistasmokev1/vistasmokev1_emods_impactdust",
		};

		if (cheat::Cvars.Visuals_rem_smoke.GetValue()) {
			for (auto mat_s : vistasmoke_wireframe)
			{
				IMaterial* mat = Source::m_pMaterialSystem->FindMaterial(mat_s, "Other Textures");
				mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true); //wireframe
			}

			for (auto mat_n : vistasmoke_nodraw)
			{
				IMaterial* mat = Source::m_pMaterialSystem->FindMaterial(mat_n, "Other Textures");
				mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
			}
		}

		if (!cheat::main::local() || !Source::m_pEngine->IsInGame() || !cheat::Cvars.Visuals_Enable.GetValue())
			return;

		static auto mat = Source::m_pMaterialSystem->CreateMaterial(true, true, false); //Source::m_pMaterialSystem->FindMaterial("chams", "Model textures");
		//static auto fake_mat = Source::m_pMaterialSystem->FindMaterial("dev/glow_armsrace.vmt", nullptr, false, 0);// "Other textures");

		if (!mat || mat->IsErrorMaterial()) {
			mat = Source::m_pMaterialSystem->CreateMaterial(true, true, false);
			return;
		}

		Color hcolor = cheat::Cvars.Visuals_chams_hidden_color.GetColor();//Color().FromHSB(cheat::settings.hidden_chams_color[0], cheat::settings.hidden_chams_color[2], 1.f);
		float hclr[] = { hcolor.r() / 255.f, hcolor.g() / 255.f, hcolor.b() / 255.f, 1.f };

		Color acolor = cheat::Cvars.Visuals_chams_history_color.GetColor();//Color().FromHSB(cheat::settings.hidden_chams_color[0], cheat::settings.hidden_chams_color[2], 1.f);
		float aclr[] = { acolor.r() / 255.f, acolor.g() / 255.f, acolor.b() / 255.f, 1.f };

		Color vcolor = cheat::Cvars.Visuals_chams_color.GetColor();//Color().FromHSB(cheat::settings.chams_color[0], cheat::settings.chams_color[2], 1.f);
		float vclr[] = { vcolor.r() / 255.f, vcolor.g() / 255.f, vcolor.b() / 255.f, 1.f };

		Color lcolor = cheat::Cvars.Visuals_lchams_color.GetColor();//Color().FromHSB(cheat::settings.chams_color[0], cheat::settings.chams_color[2], 1.f);
		float lclr[] = { lcolor.r() / 255.f, lcolor.g() / 255.f, lcolor.b() / 255.f, 1.f };

		//static std::unordered_map<int, float> history_chams_alpha = {};

		for (auto i = 1; i < 64; i++)
		{
			cheat::main::called_chams_render = false;

			auto entity = Source::m_pEntList->GetClientEntity(i);

			if (!entity || entity->IsDead() || entity->IsDormant() || !entity->IsPlayer())
				continue;

			if (!entity->GetClientClass() || entity->GetClientClass()->m_ClassID != class_ids::CCSPlayer)
				continue;

			bool is_local_player = entity == cheat::main::local() && entity->entindex() == cheat::main::local()->entindex();
			bool is_enemy = cheat::main::local()->m_iTeamNum() != entity->m_iTeamNum() || is_local_player;

			if (!is_enemy && !cheat::Cvars.Visuals_chams_teammates.GetValue())
				continue;

			auto player_record = &cheat::features::lagcomp.records[entity->entindex() - 1];
			auto resolver_record = &cheat::features::aaa.player_resolver_records[entity->entindex() - 1];

			if (!is_local_player && player_record->m_Tickrecords.size() <= 0)
				continue;
			//if (!entity->IsDead() && !entity->IsDormant())
			//{

			//auto &lol = history_chams_alpha[i];

			QAngle ang = { 0,0,0 };

			if (is_local_player)
			{

				if (cheat::Cvars.Visuals_lchams_enabled.GetValue())
				{
					//const auto origin = entity->m_vecOrigin();

					//entity->set_abs_origin(cheat::features::antiaimbot.last_sent_origin);
					mat->IncrementReferenceCount();
					mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);
					Source::m_pRenderView->SetColorModulation(lclr);
					//mat->ColorModulate(lclr[0], lclr[1], lclr[2]);
					Source::m_pModelRender->ForcedMaterialOverride(mat);

					entity->draw_model(0x1, 255);
					Source::m_pModelRender->ForcedMaterialOverride(nullptr);
					//entity->set_abs_origin(origin);
				}

				Source::m_pModelRender->ForcedMaterialOverride(nullptr);

				continue;
			}
			else if (!cheat::Cvars.Visuals_chams_type.GetValue())
				continue;

			/*C_Tickrecord backup;

			if (cheat::Cvars.Visuals_chams_resolve.GetValue()) {
				cheat::features::lagcomp.store_record_data(entity, &backup);

				if (resolver_record->resolving_method != 0)
					cheat::features::lagcomp.apply_record_data(entity, (resolver_record->resolving_method < 0 ? &resolver_record->leftrec : &resolver_record->rightrec));
			}*/

			if (cheat::Cvars.Visuals_chams_hidden.GetValue())
			{
				mat->IncrementReferenceCount();
				mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
				Source::m_pRenderView->SetColorModulation(hclr);
				//mat->ColorModulate(hclr[0], hclr[1], hclr[2]);
				Source::m_pModelRender->ForcedMaterialOverride(mat);

				entity->draw_model(0x1, 255);
				Source::m_pModelRender->ForcedMaterialOverride(nullptr);
			}

			mat->IncrementReferenceCount();
			mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);
			Source::m_pRenderView->SetColorModulation(vclr);

			//mat->ColorModulate(vclr[0], vclr[1], vclr[2]);
			Source::m_pModelRender->ForcedMaterialOverride(mat);
			entity->draw_model(0x1, 255);
			Source::m_pModelRender->ForcedMaterialOverride(nullptr);

			//if (backup.data_filled && cheat::Cvars.Visuals_chams_resolve.GetValue())
			//	cheat::features::lagcomp.apply_record_data(entity, &backup);

			/*if (lol > 0.f && player_record->m_Tickrecords.size() < 16)
				lol -= 1.f;
			else if (lol < 255.f && player_record->m_Tickrecords.size() > 16)
				lol += min(255.f - lol, 1.f);*/

			if (cheat::Cvars.Visuals_chams_type.GetValue() == 2 && player_record->m_Tickrecords.size() > 3 && !cheat::main::local()->IsDead() && !is_local_player)
			{
				C_Tickrecord backup;
				backup.reset();
				cheat::features::lagcomp.store_record_data(entity, &backup);

				auto target_record = &player_record->m_Tickrecords.at(min(13, player_record->m_Tickrecords.size()-1));

				//const auto alpha = Math::clamp(target_record->origin.DistanceSquared(backup.origin) * 0.3f, 0.1f, 255.f);

				if (backup.simulation_time > 0.f && target_record->origin.DistanceSquared(backup.origin) > 16 && !cheat::features::lagcomp.is_time_delta_too_large(target_record)) {

					cheat::features::lagcomp.apply_record_data(entity, target_record);

					//const auto old_blend = Source::m_pRenderView->GetBlend();
					//const auto old_alpha = mat->GetAlphaModulation();

					//mat->AlphaModulate(255.f / alpha);

					mat->IncrementReferenceCount();
					mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
					//Source::m_pRenderView->SetColorModulation(aclr);
					//Source::m_pRenderView->SetBlend(255.f / alpha);
					//mat->ColorModulate(aclr[0], aclr[1], aclr[2]);

					Source::m_pModelRender->ForcedMaterialOverride(mat);
					entity->draw_model(0x1, 255);
					Source::m_pModelRender->ForcedMaterialOverride(nullptr);

					mat->IncrementReferenceCount();
					mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);
					//Source::m_pRenderView->SetColorModulation(aclr);
					//Source::m_pRenderView->SetBlend(255.f / alpha);

					//mat->ColorModulate(aclr[0], aclr[1], aclr[2]);
					Source::m_pModelRender->ForcedMaterialOverride(mat);
					entity->draw_model(0x1, 255);
					Source::m_pModelRender->ForcedMaterialOverride(nullptr);

					cheat::features::lagcomp.apply_record_data(entity, &backup);

					if (backup.data_filled)
						entity->set_abs_origin(backup.abs_origin);
				}
			}
			//}

			Source::m_pModelRender->ForcedMaterialOverride(nullptr);
		}
	}

	using fn = void(__thiscall*)(void*, void*, const void*, const ModelRenderInfo_t&, matrix3x4_t*);
	void __fastcall DrawModelExecute(void* ecx, void* edx, void* ctx, const void* state, const ModelRenderInfo_t& info, matrix3x4_t* matrix) {


		if (!Source::m_pEngine->IsInGame())  //shonax
			return Source::m_pModelRenderSwap->VCall<fn>(21)(ecx, ctx, state, info, matrix);

		if (Source::m_pModelRender->IsForcedMaterialOverride()) //shonax glow
			return Source::m_pModelRenderSwap->VCall<fn>(21)(ecx, ctx, state, info, matrix);


		const bool is_arm = std::strstr(info.pModel->name, "arms");
		const bool is_attachment = std::strstr(info.pModel->name, "weapons/w_");
		const bool is_weapon = !is_arm && std::strstr(info.pModel->name, "weapons/v_");
		const bool is_player = std::strstr(info.pModel->name, "models/player");
		

		if (is_player) {
			const auto model_ent = Source::m_pEntList->GetClientEntity(info.entity_index);
			if (model_ent && model_ent->GetClientClass() && model_ent->GetClientClass()->m_ClassID == class_ids::CCSPlayer) {
				const auto local = cheat::main::local();
				if (model_ent == local) {
					for (int i = 0; i < 128; ++i)
						cheat::features::antiaimbot.last_sent_matrix[i].SetOrigin(local->get_abs_origin() - cheat::main::local_bone_origin_delta[i]);
					return Source::m_pModelRenderSwap->VCall<fn>(21)(ecx, ctx, state, info, cheat::features::antiaimbot.last_sent_matrix);
				}
				else
					return Source::m_pModelRenderSwap->VCall<fn>(21)(ecx, ctx, state, info, matrix);
			}
		}
		Source::m_pModelRenderSwap->VCall<fn>(21)(ecx, ctx, state, info, matrix);
		Source::m_pRenderView->SetBlend(1.f);
		Source::m_pModelRender->ForcedMaterialOverride(nullptr);
		//fix invisible
	}

	/*void __stdcall DrawModelExecute(void* context, void* state, ModelRenderInfo_t& info, matrix3x4_t* pCustomBoneToWorld)
	{*/
		// Source::m_pModelRenderSwap->Restore();

		/*const bool is_contactshadow = std::strstr(info.pModel->name, "contactshadow");
		const bool is_player = !is_contactshadow && std::strstr(info.pModel->name, "models/player");

		if (is_player) {
			const auto model_ent = Source::m_pEntList->GetClientEntity(info.entity_index);
			if (model_ent && model_ent->GetClientClass() && model_ent->GetClientClass()->m_ClassID == class_ids::CCSPlayer && model_ent == cheat::main::local())
				return Source::m_pModelRender->DrawModelExecute(context, state, info, cheat::features::antiaimbot.last_sent_matrix);
		}*/

		// return Source::m_pModelRenderSwap->VCall<void(__thiscall*)(void*, void*, ModelRenderInfo_t&, matrix3x4_t*)>(21)(context, state, info, pCustomBoneToWorld);

		//Source::m_pModelRenderSwap->VCall<int(__stdcall*)(void*, void*, ModelRenderInfo_t&, matrix3x4_t*)>(21)(context, state, info, pCustomBoneToWorld);
		//Source::m_pModelRender->DrawModelExecute(context, state, info, pCustomBoneToWorld);

		//if (!Source::m_pModelRender->IsForcedMaterialOverride() || cheat::Cvars.Visuals_lchams_enabled.GetValue()) 
		//{

			//static auto mat = Source::m_pMaterialSystem->CreateMaterial(true, true, false); //Source::m_pMaterialSystem->FindMaterial("chams", "Model textures");

			//Color lcolor = cheat::Cvars.Visuals_lchams_color.GetColor();
			//float lclr[] = { lcolor.r() / 255.f, lcolor.g() / 255.f, lcolor.b() / 255.f, 1.f };

			//auto modelName = Source::m_pModelInfo->GetModelName(info.pModel);

			//static auto fake_mat = Source::m_pMaterialSystem->FindMaterial("dev/glow_armsrace.vmt", nullptr, false, 0);// "Other textures");

			//if (Source::m_pEngine->IsInGame() && cheat::main::local() && Source::m_pClientState->m_iDeltaTick != -1 && info.pModel)
			//{
			//	auto model_ent = Source::m_pEntList->GetClientEntity(info.entity_index);

			//	if (model_ent && model_ent->GetClientClass() && model_ent->GetClientClass()->m_ClassID == class_ids::CCSPlayer && model_ent == cheat::main::local())
			//	{
			//		Source::m_pRenderView->SetColorModulation(lclr);
			//		Source::m_pModelRender->ForcedMaterialOverride(mat);
			//		if (cheat::Cvars.Visuals_lchams_type.GetValue() == 2)
			//			Source::m_pModelRender->DrawModelExecute(context, state, info, cheat::features::antiaimbot.last_sent_matrix);
			//		else
			//			Source::m_pModelRender->DrawModelExecute(context, state, info, pCustomBoneToWorld);
			//		Source::m_pModelRender->ForcedMaterialOverride(nullptr);
			//	}
			//}

			//int bRet = Source::m_pModelRender->DrawModelExecute(context, state, info, pCustomBoneToWorld);
			//Source::m_pModelRender->ForcedMaterialOverride(NULL);
			//Source::m_pModelRenderSwap->Replace();

			//return bRet;
		// const auto model_ent = Source::m_pEntList->GetClientEntity(info.entity_index);

		//if (model_ent != nullptr /*&& cheat::main::called_chams_render*/ && model_ent->IsDead() && cheat::main::fired_shot._target->entindex() == model_ent->entindex())
		//{
		//	Source::m_pModelRender->DrawModelExecute(context, state, info, cheat::main::fired_shot._matrix);
		//	Source::m_pModelRender->ForcedMaterialOverride(nullptr);
		//}

		/*int bRet = 0;

		if (info.pModel == nullptr || cheat::main::local() == nullptr)
			bRet = Source::m_pModelRender->DrawModelExecute(context, state, info, pCustomBoneToWorld);
		else if (strstr(info.pModel->name, _("models/player")) != nullptr && cheat::Cvars.Visuals_rem_teammates.GetValue() && model_ent != nullptr && model_ent != cheat::main::local() && model_ent->m_iTeamNum() == cheat::main::local()->m_iTeamNum())
			bRet = 0; // skip teammates
		else if (strstr(info.pModel->name, _("sleeve")) != nullptr && cheat::Cvars.Visuals_rem_sleeves.GetValue())
			bRet = 0; // skip sleeves
		else
			bRet = Source::m_pModelRender->DrawModelExecute(context, state, info, pCustomBoneToWorld);
		//Source::m_pModelRender->ForcedMaterialOverride(nullptr);
		Source::m_pModelRenderSwap->Replace();
		return bRet;*/
	// }
}