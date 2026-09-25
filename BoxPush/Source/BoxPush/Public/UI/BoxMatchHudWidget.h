#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BoxMatchHudWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

UCLASS()
class BOXPUSH_API UBoxMatchHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();
	void SetOverlayVisible(UWidget* Overlay, bool bVisible);
	class ABoxGameMode* GameMode() const;

	UFUNCTION()
	void OnPauseClicked();

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnReplayClicked();

	UFUNCTION()
	void OnNextClicked();

	UFUNCTION()
	void OnSelectClicked();

	UPROPERTY()
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlaytestText;

	UPROPERTY()
	TObjectPtr<UWidget> HintPanel;

	UPROPERTY()
	TObjectPtr<UButton> PauseButton;

	UPROPERTY()
	TObjectPtr<UWidget> PauseOverlay;

	UPROPERTY()
	TObjectPtr<UWidget> WinOverlay;

	UPROPERTY()
	TObjectPtr<UTextBlock> WinNoteText;

	UPROPERTY()
	TObjectPtr<UButton> NextButton;
};
