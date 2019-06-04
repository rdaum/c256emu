#pragma once

#include <mutex>
#include <vector>
#include <optional>

#include <lua5.1/lua.hpp>

#include "bus/int_controller.h"
#include "cpu/cpu_65816.h"

class System;

// Will be used for debugging / automation.
// Will be able to run Lua functions on breakpoints, or interactively from a
// REPL. Lua functions will include peek/poke, disassemble, register dump, set
// clear breakpoints, etc. Will be memory addressable as well, so that
// automation actions can be triggered from the running program.
class Automation {
public:
  Automation(Cpu65816 *cpu, System *system,
             InterruptController *interrupt_controller);
  ~Automation();

  // Called by the CPU before each instruction, when automation/debug mode is on.
  bool Step();

  bool LoadScript(const std::string &path);
  bool Run();
  bool Eval(const std::string &expression);

  void AddBreakpoint(const Address& address, const std::string &function_name);
  void ClearBreakpoint(const Address& address);

  struct Breakpoint {
    Address address;
    std::string lua_function_name;
  };
  std::vector<Breakpoint> GetBreakpoints() const;

private:
  void SetStopSteps(uint32_t steps);

  static int LuaStopCpu(lua_State *L);
  static int LuaContCpu(lua_State *L) ;
  static int LuaAddBreakpoint(lua_State *L) ;
  static int LuaClearBreakpoint(lua_State *L);
  static int LuaGetBreakpoints(lua_State *L) ;
  static int LuaGetCpuState(lua_State *L);
  static int LuaStep(lua_State *L);
  static const luaL_reg c256emu_methods[];

  std::recursive_mutex lua_mutex_;

  Cpu65816 *cpu_;
  System *system_;
  InterruptController *interrupt_controller_;

  lua_State *lua_state_;

  std::vector<Breakpoint> breakpoints_;
  std::optional<uint32_t> stop_steps_;
};