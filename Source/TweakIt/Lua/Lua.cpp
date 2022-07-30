#include "Lua.h"
#include <string>
#include "CoreMinimal.h"
#include "TweakIt/Lua/lib/lua.hpp"
#include "FGRecipeManager.h"
#include "FTILuaFuncManager.h"
#include "IPlatformFilePak.h"
#include "LuaState.h"
#include "Scripting/TIScriptOrchestrator.h"
#include "TweakIt/TweakItTesting.h"
#include "TweakIt/Logging/FTILog.h"
#include "TweakIt/Helpers/TIReflection.h"
#include "TweakIt/Helpers/TIContentRegistration.h"

using namespace std;

void FTILua::LuaT_ExpectLuaFunction(lua_State* L, int Index)
{
	luaL_argexpected(L, lua_isfunction(L, Index) && !lua_iscfunction(L, Index), Index, "Lua Function");
}

template<typename T>
T* FTILua::LuaT_CheckLightUserdata(lua_State* L, int Index)
{
	luaL_argexpected(L, lua_isuserdata(L, Index), Index, "light userdata");
	return static_cast<T*>(lua_touserdata(L, Index));
}

bool FTILua::LuaT_OptBoolean(lua_State* L, int Index, bool Default)
{
	if (lua_isboolean(L, Index))
	{
		return static_cast<bool>(lua_toboolean(L, Index));
	}
	return Default;
}

void FTILua::RegisterMetatable(lua_State* L, const char* Name, TArray<luaL_Reg> Regs)
{
	luaL_newmetatable(L, Name);
	for (auto Reg : Regs)
	{
		RegisterMethod(L, Reg);
	}
}

void FTILua::RegisterMethod(lua_State* L, luaL_Reg Reg)
{
	lua_pushstring(L, Reg.name);
	lua_pushcfunction(L, Reg.func);
	lua_settable(L, -3);
}

bool FTILua::CheckLua(lua_State* L, int Returned)
{
	if (Returned != LUA_OK)
	{
		string ErrorMsg = lua_tostring(L, -1);
		LOG(ErrorMsg)
		return false;
	}
	return true;
}

void FTILua::StackDump(lua_State* L)
{
	int Top = lua_gettop(L);
	for (int i = 1; i <= Top; i++)
	{
		int t = lua_type(L, i);
		string s = lua_typename(L, t);
		LOG(s);
		LOG(""); // put a separator 
	}
}

// TODO: Test return value
int FTILua::CallUFunction(lua_State* L, UObject* Object, UFunction* Function)
{
	LOG("Calling UFunction")
	check(Function->IsValidLowLevel())
	check(Object->IsValidLowLevel())
	TArray<uint8> Params;
	Params.SetNumZeroed(Function->ParmsSize);
	TArray<uint8> Return;
	FProperty* ReturnProperty = Function->GetReturnProperty();
	if (ReturnProperty)
	{
		Return.SetNumZeroed(ReturnProperty->ElementSize);
	}
	FFrame Frame = FFrame(Object, Function, Params.GetData());
	int i = 2;
	for (FProperty* Prop = Function->PropertyLink; Prop; Prop = Prop->PropertyLinkNext)
	{
		LuaToProperty(L, Prop, Params.GetData(), i);
		i++;
	}
	Function->Invoke(Object, Frame, Return.GetData());
	if (ReturnProperty)
	{
		PropertyToLua(L, ReturnProperty, Return.GetData());
	}
	return ReturnProperty->IsValidLowLevel();
}

// Mostly borrowed from FIN's source. Thanks Pana !
void FTILua::PropertyToLua(lua_State* L, FField* Field, void* Container)
{
	LOGF("Transforming from Property %s to Lua", *Field->GetName());
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Field))
	{
		lua_pushboolean(L, BoolProp->GetPropertyValue_InContainer(Container));
	}
	else if (FInt8Property* Int8Prop = CastField<FInt8Property>(Field))
	{
		lua_pushinteger(L, Int8Prop->GetPropertyValue_InContainer(Container));
	}
	else if (FInt16Property* Int16Prop = CastField<FInt16Property>(Field))
	{
		lua_pushinteger(L, Int16Prop->GetPropertyValue_InContainer(Container));
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Field))
	{
		lua_pushinteger(L, IntProp->GetPropertyValue_InContainer(Container));
	}
	else if (FInt64Property* Int64Prop = CastField<FInt64Property>(Field))
	{
		lua_pushinteger(L, Int64Prop->GetPropertyValue_InContainer(Container));
	}
	else if (FUInt16Property* UInt16Prop = CastField<FUInt16Property>(Field))
	{
		lua_pushinteger(L, UInt16Prop->GetPropertyValue_InContainer(Container));
	}
	else if (FUInt32Property* UInt32Prop = CastField<FUInt32Property>(Field))
	{
		lua_pushinteger(L, UInt32Prop->GetPropertyValue_InContainer(Container));
	}
	else if (FUInt64Property* UInt64Prop = CastField<FUInt64Property>(Field))
	{
		lua_pushinteger(L, UInt64Prop->GetPropertyValue_InContainer(Container));
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Field))
	{
		lua_pushnumber(L, FloatProp->GetPropertyValue_InContainer(Container));
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Field))
	{
		lua_pushnumber(L, DoubleProp->GetPropertyValue_InContainer(Container));
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Field))
	{
		FString String = StrProp->GetPropertyValue_InContainer(Container);
		lua_pushstring(L, TCHAR_TO_UTF8(*String));
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Field))
	{
		FString String = NameProp->ContainerPtrToValuePtr<FName>(Container)->ToString();
		lua_pushstring(L, TCHAR_TO_UTF8(*String));
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Field))
	{
		FString String = TextProp->ContainerPtrToValuePtr<FText>(Container)->ToString();
		lua_pushstring(L, TCHAR_TO_UTF8(*String));
	}
	else if (FClassProperty* ClassProp = CastField<FClassProperty>(Field))
	{
		FLuaUClass::ConstructClass(L, *ClassProp->ContainerPtrToValuePtr<UClass*>(Container));
	}
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Field))
	{
		int64 EnumValue = *EnumProp->ContainerPtrToValuePtr<uint8>(Container);
		UEnum* Enum = EnumProp->GetEnum();
		if (!Enum->IsValidEnumValue(EnumValue))
		{
			luaL_error(L, "Enum value wasn't valid. Please report this to Feyko");
		}
		FName Name = Enum->GetNameByValue(EnumValue);
		FString StringName = Name.ToString();
		lua_pushstring(L, TCHAR_TO_UTF8(*StringName));
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Field))
	{
		void* StructValue = StructProp->ContainerPtrToValuePtr<void>(Container);
		FLuaUStruct::ConstructStruct(L, StructProp->Struct, StructValue);
	}
	else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Field))
	{
		FLuaUObject::ConstructObject(L, ObjectProp->GetPropertyValue_InContainer(Container));
	}
	else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Field))
	{
		FLuaTArray::ConstructArray(L, ArrayProp, Container);
	}
	else if (FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Field))
	{
		FScriptDelegate* Value = DelegateProp->ContainerPtrToValuePtr<FScriptDelegate>(Container);
		FLuaFDelegate::Construct(L, DelegateProp->SignatureFunction, Value);
	}
	else
	{
		LOG("DIDN'T MATCH ANY CAST FLAGS")
		lua_pushnil(L);
	}
}

// Mostly borrowed from FIN's source. Thanks Pana !
void FTILua::LuaToProperty(lua_State* L, FField* Field, void* Container, int Index)
{
	LOGF("Transforming from Lua to Property %s", *Field->GetName());
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Field))
	{
		luaL_argexpected(L, lua_isboolean(L, Index), Index, "boolean");
		BoolProp->SetPropertyValue_InContainer(Container, static_cast<bool>(lua_toboolean(L, Index)));
	}
	
	else if (FInt8Property* Int8Prop = CastField<FInt8Property>(Field))
	{
		Int8Prop->SetPropertyValue_InContainer(Container, luaL_checkinteger(L, Index));
	}
	else if (FInt16Property* Int16Prop = CastField<FInt16Property>(Field))
	{
		Int16Prop->SetPropertyValue_InContainer(Container, luaL_checkinteger(L, Index));
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Field))
	{
		IntProp->SetPropertyValue_InContainer(Container, luaL_checkinteger(L, Index));
	}
	else if (FInt64Property* Int64Prop = CastField<FInt64Property>(Field))
	{
		*Int64Prop->ContainerPtrToValuePtr<std::int64_t>(Container) = luaL_checkinteger(L, Index);
	}
	else if (FUInt16Property* UInt16Prop = CastField<FUInt16Property>(Field))
	{
		UInt16Prop->SetPropertyValue_InContainer(Container, luaL_checkinteger(L, Index));
	}
	else if (FUInt32Property* UInt32Prop = CastField<FUInt32Property>(Field))
	{
		UInt32Prop->SetPropertyValue_InContainer(Container, luaL_checkinteger(L, Index));
	}
	else if (FUInt64Property* UInt64Prop = CastField<FUInt64Property>(Field))
	{
		UInt64Prop->SetPropertyValue_InContainer(Container, luaL_checkinteger(L, Index));
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Field))
	{
		FloatProp->SetPropertyValue_InContainer(Container, luaL_checknumber(L, Index));
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Field))
	{
		DoubleProp->SetPropertyValue_InContainer(Container, luaL_checknumber(L, Index));
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Field))
	{
		StrProp->SetPropertyValue_InContainer(Container, luaL_checkstring(L, Index));
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Field))
	{
		NameProp->SetPropertyValue_InContainer(Container, luaL_checkstring(L, Index));
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Field))
	{
		TextProp->SetPropertyValue_InContainer(Container, FText::FromString(luaL_checkstring(L, Index)));
	}
	else if (FClassProperty* ClassProp = CastField<FClassProperty>(Field))
	{
		FLuaUClass* LuaUClass = FLuaUClass::Get(L, Index);
		ClassProp->SetPropertyValue_InContainer(Container, LuaUClass->Class);
	}
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Field))
	{
		FName EnumValueName = luaL_checkstring(L, Index);
		UEnum* Enum = EnumProp->GetEnum();
		if (!Enum->IsValidEnumName(EnumValueName))
		{
			luaL_error(L, "invalid enum value %s for enum type %s. Example value: %s",
			           TCHAR_TO_UTF8(*EnumValueName.ToString()), TCHAR_TO_UTF8(*Enum->GetName()),
			           TCHAR_TO_UTF8(*Enum->GetNameByIndex(0).ToString()));
			return;
		}
		int64 EnumValue = Enum->GetValueByName(EnumValueName);
		*EnumProp->ContainerPtrToValuePtr<uint8>(Container) = static_cast<uint8>(EnumValue);
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Field))
	{
		FLuaUStruct* rStruct = FLuaUStruct::Get(L, Index);
		if (!(StructProp->Struct->GetFullName() == rStruct->Struct->GetFullName()))
		{
			luaL_error(L, "Mismatched struct types (%s <- %s)",
			           TCHAR_TO_UTF8(*StructProp->Struct->GetName()), TCHAR_TO_UTF8(*rStruct->Struct->GetName()));
			return;
		}
		StructProp->CopyCompleteValue_InContainer(Container, rStruct->Values);
		void* NewValue = StructProp->ContainerPtrToValuePtr<void>(Container);
		rStruct->Values = NewValue;
		rStruct->Owning = false;
	}
	else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Field))
	{
		UObject* Object = lua_isnil(L, Index) ? nullptr : FLuaUObject::Get(L, Index)->Object;
		ObjectProp->SetPropertyValue_InContainer(Container, Object);
	}
	else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Field))
	{
		luaL_argexpected(L, lua_istable(L, Index), Index, "array");
		FScriptArray* ArrayValue = ArrayProp->ContainerPtrToValuePtr<FScriptArray>(Container);

		int InputLen = luaL_len(L, Index);
		int ElementSize = ArrayProp->Inner->ElementSize;
		ArrayValue->Empty(InputLen, ElementSize);
		ArrayValue->AddZeroed(InputLen, ElementSize);

		uint8* Data = static_cast<uint8*>(ArrayValue->GetData());
		for (int i = 0; i < InputLen; ++i)
		{
			lua_pushinteger(L, i + 1);
			lua_gettable(L, Index);
			LuaToProperty(L, ArrayProp->Inner, Data + ElementSize * i, Index + 1);
			lua_pop(L, 1);
		}
	}
	else if (FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Field))
	{
		luaL_error(L, "Delegate assignment is not yet supported");
	}
	else
	{
		luaL_error(L, "Property type not supported. Please report this to Feyko");
	}
}


int FTILua::Lua_GetClass(lua_State* L)
{
	LOG("Getting a class");
	FString ClassName = luaL_checkstring(L, 1);
	FString Package = lua_isstring(L, 2) ? lua_tostring(L, 2) : "FactoryGame";
	UClass* Class = FTIReflection::FindClassByName(ClassName, Package);
	FLuaUClass::ConstructClass(L, Class);
	return 1;
}

int FTILua::Lua_MakeStructInstance(lua_State* L)
{
	LOG("Making a struct instance")
	UStruct* BaseStruct;
	if (lua_isuserdata(L, 1))
	{
		BaseStruct = FLuaUStruct::Get(L)->Struct;
	}
	else
	{
		FString StructName = luaL_checkstring(L, 1);
		FString Package = lua_isstring(L, 2) ? lua_tostring(L, 2) : "FactoryGame";
		BaseStruct = FTIReflection::FindStructByName(StructName, Package);
		if (!BaseStruct)
		{
			LOGF("Couldn't find a struct with the name %s", *StructName)
			lua_pushnil(L);
			return 1;
		}
	}
	if (!BaseStruct->IsValidLowLevel())
	{
		LOG("Trying to make an instance of an invalid struct")
		lua_pushnil(L);
		return 1;
	}
	void* Instance = FTIReflection::MakeStructInstance(BaseStruct);
	FLuaUStruct::ConstructStruct(L, BaseStruct, Instance, true);
	return 1;
}

int FTILua::Lua_MakeSubclass(lua_State* L)
{
	UClass* ParentClass = FLuaUClass::Get(L)->Class;
	FString Name = luaL_checkstring(L, 2);
	UClass* GeneratedClass = FTIReflection::GenerateUniqueSimpleClass(*("/TweakIt/Generated/" + Name), *Name,ParentClass);
	FLuaUClass::ConstructClass(L, GeneratedClass);
	return 1;
}

int FTILua::Lua_UnlockRecipe(lua_State* L)
{
	UClass* Class = FLuaUClass::Get(L)->Class;
	if (!lua_isuserdata(L, 2))
	{
		lua_getglobal(L, "WorldContext");
	}
	UObject* WorldContext = FLuaUObject::Get(L, 2)->Object;
	FTIContentRegistration::UnlockRecipe(Class, WorldContext);
	return 0;
}

int FTILua::Lua_LoadObject(lua_State* L)
{
	LOG("Loading an object")
	FString Path = lua_tostring(L, 1);
	UClass* Class = lua_isuserdata(L, 2) ? FLuaUClass::Get(L, 2)->Class : UObject::StaticClass();
	UObject* Object = StaticLoadObject(Class, nullptr, *Path);
	FLuaUObject::ConstructObject(L, Object);
	return 1;
}

int FTILua::Lua_Print(lua_State* L)
{
	FString String = luaL_checkstring(L, 1);
	LOG(String)
	return 0;
}

int FTILua::Lua_Test(lua_State* L)
{
	LOG("Running Lua_Test")
	// FPlatformProcess::Sleep(4);
	LOG(UTweakItTesting::Get()->Delegate.GetFunctionName())
	LOG(UTweakItTesting::Get()->Delegate.IsBound())
	LOG("Executing")
	UTweakItTesting::Get()->Delegate.Execute("String", 42);
	return 0;
}

int FTILua::Lua_WaitForEvent(lua_State* L)
{
	FString Event = luaL_checkstring(L, 1);
	bool Unique = LuaT_OptBoolean(L, 2, true);
	if (Unique && FTIScriptOrchestrator::Get()->HasEventPassed(Event))
	{
		return 0;
	}
	FLuaState* State = FLuaState::Get(L);
	State->EventWaitedFor = Event;
	lua_yield(L, 0);
	return 0;
}

int FTILua::Lua_WaitForMod(lua_State* L)
{
	FString Event = luaL_checkstring(L, 1);
	FString Lifecycle = "Module";
	if (lua_isstring(L, 2))
	{
		Lifecycle = luaL_checkstring(L, 2);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	lua_pushstring(L, TCHAR_TO_UTF8(*FTIScriptOrchestrator::MakeEventForMod(Event, Lifecycle)));
	Lua_WaitForEvent(L);
	return 0;
}

int FTILua::Lua_DumpFunction(lua_State* L)
{
	FString Name = luaL_checkstring(L, 1);
	FTILuaFuncManager::DumpFunction(L, Name, 2);
	return 0;
}

int FTILua::Lua_LoadFunction(lua_State* L)
{
	FString Name = luaL_checkstring(L, 1);
	TResult<FLuaFunc> Func = FTILuaFuncManager::GetSavedLuaFunc(Name);
	if (!Func)
	{
		luaL_error(L, "Function %s not previously dumped", TCHAR_TO_UTF8(*Name));
	}
	FTILuaFuncManager::LoadFunction(L, *Func, Name);
	lua_setglobal(L, TCHAR_TO_UTF8(*Name));
	return 1;
}
