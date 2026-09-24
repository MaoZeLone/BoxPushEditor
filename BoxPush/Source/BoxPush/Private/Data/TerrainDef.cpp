#include "Data/TerrainDef.h"

#include "BoxPushTags.h"
#include "Data/BoxAssetPaths.h"
FPrimaryAssetId UTerrainDef::GetPrimaryAssetId() const
{
	const FName Id = TerrainId.IsNone() ? GetFName() : TerrainId;
	return FPrimaryAssetId(TEXT("TerrainDef"), Id);
}

UTerrainDef* UTerrainDef::LoadById(FName InTerrainId)
{
	if (InTerrainId.IsNone())
	{
		return nullptr;
	}
	return LoadObject<UTerrainDef>(nullptr, *BoxAssetPaths::TerrainObject(InTerrainId.ToString()));
}

void UTerrainDef::ApplyOfficialDefaults(FName InTerrainId)
{
	TerrainId = InTerrainId;
	if (InTerrainId == TEXT("Floor"))
	{
		DisplayName = FText::FromString(TEXT("地板"));
		Type = TAG_Type_Terrain_Floor;
		Cell = ETerrainCell::Floor;
		PaletteColor = FLinearColor(0.79f, 0.64f, 0.43f);
		Sprite = nullptr;
	}
	else if (InTerrainId == TEXT("Wall"))
	{
		DisplayName = FText::FromString(TEXT("墙"));
		Type = TAG_Type_Terrain_Wall;
		Cell = ETerrainCell::Wall;
		PaletteColor = FLinearColor(0.54f, 0.40f, 0.30f);
		Sprite = nullptr;
	}
	else if (InTerrainId == TEXT("Empty"))
	{
		DisplayName = FText::FromString(TEXT("空洞"));
		Type = TAG_Type_Terrain_Empty;
		Cell = ETerrainCell::Empty;
		PaletteColor = FLinearColor(0.05f, 0.05f, 0.05f);
	}
	else
	{
		DisplayName = FText::FromName(InTerrainId);
		Type = TAG_Type_Terrain;
		Cell = ETerrainCell::Floor;
		PaletteColor = FLinearColor(0.5f, 0.5f, 0.5f);
	}
}
