#include "cpu/cpu_65816.h"

#include <gflags/gflags.h>
#include <iostream>
#include <thread>

#include <glog/logging.h>

extern "C" {
#include <readline/history.h>
#include <readline/readline.h>
}

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

  std::thread run_thread([&system]() {
    if (!FLAGS_kernel_hex.empty())
      system.LoadHex(FLAGS_kernel_hex);
    else if (!FLAGS_kernel_bin.empty())
      system.LoadBin(FLAGS_kernel_bin);
    else
      LOG(FATAL) << "No kernel";

    system.Initialize(FLAGS_automation ? FLAGS_script : "");

    if (!FLAGS_program_hex.empty())
      system.LoadHex(FLAGS_program_hex);

    system.Start(FLAGS_profile, FLAGS_automation);
  });

  if (FLAGS_automation) {
    char *buf;
    while ((buf = readline(">> ")) != nullptr) {
      if (strlen(buf) > 0) {
        add_history(buf);
      }

      system.automation()->Eval(buf);

      // readline malloc's a new buffer every time.
      free(buf);
    }
    system.Stop();
  }

  run_thread.join();

  return 0;
}