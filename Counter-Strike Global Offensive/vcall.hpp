#pragma once

#include "auto.hpp"

namespace Horizon::Memory
{

template<typename T>
T VCall( const void* instance, const std::uint32_t index )
{
	//if (IsBadCodePtr(this))
	//	return (T)nullptr;

	if (instance == nullptr)
		return (T)nullptr;

	return (T)((*(void***)instance)[index]);
}

}