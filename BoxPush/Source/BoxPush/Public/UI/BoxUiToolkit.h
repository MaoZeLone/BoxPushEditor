#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "UI/BoxKenneyUi.h"

class UBorder;
class UButton;
class USlider;
class UTextBlock;
class UWidgetTree;

namespace BoxUi
{
	inline const FLinearColor PageBg(0.94f, 0.88f, 0.74f, 1.f);
	inline const FLinearColor PanelBg(0.96f, 0.93f, 0.86f, 1.f);
	inline const FLinearColor HudBg(0.96f, 0.93f, 0.86f, 0.92f);
	inline const FLinearColor DimBg(0.02f, 0.02f, 0.03f, 0.62f);
	inline const FLinearColor ButtonBg(0.16f, 0.19f, 0.24f, 1.f);
	inline const FLinearColor ButtonDisabled(0.1f, 0.11f, 0.13f, 1.f);
	inline const FLinearColor Gold(0.55f, 0.36f, 0.16f, 1.f);
	inline const FLinearColor Ink(0.22f, 0.14f, 0.09f, 1.f);
	inline const FLinearColor Muted(0.45f, 0.36f, 0.28f, 1.f);

	FSlateFontInfo Font(int32 Size, bool bBold = false);
	UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FText& Label, int32 Size, FLinearColor Color = Ink, bool bBold = false);
	UButton* MakeButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 Size = 22, EBoxUiButton Kind = EBoxUiButton::Blue);
	UBorder* MakeBorder(UWidgetTree* Tree, FName Name, FLinearColor Color, FMargin Padding);
	UBorder* MakePanel(UWidgetTree* Tree, FName Name, FMargin Padding);
	void StyleSlider(USlider* Slider);
}
