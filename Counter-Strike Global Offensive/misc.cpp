#include "misc.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "displacement.hpp"
#include "prop_manager.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include "sdk.hpp"
#include "autowall.hpp"
#include "rmenu.hpp"

#include "thread/threading.h"
#include "thread/shared_mutex.h"

#include <thread>

static Semaphore dispatchSem;
static SharedMutex smtx;

void c_misc::unlock_cvars()
{
	ICvar::Iterator iter(Source::m_pCvar);

	static int nUnhidden = 0;

	for (iter.SetFirst(); iter.IsValid(); iter.Next())
	{
		ConCommandBase *cmd = iter.Get();
		auto &flags = *(int*)((DWORD)cmd + 20);

		if (flags & FCVAR_DEVELOPMENTONLY || flags & FCVAR_HIDDEN)
			nUnhidden++;

		flags &= ~18;
	}

	//Source::m_pCvar->ConsolePrintf("Enabled %d hidden cvars.\n", nUnhidden);
}

void c_misc::unlock_cl_cvars()
{
	if ((int)cheat::Cvars.Misc_AntiUT.GetValue())
		return;

	ICvar::Iterator iter(Source::m_pCvar);

	static int nUnhidden = 0;

	for (iter.SetFirst(); iter.IsValid(); iter.Next())
	{
		ConCommandBase *cmd = iter.Get();
		auto &flags = *(int*)((DWORD)cmd + 20);

		if (flags & FCVAR_CHEAT)
			nUnhidden++;

		flags &= FCVAR_CHEAT;
	}

	//Source::m_pCvar->ConsolePrintf("Unlocked %d cheat cvars.\n", nUnhidden);
}

int c_misc::hitbox_to_hitgroup(int Hitbox)
{
	switch (Hitbox)
	{
	case HITBOX_HEAD:
	case HITBOX_NECK:
		return HITGROUP_HEAD;
	case HITBOX_UPPER_CHEST:
	case HITBOX_CHEST:
	case HITBOX_THORAX:
	case HITBOX_LEFT_UPPER_ARM:
	case HITBOX_RIGHT_UPPER_ARM:
		return HITGROUP_CHEST;
	case HITBOX_PELVIS:
	case HITBOX_LEFT_THIGH:
	case HITBOX_RIGHT_THIGH:
	case HITBOX_BODY:
		return HITGROUP_STOMACH;
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
		return HITGROUP_LEFTLEG;
	case HITBOX_RIGHT_CALF:
	case HITBOX_RIGHT_FOOT:
		return HITGROUP_RIGHTLEG;
	case HITBOX_LEFT_FOREARM:
	case HITBOX_LEFT_HAND:
		return HITGROUP_LEFTARM;
	case HITBOX_RIGHT_FOREARM:
	case HITBOX_RIGHT_HAND:
		return HITGROUP_RIGHTARM;
	default:
		return HITGROUP_STOMACH;
	}
}

bool c_misc::goes_thru_smoke(Vector& start,Vector& end)
{
	typedef bool(__cdecl* GoesThroughSmoke)(Vector, Vector);

	static auto linegoesthrusmoke = Memory::Scan(cheat::main::clientdll, "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0");

	if (!linegoesthrusmoke)
		return false;

	static GoesThroughSmoke goesthru_fn = (GoesThroughSmoke)(linegoesthrusmoke);

	return goesthru_fn(start, end);
}