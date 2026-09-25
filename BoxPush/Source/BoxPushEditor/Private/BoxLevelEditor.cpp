#include "BoxLevelEditor.h"

#include "BoxLevelViewport.h"
#include "AdvancedPreviewScene.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/BoxPlayerCharacter.h"
#include "Data/BoxAssetPaths.h"
#include "Data/BoxInstanceParams.h"
#include "Data/BoxPushLevelTypes.h"
#include "Data/BoxTypeDisplay.h"
#include "Data/InteractableDef.h"
#include "Data/InteractableLogic.h"
#include "Data/LevelCatalog.h"
#include "Data/LevelData.h"
#include "Data/PlayerDef.h"
#include "Data/TerrainDef.h"
#include "Data/VisualComps.h"
#include "Engine/Texture2D.h"
#include "Match/BoxSokobanSheet.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Match/BoxGrid.h"
#include "Match/BoxGridSim.h"
#include "Match/BoxMatchWorld.h"
#include "Misc/MessageDialog.h"
#include "ObjectTools.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "BoxLevelEditor"

namespace
{
	const FLinearColor DetailLabelColor(0.58f, 0.58f, 0.58f);
	const FLinearColor DetailHintColor(0.46f, 0.46f, 0.46f);
	const FLinearColor DetailValueColor(0.92f, 0.92f, 0.92f);

	FSlateFontInfo DetailTitleFont()
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 15);
	}

	FSlateFontInfo DetailCategoryFont()
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 11);
	}

	FSlateFontInfo DetailLabelFont()
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9);
	}

	FSlateFontInfo DetailValueFont()
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10);
	}

	FSlateFontInfo DetailHintFont()
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 8);
	}

	TSharedRef<SWidget> DetailTitle(const TAttribute<FText>& Text)
	{
		return SNew(STextBlock)
			.Font(DetailTitleFont())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.96f, 0.96f)))
			.Text(Text);
	}

	/** bLead：紧跟标题的第一组，上面少留一截。后面的组多留，把块分开。 */
	TSharedRef<SWidget> DetailCategory(const FText& Text, bool bLead = false)
	{
		return SNew(SBorder)
			.Padding(FMargin(0.f, bLead ? 2.f : 14.f, 0.f, 0.f))
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
				[
					SNew(STextBlock)
					.Margin(FMargin(10, 6))
					.Font(DetailCategoryFont())
					.ColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.94f, 0.94f)))
					.Text(Text)
				]
			];
	}

	TSharedRef<SWidget> DetailRow(const FText& Label, TSharedRef<SWidget> Field)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 5)
			[
				SNew(SBox).WidthOverride(96)
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(DetailLabelFont())
					.ColorAndOpacity(FSlateColor(DetailLabelColor))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 5, 10, 5)
			[
				Field
			];
	}

	TSharedRef<SWidget> DetailValue(const TAttribute<FText>& Text)
	{
		return SNew(STextBlock)
			.Font(DetailValueFont())
			.ColorAndOpacity(FSlateColor(DetailValueColor))
			.Text(Text);
	}

	TSharedRef<SWidget> DetailHint(const FText& Text)
	{
		return SNew(STextBlock)
			.AutoWrapText(true)
			.Font(DetailHintFont())
			.ColorAndOpacity(FSlateColor(DetailHintColor))
			.Text(Text);
	}

	UTexture2D* TextureFromSprite(const UVisualSpriteComp* Comp)
	{
		if (!Comp)
		{
			return nullptr;
		}
		return BoxSokoban::ResolveSprite(
			Comp->Texture.LoadSynchronous(), Comp->SourceX, Comp->SourceY, Comp->SourceW, Comp->SourceH, false);
	}

	UTexture2D* TextureFromFrame(const FBoxAtlasFrame& Frame)
	{
		return BoxSokoban::ResolveSprite(
			Frame.Texture.LoadSynchronous(), Frame.SourceX, Frame.SourceY, Frame.SourceW, Frame.SourceH, false);
	}

	TSharedPtr<FSlateBrush> MakeSpriteIcon(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return nullptr;
		}
		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		Brush->SetResourceObject(Texture);
		Brush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Brush->DrawAs = ESlateBrushDrawType::Image;
		Brush->Tiling = ESlateBrushTileType::NoTile;
		return Brush;
	}

	UTexture2D* HoleIcon();

	void AssignPaletteIcon(FBoxPaletteAsset& Asset)
	{
		UTexture2D* Texture = nullptr;
		switch (Asset.Brush)
		{
		case EBoxLevelBrush::Floor:
		case EBoxLevelBrush::Wall:
		case EBoxLevelBrush::Empty:
			Texture = TextureFromSprite(Asset.Terrain.IsValid() ? Asset.Terrain->Sprite : nullptr);
			if (Asset.Brush == EBoxLevelBrush::Empty && !Texture)
			{
				Texture = HoleIcon();
			}
			break;
		case EBoxLevelBrush::Player:
			if (const UPlayerDef* Look = UPlayerDef::LoadOfficial())
			{
				Texture = TextureFromFrame(Look->PickFrame(2, false, false, 0));
			}
			break;
		case EBoxLevelBrush::Interactable:
		{
			const UInteractableDef* Def = Asset.Definition.Get();
			const UVisualSpriteComp* Comp = (Def && Def->SpriteComps.Num() > 0) ? Def->SpriteComps[0].Get() : nullptr;
			Texture = TextureFromSprite(Comp);
			break;
		}
		default:
			break;
		}
		Asset.Icon = MakeSpriteIcon(Texture);
	}

	UTexture2D* HoleIcon()
	{
		static TWeakObjectPtr<UTexture2D> Cached;
		if (Cached.IsValid())
		{
			return Cached.Get();
		}
		constexpr int32 Size = 64;
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
		if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		{
			return nullptr;
		}
		Texture->SRGB = true;
		Texture->Filter = TF_Nearest;
		Texture->NeverStream = true;
		Texture->CompressionSettings = TC_EditorIcon;
#if WITH_EDITORONLY_DATA
		Texture->MipGenSettings = TMGS_NoMipmaps;
#endif
		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		uint8* Dest = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				const bool bEdge = X < 2 || Y < 2 || X >= Size - 2 || Y >= Size - 2;
				uint8* Pixel = Dest + (Y * Size + X) * 4;
				Pixel[0] = bEdge ? 40 : 8;
				Pixel[1] = bEdge ? 40 : 8;
				Pixel[2] = bEdge ? 40 : 8;
				Pixel[3] = 255;
			}
		}
		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		Texture->AddToRoot();
		Cached = Texture;
		return Texture;
	}

	FBoxPaletteAsset MakeTool(FName Id, FGameplayTag TypeTag, const TCHAR* Name, FLinearColor Color, EBoxLevelBrush Brush)
	{
		FBoxPaletteAsset Asset;
		Asset.Id = Id;
		Asset.TypeTag = TypeTag;
		Asset.Name = FText::FromString(Name);
		Asset.Type = UBoxTypeDisplayLibrary::GetDisplayName(TypeTag);
		Asset.Color = Color;
		Asset.Brush = Brush;
		return Asset;
	}

	FString FolderLabel(FGameplayTag Tag)
	{
		return UBoxTypeDisplayLibrary::GetDisplayName(Tag).ToString();
	}

	FText DescribeLogic(const UInteractableDef* Def)
	{
		if (!Def)
		{
			return FText::GetEmpty();
		}
		TArray<FString> Parts;
		for (const UInteractableLogicComp* Comp : Def->LogicComps)
		{
			if (const UPushableLogic* Pushable = Cast<UPushableLogic>(Comp))
			{
				Parts.Add(FString::Printf(TEXT("一次推 %d 格"), Pushable->Steps));
			}
			else if (const USlideLogic* Slide = Cast<USlideLogic>(Comp))
			{
				Parts.Add(Slide->bStopOnBox ? TEXT("滑到停下，遇箱停") : TEXT("滑到停下，可穿过箱子"));
			}
			else if (const UReturnLogic* Return = Cast<UReturnLogic>(Comp))
			{
				Parts.Add(FString::Printf(TEXT("推完返回，延迟 %d 步"), Return->DelayMoves));
			}
			else if (const UGoalLogic* Goal = Cast<UGoalLogic>(Comp))
			{
				Parts.Add(FString::Printf(TEXT("目标，要叠上%s"), *FolderLabel(Goal->RequiredType)));
			}
			else if (const UPedalLogic* Pedal = Cast<UPedalLogic>(Comp))
			{
				Parts.Add(FString::Printf(TEXT("踏板，接受%s"), *FolderLabel(Pedal->AcceptType)));
			}
			else if (const UTriggerLogic* Trigger = Cast<UTriggerLogic>(Comp))
			{
				Parts.Add(Trigger->AcceptType.IsValid()
					? FString::Printf(TEXT("触发，接受%s"), *FolderLabel(Trigger->AcceptType))
					: FString(TEXT("触发，接受任意")));
			}
			else if (const UBlockingLogic* Blocking = Cast<UBlockingLogic>(Comp))
			{
				if (Blocking->bBlocksPlayer && Blocking->bBlocksPush)
				{
					Parts.Add(TEXT("阻挡"));
				}
				else if (Blocking->bBlocksPlayer)
				{
					Parts.Add(TEXT("挡人"));
				}
				else if (Blocking->bBlocksPush)
				{
					Parts.Add(TEXT("挡推"));
				}
			}
		}
		return FText::FromString(FString::Join(Parts, TEXT(" · ")));
	}

	bool ReadOverrideDefault(const UInteractableDef* Def, FName CompId, FName Key, FBoxShownParam& Out)
	{
		return BoxInstanceParams::FindDefault(Def, CompId, Key, Out);
	}
}

ULevelData* FBoxLevelListItem::Resolve()
{
	if (ULevelData* Existing = Level.Get())
	{
		return Existing;
	}
	Level = Cast<ULevelData>(Asset.GetAsset());
	if (ULevelData* Loaded = Level.Get())
	{
		if (!Loaded->LevelId.IsNone())
		{
			LevelId = Loaded->LevelId;
		}
		if (!Loaded->DisplayName.IsEmpty())
		{
			DisplayName = Loaded->DisplayName;
		}
	}
	return Level.Get();
}

bool FBoxLevelListItem::Matches(const ULevelData* InLevel) const
{
	if (!InLevel)
	{
		return false;
	}
	if (Level.Get() == InLevel)
	{
		return true;
	}
	return Asset.GetSoftObjectPath() == FSoftObjectPath(InLevel);
}

FBoxLevelEditor::FBoxLevelEditor() = default;

FBoxLevelEditor::~FBoxLevelEditor()
{
	if (FilesLoadedHandle.IsValid())
	{
		if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
		{
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().OnFilesLoaded().Remove(FilesLoadedHandle);
		}
		FilesLoadedHandle.Reset();
	}
	if (PreviewPlayer)
	{
		PreviewPlayer->Destroy();
		PreviewPlayer = nullptr;
	}
	if (MatchWorld)
	{
		MatchWorld->Destroy();
		MatchWorld = nullptr;
	}
}

void FBoxLevelEditor::Initialize()
{
	PreviewScene = MakeUnique<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
	PreviewScene->SetFloorVisibility(false);
	Status = TEXT("就绪");
	BindAssetRegistry();
	RebuildPalette();
	RebuildLevelList();
	EnsureCurrentLevel();
}

void FBoxLevelEditor::BindAssetRegistry()
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	if (Registry.IsLoadingAssets() && !FilesLoadedHandle.IsValid())
	{
		FilesLoadedHandle = Registry.OnFilesLoaded().AddSP(AsShared(), &FBoxLevelEditor::OnAssetRegistryReady);
		Status = TEXT("正在扫描关卡…");
	}
}

void FBoxLevelEditor::OnAssetRegistryReady()
{
	RebuildPalette();
	RebuildLevelList();
	EnsureCurrentLevel();
	NotifyChanged();
}

void FBoxLevelEditor::EnsureCurrentLevel()
{
	if (CurrentLevel)
	{
		return;
	}
	if (ULevelData* Startup = LoadObject<ULevelData>(nullptr, *BoxAssetPaths::StartupLevel()))
	{
		SelectLevel(Startup);
		return;
	}
	for (const TSharedPtr<FBoxLevelListItem>& Item : LevelItems)
	{
		if (Item)
		{
			if (ULevelData* Level = Item->Resolve())
			{
				SelectLevel(Level);
				return;
			}
		}
	}
}

void FBoxLevelEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(CurrentLevel);
	Collector.AddReferencedObject(MatchWorld);
	Collector.AddReferencedObject(PreviewPlayer);
}

TSharedRef<SWidget> FBoxLevelEditor::MakeWidget()
{
	if (!Viewport)
	{
		SAssignNew(Viewport, SBoxLevelViewport, AsShared());
		Viewport->FrameBoard();
	}
	return Viewport.ToSharedRef();
}

void FBoxLevelEditor::NotifyChanged()
{
	OnChanged.Broadcast();
}

struct FBoxEditScope
{
	FBoxLevelEditor* Editor = nullptr;
	bool bOwn = false;

	explicit FBoxEditScope(FBoxLevelEditor* InEditor)
		: Editor(InEditor)
		, bOwn(InEditor && !InEditor->bEditOpen)
	{
		if (bOwn)
		{
			Editor->BeginEditStroke();
		}
	}

	~FBoxEditScope()
	{
		if (bOwn && Editor)
		{
			Editor->EndEditStroke();
		}
	}
};

FBoxLevelEditor::FBoxEditSnapshot FBoxLevelEditor::CaptureEdit() const
{
	FBoxEditSnapshot Snap;
	if (!CurrentLevel)
	{
		return Snap;
	}
	Snap.Cells = CurrentLevel->Cells;
	Snap.Instances = CurrentLevel->Instances;
	Snap.PlayerSpawn = CurrentLevel->PlayerSpawn;
	Snap.Width = CurrentLevel->Width;
	Snap.Height = CurrentLevel->Height;
	Snap.DisplayName = CurrentLevel->DisplayName;
	Snap.DesignerNote = CurrentLevel->DesignerNote;
	return Snap;
}

bool FBoxLevelEditor::SameEdit(const FBoxEditSnapshot& A, const FBoxEditSnapshot& B) const
{
	if (A.Width != B.Width || A.Height != B.Height || A.PlayerSpawn != B.PlayerSpawn
		|| A.DesignerNote != B.DesignerNote || !A.DisplayName.EqualTo(B.DisplayName)
		|| A.Cells != B.Cells || A.Instances.Num() != B.Instances.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < A.Instances.Num(); ++Index)
	{
		const FBoxLevelInstance& Left = A.Instances[Index];
		const FBoxLevelInstance& Right = B.Instances[Index];
		if (Left.InstanceId != Right.InstanceId || Left.DefinitionId != Right.DefinitionId
			|| Left.Cell != Right.Cell || Left.YawSteps != Right.YawSteps
			|| Left.Definition != Right.Definition
			|| !BoxInstanceParams::SameOverrides(Left.ParamOverrides, Right.ParamOverrides))
		{
			return false;
		}
	}
	return true;
}

void FBoxLevelEditor::ApplyEdit(const FBoxEditSnapshot& Snap)
{
	if (!CurrentLevel)
	{
		return;
	}
	const bool bSizeChanged = Snap.Width != CurrentLevel->Width || Snap.Height != CurrentLevel->Height;
	CurrentLevel->Cells = Snap.Cells;
	CurrentLevel->Instances = Snap.Instances;
	CurrentLevel->PlayerSpawn = Snap.PlayerSpawn;
	CurrentLevel->Width = Snap.Width;
	CurrentLevel->Height = Snap.Height;
	CurrentLevel->DisplayName = Snap.DisplayName;
	CurrentLevel->DesignerNote = Snap.DesignerNote;
	CurrentLevel->MarkPackageDirty();
	PruneSelection();
	RebuildIssues();
	RefreshPreview();
	if (bSizeChanged)
	{
		FrameCamera();
	}
}

void FBoxLevelEditor::ClearEditHistory()
{
	UndoEdits.Reset();
	RedoEdits.Reset();
	bEditOpen = false;
}

void FBoxLevelEditor::BeginEditStroke()
{
	if (bEditOpen || bPlaying || !CurrentLevel)
	{
		return;
	}
	UndoEdits.Add(CaptureEdit());
	if (UndoEdits.Num() > 64)
	{
		UndoEdits.RemoveAt(0);
	}
	RedoEdits.Reset();
	bEditOpen = true;
}

void FBoxLevelEditor::EndEditStroke()
{
	if (!bEditOpen)
	{
		return;
	}
	bEditOpen = false;
	if (UndoEdits.Num() > 0 && SameEdit(UndoEdits.Last(), CaptureEdit()))
	{
		UndoEdits.Pop();
	}
}

void FBoxLevelEditor::UndoEdit()
{
	if (bPlaying || bEditOpen || UndoEdits.Num() == 0 || !CurrentLevel)
	{
		return;
	}
	RedoEdits.Add(CaptureEdit());
	const FBoxEditSnapshot Snap = UndoEdits.Pop();
	ApplyEdit(Snap);
	Status = TEXT("已撤销");
	NotifyChanged();
}

void FBoxLevelEditor::RedoEdit()
{
	if (bPlaying || bEditOpen || RedoEdits.Num() == 0 || !CurrentLevel)
	{
		return;
	}
	UndoEdits.Add(CaptureEdit());
	const FBoxEditSnapshot Snap = RedoEdits.Pop();
	ApplyEdit(Snap);
	Status = TEXT("已重做");
	NotifyChanged();
}

bool FBoxLevelEditor::HandleEditKey(const FKey& Key, bool bControlDown)
{
	if (bPlaying)
	{
		return false;
	}
	if (bControlDown && Key == EKeys::Z)
	{
		UndoEdit();
		return true;
	}
	if (bControlDown && Key == EKeys::Y)
	{
		RedoEdit();
		return true;
	}
	if (Mode == EBoxEditorMode::Configure && (Key == EKeys::Delete || Key == EKeys::BackSpace))
	{
		DeleteSelected();
		return true;
	}
	return false;
}

FText FBoxLevelEditor::GetControlsHint() const
{
	if (bPlaying)
	{
		return LOCTEXT("PlayHint", "WASD 移动\nZ 撤销   Y 重做\nR 重开   Esc 停止\n中键平移\nAlt+左键旋转\n滚轮缩放   F 归位");
	}
	if (Mode == EBoxEditorMode::Configure)
	{
		return LOCTEXT("ConfigHint", "左键选中物体\n按住物体拖到格子\n拖坐标轴也能移动\nDelete 删除此物体\n右键取消选中\nCtrl+Z 撤销\n中键平移\nAlt+左键旋转\n滚轮缩放   F 归位");
	}
	return LOCTEXT("EditHint", "左键铺设   右键只擦物体\nCtrl+Z 撤销   Ctrl+Y 重做\n中键平移\nAlt+左键旋转\n滚轮缩放   F 归位");
}

UDataTable* FBoxLevelEditor::LoadCatalog() const
{
	return LoadObject<UDataTable>(nullptr, *BoxAssetPaths::LevelCatalog());
}

bool FBoxLevelEditor::IsListed(const ULevelData* Level) const
{
	FLevelCatalogRow Row;
	return Level && ULevelCatalogLibrary::FindRow(LoadCatalog(), Level->LevelId, Row) && Row.bListed;
}

bool FBoxLevelEditor::IsListedAsset(const FAssetData& Asset) const
{
	UDataTable* Catalog = LoadCatalog();
	if (!Catalog)
	{
		return false;
	}
	const FSoftObjectPath Path = Asset.GetSoftObjectPath();
	bool bFound = false;
	Catalog->ForeachRow<FLevelCatalogRow>(TEXT("BoxListedAsset"), [&](const FName&, const FLevelCatalogRow& Row)
	{
		if (!bFound && Row.bListed && Row.LevelAsset.ToSoftObjectPath() == Path)
		{
			bFound = true;
		}
	});
	return bFound;
}

FString FBoxLevelEditor::GetFolderPath() const
{
	for (const TSharedPtr<FBoxPaletteFolder>& Folder : Folders)
	{
		if (Folder && Folder->Tag == FolderTag)
		{
			return Folder->Path;
		}
	}
	return TEXT("游戏");
}

void FBoxLevelEditor::RebuildLevelList()
{
	LevelItems.Reset();
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(ULevelData::StaticClass()->GetClassPathName(), Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.AssetName.LexicalLess(B.AssetName);
	});
	const FString Filter = LevelFilter.ToLower();
	for (const FAssetData& Asset : Assets)
	{
		TSharedPtr<FBoxLevelListItem> Item = MakeShared<FBoxLevelListItem>();
		Item->Asset = Asset;
		Item->LevelId = Asset.AssetName;
		FString IdTag;
		if (Asset.GetTagValue(GET_MEMBER_NAME_CHECKED(ULevelData, LevelId), IdTag) && !IdTag.IsEmpty())
		{
			Item->LevelId = *IdTag;
		}
		FString NameTag;
		if (Asset.GetTagValue(GET_MEMBER_NAME_CHECKED(ULevelData, DisplayName), NameTag) && !NameTag.IsEmpty())
		{
			Item->DisplayName = FText::FromString(NameTag);
		}
		else
		{
			Item->DisplayName = FText::FromName(Asset.AssetName);
		}
		if (CurrentLevel && Item->Matches(CurrentLevel))
		{
			Item->Level = CurrentLevel;
			Item->LevelId = CurrentLevel->LevelId.IsNone() ? Item->LevelId : CurrentLevel->LevelId;
			if (!CurrentLevel->DisplayName.IsEmpty())
			{
				Item->DisplayName = CurrentLevel->DisplayName;
			}
		}
		const FString Hay = (Item->LevelId.ToString() + TEXT(" ") + Item->DisplayName.ToString() + TEXT(" ") + Asset.AssetName.ToString()).ToLower();
		if (!Filter.IsEmpty() && !Hay.Contains(Filter))
		{
			continue;
		}
		Item->bListed = IsListedAsset(Asset);
		LevelItems.Add(Item);
	}

	UDataTable* Catalog = LoadCatalog();
	struct FCatalogSlot
	{
		FName RowName;
		int32 SortOrder = 0;
		FSoftObjectPath AssetPath;
	};
	TArray<FCatalogSlot> Slots;
	if (Catalog)
	{
		Catalog->ForeachRow<FLevelCatalogRow>(TEXT("BoxLevelOrder"), [&Slots](const FName& RowName, const FLevelCatalogRow& Row)
		{
			FCatalogSlot Slot;
			Slot.RowName = RowName;
			Slot.SortOrder = Row.SortOrder;
			Slot.AssetPath = Row.LevelAsset.ToSoftObjectPath();
			Slots.Add(Slot);
		});
		Slots.Sort([](const FCatalogSlot& A, const FCatalogSlot& B)
		{
			if (A.SortOrder != B.SortOrder)
			{
				return A.SortOrder < B.SortOrder;
			}
			return A.RowName.LexicalLess(B.RowName);
		});
	}
	for (TSharedPtr<FBoxLevelListItem>& Item : LevelItems)
	{
		if (!Item)
		{
			continue;
		}
		Item->CatalogCount = Slots.Num();
		const int32 Found = Slots.IndexOfByPredicate([Item](const FCatalogSlot& Slot)
		{
			return Slot.RowName == Item->LevelId || Slot.AssetPath == Item->Asset.GetSoftObjectPath();
		});
		Item->CatalogIndex = Found;
	}
	LevelItems.Sort([](const TSharedPtr<FBoxLevelListItem>& A, const TSharedPtr<FBoxLevelListItem>& B)
	{
		const bool bA = A.IsValid() && A->CatalogIndex != INDEX_NONE;
		const bool bB = B.IsValid() && B->CatalogIndex != INDEX_NONE;
		if (bA != bB)
		{
			return bA;
		}
		if (bA && bB && A->CatalogIndex != B->CatalogIndex)
		{
			return A->CatalogIndex < B->CatalogIndex;
		}
		const FString NameA = A.IsValid() ? A->DisplayName.ToString() : FString();
		const FString NameB = B.IsValid() ? B->DisplayName.ToString() : FString();
		return NameA < NameB;
	});
}

void FBoxLevelEditor::MoveLevel(int32 Direction)
{
	if (bPlaying || !CurrentLevel || Direction == 0)
	{
		return;
	}
	UDataTable* Catalog = LoadCatalog();
	if (!Catalog)
	{
		return;
	}
	struct FCatalogSlot
	{
		FName RowName;
		int32 SortOrder = 0;
		FSoftObjectPath AssetPath;
	};
	TArray<FCatalogSlot> Slots;
	const FSoftObjectPath CurrentPath(CurrentLevel);
	Catalog->ForeachRow<FLevelCatalogRow>(TEXT("BoxMoveLevel"), [&Slots](const FName& RowName, const FLevelCatalogRow& Row)
	{
		FCatalogSlot Slot;
		Slot.RowName = RowName;
		Slot.SortOrder = Row.SortOrder;
		Slot.AssetPath = Row.LevelAsset.ToSoftObjectPath();
		Slots.Add(Slot);
	});
	Slots.Sort([](const FCatalogSlot& A, const FCatalogSlot& B)
	{
		if (A.SortOrder != B.SortOrder)
		{
			return A.SortOrder < B.SortOrder;
		}
		return A.RowName.LexicalLess(B.RowName);
	});
	const int32 Index = Slots.IndexOfByPredicate([this, CurrentPath](const FCatalogSlot& Slot)
	{
		return Slot.RowName == CurrentLevel->LevelId || Slot.AssetPath == CurrentPath;
	});
	const int32 Next = Index + Direction;
	if (Index == INDEX_NONE || Next < 0 || Next >= Slots.Num())
	{
		return;
	}
	Slots.Swap(Index, Next);
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (FLevelCatalogRow* Row = Catalog->FindRow<FLevelCatalogRow>(Slots[SlotIndex].RowName, TEXT("BoxMoveLevelWrite")))
		{
			Row->SortOrder = SlotIndex;
		}
	}
	Catalog->MarkPackageDirty();
	RebuildLevelList();
	Status = Direction < 0 ? TEXT("已上移") : TEXT("已下移");
	NotifyChanged();
}

void FBoxLevelEditor::ReloadFromAssets()
{
	if (bPlaying)
	{
		return;
	}
	RebuildPalette();
	RebuildIssues();
	RefreshPreview();
	Status = TEXT("已刷新数据和预览");
	NotifyChanged();
}

void FBoxLevelEditor::RebuildPalette()
{
	AllAssets.Reset();

	FBoxPaletteAsset PlayerAsset = MakeTool(TEXT("player"), UBoxTypeDisplayLibrary::Character(), TEXT("玩家"), FLinearColor::White, EBoxLevelBrush::Player);
	if (UPlayerDef* Player = LoadObject<UPlayerDef>(nullptr, *BoxAssetPaths::PlayerDef()))
	{
		PlayerAsset.Color = Player->PaletteColor;
		if (!Player->Type.IsValid())
		{
			PlayerAsset.TypeTag = UBoxTypeDisplayLibrary::Character();
		}
		else
		{
			PlayerAsset.TypeTag = Player->Type;
		}
		PlayerAsset.Type = UBoxTypeDisplayLibrary::GetDisplayName(PlayerAsset.TypeTag);
		if (!Player->DisplayName.IsEmpty())
		{
			PlayerAsset.Name = Player->DisplayName;
		}
	}
	AllAssets.Add(PlayerAsset);

	auto AddTerrain = [this](UTerrainDef* Def)
	{
		if (!Def)
		{
			return;
		}
		FBoxPaletteAsset Entry;
		Entry.Id = Def->TerrainId.IsNone() ? Def->GetFName() : Def->TerrainId;
		Entry.TypeTag = Def->Type.IsValid() ? Def->Type : UBoxTypeDisplayLibrary::Terrain();
		Entry.Name = Def->DisplayName.IsEmpty() ? FText::FromName(Entry.Id) : Def->DisplayName;
		Entry.Type = UBoxTypeDisplayLibrary::GetDisplayName(Entry.TypeTag);
		Entry.Color = Def->PaletteColor;
		Entry.Terrain = Def;
		switch (Def->Cell)
		{
		case ETerrainCell::Wall:
			Entry.Brush = EBoxLevelBrush::Wall;
			break;
		case ETerrainCell::Empty:
			Entry.Brush = EBoxLevelBrush::Empty;
			break;
		default:
			Entry.Brush = EBoxLevelBrush::Floor;
			break;
		}
		AllAssets.Add(Entry);
	};

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> TerrainAssets;
	Registry.GetAssetsByClass(UTerrainDef::StaticClass()->GetClassPathName(), TerrainAssets);
	for (const FAssetData& Asset : TerrainAssets)
	{
		AddTerrain(Cast<UTerrainDef>(Asset.GetAsset()));
	}
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UInteractableDef::StaticClass()->GetClassPathName(), Assets);
	for (const FAssetData& Asset : Assets)
	{
		UInteractableDef* Def = Cast<UInteractableDef>(Asset.GetAsset());
		if (!Def)
		{
			continue;
		}
		FBoxPaletteAsset Entry;
		Entry.Id = Def->DefinitionId.IsNone() ? Asset.AssetName : Def->DefinitionId;
		Entry.TypeTag = Def->Type.IsValid() ? Def->Type : UBoxTypeDisplayLibrary::InferInteractableType(Entry.Id);
		Entry.Name = Def->DisplayName.IsEmpty() ? FText::FromName(Entry.Id) : Def->DisplayName;
		Entry.Type = UBoxTypeDisplayLibrary::GetDisplayName(Entry.TypeTag);
		Entry.Brush = EBoxLevelBrush::Interactable;
		Entry.Definition = Def;
		Entry.Color = Def->PaletteColor;
		AllAssets.Add(Entry);
	}

	for (FBoxPaletteAsset& Asset : AllAssets)
	{
		AssignPaletteIcon(Asset);
	}
	if (!FindAsset(BrushId) && AllAssets.Num() > 0)
	{
		BrushId = AllAssets[0].Id;
	}

	Folders.Reset();
	FolderRoots.Reset();
	TSharedPtr<FBoxPaletteFolder> Root = MakeShared<FBoxPaletteFolder>();
	Root->Label = FText::FromString(TEXT("游戏"));
	Root->Path = TEXT("游戏");
	Root->Depth = 0;
	Folders.Add(Root);
	FolderRoots.Add(Root);

	TMap<FGameplayTag, TSharedPtr<FBoxPaletteFolder>> FoldersByTag;
	FoldersByTag.Add(FGameplayTag(), Root);

	TSet<FGameplayTag> UsedTags;
	for (const FBoxPaletteAsset& Asset : AllAssets)
	{
		TArray<FGameplayTag> Chain;
		UBoxTypeDisplayLibrary::CollectPaletteChain(Asset.TypeTag, Chain);
		for (const FGameplayTag& Tag : Chain)
		{
			if (Tag.ToString().StartsWith(TEXT("Type.Tool")))
			{
				continue;
			}
			UsedTags.Add(Tag);
		}
	}

	TArray<FGameplayTag> SortedTags = UsedTags.Array();
	SortedTags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		auto Rank = [](const FGameplayTag& Tag)
		{
			const FString Name = Tag.ToString();
			if (Name.StartsWith(TEXT("Type.Terrain")))
			{
				return 0;
			}
			if (Name.StartsWith(TEXT("Type.Character")))
			{
				return 1;
			}
			if (Name.StartsWith(TEXT("Type.Interactable")))
			{
				return 2;
			}
			return 3;
		};
		const int32 RankA = Rank(A);
		const int32 RankB = Rank(B);
		if (RankA != RankB)
		{
			return RankA < RankB;
		}
		return A.ToString() < B.ToString();
	});

	for (const FGameplayTag& Tag : SortedTags)
	{
		TArray<FGameplayTag> Chain;
		UBoxTypeDisplayLibrary::CollectPaletteChain(Tag, Chain);
		TArray<FString> Parts;
		Parts.Add(TEXT("游戏"));
		for (const FGameplayTag& Step : Chain)
		{
			Parts.Add(UBoxTypeDisplayLibrary::GetDisplayName(Step).ToString());
		}
		TSharedPtr<FBoxPaletteFolder> Folder = MakeShared<FBoxPaletteFolder>();
		Folder->Tag = Tag;
		Folder->Label = UBoxTypeDisplayLibrary::GetDisplayName(Tag);
		Folder->Path = FString::Join(Parts, TEXT(" / "));
		Folder->Depth = Chain.Num();
		Folders.Add(Folder);
		FoldersByTag.Add(Tag, Folder);

		FGameplayTag ParentTag;
		if (Chain.Num() >= 2)
		{
			ParentTag = Chain[Chain.Num() - 2];
		}
		if (const TSharedPtr<FBoxPaletteFolder>* Parent = FoldersByTag.Find(ParentTag))
		{
			(*Parent)->Children.Add(Folder);
		}
		else
		{
			Root->Children.Add(Folder);
		}
	}

	if (FolderTag.IsValid())
	{
		bool bFound = false;
		for (const TSharedPtr<FBoxPaletteFolder>& Folder : Folders)
		{
			if (Folder && Folder->Tag == FolderTag)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			FolderTag = FGameplayTag();
		}
	}
	RebuildVisibleAssets();
}

void FBoxLevelEditor::RebuildVisibleAssets()
{
	VisibleAssets.Reset();
	const FString Filter = AssetFilter.ToLower();
	for (const FBoxPaletteAsset& Asset : AllAssets)
	{
		if (!UBoxTypeDisplayLibrary::MatchesType(Asset.TypeTag, FolderTag))
		{
			continue;
		}
		const FString Hay = (Asset.Id.ToString() + TEXT(" ") + Asset.Name.ToString() + TEXT(" ") + Asset.Type.ToString()).ToLower();
		if (!Filter.IsEmpty() && !Hay.Contains(Filter))
		{
			continue;
		}
		VisibleAssets.Add(MakeShared<FBoxPaletteAsset>(Asset));
	}
}

FBoxPaletteAsset* FBoxLevelEditor::FindAsset(FName Id)
{
	return AllAssets.FindByPredicate([Id](const FBoxPaletteAsset& Asset) { return Asset.Id == Id; });
}

void FBoxLevelEditor::SetLevelFilter(const FString& Filter)
{
	LevelFilter = Filter;
	RebuildLevelList();
	NotifyChanged();
}

void FBoxLevelEditor::SetAssetFilter(const FString& Filter)
{
	AssetFilter = Filter;
	RebuildVisibleAssets();
	NotifyChanged();
}

void FBoxLevelEditor::SelectFolder(FGameplayTag InFolderTag)
{
	if (FolderTag == InFolderTag)
	{
		return;
	}
	FolderTag = InFolderTag;
	RebuildVisibleAssets();
	NotifyChanged();
}

void FBoxLevelEditor::SelectBrush(FName InBrushId)
{
	if (bPlaying)
	{
		return;
	}
	BrushId = InBrushId;
	if (const FBoxPaletteAsset* Asset = FindAsset(InBrushId))
	{
		Status = FString::Printf(TEXT("笔刷：%s"), *Asset->Name.ToString());
	}
	NotifyChanged();
}

void FBoxLevelEditor::ResetSelection()
{
	SelectionKind = EBoxSceneSelection::None;
	SelectedInstanceId = NAME_None;
}

void FBoxLevelEditor::PruneSelection()
{
	if (SelectionKind == EBoxSceneSelection::Instance && !GetSelectedInstance())
	{
		ResetSelection();
	}
}

const FBoxLevelInstance* FBoxLevelEditor::GetSelectedInstance() const
{
	if (!CurrentLevel || SelectionKind != EBoxSceneSelection::Instance)
	{
		return nullptr;
	}
	return CurrentLevel->Instances.FindByPredicate([this](const FBoxLevelInstance& Inst)
	{
		return Inst.InstanceId == SelectedInstanceId;
	});
}

FBoxLevelInstance* FBoxLevelEditor::FindSelectedInstance()
{
	return const_cast<FBoxLevelInstance*>(GetSelectedInstance());
}

bool FBoxLevelEditor::GetSelectionCell(FIntPoint& OutCell) const
{
	if (SelectionKind == EBoxSceneSelection::Player && CurrentLevel)
	{
		OutCell = CurrentLevel->PlayerSpawn;
		return true;
	}
	if (const FBoxLevelInstance* Inst = GetSelectedInstance())
	{
		OutCell = Inst->Cell;
		return true;
	}
	return false;
}

FText FBoxLevelEditor::TitleForInstance(const FBoxLevelInstance& Inst) const
{
	if (const UInteractableDef* Def = Inst.LoadDefinition())
	{
		if (!Def->DisplayName.IsEmpty())
		{
			return Def->DisplayName;
		}
	}
	return FText::FromName(Inst.GetResolvedDefinitionId());
}

FText FBoxLevelEditor::GetSelectedTitle() const
{
	if (SelectionKind == EBoxSceneSelection::Player)
	{
		return LOCTEXT("PlayerObj", "玩家");
	}
	const FBoxLevelInstance* Inst = GetSelectedInstance();
	return Inst ? TitleForInstance(*Inst) : FText::GetEmpty();
}

void FBoxLevelEditor::GetCellPicks(FIntPoint Cell, TArray<FBoxCellPick>& Out) const
{
	Out.Reset();
	if (!CurrentLevel || !CurrentLevel->IsInside(Cell))
	{
		return;
	}
	for (const FBoxLevelInstance& Inst : CurrentLevel->Instances)
	{
		if (Inst.Cell != Cell)
		{
			continue;
		}
		FBoxCellPick Pick;
		Pick.Kind = EBoxSceneSelection::Instance;
		Pick.InstanceId = Inst.InstanceId;
		Pick.Label = TitleForInstance(Inst);
		Out.Add(Pick);
	}
	if (CurrentLevel->PlayerSpawn == Cell)
	{
		FBoxCellPick Pick;
		Pick.Kind = EBoxSceneSelection::Player;
		Pick.Label = LOCTEXT("PlayerObj", "玩家");
		Out.Add(Pick);
	}
}

void FBoxLevelEditor::SelectPick(EBoxSceneSelection Kind, FName InstanceId)
{
	if (bPlaying || Kind == EBoxSceneSelection::None)
	{
		return;
	}
	SelectionKind = Kind;
	SelectedInstanceId = Kind == EBoxSceneSelection::Instance ? InstanceId : NAME_None;
	Status = FString::Printf(TEXT("已选中 %s"), *GetSelectedTitle().ToString());
	NotifyChanged();
}

void FBoxLevelEditor::DeleteSelected()
{
	if (!CurrentLevel || bPlaying)
	{
		return;
	}
	if (SelectionKind == EBoxSceneSelection::Player)
	{
		Status = TEXT("出生点不能删，拖到别的地板上");
		NotifyChanged();
		return;
	}
	const FBoxLevelInstance* Inst = GetSelectedInstance();
	if (!Inst)
	{
		return;
	}
	FBoxEditScope Scope(this);
	const FName Id = Inst->InstanceId;
	const FString Title = GetSelectedTitle().ToString();
	CurrentLevel->Instances.RemoveAll([Id](const FBoxLevelInstance& Item) { return Item.InstanceId == Id; });
	ResetSelection();
	CurrentLevel->MarkPackageDirty();
	RebuildIssues();
	RefreshPreview();
	Status = FString::Printf(TEXT("已删除 %s"), *Title);
	NotifyChanged();
}

void FBoxLevelEditor::OpenSelectedDefinition()
{
	UObject* Asset = nullptr;
	if (SelectionKind == EBoxSceneSelection::Player)
	{
		Asset = LoadObject<UPlayerDef>(nullptr, *BoxAssetPaths::PlayerDef());
	}
	else if (const FBoxLevelInstance* Inst = GetSelectedInstance())
	{
		Asset = const_cast<UInteractableDef*>(Inst->LoadDefinition());
	}
	UAssetEditorSubsystem* AssetEditors = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
	if (!Asset || !AssetEditors || !AssetEditors->OpenEditorForAsset(Asset))
	{
		Status = TEXT("打不开这份定义");
		NotifyChanged();
	}
}

void FBoxLevelEditor::SelectIssue(int32 Index)
{
	if (!Issues.IsValidIndex(Index))
	{
		SelectedIssue = INDEX_NONE;
		HighlightCells.Reset();
		NotifyChanged();
		return;
	}
	if (SelectedIssue == Index)
	{
		SelectedIssue = INDEX_NONE;
		HighlightCells.Reset();
	}
	else
	{
		SelectedIssue = Index;
		HighlightCells = Issues[Index].Cells;
		Status = Issues[Index].Message.ToString();
	}
	NotifyChanged();
}

FText FBoxLevelEditor::GetSelectedLogicSummary() const
{
	const FBoxLevelInstance* Inst = GetSelectedInstance();
	return DescribeLogic(Inst ? Inst->LoadDefinition() : nullptr);
}

void FBoxLevelEditor::SetMode(EBoxEditorMode InMode)
{
	if (Mode == InMode)
	{
		return;
	}
	Mode = InMode;
	Status = Mode == EBoxEditorMode::Configure
		? TEXT("普通模式 · 左键点选，按住物体或拖坐标轴移动")
		: TEXT("笔刷模式");
	NotifyChanged();
}

void FBoxLevelEditor::ClearSelection()
{
	if (SelectionKind == EBoxSceneSelection::None)
	{
		return;
	}
	ResetSelection();
	Status = TEXT("已取消选中");
	NotifyChanged();
}

void FBoxLevelEditor::SelectAtCell(FIntPoint Cell)
{
	if (Mode != EBoxEditorMode::Configure || bPlaying || !CurrentLevel || !CurrentLevel->IsInside(Cell))
	{
		return;
	}

	struct FPick
	{
		EBoxSceneSelection Kind = EBoxSceneSelection::None;
		FName Id;
	};
	TArray<FPick> Picks;
	for (const FBoxLevelInstance& Inst : CurrentLevel->Instances)
	{
		if (Inst.Cell == Cell)
		{
			Picks.Add({EBoxSceneSelection::Instance, Inst.InstanceId});
		}
	}
	if (CurrentLevel->PlayerSpawn == Cell)
	{
		Picks.Add({EBoxSceneSelection::Player, NAME_None});
	}
	if (Picks.Num() == 0)
	{
		ResetSelection();
		bInspectLevel = false;
		Status = TEXT("这一格没有可配置物体");
		NotifyChanged();
		return;
	}

	int32 Index = 0;
	for (int32 PickIndex = 0; PickIndex < Picks.Num(); ++PickIndex)
	{
		const bool bSameKind = Picks[PickIndex].Kind == SelectionKind;
		const bool bSameInstance = SelectionKind != EBoxSceneSelection::Instance || Picks[PickIndex].Id == SelectedInstanceId;
		if (bSameKind && bSameInstance)
		{
			Index = (PickIndex + 1) % Picks.Num();
			break;
		}
	}

	bInspectLevel = false;
	SelectionKind = Picks[Index].Kind;
	SelectedInstanceId = Picks[Index].Id;
	Status = FString::Printf(TEXT("已选中 %s"), *GetSelectedTitle().ToString());
	NotifyChanged();
}

void FBoxLevelEditor::SetSelectedCell(FIntPoint Cell)
{
	if (!CurrentLevel || bPlaying || SelectionKind == EBoxSceneSelection::None)
	{
		return;
	}
	if (!CurrentLevel->ExpandTo(Cell))
	{
		Status = TEXT("单边最多 20 格");
		NotifyChanged();
		return;
	}
	FBoxEditScope Scope(this);
	if (CurrentLevel->GetCell(Cell) != ETerrainCell::Floor)
	{
		CurrentLevel->FitToContent();
		Status = TEXT("只能放到地板上");
		NotifyChanged();
		return;
	}

	if (SelectionKind == EBoxSceneSelection::Player)
	{
		if (HasBlockingAt(Cell))
		{
			CurrentLevel->FitToContent();
			Status = TEXT("玩家必须站在空地板上");
			NotifyChanged();
			return;
		}
		if (CurrentLevel->PlayerSpawn == Cell)
		{
			return;
		}
		CurrentLevel->PlayerSpawn = Cell;
	}
	else if (FBoxLevelInstance* Inst = FindSelectedInstance())
	{
		const UInteractableDef* Def = Inst->LoadDefinition();
		const bool bBlocking = Def && Def->FindLogic<UBlockingLogic>();
		if (Inst->Cell != Cell && bBlocking && (HasBlockingAt(Cell) || CurrentLevel->PlayerSpawn == Cell))
		{
			CurrentLevel->FitToContent();
			Status = TEXT("不能叠放阻挡物");
			NotifyChanged();
			return;
		}
		if (Inst->Cell == Cell)
		{
			return;
		}
		Inst->Cell = Cell;
	}
	else
	{
		CurrentLevel->FitToContent();
		return;
	}

	CurrentLevel->FitToContent();
	CurrentLevel->MarkPackageDirty();
	RebuildIssues();
	RefreshPreview();
	Status = FString::Printf(TEXT("已移动 %s"), *GetSelectedTitle().ToString());
	NotifyChanged();
}

void FBoxLevelEditor::SetSelectedYaw(int32 YawSteps)
{
	FBoxLevelInstance* Inst = FindSelectedInstance();
	if (!Inst || !CurrentLevel || bPlaying)
	{
		return;
	}
	YawSteps = BoxFacing::Normalize(YawSteps);
	if (Inst->YawSteps == YawSteps)
	{
		return;
	}
	FBoxEditScope Scope(this);
	Inst->YawSteps = YawSteps;
	CurrentLevel->MarkPackageDirty();
	static const TCHAR* Names[] = {TEXT("上"), TEXT("右"), TEXT("下"), TEXT("左")};
	Status = FString::Printf(TEXT("%s 朝%s"), *GetSelectedTitle().ToString(), Names[YawSteps]);
	RefreshPreview();
	NotifyChanged();
}

void FBoxLevelEditor::SetSelectedBool(FName CompId, FName Key, bool Value)
{
	FBoxLevelInstance* Inst = FindSelectedInstance();
	if (!Inst || !CurrentLevel || bPlaying)
	{
		return;
	}
	FBoxShownParam Default;
	if (!ReadOverrideDefault(Inst->LoadDefinition(), CompId, Key, Default) || Default.Kind != EBoxParamKind::Bool)
	{
		return;
	}
	const FBoxInstanceOverride* Row = Inst->ParamOverrides.FindByPredicate([CompId, Key](const FBoxInstanceOverride& Item)
	{
		return Item.CompId == CompId && Item.Key == Key && Item.Kind == EBoxParamKind::Bool;
	});
	if ((Row ? Row->BoolValue : Default.BoolValue) == Value)
	{
		return;
	}
	FBoxEditScope Scope(this);
	BoxInstanceParams::UpsertBool(Inst->ParamOverrides, CompId, Key, Value, Default.BoolValue);
	CurrentLevel->MarkPackageDirty();
	Status = FString::Printf(TEXT("%s 已改参数"), *GetSelectedTitle().ToString());
	NotifyChanged();
}

void FBoxLevelEditor::SetSelectedInt(FName CompId, FName Key, int32 Value)
{
	FBoxLevelInstance* Inst = FindSelectedInstance();
	if (!Inst || !CurrentLevel || bPlaying)
	{
		return;
	}
	FBoxShownParam Default;
	if (!ReadOverrideDefault(Inst->LoadDefinition(), CompId, Key, Default) || Default.Kind != EBoxParamKind::Int)
	{
		return;
	}
	Value = FMath::Max(Default.IntMin, Value);
	const FBoxInstanceOverride* Row = Inst->ParamOverrides.FindByPredicate([CompId, Key](const FBoxInstanceOverride& Item)
	{
		return Item.CompId == CompId && Item.Key == Key && Item.Kind == EBoxParamKind::Int;
	});
	if ((Row ? Row->IntValue : Default.IntValue) == Value)
	{
		return;
	}
	FBoxEditScope Scope(this);
	BoxInstanceParams::UpsertInt(Inst->ParamOverrides, CompId, Key, Value, Default.IntValue);
	CurrentLevel->MarkPackageDirty();
	Status = FString::Printf(TEXT("%s 已改参数"), *GetSelectedTitle().ToString());
	NotifyChanged();
}

void FBoxLevelEditor::SetSelectedName(FName CompId, FName Key, FName Value)
{
	FBoxLevelInstance* Inst = FindSelectedInstance();
	if (!Inst || !CurrentLevel || bPlaying)
	{
		return;
	}
	FBoxShownParam Default;
	if (!ReadOverrideDefault(Inst->LoadDefinition(), CompId, Key, Default) || Default.Kind != EBoxParamKind::Name)
	{
		return;
	}
	const FBoxInstanceOverride* Row = Inst->ParamOverrides.FindByPredicate([CompId, Key](const FBoxInstanceOverride& Item)
	{
		return Item.CompId == CompId && Item.Key == Key && Item.Kind == EBoxParamKind::Name;
	});
	if ((Row ? Row->NameValue : Default.NameValue) == Value)
	{
		return;
	}
	FBoxEditScope Scope(this);
	BoxInstanceParams::UpsertName(Inst->ParamOverrides, CompId, Key, Value, Default.NameValue);
	CurrentLevel->MarkPackageDirty();
	Status = FString::Printf(TEXT("%s 已改参数"), *GetSelectedTitle().ToString());
	NotifyChanged();
}

void FBoxLevelEditor::SetSelectedTag(FName CompId, FName Key, FGameplayTag Value)
{
	FBoxLevelInstance* Inst = FindSelectedInstance();
	if (!Inst || !CurrentLevel || bPlaying)
	{
		return;
	}
	FBoxShownParam Default;
	if (!ReadOverrideDefault(Inst->LoadDefinition(), CompId, Key, Default) || Default.Kind != EBoxParamKind::Tag)
	{
		return;
	}
	const FBoxInstanceOverride* Row = Inst->ParamOverrides.FindByPredicate([CompId, Key](const FBoxInstanceOverride& Item)
	{
		return Item.CompId == CompId && Item.Key == Key && Item.Kind == EBoxParamKind::Tag;
	});
	if ((Row ? Row->TagValue : Default.TagValue) == Value)
	{
		return;
	}
	FBoxEditScope Scope(this);
	BoxInstanceParams::UpsertTag(Inst->ParamOverrides, CompId, Key, Value, Default.TagValue);
	CurrentLevel->MarkPackageDirty();
	Status = FString::Printf(TEXT("%s 已改参数"), *GetSelectedTitle().ToString());
	NotifyChanged();
}

void FBoxLevelEditor::SelectLevel(ULevelData* Level)
{
	bInspectLevel = true;
	if (CurrentLevel == Level)
	{
		NotifyChanged();
		return;
	}
	if (bPlaying)
	{
		StopPlay();
	}
	ResetSelection();
	ClearEditHistory();
	CurrentLevel = Level;
	RebuildIssues();
	RefreshPreview();
	FrameCamera();
	NotifyChanged();
}

void FBoxLevelEditor::EnsurePlayer()
{
	UWorld* World = PreviewScene ? PreviewScene->GetWorld() : nullptr;
	if (!World || !MatchWorld)
	{
		return;
	}
	UPlayerDef* Def = UPlayerDef::LoadOfficial();
	if (!PreviewPlayer || !IsValid(PreviewPlayer))
	{
		UClass* PawnClass = Def && Def->PawnClass ? Def->PawnClass.Get() : ABoxPlayerCharacter::StaticClass();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PreviewPlayer = World->SpawnActor<ABoxPlayerCharacter>(PawnClass, MatchWorld->GetPlayerSpawnTransform(), Params);
	}
	if (PreviewPlayer && Def)
	{
		PreviewPlayer->ApplyPlayerDef(Def);
	}
	MatchWorld->BindPlayer(PreviewPlayer);
}

void FBoxLevelEditor::RefreshPreview()
{
	UWorld* World = PreviewScene ? PreviewScene->GetWorld() : nullptr;
	if (!World || !CurrentLevel)
	{
		return;
	}
	CurrentLevel->EnsureCellsSize();
	if (!MatchWorld || !IsValid(MatchWorld))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		MatchWorld = World->SpawnActor<ABoxMatchWorld>(ABoxMatchWorld::StaticClass(), FTransform::Identity, Params);
	}
	MatchWorld->StartLevel(CurrentLevel);
	EnsurePlayer();
}

void FBoxLevelEditor::FrameCamera()
{
	if (Viewport)
	{
		Viewport->FrameBoard();
	}
}

bool FBoxLevelEditor::HitCell(const FVector& World, FIntPoint& OutCell) const
{
	OutCell = FIntPoint(
		FMath::FloorToInt(World.X / BoxGrid::CellSize()),
		FMath::FloorToInt(World.Y / BoxGrid::CellSize()));
	return CurrentLevel && CurrentLevel->IsInside(OutCell);
}

FName FBoxLevelEditor::NextLevelId() const
{
	TSet<FName> Used;
	for (const TSharedPtr<FBoxLevelListItem>& Item : LevelItems)
	{
		if (Item && !Item->LevelId.IsNone())
		{
			Used.Add(Item->LevelId);
		}
	}
	int32 Index = 1;
	while (Used.Contains(*FString::Printf(TEXT("LV_%02d"), Index)))
	{
		++Index;
	}
	return *FString::Printf(TEXT("LV_%02d"), Index);
}

FName FBoxLevelEditor::NextInstanceId(FName DefId) const
{
	if (!CurrentLevel)
	{
		return DefId;
	}
	TSet<FName> Used;
	for (const FBoxLevelInstance& Inst : CurrentLevel->Instances)
	{
		Used.Add(Inst.InstanceId);
	}
	int32 Index = 0;
	FName Candidate = *FString::Printf(TEXT("%s_%d"), *DefId.ToString(), Index);
	while (Used.Contains(Candidate))
	{
		++Index;
		Candidate = *FString::Printf(TEXT("%s_%d"), *DefId.ToString(), Index);
	}
	return Candidate;
}

void FBoxLevelEditor::ApplyNewCatalogRow(ULevelData* Level)
{
	UDataTable* Catalog = LoadCatalog();
	if (!Catalog || !Level)
	{
		return;
	}
	int32 MaxSort = 0;
	Catalog->ForeachRow<FLevelCatalogRow>(TEXT("BoxLevelEditor"), [&MaxSort](const FName&, const FLevelCatalogRow& Row)
	{
		MaxSort = FMath::Max(MaxSort, Row.SortOrder);
	});
	FLevelCatalogRow Row;
	Row.LevelId = Level->LevelId;
	Row.LevelAsset = Level;
	Row.SortOrder = MaxSort + 1;
	Row.bListed = false;
	Catalog->AddRow(Level->LevelId, Row);
	Catalog->MarkPackageDirty();
}

namespace
{
	bool IsLevelIdToken(const FString& Id)
	{
		if (Id.IsEmpty())
		{
			return false;
		}
		for (const TCHAR Char : Id)
		{
			const bool bOk = (Char >= TEXT('A') && Char <= TEXT('Z'))
				|| (Char >= TEXT('a') && Char <= TEXT('z'))
				|| (Char >= TEXT('0') && Char <= TEXT('9'))
				|| Char == TEXT('_');
			if (!bOk)
			{
				return false;
			}
		}
		return true;
	}

	FText LevelIdError(const FString& Id, const TSet<FName>& Used)
	{
		if (Id.IsEmpty())
		{
			return LOCTEXT("NeedId", "填写关卡 ID");
		}
		if (!IsLevelIdToken(Id))
		{
			return LOCTEXT("BadId", "只能用字母、数字和下划线");
		}
		if (Used.Contains(*Id))
		{
			return LOCTEXT("DupId", "这个 ID 已经有了");
		}
		return FText::GetEmpty();
	}

	void CollectTakenLevelIds(const UDataTable* Catalog, TSet<FName>& Out)
	{
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(ULevelData::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			FString IdTag;
			if (Asset.GetTagValue(GET_MEMBER_NAME_CHECKED(ULevelData, LevelId), IdTag) && !IdTag.IsEmpty())
			{
				Out.Add(*IdTag);
			}
			const FString Name = Asset.AssetName.ToString();
			if (Name.StartsWith(TEXT("DA_")))
			{
				Out.Add(*Name.RightChop(3));
			}
		}
		if (Catalog)
		{
			Catalog->ForeachRow<FLevelCatalogRow>(TEXT("BoxNewLevelIds"), [&Out](const FName& RowName, const FLevelCatalogRow& Row)
			{
				if (!Row.LevelId.IsNone())
				{
					Out.Add(Row.LevelId);
				}
				if (!RowName.IsNone())
				{
					Out.Add(RowName);
				}
			});
		}
	}

	class SBoxNewLevelDialog : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SBoxNewLevelDialog) {}
			SLATE_ARGUMENT(FString, SuggestedId)
			SLATE_ARGUMENT(TSet<FName>, UsedIds)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			UsedIds = InArgs._UsedIds;
			LevelId = InArgs._SuggestedId;

			auto Label = [](const FText& Text)
			{
				return SNew(SBox).WidthOverride(72)
					[
						SNew(STextBlock)
						.Text(Text)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.58f, 0.58f, 0.58f)))
					];
			};

			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(12)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NewLevelTitle", "新建关卡"))
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 15))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(LOCTEXT("NewName", "显示名"))]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SEditableTextBox)
							.HintText(LOCTEXT("NewNameHint", "选关和标题上显示的名字"))
							.OnTextChanged_Lambda([this](const FText& Text) { DisplayName = Text.ToString(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(LOCTEXT("NewId", "关卡 ID"))]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SEditableTextBox)
							.Text(FText::FromString(LevelId))
							.OnTextChanged_Lambda([this](const FText& Text) { LevelId = Text.ToString().TrimStartAndEnd(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(72, 2, 0, 4)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 8))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.46f, 0.46f, 0.46f)))
						.Text_Lambda([this]
						{
							const FText Error = LevelIdError(LevelId, UsedIds);
							if (!Error.IsEmpty())
							{
								return Error;
							}
							return FText::Format(LOCTEXT("AssetNameHint", "资产名 DA_{0}，创建后不再改"), FText::FromString(LevelId));
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 2)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NewNote", "设计备注"))
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.58f, 0.58f, 0.58f)))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).HeightOverride(72)
						[
							SNew(SMultiLineEditableTextBox)
							.HintText(LOCTEXT("NewNoteHint", "可以空着"))
							.OnTextChanged_Lambda([this](const FText& Text) { DesignerNote = Text.ToString(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("CancelNew", "取消"))
							.OnClicked_Lambda([this]
							{
								Close();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("ConfirmNew", "创建"))
							.IsEnabled_Lambda([this]
							{
								return !DisplayName.TrimStartAndEnd().IsEmpty() && LevelIdError(LevelId, UsedIds).IsEmpty();
							})
							.OnClicked_Lambda([this]
							{
								bAccepted = true;
								DisplayName = DisplayName.TrimStartAndEnd();
								Close();
								return FReply::Handled();
							})
						]
					]
				]
			];
		}

		void SetWindow(TSharedRef<SWindow> InWindow) { Window = InWindow; }
		bool WasAccepted() const { return bAccepted; }
		FString GetDisplayName() const { return DisplayName; }
		FString GetLevelId() const { return LevelId; }
		FString GetDesignerNote() const { return DesignerNote; }

	private:
		void Close()
		{
			if (TSharedPtr<SWindow> Pinned = Window.Pin())
			{
				Pinned->RequestDestroyWindow();
			}
		}

		TWeakPtr<SWindow> Window;
		TSet<FName> UsedIds;
		FString DisplayName;
		FString LevelId;
		FString DesignerNote;
		bool bAccepted = false;
	};
}

void FBoxLevelEditor::NewLevel()
{
	if (bPlaying)
	{
		return;
	}
	TSet<FName> UsedIds;
	CollectTakenLevelIds(LoadCatalog(), UsedIds);
	const FString SuggestedId = NextLevelId().ToString();

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("NewLevelWindow", "新建关卡"))
		.ClientSize(FVector2D(420, 340))
		.SupportsMinimize(false)
		.SupportsMaximize(false);
	TSharedRef<SBoxNewLevelDialog> Dialog = SNew(SBoxNewLevelDialog)
		.SuggestedId(SuggestedId)
		.UsedIds(UsedIds);
	Dialog->SetWindow(Window);
	Window->SetContent(Dialog);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
	if (!Dialog->WasAccepted())
	{
		return;
	}

	const FName Id(*Dialog->GetLevelId());
	const FString AssetName = FString::Printf(TEXT("DA_%s"), *Id.ToString());
	const FString PackageName = BoxAssetPaths::LevelDirectory() + TEXT("/") + AssetName;
	UPackage* Package = CreatePackage(*PackageName);
	ULevelData* Level = NewObject<ULevelData>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	Level->ApplyNewLevelDefaults(Id);
	Level->DisplayName = FText::FromString(Dialog->GetDisplayName());
	Level->DesignerNote = Dialog->GetDesignerNote();
	FAssetRegistryModule::AssetCreated(Level);
	Package->MarkPackageDirty();
	ApplyNewCatalogRow(Level);
	RebuildLevelList();
	SelectLevel(Level);
	Status = FString::Printf(TEXT("已创建 %s"), *AssetName);
	NotifyChanged();
}

void FBoxLevelEditor::SaveLevel()
{
	TArray<UPackage*> Packages;
	if (CurrentLevel)
	{
		Packages.Add(CurrentLevel->GetOutermost());
	}
	if (UDataTable* Catalog = LoadCatalog())
	{
		Packages.Add(Catalog->GetOutermost());
	}
	if (Packages.Num() > 0)
	{
		FEditorFileUtils::PromptForCheckoutAndSave(Packages, false, false);
		Status = TEXT("已保存");
		NotifyChanged();
	}
}

void FBoxLevelEditor::DeleteLevel()
{
	if (!CurrentLevel || bPlaying)
	{
		return;
	}
	UDataTable* Catalog = LoadCatalog();
	if (Catalog)
	{
		Catalog->RemoveRow(CurrentLevel->LevelId);
		Catalog->MarkPackageDirty();
	}
	TArray<UObject*> ToDelete;
	ToDelete.Add(CurrentLevel);
	ObjectTools::DeleteObjects(ToDelete, true);
	CurrentLevel = nullptr;
	ResetSelection();
	ClearEditHistory();
	RebuildLevelList();
	if (LevelItems.Num() > 0)
	{
		SelectLevel(LevelItems[0]->Resolve());
	}
	else
	{
		RefreshPreview();
		NotifyChanged();
	}
}

void FBoxLevelEditor::RebuildIssues()
{
	Issues.Reset();
	HighlightCells.Reset();
	SelectedIssue = INDEX_NONE;
	bHasErrors = false;
	if (!CurrentLevel)
	{
		return;
	}
	ULevelCatalogLibrary::ValidateLevelForListing(CurrentLevel, LoadCatalog(), Issues);
	for (const FLevelValidationIssue& Issue : Issues)
	{
		bHasErrors = bHasErrors || Issue.bError;
	}
}

void FBoxLevelEditor::ValidateLevel()
{
	RebuildIssues();
	Status = bHasErrors ? TEXT("校验失败") : (Issues.Num() == 0 ? TEXT("校验通过") : TEXT("校验通过，有警告"));
	NotifyChanged();
}

void FBoxLevelEditor::SetListed(bool bListed)
{
	if (!CurrentLevel || bPlaying)
	{
		return;
	}
	FBoxEditScope Scope(this);
	if (bListed)
	{
		RebuildIssues();
		if (bHasErrors)
		{
			Status = TEXT("有错误，不能上架");
			NotifyChanged();
			return;
		}
	}
	UDataTable* Catalog = LoadCatalog();
	if (!Catalog)
	{
		return;
	}
	if (FLevelCatalogRow* Row = Catalog->FindRow<FLevelCatalogRow>(CurrentLevel->LevelId, TEXT("BoxLevelEditorList")))
	{
		Row->bListed = bListed;
		Catalog->MarkPackageDirty();
	}
	else if (bListed)
	{
		ApplyNewCatalogRow(CurrentLevel);
		if (FLevelCatalogRow* Created = Catalog->FindRow<FLevelCatalogRow>(CurrentLevel->LevelId, TEXT("BoxLevelEditorList")))
		{
			Created->bListed = true;
		}
	}
	RebuildLevelList();
	Status = bListed ? TEXT("已上架") : TEXT("未上架");
	NotifyChanged();
}

void FBoxLevelEditor::SetDisplayName(const FString& Name)
{
	if (!CurrentLevel || bPlaying)
	{
		return;
	}
	FBoxEditScope Scope(this);
	CurrentLevel->DisplayName = FText::FromString(Name.IsEmpty() ? TEXT("未命名关卡") : Name);
	CurrentLevel->MarkPackageDirty();
	RebuildLevelList();
	NotifyChanged();
}

void FBoxLevelEditor::SetDesignerNote(const FString& Note)
{
	if (!CurrentLevel || bPlaying)
	{
		return;
	}
	FBoxEditScope Scope(this);
	CurrentLevel->DesignerNote = Note;
	CurrentLevel->MarkPackageDirty();
	RebuildIssues();
	NotifyChanged();
}

void FBoxLevelEditor::SetSize(int32 Width, int32 Height)
{
	if (!CurrentLevel || bPlaying)
	{
		return;
	}
	Width = FMath::Clamp(Width, ULevelData::MinSize, ULevelData::MaxSize);
	Height = FMath::Clamp(Height, ULevelData::MinSize, ULevelData::MaxSize);
	if (Width == CurrentLevel->Width && Height == CurrentLevel->Height)
	{
		return;
	}
	if (CurrentLevel->ResizeWouldCrop(Width, Height))
	{
		const EAppReturnType::Type Answer = FMessageDialog::Open(
			EAppMsgType::YesNo,
			LOCTEXT("Crop", "缩小网格会裁掉玩家或实例。继续吗？"));
		if (Answer != EAppReturnType::Yes)
		{
			NotifyChanged();
			return;
		}
	}
	FBoxEditScope Scope(this);
	CurrentLevel->ResizeGrid(Width, Height);
	CurrentLevel->MarkPackageDirty();
	PruneSelection();
	RebuildIssues();
	RefreshPreview();
	FrameCamera();
	NotifyChanged();
}

void FBoxLevelEditor::RemoveInstancesAt(FIntPoint Cell)
{
	if (!CurrentLevel)
	{
		return;
	}
	CurrentLevel->Instances.RemoveAll([Cell](const FBoxLevelInstance& Inst) { return Inst.Cell == Cell; });
}

bool FBoxLevelEditor::HasBlockingAt(FIntPoint Cell) const
{
	if (!CurrentLevel)
	{
		return false;
	}
	for (const FBoxLevelInstance& Inst : CurrentLevel->Instances)
	{
		if (Inst.Cell != Cell)
		{
			continue;
		}
		if (const UInteractableDef* Def = Inst.LoadDefinition())
		{
			if (Def->FindLogic<UBlockingLogic>())
			{
				return true;
			}
		}
	}
	return false;
}

void FBoxLevelEditor::PaintCell(FIntPoint Cell, bool bErase)
{
	if (!CurrentLevel || bPlaying)
	{
		return;
	}
	if (!CurrentLevel->ExpandTo(Cell))
	{
		Status = TEXT("单边最多 20 格");
		NotifyChanged();
		return;
	}
	FBoxEditScope Scope(this);

	FName Tool = bErase ? FName(TEXT("eraser")) : BrushId;
	const FBoxPaletteAsset* Asset = FindAsset(Tool);
	EBoxLevelBrush Brush = Asset ? Asset->Brush : EBoxLevelBrush::Floor;
	if (bErase)
	{
		Brush = EBoxLevelBrush::Eraser;
	}

	switch (Brush)
	{
	case EBoxLevelBrush::Floor:
		CurrentLevel->SetCell(Cell, ETerrainCell::Floor);
		break;
	case EBoxLevelBrush::Wall:
		CurrentLevel->SetCell(Cell, ETerrainCell::Wall);
		RemoveInstancesAt(Cell);
		break;
	case EBoxLevelBrush::Empty:
		CurrentLevel->SetCell(Cell, ETerrainCell::Empty);
		RemoveInstancesAt(Cell);
		break;
	case EBoxLevelBrush::Eraser:
	{
		const int32 Removed = CurrentLevel->Instances.RemoveAll([Cell](const FBoxLevelInstance& Inst)
		{
			return Inst.Cell == Cell;
		});
		if (Removed == 0)
		{
			const ETerrainCell Terrain = CurrentLevel->GetCell(Cell);
			if (Terrain == ETerrainCell::Wall || Terrain == ETerrainCell::Empty)
			{
				CurrentLevel->SetCell(Cell, ETerrainCell::Floor);
			}
			else
			{
				CurrentLevel->FitToContent();
				Status = TEXT("这一格没有可擦的物体");
				NotifyChanged();
				return;
			}
		}
		break;
	}
	case EBoxLevelBrush::Player:
		if (CurrentLevel->GetCell(Cell) != ETerrainCell::Floor || HasBlockingAt(Cell))
		{
			CurrentLevel->FitToContent();
			Status = TEXT("玩家必须站在空地板上");
			NotifyChanged();
			return;
		}
		CurrentLevel->PlayerSpawn = Cell;
		break;
	case EBoxLevelBrush::Interactable:
		if (!Asset || !Asset->Definition.IsValid())
		{
			CurrentLevel->FitToContent();
			return;
		}
		if (CurrentLevel->GetCell(Cell) != ETerrainCell::Floor)
		{
			CurrentLevel->FitToContent();
			Status = TEXT("交互物必须放在地板上");
			NotifyChanged();
			return;
		}
		if (CurrentLevel->Instances.ContainsByPredicate([Cell, Asset](const FBoxLevelInstance& Inst)
			{
				return Inst.Cell == Cell && Inst.GetResolvedDefinitionId() == Asset->Id;
			}))
		{
			CurrentLevel->FitToContent();
			Status = TEXT("这一格已经有这个物体");
			NotifyChanged();
			return;
		}
		if (Asset->Definition->FindLogic<UBlockingLogic>() && (HasBlockingAt(Cell) || CurrentLevel->PlayerSpawn == Cell))
		{
			CurrentLevel->FitToContent();
			Status = TEXT("不能叠放阻挡物");
			NotifyChanged();
			return;
		}
		{
			FBoxLevelInstance Inst;
			Inst.InstanceId = NextInstanceId(Asset->Id);
			Inst.DefinitionId = Asset->Id;
			Inst.Definition = Asset->Definition.Get();
			Inst.Cell = Cell;
			CurrentLevel->Instances.Add(Inst);
		}
		break;
	}

	CurrentLevel->FitToContent();
	CurrentLevel->MarkPackageDirty();
	PruneSelection();
	RebuildIssues();
	RefreshPreview();
	NotifyChanged();
}

void FBoxLevelEditor::StartPlay()
{
	if (!CurrentLevel)
	{
		EnsureCurrentLevel();
	}
	if (!CurrentLevel)
	{
		Status = TEXT("还没有关卡，等资源扫完或新建一关再试玩");
		NotifyChanged();
		return;
	}
	RebuildIssues();
	if (bHasErrors)
	{
		Status = TEXT("有错误，不能试玩");
		NotifyChanged();
		return;
	}
	bPlaying = true;
	RefreshPreview();
	if (!MatchWorld || !PreviewPlayer)
	{
		bPlaying = false;
		Status = TEXT("试玩没刷出来，检查关卡和交互物数据");
		NotifyChanged();
		return;
	}
	Status = TEXT("试玩中 · 不写档");
	NotifyChanged();
}

void FBoxLevelEditor::StopPlay()
{
	bPlaying = false;
	HeldMoveKeys = 0;
	RefreshPreview();
	Status = TEXT("已停止试玩。关卡数据未改。");
	NotifyChanged();
}

void FBoxLevelEditor::RestartPlay()
{
	if (!bPlaying)
	{
		return;
	}
	RefreshPreview();
	Status = TEXT("已重开");
	NotifyChanged();
}

void FBoxLevelEditor::UndoPlay()
{
	if (bPlaying && MatchWorld && PreviewPlayer)
	{
		MatchWorld->RequestUndo(PreviewPlayer);
		if (MatchWorld->GetSim() && MatchWorld->GetSim()->IsWon())
		{
			Status = TEXT("通关 · 试玩不写档");
		}
		NotifyChanged();
	}
}

void FBoxLevelEditor::RedoPlay()
{
	if (bPlaying && MatchWorld && PreviewPlayer)
	{
		MatchWorld->RequestRedo(PreviewPlayer);
		NotifyChanged();
	}
}

void FBoxLevelEditor::RequestMove(FIntPoint Dir)
{
	if (!bPlaying || !MatchWorld || !PreviewPlayer)
	{
		return;
	}
	MatchWorld->EnqueueMove(PreviewPlayer, Dir);
	if (MatchWorld->GetSim() && MatchWorld->GetSim()->IsWon())
	{
		Status = TEXT("通关 · 试玩不写档");
	}
	NotifyChanged();
}

namespace
{
	struct FBoxEditorMoveKey
	{
		FKey Key;
		FIntPoint Dir;
	};

	const FBoxEditorMoveKey GEditorMoveKeys[] = {
		{EKeys::W, BoxGrid::North},
		{EKeys::Up, BoxGrid::North},
		{EKeys::S, BoxGrid::South},
		{EKeys::Down, BoxGrid::South},
		{EKeys::D, BoxGrid::West},
		{EKeys::Right, BoxGrid::West},
		{EKeys::A, BoxGrid::East},
		{EKeys::Left, BoxGrid::East},
	};

	int32 EditorMoveKeyIndex(const FKey& Key)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(GEditorMoveKeys); ++Index)
		{
			if (GEditorMoveKeys[Index].Key == Key)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

bool FBoxLevelEditor::NotePlayMoveKey(FKey Key, bool bDown)
{
	const int32 Index = EditorMoveKeyIndex(Key);
	if (Index == INDEX_NONE || !bPlaying || !MatchWorld || !PreviewPlayer)
	{
		return false;
	}
	const uint8 Bit = static_cast<uint8>(1 << Index);
	const uint8 Prev = HeldMoveKeys;
	if (bDown)
	{
		HeldMoveKeys |= Bit;
	}
	else
	{
		HeldMoveKeys &= static_cast<uint8>(~Bit);
	}
	PushHeldMove();
	if (bDown && Prev == 0)
	{
		MatchWorld->ArmHoldRepeat();
		RequestMove(GEditorMoveKeys[Index].Dir);
	}
	return true;
}

void FBoxLevelEditor::PushHeldMove()
{
	if (!MatchWorld)
	{
		return;
	}
	FIntPoint Dir = FIntPoint::ZeroValue;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(GEditorMoveKeys); ++Index)
	{
		if ((HeldMoveKeys & static_cast<uint8>(1 << Index)) == 0)
		{
			continue;
		}
		Dir = GEditorMoveKeys[Index].Dir;
		break;
	}
	MatchWorld->SetHeldMoveDir(Dir);
}

bool FBoxLevelEditor::HandlePlayKey(FKey Key)
{
	if (!bPlaying)
	{
		return false;
	}
	if (EditorMoveKeyIndex(Key) != INDEX_NONE)
	{
		return false;
	}
	if (Key == EKeys::Z)
	{
		UndoPlay();
		return true;
	}
	if (Key == EKeys::Y)
	{
		RedoPlay();
		return true;
	}
	if (Key == EKeys::R)
	{
		RestartPlay();
		return true;
	}
	if (Key == EKeys::Escape)
	{
		StopPlay();
		return true;
	}
	return false;
}

void FBoxLevelEditor::FocusViewport()
{
	if (Viewport.IsValid())
	{
		FSlateApplication::Get().SetAllUserFocus(Viewport, EFocusCause::SetDirectly);
	}
}

TSharedPtr<SWidget> FBoxLevelEditor::GetViewportWidget() const
{
	return Viewport;
}

TSharedRef<SWidget> SBoxLevelEditor::MakeToolbar()
{
	auto ModeButton = [this](const FText& Label, EBoxEditorMode ButtonMode)
	{
		return SNew(SButton)
			.Text(Label)
			.ButtonColorAndOpacity_Lambda([this, ButtonMode]
			{
				const bool bOn = Editor && Editor->GetMode() == ButtonMode;
				return FSlateColor(bOn ? FLinearColor(0.85f, 0.62f, 0.12f) : FLinearColor(0.22f, 0.22f, 0.22f));
			})
			.OnClicked_Lambda([this, ButtonMode]
			{
				if (Editor)
				{
					Editor->SetMode(ButtonMode);
				}
				return FReply::Handled();
			});
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			ModeButton(LOCTEXT("ModeConfig", "普通模式"), EBoxEditorMode::Configure)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			ModeButton(LOCTEXT("ModePlace", "笔刷模式"), EBoxEditorMode::Place)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(8, 2)
		[
			SNew(SSeparator).Orientation(Orient_Vertical)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Play", "试玩"))
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
			.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
			.OnClicked(this, &SBoxLevelEditor::OnPlay)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Stop", "停止"))
			.IsEnabled_Lambda([this] { return Editor && Editor->IsPlaying(); })
			.OnClicked(this, &SBoxLevelEditor::OnStop)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Restart", "重开"))
			.IsEnabled_Lambda([this] { return Editor && Editor->IsPlaying(); })
			.OnClicked(this, &SBoxLevelEditor::OnRestart)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Undo", "撤销"))
			.IsEnabled_Lambda([this] { return Editor && (Editor->IsPlaying() || Editor->CanUndoEdit()); })
			.OnClicked(this, &SBoxLevelEditor::OnUndo)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Redo", "重做"))
			.IsEnabled_Lambda([this] { return Editor && (Editor->IsPlaying() || Editor->CanRedoEdit()); })
			.OnClicked(this, &SBoxLevelEditor::OnRedo)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(8, 2)
		[
			SNew(SSeparator).Orientation(Orient_Vertical)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Reload", "刷新"))
			.ToolTipText(LOCTEXT("ReloadTip", "重新读取交互物、地形和角色数据，并刷新预览"))
			.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
			.OnClicked(this, &SBoxLevelEditor::OnReload)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Validate", "校验"))
			.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
			.OnClicked(this, &SBoxLevelEditor::OnValidate)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Save", "保存"))
			.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
			.OnClicked(this, &SBoxLevelEditor::OnSave)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(LOCTEXT("Frame", "归位"))
			.ToolTipText(LOCTEXT("FrameTip", "回到对局俯视镜头"))
			.OnClicked(this, &SBoxLevelEditor::OnFrame)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8, 0)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.56f, 0.56f)))
			.Text_Lambda([this]
			{
				if (!Editor)
				{
					return FText::GetEmpty();
				}
				ULevelData* Level = Editor->GetLevel();
				if (!Level)
				{
					return FText::GetEmpty();
				}
				return Level->DisplayName.IsEmpty() ? FText::FromName(Level->LevelId) : Level->DisplayName;
			})
		];
}

TSharedRef<SWidget> SBoxLevelEditor::MakeDetails()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(12, 12, 12, 2)
			[
				DetailTitle(TAttribute<FText>::CreateLambda([this]
				{
					ULevelData* Level = Editor ? Editor->GetLevel() : nullptr;
					if (!Level)
					{
						return LOCTEXT("NoLevel", "未选关卡");
					}
					const FText Title = Level->DisplayName.IsEmpty() ? FText::FromName(Level->LevelId) : Level->DisplayName;
					return FText::Format(LOCTEXT("LevelHeader", "关卡  {0}"), Title);
				}))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailCategory(LOCTEXT("Display", "显示"), true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailRow(LOCTEXT("LevelId", "关卡 ID"),
					SNew(SEditableTextBox)
					.IsReadOnly(true)
					.Text_Lambda([this]
					{
						ULevelData* Level = Editor ? Editor->GetLevel() : nullptr;
						return Level ? FText::FromName(Level->LevelId) : FText::GetEmpty();
					}))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailRow(LOCTEXT("DisplayName", "显示名"),
					SNew(SEditableTextBox)
					.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
					.Text_Lambda([this]
					{
						ULevelData* Level = Editor ? Editor->GetLevel() : nullptr;
						return Level ? Level->DisplayName : FText::GetEmpty();
					})
					.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
					{
						if (Editor)
						{
							Editor->SetDisplayName(Text.ToString());
						}
					}))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 6, 10, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Note", "设计备注"))
					.Font(DetailLabelFont())
					.ColorAndOpacity(FSlateColor(DetailLabelColor))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 2, 10, 6)
				[
					SNew(SBox).HeightOverride(72)
					[
						SNew(SMultiLineEditableTextBox)
						.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
						.Text_Lambda([this]
						{
							ULevelData* Level = Editor ? Editor->GetLevel() : nullptr;
							return Level ? FText::FromString(Level->DesignerNote) : FText::GetEmpty();
						})
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
						{
							if (Editor)
							{
								Editor->SetDesignerNote(Text.ToString());
							}
						})
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailCategory(LOCTEXT("Rules", "规则"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailRow(LOCTEXT("Listed", "上架"),
					SNew(SCheckBox)
					.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
					.IsChecked_Lambda([this]
					{
						if (!Editor || !Editor->GetLevel())
						{
							return ECheckBoxState::Unchecked;
						}
						for (const TSharedPtr<FBoxLevelListItem>& Item : Editor->GetLevelItems())
						{
							if (Item && Item->Matches(Editor->GetLevel()))
							{
								return Item->bListed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
							}
						}
						return ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						if (Editor)
						{
							Editor->SetListed(State == ECheckBoxState::Checked);
						}
					}))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailCategory(LOCTEXT("Placement", "摆放"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailRow(LOCTEXT("Spawn", "出生点"),
					DetailValue(TAttribute<FText>::CreateLambda([this]
					{
						ULevelData* Level = Editor ? Editor->GetLevel() : nullptr;
						return Level
							? FText::FromString(FString::Printf(TEXT("(%d, %d)"), Level->PlayerSpawn.X, Level->PlayerSpawn.Y))
							: FText::GetEmpty();
					})))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DetailRow(LOCTEXT("Count", "实例数"),
					DetailValue(TAttribute<FText>::CreateLambda([this]
					{
						ULevelData* Level = Editor ? Editor->GetLevel() : nullptr;
						return FText::AsNumber(Level ? Level->Instances.Num() : 0);
					})))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10, 12, 10, 8)
			[
				SNew(SButton)
				.Text(LOCTEXT("Delete", "删除资产"))
				.IsEnabled_Lambda([this] { return Editor && Editor->GetLevel() && !Editor->IsPlaying(); })
				.OnClicked(this, &SBoxLevelEditor::OnDelete)
			]
		];
}

TSharedRef<SWidget> SBoxLevelEditor::MakeSelectionDetails()
{
	auto HasSelection = [this]
	{
		return Editor && Editor->GetSelectionKind() != EBoxSceneSelection::None;
	};
	auto IsInstance = [this]
	{
		return Editor && Editor->GetSelectionKind() == EBoxSceneSelection::Instance;
	};

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(12, 14, 12, 8)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Visibility_Lambda([HasSelection]
				{
					return HasSelection() ? EVisibility::Collapsed : EVisibility::Visible;
				})
				.Font(DetailHintFont())
				.ColorAndOpacity(FSlateColor(DetailHintColor))
				.Text(LOCTEXT("PickObject", "在视口里点一个箱子、目标、踏板或玩家。同一格叠了多个时，再点一次切换。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(12, 12, 12, 2)
			[
				SNew(SBox)
				.Visibility_Lambda([HasSelection] { return HasSelection() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					DetailTitle(TAttribute<FText>::CreateLambda([this] { return Editor ? Editor->GetSelectedTitle() : FText::GetEmpty(); }))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(StackList, SBox)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([HasSelection] { return HasSelection() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					DetailCategory(LOCTEXT("InstanceCat", "实例"), true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([IsInstance] { return IsInstance() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					DetailRow(LOCTEXT("InstId", "实例 ID"),
						SNew(SEditableTextBox)
						.IsReadOnly(true)
						.Text_Lambda([this]
						{
							const FBoxLevelInstance* Inst = Editor ? Editor->GetSelectedInstance() : nullptr;
							return Inst ? FText::FromName(Inst->InstanceId) : FText::GetEmpty();
						}))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([HasSelection] { return HasSelection() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					DetailRow(LOCTEXT("DefId", "定义"),
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							DetailValue(TAttribute<FText>::CreateLambda([this]
							{
								if (!Editor)
								{
									return FText::GetEmpty();
								}
								if (Editor->GetSelectionKind() == EBoxSceneSelection::Player)
								{
									if (const UPlayerDef* Def = LoadObject<UPlayerDef>(nullptr, *BoxAssetPaths::PlayerDef()))
									{
										if (!Def->DisplayName.IsEmpty())
										{
											return Def->DisplayName;
										}
										if (!Def->PlayerId.IsNone())
										{
											return FText::FromName(Def->PlayerId);
										}
									}
									return FText::FromString(FSoftObjectPath(BoxAssetPaths::PlayerDef()).GetAssetName());
								}
								const FBoxLevelInstance* Inst = Editor->GetSelectedInstance();
								return Inst ? FText::FromName(Inst->GetResolvedDefinitionId()) : FText::GetEmpty();
							}))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("OpenDef", "打开定义"))
							.OnClicked_Lambda([this]
							{
								if (Editor)
								{
									Editor->OpenSelectedDefinition();
								}
								return FReply::Handled();
							})
						])
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([HasSelection] { return HasSelection() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					DetailRow(LOCTEXT("CellX", "格子 X"),
						SNew(SSpinBox<int32>)
						.MinValue(0).MaxValue(19)
						.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
						.Value_Lambda([this]
						{
							FIntPoint Cell;
							return Editor && Editor->GetSelectionCell(Cell) ? Cell.X : 0;
						})
						.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type)
						{
							FIntPoint Cell;
							if (Editor && Editor->GetSelectionCell(Cell))
							{
								Cell.X = Value;
								Editor->SetSelectedCell(Cell);
							}
						}))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([HasSelection] { return HasSelection() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					DetailRow(LOCTEXT("CellY", "格子 Y"),
						SNew(SSpinBox<int32>)
						.MinValue(0).MaxValue(19)
						.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
						.Value_Lambda([this]
						{
							FIntPoint Cell;
							return Editor && Editor->GetSelectionCell(Cell) ? Cell.Y : 0;
						})
						.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type)
						{
							FIntPoint Cell;
							if (Editor && Editor->GetSelectionCell(Cell))
							{
								Cell.Y = Value;
								Editor->SetSelectedCell(Cell);
							}
						}))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([IsInstance] { return IsInstance() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 8, 10, 2)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Facing", "朝向"))
						.Font(DetailLabelFont())
						.ColorAndOpacity(FSlateColor(DetailLabelColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 2)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(1, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("FaceUp", "上"))
							.ToolTipText(LOCTEXT("FaceUpTip", "朝画面上方，和 W 同一个方向"))
							.ButtonColorAndOpacity_Lambda([this]
							{
								const FBoxLevelInstance* Inst = Editor ? Editor->GetSelectedInstance() : nullptr;
								const bool bOn = Inst && BoxFacing::Normalize(Inst->YawSteps) == 0;
								return FSlateColor(bOn ? FLinearColor(0.85f, 0.62f, 0.12f) : FLinearColor(0.22f, 0.22f, 0.22f));
							})
							.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
							.OnClicked_Lambda([this]
							{
								if (Editor) { Editor->SetSelectedYaw(0); }
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(1, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("FaceRight", "右"))
							.ToolTipText(LOCTEXT("FaceRightTip", "朝画面右方，和 D 同一个方向"))
							.ButtonColorAndOpacity_Lambda([this]
							{
								const FBoxLevelInstance* Inst = Editor ? Editor->GetSelectedInstance() : nullptr;
								const bool bOn = Inst && BoxFacing::Normalize(Inst->YawSteps) == 1;
								return FSlateColor(bOn ? FLinearColor(0.85f, 0.62f, 0.12f) : FLinearColor(0.22f, 0.22f, 0.22f));
							})
							.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
							.OnClicked_Lambda([this]
							{
								if (Editor) { Editor->SetSelectedYaw(1); }
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(1, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("FaceDown", "下"))
							.ToolTipText(LOCTEXT("FaceDownTip", "朝画面下方，和 S 同一个方向"))
							.ButtonColorAndOpacity_Lambda([this]
							{
								const FBoxLevelInstance* Inst = Editor ? Editor->GetSelectedInstance() : nullptr;
								const bool bOn = Inst && BoxFacing::Normalize(Inst->YawSteps) == 2;
								return FSlateColor(bOn ? FLinearColor(0.85f, 0.62f, 0.12f) : FLinearColor(0.22f, 0.22f, 0.22f));
							})
							.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
							.OnClicked_Lambda([this]
							{
								if (Editor) { Editor->SetSelectedYaw(2); }
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(1, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("FaceLeft", "左"))
							.ToolTipText(LOCTEXT("FaceLeftTip", "朝画面左方，和 A 同一个方向"))
							.ButtonColorAndOpacity_Lambda([this]
							{
								const FBoxLevelInstance* Inst = Editor ? Editor->GetSelectedInstance() : nullptr;
								const bool bOn = Inst && BoxFacing::Normalize(Inst->YawSteps) == 3;
								return FSlateColor(bOn ? FLinearColor(0.85f, 0.62f, 0.12f) : FLinearColor(0.22f, 0.22f, 0.22f));
							})
							.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
							.OnClicked_Lambda([this]
							{
								if (Editor) { Editor->SetSelectedYaw(3); }
								return FReply::Handled();
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 4)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.56f, 0.56f)))
						.Text(LOCTEXT("FaceHint", "按归位后的画面。转了镜头也不会改这四个方向。"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(8, 4)
					[
						SNew(SButton)
						.Text(LOCTEXT("DeleteObject", "删除此物体"))
						.IsEnabled_Lambda([this]
						{
							return Editor && !Editor->IsPlaying() && Editor->GetSelectionKind() == EBoxSceneSelection::Instance;
						})
						.OnClicked_Lambda([this]
						{
							if (Editor)
							{
								Editor->DeleteSelected();
							}
							return FReply::Handled();
						})
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([IsInstance] { return IsInstance() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					DetailCategory(LOCTEXT("InstParams", "本实例参数"))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(ParamList, SBox)
				.Visibility_Lambda([IsInstance] { return IsInstance() ? EVisibility::Visible : EVisibility::Collapsed; })
			]
		];
}

class SBoxPaletteTile : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBoxPaletteTile) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FBoxLevelEditor> InEditor, FName InBrushId)
	{
		Editor = InEditor;
		BrushId = InBrushId;
		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanUse())
		{
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanUse())
		{
			Editor.Pin()->SelectBrush(BrushId);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (!CanUse())
		{
			return FReply::Unhandled();
		}
		return FReply::Handled().BeginDragDrop(FBoxPaletteDragDrop::New(BrushId));
	}

private:
	bool CanUse() const
	{
		const TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
		return Pinned.IsValid() && !Pinned->IsPlaying();
	}

	TWeakPtr<FBoxLevelEditor> Editor;
	FName BrushId;
};

TSharedRef<SWidget> SBoxLevelEditor::MakeAssetGrid()
{
	TSharedRef<SWrapBox> Wrap = SNew(SWrapBox).UseAllottedSize(true);
	if (!Editor)
	{
		return Wrap;
	}
	for (const TSharedPtr<FBoxPaletteAsset>& Asset : Editor->GetVisibleAssets())
	{
		if (!Asset)
		{
			continue;
		}
		const FName Id = Asset->Id;
		const bool bSelected = Editor->GetBrushId() == Id;
		const TSharedPtr<FSlateBrush> Icon = Asset->Icon;
		const FLinearColor Swatch = Asset->Color;
		TSharedRef<SWidget> Thumb = SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Swatch);
		if (Icon.IsValid())
		{
			Thumb = SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					SNew(SImage).Image(Icon.Get())
				];
		}
		Wrap->AddSlot()
			.Padding(4)
			[
				SNew(SBoxPaletteTile, Editor, Id)
				[
					SNew(SBorder)
					.Padding(bSelected ? 3.f : 1.f)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(bSelected
						? FLinearColor(1.f, 0.78f, 0.08f)
						: FLinearColor(0.22f, 0.22f, 0.22f))
					[
						SNew(SBorder)
						.Padding(4)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(bSelected
							? FLinearColor(0.22f, 0.18f, 0.06f)
							: FLinearColor(0.08f, 0.08f, 0.08f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).WidthOverride(78).HeightOverride(52)
								[
									Thumb
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
							[
								SNew(STextBlock)
								.Text(Asset->Name)
								.Justification(ETextJustify::Center)
								.Font(FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9))
								.ColorAndOpacity(bSelected
									? FSlateColor(FLinearColor(1.f, 0.92f, 0.45f))
									: FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(Asset->Type)
								.Justification(ETextJustify::Center)
								.ColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.56f, 0.56f)))
							]
						]
					]
				]
			];
	}
	return Wrap;
}

TSharedRef<SWidget> SBoxLevelEditor::MakeStackList()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	if (!Editor)
	{
		return Box;
	}
	FIntPoint Cell;
	if (!Editor->GetSelectionCell(Cell))
	{
		return Box;
	}
	TArray<FBoxCellPick> Picks;
	Editor->GetCellPicks(Cell, Picks);
	if (Picks.Num() < 2)
	{
		return Box;
	}
	Box->AddSlot().AutoHeight().Padding(10, 8, 10, 2)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("Stack", "这一格"))
		.Font(DetailCategoryFont())
		.ColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.86f, 0.86f)))
	];
	for (const FBoxCellPick& Pick : Picks)
	{
		const EBoxSceneSelection Kind = Pick.Kind;
		const FName Id = Pick.InstanceId;
		const FText Label = Pick.Label;
		Box->AddSlot().AutoHeight().Padding(8, 1)
		[
			SNew(SButton)
			.Text(Label)
			.ButtonColorAndOpacity_Lambda([this, Kind, Id]
			{
				const bool bOn = Editor
					&& Editor->GetSelectionKind() == Kind
					&& (Kind != EBoxSceneSelection::Instance || (Editor->GetSelectedInstance() && Editor->GetSelectedInstance()->InstanceId == Id));
				return FSlateColor(bOn ? FLinearColor(0.85f, 0.62f, 0.12f) : FLinearColor(0.22f, 0.22f, 0.22f));
			})
			.OnClicked_Lambda([this, Kind, Id]
			{
				if (Editor)
				{
					Editor->SelectPick(Kind, Id);
				}
				return FReply::Handled();
			})
		];
	}
	return Box;
}

TSharedRef<SWidget> SBoxLevelEditor::MakeIssueList()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	if (!Editor)
	{
		return Box;
	}
	const TArray<FLevelValidationIssue>& Issues = Editor->GetIssues();
	for (int32 Index = 0; Index < Issues.Num(); ++Index)
	{
		const bool bError = Issues[Index].bError;
		const FText Message = Issues[Index].Message;
		Box->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SButton)
			.Text(Message)
			.ButtonColorAndOpacity_Lambda([this, Index, bError]
			{
				if (Editor && Editor->GetSelectedIssue() == Index)
				{
					return FSlateColor(FLinearColor(0.85f, 0.62f, 0.12f));
				}
				return FSlateColor(bError ? FLinearColor(0.55f, 0.22f, 0.18f) : FLinearColor(0.45f, 0.38f, 0.12f));
			})
			.OnClicked_Lambda([this, Index]
			{
				if (Editor)
				{
					Editor->SelectIssue(Index);
				}
				return FReply::Handled();
			})
		];
	}
	return Box;
}

TSharedRef<SWidget> SBoxLevelEditor::MakeIssueBar()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 2)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.56f, 0.56f)))
			.Text_Lambda([this]
			{
				const int32 Count = Editor ? Editor->GetVisibleAssets().Num() : 0;
				return FText::FromString(FString::Printf(TEXT("%d 项"), Count));
			})
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8, 2)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).MaxDesiredHeight(96.f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(IssueList, SBox)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity_Lambda([this]
				{
					return Editor && Editor->HasErrors()
						? FSlateColor(FLinearColor(0.90f, 0.45f, 0.40f))
						: FSlateColor(FLinearColor(0.55f, 0.75f, 0.50f));
				})
				.Text_Lambda([this]
				{
					return Editor ? FText::FromString(Editor->GetStatus()) : FText::GetEmpty();
				})
			]
		];
}

TSharedRef<ITableRow> SBoxLevelEditor::OnGenerateLevelRow(TSharedPtr<FBoxLevelListItem> Item, const TSharedRef<STableViewBase>& Owner)
{
	ULevelData* Level = Item ? Item->Level.Get() : nullptr;
	const FText Name = (Level && !Level->DisplayName.IsEmpty())
		? Level->DisplayName
		: (Item ? Item->DisplayName : FText::FromString(TEXT("未命名关卡")));
	const FText Id = Level && !Level->LevelId.IsNone()
		? FText::FromName(Level->LevelId)
		: (Item ? FText::FromName(Item->LevelId) : FText::GetEmpty());
	const FLinearColor IconColor = Item && Item->bListed
		? FLinearColor(0.34f, 0.65f, 0.29f)
		: FLinearColor(0.79f, 0.64f, 0.15f);

	return SNew(STableRow<TSharedPtr<FBoxLevelListItem>>, Owner)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 2)
			[
				SNew(SBox).WidthOverride(10).HeightOverride(10)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(IconColor)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Name)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
			[
				SNew(STextBlock).Text(Id).ColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.56f, 0.56f)))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
			[
				SNew(SHorizontalBox)
				.Visibility_Lambda([this, Item]()
				{
					if (!Editor || !Item || Item->CatalogIndex == INDEX_NONE)
					{
						return EVisibility::Collapsed;
					}
					ULevelData* Current = Editor->GetLevel();
					const bool bSelected = Current && (Item->Matches(Current) || Item->LevelId == Current->LevelId);
					return bSelected ? EVisibility::Visible : EVisibility::Collapsed;
				})
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("MoveLevelUp", "上移"))
					.IsEnabled_Lambda([this, Item]()
					{
						return Editor && !Editor->IsPlaying() && Item && Item->CatalogIndex > 0;
					})
					.OnClicked_Lambda([this]()
					{
						if (Editor)
						{
							Editor->MoveLevel(-1);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("MoveLevelDown", "下移"))
					.IsEnabled_Lambda([this, Item]()
					{
						return Editor && !Editor->IsPlaying() && Item && Item->CatalogIndex != INDEX_NONE && Item->CatalogIndex < Item->CatalogCount - 1;
					})
					.OnClicked_Lambda([this]()
					{
						if (Editor)
						{
							Editor->MoveLevel(1);
						}
						return FReply::Handled();
					})
				]
			]
		];
}

TSharedRef<ITableRow> SBoxLevelEditor::OnGenerateFolderRow(TSharedPtr<FBoxPaletteFolder> Item, const TSharedRef<STableViewBase>& Owner)
{
	const bool bHasChildren = Item && Item->Children.Num() > 0;
	return SNew(STableRow<TSharedPtr<FBoxPaletteFolder>>, Owner)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
			[
				SNew(SImage).Image(FAppStyle::GetBrush(bHasChildren
					? "ContentBrowser.AssetTreeFolderOpen"
					: "ContentBrowser.AssetTreeFolderClosed"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4, 2)
			[
				SNew(STextBlock).Text(Item ? Item->Label : FText::GetEmpty())
			]
		];
}

void SBoxLevelEditor::OnGetFolderChildren(TSharedPtr<FBoxPaletteFolder> Item, TArray<TSharedPtr<FBoxPaletteFolder>>& OutChildren)
{
	if (Item)
	{
		OutChildren = Item->Children;
	}
}

void SBoxLevelEditor::ExpandFolderTree(const TSharedPtr<FBoxPaletteFolder>& Item)
{
	if (!FolderTree || !Item)
	{
		return;
	}
	if (Item->Children.Num() > 0)
	{
		FolderTree->SetItemExpansion(Item, true);
		for (const TSharedPtr<FBoxPaletteFolder>& Child : Item->Children)
		{
			ExpandFolderTree(Child);
		}
	}
}

void SBoxLevelEditor::Construct(const FArguments& InArgs, const TSharedRef<FBoxLevelEditor>& InEditor)
{
	Editor = InEditor;
	ChangeHandle = Editor->OnChanged.AddSP(this, &SBoxLevelEditor::Refresh);

	TSharedRef<SWidget> ViewportWidget = Editor->MakeWidget();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(8, 2))
			[
				SNew(STextBlock).Text(LOCTEXT("Brand", "BoxPush  文件  编辑  窗口"))
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot().Value(0.76f).MinSize(220.f)
			[
				SNew(SSplitter)
				+ SSplitter::Slot().Value(0.20f).MinSize(160.f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBorder).BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
							[
								SNew(STextBlock).Margin(FMargin(8, 4)).Text(LOCTEXT("Levels", "关卡"))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(4)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
							[
								SNew(SButton).Text(LOCTEXT("New", "+ 新建")).OnClicked(this, &SBoxLevelEditor::OnNew)
								.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
							]
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(SSearchBox)
								.HintText(LOCTEXT("SearchLevels", "搜索关卡"))
								.OnTextChanged_Lambda([this](const FText& Text)
								{
									if (Editor)
									{
										Editor->SetLevelFilter(Text.ToString());
									}
								})
							]
						]
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SAssignNew(LevelList, SListView<TSharedPtr<FBoxLevelListItem>>)
							.ListItemsSource(&Editor->GetLevelItems())
							.OnGenerateRow(this, &SBoxLevelEditor::OnGenerateLevelRow)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FBoxLevelListItem> Item, ESelectInfo::Type SelectInfo)
							{
								if (bRefreshing || SelectInfo == ESelectInfo::Direct || !Editor || !Item)
								{
									return;
								}
								Editor->SelectLevel(Item->Resolve());
							})
							.SelectionMode(ESelectionMode::Single)
						]
					]
				]
				+ SSplitter::Slot().Value(0.58f).MinSize(240.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(2)
						[
							MakeToolbar()
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						ViewportWidget
					]
				]
				+ SSplitter::Slot().Value(0.22f).MinSize(180.f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBorder).BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
							[
								SNew(STextBlock).Margin(FMargin(8, 4)).Text_Lambda([this]
								{
								const bool bLevel = Editor && (Editor->GetMode() != EBoxEditorMode::Configure || Editor->IsInspectingLevel());
								return bLevel ? LOCTEXT("Details", "关卡") : LOCTEXT("ConfigDetails", "普通");
								})
							]
						]
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SNew(SWidgetSwitcher)
							.WidgetIndex_Lambda([this]
							{
								const bool bLevel = !Editor || Editor->GetMode() != EBoxEditorMode::Configure || Editor->IsInspectingLevel();
								return bLevel ? 0 : 1;
							})
							+ SWidgetSwitcher::Slot()
							[
								MakeDetails()
							]
							+ SWidgetSwitcher::Slot()
							[
								MakeSelectionDetails()
							]
						]
					]
				]
			]
			+ SSplitter::Slot().Value(0.24f).MinSize(140.f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
						[
							SNew(STextBlock).Margin(FMargin(8, 4)).Text(LOCTEXT("CB", "内容浏览器"))
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8, 4)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.77f, 0.28f)))
							.Text_Lambda([this] { return FText::FromString(Editor ? Editor->GetFolderPath() : TEXT("游戏")); })
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 4)
						[
							SNew(SBox).WidthOverride(220)
							[
								SNew(SSearchBox)
								.HintText(LOCTEXT("SearchAssets", "搜索资产"))
								.OnTextChanged_Lambda([this](const FText& Text)
								{
									if (Editor)
									{
										Editor->SetAssetFilter(Text.ToString());
									}
								})
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SSplitter)
						+ SSplitter::Slot().Value(0.18f).MinSize(120.f)
						[
							SAssignNew(FolderTree, STreeView<TSharedPtr<FBoxPaletteFolder>>)
							.TreeItemsSource(&Editor->GetFolderRoots())
							.OnGenerateRow(this, &SBoxLevelEditor::OnGenerateFolderRow)
							.OnGetChildren(this, &SBoxLevelEditor::OnGetFolderChildren)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FBoxPaletteFolder> Item, ESelectInfo::Type SelectInfo)
							{
								if (bRefreshing || SelectInfo == ESelectInfo::Direct || !Editor || !Item)
								{
									return;
								}
								Editor->SelectFolder(Item->Tag);
							})
							.SelectionMode(ESelectionMode::Single)
						]
						+ SSplitter::Slot().Value(0.82f).MinSize(180.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(AssetGrid, SBox)
								[
									MakeAssetGrid()
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakeIssueBar()
					]
				]
			]
		]
	];

	Refresh();
}

SBoxLevelEditor::~SBoxLevelEditor()
{
	if (Editor && ChangeHandle.IsValid())
	{
		Editor->OnChanged.Remove(ChangeHandle);
	}
}

void SBoxLevelEditor::Refresh()
{
	if (bRefreshing)
	{
		return;
	}
	TGuardValue<bool> Guard(bRefreshing, true);

	if (LevelList)
	{
		LevelList->RequestListRefresh();
		if (Editor && Editor->GetLevel())
		{
			for (const TSharedPtr<FBoxLevelListItem>& Item : Editor->GetLevelItems())
			{
				if (Item && Item->Matches(Editor->GetLevel()))
				{
					LevelList->SetSelection(Item, ESelectInfo::Direct);
					break;
				}
			}
		}
	}
	if (FolderTree)
	{
		FolderTree->RequestTreeRefresh();
		if (!bFolderExpandInitialized)
		{
			for (const TSharedPtr<FBoxPaletteFolder>& Root : Editor->GetFolderRoots())
			{
				ExpandFolderTree(Root);
			}
			bFolderExpandInitialized = true;
		}
		if (Editor)
		{
			const FGameplayTag SelectedFolder = Editor->GetFolderTag();
			for (const TSharedPtr<FBoxPaletteFolder>& Item : Editor->GetFolders())
			{
				if (Item && Item->Tag == SelectedFolder)
				{
					FolderTree->SetSelection(Item, ESelectInfo::Direct);
					break;
				}
			}
		}
	}
	if (AssetGrid)
	{
		AssetGrid->SetContent(MakeAssetGrid());
	}
	if (IssueList)
	{
		IssueList->SetContent(MakeIssueList());
	}
	if (StackList)
	{
		StackList->SetContent(MakeStackList());
	}
	FString ParamSignature;
	if (Editor)
	{
		if (const FBoxLevelInstance* Inst = Editor->GetSelectedInstance())
		{
			ParamSignature = Inst->InstanceId.ToString();
			TArray<FBoxShownParam> Shown;
			BoxInstanceParams::Collect(Inst->LoadDefinition(), Inst->ParamOverrides, Shown);
			for (const FBoxShownParam& Param : Shown)
			{
				ParamSignature += TEXT("|");
				ParamSignature += Param.CompId.ToString();
				ParamSignature += TEXT(".");
				ParamSignature += Param.Key.ToString();
			}
		}
	}
	if (ParamList && (!bParamListReady || ParamSignature != CachedParamSignature))
	{
		bParamListReady = true;
		CachedParamSignature = ParamSignature;
		RebuildTypeChoices();
		ParamList->SetContent(MakeParamList());
	}
}

void SBoxLevelEditor::RebuildTypeChoices()
{
	TypeChoices.Reset();
	const UDataTable* Table = UBoxTypeDisplayLibrary::LoadTable();
	if (!Table)
	{
		return;
	}
	Table->ForeachRow<FBoxTypeDisplayRow>(TEXT("BoxLevelEditor"), [this](const FName&, const FBoxTypeDisplayRow& Row)
	{
		if (Row.Tag.IsValid())
		{
			TypeChoices.Add(MakeShared<FGameplayTag>(Row.Tag));
		}
	});
}

TSharedRef<SWidget> SBoxLevelEditor::MakeParamList()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const FBoxLevelInstance* Inst = Editor ? Editor->GetSelectedInstance() : nullptr;
	TArray<FBoxShownParam> Params;
	if (Inst)
	{
		BoxInstanceParams::Collect(Inst->LoadDefinition(), Inst->ParamOverrides, Params);
	}
	if (Params.Num() == 0)
	{
		Rows->AddSlot().AutoHeight().Padding(10, 8, 10, 6)
		[
			DetailHint(LOCTEXT("NoOverrides", "这份定义没有开放实例重载的参数"))
		];
		return Rows;
	}

	auto Live = [this](FName CompId, FName Key, FBoxShownParam& Out) -> bool
	{
		const FBoxLevelInstance* Current = Editor ? Editor->GetSelectedInstance() : nullptr;
		if (!Current)
		{
			return false;
		}
		TArray<FBoxShownParam> Shown;
		BoxInstanceParams::Collect(Current->LoadDefinition(), Current->ParamOverrides, Shown);
		for (const FBoxShownParam& Item : Shown)
		{
			if (Item.CompId == CompId && Item.Key == Key)
			{
				Out = Item;
				return true;
			}
		}
		return false;
	};

	for (const FBoxShownParam& Param : Params)
	{
		const FName CompId = Param.CompId;
		const FName Key = Param.Key;
		TSharedRef<SWidget> Control = SNew(STextBlock);
		if (Param.Kind == EBoxParamKind::Bool)
		{
			Control = SNew(SCheckBox)
				.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
				.IsChecked_Lambda([this, Live, CompId, Key]()
				{
					FBoxShownParam Item;
					return Live(CompId, Key, Item) && Item.BoolValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, CompId, Key](ECheckBoxState State)
				{
					if (!bRefreshing && Editor)
					{
						Editor->SetSelectedBool(CompId, Key, State == ECheckBoxState::Checked);
					}
				});
		}
		else if (Param.Kind == EBoxParamKind::Int)
		{
			const int32 Min = Param.IntMin;
			Control = SNew(SSpinBox<int32>)
				.MinValue(Min)
				.MaxValue(99)
				.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
				.Value_Lambda([this, Live, CompId, Key, Min]()
				{
					FBoxShownParam Item;
					return Live(CompId, Key, Item) ? Item.IntValue : Min;
				})
				.OnValueCommitted_Lambda([this, CompId, Key](int32 Value, ETextCommit::Type)
				{
					if (!bRefreshing && Editor)
					{
						Editor->SetSelectedInt(CompId, Key, Value);
					}
				});
		}
		else if (Param.Kind == EBoxParamKind::Name)
		{
			Control = SNew(SEditableTextBox)
				.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
				.Text_Lambda([this, Live, CompId, Key]()
				{
					FBoxShownParam Item;
					return Live(CompId, Key, Item) ? FText::FromName(Item.NameValue) : FText::GetEmpty();
				})
				.OnTextCommitted_Lambda([this, CompId, Key](const FText& Text, ETextCommit::Type)
				{
					if (!bRefreshing && Editor)
					{
						Editor->SetSelectedName(CompId, Key, FName(*Text.ToString()));
					}
				});
		}
		else
		{
			TSharedPtr<FGameplayTag> Selected;
			for (const TSharedPtr<FGameplayTag>& Choice : TypeChoices)
			{
				if (Choice.IsValid() && *Choice == Param.TagValue)
				{
					Selected = Choice;
					break;
				}
			}
			if (!Selected.IsValid() && Param.TagValue.IsValid())
			{
				Selected = MakeShared<FGameplayTag>(Param.TagValue);
				TypeChoices.Add(Selected);
			}
			Control = SNew(SComboBox<TSharedPtr<FGameplayTag>>)
				.OptionsSource(&TypeChoices)
				.InitiallySelectedItem(Selected)
				.IsEnabled_Lambda([this] { return Editor && !Editor->IsPlaying(); })
				.OnGenerateWidget_Lambda([](TSharedPtr<FGameplayTag> Item)
				{
					const FGameplayTag Tag = Item.IsValid() ? *Item : FGameplayTag();
					return SNew(STextBlock).Text(Tag.IsValid() ? UBoxTypeDisplayLibrary::GetDisplayName(Tag) : FText::FromString(TEXT("任意")));
				})
				.OnSelectionChanged_Lambda([this, CompId, Key](TSharedPtr<FGameplayTag> Item, ESelectInfo::Type)
				{
					if (!bRefreshing && Editor && Item.IsValid())
					{
						Editor->SetSelectedTag(CompId, Key, *Item);
					}
				})
				.Content()
				[
					SNew(STextBlock)
					.Text_Lambda([this, Live, CompId, Key]()
					{
						FBoxShownParam Item;
						if (!Live(CompId, Key, Item) || !Item.TagValue.IsValid())
						{
							return FText::FromString(TEXT("任意"));
						}
						return UBoxTypeDisplayLibrary::GetDisplayName(Item.TagValue);
					})
				];
		}

		if (Param.bCaptionAbove)
		{
			Rows->AddSlot().AutoHeight().Padding(10, 8, 10, 2)
			[
				SNew(STextBlock)
				.Text(Param.Label)
				.AutoWrapText(true)
				.Font(DetailLabelFont())
				.ColorAndOpacity(FSlateColor(DetailLabelColor))
			];
			Rows->AddSlot().AutoHeight().Padding(10, 0, 10, 4)
			[
				Control
			];
		}
		else
		{
			Rows->AddSlot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 5, 8, 5)
				[
					SNew(SBox).WidthOverride(96.f)
					[
						SNew(STextBlock)
						.Text(Param.Label)
						.Font(DetailLabelFont())
						.ColorAndOpacity(FSlateColor(DetailLabelColor))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					Control
				]
			];
		}
	}
	return Rows;
}

FReply SBoxLevelEditor::OnNew()
{
	if (Editor)
	{
		Editor->NewLevel();
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnSave()
{
	if (Editor)
	{
		Editor->SaveLevel();
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnPlay()
{
	if (Editor)
	{
		Editor->StartPlay();
		if (Editor->IsPlaying())
		{
			if (TSharedPtr<SWidget> View = Editor->GetViewportWidget())
			{
				return FReply::Handled().SetUserFocus(View.ToSharedRef(), EFocusCause::SetDirectly);
			}
		}
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (Editor && !InKeyEvent.IsRepeat() && Editor->HandleEditKey(InKeyEvent.GetKey(), InKeyEvent.IsControlDown()))
	{
		return FReply::Handled();
	}
	if (Editor && !InKeyEvent.IsRepeat())
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Editor->NotePlayMoveKey(Key, true) || Editor->HandlePlayKey(Key))
		{
			return FReply::Handled();
		}
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SBoxLevelEditor::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (Editor)
	{
		Editor->NotePlayMoveKey(InKeyEvent.GetKey(), false);
	}
	return SCompoundWidget::OnKeyUp(MyGeometry, InKeyEvent);
}

FReply SBoxLevelEditor::OnStop()
{
	if (Editor)
	{
		Editor->StopPlay();
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnRestart()
{
	if (Editor)
	{
		Editor->RestartPlay();
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnUndo()
{
	if (Editor)
	{
		if (Editor->IsPlaying())
		{
			Editor->UndoPlay();
		}
		else
		{
			Editor->UndoEdit();
		}
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnRedo()
{
	if (Editor)
	{
		if (Editor->IsPlaying())
		{
			Editor->RedoPlay();
		}
		else
		{
			Editor->RedoEdit();
		}
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnReload()
{
	if (Editor)
	{
		Editor->ReloadFromAssets();
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnValidate()
{
	if (Editor)
	{
		Editor->ValidateLevel();
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnDelete()
{
	if (Editor)
	{
		Editor->DeleteLevel();
	}
	return FReply::Handled();
}

FReply SBoxLevelEditor::OnFrame()
{
	if (Editor)
	{
		Editor->FrameCamera();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
