#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/InteractableState.h"
#include "Data/InteractableStateRuntime.h"
#include "Match/BoxBoard.h"
#include "BoxGridSim.generated.h"

/**
 * 格子模拟：一次有效输入 = 一步。先改格子，再让表现跟。
 * 不判断角色能不能发（那是 ActionSet + Relation），只吃方向和 bCanPush。
 */
UCLASS()
class BOXPUSH_API UBoxGridSim : public UObject
{
	GENERATED_BODY()

public:
	bool BindAndStart(UBoxBoard* InBoard);

	bool TryPlayerMove(FIntPoint Dir, bool bCanPush, FBoxStepResult& Out);
	bool Undo(FBoxStepResult& Out);
	bool Redo(FBoxStepResult& Out);
	void Restart(FBoxStepResult& Out);
	void FlushImmediateReturns(TArray<FBoxInstanceDelta>& OutMoves, TArray<FName>* OutFiredEvents = nullptr, TArray<FVisualTransitionCue>* OutVisuals = nullptr);

	void SetPaused(bool bInPaused) { bPaused = bInPaused; }
	bool IsPaused() const { return bPaused; }
	bool IsWon() const { return bWon; }
	bool CanUndo() const { return !bPaused && UndoStack.Num() > 0; }
	bool CanRedo() const { return !bPaused && RedoStack.Num() > 0; }
	UBoxBoard* GetBoard() const { return Board; }

private:
	struct FSnapshot
	{
		FIntPoint PlayerCell = FIntPoint::ZeroValue;
		TArray<FBoxRuntimeInstance> Instances;
		bool bWon = false;
	};

	int32 PushTravel(FBoxRuntimeInstance& Box, FIntPoint Dir) const;
	void ApplyEvent(FBoxRuntimeInstance& Inst, FName EventId, FBoxInstanceDelta* Delta);
	void ApplyCondition(FBoxRuntimeInstance& Inst, EBoxTransitionCondition Condition);
	bool ApplyResolvedState(FBoxRuntimeInstance& Inst, FName NewState, FBoxInstanceDelta* Delta);
	void NoteVisualTransition(const FBoxRuntimeInstance& Inst, FName FromState, EBoxTransitionCondition Condition, FName EventId, FName ToState);
	bool IsTriggerOccupied(const FBoxRuntimeInstance& Inst) const;
	void RefreshOccupancyStates();
	void RunStateActions(FBoxRuntimeInstance& Inst, FName StateId, EBoxStateActionPhase Phase);
	void TickStayActions();
	void BeginActionDispatch(TArray<FName>* FiredEvents);
	void EndActionDispatch();
	void EnsureEventListener();
	void DrainBroadcast();
	void DeliverBroadcast(FName EventId);
	virtual void BeginDestroy() override;
	bool ComputeWon() const;
	void Capture(FSnapshot& Out) const;
	void Restore(const FSnapshot& In);
	bool RestoreFromStack(TArray<FSnapshot>& Source, TArray<FSnapshot>& Dest, FBoxStepResult& Out);
	void MakePlayerDelta(FBoxStepResult& Out, FIntPoint From, FIntPoint To) const;
	void TickReturnTimers(FName SkipId);
	void FillMovedDeltas(const TArray<FBoxRuntimeInstance>& BeforeInstances, FBoxStepResult& Out) const;

	UPROPERTY()
	TObjectPtr<UBoxBoard> Board;

	bool bWon = false;
	bool bPaused = false;

	FSnapshot Initial;
	TArray<FSnapshot> UndoStack;
	TArray<FSnapshot> RedoStack;

	TArray<FName>* ActionFiredSink = nullptr;
	TArray<FName> PendingBroadcast;
	TSet<uint64> DeliveredThisStep;
	TSet<FName> EnteredThisStep;
	bool bDispatchStateActions = false;
	bool bListening = false;
	bool bDraining = false;
	TArray<FVisualTransitionCue> StepVisuals;
};
