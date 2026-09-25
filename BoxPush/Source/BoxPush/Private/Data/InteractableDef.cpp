#include "Data/InteractableDef.h"
#include "Data/BoxInstanceParams.h"
#include "Data/BoxPushDefValidation.h"
#include "Data/BoxTypeDisplay.h"

FPrimaryAssetId UInteractableDef::GetPrimaryAssetId() const
{
	const FName Id = DefinitionId.IsNone() ? GetFName() : DefinitionId;
	return FPrimaryAssetId(TEXT("InteractableDef"), Id);
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
	for (const FStateVisual& Row : StateVisuals)
	{
		if (Row.StateId.IsNone() || Row.Tasks.Num() == 0)
		{
			continue;
		}
		for (FInteractableStateDef& State : States)
		{
			if (State.StateId == Row.StateId)
			{
				if (State.Tasks.Num() == 0)
				{
					State.Tasks = Row.Tasks;
				}
				break;
			}
		}
	}
	StateVisuals.Reset();
	if (!Type.IsValid())
	{
		Type = UBoxTypeDisplayLibrary::InferInteractableType(DefinitionId);
	}

	auto SameRow = [](const FInteractableTransitionDef& A, const FInteractableTransitionDef& B)
	{
		return A.FromState == B.FromState
			&& A.Condition == B.Condition
			&& A.EventId == B.EventId
			&& A.ToState == B.ToState;
	};
	auto AddTransition = [&](FInteractableTransitionDef Row)
	{
		for (FInteractableTransitionDef& Existing : Transitions)
		{
			if (SameRow(Existing, Row))
			{
				if (Existing.Tasks.Num() == 0 && Row.Tasks.Num() > 0)
				{
					Existing.Tasks = Row.Tasks;
				}
				return;
			}
		}
		Transitions.Add(MoveTemp(Row));
	};
	for (FInteractableStateDef& State : States)
	{
		for (FInteractableTransitionDef& Row : State.Transitions)
		{
			if (Row.FromState.IsNone())
			{
				Row.FromState = State.StateId;
			}
			AddTransition(Row);
		}
		State.Transitions.Reset();
		for (const FName EventId : State.EnterEvents)
		{
			if (EventId.IsNone())
			{
				continue;
			}
			FInteractableTransitionDef Row;
			Row.EventId = EventId;
			Row.ToState = State.StateId;
			AddTransition(Row);
		}
		State.EnterEvents.Reset();
	}

	auto VisualMatches = [](const FInteractableTransitionDef& Row, const FTransitionVisual& Visual)
	{
		if (!Visual.FromState.IsNone() && Visual.FromState != Row.FromState)
		{
			return false;
		}
		if (!Row.FromState.IsNone() && !Visual.FromState.IsNone() && Row.FromState != Visual.FromState)
		{
			return false;
		}
		if (Row.Condition != Visual.Condition)
		{
			return false;
		}
		if (!Visual.ToState.IsNone() && Row.ToState != Visual.ToState)
		{
			return false;
		}
		if (Visual.Condition == EBoxTransitionCondition::OnEvent && Row.EventId != Visual.EventId)
		{
			return false;
		}
		return true;
	};
	for (const FTransitionVisual& Visual : TransitionVisuals)
	{
		if (Visual.Tasks.Num() == 0)
		{
			continue;
		}
		bool bFound = false;
		for (FInteractableTransitionDef& Row : Transitions)
		{
			if (!VisualMatches(Row, Visual))
			{
				continue;
			}
			if (Row.Tasks.Num() == 0)
			{
				Row.Tasks = Visual.Tasks;
			}
			bFound = true;
			break;
		}
		if (bFound)
		{
			continue;
		}
		FInteractableTransitionDef Row;
		Row.FromState = Visual.FromState;
		Row.Condition = Visual.Condition;
		Row.EventId = Visual.EventId;
		Row.ToState = Visual.ToState;
		Row.Tasks = Visual.Tasks;
		Transitions.Add(Row);
	}
	TransitionVisuals.Reset();
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
					TEXT("转移[%d] 的 Begin Overlap / End Overlap 需要 Trigger 逻辑组件"), Index));
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
	for (int32 Index = 0; Index < States.Num(); ++Index)
	{
		CheckTasks(States[Index].Tasks, TEXT("状态"), Index);
	}
	for (int32 Index = 0; Index < Transitions.Num(); ++Index)
	{
		CheckTasks(Transitions[Index].Tasks, TEXT("转移"), Index);
	}
}
