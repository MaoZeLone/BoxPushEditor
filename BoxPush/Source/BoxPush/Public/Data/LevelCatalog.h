#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "Data/LevelData.h"
#include "LevelCatalog.generated.h"

class UDataTable;
class UInteractableDef;

USTRUCT(BlueprintType)
struct BOXPUSH_API FLevelCatalogRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
	FName LevelId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
	TSoftObjectPtr<ULevelData> LevelAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
	bool bListed = false;
};

UCLASS()
class BOXPUSH_API ULevelCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "BoxPush|Catalog")
	static TArray<FLevelCatalogRow> GetListedRows(const UDataTable* Catalog);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Catalog")
	static bool FindRow(const UDataTable* Catalog, FName LevelId, FLevelCatalogRow& OutRow);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Catalog")
	static FName GetNextListedLevelId(const UDataTable* Catalog, FName CurrentLevelId);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Catalog")
	static void ValidateCatalog(const UDataTable* Catalog, TArray<FLevelValidationIssue>& OutIssues);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Catalog")
	static void ValidateLevelForListing(const ULevelData* Level, const UDataTable* Catalog, TArray<FLevelValidationIssue>& OutIssues);

	static bool IsPushableDefinition(const UInteractableDef* Def);
	static bool IsGoalDefinition(const UInteractableDef* Def);
};
