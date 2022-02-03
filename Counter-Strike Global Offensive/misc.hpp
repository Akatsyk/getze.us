#pragma once
#include "sdk.hpp"
#include <deque>
#include "source.hpp" 

using ThreadIDFn = int(_cdecl*)();

class c_misc
{
public:
	virtual void unlock_cvars();
	virtual	void unlock_cl_cvars();
	virtual int hitbox_to_hitgroup(int Hitbox);
	virtual bool goes_thru_smoke(Vector & start, Vector & end);
};