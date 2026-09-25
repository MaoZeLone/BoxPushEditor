#pragma once

#include "CoreMinimal.h"
#include "Data/InteractableState.h"
#include "UObject/Object.h"
#include "VisualComps.generated.h"

class UTexture2D;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class BOXPUSH_API UInteractableVisualComp : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FName CompId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FName ParentId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector RelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<FName> VisibleInStates;
};

/** 挂在交互物、地形的表现上。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Visual Sprite"))
class BOXPUSH_API UVisualSpriteComp : public UInteractableVisualComp
{
	GENERATED_BODY()

public:
	/** 空着就不画。宽高为 0 用整张；宽高大于 0 从这张贴图上裁。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "贴图"))
	TSoftObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceW = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceH = 0;

	/** 0 表示收进一格（和方块占地相同）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0", DisplayName = "世界边长"))
	float WorldSpan = 0.f;
};

UENUM()
enum class EVisualTaskType : uint8
{
	SetVisible UMETA(DisplayName = "可视度"),
	SetSprite UMETA(DisplayName = "贴图"),
	SetBlocking UMETA(DisplayName = "挡路"),
};

/** 一条表现任务。指向 SpriteComps 里的 CompId。 */
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

/** 停在这个状态时要做的表现任务。 */
USTRUCT(BlueprintType)
struct BOXPUSH_API FStateVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (GetOptions = "GetVisualStateOptions"))
	FName StateId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "任务"))
	TArray<FVisualTask> Tasks;
};

/** 刚走过这条转移时，在状态任务之后再做的表现任务。 */
USTRUCT(BlueprintType)
struct BOXPUSH_API FTransitionVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (GetOptions = "GetVisualStateOptions", ToolTip = "Empty = any current state"))
	FName FromState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	EBoxTransitionCondition Condition = EBoxTransitionCondition::OnEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual",
		meta = (EditCondition = "Condition == EBoxTransitionCondition::OnEvent", EditConditionHides))
	FName EventId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (GetOptions = "GetVisualStateOptions"))
	FName ToState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "任务"))
	TArray<FVisualTask> Tasks;
};

/** 这一步真正走过的转移。开局和撤回不填。 */
USTRUCT()
struct BOXPUSH_API FVisualTransitionCue
{
	GENERATED_BODY()

	UPROPERTY()
	FName InstanceId;

	UPROPERTY()
	FName FromState;

	UPROPERTY()
	EBoxTransitionCondition Condition = EBoxTransitionCondition::OnEvent;

	UPROPERTY()
	FName EventId;

	UPROPERTY()
	FName ToState;
};
