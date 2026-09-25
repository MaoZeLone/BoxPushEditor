#pragma once

#include "Data/InteractableState.h"

struct FBoxRuntimeInstance;

struct FBoxStateActionContext
{
	FName InstanceId;
	FName StateId;
	EBoxStateActionPhase Phase = EBoxStateActionPhase::Enter;
	FBoxRuntimeInstance* Instance = nullptr;
	TArray<FName>* FiredEvents = nullptr;
	TArray<FName>* BroadcastQueue = nullptr;
};

void ExecuteInteractableStateActions(const TArray<FInteractableStateActionDef>& Actions, const FBoxStateActionContext& Context);
