#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>
#include <thread>

#include "automation/automation.h"
#include "bus/loader.h"
#include "system.h"

DEFINE_bool(interpreter, false, "enable Lua command read prompt loop");
DEFINE_string(kernel_hex, "", "Location of kernel .hex file");
DEFINE_string(kernel_bin, "", "Location of kernel .bin file");
DEFINE_string(script, "", "Lua script to run on start (automation only)");
DEFINE_string(program_hex, "", "Program HEX file to load (optional)");

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, false);

  LOG(INFO) << "Good morning.";

  System system;

  bool kernel_loaded = false;
  if (!FLAGS_kernel_hex.empty())
    kernel_loaded = system.loader()->LoadFromHex(FLAGS_kernel_hex);
  else if (!FLAGS_kernel_bin.empty())
    kernel_loaded = system.loader()->LoadFromBin(FLAGS_kernel_bin, 0x180000);
  
  if (!kernel_loaded) {
	LOG(ERROR) << "No kernel; pass a valid kernel file with -kernel_hex or -kernel_bin";
	return -1;
  }

  if (!FLAGS_program_hex.empty() && !system.loader()->LoadFromHex(FLAGS_program_hex)) {
	LOG(ERROR) << "Invalid kernel hex file: "<< FLAGS_program_hex;
	return -1;
  }
  
  Automation* automation = system.automation();
  std::thread run_thread([&system, automation]() {
    system.Initialize();

    if (!FLAGS_script.empty()) {
      if (!automation->LoadScript(FLAGS_script)) {
        LOG(ERROR) << "Could not load automation file: " << FLAGS_script;
      }
    }
    system.Run();
  });

  run_thread.join();

  return 0;
}