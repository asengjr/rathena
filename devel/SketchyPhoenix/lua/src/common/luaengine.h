// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _LUAENGINE_H_
#define _LUAENGINE_H_

#include <lua.h>

lua_State *L;

struct LuaCommandInfo {
	const char *command;
	lua_CFunction f;
};

#define LUA_FUNC(x) static int buildin_ ## x (lua_State *NL)
#define LUA_DEF(x) {#x, buildin_ ##x}

void do_init_luaengine( void );
void do_final_luaengine( void );
void load_script_commands( void );
void charscript_run(const char *name,const char *format,...);
void luaengine_open_config( void );
void luaengine_stackdump(lua_State *L);

#endif /* _LUAENGINE_H_ */