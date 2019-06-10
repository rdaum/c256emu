#include <gflags/gflags.h>
#include <glog/logging.h>
#include <linenoise.h>

#include <iostream>
#include <thread>

#include "bus/automation.h"
#include "bus/system.h"

DEFINE_bool(profile, false, "enable CPU performance profiling");
DEFINE_bool(automation, false, "enable Lua automation / debug scripting");
DEFINE_string(kernel_hex, "", "Location of kernel .hex file");
DEFINE_string(kernel_bin, "", "Location of kernel .bin file");
DEFINE_string(script, "", "Lua script to run on start (automation only)");
DEFINE_string(program_hex, "", "Program HEX file to load (optional)");

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = true;
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, false);

  LOG(INFO) << "Good morning.";

  System system;
  if (!FLAGS_kernel_hex.empty())
    system.LoadHex(FLAGS_kernel_hex);
  else if (!FLAGS_kernel_bin.empty())
    system.LoadBin(FLAGS_kernel_bin, 0x180000);
  else
    LOG(FATAL) << "No kernel";

  if (!FLAGS_program_hex.empty()) system.LoadHex(FLAGS_program_hex);

  std::thread run_thread([&system]() {
    system.Initialize();
    system.Start(FLAGS_profile);
  });

  if (FLAGS_automation) {
    Automation automation(system.cpu(), &system, system.GetDebugInterface());

    if (!FLAGS_script.empty()) {
      automation.LoadScript(FLAGS_script);
    }

    linenoiseInstallWindowChangeHandler();

    char *buf;
    while ((buf = linenoise(">> ")) != nullptr) {
      if (strlen(buf) > 0) {
        linenoiseHistoryAdd(buf);
      }

      automation.Eval(buf);

      // readline malloc's a new buffer every time.
      free(buf);
    }
    linenoiseHistoryFree();
    system.Stop();
  }

  run_thread.join();

  return 0;
}