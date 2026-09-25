#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Match/BoxBoard.h"
#include "Match/BoxGridSim.h"
#include "BoxMatchWorld.generated.h"

class ABoxInteractableActor;
class ABoxPlayerCharacter;
class UStaticMeshComponent;
class UCameraComponent;
class ULevelData;
class UVisualSpriteComp;
struct FBoxSpriteRect;

UCLASS()
class BOXPUSH_API ABoxMatchWorld : public AActor
{
	GENERATED_BODY()

public:
	ABoxMatchWorld();

	bool StartLevel(const ULevelData* Level);
	void BindPlayer(ABoxPlayerCharacter* InPlayer);
	void ActivateSpriteCamera();

	bool RequestMove(ABoxPlayerCharacter* Player, FIntPoint Dir);
	bool EnqueueMove(ABoxPlayerCharacter* Player, FIntPoint Dir);
	void SetHeldMoveDir(FIntPoint Dir);
	void ArmHoldRepeat();
	bool RequestUndo(ABoxPlayerCharacter* Player);
	bool RequestRedo(ABoxPlayerCharacter* Player);
	bool RequestRestart(ABoxPlayerCharacter* Player);
	bool RequestPause(ABoxPlayerCharacter* Player);

	bool IsStepping() const { return bStepping; }
	FTransform GetPlayerSpawnTransform() const;
	UBoxBoard* GetBoard() const { return Board; }
	UBoxGridSim* GetSim() const { return Sim; }

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "BoxPush")
	float StepDuration = 0.36f;

private:
	void RebuildPresentation();
	void ClearTileSprites();
	UStaticMeshComponent* SpawnTileSprite(const UVisualSpriteComp* Comp, FIntPoint Cell);
	void BeginStep(const FBoxStepResult& Result, FGameplayTag StateTag);
	void FinishPrimaryPhase();
	void FinishStep();
	void ApplyDeltas(const TArray<FBoxInstanceDelta>& Moves, bool bInstant);
	void ApplyVisualCues(const TArray<FVisualTransitionCue>& Cues);
	void SnapPlayer(FIntPoint Cell);
	void FacePlayer(FIntPoint From, FIntPoint To);
	void SetPlayerTags(FGameplayTag StateTag, bool bLocked);
	void NotifyStatus();
	void ClearMoveQueue();
	void DrainMoveQueue();
	void TryContinueHeldMove();
	void ApplySpriteCamera();

	int32 ViewLockRemain = 0;

	UPROPERTY()
	TObjectPtr<UBoxBoard> Board;

	UPROPERTY()
	TObjectPtr<UBoxGridSim> Sim;

	UPROPERTY()
	TObjectPtr<ABoxPlayerCharacter> Player;

	UPROPERTY()
	TMap<FName, TObjectPtr<ABoxInteractableActor>> Actors;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TileSprites;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> SpriteView;

	FBoxStepResult ActiveStep;
	FGameplayTag ActiveStateTag;
	float StepElapsed = 0.f;
	bool bStepping = false;
	bool bReturnPhase = false;

	TArray<FIntPoint> MoveQueue;
	FIntPoint HeldMoveDir = FIntPoint::ZeroValue;
	FIntPoint LastHeldAttempt = FIntPoint::ZeroValue;
	bool bHoldRepeatArmed = false;
	static constexpr int32 MaxMoveQueue = 8;
};
