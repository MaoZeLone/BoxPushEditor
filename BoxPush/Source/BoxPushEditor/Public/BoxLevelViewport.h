#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "Input/DragAndDrop.h"
#include "SEditorViewport.h"
#include "UnrealWidgetFwd.h"

class FBoxPaletteDragDrop : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FBoxPaletteDragDrop, FDragDropOperation)

	FName BrushId;

	static TSharedRef<FBoxPaletteDragDrop> New(FName InBrushId)
	{
		TSharedRef<FBoxPaletteDragDrop> Operation = MakeShared<FBoxPaletteDragDrop>();
		Operation->BrushId = InBrushId;
		Operation->Construct();
		return Operation;
	}
};

class FAdvancedPreviewScene;
class FBoxLevelEditor;

class FBoxLevelViewportClient : public FEditorViewportClient
{
public:
	FBoxLevelViewportClient(FAdvancedPreviewScene* InPreviewScene, const TSharedRef<FBoxLevelEditor>& InEditor);

	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual void CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual EMouseCaptureMode GetMouseCaptureMode() const override;
	virtual bool HideCursorDuringCapture() const override;
	virtual bool RequiresUncapturedAxisInput() const override;
	virtual void TrackingStarted(const FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge) override;
	virtual void TrackingStopped() override;
	virtual bool InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;
	virtual bool CanSetWidgetMode(UE::Widget::EWidgetMode NewMode) const override;
	virtual bool CanCycleWidgetMode() const override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	virtual FVector GetWidgetLocation() const override;
	virtual FMatrix GetWidgetCoordSystem() const override;
	virtual ECoordSystem GetWidgetCoordSystemSpace() const override;

	void FrameBoard();
	void SetDropHover(int32 MouseX, int32 MouseY);
	void ClearDropHover();
	bool DropBrush(int32 MouseX, int32 MouseY, FName BrushId);

private:
	bool MouseToCell(int32 MouseX, int32 MouseY, FIntPoint& OutCell);
	void PaintAtCursor(int32 MouseX, int32 MouseY, bool bErase);
	void SelectAtCursor(int32 MouseX, int32 MouseY, bool bClear);
	void BeginSelectPress(int32 MouseX, int32 MouseY, bool bClear);
	void UpdateObjectDrag(int32 MouseX, int32 MouseY);
	void EndSelectPress();
	bool IsPaintMouse(FKey Key) const;
	bool IsSelectMouse(FKey Key) const;
	bool HasMoveWidget() const;
	bool IsMoveWidgetAxis(HHitProxy* Proxy) const;
	bool IsWidgetAxisHit(FViewport* InViewport) const;
	bool ShouldForwardMouseToWidget(FKey Key) const;

	TWeakPtr<FBoxLevelEditor> Editor;
	FIntPoint LastPainted = FIntPoint(MAX_int32, MAX_int32);
	FIntPoint LastSelectCell = FIntPoint(MAX_int32, MAX_int32);
	bool bLastSelectClear = false;
	double LastSelectTime = -1.0;
	UE::Widget::EWidgetMode LastWidgetMode = UE::Widget::WM_None;
	FVector WidgetDrag = FVector::ZeroVector;
	FVector DragOriginWorld = FVector::ZeroVector;
	bool bWidgetDragging = false;
	bool bPaintStroke = false;
	bool bSelectStroke = false;
	bool bObjectPress = false;
	bool bObjectDragging = false;
	int32 ObjectPressX = 0;
	int32 ObjectPressY = 0;
	FIntPoint ObjectPressCell = FIntPoint(MAX_int32, MAX_int32);
	TOptional<FIntPoint> DropHover;
};

class SBoxLevelViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SBoxLevelViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FBoxLevelEditor>& InEditor);
	void FrameBoard();

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual void PopulateViewportOverlays(TSharedRef<SOverlay> Overlay) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;

private:
	bool CursorToViewport(const FGeometry& MyGeometry, const FVector2D& ScreenPos, int32& OutX, int32& OutY) const;
	TSharedPtr<FBoxLevelEditor> Editor;
	TSharedPtr<FBoxLevelViewportClient> Client;
};
