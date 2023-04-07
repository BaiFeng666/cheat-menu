#include "hacks.h"

#include <thread>

#include "globals.h"
#include "gui.h"
#include "vector.h"
using namespace std;

// For aimbot
constexpr Vector3 CalculateAngle(const Vector3 &localPosition,
                                 const Vector3 &enemyPosition,
                                 const Vector3 &viewAngles) noexcept {
  return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

// For 0 recoil
struct Vector2 {
  float x = {}, y = {};
};
Vector2 oldPunch = Vector2{};

void hacks::VisualsThread(const Memory &mem) noexcept {
  while (gui::isRunning) {
    this_thread::sleep_for(chrono::milliseconds(1));

    const auto localPlayer =
        mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
    if (!localPlayer) continue;

    const auto glowManager = mem.Read<uintptr_t>(globals::clientAddress +
                                                 offsets::dwGlowObjectManager);
    if (!glowManager) continue;

    const auto localTeam = mem.Read<int32_t>(localPlayer + offsets::m_iTeamNum);

    for (auto i = 1; i <= 64; ++i) {
      const auto entity = mem.Read<uintptr_t>(globals::clientAddress +
                                              offsets::dwEntityList + i * 0x10);
      if (!entity) continue;

      const auto entityTeam = mem.Read<int32_t>(entity + offsets::m_iTeamNum);
      if (entityTeam == localTeam) continue;
      //----------GLOW----------
      if (globals::glow) {
        const auto glowIndex =
            mem.Read<int32_t>(entity + offsets::m_iGlowIndex);

        mem.Write(glowManager + (glowIndex * 0x38) + 0x8,
                  globals::glowColor[0]);  // red
        mem.Write(glowManager + (glowIndex * 0x38) + 0xC,
                  globals::glowColor[1]);  // green
        mem.Write(glowManager + (glowIndex * 0x38) + 0x10,
                  globals::glowColor[2]);  // blue
        mem.Write(glowManager + (glowIndex * 0x38) + 0x14,
                  globals::glowColor[3]);  // alpha

        // render when we can and can't see
        mem.Write(glowManager + (glowIndex * 0x38) + 0x27, true);
        mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);
      }
      //----------CHAMS----------
      if (globals::chams) {
        // convert from int to float because of shitty ImGui
        uint8_t EnemyColor[] = {0, 0, 0};
        for (int i = 0; i < 3; ++i)
          EnemyColor[i] = uint8_t(globals::chamsGuiEnemyColor[i] * 255);

        if (mem.Read<uintptr_t>(entity + offsets::m_iTeamNum) != localTeam)
          mem.Write(entity + offsets::m_clrRender, EnemyColor);

        // model brightness bullshit
        float brightness = 25.f;
        const auto _this = static_cast<uintptr_t>(
            globals::engineAddress + offsets::model_ambient_min - 0x2c);
        mem.Write(globals::engineAddress + offsets::model_ambient_min,
                  *reinterpret_cast<uintptr_t *>(&brightness) ^ _this);
      }
    }
  }
}

void hacks::miscThread(const Memory &mem) noexcept {
  while (gui::isRunning) {
    this_thread::sleep_for(chrono::milliseconds(1));

    // get local player
    const auto localPlayer =
        mem.Read<intptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
    if (!localPlayer) continue;
    // is alive?
    const auto health = mem.Read<int32_t>(localPlayer + offsets::m_iHealth);
    if (!health) continue;

    // ----------BHOP----------
    const auto flags = mem.Read<int32_t>(localPlayer + offsets::m_fFlags);
    if (globals::bhop)
      if (GetAsyncKeyState(VK_SPACE))
        // 6 = force jump, 4 = reset
        (flags & (1 << 0))
            ? mem.Write<intptr_t>(globals::clientAddress + offsets::dwForceJump,
                                  6)
            : mem.Write<intptr_t>(globals::clientAddress + offsets::dwForceJump,
                                  4);
    // ----------RADAR----------
    if (globals::radar)
      for (auto i = 1; i <= 64; ++i) {
        const auto entity = mem.Read<uintptr_t>(
            globals::clientAddress + offsets::dwEntityList + i * 0x10);
        if (!entity) continue;
        mem.Write(entity + offsets::m_bSpotted, true);
      }
    // ----------Ignore Flash----------
    if (globals::ignoreFlash)
      mem.Write(localPlayer + offsets::m_flFlashDuration, 0.f);
    // ----------Fov----------
    if (globals::fov) {
        if (!mem.Read<bool>(localPlayer + offsets::m_bIsScoped))
            mem.Write(localPlayer + offsets::m_iFOV, globals::fovValue);
        else
            if (!mem.Read<bool>(localPlayer + offsets::m_bIsScoped))
                mem.Write(localPlayer + offsets::m_iFOV, mem.Read<int32_t>(localPlayer + offsets::m_iDefaultFOV));
    }
    // ----------Remove Recoil----------
    if (globals::removeRecoil) {
      const auto shotsFired =
          mem.Read<int32_t>(localPlayer + offsets::m_iShotsFired);

      if (shotsFired) {
        const auto clientState = mem.Read<uintptr_t>(globals::engineAddress +
                                                     offsets::dwClientState);
        const auto viewAngles =
            mem.Read<Vector2>(clientState + offsets::dwClientState_ViewAngles);

        const auto aimPunch =
            mem.Read<Vector2>(localPlayer + offsets::m_aimPunchAngle);

        auto newAngles = Vector2{viewAngles.x + oldPunch.x - aimPunch.x * 2.f,
                                 viewAngles.y + oldPunch.y - aimPunch.y * 2.f};

        // limit is 89 and -89, if bigger quick vac :0
        if (newAngles.x > 89.f) newAngles.x = 89.f;

        if (newAngles.x < -89.f) newAngles.x = -89.f;

        if (newAngles.y > 180.f) newAngles.y -= 360.f;

        if (newAngles.y < -180.f) newAngles.y += 360.f;

        mem.Write<Vector2>(clientState + offsets::dwClientState_ViewAngles,
                           newAngles);

        oldPunch.x = aimPunch.x * 2.f;
        oldPunch.y = aimPunch.y * 2.f;
      } else {
        oldPunch.x = oldPunch.y = 0.f;
      }
    }
  }
}

void hacks::botThread(const Memory &mem) noexcept {
  while (gui::isRunning) {
    this_thread::sleep_for(chrono::milliseconds(1));

    // get local player
    const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress +
                                                      offsets::dwLocalPlayer);
    const auto localTeam =
        mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

    // eye position = origin + viewOffset
    const auto localEyePosition =
        mem.Read<Vector3>(localPlayer + offsets::m_vecOrigin) +
        mem.Read<Vector3>(localPlayer + offsets::m_vecViewOffset);

    const auto clientState = mem.Read<std::uintptr_t>(globals::engineAddress +
                                                      offsets::dwClientState);

    const auto localPlayerId = mem.Read<std::int32_t>(
        clientState + offsets::dwClientState_GetLocalPlayer);

    const auto viewAngles =
        mem.Read<Vector3>(clientState + offsets::dwClientState_ViewAngles);
    const auto aimPunch =
        mem.Read<Vector3>(localPlayer + offsets::m_aimPunchAngle) * 2;

    // aimbot fov
    auto bestFov = float(globals::aimbotFov);
    auto bestAngle = Vector3{};

    for (auto i = 1; i <= 64; ++i) {
      // ----------AIMBOT----------
      const auto player = mem.Read<std::uintptr_t>(
          globals::clientAddress + offsets::dwEntityList + i * 0x10);

      if (mem.Read<std::int32_t>(player + offsets::m_iTeamNum) == localTeam)
        continue;

      if (mem.Read<bool>(player + offsets::m_bDormant)) continue;

      if (mem.Read<std::int32_t>(player + offsets::m_lifeState)) continue;

      if (mem.Read<std::int32_t>(player + offsets::m_bSpottedByMask)) {
        const auto boneMatrix =
            mem.Read<std::uintptr_t>(player + offsets::m_dwBoneMatrix);
        // pos of player head in 3d space
        // 8 is the head bone index :)
        const auto playerHeadPosition =
            Vector3{mem.Read<float>(boneMatrix + 0x30 * 8 + 0x0C),
                    mem.Read<float>(boneMatrix + 0x30 * 8 + 0x1C),
                    mem.Read<float>(boneMatrix + 0x30 * 8 + 0x2C)};

        const auto angle = CalculateAngle(localEyePosition, playerHeadPosition,
                                          viewAngles + aimPunch);

        const auto fov = std::hypot(angle.x, angle.y);
        if (fov < bestFov) {
          bestFov = fov;
          bestAngle = angle;
        }
      }
    }
    if (globals::aimbot)
      // if we have the best angle, do aimbot
      if (!bestAngle.IsZero())
        mem.Write<Vector3>(
            clientState + offsets::dwClientState_ViewAngles,
            viewAngles +
                bestAngle / float(globals::aimbotSmooth));  // smoothing

    // ----------TRIGGERBOT----------
    if (globals::triggerbot) {
      const auto localHealth =
          mem.Read<int32_t>(localPlayer + offsets::m_iHealth);
      if (!localHealth) continue;

      // if crosshair id is bigger than 64 we are not looking at a entity
      const auto crosshairId =
          mem.Read<int32_t>(localPlayer + offsets::m_iCrosshairId);
      if (!crosshairId || crosshairId > 64) continue;

      const auto triggerBotPlayer =
          mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList +
                              (crosshairId - 1) * 0x10);

      // check if entity is dead
      if (!mem.Read<int32_t>(triggerBotPlayer + offsets::m_iHealth)) continue;

      if (mem.Read<int32_t>(triggerBotPlayer + offsets::m_iTeamNum) ==
          localTeam)
        continue;

      // same logic like bhop
      mem.Write(globals::clientAddress + offsets::dwForceAttack, 6);
      this_thread::sleep_for(chrono::milliseconds(20));
      mem.Write(globals::clientAddress + offsets::dwForceAttack, 4);
    }
  }
}