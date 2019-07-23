#include "automation/lua_repl_context.h"
#include <cstring>

#if LUA_VERSION_NUM == 501
#define EOF_MARKER "'<eof>'"
#else
#define EOF_MARKER "<eof>"
#endif

namespace {

int PushTraceback(lua_State *L) {
  lua_Debug ar;

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

  int i;
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

std::string DoLuaEval(lua_State *L, int n, int *status) {
  /* Push the error handler onto the stack. */
  int h = lua_gettop(L) - n;
  lua_pushcfunction(L, PushTraceback);
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

LuaReplContext::LuaReplContext(::lua_State *lua_state)
    : describe_(80), lua_state_(lua_state) {}

std::string LuaReplContext::ExecuteLua(int* status) {
  int h_0 = lua_gettop(lua_state_);
  std::string result = DoLuaEval(lua_state_, 0, status);
  if (result.empty()) {
    int h = lua_gettop(lua_state_) - h_0 + 1;

    for (int i = h; i > 0; i -= 1) {
      result = describe_.Describe(lua_state_, -1);

      if (h != 1) {
        std::stringstream output;
        output << h - i + 1 << ": " << result;
        result = output.str();
      }

      /* Clean up.  We need to remove the results table as well if we
 * track results. */
      lua_settop(lua_state_, h_0 - 1);

    }
  }


  return result;
}

std::string LuaReplContext::Eval(const std::string& expression) {
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

  /* That failed, so try to execute the line as-is. */

  // Check syntax.
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
    return std::string(message, n);
  }

  if (status == LUA_ERRMEM) {
    incomplete_ = false;
    return lua_tostring(lua_state_, -1);
  }

  /* Try to execute the loaded chunk. */
  incomplete_ = false;
  return ExecuteLua(&status);
}
