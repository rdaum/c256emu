#pragma once

#include <lua.hpp>
#include <sstream>

class LuaDescribe {
public:
  explicit LuaDescribe(int line_width);

  std::string Describe(lua_State *L, int index);

private:
  void DoDescribe(lua_State *L, int index);
  void BreakLine();
  void DumpString(const std::string &s);
  void DumpCharacter(const char c);
  void DumpLiteral(const std::string &s);

  std::stringstream dump_;
  size_t indent_ = 0;
  int column_ = 0;
  int line_width_;
  int ancestors_ = 0;
};