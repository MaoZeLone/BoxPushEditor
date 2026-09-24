#include "Data/LevelData.h"

#include "Data/BoxAssetPaths.h"
#include "Data/BoxInstanceParams.h"
#include "Data/InteractableDef.h"
#include "Data/InteractableLogic.h"

FPrimaryAssetId ULevelData::GetPrimaryAssetId() const
{
	const FName Id = LevelId.IsNone() ? GetFName() : LevelId;
	return FPrimaryAssetId(TEXT("LevelData"), Id);
}

bool ULevelData::IsInside(FIntPoint Cell) const
{
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Width && Cell.Y < Height;
}

ETerrainCell ULevelData::GetCell(FIntPoint Coord) const
{
	if (!IsInside(Coord) || Cells.Num() != Width * Height)
	{
		return ETerrainCell::Empty;
	}
	return Cells[CellIndex(Coord.X, Coord.Y)];
}

void ULevelData::SetCell(FIntPoint Coord, ETerrainCell Value)
{
	EnsureCellsSize();
	if (IsInside(Coord))
	{
		Cells[CellIndex(Coord.X, Coord.Y)] = Value;
	}
}

void ULevelData::EnsureCellsSize()
{
	Width = FMath::Clamp(Width, MinSize, MaxSize);
	Height = FMath::Clamp(Height, MinSize, MaxSize);
	const int32 Expected = Width * Height;
	if (Cells.Num() == Expected)
	{
		return;
	}
	if (Cells.Num() == 0)
	{
		Cells.Init(ETerrainCell::Floor, Expected);
	}
}

void ULevelData::ApplyNewLevelDefaults(FName NewLevelId)
{
	LevelId = NewLevelId;
	DisplayName = FText::FromString(TEXT("未命名关卡"));
	DesignerNote.Reset();
	Width = DefaultSize;
	Height = DefaultSize;
	Cells.Init(ETerrainCell::Floor, Width * Height);
	PlayerSpawn = FIntPoint(2, 3);
	Instances.Reset();
}

void ULevelData::ResizeGrid(int32 NewWidth, int32 NewHeight)
{
	NewWidth = FMath::Clamp(NewWidth, MinSize, MaxSize);
	NewHeight = FMath::Clamp(NewHeight, MinSize, MaxSize);
	TArray<ETerrainCell> Next;
	Next.Init(ETerrainCell::Floor, NewWidth * NewHeight);
	for (int32 Y = 0; Y < NewHeight; ++Y)
	{
		for (int32 X = 0; X < NewWidth; ++X)
		{
			if (IsInside(FIntPoint(X, Y)) && Cells.Num() == Width * Height)
			{
				Next[Y * NewWidth + X] = Cells[CellIndex(X, Y)];
			}
		}
	}
	Width = NewWidth;
	Height = NewHeight;
	Cells = MoveTemp(Next);
	if (!IsInside(PlayerSpawn))
	{
		PlayerSpawn = FIntPoint(1, 1);
	}
	Instances.RemoveAll([this](const FBoxLevelInstance& Inst) { return !IsInside(Inst.Cell); });
}

bool ULevelData::ResizeWouldCrop(int32 NewWidth, int32 NewHeight) const
{
	if (!IsInside(PlayerSpawn) || PlayerSpawn.X >= NewWidth || PlayerSpawn.Y >= NewHeight)
	{
		return true;
	}
	for (const FBoxLevelInstance& Inst : Instances)
	{
		if (Inst.Cell.X >= NewWidth || Inst.Cell.Y >= NewHeight)
		{
			return true;
		}
	}
	return false;
}

void ULevelData::Validate(TArray<FLevelValidationIssue>& OutIssues) const
{
	auto AddError = [&OutIssues](const TCHAR* Text, const TArray<FIntPoint>& IssueCells = {})
	{
		FLevelValidationIssue Issue;
		Issue.bError = true;
		Issue.Message = FText::FromString(Text);
		Issue.Cells = IssueCells;
		OutIssues.Add(Issue);
	};
	auto AddWarn = [&OutIssues](const TCHAR* Text)
	{
		FLevelValidationIssue Issue;
		Issue.bError = false;
		Issue.Message = FText::FromString(Text);
		OutIssues.Add(Issue);
	};

	if (LevelId.IsNone())
	{
		AddError(TEXT("LevelId 为空"));
	}
	if (Width < MinSize || Width > MaxSize || Height < MinSize || Height > MaxSize)
	{
		AddError(TEXT("宽或高越界（5–20）"));
	}
	if (Cells.Num() != Width * Height)
	{
		AddError(TEXT("Cells 长度必须等于 Width * Height"));
	}
	if (!IsInside(PlayerSpawn))
	{
		AddError(TEXT("玩家出生点在地图外"), { PlayerSpawn });
	}
	else if (GetCell(PlayerSpawn) != ETerrainCell::Floor)
	{
		AddError(TEXT("玩家出生点必须在地板上"), { PlayerSpawn });
	}

	TSet<FName> SeenIds;
	TMap<FIntPoint, TArray<FName>> BlockingAt;
	int32 PushableCount = 0;
	int32 GoalCount = 0;
	TArray<FIntPoint> PushableCells;
	for (const FBoxLevelInstance& Inst : Instances)
	{
		if (Inst.InstanceId.IsNone())
		{
			AddError(TEXT("实例缺少 InstanceId"), { Inst.Cell });
		}
		else if (SeenIds.Contains(Inst.InstanceId))
		{
			AddError(*FString::Printf(TEXT("InstanceId 重复：%s"), *Inst.InstanceId.ToString()), { Inst.Cell });
		}
		SeenIds.Add(Inst.InstanceId);

		if (Inst.Definition.IsNull() && Inst.DefinitionId.IsNone())
		{
			AddError(TEXT("实例缺少 Definition"), { Inst.Cell });
		}
		if (!IsInside(Inst.Cell))
		{
			AddError(*FString::Printf(TEXT("实例 %s 在地图外"), *Inst.InstanceId.ToString()), { Inst.Cell });
		}
		else if (!IsStandable(GetCell(Inst.Cell)))
		{
			AddError(TEXT("玩家或箱子落在空洞 / 墙上"), { Inst.Cell });
		}

		const UInteractableDef* Def = Inst.LoadDefinition();
		BoxInstanceParams::AppendIssues(Inst, OutIssues);
		if (!Def)
		{
			AddError(*FString::Printf(TEXT("实例 %s 的定义无法加载"), *Inst.InstanceId.ToString()), { Inst.Cell });
			continue;
		}
		if (Def->SpriteComps.Num() == 0)
		{
			AddWarn(*FString::Printf(TEXT("实例 %s 的定义没有表现组件"), *Inst.InstanceId.ToString()));
		}
		if (Def->FindLogic<UPushableLogic>())
		{
			++PushableCount;
			PushableCells.Add(Inst.Cell);
		}
		if (Def->FindLogic<UGoalLogic>())
		{
			++GoalCount;
		}
		if (Def->FindLogic<UBlockingLogic>())
		{
			BlockingAt.FindOrAdd(Inst.Cell).Add(Inst.InstanceId);
		}
	}
	if (PushableCount != GoalCount || PushableCount < 1)
	{
		AddError(*FString::Printf(TEXT("Pushable %d 个，Goal %d 个，胜利条件无法达成"), PushableCount, GoalCount), PushableCells);
	}
	for (const TPair<FIntPoint, TArray<FName>>& Pair : BlockingAt)
	{
		if (Pair.Value.Num() > 1)
		{
			AddError(*FString::Printf(TEXT("格子 (%d,%d) 叠了 %d 个阻挡实例"), Pair.Key.X, Pair.Key.Y, Pair.Value.Num()), { Pair.Key });
		}
	}

	if (DesignerNote.IsEmpty())
	{
		AddWarn(TEXT("策划备注为空，这一关的教学意图没写"));
	}

	bool bOpenRim = false;
	if (Cells.Num() == Width * Height)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (GetCell(FIntPoint(X, 0)) != ETerrainCell::Wall || GetCell(FIntPoint(X, Height - 1)) != ETerrainCell::Wall)
			{
				bOpenRim = true;
				break;
			}
		}
		for (int32 Y = 0; Y < Height && !bOpenRim; ++Y)
		{
			if (GetCell(FIntPoint(0, Y)) != ETerrainCell::Wall || GetCell(FIntPoint(Width - 1, Y)) != ETerrainCell::Wall)
			{
				bOpenRim = true;
			}
		}
	}
	if (bOpenRim)
	{
		AddWarn(TEXT("最外圈没有封闭的墙，角色可能走出去"));
	}
}

void ULevelData::PostLoad()
{
	Super::PostLoad();
	EnsureCellsSize();
	for (FBoxLevelInstance& Inst : Instances)
	{
		Inst.BackfillDefinitionIfNeeded();
	}
}

#if WITH_EDITOR
void ULevelData::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
	CachedWidth = Width;
	CachedHeight = Height;
}

void ULevelData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName Prop = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	if (Prop == GET_MEMBER_NAME_CHECKED(ULevelData, Width) || Prop == GET_MEMBER_NAME_CHECKED(ULevelData, Height))
	{
		const int32 NewWidth = Width;
		const int32 NewHeight = Height;
		Width = CachedWidth;
		Height = CachedHeight;
		ResizeGrid(NewWidth, NewHeight);
	}
}
#endif
