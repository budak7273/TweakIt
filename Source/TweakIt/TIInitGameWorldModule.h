﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Module/GameInstanceModule.h"
#include "Module/GameWorldModule.h"
#include "UObject/Object.h"
#include "TIInitGameWorldModule.generated.h"

UCLASS()
class TWEAKIT_API UTIInitGameWorldModule : public UGameWorldModule
{
	GENERATED_BODY()
	UTIInitGameWorldModule();
};
