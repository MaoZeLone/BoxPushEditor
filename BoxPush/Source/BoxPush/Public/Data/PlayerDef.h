#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/BoxPushLevelTypes.h"
#include "Data/PlayerSpriteDef.h"
#include "PlayerDef.generated.h"

class UBoxInputConfig;
class UBoxActionSet;
class UBoxAbilityTagRelationshipMapping;
class APawn;

/** PawnData：PawnClass + InputConfig + ActionSet + TagRelationship。不授 GA。表现在 Sprite 引用的 DA 上。 */
UCLASS(BlueprintType)
class BOXPUSH_API UPlayerDef : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPlayerDef();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FName PlayerId = TEXT("Player");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "显示名"))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "类型", Categories = "Type"))
	FGameplayTag Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FString DesignerNote;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UBoxInputConfig> InputConfig;

	/** 这个角色有哪些操作。填 GrantedActions[]，不要往身上堆 Tag。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TObjectPtr<UBoxActionSet> ActionSet;

	/** 当前能做什么。对齐 Lyra AbilityTagRelationshipMapping。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TObjectPtr<UBoxAbilityTagRelationshipMapping> TagRelationshipMapping;

	/** 朝向帧和换帧间隔。空着就不画角色。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "表现"))
	TObjectPtr<UPlayerSpriteDef> Sprite;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	FBoxAtlasFrame PickFrame(int32 FacingSteps, bool bPushing, bool bWalking, int32 FrameIndex) const;
	float GetFrameInterval() const;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Player")
	void ApplyOfficialDefaults();

	/** 读 DA_Player。文件打不开时用仍能加载的操作表拼一份临时定义，避免试玩完全不能走。 */
	static UPlayerDef* LoadOfficial();

	/** 表里有这一行，并且 Relation 允许。OwnerTags 只放 State.* / Status.*。 */
	UFUNCTION(BlueprintPure, Category = "BoxPush|Player")
	bool CanPerform(FGameplayTag AbilityTag, const FGameplayTagContainer& OwnerTags) const;

	void Validate(TArray<FLevelValidationIssue>& OutIssues) const;
};
