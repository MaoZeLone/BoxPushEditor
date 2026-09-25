#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/BoxPushLevelTypes.h"
#include "Data/VisualComps.h"
#include "BoxBoard.generated.h"

class UInteractableDef;
class ULevelData;

USTRUCT()
struct BOXPUSH_API FBoxRuntimeInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FName InstanceId;

	UPROPERTY()
	TObjectPtr<const UInteractableDef> Def = nullptr;

	UPROPERTY()
	FIntPoint Cell = FIntPoint::ZeroValue;

	/** 与 FBoxLevelInstance::YawSteps 相同：0 上、1 右、2 下、3 左。 */
	UPROPERTY()
	int32 YawSteps = 0;

	UPROPERTY()
	TArray<FBoxInstanceOverride> Overrides;

	UPROPERTY()
	FName CurrentState;

	UPROPERTY()
	FIntPoint ReturnHome = FIntPoint::ZeroValue;

	/** -1 = 没有回程；0 = 这一步表现完立刻回；>0 = 再过几步玩家操作后回 */
	UPROPERTY()
	int32 MovesUntilReturn = -1;

	/** 上一拍这个 Trigger 上是否有匹配对象。快照带着它，撤回不补发重叠。 */
	UPROPERTY()
	bool bTriggerOccupied = false;
};

USTRUCT()
struct BOXPUSH_API FBoxInstanceDelta
{
	GENERATED_BODY()

	UPROPERTY()
	FName InstanceId;

	UPROPERTY()
	FIntPoint From = FIntPoint::ZeroValue;

	UPROPERTY()
	FIntPoint To = FIntPoint::ZeroValue;

	UPROPERTY()
	FName NewState;
};

USTRUCT()
struct BOXPUSH_API FBoxStepResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bApplied = false;

	UPROPERTY()
	bool bPushed = false;

	UPROPERTY()
	bool bWon = false;

	UPROPERTY()
	FIntPoint PlayerFrom = FIntPoint::ZeroValue;

	UPROPERTY()
	FIntPoint PlayerTo = FIntPoint::ZeroValue;

	UPROPERTY()
	TArray<FBoxInstanceDelta> Moves;

	UPROPERTY()
	TArray<FName> FiredEvents;

	UPROPERTY()
	TArray<FVisualTransitionCue> VisualTransitions;
};

/** 地图管理器：关卡格子、占位、邻格查询。运行时格子状态活在这里。不负责走/推规则。 */
UCLASS()
class BOXPUSH_API UBoxBoard : public UObject
{
	GENERATED_BODY()

public:
	bool InitFromLevel(const ULevelData* InLevel);

	FName GetLevelId() const;
	FIntPoint GetPlayerCell() const { return PlayerCell; }
	void SetPlayerCell(FIntPoint Cell) { PlayerCell = Cell; }
	int32 GetPlayerYawSteps() const { return PlayerYawSteps; }
	int32 GetWidth() const { return Width; }
	int32 GetHeight() const { return Height; }
	const ULevelData* GetSourceLevel() const { return SourceLevel; }
	const TArray<FBoxRuntimeInstance>& GetInstances() const { return Instances; }
	TArray<FBoxRuntimeInstance>& EditInstances() { return Instances; }

	ETerrainCell GetTerrain(FIntPoint Cell) const;
	FIntPoint GetNeighbor(FIntPoint Cell, FIntPoint Dir) const { return Cell + Dir; }
	bool IsInside(FIntPoint Cell) const;
	bool IsStandable(FIntPoint Cell) const;
	bool BlocksPlayer(FIntPoint Cell, FName IgnoreId = NAME_None) const;
	bool BlocksPush(FIntPoint Cell, FName IgnoreId = NAME_None) const;
	bool HasPushableAt(FIntPoint Cell, FName IgnoreId = NAME_None) const;
	void GetInstancesAt(FIntPoint Cell, TArray<const FBoxRuntimeInstance*>& Out) const;

	FBoxRuntimeInstance* FindInstance(FName InstanceId);
	const FBoxRuntimeInstance* FindInstance(FName InstanceId) const;
	FBoxRuntimeInstance* FindPushableBlockingPlayer(FIntPoint Cell);

private:
	UPROPERTY()
	TObjectPtr<const ULevelData> SourceLevel;

	UPROPERTY()
	TArray<ETerrainCell> Cells;

	UPROPERTY()
	TArray<FBoxRuntimeInstance> Instances;

	FIntPoint PlayerCell = FIntPoint::ZeroValue;
	int32 PlayerYawSteps = 0;
	int32 Width = 0;
	int32 Height = 0;
};
