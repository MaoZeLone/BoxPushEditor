#include "Data/InteractableDef.h"
#include "BoxPushTags.h"
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

FName UInteractableDef::ResolveTransition(FName FromState, FName EventId) const
{
	if (EventId.IsNone())
	{
		return NAME_None;
	}

	for (const FInteractableTransitionDef& Row : Transitions)
	{
		if (Row.Condition != EBoxTransitionCondition::OnEvent || Row.EventId != EventId)
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
	if (!Type.IsValid())
	{
		AddIssue(OutIssues, true, TEXT("未配置类型 Tag"));
	}
}
