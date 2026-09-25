#pragma once

#include "CoreMinimal.h"
#include "Game/BoxProjectSettings.h"

/** 运行时内容路径。只读项目设置，不在这里写默认资产路径。 */
namespace BoxAssetPaths
{
	inline FString TrimDirectory(FString Path)
	{
		while (Path.EndsWith(TEXT("/")))
		{
			Path.LeftChopInline(1);
		}
		return Path;
	}

	inline const UBoxProjectSettings* Settings()
	{
		return GetDefault<UBoxProjectSettings>();
	}

	inline FString LevelCatalog()
	{
		return Settings()->LevelCatalog.ToString();
	}
	inline FString StartupLevel()
	{
		return Settings()->StartupLevel.ToString();
	}
	inline FString LevelDirectory()
	{
		return TrimDirectory(Settings()->LevelDirectory);
	}
	inline FString PlayerDef()
	{
		return Settings()->PlayerDef.ToString();
	}
	inline FString PlayerSprite()
	{
		return Settings()->PlayerSprite.ToString();
	}
	inline FString PlayerBlueprint()
	{
		return Settings()->PlayerBlueprint.ToString();
	}
	inline FString PlayerInputConfig()
	{
		return Settings()->PlayerInputConfig.ToString();
	}
	inline FString PlayerActionSet()
	{
		return Settings()->PlayerActionSet.ToString();
	}
	inline FString PlayerTagRelationships()
	{
		return Settings()->PlayerTagRelationships.ToString();
	}
	inline FString TypeDisplay()
	{
		return Settings()->TypeDisplay.ToString();
	}
	inline FString PlayMap()
	{
		return Settings()->PlayMap.ToString();
	}
	inline FString MenuMap()
	{
		return Settings()->MenuMap.ToString();
	}
	inline FString SaveSlot()
	{
		return Settings()->SaveSlot;
	}
}
