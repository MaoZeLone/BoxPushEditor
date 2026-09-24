#include "Character/BoxPlayerCharacter.h"

#include "BoxPushTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/BoxActionSet.h"
#include "Data/PlayerDef.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/BoxGameMode.h"
#include "Match/BoxMatchWorld.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/BoxInputConfig.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Match/BoxGrid.h"
#include "Match/BoxSokobanSheet.h"

ABoxPlayerCharacter::ABoxPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
	GetCapsuleComponent()->SetCapsuleRadius(40.f);

	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Skel->SetVisibility(false);
		Skel->SetHiddenInGame(true);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GravityScale = 0.f;
		Movement->DefaultLandMovementMode = MOVE_None;
		Movement->SetMovementMode(MOVE_None);
		Movement->bOrientRotationToMovement = false;
		Movement->DisableMovement();
	}

	SpritePlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpritePlane"));
	SpritePlane->SetupAttachment(GetRootComponent());
	SpritePlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpritePlane->SetCastShadow(false);
	SpritePlane->SetMobility(EComponentMobility::Movable);
	SpritePlane->SetUsingAbsoluteRotation(true);
}

void ABoxPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GravityScale = 0.f;
		Movement->DisableMovement();
	}
	BindMappingContext();
	RefreshLocomotion();
}

void ABoxPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	RefreshLocomotion();
}

void ABoxPlayerCharacter::ApplyPlayerDef(const UPlayerDef* InPlayerDef)
{
	PlayerDef = InPlayerDef;
	OwnedTags.Reset();
	BindMappingContext();
	RefreshLocomotion();
}

bool ABoxPlayerCharacter::CanPerform(FGameplayTag AbilityTag) const
{
	return PlayerDef && PlayerDef->CanPerform(AbilityTag, OwnedTags);
}

void ABoxPlayerCharacter::AddOwnedTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OwnedTags.AddTag(Tag);
		RefreshLocomotion();
	}
}

void ABoxPlayerCharacter::RemoveOwnedTag(FGameplayTag Tag)
{
	OwnedTags.RemoveTag(Tag);
	RefreshLocomotion();
}

void ABoxPlayerCharacter::BindMappingContext()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PlayerDef || !PlayerDef->InputConfig || !PlayerDef->InputConfig->DefaultMappingContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		UInputMappingContext* IMC = PlayerDef->InputConfig->DefaultMappingContext;
		Subsystem->RemoveMappingContext(IMC);
		Subsystem->AddMappingContext(IMC, 0);
	}
}

void ABoxPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!PlayerDef)
	{
		if (const ABoxGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ABoxGameMode>() : nullptr)
		{
			PlayerDef = GameMode->GetPlayerDef();
		}
	}
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC || !PlayerDef || !PlayerDef->InputConfig)
	{
		return;
	}

	const UBoxInputConfig* Config = PlayerDef->InputConfig;
	for (const FBoxInputAction& Action : Config->AbilityInputActions)
	{
		if (!Action.InputAction || !Action.InputTag.IsValid())
		{
			continue;
		}
		EIC->BindAction(Action.InputAction, ETriggerEvent::Triggered, this, &ABoxPlayerCharacter::OnAbilityInputPressed, Action.InputTag);
		EIC->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &ABoxPlayerCharacter::OnAbilityInputReleased, Action.InputTag);
	}
	if (!Config->FindAbilityInputActionForTag(TAG_Input_Redo, false))
	{
		PlayerInputComponent->BindKey(EKeys::Y, IE_Pressed, this, &ABoxPlayerCharacter::OnRedoFallback);
	}
}

ABoxGameMode* ABoxPlayerCharacter::FindGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ABoxGameMode>() : nullptr;
}

void ABoxPlayerCharacter::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (!InputTag.IsValid() || HeldAbilityInputTags.HasTagExact(InputTag))
	{
		return;
	}
	HeldAbilityInputTags.AddTag(InputTag);

	if (!PlayerDef || !PlayerDef->ActionSet)
	{
		return;
	}
	const FBoxGrantedAction* Granted = PlayerDef->ActionSet->FindByInputTag(InputTag);
	if (!Granted)
	{
		return;
	}
	PerformGrantedAbility(Granted->AbilityTag);
}

void ABoxPlayerCharacter::OnAbilityInputReleased(FGameplayTag InputTag)
{
	HeldAbilityInputTags.RemoveTag(InputTag);
}

void ABoxPlayerCharacter::PerformGrantedAbility(FGameplayTag AbilityTag)
{
	ABoxGameMode* GameMode = FindGameMode();
	if (!GameMode)
	{
		return;
	}
	if (AbilityTag.MatchesTagExact(TAG_Ability_Undo))
	{
		GameMode->RequestUndo(this);
	}
	else if (AbilityTag.MatchesTagExact(TAG_Ability_Redo))
	{
		GameMode->RequestRedo(this);
	}
	else if (AbilityTag.MatchesTagExact(TAG_Ability_Restart))
	{
		GameMode->RequestRestart(this);
	}
	else if (AbilityTag.MatchesTagExact(TAG_Ability_Pause))
	{
		GameMode->RequestPause(this);
	}
}

void ABoxPlayerCharacter::OnRedoFallback()
{
	PerformGrantedAbility(TAG_Ability_Redo);
}

void ABoxPlayerCharacter::SyncLocomotion()
{
	RefreshLocomotion();
}

void ABoxPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bMoving = OwnedTags.HasTag(TAG_State_Walking) || OwnedTags.HasTag(TAG_State_Pushing);
	if (bMoving)
	{
		const UPlayerDef* Look = PlayerDef ? PlayerDef.Get() : UPlayerDef::LoadOfficial();
		const float Interval = Look ? Look->GetFrameInterval() : 0.16f;
		SpriteAnimTime += DeltaSeconds;
		const int32 NextFrame = FMath::Max(0, static_cast<int32>(SpriteAnimTime / Interval));
		if (NextFrame != SpriteFrame)
		{
			SpriteFrame = NextFrame;
			RefreshLocomotion();
		}
	}
	else if (SpriteFrame != 0 || SpriteAnimTime != 0.f)
	{
		SpriteFrame = 0;
		SpriteAnimTime = 0.f;
		RefreshLocomotion();
	}
	PollMoveKeys();
}

void ABoxPlayerCharacter::PollMoveKeys()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	ABoxGameMode* GameMode = FindGameMode();
	ABoxMatchWorld* Match = GameMode ? GameMode->GetMatchWorld() : nullptr;
	if (!PC || !Match)
	{
		PrevMoveKeys = 0;
		return;
	}

	struct FMoveKey
	{
		FKey Key;
		FIntPoint Dir;
	};
	const FMoveKey Keys[] = {
		{EKeys::W, BoxGrid::North},
		{EKeys::Up, BoxGrid::North},
		{EKeys::S, BoxGrid::South},
		{EKeys::Down, BoxGrid::South},
		{EKeys::D, BoxGrid::West},
		{EKeys::Right, BoxGrid::West},
		{EKeys::A, BoxGrid::East},
		{EKeys::Left, BoxGrid::East},
	};

	uint8 Down = 0;
	FIntPoint Dir = FIntPoint::ZeroValue;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Keys); ++Index)
	{
		if (!PC->IsInputKeyDown(Keys[Index].Key))
		{
			continue;
		}
		Down |= static_cast<uint8>(1 << Index);
		if (Dir == FIntPoint::ZeroValue)
		{
			Dir = Keys[Index].Dir;
		}
	}
	Match->SetHeldMoveDir(Dir);
	if (PrevMoveKeys == 0 && Down != 0)
	{
		Match->ArmHoldRepeat();
		GameMode->EnqueueMove(this, Dir);
	}
	PrevMoveKeys = Down;
}

void ABoxPlayerCharacter::RefreshLocomotion()
{
	if (!SpritePlane)
	{
		return;
	}
	const UPlayerDef* Look = PlayerDef ? PlayerDef.Get() : UPlayerDef::LoadOfficial();
	const bool bPushing = OwnedTags.HasTag(TAG_State_Pushing);
	const bool bWalking = OwnedTags.HasTag(TAG_State_Walking);
	const int32 Facing = BoxFacing::Normalize(FMath::RoundToInt((GetActorRotation().Yaw - 90.f) / 90.f));
	const FBoxAtlasFrame Frame = Look
		? Look->PickFrame(Facing, bPushing, bWalking, SpriteFrame)
		: FBoxAtlasFrame();
	BoxSokoban::ApplyAtlasFrame(SpritePlane, Frame, BoxSokoban::TileSpan());
}
