#include "Match/BoxSokobanSheet.h"

#include "Data/PlayerDef.h"
#include "Engine/Texture2D.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Match/BoxGrid.h"

namespace BoxSokoban
{
	static TMap<uint64, TObjectPtr<UTexture2D>> GCrops;

	static UStaticMesh* PlaneMesh()
	{
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	}

	static UMaterialInterface* SpriteMaterial()
	{
		return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Masked.Widget3DPassThrough_Masked"));
	}

	static void FitScale(UStaticMeshComponent* Mesh, const FBoxSpriteRect& Rect, float WorldSpan)
	{
		const float MaxSide = FMath::Max(FMath::Max(Rect.W, Rect.H), 1);
		const float Unit = WorldSpan / 100.f;
		Mesh->SetRelativeScale3D(FVector(Unit * Rect.W / MaxSide, Unit * Rect.H / MaxSide, 1.f));
	}

	float TileSpan()
	{
		return BoxGrid::CellSize * 0.975f;
	}

	static UTexture2D* CropBgra(
		const uint8* Bgra,
		int32 SrcW,
		int32 SrcH,
		const FBoxSpriteRect& Rect,
		bool bForBoard,
		uint64 CacheKey)
	{
		if (!Bgra || Rect.W <= 0 || Rect.H <= 0)
		{
			return nullptr;
		}
		if (Rect.X < 0 || Rect.Y < 0 || Rect.X + Rect.W > SrcW || Rect.Y + Rect.H > SrcH)
		{
			return nullptr;
		}

		if (TObjectPtr<UTexture2D>* Found = GCrops.Find(CacheKey))
		{
			if (*Found && IsValid(*Found))
			{
				return Found->Get();
			}
		}

		constexpr int32 Pad = 1;
		const int32 TexW = Rect.W + Pad * 2;
		const int32 TexH = Rect.H + Pad * 2;
		UTexture2D* Texture = UTexture2D::CreateTransient(TexW, TexH, PF_B8G8R8A8);
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
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;

		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		uint8* Dest = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
		FMemory::Memzero(Dest, TexW * TexH * 4);
		const int32 SrcPitch = SrcW * 4;
		for (int32 Row = 0; Row < Rect.H; ++Row)
		{
			const int32 SrcRow = bForBoard ? (Rect.H - 1 - Row) : Row;
			const uint8* SrcLine = Bgra + (Rect.Y + SrcRow) * SrcPitch + Rect.X * 4;
			uint8* DstLine = Dest + (Row + Pad) * TexW * 4 + Pad * 4;
			if (!bForBoard)
			{
				FMemory::Memcpy(DstLine, SrcLine, Rect.W * 4);
				continue;
			}
			for (int32 Col = 0; Col < Rect.W; ++Col)
			{
				const int32 SrcCol = Rect.W - 1 - Col;
				FMemory::Memcpy(DstLine + Col * 4, SrcLine + SrcCol * 4, 4);
			}
		}
		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		Texture->AddToRoot();
		GCrops.Add(CacheKey, Texture);
		return Texture;
	}

	UTexture2D* ResolveSprite(UTexture2D* Texture, int32 SourceX, int32 SourceY, int32 SourceW, int32 SourceH, bool bForBoard)
	{
		if (!Texture)
		{
			return nullptr;
		}
		if (SourceW <= 0 || SourceH <= 0)
		{
			return Texture;
		}

		FImage Image;
		if (!FImageUtils::GetTexture2DSourceImage(Texture, Image))
		{
			UE_LOG(LogTemp, Warning, TEXT("Sprite crop failed, texture source is unavailable: %s"), *Texture->GetPathName());
			return nullptr;
		}
		FImage Bgra;
		Image.CopyTo(Bgra, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		const TArrayView64<const FColor> Pixels = Bgra.AsBGRA8();
		if (Pixels.Num() < Bgra.SizeX * Bgra.SizeY)
		{
			return nullptr;
		}

		FBoxSpriteRect Rect;
		Rect.X = SourceX;
		Rect.Y = SourceY;
		Rect.W = SourceW;
		Rect.H = SourceH;
		const FGuid Id = Texture->GetLightingGuid();
		const uint64 Key = (uint64)(uint32)Rect.X
			| ((uint64)(uint32)Rect.Y << 16)
			| ((uint64)(uint32)Rect.W << 32)
			| ((uint64)(uint32)Rect.H << 48)
			^ ((uint64)Id.A << 1)
			^ ((uint64)Id.B << 17)
			^ ((uint64)Texture->GetUniqueID() << 3)
			| (bForBoard ? (1ull << 63) : 0ull);
		return CropBgra(reinterpret_cast<const uint8*>(Pixels.GetData()), Bgra.SizeX, Bgra.SizeY, Rect, bForBoard, Key);
	}

	void ApplyAtlasFrame(UStaticMeshComponent* Mesh, const FBoxAtlasFrame& Frame, float WorldSpan)
	{
		if (!Mesh)
		{
			return;
		}
		UTexture2D* Texture = ResolveSprite(
			Frame.Texture.LoadSynchronous(), Frame.SourceX, Frame.SourceY, Frame.SourceW, Frame.SourceH, true);
		if (Texture)
		{
			Mesh->SetHiddenInGame(false);
			Mesh->SetVisibility(true);
			ApplySpriteTexture(Mesh, Texture, WorldSpan);
			return;
		}
		Mesh->SetHiddenInGame(true);
		Mesh->SetVisibility(false);
	}

	void ApplySpriteTexture(UStaticMeshComponent* Mesh, UTexture2D* Texture, float WorldSpan)
	{
		if (!Mesh || !Texture)
		{
			return;
		}
		UMaterialInterface* Base = SpriteMaterial();
		UStaticMesh* Plane = PlaneMesh();
		if (!Base || !Plane)
		{
			return;
		}

		Mesh->SetStaticMesh(Plane);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetUsingAbsoluteLocation(false);
		Mesh->SetUsingAbsoluteRotation(true);
		Mesh->SetWorldRotation(FRotator::ZeroRotator);
		FBoxSpriteRect Size;
		Size.W = FMath::Max(Texture->GetSizeX(), 1);
		Size.H = FMath::Max(Texture->GetSizeY(), 1);
		FitScale(Mesh, Size, WorldSpan);

		UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		if (!Mid || Mid->Parent != Base)
		{
			Mid = UMaterialInstanceDynamic::Create(Base, Mesh);
			Mesh->SetMaterial(0, Mid);
		}
		Mid->SetTextureParameterValue(TEXT("SlateUI"), Texture);
	}

	static UStaticMeshComponent* MakeSpriteMesh(AActor* Owner, USceneComponent* Parent)
	{
		if (!Owner || !Parent)
		{
			return nullptr;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
		Mesh->SetupAttachment(Parent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetHiddenInGame(false);
		Mesh->SetVisibility(true);
		return Mesh;
	}

	UStaticMeshComponent* SpawnSpriteTexture(
		AActor* Owner,
		USceneComponent* Parent,
		UTexture2D* Texture,
		float WorldSpan)
	{
		UStaticMeshComponent* Mesh = MakeSpriteMesh(Owner, Parent);
		if (!Mesh || !Texture)
		{
			return nullptr;
		}
		ApplySpriteTexture(Mesh, Texture, WorldSpan);
		Mesh->RegisterComponent();
		return Mesh;
	}
}
