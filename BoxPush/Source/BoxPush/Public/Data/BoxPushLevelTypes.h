#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BoxPushLevelTypes.generated.h"

class UInteractableDef;

UENUM(BlueprintType)
enum class EBoxParamKind : uint8
{
	Bool,
	Int,
	Name,
	Tag
};

USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxInstanceOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FName CompId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	EBoxParamKind Kind = EBoxParamKind::Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool BoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	int32 IntValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FName NameValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FGameplayTag TagValue;
};

UENUM(BlueprintType)
enum class ETerrainCell : uint8
{
	Empty UMETA(DisplayName = "空洞"),
	Floor UMETA(DisplayName = "地板"),
	Wall UMETA(DisplayName = "墙")
};

USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxLevelInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	FName InstanceId;

	/** 指向交互物定义 DA。表现和逻辑都从这份 DA 读。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	TSoftObjectPtr<UInteractableDef> Definition;

	/** 软引用为空时的回退。LoadDefinition 会用它去 /Game/Data/Interactables/DA_<Id> 找。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	FName DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	FIntPoint Cell = FIntPoint::ZeroValue;

	/** 0 上、1 右、2 下、3 左。相对归位后的俯视画面。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance", meta = (ClampMin = "0", ClampMax = "3", DisplayName = "朝向"))
	int32 YawSteps = 0;

	/** 只存和定义不同、且定义允许重载的参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	TArray<FBoxInstanceOverride> ParamOverrides;

	const UInteractableDef* LoadDefinition() const;
	FName GetResolvedDefinitionId() const;
	void BackfillDefinitionIfNeeded();
};

USTRUCT(BlueprintType)
struct BOXPUSH_API FLevelValidationIssue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	bool bError = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	TArray<FIntPoint> Cells;
};
