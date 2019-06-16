#pragma once

#include <lua.hpp>

class LuaDescribe {
public:
  explicit LuaDescribe(int line_width) : line_width_(line_width) {}

  char *luap_describe(lua_State *L, int index);

private:
  void describe(lua_State *L, int index);
  void check_fit(int size);
  void break_line();
  void dump_string(const char *s, int n);

  char *dump_;
  int length_, offset_, indent_, column_, line_width_, ancestors_;
};