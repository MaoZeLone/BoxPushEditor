#include "Match/BoxBoard.h"

#include "Data/BoxInstanceParams.h"
#include "Data/InteractableDef.h"
#include "Data/InteractableLogic.h"
#include "Data/LevelData.h"

DEFINE_LOG_CATEGORY_STATIC(LogBoxBoard, Log, All);

bool UBoxBoard::InitFromLevel(const ULevelData* InLevel)
{
	SourceLevel = InLevel;
	Instances.Reset();
	PlayerCell = FIntPoint::ZeroValue;
	Width = 0;
	Height = 0;
	Cells.Reset();

	if (!InLevel)
	{
		UE_LOG(LogBoxBoard, Error, TEXT("InitFromLevel: Level 为空"));
		return false;
	}

	Width = InLevel->Width;
	Height = InLevel->Height;
	Cells = InLevel->Cells;
	if (Cells.Num() != Width * Height)
	{
		UE_LOG(LogBoxBoard, Error, TEXT("InitFromLevel: Cells 尺寸不对"));
		return false;
	}

	PlayerCell = InLevel->PlayerSpawn;
	for (const FBoxLevelInstance& Source : InLevel->Instances)
	{
		const UInteractableDef* Def = Source.LoadDefinition();
		if (!Def)
		{
			UE_LOG(LogBoxBoard, Warning, TEXT("找不到交互物定义 %s"), *Source.GetResolvedDefinitionId().ToString());
			continue;
		}

		FBoxRuntimeInstance Inst;
		Inst.InstanceId = Source.InstanceId;
		Inst.Def = Def;
		Inst.Cell = Source.Cell;
		Inst.YawSteps = Source.YawSteps;
		Inst.Overrides = Source.ParamOverrides;
		Inst.CurrentState = Def->GetDefaultState();
		Inst.ReturnHome = Source.Cell;
		Inst.MovesUntilReturn = -1;
		Instances.Add(Inst);
	}

	TMap<FIntPoint, int32> BlockingAt;
	for (const FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.Def && Inst.Def->FindLogic<UBlockingLogic>())
		{
			BlockingAt.FindOrAdd(Inst.Cell)++;
		}
	}
	for (const TPair<FIntPoint, int32>& Pair : BlockingAt)
	{
		if (Pair.Value > 1)
		{
			UE_LOG(LogBoxBoard, Warning, TEXT("格子 (%d,%d) 叠了 %d 个阻挡实例"), Pair.Key.X, Pair.Key.Y, Pair.Value);
		}
	}

	return true;
}

ETerrainCell UBoxBoard::GetTerrain(FIntPoint Cell) const
{
	if (!IsInside(Cell) || Cells.Num() != Width * Height)
	{
		return ETerrainCell::Empty;
	}
	return Cells[Cell.Y * Width + Cell.X];
}

FName UBoxBoard::GetLevelId() const
{
	return SourceLevel ? SourceLevel->LevelId : NAME_None;
}

void UBoxBoard::GetInstancesAt(FIntPoint Cell, TArray<const FBoxRuntimeInstance*>& Out) const
{
	Out.Reset();
	for (const FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.Cell == Cell)
		{
			Out.Add(&Inst);
		}
	}
}

FBoxRuntimeInstance* UBoxBoard::FindInstance(FName InstanceId)
{
	for (FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.InstanceId == InstanceId)
		{
			return &Inst;
		}
	}
	return nullptr;
}

const FBoxRuntimeInstance* UBoxBoard::FindInstance(FName InstanceId) const
{
	for (const FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.InstanceId == InstanceId)
		{
			return &Inst;
		}
	}
	return nullptr;
}

bool UBoxBoard::IsInside(FIntPoint Cell) const
{
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Width && Cell.Y < Height;
}

bool UBoxBoard::IsStandable(FIntPoint Cell) const
{
	return GetTerrain(Cell) == ETerrainCell::Floor;
}

bool UBoxBoard::BlocksPlayer(FIntPoint Cell, FName IgnoreId) const
{
	for (const FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.Cell != Cell || Inst.InstanceId == IgnoreId || !Inst.Def)
		{
			continue;
		}
		if (BoxInstanceParams::BlocksPlayer(Inst))
		{
			return true;
		}
	}
	return false;
}

bool UBoxBoard::BlocksPush(FIntPoint Cell, FName IgnoreId) const
{
	for (const FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.Cell != Cell || Inst.InstanceId == IgnoreId || !Inst.Def)
		{
			continue;
		}
		if (BoxInstanceParams::BlocksPush(Inst))
		{
			return true;
		}
	}
	return false;
}

bool UBoxBoard::HasPushableAt(FIntPoint Cell, FName IgnoreId) const
{
	for (const FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.Cell != Cell || Inst.InstanceId == IgnoreId || !Inst.Def)
		{
			continue;
		}
		if (Inst.Def->FindLogic<UPushableLogic>())
		{
			return true;
		}
	}
	return false;
}

FBoxRuntimeInstance* UBoxBoard::FindPushableBlockingPlayer(FIntPoint Cell)
{
	for (FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.Cell != Cell || !Inst.Def)
		{
			continue;
		}
		if (!BoxInstanceParams::BlocksPlayer(Inst))
		{
			continue;
		}
		if (Inst.Def->FindLogic<UPushableLogic>())
		{
			return &Inst;
		}
	}
	return nullptr;
}
