#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VisualComps.generated.h"

class UTexture2D;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class BOXPUSH_API UInteractableVisualComp : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FName CompId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FName ParentId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector RelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<FName> VisibleInStates;
};

/** 挂在交互物、地形的表现上。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Visual Sprite"))
class BOXPUSH_API UVisualSpriteComp : public UInteractableVisualComp
{
	GENERATED_BODY()

public:
	/** 空着就不画。宽高为 0 用整张；宽高大于 0 从这张贴图上裁。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "贴图"))
	TSoftObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceW = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceH = 0;

	/** 0 表示收进一格（和方块占地相同）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0", DisplayName = "世界边长"))
	float WorldSpan = 0.f;
};
