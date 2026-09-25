#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/BoxPushLevelTypes.h"
#include "Data/InteractableLogic.h"
#include "Data/InteractableState.h"
#include "Data/VisualComps.h"
#include "InteractableDef.generated.h"

UCLASS(BlueprintType)
class BOXPUSH_API UInteractableDef : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FName DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "显示名"))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "类型", Categories = "Type"))
	FGameplayTag Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	FString DesignerNote;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (DisplayName = "色块"))
	FLinearColor PaletteColor = FLinearColor::White;

	UPROPERTY()
	bool bOfficialOverridesSeeded = false;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "表现"))
	TArray<TObjectPtr<UVisualSpriteComp>> SpriteComps;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Logic")
	TArray<TObjectPtr<UInteractableLogicComp>> LogicComps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	TArray<FInteractableStateDef> States;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	TArray<FInteractableTransitionDef> Transitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "状态表现"))
	TArray<FStateVisual> StateVisuals;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "转移表现"))
	TArray<FTransitionVisual> TransitionVisuals;

	UFUNCTION()
	TArray<FString> GetVisualStateOptions() const;

	UFUNCTION()
	TArray<FString> GetVisualCompOptions() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual void PostLoad() override;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Interactable")
	void ApplyOfficialDefaults(FName InDefinitionId);

	bool SeedOfficialOverrideFlags();

	UFUNCTION(BlueprintPure, Category = "BoxPush|Interactable")
	FName GetDefaultState() const;

	FName ResolveTransition(FName FromState, FName EventId, const TArray<FBoxInstanceOverride>* Overrides = nullptr) const;

	/** BeginOverlap / EndOverlap。OnEvent 不走这里。 */
	FName ResolveCondition(FName FromState, EBoxTransitionCondition Condition) const;

	const FInteractableStateDef* FindState(FName StateId) const;

	void Validate(TArray<FLevelValidationIssue>& OutIssues) const;

	template <typename T>
	const T* FindLogic() const
	{
		for (const UInteractableLogicComp* Comp : LogicComps)
		{
			if (const T* Casted = Cast<T>(Comp))
			{
				return Casted;
			}
		}
		return nullptr;
	}
};
