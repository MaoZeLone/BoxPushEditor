#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BoxActionSet.generated.h"

/**
 * 授给角色的一条操作。在 DA 里填行，不往身上加 Tag。
 * 有没有这个能力 = 表里有没有这一行。
 */
USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxGrantedAction
{
	GENERATED_BODY()

	/** 操作身份。Relation 表按这个查。不是授到身上的 Tag。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	/** 对应 InputConfig 的 InputTag。空 = 不绑键（官方 Push）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/** 角色操作表。对齐 AbilitySet 的「填行」，没有 GA / GE / ASC。 */
UCLASS(BlueprintType, Meta = (DisplayName = "Box Action Set"))
class BOXPUSH_API UBoxActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Actions")
	void ApplyOfficialDefaults();

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Actions")
	void AddGrantedAction(FName AbilityTag, FName InputTag);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Actions")
	void ResetGrantedActions();

	UFUNCTION(BlueprintPure, Category = "BoxPush|Actions")
	bool HasGranted(FGameplayTag AbilityTag) const;

	const FBoxGrantedAction* FindByAbilityTag(FGameplayTag AbilityTag) const;
	const FBoxGrantedAction* FindByInputTag(FGameplayTag InputTag) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions", Meta = (TitleProperty = "AbilityTag"))
	TArray<FBoxGrantedAction> GrantedActions;
};
