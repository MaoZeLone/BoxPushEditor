#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BoxInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/** IMC + Native（手动绑，如移动）+ Ability（按 Tag 自动绑）。操作是否存在看 ActionSet，不走 GAS。 */
UCLASS(BlueprintType, Meta = (DisplayName = "Box Input Config"))
class BOXPUSH_API UBoxInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Input")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound = true) const;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Input")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound = true) const;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Input")
	void AddNativeInputAction(const UInputAction* Action, FName TagName);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Input")
	void AddAbilityInputAction(const UInputAction* Action, FName TagName);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Input")
	void ResetNativeInputActions();

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Input")
	void ResetAbilityInputActions();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** 要读轴值、手动绑的操作。官方只有移动。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Native", Meta = (TitleProperty = "InputAction"))
	TArray<FBoxInputAction> NativeInputActions;

	/** 按下只发 InputTag，由 ActionSet 对上操作。对齐 Lyra AbilityInputActions。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Ability", Meta = (TitleProperty = "InputAction"))
	TArray<FBoxInputAction> AbilityInputActions;

private:
	bool SplitAbilityBindsOutOfNative();
};
