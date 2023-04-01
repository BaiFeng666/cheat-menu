#pragma once
#include "memory.h"

namespace hacks
{
	// run visual hacks
	void VisualsThread(const Memory& mem) noexcept;

	// run BHopThread
	void BHopThread(const Memory& mem) noexcept;

	void AimbotThread(const Memory& mem) noexcept;
}