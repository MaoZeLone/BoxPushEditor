#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/BoxPushLevelTypes.h"
#include "LevelData.generated.h"

UCLASS(BlueprintType)
class BOXPUSH_API ULevelData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static constexpr int32 MinSize = 1;
	static constexpr int32 MaxSize = 20;
	static constexpr int32 DefaultSize = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", AssetRegistrySearchable)
	FName LevelId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "显示名"), AssetRegistrySearchable)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FString DesignerNote;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1", ClampMax = "20"))
	int32 Width = DefaultSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1", ClampMax = "20"))
	int32 Height = DefaultSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	TArray<ETerrainCell> Cells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	FIntPoint PlayerSpawn = FIntPoint(2, 3);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	TArray<FBoxLevelInstance> Instances;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Level")
	int32 CellIndex(int32 X, int32 Y) const { return Y * Width + X; }

	UFUNCTION(BlueprintPure, Category = "BoxPush|Level")
	bool IsInside(FIntPoint Cell) const;

	UFUNCTION(BlueprintPure, Category = "BoxPush|Level")
	bool IsStandable(ETerrainCell Cell) const { return Cell == ETerrainCell::Floor; }

	UFUNCTION(BlueprintPure, Category = "BoxPush|Level")
	ETerrainCell GetCell(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Level")
	void SetCell(FIntPoint Coord, ETerrainCell Value);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Level")
	void EnsureCellsSize();

	/** Outer wall, inner floor, 1 player / 1 box / 1 target. */
	UFUNCTION(BlueprintCallable, Category = "BoxPush|Level")
	void ApplyNewLevelDefaults(FName NewLevelId);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Level")
	void ResizeGrid(int32 NewWidth, int32 NewHeight);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Level")
	bool ResizeWouldCrop(int32 NewWidth, int32 NewHeight) const;

	void Validate(TArray<FLevelValidationIssue>& OutIssues) const;

protected:
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
#if WITH_EDITOR
	int32 CachedWidth = DefaultSize;
	int32 CachedHeight = DefaultSize;
#endif
};
