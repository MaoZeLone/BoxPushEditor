#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Data/BoxPushLevelTypes.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#include "UObject/GCObject.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STreeView.h"

class ABoxMatchWorld;
class ABoxPlayerCharacter;
class FAdvancedPreviewScene;
class FBoxLevelViewportClient;
class SBoxLevelViewport;
class UDataTable;
class UInteractableDef;
class ULevelData;
class UTerrainDef;

enum class EBoxLevelBrush : uint8
{
	Floor,
	Wall,
	Empty,
	Eraser,
	Player,
	Interactable
};

enum class EBoxEditorMode : uint8
{
	Place,
	Configure
};

enum class EBoxSceneSelection : uint8
{
	None,
	Player,
	Instance
};

struct FBoxCellPick
{
	EBoxSceneSelection Kind = EBoxSceneSelection::None;
	FName InstanceId;
	FText Label;
};

struct FBoxLevelListItem
{
	FAssetData Asset;
	TWeakObjectPtr<ULevelData> Level;
	FName LevelId;
	FText DisplayName;
	bool bListed = false;
	int32 CatalogIndex = INDEX_NONE;
	int32 CatalogCount = 0;

	ULevelData* Resolve();
	bool Matches(const ULevelData* InLevel) const;
};

struct FBoxPaletteFolder
{
	FGameplayTag Tag;
	FText Label;
	FString Path;
	int32 Depth = 0;
	TArray<TSharedPtr<FBoxPaletteFolder>> Children;
};

struct FBoxPaletteAsset
{
	FName Id;
	FGameplayTag TypeTag;
	FText Name;
	FText Type;
	FLinearColor Color = FLinearColor(0.3f, 0.3f, 0.3f);
	TSharedPtr<FSlateBrush> Icon;
	EBoxLevelBrush Brush = EBoxLevelBrush::Interactable;
	TWeakObjectPtr<UInteractableDef> Definition;
	TWeakObjectPtr<UTerrainDef> Terrain;
};

class FBoxLevelEditor : public TSharedFromThis<FBoxLevelEditor>, public FGCObject
{
public:
	FBoxLevelEditor();
	virtual ~FBoxLevelEditor() override;

	void Initialize();
	TSharedRef<SWidget> MakeWidget();

	FAdvancedPreviewScene* GetPreviewScene() const { return PreviewScene.Get(); }
	ULevelData* GetLevel() const { return CurrentLevel; }
	ABoxMatchWorld* GetMatchWorld() const { return MatchWorld; }
	ABoxPlayerCharacter* GetPreviewPlayer() const { return PreviewPlayer; }
	bool IsPlaying() const { return bPlaying; }
	EBoxEditorMode GetMode() const { return Mode; }
	bool IsInspectingLevel() const { return bInspectLevel; }
	EBoxSceneSelection GetSelectionKind() const { return SelectionKind; }
	const FBoxLevelInstance* GetSelectedInstance() const;
	bool GetSelectionCell(FIntPoint& OutCell) const;
	FText GetSelectedTitle() const;
	FText GetSelectedLogicSummary() const;
	FName GetBrushId() const { return BrushId; }
	FGameplayTag GetFolderTag() const { return FolderTag; }
	const FString& GetStatus() const { return Status; }
	FText GetControlsHint() const;
	const TArray<FLevelValidationIssue>& GetIssues() const { return Issues; }
	int32 GetSelectedIssue() const { return SelectedIssue; }
	const TArray<FIntPoint>& GetHighlightCells() const { return HighlightCells; }
	bool HasErrors() const { return bHasErrors; }
	bool CanUndoEdit() const { return UndoEdits.Num() > 0 && !bEditOpen; }
	bool CanRedoEdit() const { return RedoEdits.Num() > 0 && !bEditOpen; }
	const TArray<TSharedPtr<FBoxLevelListItem>>& GetLevelItems() const { return LevelItems; }
	const TArray<TSharedPtr<FBoxPaletteFolder>>& GetFolders() const { return Folders; }
	const TArray<TSharedPtr<FBoxPaletteFolder>>& GetFolderRoots() const { return FolderRoots; }
	const TArray<TSharedPtr<FBoxPaletteAsset>>& GetVisibleAssets() const { return VisibleAssets; }
	FString GetFolderPath() const;

	void SetLevelFilter(const FString& Filter);
	void SetAssetFilter(const FString& Filter);
	void SelectLevel(ULevelData* Level);
	void SelectFolder(FGameplayTag InFolderTag);
	void SelectBrush(FName InBrushId);
	void SetMode(EBoxEditorMode InMode);
	void ReloadFromAssets();
	void SelectAtCell(FIntPoint Cell);
	void SelectPick(EBoxSceneSelection Kind, FName InstanceId);
	void GetCellPicks(FIntPoint Cell, TArray<FBoxCellPick>& Out) const;
	void ClearSelection();
	void SetSelectedCell(FIntPoint Cell);
	void SetSelectedYaw(int32 YawSteps);
	void SetSelectedBool(FName CompId, FName Key, bool Value);
	void SetSelectedInt(FName CompId, FName Key, int32 Value);
	void SetSelectedName(FName CompId, FName Key, FName Value);
	void SetSelectedTag(FName CompId, FName Key, FGameplayTag Value);
	void DeleteSelected();
	void OpenSelectedDefinition();
	void SelectIssue(int32 Index);
	void BeginEditStroke();
	void EndEditStroke();
	void UndoEdit();
	void RedoEdit();
	bool HandleEditKey(const FKey& Key, bool bControlDown);
	void NewLevel();
	void SaveLevel();
	void DeleteLevel();
	void ValidateLevel();
	void SetListed(bool bListed);
	void MoveLevel(int32 Direction);
	void SetDisplayName(const FString& Name);
	void SetDesignerNote(const FString& Note);
	void SetSize(int32 Width, int32 Height);
	void PaintCell(FIntPoint Cell, bool bErase);
	void StartPlay();
	void StopPlay();
	void RestartPlay();
	void UndoPlay();
	void RedoPlay();
	void RequestMove(FIntPoint Dir);
	bool NotePlayMoveKey(FKey Key, bool bDown);
	void PushHeldMove();
	bool HandlePlayKey(FKey Key);
	void FocusViewport();
	TSharedPtr<SWidget> GetViewportWidget() const;
	void RefreshPreview();
	void FrameCamera();
	bool HitCell(const FVector& World, FIntPoint& OutCell) const;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FBoxLevelEditor"); }

	FSimpleMulticastDelegate OnChanged;

private:
	struct FBoxEditSnapshot
	{
		TArray<ETerrainCell> Cells;
		TArray<FBoxLevelInstance> Instances;
		FIntPoint PlayerSpawn = FIntPoint::ZeroValue;
		int32 PlayerYawSteps = 0;
		int32 Width = 0;
		int32 Height = 0;
		FText DisplayName;
		FString DesignerNote;
	};

	friend struct FBoxEditScope;

	void RebuildLevelList();
	void RebuildPalette();
	void RebuildVisibleAssets();
	void RebuildIssues();
	void NotifyChanged();
	void EnsurePlayer();
	void EnsureCurrentLevel();
	void BindAssetRegistry();
	void OnAssetRegistryReady();
	void ApplyNewCatalogRow(ULevelData* Level);
	FName NextLevelId() const;
	FName NextInstanceId(FName DefId) const;
	FBoxPaletteAsset* FindAsset(FName Id);
	UDataTable* LoadCatalog() const;
	bool IsListed(const ULevelData* Level) const;
	bool IsListedAsset(const FAssetData& Asset) const;
	void RemoveInstancesAt(FIntPoint Cell);
	bool HasBlockingAt(FIntPoint Cell) const;
	void ResetSelection();
	void PruneSelection();
	FBoxLevelInstance* FindSelectedInstance();
	FText TitleForInstance(const FBoxLevelInstance& Inst) const;
	FBoxEditSnapshot CaptureEdit() const;
	bool SameEdit(const FBoxEditSnapshot& A, const FBoxEditSnapshot& B) const;
	void ApplyEdit(const FBoxEditSnapshot& Snap);
	void ClearEditHistory();

	TUniquePtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<SBoxLevelViewport> Viewport;

	TObjectPtr<ULevelData> CurrentLevel = nullptr;
	TObjectPtr<ABoxMatchWorld> MatchWorld = nullptr;
	TObjectPtr<ABoxPlayerCharacter> PreviewPlayer = nullptr;

	TArray<TSharedPtr<FBoxLevelListItem>> LevelItems;
	TArray<TSharedPtr<FBoxPaletteFolder>> Folders;
	TArray<TSharedPtr<FBoxPaletteFolder>> FolderRoots;
	TArray<FBoxPaletteAsset> AllAssets;
	TArray<TSharedPtr<FBoxPaletteAsset>> VisibleAssets;
	TArray<FLevelValidationIssue> Issues;
	TArray<FIntPoint> HighlightCells;
	TArray<FBoxEditSnapshot> UndoEdits;
	TArray<FBoxEditSnapshot> RedoEdits;
	int32 SelectedIssue = INDEX_NONE;
	bool bEditOpen = false;

	FName BrushId = TEXT("Floor");
	FGameplayTag FolderTag;
	EBoxEditorMode Mode = EBoxEditorMode::Configure;
	bool bInspectLevel = true;
	EBoxSceneSelection SelectionKind = EBoxSceneSelection::None;
	FName SelectedInstanceId;
	FString LevelFilter;
	FString AssetFilter;
	FString Status;
	bool bPlaying = false;
	uint8 HeldMoveKeys = 0;
	bool bHasErrors = false;
	FDelegateHandle FilesLoadedHandle;
};

class SBoxLevelEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBoxLevelEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FBoxLevelEditor>& InEditor);
	virtual ~SBoxLevelEditor() override;

private:
	TSharedRef<ITableRow> OnGenerateLevelRow(TSharedPtr<FBoxLevelListItem> Item, const TSharedRef<STableViewBase>& Owner);
	TSharedRef<ITableRow> OnGenerateFolderRow(TSharedPtr<FBoxPaletteFolder> Item, const TSharedRef<STableViewBase>& Owner);
	void OnGetFolderChildren(TSharedPtr<FBoxPaletteFolder> Item, TArray<TSharedPtr<FBoxPaletteFolder>>& OutChildren);
	void ExpandFolderTree(const TSharedPtr<FBoxPaletteFolder>& Item);
	TSharedRef<SWidget> MakeDetails();
	TSharedRef<SWidget> MakeSelectionDetails();
	TSharedRef<SWidget> MakeAssetGrid();
	TSharedRef<SWidget> MakeToolbar();
	TSharedRef<SWidget> MakeIssueBar();
	TSharedRef<SWidget> MakeIssueList();
	TSharedRef<SWidget> MakeStackList();
	TSharedRef<SWidget> MakeParamList();
	void RebuildTypeChoices();
	void Refresh();
	FReply OnNew();
	FReply OnSave();
	FReply OnPlay();
	FReply OnStop();
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	FReply OnRestart();
	FReply OnUndo();
	FReply OnRedo();
	FReply OnValidate();
	FReply OnReload();
	FReply OnDelete();
	FReply OnFrame();

	TSharedPtr<FBoxLevelEditor> Editor;
	TSharedPtr<SListView<TSharedPtr<FBoxLevelListItem>>> LevelList;
	TSharedPtr<STreeView<TSharedPtr<FBoxPaletteFolder>>> FolderTree;
	TSharedPtr<SBox> AssetGrid;
	TSharedPtr<SBox> IssueList;
	TSharedPtr<SBox> StackList;
	TSharedPtr<SBox> ParamList;
	TArray<TSharedPtr<FGameplayTag>> TypeChoices;
	FString CachedParamSignature;
	bool bParamListReady = false;
	FDelegateHandle ChangeHandle;
	bool bRefreshing = false;
	bool bFolderExpandInitialized = false;
};
