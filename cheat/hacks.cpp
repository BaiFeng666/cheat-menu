#include "hacks.h"
#include "globals.h"
#include "gui.h"
#include "vector.h"
#include <thread>
using namespace std;

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

void hacks::VisualsThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		this_thread::sleep_for(chrono::milliseconds(1));

		const auto localPlayer = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		if (!localPlayer)
			continue;

		const auto glowManager = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwGlowObjectManager);
		if (!glowManager)
			continue;

		const auto localTeam = mem.Read<int32_t>(localPlayer + offsets::m_iTeamNum);

		for (auto i = 1; i <= 64; ++i)
		{
			const auto entity = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

			if (!entity)
				continue;

			const auto entityTeam = mem.Read<int32_t>(entity + offsets::m_iTeamNum);
			if (entityTeam == localTeam)
				continue;

			const auto lifeState = mem.Read<int32_t>(entity + offsets::m_lifeState);

			if (lifeState != 0)
				continue;

			if (globals::glow)
			{
				const auto glowIndex = mem.Read<int32_t>(entity + offsets::m_iGlowIndex);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x8, globals::glowColor[0]); // red
				mem.Write(glowManager + (glowIndex * 0x38) + 0xC, globals::glowColor[1]); // green
				mem.Write(glowManager + (glowIndex * 0x38) + 0x10, globals::glowColor[2]); // blue
				mem.Write(glowManager + (glowIndex * 0x38) + 0x14, globals::glowColor[3]); // alpha

				// render when we can and can't see
				mem.Write(glowManager + (glowIndex * 0x38) + 0x27, true);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);

			}

			// RADAR
			if (globals::radar)
				mem.Write(entity + offsets::m_bSpotted, true);

			// CHAMS
			if (globals::chams)
			{
				if (mem.Read<uintptr_t>(entity + offsets::m_iTeamNum) == localTeam)
					mem.Write(entity + offsets::m_clrRender, globals::chamsTeamColor);
				else
					mem.Write(entity + offsets::m_clrRender, globals::chamsEnemyColor);

				// model brightness bullshit
				float brightness = 25.f;
				const auto _this = static_cast<uintptr_t>(globals::engineAddress + offsets::model_ambient_min - 0x2c);
				mem.Write(globals::engineAddress + offsets::model_ambient_min, *reinterpret_cast<uintptr_t*>(&brightness) ^ _this);

			}

			// Ignore Flash
			if (globals::ignoreFlash)
				mem.Write(localPlayer + offsets::m_flFlashMaxAlpha, 0.f);
		}

	}
}

void hacks::BHopThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		this_thread::sleep_for(chrono::milliseconds(1));

		if (!GetAsyncKeyState(VK_SPACE))
			continue;

		// get local player
		const auto localPlayer = mem.Read<intptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

		if (!localPlayer)
			continue;

		// is alive?
		const auto health = mem.Read<int32_t>(localPlayer + offsets::m_iHealth);

		if (!health)
			continue;

		const auto flags = mem.Read<int32_t>(localPlayer + offsets::m_fFlags);

		// on ground
		// 6 = force jump, 4 = reset
		if (globals::bhop) {
			(flags & (1 << 0)) ?
				mem.Write<intptr_t>(globals::clientAddress + offsets::dwForceJump, 6) :
				mem.Write<intptr_t>(globals::clientAddress + offsets::dwForceJump, 4);
		}
	}
}

void hacks::AimbotThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		this_thread::sleep_for(chrono::milliseconds(1));

		// get local player
		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

		// eye position = origin + viewOffset
		const auto localEyePosition = mem.Read<Vector3>(localPlayer + offsets::m_vecOrigin) +
			mem.Read<Vector3>(localPlayer + offsets::m_vecViewOffset);

		const auto clientState = mem.Read<std::uintptr_t>(globals::engineAddress + offsets::dwClientState);

		const auto localPlayerId =
			mem.Read<std::int32_t>(clientState + offsets::dwClientState_GetLocalPlayer);

		const auto viewAngles = mem.Read<Vector3>(clientState + offsets::dwClientState_ViewAngles);
		const auto aimPunch = mem.Read<Vector3>(localPlayer + offsets::m_aimPunchAngle) * 2;

		// aimbot fov
		auto bestFov = 5.f;
		auto bestAngle = Vector3{ };

		for (auto i = 1; i <= 64; ++i)
		{
			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

			//if (!GetAsyncKeyState(VK_RBUTTON))
			//	continue;

			if (mem.Read<std::int32_t>(player + offsets::m_iTeamNum) == localTeam)
				continue;

			if (mem.Read<bool>(player + offsets::m_bDormant))
				continue;

			if (mem.Read<std::int32_t>(player + offsets::m_lifeState))
				continue;

			if (mem.Read<std::int32_t>(player + offsets::m_bSpottedByMask))
			{
				const auto boneMatrix = mem.Read<std::uintptr_t>(player + offsets::m_dwBoneMatrix);

				// pos of player head in 3d space
				// 8 is the head bone index :)
				const auto playerHeadPosition = Vector3{
					mem.Read<float>(boneMatrix + 0x30 * 8 + 0x0C),
					mem.Read<float>(boneMatrix + 0x30 * 8 + 0x1C),
					mem.Read<float>(boneMatrix + 0x30 * 8 + 0x2C)
				};

				const auto angle = CalculateAngle(
					localEyePosition,
					playerHeadPosition,
					viewAngles + aimPunch
				);

				const auto fov = std::hypot(angle.x, angle.y);

				if (fov < bestFov)
				{
					bestFov = fov;
					bestAngle = angle;
				}
			}
		}
		if (globals::aimbot) {
			// if we have a best angle, do aimbot
			if (!bestAngle.IsZero())
				mem.Write<Vector3>(clientState + offsets::dwClientState_ViewAngles, viewAngles + bestAngle / globals::aimbotSmooth); // smoothing
		}
	}
}