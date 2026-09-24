#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BoxMatchHUD.generated.h"

class UBoxMatchHudWidget;

UCLASS()
class BOXPUSH_API ABoxMatchHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABoxMatchHUD();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UBoxMatchHudWidget> MatchWidget;
};
