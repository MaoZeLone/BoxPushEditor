#include "UI/BoxUiToolkit.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"

FSlateFontInfo BoxUi::Font(int32 Size, bool bBold)
{
	if (UFont* Kenney = BoxKenneyUi::FontObject())
	{
		return FSlateFontInfo(static_cast<const UObject*>(Kenney), static_cast<float>(Size));
	}
	return FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size);
}

UTextBlock* BoxUi::MakeText(UWidgetTree* Tree, FName Name, const FText& Label, int32 Size, FLinearColor Color, bool bBold)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Block->SetText(Label);
	Block->SetFont(Font(Size, bBold));
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetJustification(ETextJustify::Center);
	return Block;
}

UButton* BoxUi::MakeButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 Size, EBoxUiButton Kind)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	BoxKenneyUi::ApplyButton(Button, Kind);
	UTextBlock* Block = MakeText(
		Tree,
		*FString::Printf(TEXT("%s_Label"), *Name.ToString()),
		Label,
		Size,
		BoxKenneyUi::LabelColor(Kind),
		true);
	Button->AddChild(Block);
	return Button;
}

UBorder* BoxUi::MakeBorder(UWidgetTree* Tree, FName Name, FLinearColor Color, FMargin Padding)
{
	UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Border->SetBrushColor(Color);
	Border->SetPadding(Padding);
	return Border;
}

UBorder* BoxUi::MakePanel(UWidgetTree* Tree, FName Name, FMargin Padding)
{
	UBorder* Border = MakeBorder(Tree, Name, PanelBg, Padding);
	BoxKenneyUi::ApplyPanel(Border);
	return Border;
}

void BoxUi::StyleSlider(USlider* Slider)
{
	BoxKenneyUi::ApplySlider(Slider);
}
