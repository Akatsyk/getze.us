#pragma once



#define DAMAGE_NO		0
#define DAMAGE_EVENTS_ONLY	1	
#define DAMAGE_YES		2
#define DAMAGE_AIM		3

enum TextureCharacters {
	CHAR_TEX_ANTLION = 'A',
	CHAR_TEX_BLOODYFLESH = 'B',
	CHAR_TEX_CONCRETE = 'C',
	CHAR_TEX_DIRT = 'D',
	CHAR_TEX_EGGSHELL = 'E',
	CHAR_TEX_FLESH = 'F',
	CHAR_TEX_GRATE = 'G',
	CHAR_TEX_ALIENFLESH = 'H',
	CHAR_TEX_CLIP = 'I',
	CHAR_TEX_PLASTIC = 'L',
	CHAR_TEX_METAL = 'M',
	CHAR_TEX_SAND = 'N',
	CHAR_TEX_FOLIAGE = 'O',
	CHAR_TEX_COMPUTER = 'P',
	CHAR_TEX_SLOSH = 'S',
	CHAR_TEX_TILE = 'T',
	CHAR_TEX_CARDBOARD = 'U',
	CHAR_TEX_VENT = 'V',
	CHAR_TEX_WOOD = 'W',
	CHAR_TEX_GLASS = 'Y',
	CHAR_TEX_WARPSHIELD = 'Z',
};

#define CHAR_TEX_STEAM_PIPE		11

enum Hitgroups {
	HITGROUP_INVALID = -1,
	HITGROUP_GENERIC,
	HITGROUP_HEAD,
	HITGROUP_CHEST,
	HITGROUP_STOMACH,
	HITGROUP_LEFTARM,
	HITGROUP_RIGHTARM,
	HITGROUP_LEFTLEG,
	HITGROUP_RIGHTLEG,
	HITGROUP_GEAR = 10
};

extern bool usingpanorama;

class c_autowall
{
public:
	//	struct Autowall_Return_Info
	//	{
	//		int damage;
	//		int hitgroup;
	//		int penetration_count;
	//		bool did_penetrate_wall;
	//		float thickness;
	//		Vector end;
	//		CBaseEntity* hit_entity;
	//	};
	//
	//	Autowall_Return_Info CalculateDamage(Vector start, Vector end, CBaseEntity* from_entity = nullptr, CBaseEntity* to_entity = nullptr, int specific_hitgroup = -1);
	//	float GetThickness(Vector start, Vector end);
	//
	//	inline bool IsAutowalling() const
	//	{
	//		return is_autowalling;
	//	}
	//
	//private:
	//	bool is_autowalling = false;
	//
	//	struct Autowall_Info
	//	{
	//		Vector start;
	//		Vector end;
	//		Vector current_position;
	//		Vector direction;
	//
	//		ITraceFilter* filter;
	//		trace_t enter_trace;
	//
	//		float thickness;
	//		float current_damage;
	//		int penetration_count;
	//	};
	//
	//	bool CanPenetrate(CBaseEntity* attacker, Autowall_Info& info, WeaponInfo_t* weapon_data);
	//
	//	void ScaleDamage(CBaseEntity* entity, WeaponInfo_t* weapon_info, int hitgroup, float& current_damage);
	//
	//	void UTIL_ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter* filter, trace_t* tr)
	//	{
	//		static DWORD dwAddress = U::FindPattern3((usingpanorama ? "client_panorama.dll" : "client.dll"), (BYTE*)"\x53\x8B\xDC\x83\xEC\x08\x83\xE4\xF0\x83\xC4\x04\x55\x8B\x6B\x04\x89\x6C\x24\x04\x8B\xEC\x81\xEC\x00\x00\x00\x00\x8B\x43\x10", "xxxxxxxxxxxxxxxxxxxxxxxx????xxx");
	//		if (!dwAddress)
	//			return;
	//
	//		_asm
	//		{
	//			MOV EAX, filter
	//			LEA ECX, tr
	//			PUSH ECX
	//			PUSH EAX
	//			PUSH mask
	//			LEA EDX, vecAbsEnd
	//			LEA ECX, vecAbsStart
	//			CALL dwAddress
	//			ADD ESP, 0xC
	//		}
	//	}
	//
	//	void GetBulletTypeParameters(float& maxRange, float& maxDistance)
	//	{
	//		maxRange = 35.0;
	//		maxDistance = 3000.0;
	//	}
	//
	//	bool IsBreakableEntity(CBaseEntity* entity)
	//	{
	//		if (!entity || !entity->GetIndex())
	//			return false;
	//
	//		//m_takeDamage isn't properly set when using the signature.
	//		//Back it up, set it to true, then restore.
	//		int takeDamageBackup = *reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C);
	//
	//		auto pClass = entity->GetClientClass();
	//		if (!pClass)
	//			return false;
	//
	//		//				 '       ''     '      '   '
	//		//			    01234567890123456     012345678
	//		//Check against CBreakableSurface and CBaseDoor
	//
	//		//Windows etc. are CBrekableSurface
	//		//Large garage door in Office is CBaseDoor and it get's reported as a breakable when it is not one
	//		//This is seperate from "CPropDoorRotating", which is a normal door.
	//		//Normally you would also check for "CFuncBrush" but it was acting oddly so I removed it. It's below if interested.
	//		//((clientClass->m_pNetworkName[1]) != 'F' || (clientClass->m_pNetworkName[4]) != 'c' || (clientClass->m_pNetworkName[5]) != 'B' || (clientClass->m_pNetworkName[9]) != 'h')
	//
	//		if ((pClass->m_pNetworkName[1] == 'B' && pClass->m_pNetworkName[9] == 'e' && pClass->m_pNetworkName[10] == 'S' && pClass->m_pNetworkName[16] == 'e')
	//			|| (pClass->m_pNetworkName[1] != 'B' || pClass->m_pNetworkName[5] != 'D'))
	//			*reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C) = 2;
	//
	//		typedef bool(__thiscall* IsBreakbaleEntity_Fn)(CBaseEntity*);
	//		static IsBreakbaleEntity_Fn is_breakable_entity = (IsBreakbaleEntity_Fn)U::FindPattern3((usingpanorama ? "client_panorama.dll" : "client.dll"), (PBYTE)"\x55\x8B\xEC\x51\x56\x8B\xF1\x85\xF6\x74\x68", "xxxxxxxxxxx");
	//
	//		bool breakable = is_breakable_entity(entity);
	//		*reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C) = takeDamageBackup;
	//
	//		return breakable;
	//	}
	//
	//	bool TraceToExit(trace_t& enterTrace, trace_t& exitTrace, Vector startPosition, Vector direction);
	//	bool HandleBulletPenetration(WeaponInfo_t* weaponData, trace_t& enterTrace, Vector& eyePosition, Vector direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, float& current_thickness);
	void TraceLine(Vector & absStart, Vector & absEnd, unsigned int mask, C_BasePlayer * ignore, CGameTrace * ptr);
	//void ClipTraceToPlayers(Vector & absStart, Vector absEnd, unsigned int mask, ITraceFilter * filter, CGameTrace * tr);
	void GetBulletTypeParameters(float & maxRange, float & maxDistance, char * bulletType, bool sv_penetration_type);
	bool IsBreakableEntity(C_BasePlayer * entity);
	void ScaleDamage(CGameTrace & enterTrace, weapon_info * weaponData, float & currentDamage);
	//bool TraceToExit(trace_t & enterTrace, trace_t& exitTrace, Vector startPosition, Vector direction);
	bool TraceToExit(trace_t & enter_trace, trace_t & exit_trace, const Vector start_position, const Vector direction, const bool is_local);
	bool HandleBulletPenetration(weapon_info * weaponData, trace_t & enterTrace, Vector &eyePosition, Vector direction, int & possibleHitsRemaining, float & currentDamage, float penetrationPower, float shit, bool pskip = false);
	void FixTraceRay(Vector end, Vector start, trace_t * oldtrace, C_BasePlayer * ent);
	void ClipTraceToPlayers(const Vector & vecAbsStart, const Vector & vecAbsEnd, uint32_t mask, ITraceFilter * filter, trace_t * tr);
	bool FireBullet(Vector eyepos, C_WeaponCSBaseGun * pWeapon, Vector & direction, float & currentDamage, C_BasePlayer * ignore, C_BasePlayer * to_who = nullptr, int target_hitbox = 0);
	float CanHit(Vector & vecEyePos, Vector & point);
	float CanHit(Vector & vecEyePos, Vector & point, C_BasePlayer * ignore_ent, C_BasePlayer * start_ent, int target_hitbox);
	/*float ScaleDamage(C_BasePlayer * entity, float damage, float weaponArmorRatio, int hitgroup);
	bool IsBreakable(C_BasePlayer * entity);
	void UTIL_ClipTraceToPlayers(const Vector & start, const Vector & dir, unsigned int mask, ITraceFilter * filter, trace_t * trace, float minrange);
	bool TraceToExit(Vector & end, Vector start, Vector dir, trace_t * enterTrace, trace_t * exitTrace);
	bool FireBulletEmulated(Vector viewDirection, int minimalDamage, C_BasePlayer * entityToHit, Vector startpos = Vector::Zero, C_BasePlayer* entityfrom = nullptr*///);
};

//#pragma once
//
//
//
//#define DAMAGE_NO		0
//#define DAMAGE_EVENTS_ONLY	1	
//#define DAMAGE_YES		2
//#define DAMAGE_AIM		3
//
//#define CHAR_TEX_ANTLION		'A'
//#define CHAR_TEX_BLOODYFLESH	'B'
//#define	CHAR_TEX_CONCRETE		'C'
//#define CHAR_TEX_DIRT			'D'
//#define CHAR_TEX_EGGSHELL		'E' ///< the egg sacs in the tunnels in ep2.
//#define CHAR_TEX_FLESH			'F'
//#define CHAR_TEX_GRATE			'G'
//#define CHAR_TEX_ALIENFLESH		'H'
//#define CHAR_TEX_CLIP			'I'
////#define CHAR_TEX_UNUSED		'J'
//#define CHAR_TEX_SNOW			'K'
//#define CHAR_TEX_PLASTIC		'L'
//#define CHAR_TEX_METAL			'M'
//#define CHAR_TEX_SAND			'N'
//#define CHAR_TEX_FOLIAGE		'O'
//#define CHAR_TEX_COMPUTER		'P'
////#define CHAR_TEX_UNUSED		'Q'
//#define CHAR_TEX_REFLECTIVE		'R'
//#define CHAR_TEX_SLOSH			'S'
//#define CHAR_TEX_TILE			'T'
//#define CHAR_TEX_CARDBOARD		'U'
//#define CHAR_TEX_VENT			'V'
//#define CHAR_TEX_WOOD			'W'
////#define CHAR_TEX_UNUSED		'X' ///< do not use - "fake" materials use this (ladders, wading, clips, etc)
//#define CHAR_TEX_GLASS			'Y'
//#define CHAR_TEX_WARPSHIELD		'Z' ///< wierd-looking jello effect for advisor shield.
//
//#define CHAR_TEX_STEAM_PIPE		11
//
//#define	HITGROUP_GENERIC	0
//#define	HITGROUP_HEAD		1
//#define	HITGROUP_CHEST		2
//#define	HITGROUP_STOMACH	3
//#define HITGROUP_LEFTARM	4	
//#define HITGROUP_RIGHTARM	5
//#define HITGROUP_LEFTLEG	6
//#define HITGROUP_RIGHTLEG	7
//#define HITGROUP_GEAR		10			// alerts NPC, but doesn't do damage or bleed (1/100th damage)
//
//extern bool usingpanorama;
//
//class c_autowall
//{
//public:
//	//	struct Autowall_Return_Info
//	//	{
//	//		int damage;
//	//		int hitgroup;
//	//		int penetration_count;
//	//		bool did_penetrate_wall;
//	//		float thickness;
//	//		Vector end;
//	//		CBaseEntity* hit_entity;
//	//	};
//	//
//	//	Autowall_Return_Info CalculateDamage(Vector start, Vector end, CBaseEntity* from_entity = nullptr, CBaseEntity* to_entity = nullptr, int specific_hitgroup = -1);
//	//	float GetThickness(Vector start, Vector end);
//	//
//	//	inline bool IsAutowalling() const
//	//	{
//	//		return is_autowalling;
//	//	}
//	//
//	//private:
//	//	bool is_autowalling = false;
//	//
//	//	struct Autowall_Info
//	//	{
//	//		Vector start;
//	//		Vector end;
//	//		Vector current_position;
//	//		Vector direction;
//	//
//	//		ITraceFilter* filter;
//	//		trace_t enter_trace;
//	//
//	//		float thickness;
//	//		float current_damage;
//	//		int penetration_count;
//	//	};
//	//
//	//	bool CanPenetrate(CBaseEntity* attacker, Autowall_Info& info, WeaponInfo_t* weapon_data);
//	//
//	//	void ScaleDamage(CBaseEntity* entity, WeaponInfo_t* weapon_info, int hitgroup, float& current_damage);
//	//
//	//	void UTIL_ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter* filter, trace_t* tr)
//	//	{
//	//		static DWORD dwAddress = U::FindPattern3((usingpanorama ? "client_panorama.dll" : "client.dll"), (BYTE*)"\x53\x8B\xDC\x83\xEC\x08\x83\xE4\xF0\x83\xC4\x04\x55\x8B\x6B\x04\x89\x6C\x24\x04\x8B\xEC\x81\xEC\x00\x00\x00\x00\x8B\x43\x10", "xxxxxxxxxxxxxxxxxxxxxxxx????xxx");
//	//		if (!dwAddress)
//	//			return;
//	//
//	//		_asm
//	//		{
//	//			MOV EAX, filter
//	//			LEA ECX, tr
//	//			PUSH ECX
//	//			PUSH EAX
//	//			PUSH mask
//	//			LEA EDX, vecAbsEnd
//	//			LEA ECX, vecAbsStart
//	//			CALL dwAddress
//	//			ADD ESP, 0xC
//	//		}
//	//	}
//	//
//	//	void GetBulletTypeParameters(float& maxRange, float& maxDistance)
//	//	{
//	//		maxRange = 35.0;
//	//		maxDistance = 3000.0;
//	//	}
//	//
//	//	bool IsBreakableEntity(CBaseEntity* entity)
//	//	{
//	//		if (!entity || !entity->GetIndex())
//	//			return false;
//	//
//	//		//m_takeDamage isn't properly set when using the signature.
//	//		//Back it up, set it to true, then restore.
//	//		int takeDamageBackup = *reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C);
//	//
//	//		auto pClass = entity->GetClientClass();
//	//		if (!pClass)
//	//			return false;
//	//
//	//		//				 '       ''     '      '   '
//	//		//			    01234567890123456     012345678
//	//		//Check against CBreakableSurface and CBaseDoor
//	//
//	//		//Windows etc. are CBrekableSurface
//	//		//Large garage door in Office is CBaseDoor and it get's reported as a breakable when it is not one
//	//		//This is seperate from "CPropDoorRotating", which is a normal door.
//	//		//Normally you would also check for "CFuncBrush" but it was acting oddly so I removed it. It's below if interested.
//	//		//((clientClass->m_pNetworkName[1]) != 'F' || (clientClass->m_pNetworkName[4]) != 'c' || (clientClass->m_pNetworkName[5]) != 'B' || (clientClass->m_pNetworkName[9]) != 'h')
//	//
//	//		if ((pClass->m_pNetworkName[1] == 'B' && pClass->m_pNetworkName[9] == 'e' && pClass->m_pNetworkName[10] == 'S' && pClass->m_pNetworkName[16] == 'e')
//	//			|| (pClass->m_pNetworkName[1] != 'B' || pClass->m_pNetworkName[5] != 'D'))
//	//			*reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C) = 2;
//	//
//	//		typedef bool(__thiscall* IsBreakbaleEntity_Fn)(CBaseEntity*);
//	//		static IsBreakbaleEntity_Fn is_breakable_entity = (IsBreakbaleEntity_Fn)U::FindPattern3((usingpanorama ? "client_panorama.dll" : "client.dll"), (PBYTE)"\x55\x8B\xEC\x51\x56\x8B\xF1\x85\xF6\x74\x68", "xxxxxxxxxxx");
//	//
//	//		bool breakable = is_breakable_entity(entity);
//	//		*reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C) = takeDamageBackup;
//	//
//	//		return breakable;
//	//	}
//	//
//	//	bool TraceToExit(trace_t& enterTrace, trace_t& exitTrace, Vector startPosition, Vector direction);
//	//	bool HandleBulletPenetration(WeaponInfo_t* weaponData, trace_t& enterTrace, Vector& eyePosition, Vector direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, float& current_thickness);
//	/*void TraceLine(Vector & absStart, Vector & absEnd, unsigned int mask, C_BasePlayer * ignore, CGameTrace * ptr);
//	void ClipTraceToPlayers(Vector & absStart, Vector absEnd, unsigned int mask, ITraceFilter * filter, CGameTrace * tr);
//	void GetBulletTypeParameters(float & maxRange, float & maxDistance, char * bulletType, bool sv_penetration_type);
//	bool IsBreakableEntity(C_BasePlayer * entity);
//	void ScaleDamage(CGameTrace & enterTrace, weapon_info * weaponData, float & currentDamage);
//	bool TraceToExit(trace_t & enterTrace, trace_t & exitTrace, Vector startPosition, Vector direction);
//	bool HandleBulletPenetration(weapon_info * weaponData, trace_t & enterTrace, Vector & eyePosition, Vector direction, int & possibleHitsRemaining, float & currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration);
//	bool FireBullet(Vector eyepos, C_WeaponCSBaseGun * pWeapon, Vector & direction, float & currentDamage, C_BasePlayer * ignore, C_BasePlayer * to_who = nullptr);
//	float CanHit(Vector & vecEyePos, Vector & point);
//	float CanHit(Vector & vecEyePos, Vector & point, C_BasePlayer * ignore_ent, C_BasePlayer * start_ent);*/
//
//	struct FireBulletData {
//		Vector m_vecStart;
//		Vector m_vecDirection;
//
//		trace_t m_EnterTrace;
//
//		float m_flMaxLength = 0.0f;
//		float m_flTraceLength;
//		float m_flTraceLengthRemaining;
//		float m_flCurrentDamage;
//		int m_iPenetrationCount;
//
//		C_BasePlayer* m_TargetPlayer;
//		C_BasePlayer* m_Player;
//		weapon_info* m_WeaponData;
//
//		C_BaseCombatWeapon* weapon;
//
//		surfacedata_t* m_EnterSurfaceData;
//
//		bool m_bPenetration = false;
//	};
//
//private:
//	bool TraceToExit(Vector& end, trace_t* enter_trace, Vector start, Vector direction, trace_t* exit_trace);
//
//	bool HandleBulletPenetration(FireBulletData& data);
//
//public:
//	bool IsBreakable(C_BaseEntity * entity);
//	float FireBullets(FireBulletData& data);
//	float CanHit(Vector start_point, Vector end_point);
//	float CanHit(Vector start_point, Vector end_point, C_BasePlayer * start_ent, C_BasePlayer * end_ent);
//	/*float ScaleDamage(C_BasePlayer * entity, float damage, float weaponArmorRatio, int hitgroup);
//	bool IsBreakable(C_BasePlayer * entity);
//	void UTIL_ClipTraceToPlayers(const Vector & start, const Vector & dir, unsigned int mask, ITraceFilter * filter, trace_t * trace, float minrange);
//	bool TraceToExit(Vector & end, Vector start, Vector dir, trace_t * enterTrace, trace_t * exitTrace);
//	bool FireBulletEmulated(Vector viewDirection, int minimalDamage, C_BasePlayer * entityToHit, Vector startpos = Vector::Zero, C_BasePlayer* entityfrom = nullptr*///);
//};