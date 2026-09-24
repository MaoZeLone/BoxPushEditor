#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerSpriteDef.generated.h"

class UTexture2D;

/** 一帧角色图。贴图空着就是没配。宽高大于 0 时从这张贴图上裁。 */
USTRUCT(BlueprintType)
struct FBoxAtlasFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "贴图"))
	TSoftObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceW = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 SourceH = 0;

	bool IsSet() const;
};

/** 一个朝向：站立、走路、推。FacingSteps 0 上、1 屏幕右、2 下、3 屏幕左。 */
USTRUCT(BlueprintType)
struct FPlayerFacingLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "站立"))
	FBoxAtlasFrame Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "走路"))
	TArray<FBoxAtlasFrame> Walk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "推"))
	TArray<FBoxAtlasFrame> Push;
};

/** 角色表现。DA_Player 引用这一份，不把帧直接写在玩法定义上。 */
UCLASS(BlueprintType)
class BOXPUSH_API UPlayerSpriteDef : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "朝上"))
	FPlayerFacingLook North;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "屏幕右"))
	FPlayerFacingLook ScreenRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "朝下"))
	FPlayerFacingLook South;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (DisplayName = "屏幕左"))
	FPlayerFacingLook ScreenLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0.05", DisplayName = "换帧间隔"))
	float FrameInterval = 0.16f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	const FPlayerFacingLook& GetFacing(int32 FacingSteps) const;
	FBoxAtlasFrame PickFrame(int32 FacingSteps, bool bPushing, bool bWalking, int32 FrameIndex) const;
};
