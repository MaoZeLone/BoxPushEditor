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
	static constexpr int32 DefaultSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", AssetRegistrySearchable)
	FName LevelId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "显示名"), AssetRegistrySearchable)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FString DesignerNote;

	/** 由已铺地形、出生点和交互物算出来，不手填。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rules")
	int32 Width = DefaultSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rules")
	int32 Height = DefaultSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	TArray<ETerrainCell> Cells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	FIntPoint PlayerSpawn = FIntPoint(0, 0);

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

	/** 一格地板。宽高随后按内容自己算。 */
	UFUNCTION(BlueprintCallable, Category = "BoxPush|Level")
	void ApplyNewLevelDefaults(FName NewLevelId);

	/** 把格子扩到能放下 Cell。新格是空洞。超出单边上限则失败，Cell 不变。成功时 Cell 改成扩完后的坐标。 */
	bool ExpandTo(FIntPoint& Cell);

	/** 裁掉四周全空、且没有出生点或交互物的边。 */
	void FitToContent();

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
