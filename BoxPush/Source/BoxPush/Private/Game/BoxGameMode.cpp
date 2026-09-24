#include "Game/BoxGameMode.h"

#include "Character/BoxPlayerCharacter.h"
#include "Data/BoxAssetPaths.h"
#include "Data/LevelData.h"
#include "Data/PlayerDef.h"
#include "Game/BoxGameInstance.h"
#include "Game/BoxMatchHUD.h"
#include "Game/BoxPlayerController.h"
#include "Match/BoxBoard.h"
#include "Match/BoxGridSim.h"
#include "Match/BoxMatchWorld.h"

ABoxGameMode::ABoxGameMode()
{
	DefaultPawnClass = ABoxPlayerCharacter::StaticClass();
	PlayerControllerClass = ABoxPlayerController::StaticClass();
	HUDClass = ABoxMatchHUD::StaticClass();
	DefaultPlayerDef = TSoftObjectPtr<UPlayerDef>(FSoftObjectPath(BoxAssetPaths::PlayerDef()));
	StartupLevel = TSoftObjectPtr<ULevelData>(FSoftObjectPath(BoxAssetPaths::StartupLevel()));
}

void ABoxGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	PlayerDef = UPlayerDef::LoadOfficial();
	ResolveStartingLevel();
}

void ABoxGameMode::ResolveStartingLevel()
{
	CurrentLevel = nullptr;
	bPlaytest = false;
	bWinSettled = false;

	if (UBoxGameInstance* GI = GetGameInstance<UBoxGameInstance>())
	{
		const FBoxPlayRequest Request = GI->ConsumePlayRequest();
		if (Request.IsSet())
		{
			CurrentLevel = GI->ResolveLevel(Request);
			bPlaytest = Request.bPlaytest;
		}
	}

	if (!CurrentLevel && bPlaytest)
	{
		CurrentLevel = StartupLevel.LoadSynchronous();
		if (!CurrentLevel)
		{
			CurrentLevel = LoadObject<ULevelData>(nullptr, *BoxAssetPaths::StartupLevel());
		}
	}
}

void ABoxGameMode::StartPlay()
{
	if (!CurrentLevel)
	{
		Super::StartPlay();
		if (UBoxGameInstance* GI = GetGameInstance<UBoxGameInstance>())
		{
			GI->OpenMenu();
		}
		return;
	}

	SetPhase(EBoxFlowPhase::Playing);
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.Name = TEXT("BoxMatchWorld");
		MatchWorld = World->SpawnActor<ABoxMatchWorld>(ABoxMatchWorld::StaticClass(), FTransform::Identity, Params);
		if (MatchWorld)
		{
			MatchWorld->StartLevel(CurrentLevel);
		}
	}
	Super::StartPlay();
	if (MatchWorld)
	{
		MatchWorld->ActivateSpriteCamera();
	}
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			RestartPlayer(It->Get());
		}
	}
	if (MatchWorld)
	{
		MatchWorld->ActivateSpriteCamera();
	}
}

UClass* ABoxGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (PlayerDef && PlayerDef->PawnClass)
	{
		return PlayerDef->PawnClass;
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ABoxGameMode::RestartPlayer(AController* NewPlayer)
{
	if (MatchWorld && NewPlayer)
	{
		const FTransform Spawn = MatchWorld->GetPlayerSpawnTransform();
		if (APawn* Existing = NewPlayer->GetPawn())
		{
			Existing->SetActorTransform(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			RestartPlayerAtTransform(NewPlayer, Spawn);
		}
		if (ABoxPlayerCharacter* Pawn = Cast<ABoxPlayerCharacter>(NewPlayer->GetPawn()))
		{
			Pawn->ApplyPlayerDef(PlayerDef);
			MatchWorld->BindPlayer(Pawn);
		}
		if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
		{
			PC->bAutoManageActiveCameraTarget = false;
		}
		MatchWorld->ActivateSpriteCamera();
		return;
	}
	Super::RestartPlayer(NewPlayer);
}

UBoxBoard* ABoxGameMode::GetBoard() const
{
	return MatchWorld ? MatchWorld->GetBoard() : nullptr;
}

UBoxGridSim* ABoxGameMode::GetSim() const
{
	return MatchWorld ? MatchWorld->GetSim() : nullptr;
}

void ABoxGameMode::SetPhase(EBoxFlowPhase NewPhase)
{
	if (Phase == NewPhase)
	{
		return;
	}
	Phase = NewPhase;
	OnFlowPhaseChanged.Broadcast(Phase);
}

bool ABoxGameMode::RequestMove(ABoxPlayerCharacter* Player, FIntPoint Dir)
{
	if (!MatchWorld || Phase == EBoxFlowPhase::Won || Phase == EBoxFlowPhase::Paused)
	{
		return false;
	}
	return MatchWorld->RequestMove(Player, Dir);
}

bool ABoxGameMode::EnqueueMove(ABoxPlayerCharacter* Player, FIntPoint Dir)
{
	if (!MatchWorld || Phase == EBoxFlowPhase::Won || Phase == EBoxFlowPhase::Paused)
	{
		return false;
	}
	return MatchWorld->EnqueueMove(Player, Dir);
}

bool ABoxGameMode::RequestUndo(ABoxPlayerCharacter* Player)
{
	if (!MatchWorld || Phase == EBoxFlowPhase::Paused)
	{
		return false;
	}
	const bool bOk = MatchWorld->RequestUndo(Player);
	if (bOk)
	{
		bWinSettled = false;
		if (const UBoxGridSim* GridSim = GetSim())
		{
			SetPhase(GridSim->IsWon() ? EBoxFlowPhase::Won : EBoxFlowPhase::Playing);
		}
	}
	return bOk;
}

bool ABoxGameMode::RequestRedo(ABoxPlayerCharacter* Player)
{
	if (!MatchWorld || Phase == EBoxFlowPhase::Paused)
	{
		return false;
	}
	const bool bOk = MatchWorld->RequestRedo(Player);
	if (bOk)
	{
		bWinSettled = false;
		if (const UBoxGridSim* GridSim = GetSim())
		{
			SetPhase(GridSim->IsWon() ? EBoxFlowPhase::Won : EBoxFlowPhase::Playing);
		}
	}
	return bOk;
}

bool ABoxGameMode::RequestRestart(ABoxPlayerCharacter* Player)
{
	if (!MatchWorld)
	{
		return false;
	}
	const bool bOk = MatchWorld->RequestRestart(Player);
	if (bOk)
	{
		bWinSettled = false;
		SetPhase(EBoxFlowPhase::Playing);
	}
	return bOk;
}

bool ABoxGameMode::RequestPause(ABoxPlayerCharacter* Player)
{
	if (!MatchWorld || Phase == EBoxFlowPhase::Won)
	{
		return false;
	}
	if (!MatchWorld->RequestPause(Player))
	{
		return false;
	}
	const UBoxGridSim* GridSim = GetSim();
	SetPhase(GridSim && GridSim->IsPaused() ? EBoxFlowPhase::Paused : EBoxFlowPhase::Playing);
	return true;
}

void ABoxGameMode::ReturnToSelect()
{
	if (UBoxGameInstance* GI = GetGameInstance<UBoxGameInstance>())
	{
		GI->OpenSelect();
	}
}

void ABoxGameMode::PlayNextLevel()
{
	UBoxGameInstance* GI = GetGameInstance<UBoxGameInstance>();
	const UBoxBoard* Board = GetBoard();
	if (!GI || !Board)
	{
		return;
	}

	const FName NextId = GI->GetNextListedLevelId(Board->GetLevelId());
	if (NextId.IsNone())
	{
		ReturnToSelect();
		return;
	}

	GI->PlayLevel(NextId, bPlaytest);
	if (!GI->PlayMap.IsNull())
	{
		return;
	}

	ResolveStartingLevel();
	if (MatchWorld)
	{
		MatchWorld->StartLevel(CurrentLevel);
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			RestartPlayer(PC);
		}
	}
	SetPhase(EBoxFlowPhase::Playing);
}

void ABoxGameMode::SettleWin()
{
	UBoxBoard* Board = GetBoard();
	UBoxGridSim* GridSim = GetSim();
	if (!Board || !GridSim || !GridSim->IsWon() || bWinSettled)
	{
		return;
	}

	bWinSettled = true;
	SetPhase(EBoxFlowPhase::Won);

	if (!bPlaytest)
	{
		if (UBoxGameInstance* GI = GetGameInstance<UBoxGameInstance>())
		{
			GI->RecordClear(Board->GetLevelId());
		}
	}

	OnMatchWon.Broadcast(Board->GetLevelId());
}

void ABoxGameMode::NotifyMatchStatus()
{
	const UBoxGridSim* GridSim = GetSim();
	if (!GridSim)
	{
		return;
	}
	if (GridSim->IsWon())
	{
		SettleWin();
	}
}

ABoxPlayerCharacter* ABoxGameMode::FindMatchPlayer() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			return Cast<ABoxPlayerCharacter>(PC->GetPawn());
		}
	}
	return nullptr;
}

void ABoxGameMode::PauseMatch()
{
	if (Phase == EBoxFlowPhase::Playing)
	{
		RequestPause(FindMatchPlayer());
	}
}

void ABoxGameMode::ResumeMatch()
{
	if (Phase == EBoxFlowPhase::Paused)
	{
		RequestPause(FindMatchPlayer());
	}
}

void ABoxGameMode::RestartMatch()
{
	RequestRestart(FindMatchPlayer());
}

FText ABoxGameMode::GetLevelDisplayName() const
{
	if (CurrentLevel)
	{
		if (!CurrentLevel->DisplayName.IsEmpty())
		{
			return CurrentLevel->DisplayName;
		}
		return FText::FromName(CurrentLevel->LevelId);
	}
	return FText::GetEmpty();
}

bool ABoxGameMode::HasNextListedLevel() const
{
	const UBoxGameInstance* GI = GetGameInstance<UBoxGameInstance>();
	if (!GI || !CurrentLevel)
	{
		return false;
	}
	return !GI->GetNextListedLevelId(CurrentLevel->LevelId).IsNone();
}
