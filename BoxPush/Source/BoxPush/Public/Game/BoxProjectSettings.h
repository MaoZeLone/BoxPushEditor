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

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "存档槽"))
	FString SaveSlot;

	UPROPERTY(Config, EditAnywhere, Category = "内容", meta = (DisplayName = "角色表现"))
	FSoftObjectPath PlayerSprite;

	UPROPERTY(Config, EditAnywhere, Category = "棋盘", meta = (ClampMin = "1", DisplayName = "格子边长"))
	float CellSize = 200.f;

	UPROPERTY(Config, EditAnywhere, Category = "棋盘", meta = (DisplayName = "地面高度"))
	float OriginZ = 200.f;

	/** 世界边长为 0 时，面片占格子边长的这个比例。 */
	UPROPERTY(Config, EditAnywhere, Category = "棋盘", meta = (ClampMin = "0.01", ClampMax = "1", DisplayName = "贴图占格"))
	float TileFit = 0.975f;

	UPROPERTY(Config, EditAnywhere, Category = "棋盘", meta = (ClampMin = "0.01", DisplayName = "一步秒数"))
	float StepDuration = 0.36f;

	UPROPERTY(Config, EditAnywhere, Category = "镜头", meta = (DisplayName = "相机偏移"))
	FVector CameraOffset = FVector(0.f, 0.f, 3000.f);

	UPROPERTY(Config, EditAnywhere, Category = "镜头", meta = (DisplayName = "相机旋转"))
	FRotator CameraRotation = FRotator(-90.f, 90.f, 0.f);

	UPROPERTY(Config, EditAnywhere, Category = "镜头", meta = (ClampMin = "1", DisplayName = "视野"))
	float CameraFOV = 50.f;

	FString ResolveUiPackRoot() const;
};
