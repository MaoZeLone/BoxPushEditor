#include "Match/BoxGridSim.h"

#include "Data/BoxInstanceParams.h"
#include "Data/BoxTypeDisplay.h"
#include "Data/InteractableDef.h"
#include "Data/InteractableLogic.h"
#include "Data/InteractableStateRuntime.h"
#include "GMPCore.h"

DEFINE_LOG_CATEGORY_STATIC(LogBoxGridSim, Log, All);

void UBoxGridSim::BeginActionDispatch(TArray<FName>* FiredEvents)
{
	EnteredThisStep.Reset();
	StepVisuals.Reset();
	ActionFiredSink = FiredEvents;
	bDispatchStateActions = true;
}

void UBoxGridSim::BeginDestroy()
{
	if (bListening)
	{
		FGMPHelper::UnbindMessage(MSGKEY("BoxPush.Interactable.Event"), this);
		bListening = false;
	}
	Super::BeginDestroy();
}

void UBoxGridSim::EnsureEventListener()
{
	if (bListening || !GetWorld())
	{
		return;
	}

	bListening = true;
	FGMPHelper::ListenWorldMessage(
		this,
		MSGKEY("BoxPush.Interactable.Event"),
		this,
		[this](const FBoxInteractableEvent& Event)
		{
			DeliverBroadcast(Event.EventId);
		});
}

void UBoxGridSim::DeliverBroadcast(FName EventId)
{
	if (!Board || EventId.IsNone())
	{
		return;
	}

	for (FBoxRuntimeInstance& Inst : Board->EditInstances())
	{
		const uint64 Key = (uint64(GetTypeHash(Inst.InstanceId)) << 32) ^ uint64(GetTypeHash(EventId));
		if (DeliveredThisStep.Contains(Key))
		{
			continue;
		}
		DeliveredThisStep.Add(Key);
		ApplyEvent(Inst, EventId, nullptr);
	}
}

void UBoxGridSim::DrainBroadcast()
{
	if (bDraining || PendingBroadcast.Num() == 0)
	{
		return;
	}

	EnsureEventListener();
	bDraining = true;
	for (int32 Generation = 0; Generation < 4 && PendingBroadcast.Num() > 0; ++Generation)
	{
		TArray<FName> Batch = MoveTemp(PendingBroadcast);
		PendingBroadcast.Reset();
		for (const FName EventId : Batch)
		{
			if (EventId.IsNone())
			{
				continue;
			}
			if (!GetWorld())
			{
				DeliverBroadcast(EventId);
				continue;
			}
			FBoxInteractableEvent Payload;
			Payload.EventId = EventId;
			FGMPHelper::NotifyWorldMessage(this, MSGKEY("BoxPush.Interactable.Event"), Payload);
		}
	}
	if (PendingBroadcast.Num() > 0)
	{
		UE_LOG(LogBoxGridSim, Warning, TEXT("交互物事件连锁超过 4 层，后面的不再送"));
		PendingBroadcast.Reset();
	}
	bDraining = false;
}

void UBoxGridSim::EndActionDispatch()
{
	DrainBroadcast();
	ActionFiredSink = nullptr;
	bDispatchStateActions = false;
	EnteredThisStep.Reset();
	DeliveredThisStep.Reset();
	PendingBroadcast.Reset();
}

void UBoxGridSim::RunStateActions(FBoxRuntimeInstance& Inst, FName StateId, EBoxStateActionPhase Phase)
{
	if (!Inst.Def || StateId.IsNone())
	{
		return;
	}

	const FInteractableStateDef* State = Inst.Def->FindState(StateId);
	if (!State)
	{
		return;
	}

	const TArray<FInteractableStateActionDef>* Actions = nullptr;
	switch (Phase)
	{
	case EBoxStateActionPhase::Enter:
		Actions = &State->OnEnter;
		break;
	case EBoxStateActionPhase::Stay:
		Actions = &State->OnStay;
		break;
	case EBoxStateActionPhase::Exit:
		Actions = &State->OnExit;
		break;
	}
	if (!Actions)
	{
		return;
	}

	FBoxStateActionContext Context;
	Context.InstanceId = Inst.InstanceId;
	Context.StateId = StateId;
	Context.Phase = Phase;
	Context.Instance = &Inst;
	Context.FiredEvents = ActionFiredSink;
	Context.BroadcastQueue = &PendingBroadcast;
	ExecuteInteractableStateActions(*Actions, Context);
}

void UBoxGridSim::TickStayActions()
{
	if (!Board)
	{
		return;
	}

	for (FBoxRuntimeInstance& Inst : Board->EditInstances())
	{
		if (EnteredThisStep.Contains(Inst.InstanceId))
		{
			continue;
		}
		RunStateActions(Inst, Inst.CurrentState, EBoxStateActionPhase::Stay);
	}
}

bool UBoxGridSim::BindAndStart(UBoxBoard* InBoard)
{
	Board = InBoard;
	bWon = false;
	bPaused = false;
	UndoStack.Reset();
	RedoStack.Reset();
	EndActionDispatch();

	if (!Board)
	{
		UE_LOG(LogBoxGridSim, Error, TEXT("BindAndStart: Board 为空"));
		return false;
	}

	RefreshOccupancyStates();

	TArray<FName> SpawnEvents;
	BeginActionDispatch(&SpawnEvents);
	for (FBoxRuntimeInstance& Inst : Board->EditInstances())
	{
		RunStateActions(Inst, Inst.CurrentState, EBoxStateActionPhase::Enter);
	}
	EndActionDispatch();
	for (const FName EventId : SpawnEvents)
	{
		UE_LOG(LogBoxGridSim, Log, TEXT("Spawn state action event %s"), *EventId.ToString());
	}

	bWon = ComputeWon();
	Capture(Initial);
	return true;
}

int32 UBoxGridSim::PushTravel(FBoxRuntimeInstance& Box, FIntPoint Dir) const
{
	if (!Board)
	{
		return 0;
	}

	const UPushableLogic* Pushable = Box.Def ? Box.Def->FindLogic<UPushableLogic>() : nullptr;
	if (!Pushable)
	{
		return 0;
	}

	const USlideLogic* Slide = Box.Def->FindLogic<USlideLogic>();
	const bool bSlide = Slide != nullptr;
	const int32 MaxSteps = bSlide ? FMath::Max(Board->GetWidth(), Board->GetHeight()) : BoxInstanceParams::PushSteps(Box);

	int32 Travel = 0;
	FIntPoint Cursor = Box.Cell;
	for (int32 Step = 0; Step < MaxSteps; ++Step)
	{
		const FIntPoint Next = Board->GetNeighbor(Cursor, Dir);
		if (!Board->IsStandable(Next))
		{
			break;
		}
		if (Board->BlocksPush(Next, Box.InstanceId))
		{
			const bool bPassThroughBox = bSlide && !BoxInstanceParams::StopOnBox(Box) && Board->HasPushableAt(Next, Box.InstanceId);
			if (!bPassThroughBox)
			{
				break;
			}
		}
		Cursor = Next;
		++Travel;
	}
	return Travel;
}

void UBoxGridSim::NoteVisualTransition(const FBoxRuntimeInstance& Inst, FName FromState, EBoxTransitionCondition Condition, FName EventId, FName ToState)
{
	if (!bDispatchStateActions)
	{
		return;
	}
	FVisualTransitionCue Cue;
	Cue.InstanceId = Inst.InstanceId;
	Cue.FromState = FromState;
	Cue.Condition = Condition;
	Cue.EventId = EventId;
	Cue.ToState = ToState;
	StepVisuals.Add(Cue);
}

bool UBoxGridSim::ApplyResolvedState(FBoxRuntimeInstance& Inst, FName NewState, FBoxInstanceDelta* Delta)
{
	if (NewState.IsNone() || NewState == Inst.CurrentState)
	{
		return false;
	}

	if (bDispatchStateActions)
	{
		RunStateActions(Inst, Inst.CurrentState, EBoxStateActionPhase::Exit);
		EnteredThisStep.Add(Inst.InstanceId);
	}

	Inst.CurrentState = NewState;
	if (Delta)
	{
		Delta->NewState = NewState;
	}

	if (bDispatchStateActions)
	{
		RunStateActions(Inst, NewState, EBoxStateActionPhase::Enter);
	}
	return true;
}

void UBoxGridSim::ApplyEvent(FBoxRuntimeInstance& Inst, FName EventId, FBoxInstanceDelta* Delta)
{
	if (!Inst.Def)
	{
		return;
	}
	const FName FromState = Inst.CurrentState;
	const FName ToState = Inst.Def->ResolveTransition(FromState, EventId, &Inst.Overrides);
	if (ApplyResolvedState(Inst, ToState, Delta))
	{
		NoteVisualTransition(Inst, FromState, EBoxTransitionCondition::OnEvent, EventId, ToState);
	}
}

void UBoxGridSim::ApplyCondition(FBoxRuntimeInstance& Inst, EBoxTransitionCondition Condition)
{
	if (!Inst.Def)
	{
		return;
	}
	const FName FromState = Inst.CurrentState;
	const FName ToState = Inst.Def->ResolveCondition(FromState, Condition);
	if (ApplyResolvedState(Inst, ToState, nullptr))
	{
		NoteVisualTransition(Inst, FromState, Condition, NAME_None, ToState);
	}
}

bool UBoxGridSim::IsTriggerOccupied(const FBoxRuntimeInstance& Inst) const
{
	if (!Board || !Inst.Def || !Inst.Def->FindLogic<UTriggerLogic>())
	{
		return false;
	}

	const FGameplayTag Accept = BoxInstanceParams::TriggerAcceptType(Inst);
	const bool bAnyone = !Accept.IsValid();
	if (bAnyone && Board->GetPlayerCell() == Inst.Cell)
	{
		return true;
	}
	for (const FBoxRuntimeInstance& Other : Board->GetInstances())
	{
		if (Other.InstanceId == Inst.InstanceId || Other.Cell != Inst.Cell || !Other.Def)
		{
			continue;
		}
		if (bAnyone || UBoxTypeDisplayLibrary::MatchesType(Other.Def->Type, Accept))
		{
			return true;
		}
	}
	return false;
}

void UBoxGridSim::RefreshOccupancyStates()
{
	if (!Board)
	{
		return;
	}

	TArray<FBoxRuntimeInstance>& Instances = Board->EditInstances();
	for (FBoxRuntimeInstance& Inst : Instances)
	{
		if (!Inst.Def)
		{
			continue;
		}

		bool bOccupied = false;
		if (const UGoalLogic* Goal = Inst.Def->FindLogic<UGoalLogic>())
		{
			bOccupied = Goal->IsWinSatisfied(*Board, Inst);
			ApplyEvent(Inst, bOccupied ? BoxInstanceParams::OccupiedEvent(Inst) : BoxInstanceParams::ClearedEvent(Inst), nullptr);
		}

		if (Inst.Def->FindLogic<UPedalLogic>())
		{
			bOccupied = false;
			const FGameplayTag Accept = BoxInstanceParams::AcceptType(Inst);
			const bool bPlayerCounts = !Accept.IsValid();
			if (bPlayerCounts && Board->GetPlayerCell() == Inst.Cell)
			{
				bOccupied = true;
			}
			for (const FBoxRuntimeInstance& Other : Instances)
			{
				if (Other.InstanceId == Inst.InstanceId || Other.Cell != Inst.Cell || !Other.Def)
				{
					continue;
				}
				if (!Accept.IsValid() || UBoxTypeDisplayLibrary::MatchesType(Other.Def->Type, Accept))
				{
					bOccupied = true;
					break;
				}
			}
			ApplyEvent(Inst, bOccupied ? BoxInstanceParams::OccupiedEvent(Inst) : BoxInstanceParams::ClearedEvent(Inst), nullptr);
		}

		if (Inst.Def->FindLogic<UTriggerLogic>())
		{
			const bool bNow = IsTriggerOccupied(Inst);
			if (bNow != Inst.bTriggerOccupied)
			{
				ApplyCondition(Inst, bNow ? EBoxTransitionCondition::BeginOverlap : EBoxTransitionCondition::EndOverlap);
				Inst.bTriggerOccupied = bNow;
			}
		}
	}
}

bool UBoxGridSim::ComputeWon() const
{
	if (!Board)
	{
		return false;
	}

	bool bHasWinComp = false;
	for (const FBoxRuntimeInstance& Inst : Board->GetInstances())
	{
		if (!Inst.Def)
		{
			continue;
		}
		for (const UInteractableLogicComp* Comp : Inst.Def->LogicComps)
		{
			if (!Comp || !Comp->CountsTowardWin())
			{
				continue;
			}
			bHasWinComp = true;
			if (!Comp->IsWinSatisfied(*Board, Inst))
			{
				return false;
			}
		}
	}
	return bHasWinComp;
}

void UBoxGridSim::Capture(FSnapshot& Out) const
{
	if (!Board)
	{
		return;
	}
	Out.PlayerCell = Board->GetPlayerCell();
	Out.Instances = Board->GetInstances();
	Out.bWon = bWon;
}

void UBoxGridSim::Restore(const FSnapshot& In)
{
	if (!Board)
	{
		return;
	}
	Board->SetPlayerCell(In.PlayerCell);
	Board->EditInstances() = In.Instances;
	bWon = In.bWon;
}

bool UBoxGridSim::RestoreFromStack(TArray<FSnapshot>& Source, TArray<FSnapshot>& Dest, FBoxStepResult& Out)
{
	Out = FBoxStepResult();
	if (!Board || bPaused || Source.Num() == 0)
	{
		return false;
	}

	FSnapshot Current;
	Capture(Current);
	const FIntPoint From = Board->GetPlayerCell();
	TArray<FBoxRuntimeInstance> BeforeInstances = Board->GetInstances();
	Dest.Add(MoveTemp(Current));
	Restore(Source.Pop());
	MakePlayerDelta(Out, From, Board->GetPlayerCell());
	FillMovedDeltas(BeforeInstances, Out);
	Out.bApplied = true;
	Out.bWon = bWon;
	return true;
}

void UBoxGridSim::TickReturnTimers(FName SkipId)
{
	if (!Board)
	{
		return;
	}
	for (FBoxRuntimeInstance& Inst : Board->EditInstances())
	{
		if (Inst.InstanceId == SkipId || Inst.MovesUntilReturn <= 0)
		{
			continue;
		}
		--Inst.MovesUntilReturn;
	}
}

void UBoxGridSim::MakePlayerDelta(FBoxStepResult& Out, FIntPoint From, FIntPoint To) const
{
	Out.PlayerFrom = From;
	Out.PlayerTo = To;
}

void UBoxGridSim::FillMovedDeltas(const TArray<FBoxRuntimeInstance>& BeforeInstances, FBoxStepResult& Out) const
{
	if (!Board)
	{
		return;
	}

	TMap<FName, TPair<FIntPoint, FName>> Old;
	for (const FBoxRuntimeInstance& Inst : BeforeInstances)
	{
		Old.Add(Inst.InstanceId, TPair<FIntPoint, FName>(Inst.Cell, Inst.CurrentState));
	}
	for (const FBoxRuntimeInstance& Inst : Board->GetInstances())
	{
		const TPair<FIntPoint, FName>* Prev = Old.Find(Inst.InstanceId);
		if (!Prev)
		{
			continue;
		}
		if (Prev->Key != Inst.Cell || Prev->Value != Inst.CurrentState)
		{
			FBoxInstanceDelta Delta;
			Delta.InstanceId = Inst.InstanceId;
			Delta.From = Prev->Key;
			Delta.To = Inst.Cell;
			Delta.NewState = Inst.CurrentState;
			Out.Moves.Add(Delta);
		}
	}
}

bool UBoxGridSim::TryPlayerMove(FIntPoint Dir, bool bCanPush, FBoxStepResult& Out)
{
	Out = FBoxStepResult();
	if (!Board || bPaused || bWon)
	{
		return false;
	}
	if (FMath::Abs(Dir.X) + FMath::Abs(Dir.Y) != 1)
	{
		return false;
	}

	const FIntPoint From = Board->GetPlayerCell();
	const FIntPoint Dest = Board->GetNeighbor(From, Dir);
	if (!Board->IsStandable(Dest))
	{
		return false;
	}

	if (Board->BlocksPlayer(Dest))
	{
		FBoxRuntimeInstance* PushTarget = Board->FindPushableBlockingPlayer(Dest);
		if (!PushTarget || !bCanPush)
		{
			return false;
		}

		const int32 Travel = PushTravel(*PushTarget, Dir);
		if (Travel <= 0)
		{
			return false;
		}

		FSnapshot Before;
		Capture(Before);

		FBoxInstanceDelta Delta;
		Delta.InstanceId = PushTarget->InstanceId;
		Delta.From = PushTarget->Cell;
		Delta.To = Board->GetNeighbor(PushTarget->Cell, Dir * Travel);
		PushTarget->ReturnHome = PushTarget->Cell;
		PushTarget->Cell = Delta.To;
		if (const UReturnLogic* ReturnLogic = PushTarget->Def->FindLogic<UReturnLogic>())
		{
			PushTarget->MovesUntilReturn = BoxInstanceParams::DelayMoves(*PushTarget);
		}
		Delta.NewState = PushTarget->CurrentState;
		Out.Moves.Add(Delta);
		Out.bPushed = true;

		Board->SetPlayerCell(Dest);
		TickReturnTimers(PushTarget->InstanceId);
		BeginActionDispatch(&Out.FiredEvents);
		RefreshOccupancyStates();
		TickStayActions();
		EndActionDispatch();
		bWon = ComputeWon();
		UndoStack.Add(MoveTemp(Before));
		RedoStack.Reset();
		MakePlayerDelta(Out, From, Dest);
		Out.VisualTransitions = StepVisuals;
		Out.bApplied = true;
		Out.bWon = bWon;
		return true;
	}

	FSnapshot Before;
	Capture(Before);
	Board->SetPlayerCell(Dest);
	TickReturnTimers(NAME_None);
	BeginActionDispatch(&Out.FiredEvents);
	RefreshOccupancyStates();
	TickStayActions();
	EndActionDispatch();
	bWon = ComputeWon();
	UndoStack.Add(MoveTemp(Before));
	RedoStack.Reset();
	MakePlayerDelta(Out, From, Dest);
	Out.VisualTransitions = StepVisuals;
	Out.bApplied = true;
	Out.bWon = bWon;
	return true;
}

void UBoxGridSim::FlushImmediateReturns(TArray<FBoxInstanceDelta>& OutMoves, TArray<FName>* OutFiredEvents, TArray<FVisualTransitionCue>* OutVisuals)
{
	OutMoves.Reset();
	if (!Board)
	{
		return;
	}

	TArray<FBoxRuntimeInstance>& Instances = Board->EditInstances();
	for (FBoxRuntimeInstance& Inst : Instances)
	{
		if (Inst.MovesUntilReturn != 0 || !Inst.Def)
		{
			continue;
		}

		const FIntPoint Home = Inst.ReturnHome;
		if (Home != Inst.Cell && Board->IsStandable(Home) && !Board->BlocksPush(Home, Inst.InstanceId) && Home != Board->GetPlayerCell())
		{
			FBoxInstanceDelta Delta;
			Delta.InstanceId = Inst.InstanceId;
			Delta.From = Inst.Cell;
			Delta.To = Home;
			Inst.Cell = Home;
			Delta.NewState = Inst.CurrentState;
			OutMoves.Add(Delta);
		}
		Inst.MovesUntilReturn = -1;
	}

	BeginActionDispatch(OutFiredEvents);
	RefreshOccupancyStates();
	EndActionDispatch();
	if (OutVisuals)
	{
		*OutVisuals = StepVisuals;
	}
	bWon = ComputeWon();
}

bool UBoxGridSim::Undo(FBoxStepResult& Out)
{
	return RestoreFromStack(UndoStack, RedoStack, Out);
}

bool UBoxGridSim::Redo(FBoxStepResult& Out)
{
	return RestoreFromStack(RedoStack, UndoStack, Out);
}

void UBoxGridSim::Restart(FBoxStepResult& Out)
{
	Out = FBoxStepResult();
	if (!Board)
	{
		return;
	}

	bPaused = false;
	const FIntPoint From = Board->GetPlayerCell();
	TArray<FBoxRuntimeInstance> BeforeInstances = Board->GetInstances();
	Restore(Initial);
	UndoStack.Reset();
	RedoStack.Reset();
	MakePlayerDelta(Out, From, Board->GetPlayerCell());
	FillMovedDeltas(BeforeInstances, Out);
	Out.bApplied = true;
	Out.bWon = bWon;
}
