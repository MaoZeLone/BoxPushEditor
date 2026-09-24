#include "UI/BoxKenneyUi.h"

#include "Game/BoxProjectSettings.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

namespace
{
	TMap<FString, TObjectPtr<UTexture2D>> GTextures;
	TSet<FString> GMissing;
	TObjectPtr<UFont> GFont;
	bool bFontTried = false;

	FString RootDir()
	{
		const UBoxProjectSettings* Settings = GetDefault<UBoxProjectSettings>();
		return Settings ? Settings->ResolveUiPackRoot() : FString();
	}

	FString PackFile(const TCHAR* Rel)
	{
		return FPaths::Combine(RootDir(), Rel);
	}

	UTexture2D* LoadPng(const FString& Rel)
	{
		if (TObjectPtr<UTexture2D>* Found = GTextures.Find(Rel))
		{
			return Found->Get();
		}
		if (GMissing.Contains(Rel))
		{
			return nullptr;
		}

		const FString File = PackFile(*Rel);
		TArray<uint8> Compressed;
		if (!FPaths::FileExists(File) || !FFileHelper::LoadFileToArray(Compressed, *File))
		{
			GMissing.Add(Rel);
			UE_LOG(LogTemp, Warning, TEXT("Kenney UI missing: %s"), *File);
			return nullptr;
		}

		IImageWrapperModule& Images = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const TSharedPtr<IImageWrapper> Wrapper = Images.CreateImageWrapper(EImageFormat::PNG);
		TArray<uint8> Raw;
		if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Compressed.GetData(), Compressed.Num())
			|| !Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
		{
			GMissing.Add(Rel);
			UE_LOG(LogTemp, Warning, TEXT("Kenney UI decode failed: %s"), *File);
			return nullptr;
		}

		const int32 W = Wrapper->GetWidth();
		const int32 H = Wrapper->GetHeight();
		UTexture2D* Texture = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		{
			GMissing.Add(Rel);
			return nullptr;
		}

		Texture->SRGB = true;
		Texture->Filter = TF_Default;
		Texture->NeverStream = true;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;

		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		void* Dest = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, Raw.GetData(), Raw.Num());
		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		Texture->AddToRoot();
		GTextures.Add(Rel, Texture);
		return Texture;
	}

	FSlateBrush MakeBoxBrush(UTexture2D* Texture)
	{
		FSlateBrush Brush;
		if (!Texture)
		{
			return Brush;
		}
		const float W = FMath::Max(Texture->GetSizeX(), 1);
		const float H = FMath::Max(Texture->GetSizeY(), 1);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Tiling = ESlateBrushTileType::NoTile;
		Brush.ImageSize = FVector2D(W, H);
		Brush.Margin = FMargin(16.f / W, 16.f / H);
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.SetResourceObject(Texture);
		return Brush;
	}

	FSlateBrush MakeImageBrush(UTexture2D* Texture)
	{
		FSlateBrush Brush;
		if (!Texture)
		{
			return Brush;
		}
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.SetResourceObject(Texture);
		return Brush;
	}

	const TCHAR* ColorFolder(EBoxUiButton Kind)
	{
		switch (Kind)
		{
		case EBoxUiButton::Yellow: return TEXT("Yellow");
		case EBoxUiButton::Green: return TEXT("Green");
		case EBoxUiButton::Red: return TEXT("Red");
		case EBoxUiButton::Grey: return TEXT("Grey");
		default: return TEXT("Blue");
		}
	}

	UTexture2D* RectButton(EBoxUiButton Kind, const TCHAR* File)
	{
		return LoadPng(FString::Printf(TEXT("UIPack/PNG/%s/Default/%s"), ColorFolder(Kind), File));
	}

	FLinearColor FallbackFill(EBoxUiButton Kind)
	{
		switch (Kind)
		{
		case EBoxUiButton::Yellow: return FLinearColor(0.86f, 0.68f, 0.22f, 1.f);
		case EBoxUiButton::Green: return FLinearColor(0.28f, 0.62f, 0.32f, 1.f);
		case EBoxUiButton::Red: return FLinearColor(0.72f, 0.28f, 0.24f, 1.f);
		case EBoxUiButton::Grey: return FLinearColor(0.55f, 0.55f, 0.55f, 1.f);
		default: return FLinearColor(0.22f, 0.48f, 0.78f, 1.f);
		}
	}
}

bool BoxKenneyUi::IsAvailable()
{
	return FPaths::FileExists(PackFile(TEXT("UIPack/PNG/Blue/Default/button_rectangle_depth_flat.png")));
}

UFont* BoxKenneyUi::FontObject()
{
	if (bFontTried)
	{
		return GFont.Get();
	}
	bFontTried = true;

	const FString Ttf = PackFile(TEXT("UIPack/Font/Kenney Future.ttf"));
	if (!FPaths::FileExists(Ttf))
	{
		return nullptr;
	}

	UFont* Font = NewObject<UFont>(GetTransientPackage(), NAME_None, RF_Transient);
	Font->FontCacheType = EFontCacheType::Runtime;
	Font->CompositeFont.DefaultTypeface.AppendFont(
		TEXT("Regular"),
		Ttf,
		EFontHinting::Default,
		EFontLoadingPolicy::LazyLoad);

	const TSharedRef<const FCompositeFont> Default = FCoreStyle::GetDefaultFont();
	Font->CompositeFont.FallbackTypeface = Default->FallbackTypeface;
	Font->CompositeFont.SubTypefaces = Default->SubTypefaces;
	Font->AddToRoot();
	GFont = Font;
	return Font;
}

FLinearColor BoxKenneyUi::LabelColor(EBoxUiButton Kind)
{
	switch (Kind)
	{
	case EBoxUiButton::Yellow:
	case EBoxUiButton::Grey:
		return FLinearColor(0.22f, 0.14f, 0.09f, 1.f);
	default:
		return FLinearColor(0.98f, 0.98f, 0.95f, 1.f);
	}
}

void BoxKenneyUi::ApplyButton(UButton* Button, EBoxUiButton Kind)
{
	if (!Button)
	{
		return;
	}

	UTexture2D* Face = RectButton(Kind, TEXT("button_rectangle_depth_flat.png"));
	if (!Face)
	{
		Button->SetBackgroundColor(FallbackFill(Kind));
		return;
	}

	const FSlateBrush FaceBrush = MakeBoxBrush(Face);
	FButtonStyle Style;
	Style.SetNormal(FaceBrush);
	Style.SetHovered(FaceBrush);
	Style.SetPressed(FaceBrush);
	Style.SetDisabled(FaceBrush);
	Style.NormalPadding = FMargin(16.f, 10.f);
	Style.PressedPadding = FMargin(16.f, 12.f, 16.f, 8.f);
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
	if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
	{
		Label->SetColorAndOpacity(FSlateColor(LabelColor(Kind)));
	}
}

void BoxKenneyUi::ApplyPanel(UBorder* Border)
{
	if (!Border)
	{
		return;
	}
	UTexture2D* Panel = LoadPng(TEXT("UIPack/PNG/Extra/Default/input_rectangle.png"));
	if (!Panel)
	{
		return;
	}
	Border->SetBrush(MakeBoxBrush(Panel));
	Border->SetBrushColor(FLinearColor::White);
}

void BoxKenneyUi::ApplySlider(USlider* Slider)
{
	if (!Slider)
	{
		return;
	}
	UTexture2D* Bar = LoadPng(TEXT("UIPack/PNG/Grey/Default/slide_horizontal_grey.png"));
	UTexture2D* Thumb = LoadPng(TEXT("UIPack/PNG/Grey/Default/slide_hangle.png"));
	if (!Bar || !Thumb)
	{
		return;
	}

	FSliderStyle Style = Slider->GetWidgetStyle();
	const FSlateBrush BarBrush = MakeImageBrush(Bar);
	const FSlateBrush ThumbBrush = MakeImageBrush(Thumb);
	Style.SetNormalBarImage(BarBrush);
	Style.SetHoveredBarImage(BarBrush);
	Style.SetDisabledBarImage(BarBrush);
	Style.SetNormalThumbImage(ThumbBrush);
	Style.SetHoveredThumbImage(ThumbBrush);
	Style.SetDisabledThumbImage(ThumbBrush);
	Style.BarThickness = 14.f;
	Slider->SetWidgetStyle(Style);
}

UTexture2D* BoxKenneyUi::Icon(const TCHAR* FileName)
{
	return LoadPng(FString::Printf(TEXT("GameIcons/PNG/White/2x/%s"), FileName));
}
