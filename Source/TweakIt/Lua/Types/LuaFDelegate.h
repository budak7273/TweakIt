#pragma once
#include "CoreMinimal.h"

#include "TweakIt/Lua/Lua.h"

int Lua_MakeSubclass(lua_State* L); // Forward declaration. I hate C++

struct FLuaFDelegate
{
	UFunction* SignatureFunction;
	FScriptDelegate* Delegate;
	
	static int Construct(lua_State* L, UFunction* SignatureFunction, FScriptDelegate* Delegate);
	static FLuaFDelegate* Get(lua_State* L, int Index = 1);
	
	FString ToString() const;

	static int Lua_Bind(lua_State* L);
	static int Lua_IsBound(lua_State* L);
	static int Lua_Unbind(lua_State* L);
	static int Lua_Wait(lua_State* L);
	static int Lua_Trigger(lua_State* L);

	static int Lua__index(lua_State* L);
	static int Lua__call(lua_State* L);
	static int Lua__tostring(lua_State* L);

	static void RegisterMetadata(lua_State* L);
	inline static const char* Name = "FDelegate";

private:
	inline static TArray<luaL_Reg> Metadata = {
		{"__index", Lua__index},
		{"__call", Lua__call},
		{"__tostring", Lua__tostring},
	};

	inline static TMap<FString, lua_CFunction> Methods = {
		{"Bind", Lua_Bind},
		{"IsBound", Lua_IsBound},
		{"Unbind", Lua_Unbind},
		{"Wait", Lua_Wait}
	};
};
