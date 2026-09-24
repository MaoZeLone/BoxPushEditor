#include "Game/BoxMatchHUD.h"

#include "Game/BoxPlayerController.h"
#include "UI/BoxMatchHudWidget.h"

ABoxMatchHUD::ABoxMatchHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABoxMatchHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	MatchWidget = CreateWidget<UBoxMatchHudWidget>(PC);
	if (!MatchWidget)
	{
		return;
	}
	MatchWidget->AddToViewport(10);
	MatchWidget->Refresh();
	if (ABoxPlayerController* BoxPC = Cast<ABoxPlayerController>(PC))
	{
		BoxPC->ApplyMatchInput(MatchWidget);
	}
}

void ABoxMatchHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MatchWidget)
	{
		MatchWidget->RemoveFromParent();
		MatchWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ABoxMatchHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (MatchWidget)
	{
		MatchWidget->Refresh();
	}
}
