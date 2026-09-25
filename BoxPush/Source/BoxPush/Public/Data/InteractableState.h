#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
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
	BeginOverlap UMETA(DisplayName = "Begin Overlap"),
	EndOverlap UMETA(DisplayName = "End Overlap"),
};

/** 状态动作 FireEvent 广播的载荷。本关所有实例用自己的转移表对 EventId。 */
USTRUCT()
struct BOXPUSH_API FBoxInteractableEvent
{
	GENERATED_BODY()

	UPROPERTY()
	FName EventId;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action",
		meta = (EditCondition = "Type == EBoxStateActionType::FireEvent", EditConditionHides, DisplayName = "可实例重载"))
	bool bAllowOverride = false;
};

UENUM()
enum class EVisualTaskType : uint8
{
	SetVisible UMETA(DisplayName = "可视度"),
	SetSprite UMETA(DisplayName = "贴图"),
	SetBlocking UMETA(DisplayName = "挡路"),
};

/** 一条表现任务。指向 SpriteComps 里的 CompId。挡路不指向贴图。 */
USTRUCT(BlueprintType)
struct BOXPUSH_API FVisualTask
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task")
	EVisualTaskType Type = EVisualTaskType::SetVisible;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (GetOptions = "GetVisualCompOptions", EditCondition = "Type != EVisualTaskType::SetBlocking", EditConditionHides))
	FName CompId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetBlocking", EditConditionHides, DisplayName = "挡人"))
	bool bBlocksPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetBlocking", EditConditionHides, DisplayName = "挡推"))
	bool bBlocksPush = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetVisible", EditConditionHides, DisplayName = "可见"))
	bool bVisible = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetSprite", EditConditionHides, DisplayName = "贴图"))
	TSoftObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetSprite", EditConditionHides, ClampMin = "0"))
	int32 SourceX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetSprite", EditConditionHides, ClampMin = "0"))
	int32 SourceY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetSprite", EditConditionHides, ClampMin = "0"))
	int32 SourceW = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task",
		meta = (EditCondition = "Type == EVisualTaskType::SetSprite", EditConditionHides, ClampMin = "0"))
	int32 SourceH = 0;
};

/** One transition: from-state + condition -> to-state. */
USTRUCT(BlueprintType)
struct BOXPUSH_API FInteractableTransitionDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition",
		meta = (GetOptions = "GetVisualStateOptions", ToolTip = "空 = 任意当前状态"))
	FName FromState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	EBoxTransitionCondition Condition = EBoxTransitionCondition::OnEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition",
		meta = (EditCondition = "Condition == EBoxTransitionCondition::OnEvent", EditConditionHides))
	FName EventId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition",
		meta = (EditCondition = "Condition == EBoxTransitionCondition::OnEvent", EditConditionHides, DisplayName = "可实例重载"))
	bool bAllowOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (GetOptions = "GetVisualStateOptions"))
	FName ToState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (DisplayName = "表现"))
	TArray<FVisualTask> Tasks;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State", meta = (DisplayName = "表现"))
	TArray<FVisualTask> Tasks;

	/** 误挂在状态下的转移。加载时抬回定义上的转移表。 */
	UPROPERTY(meta = (DeprecatedProperty))
	TArray<FInteractableTransitionDef> Transitions;

	UPROPERTY(meta = (DeprecatedProperty))
	TArray<FName> EnterEvents;
};
