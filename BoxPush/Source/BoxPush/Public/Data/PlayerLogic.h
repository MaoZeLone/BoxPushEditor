#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "PlayerLogic.generated.h"

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class BOXPUSH_API UPlayerLogicComp : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
	FName CompId;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Player Move"))
class BOXPUSH_API UPlayerMoveLogic : public UPlayerLogicComp
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (ClampMin = "0.01"))
	float StepDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
	bool bFourDirections = true;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Player Push"))
class BOXPUSH_API UPlayerPushLogic : public UPlayerLogicComp
{
	GENERATED_BODY()

public:
	UPlayerPushLogic();
	virtual void PostLoad() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (Categories = "Type"))
	FGameplayTag PushType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
	bool bPushOnWalk = true;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Player Camera"))
class BOXPUSH_API UPlayerCameraLogic : public UPlayerLogicComp
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
	FVector BoomOffset = FVector(0.f, -800.f, 800.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
	FVector LookAtOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
	bool bFollowPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic", meta = (ClampMin = "1"))
	float FieldOfView = 50.f;
};
