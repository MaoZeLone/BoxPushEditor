#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;
class AActor;
class USceneComponent;
class UTexture2D;
struct FBoxAtlasFrame;

/** 贴图上的一块像素矩形。宽高为 0 表示用整张。 */
struct FBoxSpriteRect
{
	int32 X = 0;
	int32 Y = 0;
	int32 W = 0;
	int32 H = 0;
};

/** 俯视图块。面片平铺在格子上，不跟着相机转，避免叠在一起闪。 */
namespace BoxSokoban
{
	/** 和方块占地一样，收在一格里面，不压到隔壁。 */
	BOXPUSH_API float TileSpan();

	/** 贴图空着不画。宽高为 0 用整张；宽高大于 0 只从这张贴图裁。bForBoard 时按棋盘面片翻转。 */
	BOXPUSH_API UTexture2D* ResolveSprite(UTexture2D* Texture, int32 SourceX, int32 SourceY, int32 SourceW, int32 SourceH, bool bForBoard);
	/** 配了贴图才画。没配就藏起面片。 */
	BOXPUSH_API void ApplyAtlasFrame(UStaticMeshComponent* Mesh, const FBoxAtlasFrame& Frame, float WorldSpan);
	BOXPUSH_API void ApplySpriteTexture(UStaticMeshComponent* Mesh, UTexture2D* Texture, float WorldSpan);
	BOXPUSH_API UStaticMeshComponent* SpawnSpriteTexture(
		AActor* Owner,
		USceneComponent* Parent,
		UTexture2D* Texture,
		float WorldSpan);
}
