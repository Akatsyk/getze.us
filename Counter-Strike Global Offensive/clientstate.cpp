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

namespace Hooked
{
	void __fastcall PacketStart(void* ecx, void* edx, int incoming_sequence, int outgoing_acknowledged)
	{
		using Fn = void(__thiscall*)(void*, int, int);
		auto ofunc = Source::m_pClientStateSwap->VCall<Fn>(5);

		if (!cheat::main::local() || cheat::main::local()->IsDead() || !cheat::Cvars.RageBot_Enable.GetValue() || !Source::m_pEngine->IsInGame() || cheat::main::command_numbers.empty()
			/*|| game_rules->is_freeze_period() || !c_events::is_active_round*/)
			return ofunc(ecx, incoming_sequence, outgoing_acknowledged);

		for (auto it = cheat::main::command_numbers.begin(); it != cheat::main::command_numbers.end();) 
		{
			if (*it == outgoing_acknowledged)
			{
				cheat::main::command_numbers.erase(it);
				return ofunc(ecx, incoming_sequence, outgoing_acknowledged);
			}

			++it;
		}
	}
}