#include "Actors/BoxInteractableActor.h"

#include "Data/InteractableDef.h"
#include "Data/VisualComps.h"
#include "Engine/Texture2D.h"
#include "Match/BoxGrid.h"
#include "Match/BoxSokobanSheet.h"

ABoxInteractableActor::ABoxInteractableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ABoxInteractableActor::SetupFromInstance(const FBoxRuntimeInstance& Inst)
{
	InstanceId = Inst.InstanceId;
	Def = Inst.Def;
	CurrentState = Inst.CurrentState;
	RebuildSprites();
	SetActorRotation(BoxFacing::ToRotator(Inst.YawSteps));
	SetCell(Inst.Cell, true);
	SetState(CurrentState);
}

void ABoxInteractableActor::ClearVisuals()
{
	TArray<USceneComponent*> Attached;
	if (SceneRoot)
	{
		SceneRoot->GetChildrenComponents(true, Attached);
	}
	for (USceneComponent* Child : Attached)
	{
		if (Child)
		{
			Child->DestroyComponent();
		}
	}
	VisualNodes.Reset();
}

void ABoxInteractableActor::RebuildSprites()
{
	ClearVisuals();
	if (!SceneRoot || !Def)
	{
		return;
	}

	TMap<FName, USceneComponent*> Nodes;
	Nodes.Add(NAME_None, SceneRoot);

	for (const TObjectPtr<UVisualSpriteComp>& Comp : Def->SpriteComps)
	{
		if (!Comp || Comp->CompId.IsNone())
		{
			continue;
		}

		USceneComponent* Parent = Nodes.FindRef(Comp->ParentId);
		if (!Parent)
		{
			Parent = SceneRoot;
		}

		USceneComponent* Node = nullptr;
		const float Span = Comp->WorldSpan > 0.f ? Comp->WorldSpan : BoxSokoban::TileSpan();
		if (UTexture2D* Texture = BoxSokoban::ResolveSprite(
			Comp->Texture.LoadSynchronous(), Comp->SourceX, Comp->SourceY, Comp->SourceW, Comp->SourceH, true))
		{
			Node = BoxSokoban::SpawnSpriteTexture(this, Parent, Texture, Span);
		}
		if (!Node)
		{
			Node = NewObject<USceneComponent>(this);
			Node->SetupAttachment(Parent);
			Node->RegisterComponent();
		}

		Node->SetRelativeLocation(Comp->RelativeLocation);
		Node->SetRelativeRotation(Comp->RelativeRotation);
		Nodes.Add(Comp->CompId, Node);
		VisualNodes.Add(Node);
	}
}

void ABoxInteractableActor::SetCell(FIntPoint Cell, bool bInstant)
{
	const FVector Target = BoxGrid::CellToWorld(Cell);
	if (bInstant)
	{
		FromWorld = Target;
		ToWorld = Target;
		InterpAlpha = 1.f;
		SetActorLocation(Target);
		return;
	}

	FromWorld = GetActorLocation();
	ToWorld = Target;
	InterpAlpha = 0.f;
}

void ABoxInteractableActor::SetState(FName StateId)
{
	CurrentState = StateId;
	RefreshVisibility();
}

void ABoxInteractableActor::RefreshVisibility()
{
	if (!Def)
	{
		return;
	}
	for (int32 Index = 0; Index < Def->SpriteComps.Num() && Index < VisualNodes.Num(); ++Index)
	{
		const UVisualSpriteComp* Comp = Def->SpriteComps[Index];
		USceneComponent* Node = VisualNodes[Index];
		if (!Comp || !Node)
		{
			continue;
		}
		const bool bVisible = Comp->VisibleInStates.Num() == 0 || Comp->VisibleInStates.Contains(CurrentState);
		Node->SetVisibility(bVisible, true);
	}
}

void ABoxInteractableActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (InterpAlpha >= 1.f)
	{
		return;
	}
	InterpAlpha = FMath::Clamp(InterpAlpha + DeltaSeconds / InterpDuration, 0.f, 1.f);
	const float Alpha = FMath::InterpEaseInOut(0.f, 1.f, InterpAlpha, 2.f);
	SetActorLocation(FMath::Lerp(FromWorld, ToWorld, Alpha));
}
