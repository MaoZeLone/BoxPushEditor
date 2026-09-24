#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/BoxFlowTypes.h"
#include "BoxMenuWidget.generated.h"

class UButton;
class UScrollBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

UCLASS()
class BOXPUSH_API UBoxSelectCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FBoxSelectEntry& Entry);

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void OnClicked();

	FName LevelId;
	bool bUnlocked = false;

	UPROPERTY()
	TObjectPtr<UButton> CardButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};

UCLASS()
class BOXPUSH_API UBoxMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBoxMenuWidget(const FObjectInitializer& ObjectInitializer);

	void ShowScreen(EBoxMenuScreen Screen);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	void RebuildSelectList();
	void RefreshVolume();
	class UBoxGameInstance* GameInstance() const;

	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnVolumeChanged(float Value);

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY()
	TObjectPtr<UScrollBox> SelectList;

	UPROPERTY()
	TObjectPtr<USlider> VolumeSlider;

	UPROPERTY()
	TObjectPtr<UTextBlock> VolumeText;
};
