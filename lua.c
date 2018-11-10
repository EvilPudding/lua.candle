#include "lua.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


c_lua_t *c_lua_new()
{
	c_lua_t *self = component_new("lua");
	self->state = luaL_newstate();
	if(self->state) luaL_openlibs(self->state);
	return self;
}

int c_lua_loadexpr(c_lua_t *self, const char *expr, char **pmsg)
{
	int err;
	char *buf;

	if(!self->state)
	{
		if(pmsg) *pmsg = strdup("LE library not initialized");
		return LUA_NOREF;
	}
	buf = malloc(strlen(expr)+8);
	if(!buf)
	{
		if(pmsg) *pmsg = strdup("Insufficient memory");
		return LUA_NOREF;
	}
	strcpy(buf, "return ");
	strcat(buf, expr);
	err = luaL_loadstring(self->state,buf);
	free(buf);
	if(err)
	{
		if(pmsg) *pmsg = strdup(lua_tostring(self->state,-1));
		lua_pop(self->state,1);
		return LUA_NOREF;
	}
	if(pmsg) *pmsg = NULL;
	return luaL_ref(self->state, LUA_REGISTRYINDEX);
}

double c_lua_eval(c_lua_t *self, int ref, char **pmsg)
{
	int err;
	double ret;

	if(!self->state)
	{
		if(pmsg) *pmsg = strdup("LE library not initialized");
		return 0;
	}
	lua_rawgeti(self->state, LUA_REGISTRYINDEX, ref);
	err = lua_pcall(self->state,0,1,0);
	if(err)
	{
		if(pmsg) *pmsg = strdup(lua_tostring(self->state,-1));
		lua_pop(self->state,1);
		return 0;
	}
	if(pmsg) *pmsg = NULL;
	ret = (double)lua_tonumber(self->state,-1);
	lua_pop(self->state,1);
	return ret;
}

void c_lua_unref(c_lua_t *self, int ref)
{
	if(!self->state) return;
	luaL_unref(self->state, LUA_REGISTRYINDEX, ref);    
}

void c_lua_setvar(c_lua_t *self, char *name, double value)
{
	if(!self->state) return;
	lua_pushnumber(self->state,value);
	lua_setglobal(self->state,name);
}

double c_lua_getvar(c_lua_t *self, char *name)
{
	double ret;

	if(!self->state)
		return 0;
	lua_getglobal(self->state,name);
	ret = (double)lua_tonumber(self->state,-1);
	lua_pop(self->state, 1);
	return ret;
}

static int c_lua_expr_del(c_lua_t *self, int *ref)
{
	c_lua_unref(self, *ref);
	return STOP;
}

static int c_lua_expr_load(c_lua_t *self, const char *expr, int *result)
{
	char *msg = NULL;
	*result = c_lua_loadexpr(self, expr, &msg);
	if(msg)
	{
		*result = 0;
		puts(msg);
		free(msg);
	}

	return STOP;
}

static int c_lua_expr_eval(c_lua_t *self, int *ref, float *result)
{
	char *msg = NULL;
	*result = c_lua_eval(self, *ref, &msg);
	if (msg)
	{
		*result = NAN;
		puts(msg);
		free(msg);
	}
	return STOP;
}

static int c_lua_expr_var(c_lua_t *self,
		struct set_var *set,
		struct set_var *get)
{
	if(set) c_lua_setvar(self, set->ident, set->value);
	if(get) get->value = c_lua_getvar(self, get->ident);

	return STOP;
}

REG()
{
	ct_t *ct = ct_new("lua", sizeof(c_lua_t), NULL, NULL, 0);

	ct_listener(ct, WORLD | 100, sig("expr_eval"), c_lua_expr_eval);
	ct_listener(ct, WORLD | 100, sig("expr_load"), c_lua_expr_load);
	ct_listener(ct, WORLD | 100, sig("expr_del"), c_lua_expr_del);
	ct_listener(ct, WORLD | 100, sig("expr_var"), c_lua_expr_var);
}

