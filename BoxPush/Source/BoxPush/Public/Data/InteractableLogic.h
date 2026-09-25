#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "InteractableLogic.generated.h"

class UBoxBoard;
struct FBoxRuntimeInstance;

/** 逻辑组件：有就生效，同类最多一条。格子模拟只读这一数组，不读 CurrentState。 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class BOXPUSH_API UInteractableLogicComp : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
	FName CompId;

	/** True: this instance must be satisfied for the level to win. */
	virtual bool CountsTowardWin() const { return false; }

	virtual bool IsWinSatisfied(const UBoxBoard& Board, const FBoxRuntimeInstance& Self) const;
};

/** 这格有没有实体。墙用地形，不挂这个。目标 / 踏板不挂，所以箱子可以叠上去。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Blocking"))
class BOXPUSH_API UBlockingLogic : public UInteractableLogicComp
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "挡人", InstanceOverride = "bAllowBlocksPlayer"))
	bool bBlocksPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowBlocksPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "挡推", InstanceOverride = "bAllowBlocksPush"))
	bool bBlocksPush = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowBlocksPush = false;
};

/** 人朝它再走一步则尝试推。可推箱子的最小配方 = Blocking + Pushable。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Pushable"))
class BOXPUSH_API UPushableLogic : public UInteractableLogicComp
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (ClampMin = "1", DisplayName = "一次推几格", InstanceOverride = "bAllowSteps"))
	int32 Steps = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowSteps = false;
};

/** 叠在 Pushable 上。有它就忽略 Steps，沿本次方向滑到停。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Slide"))
class BOXPUSH_API USlideLogic : public UInteractableLogicComp
{
	GENERATED_BODY()

public:
	/** true：前方另一只箱子则停。false：可从箱子格穿过，仍停在墙 / 非箱阻挡上。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "遇箱停下", InstanceOverride = "bAllowStopOnBox"))
	bool bStopOnBox = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowStopOnBox = false;
};

/** 叠在 Pushable 上。记下这一步开始时的格子，DelayMoves=0 则表现完立刻回。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Return"))
class BOXPUSH_API UReturnLogic : public UInteractableLogicComp
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (ClampMin = "0", DisplayName = "延迟几步", InstanceOverride = "bAllowDelayMoves"))
	int32 DelayMoves = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowDelayMoves = false;
};

/** 胜利点。上面叠了匹配 RequiredType 且带 Pushable 的实例，算占满。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Goal"))
class BOXPUSH_API UGoalLogic : public UInteractableLogicComp
{
	GENERATED_BODY()

public:
	UGoalLogic();
	virtual void PostLoad() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (Categories = "Type", DisplayName = "要叠上的类型", InstanceOverride = "bAllowRequiredType"))
	FGameplayTag RequiredType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowRequiredType = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "压上事件", InstanceOverride = "bAllowOccupiedEvent"))
	FName OccupiedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowOccupiedEvent = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "离开事件", InstanceOverride = "bAllowClearedEvent"))
	FName ClearedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowClearedEvent = false;

	virtual bool CountsTowardWin() const override { return true; }
	virtual bool IsWinSatisfied(const UBoxBoard& Board, const FBoxRuntimeInstance& Self) const override;
};

/** 被压下时发压上事件，离开时发离开事件。EventId 留给以后开门。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Pedal"))
class BOXPUSH_API UPedalLogic : public UInteractableLogicComp
{
	GENERATED_BODY()

public:
	UPedalLogic();
	virtual void PostLoad() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (Categories = "Type", DisplayName = "接受的类型", InstanceOverride = "bAllowAcceptType"))
	FGameplayTag AcceptType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowAcceptType = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "事件 Id", InstanceOverride = "bAllowEventId"))
	FName EventId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowEventId = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "压上事件", InstanceOverride = "bAllowOccupiedEvent"))
	FName OccupiedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowOccupiedEvent = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "离开事件", InstanceOverride = "bAllowClearedEvent"))
	FName ClearedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowClearedEvent = false;
};

/** 格子传感器。有它，Begin Overlap / End Overlap 才生效。不挡人、不挡推，也不算胜利。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Trigger"))
class BOXPUSH_API UTriggerLogic : public UInteractableLogicComp
{
	GENERATED_BODY()

public:
	/** 空：玩家和任意交互物都算。填了类型：只算 Type 匹配的实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (Categories = "Type", DisplayName = "接受的类型", InstanceOverride = "bAllowAcceptType"))
	FGameplayTag AcceptType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (DisplayName = "可实例重载"))
	bool bAllowAcceptType = false;
};
