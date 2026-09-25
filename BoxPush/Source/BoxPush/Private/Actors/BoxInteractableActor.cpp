#include "Actors/BoxInteractableActor.h"

#include "Data/InteractableDef.h"
#include "Data/VisualComps.h"
#include "Components/StaticMeshComponent.h"
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

		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
		Mesh->SetupAttachment(Parent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->RegisterComponent();
		USceneComponent* Node = Mesh;

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

const UVisualSpriteComp* ABoxInteractableActor::FindSpriteComp(FName CompId) const
{
	if (!Def)
	{
		return nullptr;
	}
	for (const TObjectPtr<UVisualSpriteComp>& Comp : Def->SpriteComps)
	{
		if (Comp && Comp->CompId == CompId)
		{
			return Comp;
		}
	}
	return nullptr;
}

UStaticMeshComponent* ABoxInteractableActor::FindSprite(FName CompId) const
{
	if (!Def)
	{
		return nullptr;
	}
	int32 NodeIndex = 0;
	for (const TObjectPtr<UVisualSpriteComp>& Comp : Def->SpriteComps)
	{
		if (!Comp || Comp->CompId.IsNone())
		{
			continue;
		}
		USceneComponent* Node = VisualNodes.IsValidIndex(NodeIndex) ? VisualNodes[NodeIndex].Get() : nullptr;
		++NodeIndex;
		if (Comp->CompId == CompId)
		{
			return Cast<UStaticMeshComponent>(Node);
		}
	}
	return nullptr;
}

void ABoxInteractableActor::ApplyTasks(const TArray<FVisualTask>& Tasks)
{
	for (const FVisualTask& Task : Tasks)
	{
		UStaticMeshComponent* Mesh = FindSprite(Task.CompId);
		const UVisualSpriteComp* Comp = FindSpriteComp(Task.CompId);
		if (!Mesh || !Comp)
		{
			continue;
		}
		if (Task.Type == EVisualTaskType::SetBlocking)
		{
			continue;
		}
		if (Task.Type == EVisualTaskType::SetVisible)
		{
			Mesh->SetHiddenInGame(!Task.bVisible);
			Mesh->SetVisibility(Task.bVisible, true);
			continue;
		}
		const float Span = Comp->WorldSpan > 0.f ? Comp->WorldSpan : BoxSokoban::TileSpan();
		UTexture2D* Texture = BoxSokoban::ResolveSprite(
			Task.Texture.LoadSynchronous(), Task.SourceX, Task.SourceY, Task.SourceW, Task.SourceH, true);
		if (!Texture)
		{
			Mesh->SetHiddenInGame(true);
			Mesh->SetVisibility(false, true);
			continue;
		}
		Mesh->SetHiddenInGame(false);
		Mesh->SetVisibility(true, true);
		BoxSokoban::ApplySpriteTexture(Mesh, Texture, Span);
	}
}

void ABoxInteractableActor::RestoreDefaultLook()
{
	if (!Def)
	{
		return;
	}
	int32 NodeIndex = 0;
	for (const TObjectPtr<UVisualSpriteComp>& Comp : Def->SpriteComps)
	{
		if (!Comp || Comp->CompId.IsNone())
		{
			continue;
		}
		UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(VisualNodes.IsValidIndex(NodeIndex) ? VisualNodes[NodeIndex].Get() : nullptr);
		++NodeIndex;
		if (!Mesh)
		{
			continue;
		}
		const bool bVisible = Comp->VisibleInStates.Num() == 0 || Comp->VisibleInStates.Contains(CurrentState);
		const float Span = Comp->WorldSpan > 0.f ? Comp->WorldSpan : BoxSokoban::TileSpan();
		UTexture2D* Texture = BoxSokoban::ResolveSprite(
			Comp->Texture.LoadSynchronous(), Comp->SourceX, Comp->SourceY, Comp->SourceW, Comp->SourceH, true);
		if (!bVisible || !Texture)
		{
			Mesh->SetHiddenInGame(true);
			Mesh->SetVisibility(false, true);
			continue;
		}
		Mesh->SetHiddenInGame(false);
		Mesh->SetVisibility(true, true);
		BoxSokoban::ApplySpriteTexture(Mesh, Texture, Span);
	}
}

void ABoxInteractableActor::SetState(FName StateId)
{
	CurrentState = StateId;
	RestoreDefaultLook();
	if (!Def)
	{
		return;
	}
	for (const FStateVisual& Row : Def->StateVisuals)
	{
		if (Row.StateId == StateId)
		{
			ApplyTasks(Row.Tasks);
			break;
		}
	}
}

void ABoxInteractableActor::ApplyTransitionVisual(const FVisualTransitionCue& Cue)
{
	if (!Def || Cue.InstanceId != InstanceId)
	{
		return;
	}
	for (const FTransitionVisual& Row : Def->TransitionVisuals)
	{
		if (Row.Condition != Cue.Condition)
		{
			continue;
		}
		if (!Row.FromState.IsNone() && Row.FromState != Cue.FromState)
		{
			continue;
		}
		if (!Row.ToState.IsNone() && Row.ToState != Cue.ToState)
		{
			continue;
		}
		if (Row.Condition == EBoxTransitionCondition::OnEvent && Row.EventId != Cue.EventId)
		{
			continue;
		}
		ApplyTasks(Row.Tasks);
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
