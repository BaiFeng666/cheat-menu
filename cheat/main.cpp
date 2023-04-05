/*
   ____  _____                        __        _ __
  ( __ )/ ___/   ____ ___  ____ _____/ /__     (_) /_
 / __  / __ \   / __ `__ \/ __ `/ __  / _ \   / / __/
/ /_/ / /_/ /  / / / / / / /_/ / /_/ /  __/  / / /_
\____/\____/  /_/ /_/ /_/\__,_/\__,_/\___/  /_/\__/
*/

#include <thread>

#include "globals.h"
#include "gui.h"
#include "hacks.h"

int __stdcall wWinMain(HINSTANCE instance, HINSTANCE previousInstance,
                       PWSTR arguments, int commandShow) {
  Memory mem{"csgo.exe"};

  globals::clientAddress = mem.GetModuleAddress("client.dll");
  globals::engineAddress = mem.GetModuleAddress("engine.dll");

  std::thread(hacks::VisualsThread, mem).detach();
  std::thread(hacks::miscThread, mem).detach();
  std::thread(hacks::botThread, mem).detach();

  // create gui
  gui::CreateHWindow("86");
  gui::CreateDevice();
  gui::CreateImGui();

  while (gui::isRunning) {
    gui::BeginRender();
    gui::Render();
    gui::EndRender();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  // destroy gui
  gui::DestroyImGui();
  gui::DestroyDevice();
  gui::DestroyHWindow();

  return EXIT_SUCCESS;
}
