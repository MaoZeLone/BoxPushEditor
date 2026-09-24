#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Game/BoxFlowTypes.h"
#include "BoxGameInstance.generated.h"

class UBoxSaveGame;
class UDataTable;
class ULevelData;

/** 局外流程：选关、存档、试玩请求。局内开局由 GameMode 消费这份请求。 */
UCLASS()
class BOXPUSH_API UBoxGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void PlayLevel(FName LevelId, bool bPlaytest = false);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void QueuePlaytest(ULevelData* Level);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void OpenSelect();

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void OpenMenu();

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	TArray<FBoxSelectEntry> GetSelectEntries() const;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	bool IsLevelUnlocked(FName LevelId) const;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	FBoxLevelRecord GetLevelRecord(FName LevelId) const;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void RecordClear(FName LevelId);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Flow")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Flow")
	FName GetNextListedLevelId(FName CurrentLevelId) const;

	ULevelData* ResolveLevel(const FBoxPlayRequest& Request) const;
	FBoxPlayRequest ConsumePlayRequest();
	EBoxMenuScreen ConsumeMenuScreen();
	const FBoxPlayRequest& PeekPlayRequest() const { return PendingPlay; }
	UDataTable* GetCatalog() const;

	UPROPERTY(EditAnywhere, Category = "BoxPush")
	TSoftObjectPtr<UDataTable> LevelCatalog;

	UPROPERTY(EditAnywhere, Category = "BoxPush")
	TSoftObjectPtr<UWorld> SelectMap;

	UPROPERTY(EditAnywhere, Category = "BoxPush")
	TSoftObjectPtr<UWorld> PlayMap;

private:
	void LoadCatalog();
	void LoadOrCreateSave();
	void WriteSave() const;
	void ApplyMasterVolume() const;
	void OpenMenuMap();
	ULevelData* FindListedLevel(FName LevelId) const;

	UPROPERTY()
	TObjectPtr<UDataTable> Catalog;

	UPROPERTY()
	TObjectPtr<UBoxSaveGame> SaveData;

	FBoxPlayRequest PendingPlay;
	EBoxMenuScreen PendingMenuScreen = EBoxMenuScreen::Main;
};
