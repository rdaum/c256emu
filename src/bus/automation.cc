#include "bus/automation.h"

#include <algorithm>

#include "bus/system.h"

namespace {

constexpr const char kSystemLuaObj[] = "System";
constexpr const char kAutomationLuaObj[] = "Automation";

System *GetSystem(lua_State *L) {
  lua_getglobal(L, kSystemLuaObj);
  System *system = (System *)lua_touserdata(L, -1);
  CHECK(system);
  return system;
}

void PushTable(lua_State *L, const std::string &label, bool val) {
  lua_pushstring(L, label.c_str());
  lua_pushboolean(L, val);
  lua_settable(L, -3);
}

} // namespace

Automation::Automation(Cpu65816 *cpu, System *system,
                       InterruptController *interrupt_controller)
    : cpu_(cpu), system_(system), interrupt_controller_(interrupt_controller) {
  lua_state_ = luaL_newstate();
  luaL_openlibs(lua_state_);
  lua_pushlightuserdata(lua_state_, system);
  lua_setglobal(lua_state_, kSystemLuaObj);
  lua_pushlightuserdata(lua_state_, this);
  lua_setglobal(lua_state_, kAutomationLuaObj);

  luaL_openlib(lua_state_, "c256emu", c256emu_methods, 0);
}

Automation::~Automation() { lua_close(lua_state_); }

bool Automation::LoadScript(const std::string &path) {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);

  auto result = luaL_loadfile(lua_state_, path.c_str());
  if (result != 0) {
    LOG(ERROR) << "LoadScript load failure: " << lua_tostring(lua_state_, -1);
    return false;
  }
  if (lua_pcall(lua_state_, 0, LUA_MULTRET, 0) != 0) {
    LOG(ERROR) << "LoadScript run failure: " << lua_tostring(lua_state_, -1);
    return false;
  }
  LOG(INFO) << "Loaded automation script: " << path;
  return true;
}

bool Automation::Run() {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);
  stop_steps_.reset();

  return true;
}

bool Automation::Eval(const std::string &expression) {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);

  auto result = luaL_dostring(lua_state_, expression.c_str());
  if (result != 0) {
    LOG(ERROR) << "Eval failure: " << lua_tostring(lua_state_, -1);
    return false;
  }
  return true;
}

void Automation::SetStopSteps(uint32_t steps) {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);

  stop_steps_ = steps;
}

bool Automation::Step() {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);

  if (stop_steps_) {
    if (!stop_steps_.value() || !(stop_steps_.value()--)) {
      return false;
    }
  }
  for (auto &breakpoint : breakpoints_) {
    if (breakpoint.address == cpu_->program_address()) {
      lua_getglobal(lua_state_, breakpoint.lua_function_name.c_str());
      lua_pushinteger(lua_state_, cpu_->program_address().AsInt());
      lua_call(lua_state_, 1, 1);
      bool lua_result = lua_toboolean(lua_state_, -1);
      return lua_result;
    }
  }
  return true;
}

void Automation::AddBreakpoint(const Address &address,
                               const std::string &function_name) {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);

  breakpoints_.push_back({address, function_name});
}

void Automation::ClearBreakpoint(const Address &address) {
  auto bp = std::find_if(
      breakpoints_.begin(), breakpoints_.end(),
      [address](const Breakpoint &b) { return b.address == address; });
  if (bp != breakpoints_.end()) {
    breakpoints_.erase(bp);
  }
}

std::vector<Automation::Breakpoint> Automation::GetBreakpoints() const {
  return breakpoints_;
}

// static
const luaL_reg Automation::c256emu_methods[] = {
    {"stop", Automation::LuaStopCpu},
    {"continue", Automation::LuaContCpu},
    {"step", Automation::LuaStep},
    {"add_breakpoint", Automation::LuaAddBreakpoint},
    {"clear_breakpoint", Automation::LuaClearBreakpoint},
    {"breakpoints", Automation::LuaGetBreakpoints},
    {"cpu_state", Automation::LuaGetCpuState},
    {"peek", Automation::LuaPeek},
    {"poke", Automation::LuaPoke},
    {"peek16", Automation::LuaPeek16},
    {"poke16", Automation::LuaPoke16},
    {"peekbuf", Automation::LuaPeekBuf},
    {"load_hex", Automation::LuaLoadHex},
    {"load_bin", Automation::LuaLoadBin},
    {0, 0}};

// static
int Automation::LuaAddBreakpoint(lua_State *L) {
  lua_getglobal(L, kAutomationLuaObj);
  Automation *automation = (Automation *)lua_touserdata(L, -1);
  uint32_t addr = lua_tointeger(L, -1);
  Address break_addr(addr);
  size_t len;
  const char *function_cstr = lua_tolstring(L, -1, &len);
  std::string function(function_cstr, len);
  automation->AddBreakpoint(break_addr, function);

  return 0;
}

// static
int Automation::LuaClearBreakpoint(lua_State *L) {
  lua_getglobal(L, kAutomationLuaObj);
  Automation *automation = (Automation *)lua_touserdata(L, -1);
  uint32_t addr = lua_tointeger(L, -1);
  Address break_addr(addr);
  automation->ClearBreakpoint(break_addr);

  return 0;
}

// static
int Automation::LuaGetBreakpoints(lua_State *L) {
  lua_getglobal(L, kAutomationLuaObj);
  Automation *automation = (Automation *)lua_touserdata(L, -1);
  uint32_t addr = lua_tointeger(L, -1);
  Address break_addr(addr);
  auto breakpoints = automation->GetBreakpoints();

  lua_createtable(L, breakpoints.size(), 0);
  for (size_t i = 0; i < breakpoints.size(); i++) {
    const auto breakpoint = breakpoints[i];
    lua_pushinteger(L, breakpoint.address.AsInt());
    lua_settable(L, i);
  }

  return 1;
}

// static
int Automation::LuaPeek(lua_State *L) {
  System *sys = GetSystem(L);
  lua_pop(L, 1);
  uint32_t addr = lua_tointeger(L, -1);
  auto v = sys->ReadByte(Address(addr));
  lua_pushinteger(L, v);
  return 1;
}

// static
int Automation::LuaPoke(lua_State *L) {
  System *sys = GetSystem(L);
  lua_pop(L, 1);
  uint32_t addr = lua_tointeger(L, -1);
  uint32_t v = lua_tointeger(L, -1);
  sys->StoreByte(Address(addr), v);
  return 0;
}

// static
int Automation::LuaPeek16(lua_State *L) {
  System *sys = GetSystem(L);
  lua_pop(L, 1);
  uint32_t addr = lua_tointeger(L, -1);
  auto v = sys->ReadTwoBytes(Address(addr));
  lua_pushinteger(L, v);
  return 1;
}

// static
int Automation::LuaPoke16(lua_State *L) {
  System *sys = GetSystem(L);
  lua_pop(L, 1);
  uint32_t addr = lua_tointeger(L, -1);
  uint32_t v = lua_tointeger(L, -1);
  sys->StoreByte(Address(addr), v);
  return 0;
}

// static
int Automation::LuaPeekBuf(lua_State *L) {
  System *sys = GetSystem(L);
  lua_pop(L, 1);
  uint32_t start_addr = lua_tointeger(L, -1);
  uint32_t buf_size = lua_tointeger(L, -1);

  Address addr(start_addr);
  std::vector<uint8_t> buffer;
  for (size_t i = 0; i < buf_size; i++) {
    buffer.push_back(sys->ReadByte(addr));
    addr = addr + 1;
  }

  lua_pushlstring(L, (const char*)buffer.data(), buf_size);

  return 1;
}

// static
int Automation::LuaLoadHex(lua_State *L) {
  System *sys = GetSystem(L);
  lua_pop(L, 1);
  const std::string path = lua_tostring(L, -1);
  sys->LoadHex(path);
  return 0;
}

//static
int Automation::LuaLoadBin(lua_State *L) {
  System *sys = GetSystem(L);
  lua_pop(L, 1);
  const std::string path = lua_tostring(L, -1);
  uint32_t addr = lua_tointeger(L, -1);
  sys->LoadBin(path, Address(addr));
  return 0;
}


// static
int Automation::LuaGetCpuState(lua_State *L) {
  System *system = GetSystem(L);
  const CpuStatus *cpu_status = system->cpu()->cpu_status();

  lua_createtable(L, 0, 5);
  lua_pushstring(L, "a");
  lua_pushinteger(L, system->cpu()->a());
  lua_settable(L, -3);
  lua_pushstring(L, "x");
  lua_pushinteger(L, system->cpu()->x());
  lua_settable(L, -3);
  lua_pushstring(L, "y");
  lua_pushinteger(L, system->cpu()->y());
  lua_settable(L, -3);
  lua_pushstring(L, "pc");
  lua_pushinteger(L, system->cpu()->program_address().AsInt());
  lua_settable(L, -3);
  lua_pushstring(L, "cycle_count");
  lua_pushinteger(L, system->cpu()->TotalCycles());
  lua_settable(L, -3);
  lua_pushstring(L, "status");
  lua_createtable(L, 0, 10);
  PushTable(L, "carry_flag", cpu_status->carry_flag);
  PushTable(L, "zero_flag", cpu_status->zero_flag);
  PushTable(L, "interrupt_disable_flag", cpu_status->interrupt_disable_flag);
  PushTable(L, "decimal_flag", cpu_status->decimal_flag);
  PushTable(L, "break_flag", cpu_status->break_flag);
  PushTable(L, "accumulator_width_flag", cpu_status->accumulator_width_flag);
  PushTable(L, "index_width_flag", cpu_status->index_width_flag);
  PushTable(L, "emulation_flag", cpu_status->emulation_flag);
  PushTable(L, "overflow_flag", cpu_status->overflow_flag);
  PushTable(L, "sign_flag", cpu_status->sign_flag);
  lua_settable(L, -3);

  return 1;
}

// static
int Automation::LuaStopCpu(lua_State *L) {
  GetSystem(L)->Suspend();

  return 0;
}

// static
int Automation::LuaContCpu(lua_State *L) {
  GetSystem(L)->Resume();

  return 0;
}

// static
int Automation::LuaStep(lua_State *L) {
  lua_getglobal(L, kAutomationLuaObj);
  Automation *automation = (Automation *)lua_touserdata(L, -1);

  automation->SetStopSteps(1);

  return 0;
}
