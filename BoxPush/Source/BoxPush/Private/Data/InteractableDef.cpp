#include "Data/InteractableDef.h"
#include <initializer_list>
#include "BoxPushTags.h"
#include "Data/BoxInstanceParams.h"
#include "Data/BoxPushDefValidation.h"
#include "Data/BoxTypeDisplay.h"
namespace
{
	FInteractableStateDef MakeState(FName StateId, const TCHAR* Display, bool bDefault)
	{
		FInteractableStateDef State;
		State.StateId = StateId;
		State.DisplayName = FText::FromString(Display);
		State.bDefault = bDefault;
		return State;
	}

	void AddTransition(UInteractableDef* Def, FName FromState, FName EventId, FName ToState)
	{
		FInteractableTransitionDef Row;
		Row.FromState = FromState;
		Row.EventId = EventId;
		Row.ToState = ToState;
		Def->Transitions.Add(Row);
	}

	void AddOverlap(UInteractableDef* Def, FName FromState, EBoxTransitionCondition Condition, FName ToState)
	{
		FInteractableTransitionDef Row;
		Row.FromState = FromState;
		Row.Condition = Condition;
		Row.ToState = ToState;
		Def->Transitions.Add(Row);
	}

	void AddFireOnEnter(FInteractableStateDef& State, FName EventId)
	{
		FInteractableStateActionDef Action;
		Action.Type = EBoxStateActionType::FireEvent;
		Action.EventId = EventId;
		State.OnEnter.Add(Action);
	}

	void AddStateTasks(UInteractableDef* Def, FName StateId, std::initializer_list<FVisualTask> Tasks)
	{
		FStateVisual Row;
		Row.StateId = StateId;
		for (const FVisualTask& Task : Tasks)
		{
			Row.Tasks.Add(Task);
		}
		Def->StateVisuals.Add(Row);
	}

	template <typename T>
	T* AddLogic(UInteractableDef* Def, FName CompId)
	{
		T* Comp = NewObject<T>(Def, NAME_None, RF_Transactional);
		Comp->CompId = CompId;
		Def->LogicComps.Add(Comp);
		return Comp;
	}

	bool IsOfficialBoxId(FName DefinitionId)
	{
		return DefinitionId == TEXT("Box_Normal")
			|| DefinitionId == TEXT("Box_Slide")
			|| DefinitionId == TEXT("Box_Return");
	}

	bool IsOfficialInteractableId(FName DefinitionId)
	{
		return IsOfficialBoxId(DefinitionId)
			|| DefinitionId == TEXT("Target")
			|| DefinitionId == TEXT("Pedal");
	}

	void EnableOfficialOverrideFlags(UInteractableDef* Def)
	{
		for (UInteractableLogicComp* Comp : Def->LogicComps)
		{
			if (UPushableLogic* Pushable = Cast<UPushableLogic>(Comp))
			{
				Pushable->bAllowSteps = true;
			}
			else if (USlideLogic* Slide = Cast<USlideLogic>(Comp))
			{
				Slide->bAllowStopOnBox = true;
			}
			else if (UReturnLogic* Return = Cast<UReturnLogic>(Comp))
			{
				Return->bAllowDelayMoves = true;
			}
			else if (UGoalLogic* Goal = Cast<UGoalLogic>(Comp))
			{
				Goal->bAllowRequiredType = true;
			}
			else if (UPedalLogic* Pedal = Cast<UPedalLogic>(Comp))
			{
				Pedal->bAllowAcceptType = true;
				Pedal->bAllowEventId = true;
			}
		}
	}

}

FPrimaryAssetId UInteractableDef::GetPrimaryAssetId() const
{
	const FName Id = DefinitionId.IsNone() ? GetFName() : DefinitionId;
	return FPrimaryAssetId(TEXT("InteractableDef"), Id);
}

void UInteractableDef::ApplyOfficialDefaults(FName InDefinitionId)
{
	DefinitionId = InDefinitionId;
	SpriteComps.Reset();
	LogicComps.Reset();
	States.Reset();
	Transitions.Reset();
	StateVisuals.Reset();
	TransitionVisuals.Reset();

	if (InDefinitionId == TEXT("Box_Normal"))
	{
		DisplayName = FText::FromString(TEXT("普通箱子"));
		Type = TAG_Type_Interactable_Box;
		DesignerNote = TEXT("Blocking + Pushable. One cell per push. Idle only; push is not a state.");
		AddLogic<UBlockingLogic>(this, TEXT("Blocking"));
		AddLogic<UPushableLogic>(this, TEXT("Pushable"));
		States.Add(MakeState(TEXT("Idle"), TEXT("Idle"), true));
	}
	else if (InDefinitionId == TEXT("Box_Slide"))
	{
		DisplayName = FText::FromString(TEXT("滑动箱子"));
		Type = TAG_Type_Interactable_Box;
		DesignerNote = TEXT("Normal box plus Slide. Slides until blocked. Idle only; slide is the Slide component.");
		AddLogic<UBlockingLogic>(this, TEXT("Blocking"));
		AddLogic<UPushableLogic>(this, TEXT("Pushable"));
		AddLogic<USlideLogic>(this, TEXT("Slide"));
		States.Add(MakeState(TEXT("Idle"), TEXT("Idle"), true));
	}
	else if (InDefinitionId == TEXT("Box_Return"))
	{
		DisplayName = FText::FromString(TEXT("返回箱子"));
		Type = TAG_Type_Interactable_Box;
		DesignerNote = TEXT("Normal box plus Return. Returns to the cell before the push. Idle only; return is the Return component.");
		AddLogic<UBlockingLogic>(this, TEXT("Blocking"));
		AddLogic<UPushableLogic>(this, TEXT("Pushable"));
		AddLogic<UReturnLogic>(this, TEXT("Return"));
		States.Add(MakeState(TEXT("Idle"), TEXT("Idle"), true));
	}
	else if (InDefinitionId == TEXT("Target"))
	{
		DisplayName = FText::FromString(TEXT("目标"));
		Type = TAG_Type_Interactable_Target;
		DesignerNote = TEXT("Goal only. Does not block. Occupied when a matching box stacks on it.");
		AddLogic<UGoalLogic>(this, TEXT("Goal"));
		States.Add(MakeState(TEXT("Idle"), TEXT("Idle"), true));
		States.Add(MakeState(TEXT("Occupied"), TEXT("Occupied"), false));
		AddTransition(this, TEXT("Idle"), TEXT("Pressed"), TEXT("Occupied"));
		AddTransition(this, TEXT("Occupied"), TEXT("Released"), TEXT("Idle"));
	}
	else if (InDefinitionId == TEXT("Pedal"))
	{
		DisplayName = FText::FromString(TEXT("踏板"));
		Type = TAG_Type_Interactable_Pedal;
		DesignerNote = TEXT("Pedal only. Pressed when a box is on it. Put door events on Pressed OnEnter.");
		AddLogic<UPedalLogic>(this, TEXT("Pedal"));
		States.Add(MakeState(TEXT("Idle"), TEXT("Idle"), true));
		States.Add(MakeState(TEXT("Pressed"), TEXT("Pressed"), false));
		AddTransition(this, TEXT("Idle"), TEXT("Pressed"), TEXT("Pressed"));
		AddTransition(this, TEXT("Pressed"), TEXT("Released"), TEXT("Idle"));
	}
	else if (InDefinitionId == TEXT("Trigger"))
	{
		DisplayName = FText::FromString(TEXT("触发器"));
		Type = TAG_Type_Interactable;
		DesignerNote = TEXT("Box on it opens the gate. Box off it closes the gate.");
		UTriggerLogic* Trigger = AddLogic<UTriggerLogic>(this, TEXT("Trigger"));
		Trigger->AcceptType = TAG_Type_Interactable_Box;
		Trigger->bAllowAcceptType = true;
		FInteractableStateDef Empty = MakeState(TEXT("Empty"), TEXT("空"), true);
		AddFireOnEnter(Empty, TEXT("WallClose"));
		FInteractableStateDef Held = MakeState(TEXT("Held"), TEXT("压住"), false);
		AddFireOnEnter(Held, TEXT("WallOpen"));
		States.Add(Empty);
		States.Add(Held);
		AddOverlap(this, TEXT("Empty"), EBoxTransitionCondition::BeginOverlap, TEXT("Held"));
		AddOverlap(this, TEXT("Held"), EBoxTransitionCondition::EndOverlap, TEXT("Empty"));
		bOfficialOverridesSeeded = true;
	}
	else if (InDefinitionId == TEXT("Gate"))
	{
		DisplayName = FText::FromString(TEXT("可消失的墙"));
		Type = TAG_Type_Interactable;
		DesignerNote = TEXT("Blocks until WallOpen. WallClose brings it back.");
		AddLogic<UBlockingLogic>(this, TEXT("Blocking"));
		States.Add(MakeState(TEXT("Closed"), TEXT("挡住"), true));
		States.Add(MakeState(TEXT("Open"), TEXT("消失"), false));
		AddTransition(this, TEXT("Closed"), TEXT("WallOpen"), TEXT("Open"));
		AddTransition(this, TEXT("Open"), TEXT("WallClose"), TEXT("Closed"));
		FVisualTask Hide;
		Hide.Type = EVisualTaskType::SetVisible;
		Hide.CompId = TEXT("Sprite");
		Hide.bVisible = false;
		FVisualTask Unblock;
		Unblock.Type = EVisualTaskType::SetBlocking;
		Unblock.bBlocksPlayer = false;
		Unblock.bBlocksPush = false;
		AddStateTasks(this, TEXT("Open"), { Hide, Unblock });
	}
	else
	{
		DisplayName = FText::FromName(InDefinitionId);
		Type = TAG_Type_Interactable_Box;
		DesignerNote = TEXT("Unknown official Id.");
	}

	if (IsOfficialInteractableId(InDefinitionId))
	{
		EnableOfficialOverrideFlags(this);
		bOfficialOverridesSeeded = true;
	}
}

bool UInteractableDef::SeedOfficialOverrideFlags()
{
	if (bOfficialOverridesSeeded || !IsOfficialInteractableId(DefinitionId))
	{
		return false;
	}
	EnableOfficialOverrideFlags(this);
	bOfficialOverridesSeeded = true;
	return true;
}

TArray<FString> UInteractableDef::GetVisualStateOptions() const
{
	TArray<FString> Options;
	Options.Add(FString());
	for (const FInteractableStateDef& State : States)
	{
		if (!State.StateId.IsNone())
		{
			Options.Add(State.StateId.ToString());
		}
	}
	return Options;
}

TArray<FString> UInteractableDef::GetVisualCompOptions() const
{
	TArray<FString> Options;
	for (const TObjectPtr<UVisualSpriteComp>& Comp : SpriteComps)
	{
		if (Comp && !Comp->CompId.IsNone())
		{
			Options.Add(Comp->CompId.ToString());
		}
	}
	return Options;
}

FName UInteractableDef::GetDefaultState() const
{
	for (const FInteractableStateDef& State : States)
	{
		if (State.bDefault)
		{
			return State.StateId;
		}
	}
	return NAME_None;
}

void UInteractableDef::PostLoad()
{
	Super::PostLoad();
	if (!Type.IsValid())
	{
		Type = UBoxTypeDisplayLibrary::InferInteractableType(DefinitionId);
	}
	if (Transitions.Num() > 0)
	{
		return;
	}
	for (FInteractableStateDef& State : States)
	{
		for (const FName EventId : State.EnterEvents)
		{
			if (EventId.IsNone())
			{
				continue;
			}
			FInteractableTransitionDef Row;
			Row.EventId = EventId;
			Row.ToState = State.StateId;
			Transitions.Add(Row);
		}
		State.EnterEvents.Reset();
	}
}

FName UInteractableDef::ResolveTransition(FName FromState, FName EventId, const TArray<FBoxInstanceOverride>* Overrides) const
{
	if (EventId.IsNone())
	{
		return NAME_None;
	}

	for (int32 Index = 0; Index < Transitions.Num(); ++Index)
	{
		const FInteractableTransitionDef& Row = Transitions[Index];
		if (Row.Condition != EBoxTransitionCondition::OnEvent)
		{
			continue;
		}
		FName ListenId = Row.EventId;
		if (Row.bAllowOverride && Overrides)
		{
			ListenId = BoxInstanceParams::ResolveOverriddenName(
				*Overrides,
				BoxInstanceParams::TransitionOverrideComp(),
				BoxInstanceParams::TransitionOverrideKey(Index),
				Row.EventId);
		}
		if (ListenId != EventId)
		{
			continue;
		}
		if (!Row.FromState.IsNone() && Row.FromState != FromState)
		{
			continue;
		}
		if (!Row.ToState.IsNone())
		{
			return Row.ToState;
		}
	}

	for (const FInteractableStateDef& State : States)
	{
		if (State.EnterEvents.Contains(EventId))
		{
			return State.StateId;
		}
	}
	return NAME_None;
}

FName UInteractableDef::ResolveCondition(FName FromState, EBoxTransitionCondition Condition) const
{
	if (Condition == EBoxTransitionCondition::OnEvent)
	{
		return NAME_None;
	}

	for (const FInteractableTransitionDef& Row : Transitions)
	{
		if (Row.Condition != Condition)
		{
			continue;
		}
		if (!Row.FromState.IsNone() && Row.FromState != FromState)
		{
			continue;
		}
		if (!Row.ToState.IsNone())
		{
			return Row.ToState;
		}
	}
	return NAME_None;
}

const FInteractableStateDef* UInteractableDef::FindState(FName StateId) const
{
	if (StateId.IsNone())
	{
		return nullptr;
	}
	for (const FInteractableStateDef& State : States)
	{
		if (State.StateId == StateId)
		{
			return &State;
		}
	}
	return nullptr;
}

void UInteractableDef::Validate(TArray<FLevelValidationIssue>& OutIssues) const
{
	using namespace BoxPushDefValidation;

	if (DefinitionId.IsNone())
	{
		AddIssue(OutIssues, true, TEXT("DefinitionId 为空"));
	}
	else
	{
		const FString Expected = FString::Printf(TEXT("DA_%s"), *DefinitionId.ToString());
		if (GetName() != Expected)
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("资产名 %s 应等于 %s"), *GetName(), *Expected));
		}
	}

	ValidateStates(States, OutIssues);
	ValidateTransitions(States, Transitions, OutIssues);

	TMap<UClass*, int32> LogicCounts;
	TSet<FName> LogicIds;
	for (int32 Index = 0; Index < LogicComps.Num(); ++Index)
	{
		const UInteractableLogicComp* Comp = LogicComps[Index];
		if (!Comp)
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("LogicComps[%d] 为空"), Index));
			continue;
		}
		if (Comp->CompId.IsNone())
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("LogicComps[%d] 缺少 CompId"), Index));
		}
		else if (LogicIds.Contains(Comp->CompId))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("Logic CompId 重复：%s"), *Comp->CompId.ToString()));
		}
		LogicIds.Add(Comp->CompId);
		LogicCounts.FindOrAdd(Comp->GetClass())++;
	}
	for (const TPair<UClass*, int32>& Pair : LogicCounts)
	{
		if (Pair.Value > 1)
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("逻辑组件 %s 出现了 %d 次，同类最多一条"), *Pair.Key->GetName(), Pair.Value));
		}
	}
	if ((FindLogic<USlideLogic>() || FindLogic<UReturnLogic>()) && !FindLogic<UPushableLogic>())
	{
		AddIssue(OutIssues, true, TEXT("Slide / Return 必须同时有 Pushable"));
	}
	if (!FindLogic<UTriggerLogic>())
	{
		for (int32 Index = 0; Index < Transitions.Num(); ++Index)
		{
			const EBoxTransitionCondition Condition = Transitions[Index].Condition;
			if (Condition == EBoxTransitionCondition::BeginOverlap || Condition == EBoxTransitionCondition::EndOverlap)
			{
				AddIssue(OutIssues, true, FString::Printf(
					TEXT("Transition[%d] 的 Begin Overlap / End Overlap 需要 Trigger 逻辑组件"), Index));
			}
		}
	}
	if (!Type.IsValid())
	{
		AddIssue(OutIssues, true, TEXT("未配置类型 Tag"));
	}

	TSet<FName> SpriteIds;
	for (const TObjectPtr<UVisualSpriteComp>& Comp : SpriteComps)
	{
		if (Comp && !Comp->CompId.IsNone())
		{
			SpriteIds.Add(Comp->CompId);
		}
	}
	TSet<FName> KnownStates;
	for (const FInteractableStateDef& State : States)
	{
		if (!State.StateId.IsNone())
		{
			KnownStates.Add(State.StateId);
		}
	}
	auto CheckTasks = [&](const TArray<FVisualTask>& Tasks, const TCHAR* Slot, int32 Row)
	{
		for (int32 Index = 0; Index < Tasks.Num(); ++Index)
		{
			const FVisualTask& Task = Tasks[Index];
			if (Task.Type == EVisualTaskType::SetBlocking)
			{
				continue;
			}
			if (Task.CompId.IsNone() || !SpriteIds.Contains(Task.CompId))
			{
				AddIssue(OutIssues, true, FString::Printf(
					TEXT("%s[%d] 任务[%d] 的 CompId 不在表现里"), Slot, Row, Index));
			}
		}
	};
	TSet<FName> SeenStateVisuals;
	for (int32 Index = 0; Index < StateVisuals.Num(); ++Index)
	{
		const FStateVisual& Row = StateVisuals[Index];
		if (Row.StateId.IsNone() || !KnownStates.Contains(Row.StateId))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("状态表现[%d] 的状态不存在"), Index));
		}
		else if (SeenStateVisuals.Contains(Row.StateId))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("状态表现[%d] 和前面的状态重复"), Index));
		}
		SeenStateVisuals.Add(Row.StateId);
		CheckTasks(Row.Tasks, TEXT("状态表现"), Index);
	}
	TSet<FString> SeenTransitionVisuals;
	for (int32 Index = 0; Index < TransitionVisuals.Num(); ++Index)
	{
		const FTransitionVisual& Row = TransitionVisuals[Index];
		if (!Row.FromState.IsNone() && !KnownStates.Contains(Row.FromState))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("转移表现[%d] 的 FromState 不存在"), Index));
		}
		if (!Row.ToState.IsNone() && !KnownStates.Contains(Row.ToState))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("转移表现[%d] 的 ToState 不存在"), Index));
		}
		if (Row.Condition == EBoxTransitionCondition::OnEvent && Row.EventId.IsNone())
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("转移表现[%d] 缺少 EventId"), Index));
		}
		const FString Key = FString::Printf(
			TEXT("%s|%d|%s|%s"),
			*Row.FromState.ToString(),
			static_cast<int32>(Row.Condition),
			Row.Condition == EBoxTransitionCondition::OnEvent ? *Row.EventId.ToString() : TEXT(""),
			*Row.ToState.ToString());
		if (SeenTransitionVisuals.Contains(Key))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("转移表现[%d] 和前面的转移重复"), Index));
		}
		SeenTransitionVisuals.Add(Key);
		CheckTasks(Row.Tasks, TEXT("转移表现"), Index);
	}
}
