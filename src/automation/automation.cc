#include "automation.h"

#include <algorithm>

#include "system.h"

namespace {

constexpr const char kSystemLuaObj[] = "System";
constexpr const char kAutomationLuaObj[] = "Automation";

#if LUA_VERSION_NUM == 501
#define EOF_MARKER "'<eof>'"
#else
#define EOF_MARKER "<eof>"
#endif

System* GetSystem(lua_State* L) {
  lua_getglobal(L, kAutomationLuaObj);
  Automation* automation = (Automation*)lua_touserdata(L, -1);
  lua_pop(L, 1);
  return automation->system();
}

Automation* GetAutomation(lua_State* L) {
  lua_getglobal(L, kAutomationLuaObj);
  Automation* automation = (Automation*)lua_touserdata(L, -1);
  lua_pop(L, 1);
  return automation;
}

void PushTable(lua_State* L, const std::string& label, bool val) {
  lua_pushstring(L, label.c_str());
  lua_pushboolean(L, val);
  lua_settable(L, -3);
}

int traceback(lua_State* L) {
  lua_Debug ar;
  int i;

  if (lua_isnoneornil(L, 1) ||
      (!lua_isstring(L, 1) && !luaL_callmeta(L, 1, "__tostring"))) {
    lua_pushliteral(L, "(no error message)");
  }

  if (lua_gettop(L) > 1) {
    lua_replace(L, 1);
    lua_settop(L, 1);
  }

  /* Print the Lua stack. */

  lua_pushstring(L, "\n\nStack trace:\n");

  for (i = 0; lua_getstack(L, i, &ar); i += 1) {
    lua_getinfo(L, "Snlt", &ar);

    if (ar.istailcall) {
      lua_pushfstring(L, "\t... tail calls\n");
    }

    if (!strcmp(ar.what, "C")) {
      lua_pushfstring(L, "\t#%d [C]: in function ", i);

      if (ar.name) {
        lua_pushfstring(L, "'%s'\n", ar.name);
      } else {
        lua_pushfstring(L, "?\n");
      }
    } else if (!strcmp(ar.what, "main")) {
      lua_pushfstring(L, "\t#%d %s:%d: in the main chunk\n", i, ar.short_src,
                      ar.currentline);
    } else if (!strcmp(ar.what, "Lua")) {
      lua_pushfstring(L, "\t#%d %s:%d: in function ", i, ar.short_src,
                      ar.currentline);

      if (ar.name) {
        lua_pushfstring(L, "'%s'\n", ar.name);
      } else {
        lua_pushfstring(L, "?\n");
      }
    }
  }

  if (i == 0) {
    lua_pushstring(L, "No activation records.\n");
  }

  lua_concat(L, lua_gettop(L));

  return 1;
}

std::string luap_call(lua_State* L, int n, int* status) {
  int h;

  /* Push the error handler onto the stack. */

  h = lua_gettop(L) - n;
  lua_pushcfunction(L, traceback);
  lua_insert(L, h);

  /* Try to execute the supplied chunk and keep note of any return
   * values. */

  *status = lua_pcall(L, n, LUA_MULTRET, h);

  /* Print any errors. */

  if (*status != LUA_OK) {
    std::string result = lua_tostring(L, -1);
    lua_pop(L, 1);
    return result;
  }

  /* Remove the error handler. */
  lua_remove(L, h);

  return "";
}

}  // namespace

// static
const ::luaL_Reg Automation::c256emu_methods[] = {
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
    {"load_o65", Automation::LuaLoadO65},
    {"disassemble", Automation::LuaDisasm},
    {"sys", Automation::LuaSys},
    {0, 0}};

Automation::Automation(WDC65C816* cpu,
                       System* sys,
                       DebugInterface* debug_interface)
    : cpu_(cpu),
      system_(sys),
      debug_interface_(debug_interface),
      describe_(80) {
  lua_state_ = luaL_newstate();
  luaL_openlibs(lua_state_);
  lua_pushlightuserdata(lua_state_, this);
  lua_setglobal(lua_state_, kAutomationLuaObj);

  luaL_newlib(lua_state_, c256emu_methods);
  lua_setglobal(lua_state_, "c256emu");
}

Automation::~Automation() {
  lua_close(lua_state_);
}

bool Automation::LoadScript(const std::string& path) {
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

std::string Automation::ExecuteLua(int* status) {
  int i, h_0, h;

  std::string result;
  h_0 = lua_gettop(lua_state_);
  result = luap_call(lua_state_, 0, status);
  h = lua_gettop(lua_state_) - h_0 + 1;

  for (i = h; i > 0; i -= 1) {
    result = describe_.luap_describe(lua_state_, -1);

    if (h != 1) {
      std::stringstream output;
      output << h - i + 1 << ": " << result;
      result = output.str();
    }
  }

  /* Clean up.  We need to remove the results table as well if we
   * track results. */

  lua_settop(lua_state_, h_0 - 1);

  return result;
}

std::string Automation::Eval(const std::string& expression) {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);

  /* Try to execute the line with a return prepended first.  If
   * this works we can show returned values. */
  if (incomplete_) {
    buffer_.append(expression);
  } else {
    buffer_ = expression;
  }

  std::string l = "return " + buffer_;
  if (luaL_loadbuffer(lua_state_, l.c_str(), l.size(), "lua") == LUA_OK) {
    incomplete_ = false;
    int status;
    std::string result = ExecuteLua(&status);
    if (status == LUA_OK)
      return result;
  }

  lua_pop(lua_state_, 1);

  /* Try to execute the line as-is. */
  int status =
      luaL_loadbuffer(lua_state_, buffer_.c_str(), buffer_.size(), "lua");

  incomplete_ = false;
  if (status == LUA_ERRSYNTAX) {
    const int k = sizeof(EOF_MARKER) / sizeof(char) - 1;
    size_t n;
    const char* message = lua_tolstring(lua_state_, -1, &n);

    /* If the error message mentions an unexpected eof
     * then consider this a multi-line statement and wait
     * for more input.  If not then just print the error
     * message.*/
    if ((int)n > k && !strncmp(message + n - k, EOF_MARKER, k)) {
      lua_pop(lua_state_, 1);

      incomplete_ = true;
      return "";
    }

    lua_pop(lua_state_, 1);
    incomplete_ = false;
    return lua_tostring(lua_state_, -1);
  }

  if (status == LUA_ERRMEM) {
    incomplete_ = false;
    return lua_tostring(lua_state_, -1);
  }

  /* Try to execute the loaded chunk. */
  incomplete_ = false;
  return ExecuteLua(&status);
}

void Automation::AddBreakpoint(uint32_t address,
                               const std::string& function_name) {
  std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);

  breakpoints_.push_back({address, function_name});
  debug_interface_->SetBreakpoint(address, [this, address](EmulatedCpu*) {
    std::lock_guard<std::recursive_mutex> lua_lock(lua_mutex_);
    for (auto& bp : breakpoints_) {
      if (bp.address == address) {
        lua_getglobal(lua_state_, bp.lua_function_name.c_str());
        lua_pcall(lua_state_, 0, 0, 0);
        break;
      }
    }
  });
}

void Automation::ClearBreakpoint(uint32_t address) {
  auto bp = std::find_if(
      breakpoints_.begin(), breakpoints_.end(),
      [address](const Breakpoint& b) { return b.address == address; });
  if (bp != breakpoints_.end()) {
    breakpoints_.erase(bp);
    debug_interface_->ClearBreakpoint(address);
  }
}

std::vector<Automation::Breakpoint> Automation::GetBreakpoints() const {
  return breakpoints_;
}

// static
int Automation::LuaAddBreakpoint(lua_State* L) {
  Automation* automation = GetAutomation(L);
  uint32_t addr = lua_tointeger(L, -3);
  size_t len;
  const char* function_cstr = lua_tolstring(L, -2, &len);
  std::string function(function_cstr, len);
  automation->AddBreakpoint(addr, function);

  return 0;
}

// static
int Automation::LuaClearBreakpoint(lua_State* L) {
  Automation* automation = GetAutomation(L);
  uint32_t addr = lua_tointeger(L, -2);
  automation->ClearBreakpoint(addr);

  return 0;
}

// static
int Automation::LuaGetBreakpoints(lua_State* L) {
  Automation* automation = GetAutomation(L);
  auto breakpoints = automation->GetBreakpoints();

  lua_createtable(L, breakpoints.size(), 0);
  for (size_t i = 0; i < breakpoints.size(); i++) {
    const auto breakpoint = breakpoints[i];
    lua_pushinteger(L, breakpoint.address);
    lua_settable(L, i);
  }

  return 1;
}

// static
int Automation::LuaPeek(lua_State* L) {
  System* sys = GetSystem(L);
  uint32_t addr = lua_tointeger(L, -1);
  auto v = sys->ReadByte(addr);
  lua_pushinteger(L, v);
  return 1;
}

// static
int Automation::LuaPoke(lua_State* L) {
  System* sys = GetSystem(L);
  uint32_t addr = lua_tointeger(L, -2);
  uint32_t v = lua_tointeger(L, -1);
  sys->StoreByte(addr, v);
  return 0;
}

// static
int Automation::LuaPeek16(lua_State* L) {
  System* sys = GetSystem(L);
  uint32_t addr = lua_tointeger(L, -1);
  auto v = sys->ReadTwoBytes(addr);
  lua_pushinteger(L, v);
  return 1;
}

// static
int Automation::LuaPoke16(lua_State* L) {
  System* sys = GetSystem(L);
  uint32_t addr = lua_tointeger(L, -2);
  uint32_t v = lua_tointeger(L, -1);
  sys->StoreByte(addr, v);
  return 0;
}

// static
int Automation::LuaPeekBuf(lua_State* L) {
  System* sys = GetSystem(L);
  uint32_t start_addr = lua_tointeger(L, -2);
  uint32_t buf_size = lua_tointeger(L, -1);

  uint32_t end_addr(start_addr + buf_size);
  std::vector<uint8_t> buffer;
  for (auto i = start_addr; i < end_addr; i++) {
    buffer.push_back(sys->ReadByte(i));
  }

  lua_pushlstring(L, (const char*)buffer.data(), buf_size);

  return 1;
}

// static
int Automation::LuaLoadHex(lua_State* L) {
  System* sys = GetSystem(L);
  const std::string path = lua_tostring(L, -1);
  sys->loader()->LoadFromHex(path);
  return 0;
}

// static
int Automation::LuaLoadBin(lua_State* L) {
  System* sys = GetSystem(L);
  const std::string path = lua_tostring(L, -2);
  uint32_t addr = lua_tointeger(L, -1);
  sys->loader()->LoadFromBin(path, addr);

  return 0;
}

// static
int Automation::LuaLoadO65(lua_State* L) {
  System* sys = GetSystem(L);
  const std::string path = lua_tostring(L, -2);
  uint32_t addr = lua_tointeger(L, -1);
  sys->loader()->LoadFromO65(path, addr);

  return 0;
}

// static
int Automation::LuaSys(lua_State* L) {
  System* sys = GetSystem(L);
  Automation* automation = GetAutomation(L);

  uint32_t addr = lua_tointeger(L, -1);

  automation->debug_interface_->Pause();
  sys->Sys(addr);
  automation->debug_interface_->Resume();
  return 0;
}

// static
int Automation::LuaDisasm(lua_State* L) {
  System* sys = GetSystem(L);
  Automation* automation = GetAutomation(L);
  WDC65C816* cpu = sys->cpu();
  auto disassembler = cpu->GetDisassembler();
  cpuaddr_t addr;
  if (lua_isnumber(L, -1)) {
    addr = (cpuaddr_t)lua_tointeger(L, -1);
  } else if (!automation->debug_interface_->paused()) {
    return luaL_error(
        L, "c256emu: cannot disassemble current address while not paused");
  } else {
    addr = cpu->GetCpuState()->GetCanonicalAddress();
  }
  Disassembler::Config config;
  if (lua_isnumber(L, 2))
    config.max_instruction_count = (uint32_t)lua_tointeger(L, -2);

  auto list = disassembler->Disassemble(config, addr);
  lua_newtable(L);
  for (uint32_t i = 0; i < list.size(); i++) {
    lua_pushlstring(L, list[i].asm_string.c_str(), list[i].asm_string.size());
    lua_rawseti(L, -2, i + 1);
  }
  return 1;
}

// static
int Automation::LuaStopCpu(lua_State* L) {
  Automation* automation = GetAutomation(L);
  automation->debug_interface_->Pause();

  return 0;
}

// static
int Automation::LuaContCpu(lua_State* L) {
  Automation* automation = GetAutomation(L);
  automation->debug_interface_->Resume();

  return 0;
}

// static
int Automation::LuaStep(lua_State* L) {
  Automation* automation = GetAutomation(L);
  automation->debug_interface_->SingleStep(1);

  return 0;
}

// static
int Automation::LuaGetCpuState(lua_State* L) {
  Automation* automation = GetAutomation(L);
  const auto& cpu_state = automation->cpu_->cpu_state;

  lua_createtable(L, 0, 5);
  lua_pushstring(L, "a");
  lua_pushinteger(L, automation->cpu_->a());
  lua_settable(L, -3);
  lua_pushstring(L, "x");
  lua_pushinteger(L, automation->cpu_->x());
  lua_settable(L, -3);
  lua_pushstring(L, "y");
  lua_pushinteger(L, automation->cpu_->y());
  lua_settable(L, -3);
  lua_pushstring(L, "pc");
  lua_pushinteger(L, automation->cpu_->program_address());
  lua_settable(L, -3);
  lua_pushstring(L, "cycle_count");
  lua_pushinteger(L, cpu_state.cycle);
  lua_settable(L, -3);
  lua_pushstring(L, "status");
  lua_createtable(L, 0, 10);
  PushTable(L, "carry_flag", cpu_state.is_carry());
  PushTable(L, "zero_flag", cpu_state.is_zero());
  PushTable(L, "interrupt_disable_flag", !cpu_state.interrupts_enabled());
  PushTable(L, "decimal_flag", cpu_state.is_decimal());
  PushTable(L, "break_flag", 1);
  PushTable(L, "accumulator_width_flag", !automation->cpu_->mode_long_a);
  PushTable(L, "index_width_flag", !automation->cpu_->mode_long_xy);
  PushTable(L, "emulation_flag", automation->cpu_->mode_emulation);
  PushTable(L, "overflow_flag", cpu_state.is_overflow());
  PushTable(L, "sign_flag", cpu_state.is_negative());
  lua_settable(L, -3);

  return 1;
}
