#include "Data/TerrainDef.h"

#include "BoxPushTags.h"
#include "Engine/AssetManager.h"

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
	if (!UAssetManager::IsInitialized())
	{
		return nullptr;
	}
	const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(FPrimaryAssetId(TEXT("TerrainDef"), InTerrainId));
	return Path.IsValid() ? Cast<UTerrainDef>(Path.TryLoad()) : nullptr;
}

UTerrainDef* UTerrainDef::FindByCell(ETerrainCell Cell)
{
	if (!UAssetManager::IsInitialized())
	{
		return nullptr;
	}
	TArray<FPrimaryAssetId> Ids;
	UAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("TerrainDef")), Ids);
	for (const FPrimaryAssetId& Id : Ids)
	{
		UTerrainDef* Def = LoadById(Id.PrimaryAssetName);
		if (Def && Def->Cell == Cell)
		{
			return Def;
		}
	}
	return nullptr;
}

void UTerrainDef::ApplyOfficialDefaults(FName InTerrainId)
{
	TerrainId = InTerrainId;
	if (InTerrainId == TEXT("Floor"))
	{
		DisplayName = FText::FromString(TEXT("地板"));
		Type = TAG_Type_Terrain_Floor;
		Cell = ETerrainCell::Floor;
	}
	else if (InTerrainId == TEXT("Wall"))
	{
		DisplayName = FText::FromString(TEXT("墙"));
		Type = TAG_Type_Terrain_Wall;
		Cell = ETerrainCell::Wall;
	}
	else if (InTerrainId == TEXT("Empty"))
	{
		DisplayName = FText::FromString(TEXT("空洞"));
		Type = TAG_Type_Terrain_Empty;
		Cell = ETerrainCell::Empty;
	}
	else
	{
		DisplayName = FText::FromName(InTerrainId);
		Type = TAG_Type_Terrain;
		Cell = ETerrainCell::Floor;
	}
}
