#include "BoxLevelViewport.h"

#include "BoxLevelEditor.h"
#include "AdvancedPreviewScene.h"
#include "SceneManagement.h"
#include "CameraController.h"
#include "Data/LevelData.h"
#include "HitProxies.h"
#include "Match/BoxGrid.h"
#include "Styling/AppStyle.h"
#include "UnrealWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

FBoxLevelViewportClient::FBoxLevelViewportClient(FAdvancedPreviewScene* InPreviewScene, const TSharedRef<FBoxLevelEditor>& InEditor)
	: FEditorViewportClient(nullptr, InPreviewScene, nullptr)
	, Editor(InEditor)
{
	SetRealtime(true);
	SetViewMode(VMI_Lit);
	EngineShowFlags.SetGrid(false);
	EngineShowFlags.SetPivot(false);
	EngineShowFlags.SetSelectionOutline(false);
	bSetListenerPosition = false;
	bUsingOrbitCamera = false;
	ViewFOV = BoxPlayCamera::FieldOfView();
	FOVAngle = BoxPlayCamera::FieldOfView();
	OverrideNearClipPlane(1.f);
	SetGameView(false);
	ShowWidget(true);
	SetRequiredCursor(true, false);
	bLockFlightCamera = true;
}

bool FBoxLevelViewportClient::IsPaintMouse(FKey Key) const
{
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	return Pinned && Pinned->GetMode() == EBoxEditorMode::Place && !Pinned->IsPlaying()
		&& !IsAltPressed() && !IsCtrlPressed() && !IsShiftPressed()
		&& (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton);
}

bool FBoxLevelViewportClient::IsSelectMouse(FKey Key) const
{
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	return Pinned && Pinned->GetMode() == EBoxEditorMode::Configure && !Pinned->IsPlaying()
		&& !IsAltPressed() && !IsCtrlPressed() && !IsShiftPressed()
		&& (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton);
}

EMouseCaptureMode FBoxLevelViewportClient::GetMouseCaptureMode() const
{
	return EMouseCaptureMode::CaptureDuringMouseDown;
}

bool FBoxLevelViewportClient::HideCursorDuringCapture() const
{
	return false;
}

bool FBoxLevelViewportClient::RequiresUncapturedAxisInput() const
{
	return !bWidgetDragging && !bObjectPress && !bObjectDragging;
}

void FBoxLevelViewportClient::FrameBoard()
{
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	ULevelData* Level = Pinned ? Pinned->GetLevel() : nullptr;
	if (!Level)
	{
		return;
	}

	if (CameraController)
	{
		CameraController->ResetVelocity();
	}

	bUsingOrbitCamera = false;
	ViewFOV = BoxPlayCamera::FieldOfView();
	FOVAngle = BoxPlayCamera::FieldOfView();
	SetViewLocation(BoxPlayCamera::ViewLocation(Level->Width, Level->Height));
	SetViewRotation(BoxPlayCamera::Rotation());
	Invalidate();
}

bool FBoxLevelViewportClient::MouseToCell(int32 MouseX, int32 MouseY, FIntPoint& OutCell)
{
	if (!Viewport)
	{
		return false;
	}

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetScene(), EngineShowFlags).SetRealtimeUpdate(IsRealtime()));
	FSceneView* View = CalcSceneView(&ViewFamily);
	if (!View)
	{
		return false;
	}

	const FViewportCursorLocation Cursor(View, this, MouseX, MouseY);
	const FVector Origin = Cursor.GetOrigin();
	const FVector Dir = Cursor.GetDirection();
	if (FMath::Abs(Dir.Z) < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float T = (BoxGrid::OriginZ() - Origin.Z) / Dir.Z;
	if (T < 0.f)
	{
		return false;
	}
	const FVector Hit = Origin + Dir * T;
	OutCell = FIntPoint(
		FMath::FloorToInt(Hit.X / BoxGrid::CellSize()),
		FMath::FloorToInt(Hit.Y / BoxGrid::CellSize()));
	return true;
}

void FBoxLevelViewportClient::PaintAtCursor(int32 MouseX, int32 MouseY, bool bErase)
{
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	if (!Pinned || Pinned->IsPlaying())
	{
		return;
	}
	FIntPoint Cell;
	if (!MouseToCell(MouseX, MouseY, Cell) || Cell == LastPainted)
	{
		return;
	}
	LastPainted = Cell;
	Pinned->PaintCell(Cell, bErase);
}

void FBoxLevelViewportClient::SetDropHover(int32 MouseX, int32 MouseY)
{
	FIntPoint Cell;
	if (!MouseToCell(MouseX, MouseY, Cell))
	{
		ClearDropHover();
		return;
	}
	if (!DropHover.IsSet() || DropHover.GetValue() != Cell)
	{
		DropHover = Cell;
		Invalidate();
	}
}

void FBoxLevelViewportClient::ClearDropHover()
{
	if (DropHover.IsSet())
	{
		DropHover.Reset();
		Invalidate();
	}
}

bool FBoxLevelViewportClient::DropBrush(int32 MouseX, int32 MouseY, FName BrushId)
{
	ClearDropHover();
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	if (!Pinned || Pinned->IsPlaying() || BrushId.IsNone())
	{
		return false;
	}
	FIntPoint Cell;
	if (!MouseToCell(MouseX, MouseY, Cell))
	{
		return false;
	}
	Pinned->SelectBrush(BrushId);
	Pinned->PaintCell(Cell, BrushId == TEXT("eraser"));
	return true;
}

bool FBoxLevelViewportClient::HasMoveWidget() const
{
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	FIntPoint Cell;
	return Pinned
		&& Pinned->GetMode() == EBoxEditorMode::Configure
		&& !Pinned->IsPlaying()
		&& Pinned->GetSelectionCell(Cell);
}

bool FBoxLevelViewportClient::IsMoveWidgetAxis(HHitProxy* Proxy) const
{
	const HWidgetAxis* AxisProxy = HitProxyCast<HWidgetAxis>(Proxy);
	if (!AxisProxy)
	{
		return false;
	}
	return AxisProxy->Axis == EAxisList::X || AxisProxy->Axis == EAxisList::Y;
}

bool FBoxLevelViewportClient::IsWidgetAxisHit(FViewport* InViewport) const
{
	if (!InViewport || !HasMoveWidget())
	{
		return false;
	}
	return IsMoveWidgetAxis(InViewport->GetHitProxy(InViewport->GetMouseX(), InViewport->GetMouseY()));
}

bool FBoxLevelViewportClient::ShouldForwardMouseToWidget(FKey Key) const
{
	if (Key != EKeys::LeftMouseButton)
	{
		return false;
	}
	return bWidgetDragging || IsWidgetAxisHit(Viewport);
}

UE::Widget::EWidgetMode FBoxLevelViewportClient::GetWidgetMode() const
{
	return HasMoveWidget() ? UE::Widget::WM_Translate : UE::Widget::WM_None;
}

bool FBoxLevelViewportClient::CanSetWidgetMode(UE::Widget::EWidgetMode NewMode) const
{
	return NewMode == UE::Widget::WM_Translate || NewMode == UE::Widget::WM_None;
}

bool FBoxLevelViewportClient::CanCycleWidgetMode() const
{
	return false;
}

FVector FBoxLevelViewportClient::GetWidgetLocation() const
{
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	FIntPoint Cell;
	if (Pinned && Pinned->GetSelectionCell(Cell))
	{
		return BoxGrid::CellToWorld(Cell, 40.f);
	}
	return FVector::ZeroVector;
}

FMatrix FBoxLevelViewportClient::GetWidgetCoordSystem() const
{
	return FMatrix::Identity;
}

ECoordSystem FBoxLevelViewportClient::GetWidgetCoordSystemSpace() const
{
	return COORD_World;
}

void FBoxLevelViewportClient::TrackingStarted(const FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge)
{
	FEditorViewportClient::TrackingStarted(InInputState, bIsDraggingWidget, bNudge);
	if (bIsDraggingWidget)
	{
		bWidgetDragging = true;
		WidgetDrag = FVector::ZeroVector;
		DragOriginWorld = GetWidgetLocation();
		if (TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin())
		{
			Pinned->BeginEditStroke();
		}
	}
}

void FBoxLevelViewportClient::TrackingStopped()
{
	FEditorViewportClient::TrackingStopped();
	WidgetDrag = FVector::ZeroVector;
	if (bWidgetDragging)
	{
		bWidgetDragging = false;
		if (TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin())
		{
			Pinned->EndEditStroke();
		}
	}
}

bool FBoxLevelViewportClient::InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
	(void)CurrentAxis;
	(void)Drag;
	(void)Rot;
	(void)Scale;
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	if (!Pinned || !HasMoveWidget() || !InViewport)
	{
		return false;
	}

	FIntPoint Cell;
	if (MouseToCell(InViewport->GetMouseX(), InViewport->GetMouseY(), Cell))
	{
		Pinned->SetSelectedCell(Cell);
	}
	return true;
}

void FBoxLevelViewportClient::SelectAtCursor(int32 MouseX, int32 MouseY, bool bClear)
{
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	if (!Pinned || Pinned->GetMode() != EBoxEditorMode::Configure || Pinned->IsPlaying())
	{
		return;
	}

	FIntPoint Cell(MAX_int32, MAX_int32);
	if (!bClear && !MouseToCell(MouseX, MouseY, Cell))
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (Cell == LastSelectCell && bClear == bLastSelectClear && Now - LastSelectTime < 0.05)
	{
		return;
	}
	LastSelectCell = Cell;
	bLastSelectClear = bClear;
	LastSelectTime = Now;

	if (bClear)
	{
		Pinned->ClearSelection();
	}
	else
	{
		Pinned->SelectAtCell(Cell);
	}
}

void FBoxLevelViewportClient::BeginSelectPress(int32 MouseX, int32 MouseY, bool bClear)
{
	if (bSelectStroke || bObjectPress || bObjectDragging)
	{
		return;
	}
	bSelectStroke = true;
	if (bClear || IsWidgetAxisHit(Viewport))
	{
		if (bClear)
		{
			SelectAtCursor(MouseX, MouseY, true);
		}
		return;
	}

	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	FIntPoint SelectionCell;
	FIntPoint Cell;
	if (Pinned && Pinned->GetSelectionCell(SelectionCell) && MouseToCell(MouseX, MouseY, Cell) && Cell == SelectionCell)
	{
		bObjectPress = true;
		bObjectDragging = false;
		ObjectPressX = MouseX;
		ObjectPressY = MouseY;
		ObjectPressCell = Cell;
		return;
	}
	SelectAtCursor(MouseX, MouseY, false);
}

void FBoxLevelViewportClient::UpdateObjectDrag(int32 MouseX, int32 MouseY)
{
	if (!bObjectPress || bWidgetDragging)
	{
		return;
	}
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	if (!Pinned)
	{
		return;
	}
	FIntPoint Cell;
	const bool bHasCell = MouseToCell(MouseX, MouseY, Cell);
	if (!bObjectDragging)
	{
		const int32 Dx = MouseX - ObjectPressX;
		const int32 Dy = MouseY - ObjectPressY;
		const bool bMovedCell = bHasCell && Cell != ObjectPressCell;
		if (!bMovedCell && (Dx * Dx + Dy * Dy) < 16)
		{
			return;
		}
		bObjectDragging = true;
		Pinned->BeginEditStroke();
	}
	if (bHasCell)
	{
		Pinned->SetSelectedCell(Cell);
	}
}

void FBoxLevelViewportClient::EndSelectPress()
{
	const bool bWasDragging = bObjectDragging;
	const bool bWasArmed = bObjectPress;
	const int32 PressX = ObjectPressX;
	const int32 PressY = ObjectPressY;
	bObjectPress = false;
	bObjectDragging = false;
	bSelectStroke = false;
	if (bWasDragging)
	{
		if (TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin())
		{
			Pinned->EndEditStroke();
		}
	}
	else if (bWasArmed)
	{
		SelectAtCursor(PressX, PressY, false);
	}
}

void FBoxLevelViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);
	TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin();
	if (!PDI || !Pinned || Pinned->IsPlaying())
	{
		return;
	}
	for (const FIntPoint& Cell : Pinned->GetHighlightCells())
	{
		const FVector Mark = BoxGrid::CellToWorld(Cell, 18.f);
		const float Half = BoxGrid::CellSize() * 0.48f;
		const FLinearColor MarkColor(0.95f, 0.28f, 0.22f);
		const FVector A = Mark + FVector(-Half, -Half, 0.f);
		const FVector B = Mark + FVector(Half, -Half, 0.f);
		const FVector C = Mark + FVector(Half, Half, 0.f);
		const FVector D = Mark + FVector(-Half, Half, 0.f);
		PDI->DrawLine(A, B, MarkColor, SDPG_Foreground, 3.f, 0.f, true);
		PDI->DrawLine(B, C, MarkColor, SDPG_Foreground, 3.f, 0.f, true);
		PDI->DrawLine(C, D, MarkColor, SDPG_Foreground, 3.f, 0.f, true);
		PDI->DrawLine(D, A, MarkColor, SDPG_Foreground, 3.f, 0.f, true);
	}
	if (DropHover.IsSet())
	{
		const FVector Mark = BoxGrid::CellToWorld(DropHover.GetValue(), 22.f);
		const float Half = BoxGrid::CellSize() * 0.46f;
		const FLinearColor HoverColor(1.f, 0.78f, 0.08f);
		const FVector A = Mark + FVector(-Half, -Half, 0.f);
		const FVector B = Mark + FVector(Half, -Half, 0.f);
		const FVector C = Mark + FVector(Half, Half, 0.f);
		const FVector D = Mark + FVector(-Half, Half, 0.f);
		PDI->DrawLine(A, B, HoverColor, SDPG_Foreground, 3.f, 0.f, true);
		PDI->DrawLine(B, C, HoverColor, SDPG_Foreground, 3.f, 0.f, true);
		PDI->DrawLine(C, D, HoverColor, SDPG_Foreground, 3.f, 0.f, true);
		PDI->DrawLine(D, A, HoverColor, SDPG_Foreground, 3.f, 0.f, true);
	}
	if (Pinned->GetMode() != EBoxEditorMode::Configure)
	{
		return;
	}
	if (ULevelData* Level = Pinned->GetLevel())
	{
		const FBoxLevelInstance* Selected = Pinned->GetSelectedInstance();
		for (const FBoxLevelInstance& Inst : Level->Instances)
		{
			const bool bSelected = Selected && Selected->InstanceId == Inst.InstanceId;
			const FIntPoint Dir = BoxFacing::ToDir(Inst.YawSteps);
			const FVector Forward(Dir.X, Dir.Y, 0.f);
			const FVector Side(-static_cast<float>(Dir.Y), static_cast<float>(Dir.X), 0.f);
			const FVector Center = BoxGrid::CellToWorld(Inst.Cell, bSelected ? 56.f : 40.f);
			const float Len = BoxGrid::CellSize() * 0.28f;
			const FVector Tail = Center - Forward * Len * 0.35f;
			const FVector Tip = Center + Forward * Len;
			const FVector HeadBase = Tip - Forward * (Len * 0.45f);
			const float Head = BoxGrid::CellSize() * 0.08f;
			const FLinearColor ArrowColor = bSelected
				? FLinearColor(1.f, 0.78f, 0.08f)
				: FLinearColor(0.75f, 0.75f, 0.7f);
			const float Thickness = bSelected ? 2.5f : 1.5f;
			PDI->DrawLine(Tail, Tip, ArrowColor, SDPG_Foreground, Thickness, 0.f, true);
			PDI->DrawLine(Tip, HeadBase + Side * Head, ArrowColor, SDPG_Foreground, Thickness, 0.f, true);
			PDI->DrawLine(Tip, HeadBase - Side * Head, ArrowColor, SDPG_Foreground, Thickness, 0.f, true);
		}
	}

	FIntPoint Cell;
	if (!Pinned->GetSelectionCell(Cell))
	{
		return;
	}

	const FVector Center = BoxGrid::CellToWorld(Cell, 24.f);
	const float Half = BoxGrid::CellSize() * 0.46f;
	const FLinearColor Color(1.f, 0.78f, 0.08f);
	const FVector A = Center + FVector(-Half, -Half, 0.f);
	const FVector B = Center + FVector(Half, -Half, 0.f);
	const FVector C = Center + FVector(Half, Half, 0.f);
	const FVector D = Center + FVector(-Half, Half, 0.f);
	PDI->DrawLine(A, B, Color, SDPG_Foreground, 2.f, 0.f, true);
	PDI->DrawLine(B, C, Color, SDPG_Foreground, 2.f, 0.f, true);
	PDI->DrawLine(C, D, Color, SDPG_Foreground, 2.f, 0.f, true);
	PDI->DrawLine(D, A, Color, SDPG_Foreground, 2.f, 0.f, true);
}

void FBoxLevelViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
	if (TSharedPtr<FBoxLevelEditor> HeldEditor = Editor.Pin())
	{
		HeldEditor->PushHeldMove();
	}
	if (UWorld* World = PreviewScene ? PreviewScene->GetWorld() : nullptr)
	{
		World->Tick(LEVELTICK_All, DeltaSeconds);
	}
	const UE::Widget::EWidgetMode WidgetMode = GetWidgetMode();
	if (WidgetMode != LastWidgetMode)
	{
		LastWidgetMode = WidgetMode;
		if (Viewport)
		{
			Viewport->InvalidateHitProxy();
		}
		Invalidate();
	}
	if (!Viewport)
	{
		return;
	}
	const int32 X = Viewport->GetMouseX();
	const int32 Y = Viewport->GetMouseY();
	const bool bPainting = (Viewport->KeyState(EKeys::LeftMouseButton) && IsPaintMouse(EKeys::LeftMouseButton))
		|| (Viewport->KeyState(EKeys::RightMouseButton) && IsPaintMouse(EKeys::RightMouseButton));
	if (TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin())
	{
		if (bPainting && !bPaintStroke)
		{
			Pinned->BeginEditStroke();
			bPaintStroke = true;
		}
		else if (!bPainting && bPaintStroke)
		{
			Pinned->EndEditStroke();
			bPaintStroke = false;
		}
	}
	if (Viewport->KeyState(EKeys::LeftMouseButton) && IsPaintMouse(EKeys::LeftMouseButton))
	{
		PaintAtCursor(X, Y, false);
	}
	else if (Viewport->KeyState(EKeys::RightMouseButton) && IsPaintMouse(EKeys::RightMouseButton))
	{
		PaintAtCursor(X, Y, true);
	}
	else
	{
		LastPainted = FIntPoint(MAX_int32, MAX_int32);
	}

	const bool bSelecting = (Viewport->KeyState(EKeys::LeftMouseButton) && IsSelectMouse(EKeys::LeftMouseButton))
		|| (Viewport->KeyState(EKeys::RightMouseButton) && IsSelectMouse(EKeys::RightMouseButton));
	if (bSelecting)
	{
		if (!bWidgetDragging && !bSelectStroke && !bObjectPress && !bObjectDragging)
		{
			const bool bClear = Viewport->KeyState(EKeys::RightMouseButton) && IsSelectMouse(EKeys::RightMouseButton);
			BeginSelectPress(X, Y, bClear);
		}
		else if (bObjectPress || bObjectDragging)
		{
			UpdateObjectDrag(X, Y);
		}
	}
	else if (bSelectStroke || bObjectPress || bObjectDragging)
	{
		EndSelectPress();
	}
}

void FBoxLevelViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	if (IsMoveWidgetAxis(HitProxy))
	{
		FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
		return;
	}
	if (Event == IE_Pressed && IsSelectMouse(Key))
	{
		BeginSelectPress(static_cast<int32>(HitX), static_cast<int32>(HitY), Key == EKeys::RightMouseButton);
		return;
	}
	if (Event == IE_Pressed && IsPaintMouse(Key))
	{
		LastPainted = FIntPoint(MAX_int32, MAX_int32);
		PaintAtCursor(static_cast<int32>(HitX), static_cast<int32>(HitY), Key == EKeys::RightMouseButton);
		return;
	}
	FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
}

bool FBoxLevelViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	if (ShouldForwardMouseToWidget(EventArgs.Key))
	{
		return FEditorViewportClient::InputKey(EventArgs);
	}

	if (IsSelectMouse(EventArgs.Key))
	{
		if (EventArgs.Event == IE_Pressed && EventArgs.Viewport)
		{
			BeginSelectPress(EventArgs.Viewport->GetMouseX(), EventArgs.Viewport->GetMouseY(), EventArgs.Key == EKeys::RightMouseButton);
		}
		else if (EventArgs.Event == IE_Released)
		{
			EndSelectPress();
		}
		return true;
	}

	if (IsPaintMouse(EventArgs.Key))
	{
		if (EventArgs.Event == IE_Pressed && EventArgs.Viewport)
		{
			if (!bPaintStroke)
			{
				if (TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin())
				{
					Pinned->BeginEditStroke();
				}
				bPaintStroke = true;
			}
			LastPainted = FIntPoint(MAX_int32, MAX_int32);
			PaintAtCursor(EventArgs.Viewport->GetMouseX(), EventArgs.Viewport->GetMouseY(), EventArgs.Key == EKeys::RightMouseButton);
		}
		return true;
	}

	if (TSharedPtr<FBoxLevelEditor> Pinned = Editor.Pin())
	{
		if (EventArgs.Event == IE_Pressed && EventArgs.Key == EKeys::F && !IsCtrlPressed())
		{
			Pinned->FrameCamera();
			return true;
		}

		if (EventArgs.Event == IE_Pressed && Pinned->HandleEditKey(EventArgs.Key, IsCtrlPressed()))
		{
			return true;
		}

		const FKey Key = EventArgs.Key;
		const bool bMoveKey = Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D
			|| Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right;
		if (bMoveKey)
		{
			if (EventArgs.Event == IE_Pressed)
			{
				Pinned->NotePlayMoveKey(Key, true);
			}
			else if (EventArgs.Event == IE_Released)
			{
				Pinned->NotePlayMoveKey(Key, false);
			}
			return true;
		}

		if (EventArgs.Event == IE_Pressed && Pinned->HandlePlayKey(Key))
		{
			return true;
		}

		if (Pinned->IsPlaying()
			&& (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton)
			&& !IsAltPressed())
		{
			return true;
		}
	}
	return FEditorViewportClient::InputKey(EventArgs);
}

void FBoxLevelViewportClient::CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (!InViewport)
	{
		return;
	}
	if (bObjectPress || bObjectDragging)
	{
		UpdateObjectDrag(InMouseX, InMouseY);
		return;
	}
	if (InViewport->KeyState(EKeys::LeftMouseButton) && IsPaintMouse(EKeys::LeftMouseButton))
	{
		PaintAtCursor(InMouseX, InMouseY, false);
		return;
	}
	if (InViewport->KeyState(EKeys::RightMouseButton) && IsPaintMouse(EKeys::RightMouseButton))
	{
		PaintAtCursor(InMouseX, InMouseY, true);
		return;
	}
	FEditorViewportClient::CapturedMouseMove(InViewport, InMouseX, InMouseY);
}

void SBoxLevelViewport::Construct(const FArguments& InArgs, const TSharedRef<FBoxLevelEditor>& InEditor)
{
	Editor = InEditor;
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SBoxLevelViewport::MakeEditorViewportClient()
{
	Client = MakeShared<FBoxLevelViewportClient>(Editor->GetPreviewScene(), Editor.ToSharedRef());
	return Client.ToSharedRef();
}

void SBoxLevelViewport::FrameBoard()
{
	if (Client)
	{
		Client->FrameBoard();
	}
}

bool SBoxLevelViewport::CursorToViewport(const FGeometry& MyGeometry, const FVector2D& ScreenPos, int32& OutX, int32& OutY) const
{
	if (!Client || !Client->Viewport)
	{
		return false;
	}
	const FGeometry& Geo = ViewportWidget.IsValid() ? ViewportWidget->GetCachedGeometry() : MyGeometry;
	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos);
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.X < 1.f || Size.Y < 1.f)
	{
		return false;
	}
	const FIntPoint Vp = Client->Viewport->GetSizeXY();
	OutX = FMath::Clamp(FMath::FloorToInt(Local.X / Size.X * Vp.X), 0, FMath::Max(Vp.X - 1, 0));
	OutY = FMath::Clamp(FMath::FloorToInt(Local.Y / Size.Y * Vp.Y), 0, FMath::Max(Vp.Y - 1, 0));
	return true;
}

FReply SBoxLevelViewport::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FBoxPaletteDragDrop> Drag = DragDropEvent.GetOperationAs<FBoxPaletteDragDrop>();
	if (!Drag || !Client || !Editor || Editor->IsPlaying())
	{
		return FReply::Unhandled();
	}
	int32 X = 0;
	int32 Y = 0;
	if (CursorToViewport(MyGeometry, DragDropEvent.GetScreenSpacePosition(), X, Y))
	{
		Client->SetDropHover(X, Y);
	}
	return FReply::Handled();
}

FReply SBoxLevelViewport::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FBoxPaletteDragDrop> Drag = DragDropEvent.GetOperationAs<FBoxPaletteDragDrop>();
	if (!Drag || !Client || !Editor || Editor->IsPlaying())
	{
		if (Client)
		{
			Client->ClearDropHover();
		}
		return FReply::Unhandled();
	}
	int32 X = 0;
	int32 Y = 0;
	if (!CursorToViewport(MyGeometry, DragDropEvent.GetScreenSpacePosition(), X, Y) || !Client->DropBrush(X, Y, Drag->BrushId))
	{
		Client->ClearDropHover();
		return FReply::Unhandled();
	}
	return FReply::Handled();
}

void SBoxLevelViewport::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	if (Client)
	{
		Client->ClearDropHover();
	}
	SEditorViewport::OnDragLeave(DragDropEvent);
}

void SBoxLevelViewport::PopulateViewportOverlays(TSharedRef<SOverlay> Overlay)
{
	Overlay->AddSlot()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Left)
		.Padding(10, 10)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			.Padding(FMargin(10, 6))
			[
				SNew(STextBlock)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.92f, 0.86f)))
				.Text_Lambda([this]
				{
					return Editor ? Editor->GetControlsHint() : FText::GetEmpty();
				})
			]
		];
}
