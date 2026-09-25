#include "Match/BoxMatchWorld.h"

#include "Actors/BoxInteractableActor.h"
#include "BoxPushTags.h"
#include "Character/BoxPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/LevelData.h"
#include "Data/TerrainDef.h"
#include "Data/VisualComps.h"
#include "Engine/Texture2D.h"
#include "Game/BoxGameMode.h"
#include "Game/BoxProjectSettings.h"
#include "Game/BoxPlayerController.h"
#include "Match/BoxGrid.h"
#include "Match/BoxGridSim.h"
#include "Match/BoxSokobanSheet.h"

DEFINE_LOG_CATEGORY_STATIC(LogBoxMatchWorld, Log, All);

ABoxMatchWorld::ABoxMatchWorld()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SpriteView = CreateDefaultSubobject<UCameraComponent>(TEXT("SpriteView"));
	SpriteView->SetupAttachment(Root);
	SpriteView->SetProjectionMode(ECameraProjectionMode::Orthographic);
	SpriteView->bUsePawnControlRotation = false;
	SpriteView->bConstrainAspectRatio = false;
}

bool ABoxMatchWorld::StartLevel(const ULevelData* Level)
{
	StepDuration = GetDefault<UBoxProjectSettings>()->StepDuration;
	ClearMoveQueue();
	if (!Board)
	{
		Board = NewObject<UBoxBoard>(this);
	}
	if (!Sim)
	{
		Sim = NewObject<UBoxGridSim>(this);
	}
	if (!Board->InitFromLevel(Level) || !Sim->BindAndStart(Board))
	{
		return false;
	}

	RebuildPresentation();
	ActivateSpriteCamera();
	NotifyStatus();
	return true;
}

void ABoxMatchWorld::ClearTileSprites()
{
	for (UStaticMeshComponent* Sprite : TileSprites)
	{
		if (Sprite)
		{
			Sprite->DestroyComponent();
		}
	}
	TileSprites.Reset();
}

void ABoxMatchWorld::BindPlayer(ABoxPlayerCharacter* InPlayer)
{
	const bool bNewPlayer = Player != InPlayer;
	Player = InPlayer;
	if (Player)
	{
		AddTickPrerequisiteActor(Player);
	}
	if (bNewPlayer && Player && Board)
	{
		SnapPlayer(Board->GetPlayerCell());
	}
	if (bNewPlayer)
	{
		ActivateSpriteCamera();
	}
}

void ABoxMatchWorld::ActivateSpriteCamera()
{
	ApplySpriteCamera();
	ViewLockRemain = 12;
}

void ABoxMatchWorld::ApplySpriteCamera()
{
	if (!SpriteView || !Board)
	{
		return;
	}

	const FVector ViewLocation = BoxPlayCamera::ViewLocation(Board->GetWidth(), Board->GetHeight());
	SpriteView->SetUsingAbsoluteLocation(true);
	SpriteView->SetUsingAbsoluteRotation(true);
	SpriteView->SetWorldLocation(ViewLocation);
	SpriteView->SetWorldRotation(BoxPlayCamera::Rotation());
	SpriteView->SetProjectionMode(ECameraProjectionMode::Perspective);
	SpriteView->SetFieldOfView(BoxPlayCamera::FieldOfView());
	SpriteView->bAutoCalculateOrthoPlanes = false;
	SpriteView->SetActive(true);
	SpriteView->bUsePawnControlRotation = false;
	SpriteView->bConstrainAspectRatio = false;

	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PC && GetWorld())
	{
		PC = GetWorld()->GetFirstPlayerController();
	}
	if (!PC)
	{
		return;
	}

	PC->bAutoManageActiveCameraTarget = false;
	if (ABoxPlayerController* BoxPC = Cast<ABoxPlayerController>(PC))
	{
		BoxPC->UnlockTopDownView();
	}
	else if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->ViewPitchMin = -90.f;
		PC->PlayerCameraManager->ViewPitchMax = 90.f;
	}
	PC->SetControlRotation(BoxPlayCamera::Rotation());
	PC->SetViewTargetWithBlend(this, 0.f);
}

FTransform ABoxMatchWorld::GetPlayerSpawnTransform() const
{
	const FIntPoint Cell = Board ? Board->GetPlayerCell() : FIntPoint::ZeroValue;
	return FTransform(BoxGrid::CellToWorld(Cell, 90.f));
}

UStaticMeshComponent* ABoxMatchWorld::SpawnTileSprite(const UVisualSpriteComp* Comp, FIntPoint Cell)
{
	const float Span = Comp && Comp->WorldSpan > 0.f ? Comp->WorldSpan : BoxSokoban::TileSpan();
	UStaticMeshComponent* Sprite = nullptr;
	if (Comp)
	{
		if (UTexture2D* Texture = BoxSokoban::ResolveSprite(
			Comp->Texture.LoadSynchronous(), Comp->SourceX, Comp->SourceY, Comp->SourceW, Comp->SourceH, true))
		{
			Sprite = BoxSokoban::SpawnSpriteTexture(this, GetRootComponent(), Texture, Span);
		}
	}
	if (!Sprite)
	{
		return nullptr;
	}
	if (Sprite)
	{
		const float Z = Comp ? Comp->RelativeLocation.Z : 0.f;
		Sprite->SetWorldLocation(BoxGrid::CellToWorld(Cell, Z));
		TileSprites.Add(Sprite);
	}
	return Sprite;
}

void ABoxMatchWorld::RebuildPresentation()
{
	ClearTileSprites();
	for (auto& Pair : Actors)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	Actors.Reset();

	if (!Board)
	{
		return;
	}
	const UTerrainDef* FloorDef = UTerrainDef::FindByCell(ETerrainCell::Floor);
	const UTerrainDef* WallDef = UTerrainDef::FindByCell(ETerrainCell::Wall);

	for (int32 Y = 0; Y < Board->GetHeight(); ++Y)
	{
		for (int32 X = 0; X < Board->GetWidth(); ++X)
		{
			const FIntPoint Cell(X, Y);
			const ETerrainCell Terrain = Board->GetTerrain(Cell);
			if (Terrain == ETerrainCell::Wall)
			{
				SpawnTileSprite(WallDef ? WallDef->Sprite : nullptr, Cell);
			}
			else if (Terrain == ETerrainCell::Floor)
			{
				SpawnTileSprite(FloorDef ? FloorDef->Sprite : nullptr, Cell);
			}
		}
	}

	UWorld* World = GetWorld();
	for (const FBoxRuntimeInstance& Inst : Board->GetInstances())
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		ABoxInteractableActor* Actor = World->SpawnActor<ABoxInteractableActor>(
			ABoxInteractableActor::StaticClass(), BoxGrid::CellToWorld(Inst.Cell), FRotator::ZeroRotator, Params);
		if (Actor)
		{
			Actor->SetupFromInstance(Inst);
			Actors.Add(Inst.InstanceId, Actor);
		}
	}

	if (Player)
	{
		SnapPlayer(Board->GetPlayerCell());
	}
}

void ABoxMatchWorld::SnapPlayer(FIntPoint Cell)
{
	if (Player)
	{
		Player->SetActorLocation(BoxGrid::CellToWorld(Cell, 90.f));
	}
}

void ABoxMatchWorld::FacePlayer(FIntPoint From, FIntPoint To)
{
	if (!Player)
	{
		return;
	}
	const FIntPoint Dir = To - From;
	if (Dir == FIntPoint::ZeroValue)
	{
		return;
	}
	Player->SetActorRotation(BoxFacing::ToRotator(BoxFacing::FromDir(Dir)));
	Player->SyncLocomotion();
}

void ABoxMatchWorld::ApplyDeltas(const TArray<FBoxInstanceDelta>& Moves, bool bInstant)
{
	for (const FBoxInstanceDelta& Delta : Moves)
	{
		if (ABoxInteractableActor* Actor = Actors.FindRef(Delta.InstanceId))
		{
			Actor->SetCell(Delta.To, bInstant);
			if (!Delta.NewState.IsNone())
			{
				Actor->SetState(Delta.NewState);
			}
		}
	}

	if (Board)
	{
		for (const FBoxRuntimeInstance& Inst : Board->GetInstances())
		{
			if (ABoxInteractableActor* Actor = Actors.FindRef(Inst.InstanceId))
			{
				Actor->SetState(Inst.CurrentState);
			}
		}
	}
}

void ABoxMatchWorld::ApplyVisualCues(const TArray<FVisualTransitionCue>& Cues)
{
	for (const FVisualTransitionCue& Cue : Cues)
	{
		if (ABoxInteractableActor* Actor = Actors.FindRef(Cue.InstanceId))
		{
			Actor->ApplyTransitionVisual(Cue);
		}
	}
}

void ABoxMatchWorld::SetPlayerTags(FGameplayTag StateTag, bool bLocked)
{
	if (!Player)
	{
		return;
	}
	Player->RemoveOwnedTag(TAG_State_Walking);
	Player->RemoveOwnedTag(TAG_State_Pushing);
	Player->RemoveOwnedTag(TAG_Status_StepLocked);
	if (StateTag.IsValid())
	{
		Player->AddOwnedTag(StateTag);
	}
	if (bLocked)
	{
		Player->AddOwnedTag(TAG_Status_StepLocked);
	}
}

void ABoxMatchWorld::BeginStep(const FBoxStepResult& Result, FGameplayTag StateTag)
{
	ActiveStep = Result;
	ActiveStateTag = StateTag;
	bStepping = true;
	bReturnPhase = false;
	LastHeldAttempt = FIntPoint::ZeroValue;
	StepElapsed = 0.f;
	SetPlayerTags(StateTag, true);
	FacePlayer(Result.PlayerFrom, Result.PlayerTo);
	if (Player)
	{
		Player->SetActorLocation(BoxGrid::CellToWorld(Result.PlayerFrom, 90.f));
	}
	ApplyDeltas(Result.Moves, false);
	ApplyVisualCues(Result.VisualTransitions);
}

void ABoxMatchWorld::FinishPrimaryPhase()
{
	TArray<FBoxInstanceDelta> Returns;
	TArray<FName> ReturnEvents;
	TArray<FVisualTransitionCue> ReturnVisuals;
	if (Sim)
	{
		Sim->FlushImmediateReturns(Returns, &ReturnEvents, &ReturnVisuals);
	}
	ActiveStep.FiredEvents.Append(ReturnEvents);
	ActiveStep.VisualTransitions.Append(ReturnVisuals);
	if (Returns.Num() > 0 || ReturnVisuals.Num() > 0)
	{
		bReturnPhase = Returns.Num() > 0;
		StepElapsed = 0.f;
		ApplyDeltas(Returns, false);
		ApplyVisualCues(ActiveStep.VisualTransitions);
		if (Returns.Num() > 0)
		{
			return;
		}
	}
	FinishStep();
}

void ABoxMatchWorld::FinishStep()
{
	bStepping = false;
	bReturnPhase = false;
	SetPlayerTags(FGameplayTag(), false);
	if (Player && Board)
	{
		SnapPlayer(Board->GetPlayerCell());
	}
	if (Board)
	{
		for (const FBoxRuntimeInstance& Inst : Board->GetInstances())
		{
			if (ABoxInteractableActor* Actor = Actors.FindRef(Inst.InstanceId))
			{
				Actor->SetCell(Inst.Cell, true);
				Actor->SetState(Inst.CurrentState);
			}
		}
	}
	ApplyVisualCues(ActiveStep.VisualTransitions);
	for (const FName EventId : ActiveStep.FiredEvents)
	{
		UE_LOG(LogBoxMatchWorld, Log, TEXT("State action event %s"), *EventId.ToString());
	}
	NotifyStatus();
	DrainMoveQueue();
}

void ABoxMatchWorld::NotifyStatus()
{
	if (ABoxGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ABoxGameMode>() : nullptr)
	{
		GameMode->NotifyMatchStatus();
	}
}

bool ABoxMatchWorld::RequestMove(ABoxPlayerCharacter* InPlayer, FIntPoint Dir)
{
	if (!Board || !Sim || !InPlayer || bStepping)
	{
		return false;
	}
	if (Sim->IsPaused() || Sim->IsWon())
	{
		return false;
	}
	if (!InPlayer->CanPerform(TAG_Ability_Move))
	{
		return false;
	}

	const bool bCanPush = InPlayer->CanPerform(TAG_Ability_Push);
	FBoxStepResult Result;
	if (!Sim->TryPlayerMove(Dir, bCanPush, Result))
	{
		return false;
	}

	BindPlayer(InPlayer);
	const FGameplayTag StateTag = Result.bPushed ? TAG_State_Pushing : TAG_State_Walking;
	BeginStep(Result, StateTag);
	return true;
}

void ABoxMatchWorld::SetHeldMoveDir(FIntPoint Dir)
{
	HeldMoveDir = Dir;
}

void ABoxMatchWorld::ArmHoldRepeat()
{
	bHoldRepeatArmed = true;
}

void ABoxMatchWorld::ClearMoveQueue()
{
	MoveQueue.Reset();
	bHoldRepeatArmed = false;
	HeldMoveDir = FIntPoint::ZeroValue;
	LastHeldAttempt = FIntPoint::ZeroValue;
}

void ABoxMatchWorld::TryContinueHeldMove()
{
	if (!bHoldRepeatArmed || HeldMoveDir == FIntPoint::ZeroValue || bStepping || MoveQueue.Num() > 0)
	{
		return;
	}
	if (!Player || !Sim || Sim->IsPaused() || Sim->IsWon())
	{
		return;
	}
	if (HeldMoveDir == LastHeldAttempt)
	{
		return;
	}
	LastHeldAttempt = HeldMoveDir;
	EnqueueMove(Player, HeldMoveDir);
}

void ABoxMatchWorld::DrainMoveQueue()
{
	while (!bStepping && MoveQueue.Num() > 0)
	{
		if (!Player || !Sim || Sim->IsPaused() || Sim->IsWon())
		{
			MoveQueue.Reset();
			return;
		}
		const FIntPoint Dir = MoveQueue[0];
		MoveQueue.RemoveAt(0);
		RequestMove(Player, Dir);
	}
}

bool ABoxMatchWorld::EnqueueMove(ABoxPlayerCharacter* InPlayer, FIntPoint Dir)
{
	if (!Board || !Sim || !InPlayer || Dir == FIntPoint::ZeroValue)
	{
		return false;
	}
	if (Sim->IsPaused() || Sim->IsWon())
	{
		return false;
	}

	BindPlayer(InPlayer);
	MoveQueue.Add(Dir);
	if (MoveQueue.Num() > MaxMoveQueue)
	{
		MoveQueue.RemoveAt(0);
	}
	if (!bStepping)
	{
		DrainMoveQueue();
	}
	return true;
}

bool ABoxMatchWorld::RequestUndo(ABoxPlayerCharacter* InPlayer)
{
	if (!Board || !Sim || !InPlayer || bStepping || Sim->IsPaused())
	{
		return false;
	}
	if (!InPlayer->CanPerform(TAG_Ability_Undo))
	{
		return false;
	}

	FBoxStepResult Result;
	if (!Sim->Undo(Result))
	{
		return false;
	}
	ClearMoveQueue();
	BindPlayer(InPlayer);
	SnapPlayer(Result.PlayerTo);
	FacePlayer(Result.PlayerFrom, Result.PlayerTo);
	ApplyDeltas(Result.Moves, true);
	NotifyStatus();
	return true;
}

bool ABoxMatchWorld::RequestRedo(ABoxPlayerCharacter* InPlayer)
{
	if (!Board || !Sim || !InPlayer || bStepping || Sim->IsPaused())
	{
		return false;
	}
	if (!InPlayer->CanPerform(TAG_Ability_Redo))
	{
		return false;
	}

	FBoxStepResult Result;
	if (!Sim->Redo(Result))
	{
		return false;
	}
	ClearMoveQueue();
	BindPlayer(InPlayer);
	SnapPlayer(Result.PlayerTo);
	FacePlayer(Result.PlayerFrom, Result.PlayerTo);
	ApplyDeltas(Result.Moves, true);
	NotifyStatus();
	return true;
}

bool ABoxMatchWorld::RequestRestart(ABoxPlayerCharacter* InPlayer)
{
	if (!Board || !Sim || bStepping)
	{
		return false;
	}
	ABoxPlayerCharacter* Target = InPlayer;
	if (!Target)
	{
		Target = Player.Get();
	}
	if (!Target)
	{
		return false;
	}
	if (!Target->CanPerform(TAG_Ability_Restart))
	{
		return false;
	}

	FBoxStepResult Result;
	Sim->Restart(Result);
	ClearMoveQueue();
	BindPlayer(Target);
	SnapPlayer(Board->GetPlayerCell());
	ApplyDeltas(Result.Moves, true);
	for (const FBoxRuntimeInstance& Inst : Board->GetInstances())
	{
		if (ABoxInteractableActor* Actor = Actors.FindRef(Inst.InstanceId))
		{
			Actor->SetCell(Inst.Cell, true);
			Actor->SetState(Inst.CurrentState);
		}
	}
	NotifyStatus();
	return true;
}

bool ABoxMatchWorld::RequestPause(ABoxPlayerCharacter* InPlayer)
{
	if (!Sim)
	{
		return false;
	}
	ABoxPlayerCharacter* Target = InPlayer;
	if (!Target)
	{
		Target = Player.Get();
	}
	if (!Target)
	{
		return false;
	}
	if (!Target->CanPerform(TAG_Ability_Pause))
	{
		return false;
	}
	Sim->SetPaused(!Sim->IsPaused());
	ClearMoveQueue();
	NotifyStatus();
	return true;
}

void ABoxMatchWorld::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bStepping && Player && Board && Sim)
	{
		StepElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(StepElapsed / StepDuration, 0.f, 1.f);
		const float Ease = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
		const FVector From = BoxGrid::CellToWorld(bReturnPhase ? Board->GetPlayerCell() : ActiveStep.PlayerFrom, 90.f);
		const FVector To = BoxGrid::CellToWorld(bReturnPhase ? Board->GetPlayerCell() : ActiveStep.PlayerTo, 90.f);
		Player->SetActorLocation(FMath::Lerp(From, To, Ease));

		if (Alpha >= 1.f)
		{
			if (!bReturnPhase)
			{
				FinishPrimaryPhase();
			}
			else
			{
				FinishStep();
			}
		}
	}
	TryContinueHeldMove();
	if (ViewLockRemain > 0)
	{
		--ViewLockRemain;
		ApplySpriteCamera();
	}
}
