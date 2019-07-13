/* Copyright (C) 2012-2015 Papavasileiou Dimitris
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "automation/lua_describe.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

/*
 * All this code borrowed / forked from https://github.com/dpapavas/luaprompt
 */

#define absolute(L, i) (i < 0 ? lua_gettop(L) + i + 1 : i)

namespace {

int printed_width(const std::string &s) {
  int n;
  bool discard = 0;

  /* Calculate the printed width of the chunk s ignoring escape
   * sequences. */
  for (char c : s) {
    if (!discard && c == '\033') {
      discard = true;
    }
    if (!discard) {
      n++;
    }
    if (discard && c == 'm') {
      discard = false;
    }
  }
  return n;
}
} // namespace

void LuaDescribe::dump_literal(const std::string &s) {
  dump_ << s;
  column_ += printed_width(s);
}

void LuaDescribe::dump_character(const char c) {
  dump_ << c;
  column_ += 1;
}

static int is_identifier(const char *s, int n) {
  int i;

  /* Check whether a string can be used as a key without quotes and
   * braces. */

  for (i = 0; i < n; i += 1) {
    if (!isalpha(s[i]) && (i == 0 || !isalnum(s[i])) && s[i] != '_') {
      return 0;
    }
  }

  return 1;
}

void LuaDescribe::break_line() {
  int i;

  /* Add a line break. */

  dump_ << '\n';

  /* And indent to the current level. */

  for (i = 1; i <= indent_; i += 1) {
    dump_ << ' ';
  }

  column_ = indent_;
}

void LuaDescribe::dump_string(const std::string &s) {
  int l;

  /* Break the line if the current chunk doesn't fit but it would
   * fit if we started on a fresh line at the current indent. */

  l = printed_width(s);

  if (column_ + l > line_width_ && indent_ + l <= line_width_) {
    break_line();
  }

  /* Copy the string to the buffer. */
  dump_ << s;

  column_ += l;
}

void LuaDescribe::describe(lua_State *L, int index) {
  char *s;
  size_t n;
  int type;

  index = absolute(L, index);
  type = lua_type(L, index);

  if (luaL_getmetafield(L, index, "__tostring")) {
    lua_pushvalue(L, index);
    lua_pcall(L, 1, 1, 0);
    s = (char *)lua_tolstring(L, -1, &n);
    lua_pop(L, 1);

    dump_string(s);
  } else if (type == LUA_TNUMBER) {
    /* Copy the value to avoid mutating it. */

    lua_pushvalue(L, index);
    s = (char *)lua_tolstring(L, -1, &n);
    lua_pop(L, 1);

    dump_string(s);
  } else if (type == LUA_TSTRING) {
    int i, started, score, level, uselevel = 0;

    s = (char *)lua_tolstring(L, index, &n);

    /* Scan the string to decide how to print it. */

    for (i = 0, score = n, started = 0; i < (int)n; i += 1) {
      if (s[i] == '\n' || s[i] == '\t' || s[i] == '\v' || s[i] == '\r') {
        /* These characters show up better in a long sting so
         * bias towards that. */

        score += line_width_ / 2;
      } else if (s[i] == '\a' || s[i] == '\b' || s[i] == '\f' ||
                 !isprint(s[i])) {
        /* These however go better with an escaped short
         * string (unless you like the bell or weird
         * characters). */

        score -= line_width_ / 4;
      }

      /* Check what long string delimeter level to use so that
       * the string won't be closed prematurely. */

      if (!started) {
        if (s[i] == ']') {
          started = 1;
          level = 0;
        }
      } else {
        if (s[i] == '=') {
          level += 1;
        } else if (s[i] == ']') {
          if (level >= uselevel) {
            uselevel = level + 1;
          }
        } else {
          started = 0;
        }
      }
    }

    if (score > line_width_) {
      /* Dump the string as a long string. */

      dump_character('[');
      for (i = 0; i < uselevel; i += 1) {
        dump_character('=');
      }
      dump_literal("[\n");

      dump_string(s);

      dump_character(']');
      for (i = 0; i < uselevel; i += 1) {
        dump_character('=');
      }
      dump_literal("]");
    } else {
      /* Escape the string as needed and print it as a normal
       * string. */

      dump_literal("\"");

      for (i = 0; i < (int)n; i += 1) {
        if (s[i] == '"' || s[i] == '\\') {
          dump_literal("\\");
          dump_character(s[i]);
        } else if (s[i] == '\a') {
          dump_literal("\\a");
        } else if (s[i] == '\b') {
          dump_literal("\\b");
        } else if (s[i] == '\f') {
          dump_literal("\\f");
        } else if (s[i] == '\n') {
          dump_literal("\\n");
        } else if (s[i] == '\r') {
          dump_literal("\\r");
        } else if (s[i] == '\t') {
          dump_literal("\\t");
        } else if (s[i] == '\v') {
          dump_literal("\\v");
        } else if (isprint(s[i])) {
          dump_character(s[i]);
        } else {
          char *t = new char[5];
          size_t n;

          sprintf(t, "\\%03u", ((unsigned char *)s)[i]);
          dump_string(t);
        }
      }

      dump_literal("\"");
    }
  } else if (type == LUA_TNIL) {
    dump_string("nil");
  } else if (type == LUA_TBOOLEAN) {
    s = lua_toboolean(L, index) ? "true" : "false";
    dump_string(s);
  } else if (type == LUA_TFUNCTION) {
    std::stringstream ss;
    ss << "<function:" << std::hex << lua_topointer(L, index) << ">";
    dump_string(ss.str());
  } else if (type == LUA_TUSERDATA) {
    std::stringstream ss;
    ss << "<userdata:" << std::hex << lua_topointer(L, index) << ">";
    dump_string(ss.str());
  } else if (type == LUA_TTHREAD) {
    std::stringstream ss;
    ss << "<thread:" << std::hex << lua_topointer(L, index) << ">";
    dump_string(ss.str());
  } else if (type == LUA_TTABLE) {
    std::stringstream ss;
    int i, l, n, oldindent, multiline, nobreak;

    /* Check if table is too deeply nested. */

    if (indent_ > 8 * line_width_ / 10) {
      std::stringstream ss;
      ss << "{ ... }";
      dump_string(ss.str());

      return;
    }

    /* Check if the table introduces a cycle by checking whether
     * it is a back-edge (that is equal to an ancestor table. */

    lua_rawgeti(L, LUA_REGISTRYINDEX, ancestors_);
    n = lua_rawlen(L, -1);

    for (i = 0; i < n; i += 1) {
      lua_rawgeti(L, -1, n - i);
      if (lua_compare(L, -1, -3, LUA_OPEQ)) {
        size_t n;

        ss << "{ [" << -(i + 1) << "]... }";
        dump_string(ss.str());
        lua_pop(L, 2);

        return;
      }

      lua_pop(L, 1);
    }

    /* Add the table to the ancestor list and pop the ancestor
     * list table. */

    lua_pushvalue(L, index);
    lua_rawseti(L, -2, n + 1);
    lua_pop(L, 1);

    /* Open the table and update the indentation level to the
     * current column. */

    dump_literal("{ ");
    oldindent = indent_;
    indent_ = column_;
    multiline = 0;
    nobreak = 0;

    l = lua_rawlen(L, index);

    /* Traverse the array part first. */

    for (i = 0; i < l; i += 1) {
      lua_pushinteger(L, i + 1);
      lua_gettable(L, index);

      /* Start a fresh line when dumping tables to make sure
       * there's plenty of room. */

      if (lua_istable(L, -1)) {
        if (!nobreak) {
          break_line();
        }

        multiline = 1;
      }

      nobreak = 0;

      /* Dump the value and separating comma. */

      describe(L, -1);
      dump_literal(", ");

      if (lua_istable(L, -1) && i != l - 1) {
        break_line();
        nobreak = 1;
      }

      lua_pop(L, 1);
    }

    /* Now for the hash part. */

    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
      if (lua_type(L, -2) != LUA_TNUMBER ||
          lua_tonumber(L, -2) != lua_tointeger(L, -2) ||
          lua_tointeger(L, -2) < 1 || lua_tointeger(L, -2) > l) {
        /* Keep each key-value pair on a separate line. */

        break_line();
        multiline = 1;

        /* Dump the key and value. */

        if (lua_type(L, -2) == LUA_TSTRING) {
          char *s;
          size_t n;

          s = (char *)lua_tolstring(L, -2, &n);

          if (is_identifier(s, n)) {
            dump_string(s);
          } else {
            dump_literal("[");
            describe(L, -2);
            dump_literal("]");
          }
        } else {
          dump_literal("[");
          describe(L, -2);
          dump_literal("]");
        }

        dump_literal(" = ");
        describe(L, -1);
        dump_literal(",");
      }

      lua_pop(L, 1);
    }

    /* Remove the table from the ancestor list. */

    lua_rawgeti(L, LUA_REGISTRYINDEX, ancestors_);
    lua_pushnil(L);
    lua_rawseti(L, -2, n + 1);
    lua_pop(L, 1);

    /* Pop the indentation level. */

    indent_ = oldindent;

    if (multiline) {
      break_line();
      dump_literal("}");
    } else {
      dump_literal(" }");
    }
  }
}

std::string LuaDescribe::luap_describe(lua_State *L, int index) {
  index = absolute(L, index);
  indent_ = 0;
  column_ = 0;
  dump_ = std::stringstream();

  /* Create a table to hold the ancestors for checking for cycles
   * when printing table hierarchies. */

  lua_newtable(L);
  ancestors_ = luaL_ref(L, LUA_REGISTRYINDEX);

  describe(L, index);

  luaL_unref(L, LUA_REGISTRYINDEX, ancestors_);

  return dump_.str();
}
