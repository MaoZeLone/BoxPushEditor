#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "BoxDataLibrary.generated.h"

class UDataTable;

/**
 * 写表：DataTable 行、建 DA、存盘、Tag。
 * 角色 3C 不要走这里——InputConfig / ActionSet 自己有 AddNativeInputAction、AddGrantedAction。
 * 交互物组件树也不走这里。
 */
UCLASS()
class BOXPUSH_API UBoxDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "BoxPush|Data")
	static FGameplayTag MakeGameplayTag(FName TagName, bool bErrorIfNotFound = false);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Data")
	static UObject* CreateOrLoadAsset(TSubclassOf<UObject> AssetClass, const FString& PackagePath);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Data")
	static UDataTable* CreateOrLoadDataTable(UScriptStruct* RowStruct, const FString& PackagePath);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Data")
	static bool SaveAsset(UObject* Asset);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Data")
	static bool SetTableRowExportText(UDataTable* Table, FName RowName, const FString& ExportText);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Data")
	static FString GetTableRowExportText(const UDataTable* Table, FName RowName);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Data")
	static bool RemoveTableRow(UDataTable* Table, FName RowName);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Data")
	static TArray<FName> GetTableRowNames(const UDataTable* Table);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Data")
	static bool HasTableRow(const UDataTable* Table, FName RowName);
};
