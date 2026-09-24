#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BoxProjectSettings.generated.h"

/** 项目设置里的外部资源路径。相对路径相对工程目录。 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "BoxPush"))
class BOXPUSH_API UBoxProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UBoxProjectSettings();

	virtual FName GetCategoryName() const override;

	/** 空则用工程旁的 Asset/UI。 */
	UPROPERTY(Config, EditAnywhere, Category = "2D", meta = (DisplayName = "界面图包目录"))
	FDirectoryPath UiPackRoot;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "关卡目录"))
	FSoftObjectPath LevelCatalog;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "开局关"))
	FSoftObjectPath StartupLevel;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "新建关卡目录"))
	FString LevelDirectory;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "玩法角色"))
	FSoftObjectPath PlayerDef;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "角色蓝图"))
	FSoftObjectPath PlayerBlueprint;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "输入配置"))
	FSoftObjectPath PlayerInputConfig;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "动作集"))
	FSoftObjectPath PlayerActionSet;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "标签关系"))
	FSoftObjectPath PlayerTagRelationships;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "类型表"))
	FSoftObjectPath TypeDisplay;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "游玩地图"))
	FSoftObjectPath PlayMap;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "菜单地图"))
	FSoftObjectPath MenuMap;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "地形目录"))
	FString TerrainDirectory;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "交互物目录"))
	FString InteractableDirectory;

	FString ResolveUiPackRoot() const;
};
