#pragma once

#include <string>
#include <lua.hpp>

#include "automation/lua_describe.h"

class LuaReplContext {
public:
  explicit LuaReplContext(::lua_State *lua_state);

  std::string Eval(const std::string& expression);
 private:
  bool incomplete_ = false;

  LuaDescribe describe_;

  ::lua_State* lua_state_;
  std::string buffer_;
  std::string ExecuteLua(int * status);
};