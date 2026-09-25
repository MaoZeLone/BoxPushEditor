#include "Data/LevelData.h"

#include "Data/BoxAssetPaths.h"
#include "Data/BoxInstanceParams.h"
#include "Data/InteractableDef.h"
#include "Data/InteractableLogic.h"
#include "Match/BoxBoard.h"

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
	Width = 1;
	Height = 1;
	Cells.Init(ETerrainCell::Floor, 1);
	PlayerSpawn = FIntPoint(0, 0);
	Instances.Reset();
}

bool ULevelData::ExpandTo(FIntPoint& Cell)
{
	EnsureCellsSize();
	const int32 ShiftX = Cell.X < 0 ? -Cell.X : 0;
	const int32 ShiftY = Cell.Y < 0 ? -Cell.Y : 0;
	const int32 NeedW = FMath::Max(Width + ShiftX, Cell.X + ShiftX + 1);
	const int32 NeedH = FMath::Max(Height + ShiftY, Cell.Y + ShiftY + 1);
	if (NeedW > MaxSize || NeedH > MaxSize || NeedW < MinSize || NeedH < MinSize)
	{
		return false;
	}
	if (NeedW == Width && NeedH == Height && ShiftX == 0 && ShiftY == 0)
	{
		return true;
	}

	TArray<ETerrainCell> Next;
	Next.Init(ETerrainCell::Empty, NeedW * NeedH);
	if (Cells.Num() == Width * Height)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				Next[(Y + ShiftY) * NeedW + (X + ShiftX)] = Cells[CellIndex(X, Y)];
			}
		}
	}
	Width = NeedW;
	Height = NeedH;
	Cells = MoveTemp(Next);
	PlayerSpawn += FIntPoint(ShiftX, ShiftY);
	for (FBoxLevelInstance& Inst : Instances)
	{
		Inst.Cell += FIntPoint(ShiftX, ShiftY);
	}
	Cell += FIntPoint(ShiftX, ShiftY);
	return true;
}

void ULevelData::FitToContent()
{
	EnsureCellsSize();
	int32 MinX = Width;
	int32 MinY = Height;
	int32 MaxX = -1;
	int32 MaxY = -1;
	auto Mark = [&](int32 X, int32 Y)
	{
		MinX = FMath::Min(MinX, X);
		MinY = FMath::Min(MinY, Y);
		MaxX = FMath::Max(MaxX, X);
		MaxY = FMath::Max(MaxY, Y);
	};
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (GetCell(FIntPoint(X, Y)) != ETerrainCell::Empty)
			{
				Mark(X, Y);
			}
		}
	}
	if (IsInside(PlayerSpawn))
	{
		Mark(PlayerSpawn.X, PlayerSpawn.Y);
	}
	for (const FBoxLevelInstance& Inst : Instances)
	{
		if (IsInside(Inst.Cell))
		{
			Mark(Inst.Cell.X, Inst.Cell.Y);
		}
	}
	if (MaxX < 0)
	{
		Width = 1;
		Height = 1;
		Cells.Init(ETerrainCell::Floor, 1);
		PlayerSpawn = FIntPoint(0, 0);
		return;
	}

	const int32 NewW = MaxX - MinX + 1;
	const int32 NewH = MaxY - MinY + 1;
	if (NewW == Width && NewH == Height && MinX == 0 && MinY == 0)
	{
		return;
	}
	TArray<ETerrainCell> Next;
	Next.Reserve(NewW * NewH);
	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			Next.Add(GetCell(FIntPoint(X, Y)));
		}
	}
	Width = NewW;
	Height = NewH;
	Cells = MoveTemp(Next);
	PlayerSpawn -= FIntPoint(MinX, MinY);
	for (FBoxLevelInstance& Inst : Instances)
	{
		Inst.Cell -= FIntPoint(MinX, MinY);
	}
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
		AddError(TEXT("关卡资产名为空"));
	}
	if (Width < MinSize || Width > MaxSize || Height < MinSize || Height > MaxSize)
	{
		AddError(TEXT("宽或高越界（1–20）"));
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

	TSet<FName> FiredNames;
	TSet<FName> ListenNames;
	for (const FBoxLevelInstance& Inst : Instances)
	{
		const UInteractableDef* Def = Inst.LoadDefinition();
		if (!Def)
		{
			continue;
		}
		auto CollectActions = [&](const TArray<FInteractableStateActionDef>& Actions, FName StateId, EBoxStateActionPhase Phase)
		{
			for (int32 Index = 0; Index < Actions.Num(); ++Index)
			{
				const FInteractableStateActionDef& Action = Actions[Index];
				if (Action.Type != EBoxStateActionType::FireEvent || Action.EventId.IsNone())
				{
					continue;
				}
				FName EventId = Action.EventId;
				if (Action.bAllowOverride)
				{
					EventId = BoxInstanceParams::ResolveOverriddenName(
						Inst.ParamOverrides, StateId, BoxInstanceParams::FireEventOverrideKey(Phase, Index), Action.EventId);
				}
				if (!EventId.IsNone())
				{
					FiredNames.Add(EventId);
				}
			}
		};
		for (const FInteractableStateDef& State : Def->States)
		{
			CollectActions(State.OnEnter, State.StateId, EBoxStateActionPhase::Enter);
			CollectActions(State.OnStay, State.StateId, EBoxStateActionPhase::Stay);
			CollectActions(State.OnExit, State.StateId, EBoxStateActionPhase::Exit);
		}
		for (int32 Index = 0; Index < Def->Transitions.Num(); ++Index)
		{
			const FInteractableTransitionDef& Row = Def->Transitions[Index];
			if (Row.Condition != EBoxTransitionCondition::OnEvent || Row.EventId.IsNone())
			{
				continue;
			}
			FName EventId = Row.EventId;
			if (Row.bAllowOverride)
			{
				EventId = BoxInstanceParams::ResolveOverriddenName(
					Inst.ParamOverrides,
					BoxInstanceParams::TransitionOverrideComp(),
					BoxInstanceParams::TransitionOverrideKey(Index),
					Row.EventId);
			}
			if (!EventId.IsNone())
			{
				ListenNames.Add(EventId);
			}
		}
	}
	for (const FName& Name : FiredNames)
	{
		if (!ListenNames.Contains(Name))
		{
			AddWarn(*FString::Printf(TEXT("事件 %s 有人发，没有转移在听"), *Name.ToString()));
		}
	}

	if (GoalCount < 1 || PushableCount < GoalCount)
	{
		AddError(*FString::Printf(TEXT("箱子 %d 个，目标 %d 个，箱子不能比目标少"), PushableCount, GoalCount), PushableCells);
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
