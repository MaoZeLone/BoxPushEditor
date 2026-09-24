#pragma once

#include "CoreMinimal.h"

class UBorder;
class UButton;
class UFont;
class USlider;
class UTexture2D;

enum class EBoxUiButton : uint8
{
	Blue,
	Yellow,
	Green,
	Red,
	Grey
};

/** Kenney UI Pack 2.0 + Game Icons，从 Asset/UI 读 PNG / TTF。缺文件时调用方走纯色。 */
namespace BoxKenneyUi
{
	bool IsAvailable();
	UFont* FontObject();
	FLinearColor LabelColor(EBoxUiButton Kind);
	void ApplyButton(UButton* Button, EBoxUiButton Kind);
	void ApplyPanel(UBorder* Border);
	void ApplySlider(USlider* Slider);
	UTexture2D* Icon(const TCHAR* FileName);
}
