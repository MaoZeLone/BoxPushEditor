#include "Game/BoxPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraTypes.h"
#include "EngineUtils.h"
#include "Game/BoxGameMode.h"
#include "GameFramework/SpectatorPawn.h"
#include "Match/BoxBoard.h"
#include "Match/BoxGrid.h"
#include "Match/BoxMatchWorld.h"

ABoxPlayerCameraManager::ABoxPlayerCameraManager()
{
	bIsOrthographic = false;
	bAutoCalculateOrthoPlanes = false;
	ViewPitchMin = -90.f;
	ViewPitchMax = 90.f;
}

void ABoxPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);

	const UWorld* World = GetWorld();
	const ABoxGameMode* GM = World ? World->GetAuthGameMode<ABoxGameMode>() : nullptr;
	const ABoxMatchWorld* Match = GM ? GM->GetMatchWorld() : nullptr;
	if (!Match && World)
	{
		for (TActorIterator<ABoxMatchWorld> It(World); It; ++It)
		{
			Match = *It;
			break;
		}
	}
	const UBoxBoard* Board = Match ? Match->GetBoard() : nullptr;
	if (!Board || Board->GetWidth() <= 0 || Board->GetHeight() <= 0)
	{
		return;
	}

	OutVT.POV.Location = BoxPlayCamera::ViewLocation(Board->GetWidth(), Board->GetHeight());
	OutVT.POV.Rotation = BoxPlayCamera::Rotation();
	OutVT.POV.FOV = BoxPlayCamera::FieldOfView();
	OutVT.POV.ProjectionMode = ECameraProjectionMode::Perspective;
	OutVT.POV.bConstrainAspectRatio = false;
	OutVT.POV.bAutoCalculateOrthoPlanes = false;
}

ABoxPlayerController::ABoxPlayerController()
{
	PlayerCameraManagerClass = ABoxPlayerCameraManager::StaticClass();
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bAutoManageActiveCameraTarget = false;
}

void ABoxPlayerController::BeginPlay()
{
	Super::BeginPlay();
	LockBoardView();
}

void ABoxPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	LockBoardView();
}

void ABoxPlayerController::UnlockTopDownView()
{
	bAutoManageActiveCameraTarget = false;
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -90.f;
		PlayerCameraManager->ViewPitchMax = 90.f;
		PlayerCameraManager->ViewYawMin = 0.f;
		PlayerCameraManager->ViewYawMax = 359.999f;
		PlayerCameraManager->ViewRollMin = -180.f;
		PlayerCameraManager->ViewRollMax = 180.f;
	}
}

void ABoxPlayerController::LockBoardView()
{
	UnlockTopDownView();
	if (ABoxGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABoxGameMode>() : nullptr)
	{
		if (ABoxMatchWorld* Match = GM->GetMatchWorld())
		{
			Match->ActivateSpriteCamera();
			return;
		}
	}

	AActor* View = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACameraActor> It(World); It; ++It)
		{
			View = *It;
			break;
		}
	}
	if (!View)
	{
		View = this;
	}
	SetControlRotation(BoxPlayCamera::Rotation());
	if (APawn* Possessed = GetPawn())
	{
		Possessed->SetActorRotation(BoxPlayCamera::Rotation());
		if (ASpectatorPawn* Spectator = Cast<ASpectatorPawn>(Possessed))
		{
			Spectator->SetActorLocation(BoxPlayCamera::Offset());
			Spectator->SetActorRotation(BoxPlayCamera::Rotation());
		}
	}
	SetViewTargetWithBlend(View, 0.f);
}

void ABoxPlayerController::ApplyMenuInput(UUserWidget* FocusWidget)
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (FocusWidget && FocusWidget->GetCachedWidget().IsValid())
	{
		Mode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	SetInputMode(Mode);
	FlushPressedKeys();
}

void ABoxPlayerController::ApplyMatchInput(UUserWidget* FocusWidget)
{
	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	if (FocusWidget)
	{
		Mode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	SetInputMode(Mode);
}
