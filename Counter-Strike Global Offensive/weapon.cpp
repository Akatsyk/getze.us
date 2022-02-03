#include "weapon.hpp"
#include "displacement.hpp"
#include "source.hpp"
#include "prop_manager.hpp"
#include "lag_compensation.hpp"
#include "player.hpp"
#include "aimbot.hpp"

float& C_BaseCombatWeapon::m_flNextPrimaryAttack()
{
	return *( float* )( this + Engine::Displacement::DT_BaseCombatWeapon::m_flNextPrimaryAttack );
}

float& C_BaseCombatWeapon::m_flNextSecondaryAttack()
{
	return *( float* )( this + Engine::Displacement::DT_BaseCombatWeapon::m_flNextSecondaryAttack );
}

CBaseHandle& C_BaseCombatWeapon::m_hOwner()
{
	return *( CBaseHandle* )( this + Engine::Displacement::DT_BaseCombatWeapon::m_hOwner );
}

int& C_BaseCombatWeapon::m_iClip1()
{
	return *( int* )( this + Engine::Displacement::DT_BaseCombatWeapon::m_iClip1 );
}

short& C_BaseCombatWeapon::m_iItemDefinitionIndex()
{
	return *(short* )( this + Engine::Displacement::DT_BaseCombatWeapon::m_iItemDefinitionIndex );
}

int & C_BaseCombatWeapon::m_iItemIDHigh()
{
	static auto m_iItemIDHigh = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_iItemIDHigh");

	return *(int*)((DWORD)this + m_iItemIDHigh);
}

int & C_BaseCombatWeapon::m_nFallbackPaintKit()
{
	static auto m_nFallbackPaintKit = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_nFallbackPaintKit");

	return *(int*)((DWORD)this + m_nFallbackPaintKit);
}

int & C_BaseCombatWeapon::m_nFallbackStatTrak()
{
	static auto m_nFallbackStatTrak = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_nFallbackStatTrak");

	return *(int*)((DWORD)this + m_nFallbackStatTrak);
}

float &C_BaseCombatWeapon::m_flFallbackWear()
{
	static auto m_flFallbackWear = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_flFallbackWear");

	return *(float*)((DWORD)this + m_flFallbackWear);
}

float & C_BaseCombatWeapon::m_nFallbackSeed()
{
	static auto m_nFallbackSeed = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_nFallbackSeed");

	return *(float*)((DWORD)this + m_nFallbackSeed);
}

int & C_BaseCombatWeapon::m_iAccountID()
{
	static auto m_iAccountID = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_iAccountID");

	return *(int*)((DWORD)this + m_iAccountID);
}

int & C_BaseCombatWeapon::m_OriginalOwnerXuidLow()
{
	static auto m_OriginalOwnerXuidLow = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_OriginalOwnerXuidLow");

	return *(int*)((DWORD)this + m_OriginalOwnerXuidLow);
}

int & C_BaseCombatWeapon::m_OriginalOwnerXuidHigh()
{
	static auto m_OriginalOwnerXuidHigh = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_OriginalOwnerXuidHigh");

	return *(int*)((DWORD)this + m_OriginalOwnerXuidHigh);
}

int & C_BaseCombatWeapon::m_iEntityQuality()
{
	static auto m_iEntityQuality = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_iEntityQuality");

	return *(int*)((DWORD)this + m_iEntityQuality);
}

CBaseHandle & C_BaseCombatWeapon::m_hWeaponWorldModel()
{
	static auto m_hWeaponWorldModel = Engine::PropManager::Instance()->GetOffset("DT_BaseCombatWeapon", "m_hWeaponWorldModel");

	return *(CBaseHandle*)((DWORD)this + m_hWeaponWorldModel);
}

void C_BaseCombatWeapon::set_model_index(int index)
{
	typedef void(__thiscall* OriginalFn)(PVOID, int);
	return Memory::VCall<OriginalFn>(this, 75)(this, index);
}

bool C_BaseCombatWeapon::IsGun()
{
	if (!this)
		return false;

	int id = this->m_iItemDefinitionIndex();

	switch (id)
	{
	case weapon_deagle:
	case weapon_elite:
	case weapon_fiveseven:
	case weapon_glock:
	case weapon_ak47:
	case weapon_aug:
	case weapon_awp:
	case weapon_famas:
	case weapon_g3sg1:
	case weapon_galilar:
	case weapon_m249:
	case weapon_m4a4:
	case weapon_mac10:
	case weapon_p90:
	case weapon_mp5:
	case weapon_ump45:
	case weapon_xm1014:
	case weapon_bizon:
	case weapon_mag7:
	case weapon_negev:
	case weapon_sawedoff:
	case weapon_tec9:
	case weapon_taser:
	case weapon_p2000:
	case weapon_mp7:
	case weapon_mp9:
	case weapon_nova:
	case weapon_p250:
	case weapon_scar20:
	case weapon_sg556:
	case weapon_ssg08:
		return true;
	case weapon_knife_ct:
	case weapon_flashbang:
	case weapon_hegrenade:
	case weapon_smokegrenade:
	case weapon_molotov:
	case weapon_decoy:
	case weapon_incgrenade:
	case weapon_c4:
	case weapon_knife_t:
		return false;
	case weapon_m4a1s:
	case weapon_usp:
	case weapon_cz75:
	case weapon_revolver:
		return true;
	default:
		return false;
	}
}

bool C_BaseCombatWeapon::is_default_knife()
{
	return (m_iItemDefinitionIndex() == weapon_knife_ct || m_iItemDefinitionIndex() == weapon_knife_t);
}

void C_BaseCombatWeapon::pre_data_update(int updateType)
{
	if (!this) return;

	PVOID pNetworkable = (PVOID)((DWORD)(this) + 0x8);

	if (!pNetworkable)
		return;

	typedef void(__thiscall* OriginalFn)(PVOID, int);
	return Memory::VCall<OriginalFn>(pNetworkable, 6)(pNetworkable, updateType);
}

void C_BaseCombatWeapon::post_data_update(int updateType)
{
	if (!this) return;

	PVOID pNetworkable = (PVOID)((DWORD)(this) + 0x8);

	if (!pNetworkable)
		return;

	typedef void(__thiscall* OriginalFn)(PVOID, int);
	return Memory::VCall<OriginalFn>(pNetworkable, 7)(pNetworkable, updateType);
}

void C_BaseCombatWeapon::on_data_changed(int updateType)
{
	if (!this) return;

	PVOID pNetworkable = (PVOID)((DWORD)(this) + 0x8);

	if (!pNetworkable)
		return;

	typedef void(__thiscall* OriginalFn)(PVOID, int);
	return Memory::VCall<OriginalFn>(pNetworkable, 5)(pNetworkable, updateType);
}

bool C_BaseCombatWeapon::is_reloading()
{
	return *(bool*)(this + Engine::Displacement::DT_BaseCombatWeapon::m_flNextPrimaryAttack + 0x6D);
}

bool C_BaseCombatWeapon::is_knife()
{
	return (is_default_knife() || m_iItemDefinitionIndex() == weapon_knifegg || m_iItemDefinitionIndex() == weapon_bayonet || m_iItemDefinitionIndex() == weapon_butterfly
		|| m_iItemDefinitionIndex() == weapon_falchion || m_iItemDefinitionIndex() == weapon_flip || m_iItemDefinitionIndex() == weapon_gut
		|| m_iItemDefinitionIndex() == weapon_karambit || m_iItemDefinitionIndex() == weapon_m9bayonet || m_iItemDefinitionIndex() == weapon_pushdagger
		|| m_iItemDefinitionIndex() == weapon_bowie || m_iItemDefinitionIndex() == weapon_huntsman || m_iItemDefinitionIndex() == weapon_ursus
		|| m_iItemDefinitionIndex() == weapon_navaja || m_iItemDefinitionIndex() == weapon_stiletto || m_iItemDefinitionIndex() == weapon_talon);
}
//
//bool is_grenade()
//{
//	return m_iValue == WEAPON_FLASHBANG || m_iValue == WEAPON_HEGRENADE || m_iValue == WEAPON_SMOKEGRENADE
//		|| m_iValue == WEAPON_MOLOTOV || m_iValue == WEAPON_INCGRENADE || m_iValue == WEAPON_DECOY;
//}
//
//bool is_pistol()
//{
//	return m_iValue == WEAPON_DEAGLE || m_iValue == WEAPON_ELITE || m_iValue == WEAPON_FIVESEVEN
//		|| m_iValue == WEAPON_GLOCK || m_iValue == WEAPON_HKP2000 || m_iValue == WEAPON_P250
//		|| m_iValue == WEAPON_TEC9 || m_iValue == WEAPON_USP_SILENCER;
//}

float& C_WeaponCSBaseGun::m_flRecoilIndex()
{
	return *( float* )( this + Engine::Displacement::DT_WeaponCSBase::m_flRecoilIndex );
}

float C_WeaponCSBaseGun::GetSpread()
{
	using Fn = float ( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, 452 )( this );
}

float & C_WeaponCSBaseGun::m_flLastShotTime()
{
	static auto m_fLastShotTime = Engine::PropManager::Instance()->GetOffset("DT_WeaponCSBase", "m_fLastShotTime");

	return *(float*)((DWORD)this + m_fLastShotTime);
}

float C_WeaponCSBaseGun::GetInaccuracy()
{
	using Fn = float ( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, 482 )( this );
}

void C_WeaponCSBaseGun::UpdateAccuracyPenalty()
{
	using Fn = void ( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, 483 )( this );
}

bool &C_WeaponCSBaseGun::m_bPinPulled()
{
	static auto m_bPinPulled = Engine::PropManager::Instance()->GetOffset("DT_BaseCSGrenade", "m_bPinPulled");
	return *(bool*)((DWORD)this + m_bPinPulled);
}

float &C_WeaponCSBaseGun::m_fThrowTime()
{
	static auto m_fThrowTime = Engine::PropManager::Instance()->GetOffset("DT_BaseCSGrenade", "m_fThrowTime");
	return *(float *)((DWORD)this + m_fThrowTime);
}

int & C_WeaponCSBaseGun::m_Activity()
{
	static auto nigger = ((C_BasePlayer*)this)->FindInDataMap(((C_BasePlayer*)this)->GetPredDescMap(), "m_Activity");
	return *(int*)((DWORD)this + nigger);
}

bool C_WeaponCSBaseGun::IsBeingThrowed(CUserCmd* cmd)
{
	if (!m_bPinPulled()) {
		float throwTime = m_fThrowTime();

		if (throwTime > 0.f)
			return true;
	}

	if ((cmd->buttons & IN_ATTACK) || (cmd->buttons & IN_ATTACK2)) {
		if (m_fThrowTime() > 0.f)
			return true;
	}

	return false;
}

float& C_WeaponCSBaseGun::m_flThrowStrength()
{
	static auto m_flThrowStrength = Engine::PropManager::Instance()->GetOffset("DT_BaseCSGrenade", "m_flThrowStrength");
	return *(float*)((DWORD)this + m_flThrowStrength);
}

weapon_info* C_WeaponCSBaseGun::GetCSWeaponData()
{
	using Fn = weapon_info*(__thiscall*)(void*);
	return Memory::VCall<Fn>(this, 460 )(this);
}

bool C_WeaponCSBaseGun::IsFireTime(bool acc)
{
	if (m_iItemDefinitionIndex() == weapon_revolver) {
		//int diff = cheat::main::local()->m_nTickBase() - TIME_TO_TICKS(m_flPostponeFireReadyTime());

		//if (diff > -1)
		//	return true;

		if (m_flPostponeFireReadyTime() < Source::m_pGlobalVars->curtime && m_Activity() == 208)
		{
			if (IsSecondaryFireTime())
				return 1;
		}
		return 0;
	}

	return (Source::m_pGlobalVars->curtime >= m_flNextPrimaryAttack() || IsSecondaryFireTime());

	//const auto isr8 = (m_iItemDefinitionIndex() == weapon_revolver);

	//if (isr8 && acc) {
	//	//if (acc) {
	//	if (m_flPostponeFireReadyTime() < Source::m_pGlobalVars->curtime)
	//	{
	//		if (IsSecondaryFireTime())
	//			return true;
	//	}

	//	return false;
	//	//}
	//	//else
	//	//	return cheat::features::aimbot.unk_3CCA9A51;
	//}

	//if (Source::m_pGlobalVars->curtime < cheat::main::local()->m_flNextAttack())
	//	return false;

	//if (Source::m_pGlobalVars->curtime < m_flNextPrimaryAttack())
	//	return false;

	////if (IsSecondaryFireTime())
	////	return true;

	//if (isr8)
	//	return cheat::features::aimbot.unk_3CCA9A51;
	//else
	//	return ( Source::m_pGlobalVars->curtime >= m_flNextPrimaryAttack() || IsSecondaryFireTime());
}

int& C_WeaponCSBaseGun::m_weaponMode()
{
	static auto m_weaponMode = Engine::PropManager::Instance()->GetOffset("DT_WeaponCSBase", "m_weaponMode");
	return *(int*)((DWORD)this + m_weaponMode);
}

int& C_WeaponCSBaseGun::m_zoomLevel()
{
	static auto m_zoomLevel = Engine::PropManager::Instance()->GetOffset("DT_WeaponCSBaseGun", "m_zoomLevel");
	return *(int*)((DWORD)this + m_zoomLevel);
}

float& C_WeaponCSBaseGun::m_flPostponeFireReadyTime()
{
	static auto m_flPostponeFireReadyTime = Engine::PropManager::Instance()->GetOffset("DT_WeaponCSBase", "m_flPostponeFireReadyTime");
	return *(float*)((DWORD)this + m_flPostponeFireReadyTime);
}

float& C_WeaponCSBaseGun::m_fAccuracyPenalty()
{
	static auto m_fAccuracyPenalty = Engine::PropManager::Instance()->GetOffset("DT_WeaponCSBase", "m_fAccuracyPenalty");
	return *(float*)((DWORD)this + m_fAccuracyPenalty);
}

float C_WeaponCSBaseGun::GetMaxWeaponSpeed() {
	if (!this || !GetCSWeaponData())
		return 250.0f;

	if (m_weaponMode() == 0)
		return GetCSWeaponData()->max_speed;

	return GetCSWeaponData()->max_speed_alt;
}

bool C_WeaponCSBaseGun::IsSecondaryFireTime()
{
	return ( Source::m_pGlobalVars->curtime >= m_flNextSecondaryAttack() );
}

bool C_WeaponCSBaseGun::IsSniper()
{
	if (!this)
		return false;

	auto id = m_iItemDefinitionIndex();

	if (id == weapon_awp || id == weapon_ssg08 || id == weapon_scar20 || id == weapon_g3sg1)
		return true;
	else
		return false;
}

bool C_WeaponCSBaseGun::IsRifle()
{
	if (!this)
		return false;

	auto id = m_iItemDefinitionIndex();

	if (id == weapon_ak47 || id == weapon_aug || id == weapon_famas || id == weapon_galilar || id == weapon_m4a1s || id == weapon_m4a4 || id == weapon_m249 || id == weapon_negev || id == weapon_sg556 || id == weapon_galilar || id == weapon_galilar)
		return true;
	else
		return false;
}

bool C_WeaponCSBaseGun::IsSmg()
{
	if (!this)
		return false;

	auto id = m_iItemDefinitionIndex();

	if (id == weapon_mp5 || id == weapon_mp7 || id == weapon_mp9 || id == weapon_p90 || id == weapon_bizon || id == weapon_ump45 || id == weapon_mac10)
		return true;
	else
		return false;
}

bool C_WeaponCSBaseGun::IsShotgun()
{
	if (!this)
		return false;

	auto id = m_iItemDefinitionIndex();

	if (id == weapon_xm1014 || id == weapon_sawedoff || id == weapon_nova|| id == weapon_mag7)
		return true;
	else
		return false;
}

bool C_WeaponCSBaseGun::IsPistol()
{
	if (!this)
		return false;

	auto id = m_iItemDefinitionIndex();

	if (id == weapon_deagle || id == weapon_revolver || id == weapon_tec9 || id == weapon_elite || id == weapon_fiveseven || id == weapon_glock || id == weapon_usp || id == weapon_negev || id == weapon_cz75 || id == weapon_p2000 || id == weapon_p250)
		return true;
	else
		return false;
}

int C_WeaponCSBaseGun::GetWeaponType()
{
	//pistols aim accuracy","smgs aim accuracy", "shotguns aim accuracy","rifles aim accuracy","snipers aim accuracy
	
	int ret = 1;

	if (IsSmg())
		ret = 2;
	else if (IsShotgun())
		ret = 3;
	else if (IsRifle())
		ret = 4;
	else if (IsSniper())
		ret = 5;

	return ret;
}