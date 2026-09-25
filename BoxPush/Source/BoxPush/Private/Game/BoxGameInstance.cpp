#include "Game/BoxGameInstance.h"

#include "AudioDevice.h"
#include "Data/BoxAssetPaths.h"
#include "Data/LevelCatalog.h"
#include "Data/LevelData.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Game/BoxSaveGame.h"
#include "Kismet/GameplayStatics.h"

void UBoxGameInstance::Init()
{
	Super::Init();
	LevelCatalog = TSoftObjectPtr<UDataTable>(FSoftObjectPath(BoxAssetPaths::LevelCatalog()));
	PlayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(BoxAssetPaths::PlayMap()));
	SelectMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(BoxAssetPaths::MenuMap()));
	LoadCatalog();
	LoadOrCreateSave();
	ApplyMasterVolume();
}

void UBoxGameInstance::Shutdown()
{
	WriteSave();
	Super::Shutdown();
}

void UBoxGameInstance::LoadCatalog()
{
	if (!Catalog)
	{
		Catalog = LevelCatalog.LoadSynchronous();
	}
	if (!Catalog)
	{
		Catalog = LoadObject<UDataTable>(nullptr, *BoxAssetPaths::LevelCatalog());
	}
}

UDataTable* UBoxGameInstance::GetCatalog() const
{
	if (!Catalog)
	{
		const_cast<UBoxGameInstance*>(this)->LoadCatalog();
	}
	return Catalog;
}

void UBoxGameInstance::LoadOrCreateSave()
{
	const FString Slot = BoxAssetPaths::SaveSlot();
	if (!Slot.IsEmpty() && UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		SaveData = Cast<UBoxSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	}
	if (!SaveData)
	{
		SaveData = Cast<UBoxSaveGame>(UGameplayStatics::CreateSaveGameObject(UBoxSaveGame::StaticClass()));
	}
}

void UBoxGameInstance::WriteSave() const
{
	const FString Slot = BoxAssetPaths::SaveSlot();
	if (SaveData && !Slot.IsEmpty())
	{
		UGameplayStatics::SaveGameToSlot(SaveData, Slot, 0);
	}
}

void UBoxGameInstance::ApplyMasterVolume() const
{
	const float Volume = GetMasterVolume();
	if (GEngine)
	{
		if (FAudioDevice* Device = GEngine->GetMainAudioDeviceRaw())
		{
			Device->SetTransientPrimaryVolume(Volume);
		}
	}
}

void UBoxGameInstance::OpenMenuMap()
{
	TSoftObjectPtr<UWorld> Map = SelectMap;
	if (Map.IsNull())
	{
		Map = TSoftObjectPtr<UWorld>(FSoftObjectPath(BoxAssetPaths::MenuMap()));
	}
	if (!Map.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Map);
	}
}

FBoxPlayRequest UBoxGameInstance::ConsumePlayRequest()
{
	FBoxPlayRequest Request = PendingPlay;
	PendingPlay.Reset();
	return Request;
}

ULevelData* UBoxGameInstance::FindListedLevel(FName LevelId) const
{
	FLevelCatalogRow Row;
	if (!ULevelCatalogLibrary::FindRow(GetCatalog(), LevelId, Row))
	{
		return nullptr;
	}
	return Row.LevelAsset.LoadSynchronous();
}

ULevelData* UBoxGameInstance::ResolveLevel(const FBoxPlayRequest& Request) const
{
	if (ULevelData* Loaded = Request.Level.LoadSynchronous())
	{
		return Loaded;
	}
	if (!Request.LevelId.IsNone())
	{
		return FindListedLevel(Request.LevelId);
	}
	return nullptr;
}

void UBoxGameInstance::PlayLevel(FName LevelId, bool bPlaytest)
{
	LoadCatalog();
	PendingPlay.Reset();
	PendingPlay.LevelId = LevelId;
	PendingPlay.Level = FindListedLevel(LevelId);
	PendingPlay.bPlaytest = bPlaytest;
	if (PendingPlay.Level.IsNull() && LevelId.IsNone())
	{
		return;
	}
	TSoftObjectPtr<UWorld> Map = PlayMap;
	if (Map.IsNull())
	{
		Map = TSoftObjectPtr<UWorld>(FSoftObjectPath(BoxAssetPaths::PlayMap()));
	}
	if (!Map.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Map);
	}
}

void UBoxGameInstance::QueuePlaytest(ULevelData* Level)
{
	PendingPlay.Reset();
	PendingPlay.bPlaytest = true;
	if (Level)
	{
		PendingPlay.Level = Level;
		PendingPlay.LevelId = Level->LevelId;
	}
}

void UBoxGameInstance::OpenSelect()
{
	PendingPlay.Reset();
	PendingMenuScreen = EBoxMenuScreen::Select;
	OpenMenuMap();
}

void UBoxGameInstance::OpenMenu()
{
	PendingPlay.Reset();
	PendingMenuScreen = EBoxMenuScreen::Main;
	OpenMenuMap();
}

EBoxMenuScreen UBoxGameInstance::ConsumeMenuScreen()
{
	const EBoxMenuScreen Screen = PendingMenuScreen;
	PendingMenuScreen = EBoxMenuScreen::Main;
	return Screen;
}

bool UBoxGameInstance::IsLevelUnlocked(FName LevelId) const
{
	const TArray<FLevelCatalogRow> Listed = ULevelCatalogLibrary::GetListedRows(GetCatalog());
	if (Listed.Num() == 0 || LevelId.IsNone())
	{
		return false;
	}
	if (Listed[0].LevelId == LevelId)
	{
		return true;
	}
	for (int32 Index = 1; Index < Listed.Num(); ++Index)
	{
		if (Listed[Index].LevelId == LevelId)
		{
			return GetLevelRecord(Listed[Index - 1].LevelId).bCleared;
		}
	}
	return false;
}

FBoxLevelRecord UBoxGameInstance::GetLevelRecord(FName LevelId) const
{
	return SaveData ? SaveData->GetRecord(LevelId) : FBoxLevelRecord();
}

TArray<FBoxSelectEntry> UBoxGameInstance::GetSelectEntries() const
{
	TArray<FBoxSelectEntry> Entries;
	const TArray<FLevelCatalogRow> Listed = ULevelCatalogLibrary::GetListedRows(GetCatalog());
	for (const FLevelCatalogRow& Row : Listed)
	{
		FBoxSelectEntry Entry;
		Entry.LevelId = Row.LevelId;
		Entry.SortOrder = Row.SortOrder;
		Entry.bUnlocked = IsLevelUnlocked(Row.LevelId);
		const FBoxLevelRecord Record = GetLevelRecord(Row.LevelId);
		Entry.bCleared = Record.bCleared;
		if (const ULevelData* Level = Row.LevelAsset.LoadSynchronous())
		{
			Entry.DisplayName = Level->DisplayName.IsEmpty()
				? FText::FromString(TEXT("未命名关卡"))
				: Level->DisplayName;
		}
		else
		{
			Entry.DisplayName = FText::FromString(TEXT("未命名关卡"));
		}
		Entries.Add(Entry);
	}
	return Entries;
}

void UBoxGameInstance::RecordClear(FName LevelId)
{
	if (!SaveData || LevelId.IsNone())
	{
		return;
	}
	SaveData->RecordClear(LevelId);
	WriteSave();
}

void UBoxGameInstance::SetMasterVolume(float Volume)
{
	if (!SaveData)
	{
		LoadOrCreateSave();
	}
	if (SaveData)
	{
		SaveData->MasterVolume = FMath::Clamp(Volume, 0.f, 1.f);
		WriteSave();
	}
	ApplyMasterVolume();
}

float UBoxGameInstance::GetMasterVolume() const
{
	return SaveData ? FMath::Clamp(SaveData->MasterVolume, 0.f, 1.f) : 1.f;
}

FName UBoxGameInstance::GetNextListedLevelId(FName CurrentLevelId) const
{
	return ULevelCatalogLibrary::GetNextListedLevelId(GetCatalog(), CurrentLevelId);
}
