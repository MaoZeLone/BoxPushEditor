#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/BoxPushLevelTypes.h"
#include "Data/VisualComps.h"
#include "TerrainDef.generated.h"

UCLASS(BlueprintType)
class BOXPUSH_API UTerrainDef : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FName TerrainId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "显示名"))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "类型", Categories = "Type"))
	FGameplayTag Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	ETerrainCell Cell = ETerrainCell::Floor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FLinearColor PaletteColor = FLinearColor(0.79f, 0.64f, 0.43f);

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "表现"))
	TObjectPtr<UVisualSpriteComp> Sprite;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static UTerrainDef* LoadById(FName InTerrainId);
	static UTerrainDef* FindByCell(ETerrainCell Cell);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Terrain")
	void ApplyOfficialDefaults(FName InTerrainId);
};
