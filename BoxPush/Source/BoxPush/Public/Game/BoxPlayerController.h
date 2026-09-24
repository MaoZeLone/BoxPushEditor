#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "BoxPlayerController.generated.h"

UCLASS()
class BOXPUSH_API ABoxPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	ABoxPlayerCameraManager();

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
};

UCLASS()
class BOXPUSH_API ABoxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABoxPlayerController();

	virtual void BeginPlay() override;
	virtual void AcknowledgePossession(APawn* P) override;

	void ApplyMenuInput(class UUserWidget* FocusWidget = nullptr);
	void ApplyMatchInput(class UUserWidget* FocusWidget);
	void LockBoardView();
	void UnlockTopDownView();
};
