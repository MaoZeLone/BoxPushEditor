#include "UI/BoxMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Game/BoxGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
#include "UI/BoxUiToolkit.h"

namespace
{
	UVerticalBox* MakeStack(UWidgetTree* Tree, FName Name)
	{
		return Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
	}

	void AddStackChild(UVerticalBox* Box, UWidget* Child, float BottomPad)
	{
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPad));
	}

	USizeBox* WrapWidth(UWidgetTree* Tree, FName Name, UWidget* Child, float Width)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetWidthOverride(Width);
		Box->AddChild(Child);
		return Box;
	}
}

void UBoxSelectCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	CardButton = BoxUi::MakeButton(WidgetTree, TEXT("CardButton"), FText::GetEmpty(), 20, EBoxUiButton::Yellow);
	CardButton->OnClicked.AddDynamic(this, &UBoxSelectCardWidget::OnClicked);
	WidgetTree->RootWidget = CardButton;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	CardButton->ClearChildren();
	CardButton->AddChild(Row);

	NameText = BoxUi::MakeText(WidgetTree, TEXT("NameText"), FText::GetEmpty(), 20, BoxUi::Ink, true);
	NameText->SetJustification(ETextJustify::Left);
	UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(8.f, 10.f));

	StatusText = BoxUi::MakeText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), 16, BoxUi::Muted, false);
	UHorizontalBoxSlot* StatusSlot = Row->AddChildToHorizontalBox(StatusText);
	StatusSlot->SetVerticalAlignment(VAlign_Center);
	StatusSlot->SetPadding(FMargin(8.f, 10.f, 12.f, 10.f));
}

void UBoxSelectCardWidget::Setup(const FBoxSelectEntry& Entry)
{
	LevelId = Entry.LevelId;
	bUnlocked = Entry.bUnlocked;
	if (NameText)
	{
		NameText->SetText(Entry.DisplayName.IsEmpty() ? FText::FromName(Entry.LevelId) : Entry.DisplayName);
		NameText->SetColorAndOpacity(FSlateColor(bUnlocked ? BoxUi::Ink : BoxUi::Muted));
	}
	if (StatusText)
	{
		StatusText->SetText(bUnlocked
			? (Entry.bCleared
				? FText::FromString(TEXT("已通关"))
				: FText::FromString(TEXT("未通关")))
			: FText::FromString(TEXT("锁定")));
	}
	if (CardButton)
	{
		CardButton->SetIsEnabled(bUnlocked);
		BoxKenneyUi::ApplyButton(CardButton, bUnlocked ? EBoxUiButton::Yellow : EBoxUiButton::Grey);
	}
}

void UBoxSelectCardWidget::OnClicked()
{
	if (!bUnlocked || LevelId.IsNone())
	{
		return;
	}
	if (UBoxGameInstance* GI = GetGameInstance<UBoxGameInstance>())
	{
		GI->PlayLevel(LevelId, false);
	}
}

UBoxMenuWidget::UBoxMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UBoxMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
	EBoxMenuScreen Screen = EBoxMenuScreen::Main;
	if (UBoxGameInstance* GI = GameInstance())
	{
		Screen = GI->ConsumeMenuScreen();
	}
	ShowScreen(Screen);
}

void UBoxMenuWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Root = BoxUi::MakeBorder(WidgetTree, TEXT("Root"), BoxUi::PageBg, FMargin(0.f));
	WidgetTree->RootWidget = Root;
	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Switcher"));
	Root->AddChild(Switcher);

	auto MakePage = [this](FName Name, const FText& Title, UVerticalBox*& OutStack)
	{
		UVerticalBox* Page = MakeStack(WidgetTree, Name);
		UVerticalBox* Stack = MakeStack(WidgetTree, *FString::Printf(TEXT("%s_Stack"), *Name.ToString()));
		USizeBox* Width = WrapWidth(WidgetTree, *FString::Printf(TEXT("%s_Width"), *Name.ToString()), Stack, 460.f);
		UVerticalBoxSlot* TitleSlot = Page->AddChildToVerticalBox(BoxUi::MakeText(WidgetTree, *FString::Printf(TEXT("%s_Title"), *Name.ToString()), Title, 42, BoxUi::Gold, true));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 80.f, 0.f, 28.f));
		UVerticalBoxSlot* BodySlot = Page->AddChildToVerticalBox(Width);
		BodySlot->SetHorizontalAlignment(HAlign_Center);
		Switcher->AddChild(Page);
		OutStack = Stack;
	};

	UVerticalBox* MainStack = nullptr;
	MakePage(TEXT("MainPage"), FText::FromString(TEXT("推箱子")), MainStack);
	UButton* Start = BoxUi::MakeButton(WidgetTree, TEXT("StartButton"), FText::FromString(TEXT("开始")), 24, EBoxUiButton::Yellow);
	Start->OnClicked.AddDynamic(this, &UBoxMenuWidget::OnStartClicked);
	AddStackChild(MainStack, Start, 12.f);
	UButton* Settings = BoxUi::MakeButton(WidgetTree, TEXT("SettingsButton"), FText::FromString(TEXT("设置")), 24, EBoxUiButton::Blue);
	Settings->OnClicked.AddDynamic(this, &UBoxMenuWidget::OnSettingsClicked);
	AddStackChild(MainStack, Settings, 12.f);
	UButton* Quit = BoxUi::MakeButton(WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("退出")), 24, EBoxUiButton::Red);
	Quit->OnClicked.AddDynamic(this, &UBoxMenuWidget::OnQuitClicked);
	AddStackChild(MainStack, Quit, 0.f);

	UVerticalBox* SelectStack = nullptr;
	MakePage(TEXT("SelectPage"), FText::FromString(TEXT("选关")), SelectStack);
	SelectList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SelectList"));
	USizeBox* ListHeight = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectHeight"));
	ListHeight->SetHeightOverride(420.f);
	ListHeight->AddChild(SelectList);
	AddStackChild(SelectStack, ListHeight, 16.f);
	UButton* SelectBack = BoxUi::MakeButton(WidgetTree, TEXT("SelectBack"), FText::FromString(TEXT("返回")), 22, EBoxUiButton::Grey);
	SelectBack->OnClicked.AddDynamic(this, &UBoxMenuWidget::OnBackClicked);
	AddStackChild(SelectStack, SelectBack, 0.f);

	UVerticalBox* SettingsStack = nullptr;
	MakePage(TEXT("SettingsPage"), FText::FromString(TEXT("设置")), SettingsStack);
	UHorizontalBox* VolumeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VolumeRow"));
	UTextBlock* VolumeLabel = BoxUi::MakeText(WidgetTree, TEXT("VolumeLabel"), FText::FromString(TEXT("主音量")), 20, BoxUi::Ink, true);
	VolumeLabel->SetJustification(ETextJustify::Left);
	UHorizontalBoxSlot* LabelSlot = VolumeRow->AddChildToHorizontalBox(VolumeLabel);
	LabelSlot->SetVerticalAlignment(VAlign_Center);
	LabelSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	VolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("VolumeSlider"));
	VolumeSlider->SetMinValue(0.f);
	VolumeSlider->SetMaxValue(1.f);
	VolumeSlider->SetStepSize(0.05f);
	VolumeSlider->OnValueChanged.AddDynamic(this, &UBoxMenuWidget::OnVolumeChanged);
	BoxUi::StyleSlider(VolumeSlider);
	UHorizontalBoxSlot* SliderSlot = VolumeRow->AddChildToHorizontalBox(VolumeSlider);
	SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SliderSlot->SetVerticalAlignment(VAlign_Center);
	VolumeText = BoxUi::MakeText(WidgetTree, TEXT("VolumeText"), FText::FromString(TEXT("100%")), 18, BoxUi::Gold, true);
	UHorizontalBoxSlot* ValueSlot = VolumeRow->AddChildToHorizontalBox(VolumeText);
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	ValueSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	AddStackChild(SettingsStack, VolumeRow, 24.f);
	UButton* SettingsBack = BoxUi::MakeButton(WidgetTree, TEXT("SettingsBack"), FText::FromString(TEXT("返回")), 22, EBoxUiButton::Grey);
	SettingsBack->OnClicked.AddDynamic(this, &UBoxMenuWidget::OnBackClicked);
	AddStackChild(SettingsStack, SettingsBack, 0.f);
}

void UBoxMenuWidget::ShowScreen(EBoxMenuScreen Screen)
{
	if (!Switcher)
	{
		return;
	}
	int32 Index = 0;
	switch (Screen)
	{
	case EBoxMenuScreen::Select:
		Index = 1;
		RebuildSelectList();
		break;
	case EBoxMenuScreen::Settings:
		Index = 2;
		RefreshVolume();
		break;
	default:
		Index = 0;
		break;
	}
	Switcher->SetActiveWidgetIndex(Index);
	SetKeyboardFocus();
}

void UBoxMenuWidget::RebuildSelectList()
{
	if (!SelectList)
	{
		return;
	}
	SelectList->ClearChildren();
	UBoxGameInstance* GI = GameInstance();
	if (!GI)
	{
		return;
	}
	const TArray<FBoxSelectEntry> Entries = GI->GetSelectEntries();
	if (Entries.Num() == 0)
	{
		SelectList->AddChild(BoxUi::MakeText(WidgetTree, TEXT("EmptySelect"), FText::FromString(TEXT("还没有上架的关卡")), 18, BoxUi::Muted, false));
		return;
	}
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		UBoxSelectCardWidget* Card = CreateWidget<UBoxSelectCardWidget>(this);
		if (!Card)
		{
			continue;
		}
		Card->Setup(Entries[Index]);
		SelectList->AddChild(Card);
	}
}

void UBoxMenuWidget::RefreshVolume()
{
	const float Volume = GameInstance() ? GameInstance()->GetMasterVolume() : 1.f;
	if (VolumeSlider)
	{
		VolumeSlider->SetValue(Volume);
	}
	if (VolumeText)
	{
		VolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Volume * 100.f))));
	}
}

UBoxGameInstance* UBoxMenuWidget::GameInstance() const
{
	return GetGameInstance<UBoxGameInstance>();
}

void UBoxMenuWidget::OnStartClicked()
{
	ShowScreen(EBoxMenuScreen::Select);
}

void UBoxMenuWidget::OnSettingsClicked()
{
	ShowScreen(EBoxMenuScreen::Settings);
}

void UBoxMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UBoxMenuWidget::OnBackClicked()
{
	ShowScreen(EBoxMenuScreen::Main);
}

void UBoxMenuWidget::OnVolumeChanged(float Value)
{
	if (UBoxGameInstance* GI = GameInstance())
	{
		GI->SetMasterVolume(Value);
	}
	RefreshVolume();
}

FReply UBoxMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && Switcher && Switcher->GetActiveWidgetIndex() != 0)
	{
		ShowScreen(EBoxMenuScreen::Main);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
