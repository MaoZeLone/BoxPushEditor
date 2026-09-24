#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Match/BoxBoard.h"
#include "BoxInteractableActor.generated.h"

class UInteractableDef;
class USceneComponent;
class UVisualSpriteComp;

UCLASS()
class BOXPUSH_API ABoxInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	ABoxInteractableActor();

	void SetupFromInstance(const FBoxRuntimeInstance& Inst);
	void SetCell(FIntPoint Cell, bool bInstant);
	void SetState(FName StateId);
	FName GetInstanceId() const { return InstanceId; }

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

private:
	void ClearVisuals();
	void RebuildSprites();
	void RefreshVisibility();

	UPROPERTY()
	TObjectPtr<const UInteractableDef> Def;

	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> VisualNodes;

	FName InstanceId;
	FName CurrentState;
	FVector FromWorld = FVector::ZeroVector;
	FVector ToWorld = FVector::ZeroVector;
	float InterpAlpha = 1.f;
	float InterpDuration = 0.36f;
};
