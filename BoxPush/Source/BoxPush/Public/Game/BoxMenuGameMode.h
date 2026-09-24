#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "BoxMenuGameMode.generated.h"

class UBoxMenuWidget;

UCLASS()
class BOXPUSH_API ABoxMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<UBoxMenuWidget> MenuWidget;
};

/** 局外：主菜单 / 选关 / 设置。不对局、不刷棋盘。 */
UCLASS()
class BOXPUSH_API ABoxMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABoxMenuGameMode();
	virtual void StartPlay() override;
};
