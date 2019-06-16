#include <gflags/gflags.h>
#include <glog/logging.h>

#include <SDL2/SDL.h>
#include <iostream>
#include <thread>

#include "automation/automation.h"
#include "system.h"

DEFINE_bool(interpreter, false, "enable Lua command read prompt loop");
DEFINE_string(kernel_hex, "", "Location of kernel .hex file");
DEFINE_string(kernel_bin, "", "Location of kernel .bin file");
DEFINE_string(script, "", "Lua script to run on start (automation only)");
DEFINE_string(program_hex, "", "Program HEX file to load (optional)");

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = true;
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, false);

  SDL_Init(SDL_INIT_VIDEO);

  LOG(INFO) << "Good morning.";

  System system;
  if (!FLAGS_kernel_hex.empty())
    system.LoadHex(FLAGS_kernel_hex);
  else if (!FLAGS_kernel_bin.empty())
    system.LoadBin(FLAGS_kernel_bin, 0x180000);
  else
    LOG(FATAL) << "No kernel";

  if (!FLAGS_program_hex.empty())
    system.LoadHex(FLAGS_program_hex);

  Automation *automation = system.automation();
  std::thread run_thread([&system, automation]() {
    system.Initialize();

    if (!FLAGS_script.empty()) {
      automation->LoadScript(FLAGS_script);
    }
    system.Run();
  });

  run_thread.join();

  return 0;
}