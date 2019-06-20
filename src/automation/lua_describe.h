#pragma once

#include <lua.hpp>
#include <sstream>

class LuaDescribe {
 public:
  explicit LuaDescribe(int line_width) : line_width_(line_width) {}

  std::string luap_describe(lua_State* L, int index);

 private:
  void describe(lua_State* L, int index);
  void break_line();
  void dump_string(const std::string& s);
  void dump_character(const char c);
  void dump_literal(const std::string& s);

  std::stringstream dump_;
  int indent_, column_, line_width_, ancestors_;
};