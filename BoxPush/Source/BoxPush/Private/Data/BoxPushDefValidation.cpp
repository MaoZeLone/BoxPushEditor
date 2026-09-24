#include "Data/BoxPushDefValidation.h"
#include "Data/VisualComps.h"

void BoxPushDefValidation::AddIssue(TArray<FLevelValidationIssue>& OutIssues, bool bError, const FString& Text)
{
	FLevelValidationIssue Issue;
	Issue.bError = bError;
	Issue.Message = FText::FromString(Text);
	OutIssues.Add(Issue);
}

void BoxPushDefValidation::ValidateStates(
	const TArray<FInteractableStateDef>& States,
	TArray<FLevelValidationIssue>& OutIssues)
{
	TSet<FName> SeenStates;
	int32 DefaultCount = 0;
	for (const FInteractableStateDef& State : States)
	{
		if (State.StateId.IsNone())
		{
			AddIssue(OutIssues, true, TEXT("State is missing StateId"));
			continue;
		}
		if (SeenStates.Contains(State.StateId))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("Duplicate StateId: %s"), *State.StateId.ToString()));
		}
		SeenStates.Add(State.StateId);
		if (State.bDefault)
		{
			++DefaultCount;
		}

		auto CheckActions = [&](const TArray<FInteractableStateActionDef>& Actions, const TCHAR* Slot)
		{
			for (int32 Index = 0; Index < Actions.Num(); ++Index)
			{
				const FInteractableStateActionDef& Action = Actions[Index];
				if (Action.Type == EBoxStateActionType::None)
				{
					AddIssue(OutIssues, true, FString::Printf(
						TEXT("State %s %s[%d] has no action type"),
						*State.StateId.ToString(), Slot, Index));
				}
				else if (Action.Type == EBoxStateActionType::FireEvent && Action.EventId.IsNone())
				{
					AddIssue(OutIssues, true, FString::Printf(
						TEXT("State %s %s[%d] is missing EventId"),
						*State.StateId.ToString(), Slot, Index));
				}
			}
		};
		CheckActions(State.OnEnter, TEXT("OnEnter"));
		CheckActions(State.OnStay, TEXT("OnStay"));
		CheckActions(State.OnExit, TEXT("OnExit"));
	}
	if (States.Num() == 0)
	{
		AddIssue(OutIssues, true, TEXT("At least one state is required"));
	}
	else if (DefaultCount != 1)
	{
		AddIssue(OutIssues, true, TEXT("States must have exactly one bDefault"));
	}
}

void BoxPushDefValidation::ValidateTransitions(
	const TArray<FInteractableStateDef>& States,
	const TArray<FInteractableTransitionDef>& Transitions,
	TArray<FLevelValidationIssue>& OutIssues)
{
	TSet<FName> KnownStates;
	for (const FInteractableStateDef& State : States)
	{
		if (!State.StateId.IsNone())
		{
			KnownStates.Add(State.StateId);
		}
	}

	TSet<FString> SeenKeys;
	for (int32 Index = 0; Index < Transitions.Num(); ++Index)
	{
		const FInteractableTransitionDef& Row = Transitions[Index];
		if (!Row.FromState.IsNone() && !KnownStates.Contains(Row.FromState))
		{
			AddIssue(OutIssues, true, FString::Printf(
				TEXT("Transition[%d] FromState %s does not exist"),
				Index, *Row.FromState.ToString()));
		}
		if (Row.ToState.IsNone())
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("Transition[%d] is missing ToState"), Index));
		}
		else if (!KnownStates.Contains(Row.ToState))
		{
			AddIssue(OutIssues, true, FString::Printf(
				TEXT("Transition[%d] ToState %s does not exist"),
				Index, *Row.ToState.ToString()));
		}
		if (Row.Condition == EBoxTransitionCondition::OnEvent && Row.EventId.IsNone())
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("Transition[%d] is missing EventId"), Index));
		}

		const FString Key = FString::Printf(
			TEXT("%s|%d|%s"),
			*Row.FromState.ToString(),
			static_cast<int32>(Row.Condition),
			*Row.EventId.ToString());
		if (SeenKeys.Contains(Key))
		{
			AddIssue(OutIssues, true, FString::Printf(
				TEXT("Transition[%d] duplicates an earlier from-state and condition"),
				Index));
		}
		SeenKeys.Add(Key);
	}
}

void BoxPushDefValidation::ValidateVisualTree(
	const TArray<UInteractableVisualComp*>& Comps,
	TArray<FLevelValidationIssue>& OutIssues)
{
	static const TArray<FInteractableStateDef> Empty;
	ValidateVisualTree(Comps, Empty, OutIssues);
}

void BoxPushDefValidation::ValidateVisualTree(
	const TArray<UInteractableVisualComp*>& Comps,
	const TArray<FInteractableStateDef>& States,
	TArray<FLevelValidationIssue>& OutIssues)
{
	TSet<FName> KnownStates;
	for (const FInteractableStateDef& State : States)
	{
		if (!State.StateId.IsNone())
		{
			KnownStates.Add(State.StateId);
		}
	}

	TMap<FName, int32> IdToIndex;
	for (int32 Index = 0; Index < Comps.Num(); ++Index)
	{
		const UInteractableVisualComp* Comp = Comps[Index];
		if (!Comp)
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("VisualComps[%d] 为空"), Index));
			continue;
		}
		if (Comp->CompId.IsNone())
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("VisualComps[%d] 缺少 CompId"), Index));
			continue;
		}
		if (IdToIndex.Contains(Comp->CompId))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("Visual CompId 重复：%s"), *Comp->CompId.ToString()));
		}
		IdToIndex.Add(Comp->CompId, Index);

		for (const FName StateId : Comp->VisibleInStates)
		{
			if (KnownStates.Num() == 0)
			{
				break;
			}
			if (!StateId.IsNone() && !KnownStates.Contains(StateId))
			{
				AddIssue(OutIssues, true, FString::Printf(
					TEXT("表现组件 %s 的 VisibleInStates 引用了未知状态 %s"),
					*Comp->CompId.ToString(), *StateId.ToString()));
			}
		}
	}

	TSet<FName> Visiting;
	TSet<FName> Visited;
	TFunction<bool(FName)> WalkCycle = [&](FName CompId) -> bool
	{
		if (Visited.Contains(CompId))
		{
			return false;
		}
		if (Visiting.Contains(CompId))
		{
			return true;
		}
		Visiting.Add(CompId);
		const int32* Found = IdToIndex.Find(CompId);
		if (Found && Comps.IsValidIndex(*Found) && Comps[*Found] && !Comps[*Found]->ParentId.IsNone())
		{
			if (WalkCycle(Comps[*Found]->ParentId))
			{
				return true;
			}
		}
		Visiting.Remove(CompId);
		Visited.Add(CompId);
		return false;
	};

	for (const UInteractableVisualComp* Comp : Comps)
	{
		if (!Comp || Comp->CompId.IsNone())
		{
			continue;
		}
		if (!Comp->ParentId.IsNone())
		{
			if (Comp->ParentId == Comp->CompId)
			{
				AddIssue(OutIssues, true, FString::Printf(TEXT("表现组件 %s 的 ParentId 指向自己"), *Comp->CompId.ToString()));
			}
			else if (!IdToIndex.Contains(Comp->ParentId))
			{
				AddIssue(OutIssues, true, FString::Printf(
					TEXT("表现组件 %s 的 ParentId %s 不存在"),
					*Comp->CompId.ToString(), *Comp->ParentId.ToString()));
			}
		}
		if (WalkCycle(Comp->CompId))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("表现组件树成环，含 %s"), *Comp->CompId.ToString()));
		}
	}
}
