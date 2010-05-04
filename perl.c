#include <EXTERN.h>
#include <perl.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static PerlInterpreter *my_perl;

/********************************* Utilities **********************************/
void push_sv(lua_State *L, SV *sv)
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
    SV *sv, *errsv;
    const char *perl = luaL_checkstring(L, 1);

    sv = eval_pv(perl, 0);
    errsv = get_sv("@", GV_ADD);
    if(SvOK(errsv) && SvCUR(errsv)) {
	lua_pushnil(L);
	push_sv(L, errsv);

	return 2;
    } else {
	push_sv(L, sv);
	return 1;
    }
}

static luaL_Reg perl_interpreter_methods[] = {
    { "eval", perl_interp_eval },
    { NULL, NULL }
};

/*************************** Module Initialization ****************************/

EXTERN_C void boot_DynaLoader(pTHX_ CV* cv);

static void xs_init(pTHX)
{
    char *file = __FILE__;

    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

int luaopen_perl(lua_State *L)
{
    int fake_argc = 3;
    char *fake_argv[] = { "", "-e", "0" };

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
