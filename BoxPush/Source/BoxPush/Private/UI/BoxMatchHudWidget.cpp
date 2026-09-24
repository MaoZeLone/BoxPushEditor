#include "UI/BoxMatchHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/BoxGameMode.h"
#include "UI/BoxUiToolkit.h"

namespace
{
	void FillCanvas(UCanvasPanel* Canvas, UWidget* Child, const FAnchors& Anchors, FMargin Offsets, int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Child);
		Slot->SetAnchors(Anchors);
		Slot->SetOffsets(Offsets);
		Slot->SetZOrder(ZOrder);
	}

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

void UBoxMatchHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
	Refresh();
}

void UBoxMatchHudWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* TopBar = BoxUi::MakePanel(WidgetTree, TEXT("TopBar"), FMargin(20.f, 10.f));
	UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopRow"));
	TopBar->AddChild(TopRow);

	LevelText = BoxUi::MakeText(WidgetTree, TEXT("LevelText"), FText::GetEmpty(), 22, BoxUi::Ink, true);
	LevelText->SetJustification(ETextJustify::Left);
	UHorizontalBoxSlot* LevelSlot = TopRow->AddChildToHorizontalBox(LevelText);
	LevelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LevelSlot->SetVerticalAlignment(VAlign_Center);

	PlaytestText = BoxUi::MakeText(WidgetTree, TEXT("PlaytestText"), FText::FromString(TEXT("试玩")), 18, BoxUi::Gold, true);
	PlaytestText->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* PlaytestSlot = TopRow->AddChildToHorizontalBox(PlaytestText);
	PlaytestSlot->SetPadding(FMargin(12.f, 0.f));
	PlaytestSlot->SetVerticalAlignment(VAlign_Center);

	PauseButton = BoxUi::MakeButton(WidgetTree, TEXT("PauseButton"), FText::FromString(TEXT("暂停")), 20, EBoxUiButton::Blue);
	PauseButton->OnClicked.AddDynamic(this, &UBoxMatchHudWidget::OnPauseClicked);
	UHorizontalBoxSlot* PauseSlot = TopRow->AddChildToHorizontalBox(WrapWidth(WidgetTree, TEXT("PauseWrap"), PauseButton, 120.f));
	PauseSlot->SetVerticalAlignment(VAlign_Center);

	FillCanvas(Root, TopBar, FAnchors(0.f, 0.f, 1.f, 0.f), FMargin(24.f, 16.f, 24.f, 72.f), 1);

	auto MakeCenteredOverlay = [this](FName Name, UWidget* Panel) -> UOverlay*
	{
		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), Name);
		UBorder* Dim = BoxUi::MakeBorder(WidgetTree, *FString::Printf(TEXT("%s_Dim"), *Name.ToString()), BoxUi::DimBg, FMargin(0.f));
		UOverlaySlot* DimSlot = Overlay->AddChildToOverlay(Dim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
		UOverlaySlot* PanelSlot = Overlay->AddChildToOverlay(Panel);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
		return Overlay;
	};

	UVerticalBox* PauseStack = MakeStack(WidgetTree, TEXT("PauseStack"));
	UBorder* PausePanel = BoxUi::MakePanel(WidgetTree, TEXT("PausePanel"), FMargin(28.f, 24.f));
	PausePanel->AddChild(WrapWidth(WidgetTree, TEXT("PauseWidth"), PauseStack, 420.f));
	AddStackChild(PauseStack, BoxUi::MakeText(WidgetTree, TEXT("PauseTitle"), FText::FromString(TEXT("暂停")), 34, BoxUi::Gold, true), 16.f);
	UButton* Resume = BoxUi::MakeButton(WidgetTree, TEXT("ResumeButton"), FText::FromString(TEXT("继续")), 22, EBoxUiButton::Green);
	Resume->OnClicked.AddDynamic(this, &UBoxMatchHudWidget::OnResumeClicked);
	AddStackChild(PauseStack, Resume, 10.f);
	UButton* Restart = BoxUi::MakeButton(WidgetTree, TEXT("RestartButton"), FText::FromString(TEXT("重开")), 22, EBoxUiButton::Yellow);
	Restart->OnClicked.AddDynamic(this, &UBoxMatchHudWidget::OnRestartClicked);
	AddStackChild(PauseStack, Restart, 10.f);
	UButton* PauseSelect = BoxUi::MakeButton(WidgetTree, TEXT("PauseSelectButton"), FText::FromString(TEXT("回选关")), 22, EBoxUiButton::Grey);
	PauseSelect->OnClicked.AddDynamic(this, &UBoxMatchHudWidget::OnSelectClicked);
	AddStackChild(PauseStack, PauseSelect, 0.f);
	PauseOverlay = MakeCenteredOverlay(TEXT("PauseOverlay"), PausePanel);
	PauseOverlay->SetVisibility(ESlateVisibility::Collapsed);
	FillCanvas(Root, PauseOverlay, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 10);

	UVerticalBox* Center = MakeStack(WidgetTree, TEXT("WinCenter"));
	UBorder* Panel = BoxUi::MakePanel(WidgetTree, TEXT("WinPanel"), FMargin(28.f, 24.f));
	Panel->AddChild(WrapWidth(WidgetTree, TEXT("WinWidth"), Center, 420.f));
	AddStackChild(Center, BoxUi::MakeText(WidgetTree, TEXT("WinTitle"), FText::FromString(TEXT("通关")), 36, BoxUi::Gold, true), 12.f);
	WinNoteText = BoxUi::MakeText(WidgetTree, TEXT("WinNote"), FText::GetEmpty(), 16, BoxUi::Muted, false);
	AddStackChild(Center, WinNoteText, 16.f);
	UButton* Replay = BoxUi::MakeButton(WidgetTree, TEXT("ReplayButton"), FText::FromString(TEXT("再玩")), 22, EBoxUiButton::Yellow);
	Replay->OnClicked.AddDynamic(this, &UBoxMatchHudWidget::OnReplayClicked);
	AddStackChild(Center, Replay, 10.f);
	NextButton = BoxUi::MakeButton(WidgetTree, TEXT("NextButton"), FText::FromString(TEXT("下一关")), 22, EBoxUiButton::Green);
	NextButton->OnClicked.AddDynamic(this, &UBoxMatchHudWidget::OnNextClicked);
	AddStackChild(Center, NextButton, 10.f);
	UButton* ToSelect = BoxUi::MakeButton(WidgetTree, TEXT("WinSelectButton"), FText::FromString(TEXT("回选关")), 22, EBoxUiButton::Grey);
	ToSelect->OnClicked.AddDynamic(this, &UBoxMatchHudWidget::OnSelectClicked);
	AddStackChild(Center, ToSelect, 0.f);
	WinOverlay = MakeCenteredOverlay(TEXT("WinOverlay"), Panel);
	WinOverlay->SetVisibility(ESlateVisibility::Collapsed);
	FillCanvas(Root, WinOverlay, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 11);
}

void UBoxMatchHudWidget::SetOverlayVisible(UWidget* Overlay, bool bVisible)
{
	if (Overlay)
	{
		Overlay->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

ABoxGameMode* UBoxMatchHudWidget::GameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ABoxGameMode>() : nullptr;
}

void UBoxMatchHudWidget::Refresh()
{
	ABoxGameMode* GM = GameMode();
	if (!GM)
	{
		return;
	}

	const EBoxFlowPhase Phase = GM->GetFlowPhase();
	if (LevelText)
	{
		LevelText->SetText(GM->GetLevelDisplayName());
	}
	if (PlaytestText)
	{
		PlaytestText->SetVisibility(GM->IsPlaytest() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	const bool bPaused = Phase == EBoxFlowPhase::Paused;
	const bool bWon = Phase == EBoxFlowPhase::Won;
	SetOverlayVisible(PauseOverlay, bPaused);
	SetOverlayVisible(WinOverlay, bWon);
	if (PauseButton)
	{
		PauseButton->SetVisibility(bWon ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		PauseButton->SetIsEnabled(!bPaused);
	}
	if (bWon)
	{
		if (WinNoteText)
		{
			WinNoteText->SetText(GM->IsPlaytest()
				? FText::FromString(TEXT("试玩不写档"))
				: FText::GetEmpty());
			WinNoteText->SetVisibility(GM->IsPlaytest() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (NextButton)
		{
			const bool bHasNext = GM->HasNextListedLevel();
			NextButton->SetIsEnabled(bHasNext);
			BoxKenneyUi::ApplyButton(NextButton, bHasNext ? EBoxUiButton::Green : EBoxUiButton::Grey);
		}
	}
}

void UBoxMatchHudWidget::OnPauseClicked()
{
	if (ABoxGameMode* GM = GameMode())
	{
		GM->PauseMatch();
	}
}

void UBoxMatchHudWidget::OnResumeClicked()
{
	if (ABoxGameMode* GM = GameMode())
	{
		GM->ResumeMatch();
	}
}

void UBoxMatchHudWidget::OnRestartClicked()
{
	if (ABoxGameMode* GM = GameMode())
	{
		GM->RestartMatch();
	}
}

void UBoxMatchHudWidget::OnReplayClicked()
{
	OnRestartClicked();
}

void UBoxMatchHudWidget::OnNextClicked()
{
	if (ABoxGameMode* GM = GameMode())
	{
		GM->PlayNextLevel();
	}
}

void UBoxMatchHudWidget::OnSelectClicked()
{
	if (ABoxGameMode* GM = GameMode())
	{
		GM->ReturnToSelect();
	}
}
