#include "Game/BoxProjectSettings.h"

#include "Misc/Paths.h"

UBoxProjectSettings::UBoxProjectSettings()
{
	UiPackRoot.Path = TEXT("../Asset/UI");
	LevelCatalog = FSoftObjectPath(TEXT("/Game/Data/DT_LevelCatalog.DT_LevelCatalog"));
	StartupLevel = FSoftObjectPath(TEXT("/Game/Data/Levels/BuiltIn/DA_Level_01.DA_Level_01"));
	LevelDirectory = TEXT("/Game/Data/Levels/BuiltIn");
	PlayerDef = FSoftObjectPath(TEXT("/Game/Data/Characters/DA_Player.DA_Player"));
	PlayerBlueprint = FSoftObjectPath(TEXT("/Game/Data/Characters/BP_Player.BP_Player_C"));
	PlayerInputConfig = FSoftObjectPath(TEXT("/Game/Data/Characters/Input/DA_InputConfig_Player.DA_InputConfig_Player"));
	PlayerActionSet = FSoftObjectPath(TEXT("/Game/Data/Characters/DA_ActionSet_Player.DA_ActionSet_Player"));
	PlayerTagRelationships = FSoftObjectPath(TEXT("/Game/Data/Characters/DA_AbilityTagRelationships_Player.DA_AbilityTagRelationships_Player"));
	TypeDisplay = FSoftObjectPath(TEXT("/Game/Data/DT_TypeDisplay.DT_TypeDisplay"));
	PlayMap = FSoftObjectPath(TEXT("/Game/Maps/M_Play.M_Play"));
	MenuMap = FSoftObjectPath(TEXT("/Game/Maps/M_Menu.M_Menu"));
	TerrainDirectory = TEXT("/Game/Data/Terrain");
	InteractableDirectory = TEXT("/Game/Data/Interactables");
}

FName UBoxProjectSettings::GetCategoryName() const
{
	return TEXT("Game");
}

static FString ResolveAgainstProject(const FString& Path)
{
	if (Path.IsEmpty())
	{
		return FString();
	}
	if (FPaths::IsRelative(Path))
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), Path));
	}
	return FPaths::ConvertRelativePathToFull(Path);
}

FString UBoxProjectSettings::ResolveUiPackRoot() const
{
	const FString Configured = UiPackRoot.Path.IsEmpty()
		? FString(TEXT("../Asset/UI"))
		: UiPackRoot.Path;
	return ResolveAgainstProject(Configured);
}
