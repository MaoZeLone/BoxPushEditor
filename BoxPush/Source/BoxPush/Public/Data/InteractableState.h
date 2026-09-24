#pragma once

#include "CoreMinimal.h"
#include "InteractableState.generated.h"

UENUM()
enum class EBoxStateActionPhase : uint8
{
	Enter,
	Stay,
	Exit,
};

UENUM()
enum class EBoxStateActionType : uint8
{
	None,
	FireEvent UMETA(DisplayName = "Fire Event"),
};

UENUM()
enum class EBoxTransitionCondition : uint8
{
	OnEvent UMETA(DisplayName = "On Event"),
};

/** One action row: pick a type, then fill that type's parameters. */
USTRUCT(BlueprintType)
struct BOXPUSH_API FInteractableStateActionDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EBoxStateActionType Type = EBoxStateActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action",
		meta = (EditCondition = "Type == EBoxStateActionType::FireEvent", EditConditionHides))
	FName EventId;
};

/** One transition: from-state + condition -> to-state. */
USTRUCT(BlueprintType)
struct BOXPUSH_API FInteractableTransitionDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ToolTip = "Empty = any current state"))
	FName FromState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	EBoxTransitionCondition Condition = EBoxTransitionCondition::OnEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition",
		meta = (EditCondition = "Condition == EBoxTransitionCondition::OnEvent", EditConditionHides))
	FName EventId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	FName ToState;
};

USTRUCT(BlueprintType)
struct BOXPUSH_API FInteractableStateDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	FName StateId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	bool bDefault = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	TArray<FInteractableStateActionDef> OnEnter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	TArray<FInteractableStateActionDef> OnStay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	TArray<FInteractableStateActionDef> OnExit;

	UPROPERTY(meta = (DeprecatedProperty))
	TArray<FName> EnterEvents;
};
