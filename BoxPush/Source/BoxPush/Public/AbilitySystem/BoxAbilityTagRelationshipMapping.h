#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BoxAbilityTagRelationshipMapping.generated.h"

/**
 * 一条 Ability Tag 关系。对齐 Lyra FLyraAbilityTagRelationship。
 * AbilityTag 是操作身份（Ability.Move），不是 InputTag。
 */
USTRUCT(BlueprintType)
struct BOXPUSH_API FBoxAbilityTagRelationship
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ability", Meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	/** 此操作激活期间，阻止激活带这些 Tag 的操作 */
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTagContainer AbilityTagsToBlock;

	/** 此操作激活期间，取消带这些 Tag 的操作 */
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTagContainer AbilityTagsToCancel;

	/** 激活此操作时，身上必须有这些 Tag */
	UPROPERTY(EditAnywhere, Category = "Activation")
	FGameplayTagContainer ActivationRequiredTags;

	/** 激活此操作时，身上不能有这些 Tag */
	UPROPERTY(EditAnywhere, Category = "Activation")
	FGameplayTagContainer ActivationBlockedTags;
};

/**
 * 当前能做什么：用身上的 Tag 查这张表。
 * 对齐 Lyra ULyraAbilityTagRelationshipMapping，挂在 PlayerDef 上。
 */
UCLASS(BlueprintType, Meta = (DisplayName = "Box Ability Tag Relationships"))
class BOXPUSH_API UBoxAbilityTagRelationshipMapping : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "BoxPush|Ability")
	void ApplyOfficialDefaults();

	UFUNCTION(BlueprintPure, Category = "BoxPush|Ability")
	bool CanActivate(FGameplayTag AbilityTag, const FGameplayTagContainer& OwnerTags) const;

	void GetActivationRequirements(
		FGameplayTag AbilityTag,
		FGameplayTagContainer& OutRequired,
		FGameplayTagContainer& OutBlocked) const;

	void GetBlockAndCancelTags(
		const FGameplayTagContainer& AbilityTags,
		FGameplayTagContainer& OutTagsToBlock,
		FGameplayTagContainer& OutTagsToCancel) const;

	UPROPERTY(EditAnywhere, Category = "Ability", Meta = (TitleProperty = "AbilityTag"))
	TArray<FBoxAbilityTagRelationship> Relationships;
};
