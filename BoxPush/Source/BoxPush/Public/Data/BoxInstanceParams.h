#pragma once

#include "CoreMinimal.h"
#include "Data/BoxPushLevelTypes.h"
#include "Data/InteractableState.h"

class UInteractableDef;
struct FBoxRuntimeInstance;
struct FLevelValidationIssue;

struct BOXPUSH_API FBoxShownParam
{
	FName CompId;
	FName Key;
	FText Label;
	bool bCaptionAbove = false;
	EBoxParamKind Kind = EBoxParamKind::Bool;
	bool BoolValue = false;
	int32 IntValue = 0;
	int32 IntMin = 0;
	FName NameValue;
	FGameplayTag TagValue;
};

namespace BoxInstanceParams
{
	BOXPUSH_API void Collect(const UInteractableDef* Def, const TArray<FBoxInstanceOverride>& Overrides, TArray<FBoxShownParam>& Out);
	BOXPUSH_API bool FindDefault(const UInteractableDef* Def, FName CompId, FName Key, FBoxShownParam& Out);
	BOXPUSH_API void UpsertBool(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, bool Value, bool Default);
	BOXPUSH_API void UpsertInt(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, int32 Value, int32 Default);
	BOXPUSH_API void UpsertName(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, FName Value, FName Default);
	BOXPUSH_API void UpsertTag(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, FGameplayTag Value, FGameplayTag Default);
	BOXPUSH_API bool SameOverrides(const TArray<FBoxInstanceOverride>& A, const TArray<FBoxInstanceOverride>& B);
	BOXPUSH_API void AppendIssues(const FBoxLevelInstance& Inst, TArray<FLevelValidationIssue>& Out);

	BOXPUSH_API bool BlocksPlayer(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API bool BlocksPush(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API int32 PushSteps(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API bool StopOnBox(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API int32 DelayMoves(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API FGameplayTag RequiredType(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API FGameplayTag AcceptType(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API FGameplayTag TriggerAcceptType(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API FName OccupiedEvent(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API FName ClearedEvent(const FBoxRuntimeInstance& Inst);
	BOXPUSH_API FName ResolveOverriddenName(const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, FName Default);

	inline FName FireEventOverrideKey(EBoxStateActionPhase Phase, int32 Index)
	{
		return FName(*FString::Printf(TEXT("Fire.%d.%d"), static_cast<int32>(Phase), Index));
	}

	inline FName TransitionOverrideComp()
	{
		return TEXT("Transition");
	}

	inline FName TransitionOverrideKey(int32 Index)
	{
		return FName(*FString::Printf(TEXT("Listen.%d"), Index));
	}
}
