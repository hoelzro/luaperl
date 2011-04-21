/*
 * Copyright (c) 2010-2011 Rob Hoelz <rob@hoelz.ro>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <EXTERN.h>
#include <perl.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef LUA_DL_DLOPEN
# include <dlfcn.h>
static void *dummy_handle = NULL;
#endif

static PerlInterpreter *my_perl;

/********************************* Utilities **********************************/
static void push_sv(lua_State *L, SV *sv)
{
    if(! SvOK(sv)) {
	lua_pushnil(L);
    } else if(SvIOK(sv)) {
	lua_pushinteger(L, SvIV(sv));
    } else if(SvNOK(sv)) {
	lua_pushnumber(L, SvNV(sv));
    } else if(SvPOK(sv)) {
	lua_pushstring(L, SvPVX(sv));
    } else {
	printf("WTF!\n");
    }
}

/* If throw is true, the error will be throwing using lua_error.
 * If throw is false, nil will be pushed onto the stack, then the error.
 */
static int check_perl_error(lua_State *L, int throw)
{
    SV *errsv = get_sv("@", GV_ADD);
    if(SvOK(errsv) && SvCUR(errsv)) {
	if(! throw) {
	    lua_pushnil(L);
	}
	push_sv(L, errsv);
	if(throw) {
	    return lua_error(L);
	}
	return 1;
    }
    return 0;
}

/******************************** Metamethods *********************************/
static int perl_interp_gc(lua_State *L)
{
    perl_destruct(my_perl);
    perl_free(my_perl);
    PERL_SYS_TERM();

    return 0;
}

static perl_interp_index(lua_State *L)
{
    lua_getfenv(L, 1);
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);

    return 1;
}

static luaL_Reg perl_interpreter_mmethods[] = {
    { "__gc", perl_interp_gc },
    { "__index", perl_interp_index },
    { NULL, NULL }
};

/********************************** Methods ***********************************/
static int perl_interp_eval(lua_State *L)
{
    SV *sv;
    const char *error = NULL;
    const char *perl = luaL_checkstring(L, 1);

    sv = eval_pv(perl, 0);
    if(! check_perl_error(L, 0)) {
	push_sv(L, sv);
	return 1;
    }
    return 2;
}

static luaL_Reg perl_interpreter_methods[] = {
    { "eval", perl_interp_eval },
    { NULL, NULL }
};

/*************************** Module Initialization ****************************/

EXTERN_C void boot_DynaLoader(pTHX_ CV* cv);

static void boot_threads(pTHX_ CV* cv)
{
    croak("Unable to load threads when embedded in Lua!");
}

static void xs_init(pTHX)
{
    char *file = __FILE__;

    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
    newXS("threads::bootstrap", boot_threads, file);
}

#ifdef LUA_DL_DLOPEN
static int try_load(lua_State *L, const char *path)
{
    if(! strstr(path, LUA_PATH_MARK)) {
	return 0;
    }
    path = luaL_gsub(L, path, LUA_PATH_MARK, "luaperl_dummy");
    dummy_handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    lua_pop(L, 1);
    return dummy_handle ? 1 : 0;
}

static void load_dummy(lua_State *L)
{
    const char *package_cpath = NULL;
    const char *path = NULL;
    char *saveptr = NULL;
    int loaded = 0;

    lua_getglobal(L, "package");
    if(lua_isnil(L, -1)) {
	luaL_error(L, "Unable to find package in global scope!");
    }
    lua_getfield(L, -1, "cpath");
    if(lua_isnil(L, -1)) {
	luaL_error(L, "Unable to find cpath under package!");
    }

    package_cpath = luaL_checkstring(L, -1);
    lua_pop(L, 2);
    path = strtok_r(package_cpath, LUA_PATHSEP, &saveptr);
    loaded = try_load(L, path);
    while(! loaded && (path = strtok_r(NULL, LUA_PATHSEP, &saveptr))) {
	loaded = try_load(L, path);
	if(loaded) {
	    break;
	}
    }
    if(! loaded) {
	luaL_error(L, "Unable to find luaperl_dummy.so!");
    }
}
#else
# define load_dummy(L)
#endif

int luaopen_perl(lua_State *L)
{
    int fake_argc = 3;
    char *fake_argv[] = { "", "-e", "0" };

    load_dummy(L);

    lua_newuserdata(L, 0);

    PERL_SYS_INIT(&fake_argc, &fake_argv);
    my_perl = perl_alloc();
    perl_construct(my_perl);
    perl_parse(my_perl, xs_init, fake_argc, fake_argv, NULL);

    luaL_newmetatable(L, "PerlInterpreter");
    luaL_register(L, NULL, perl_interpreter_mmethods);
    lua_setmetatable(L, -2);

    lua_createtable(L, 0, sizeof(perl_interpreter_methods) / sizeof(luaL_Reg) - 1);
    luaL_register(L, NULL, perl_interpreter_methods);
    lua_setfenv(L, -2);

    lua_pushvalue(L, -1);
    lua_setglobal(L, "perl");

    return 1;
}
