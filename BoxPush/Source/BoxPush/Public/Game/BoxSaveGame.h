#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Game/BoxFlowTypes.h"
#include "BoxSaveGame.generated.h"

UCLASS()
class BOXPUSH_API UBoxSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FName, FBoxLevelRecord> Levels;

	UPROPERTY()
	float MasterVolume = 1.f;

	FBoxLevelRecord GetRecord(FName LevelId) const;
	void RecordClear(FName LevelId);
};
