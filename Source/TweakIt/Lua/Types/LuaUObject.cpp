#include "LuaUObject.h"

#include "LuaUClass.h"
#include <string>

#include "TweakIt/TweakItModule.h"
#include "TweakIt/Helpers/TIReflection.h"
using namespace std;

int LuaUObject::lua__index(lua_State* L) {
	LuaUObject* self = Get(L);
	FString PropertyName = luaL_checkstring(L, 2);
	LOGF("Indexing a LuaUObject with %s",*PropertyName)
	if (PropertyName == "GetClass") {
		lua_pushcfunction(L, lua_GetClass);
	} else if (PropertyName == "DumpProperties") {
		lua_pushcfunction(L, lua_DumpProperties);
	}
	UProperty* Property = FTIReflection::FindPropertyByName(self->Object->GetClass(), *PropertyName);
	if (!Property->IsValidLowLevel()) {
		lua_pushnil(L);
		return 1; 
	}
	LOG("Found property")
	PropertyToLua(L, Property, self->Object);
	return 1;
}

int LuaUObject::lua__newindex(lua_State* L) {
	{
		LuaUObject* self = Get(L);
		FString PropertyName = luaL_checkstring(L, 2);
		LOGF("Newindexing a LuaUObject with %s",*PropertyName)
		UProperty* Property = FTIReflection::FindPropertyByName(self->Object->GetClass(), *PropertyName);
		if (!Property->IsValidLowLevel()) {
			LOGF("No property '%s' found", *PropertyName)
			return 0;
		}
		LOG("Found property")
		LuaToProperty(L, Property, self->Object, 3);
		return 0;
	}
}

int LuaUObject::lua_DumpProperties(lua_State* L) {
	LuaUObject* self = Get(L);
	LOGFS(FString("Dumping the properties for " + self->Object->GetName()))
	if (self->Object->IsA(AActor::StaticClass())) {
		AActor* Actor = Cast<AActor>(self->Object);
		TArray<UActorComponent*> components;
		Actor->GetComponents(components);
		for (auto component : components) {
			LOGFS(component->GetName())
		}
	}
	for (UProperty* Property = self->Object->GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext
	) {
		LOGFS(Property->GetName())
	}
	return 0;
}

int LuaUObject::ConstructObject(lua_State* L, UObject* Object) {
	LOG("Constructing a LuaUObject")
	if (!Object->IsValidLowLevel()) {
		LOG("Trying to construct a LuaUObject from an invalid object")
		lua_pushnil(L);
		return 1;
	}
	LuaUObject* ReturnedInstance = static_cast<LuaUObject*>(lua_newuserdata(L, sizeof(LuaUObject)));
	new(ReturnedInstance) LuaUObject{Object};
	luaL_getmetatable(L, LuaUObject::Name);
	lua_setmetatable(L, -2);
	return 1;
}

int LuaUObject::lua__gc(lua_State* L) {
	LuaUObject* self = Get(L);
	self->~LuaUObject();
	return 0;
}

int LuaUObject::lua_GetClass(lua_State* L) {
	LuaUObject* self = Get(L);
	LuaUClass::ConstructClass(L, self->Object->GetClass());
	return 1;
}

int LuaUObject::lua__tostring(lua_State* L) {
	LuaUObject* self = Get(L);
	lua_pushstring(L, TCHAR_TO_UTF8(*self->Object->GetName()));
	return 1;
}

void LuaUObject::RegisterMetadata(lua_State* L)
{
	RegisterMetatable(L, Name, Metadata);
}

LuaUObject* LuaUObject::Get(lua_State* L, int i)
{
	return static_cast<LuaUObject*>(luaL_checkudata(L, i, Name));
}