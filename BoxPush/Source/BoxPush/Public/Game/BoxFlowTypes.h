#pragma once

#include "CoreMinimal.h"
#include "BoxFlowTypes.generated.h"

class ULevelData;

UENUM(BlueprintType)
enum class EBoxFlowPhase : uint8
{
	None,
	Select,
	Playing,
	Paused,
	Won
};

UENUM(BlueprintType)
enum class EBoxMenuScreen : uint8
{
	Main,
	Select,
	Settings
};

USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxPlayRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<ULevelData> Level;

	UPROPERTY()
	FName LevelId;

	UPROPERTY()
	bool bPlaytest = false;

	bool IsSet() const
	{
		return !LevelId.IsNone() || !Level.IsNull();
	}

	void Reset()
	{
		Level.Reset();
		LevelId = NAME_None;
		bPlaytest = false;
	}
};

USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxLevelRecord
{
	GENERATED_BODY()

	UPROPERTY()
	bool bCleared = false;
};

USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxSelectEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BoxPush")
	FName LevelId;

	UPROPERTY(BlueprintReadOnly, Category = "BoxPush")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "BoxPush")
	int32 SortOrder = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BoxPush")
	bool bUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "BoxPush")
	bool bCleared = false;
};
