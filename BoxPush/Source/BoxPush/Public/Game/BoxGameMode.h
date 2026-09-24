#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/BoxFlowTypes.h"
#include "BoxGameMode.generated.h"

class ABoxMatchWorld;
class ABoxPlayerCharacter;
class UBoxBoard;
class UBoxGridSim;
class ULevelData;
class UPlayerDef;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBoxFlowPhaseChanged, EBoxFlowPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBoxMatchWon, FName, LevelId);

/** 局内流程：开局、撤销、重开、暂停、胜利。选关/存档走 GameInstance。 */
UCLASS()
class BOXPUSH_API ABoxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABoxGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	UFUNCTION(BlueprintPure, Category = "BoxPush")
	UPlayerDef* GetPlayerDef() const { return PlayerDef; }

	UFUNCTION(BlueprintPure, Category = "BoxPush")
	ABoxMatchWorld* GetMatchWorld() const { return MatchWorld; }

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	ULevelData* GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	UBoxBoard* GetBoard() const;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	UBoxGridSim* GetSim() const;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	bool IsPlaytest() const { return bPlaytest; }

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	EBoxFlowPhase GetFlowPhase() const { return Phase; }

	bool RequestMove(ABoxPlayerCharacter* Player, FIntPoint Dir);
	bool EnqueueMove(ABoxPlayerCharacter* Player, FIntPoint Dir);
	bool RequestUndo(ABoxPlayerCharacter* Player);
	bool RequestRedo(ABoxPlayerCharacter* Player);
	bool RequestRestart(ABoxPlayerCharacter* Player);
	bool RequestPause(ABoxPlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void ReturnToSelect();

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void PlayNextLevel();

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void PauseMatch();

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void ResumeMatch();

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void RestartMatch();

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	FText GetLevelDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	bool HasNextListedLevel() const;

	void NotifyMatchStatus();

	UPROPERTY(BlueprintAssignable, Category = "BoxPush|Flow")
	FBoxFlowPhaseChanged OnFlowPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "BoxPush|Flow")
	FBoxMatchWon OnMatchWon;

	UPROPERTY(EditAnywhere, Category = "BoxPush")
	TSoftObjectPtr<UPlayerDef> DefaultPlayerDef;

	UPROPERTY(EditAnywhere, Category = "BoxPush")
	TSoftObjectPtr<ULevelData> StartupLevel;

private:
	void ResolveStartingLevel();
	void SetPhase(EBoxFlowPhase NewPhase);
	void SettleWin();
	ABoxPlayerCharacter* FindMatchPlayer() const;

	UPROPERTY()
	TObjectPtr<UPlayerDef> PlayerDef;

	UPROPERTY()
	TObjectPtr<ULevelData> CurrentLevel;

	UPROPERTY()
	TObjectPtr<ABoxMatchWorld> MatchWorld;

	bool bPlaytest = false;
	bool bWinSettled = false;
	EBoxFlowPhase Phase = EBoxFlowPhase::None;
};
