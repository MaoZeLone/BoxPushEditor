#pragma once

#include "CoreMinimal.h"
#include "Game/BoxProjectSettings.h"

/** 运行时内容路径。项目设置里填了就用设置，空着用这里的默认。 */
namespace BoxAssetPaths
{
	inline FString FromSetting(const FSoftObjectPath& Path, const TCHAR* Fallback)
	{
		const FString Text = Path.ToString();
		return Text.IsEmpty() ? FString(Fallback) : Text;
	}

	inline FString FromDirectory(const FString& Path, const TCHAR* Fallback)
	{
		FString Text = Path.IsEmpty() ? FString(Fallback) : Path;
		while (Text.EndsWith(TEXT("/")))
		{
			Text.LeftChopInline(1);
		}
		return Text;
	}

	inline const UBoxProjectSettings* Settings()
	{
		return GetDefault<UBoxProjectSettings>();
	}

	inline FString LevelCatalog()
	{
		return FromSetting(Settings()->LevelCatalog, TEXT("/Game/Data/DT_LevelCatalog.DT_LevelCatalog"));
	}
	inline FString StartupLevel()
	{
		return FromSetting(Settings()->StartupLevel, TEXT("/Game/Data/Levels/BuiltIn/DA_Level_01.DA_Level_01"));
	}
	inline FString LevelDirectory()
	{
		return FromDirectory(Settings()->LevelDirectory, TEXT("/Game/Data/Levels/BuiltIn"));
	}
	inline FString PlayerDef()
	{
		return FromSetting(Settings()->PlayerDef, TEXT("/Game/Data/Characters/DA_Player.DA_Player"));
	}
	inline FString PlayerSprite()
	{
		return TEXT("/Game/Data/Characters/DA_PlayerSprite.DA_PlayerSprite");
	}
	inline FString PlayerBlueprint()
	{
		return FromSetting(Settings()->PlayerBlueprint, TEXT("/Game/Data/Characters/BP_Player.BP_Player_C"));
	}
	inline FString PlayerInputConfig()
	{
		return FromSetting(Settings()->PlayerInputConfig, TEXT("/Game/Data/Characters/Input/DA_InputConfig_Player.DA_InputConfig_Player"));
	}
	inline FString PlayerActionSet()
	{
		return FromSetting(Settings()->PlayerActionSet, TEXT("/Game/Data/Characters/DA_ActionSet_Player.DA_ActionSet_Player"));
	}
	inline FString PlayerTagRelationships()
	{
		return FromSetting(Settings()->PlayerTagRelationships, TEXT("/Game/Data/Characters/DA_AbilityTagRelationships_Player.DA_AbilityTagRelationships_Player"));
	}
	inline FString TypeDisplay()
	{
		return FromSetting(Settings()->TypeDisplay, TEXT("/Game/Data/DT_TypeDisplay.DT_TypeDisplay"));
	}
	inline FString PlayMap()
	{
		return FromSetting(Settings()->PlayMap, TEXT("/Game/Maps/M_Play.M_Play"));
	}
	inline FString MenuMap()
	{
		return FromSetting(Settings()->MenuMap, TEXT("/Game/Maps/M_Menu.M_Menu"));
	}

	inline FString TerrainObject(const FString& TerrainId)
	{
		const FString Dir = FromDirectory(Settings()->TerrainDirectory, TEXT("/Game/Data/Terrain"));
		return FString::Printf(TEXT("%s/DA_Terrain_%s.DA_Terrain_%s"), *Dir, *TerrainId, *TerrainId);
	}

	inline FString InteractableObject(const FString& DefinitionId)
	{
		const FString Dir = FromDirectory(Settings()->InteractableDirectory, TEXT("/Game/Data/Interactables"));
		return FString::Printf(TEXT("%s/DA_%s.DA_%s"), *Dir, *DefinitionId, *DefinitionId);
	}
}
