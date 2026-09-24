#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BoxTypeDisplay.generated.h"

USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxTypeDisplayRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Type")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Type", meta = (DisplayName = "显示名"))
	FText DisplayName;
};

UCLASS()
class BOXPUSH_API UBoxTypeDisplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "BoxPush|Type")
	static FText GetDisplayName(FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Type")
	static bool MatchesType(FGameplayTag AssetType, FGameplayTag Filter);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Type")
	static void ApplyOfficialDefaults(UDataTable* Table);

	static const UDataTable* LoadTable();
	static FGameplayTag InferInteractableType(FName DefinitionId);
	static void CollectPaletteChain(FGameplayTag Tag, TArray<FGameplayTag>& OutChain);

	static FGameplayTag Terrain();
	static FGameplayTag TerrainFloor();
	static FGameplayTag TerrainWall();
	static FGameplayTag TerrainEmpty();
	static FGameplayTag Character();
	static FGameplayTag InteractableTarget();
	static FGameplayTag InteractablePedal();
	static FGameplayTag ToolEraser();
};
