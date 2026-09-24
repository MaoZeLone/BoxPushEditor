#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "BoxPlayerCharacter.generated.h"

class UStaticMeshComponent;
class UInputAction;
class UPlayerDef;
class ABoxGameMode;

UCLASS()
class BOXPUSH_API ABoxPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABoxPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Player")
	void ApplyPlayerDef(const UPlayerDef* InPlayerDef);

	UFUNCTION(BlueprintPure, Category = "BoxPush|Player")
	bool CanPerform(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Player")
	void AddOwnedTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "BoxPush|Player")
	void RemoveOwnedTag(FGameplayTag Tag);

	void SyncLocomotion();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<const UPlayerDef> PlayerDef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	FGameplayTagContainer OwnedTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> SpritePlane;

	virtual void BeginPlay() override;

private:
	void BindMappingContext();
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void PerformGrantedAbility(FGameplayTag AbilityTag);
	void OnRedoFallback();

	ABoxGameMode* FindGameMode() const;
	void PollMoveKeys();
	void RefreshLocomotion();

	uint8 PrevMoveKeys = 0;
	FGameplayTagContainer HeldAbilityInputTags;
	int32 SpriteFrame = 0;
	float SpriteAnimTime = 0.f;
};
