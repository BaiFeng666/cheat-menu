#pragma once
#include "memory.h"

namespace hacks
{
	// run visual hacks
	void VisualsThread(const Memory& mem) noexcept;
	// run misc thread
	void miscThread(const Memory& mem) noexcept;
    // run aimbot thread
	void botThread(const Memory& mem) noexcept;
}