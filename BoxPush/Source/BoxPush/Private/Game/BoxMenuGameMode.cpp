#include "Game/BoxMenuGameMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Game/BoxPlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Match/BoxGrid.h"
#include "TimerManager.h"
#include "UI/BoxMenuWidget.h"

ABoxMenuGameMode::ABoxMenuGameMode()
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	PlayerControllerClass = ABoxPlayerController::StaticClass();
	HUDClass = ABoxMenuHUD::StaticClass();
}

void ABoxMenuGameMode::StartPlay()
{
	Super::StartPlay();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACameraActor* Cam = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(),
		BoxPlayCamera::Offset,
		BoxPlayCamera::Rotation,
		Params);
	if (Cam)
	{
		if (UCameraComponent* View = Cam->GetCameraComponent())
		{
			View->SetProjectionMode(ECameraProjectionMode::Orthographic);
			View->SetOrthoWidth(2400.f);
			View->bConstrainAspectRatio = false;
			View->bUsePawnControlRotation = false;
		}
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		PC->bAutoManageActiveCameraTarget = false;
		if (ABoxPlayerController* BoxPC = Cast<ABoxPlayerController>(PC))
		{
			BoxPC->UnlockTopDownView();
		}
		PC->SetControlRotation(BoxPlayCamera::Rotation);
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->DisableInput(PC);
			Pawn->SetActorLocationAndRotation(BoxPlayCamera::Offset, BoxPlayCamera::Rotation);
		}
		if (Cam)
		{
			PC->SetViewTargetWithBlend(Cam, 0.f);
		}
		if (ABoxPlayerController* BoxPC = Cast<ABoxPlayerController>(PC))
		{
			BoxPC->LockBoardView();
		}
	}
}

void ABoxMenuHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	MenuWidget = CreateWidget<UBoxMenuWidget>(PC);
	if (!MenuWidget)
	{
		return;
	}
	MenuWidget->AddToViewport(10);
	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->SetInputMode(FInputModeUIOnly());
	TWeakObjectPtr<ABoxMenuHUD> WeakThis(this);
	PC->GetWorldTimerManager().SetTimerForNextTick([WeakThis]()
	{
		ABoxMenuHUD* HUD = WeakThis.Get();
		if (!HUD || !HUD->MenuWidget)
		{
			return;
		}
		APlayerController* OwnerPC = HUD->GetOwningPlayerController();
		if (!OwnerPC)
		{
			return;
		}
		if (ABoxPlayerController* BoxPC = Cast<ABoxPlayerController>(OwnerPC))
		{
			BoxPC->ApplyMenuInput(HUD->MenuWidget);
			HUD->MenuWidget->SetKeyboardFocus();
		}
		else
		{
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(HUD->MenuWidget->TakeWidget());
			OwnerPC->SetInputMode(Mode);
			OwnerPC->bShowMouseCursor = true;
		}
	});
}

void ABoxMenuHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
