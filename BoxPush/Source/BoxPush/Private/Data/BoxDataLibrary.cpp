#include "Data/BoxDataLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogBoxData, Log, All);

namespace BoxDataPrivate
{
	void Warn(const TCHAR* What, const FString& Detail)
	{
		UE_LOG(LogBoxData, Warning, TEXT("BoxDataLibrary.%s 失败：%s"), What, *Detail);
	}

	UObject* NewStandaloneAsset(UClass* Class, const FString& PackagePath)
	{
#if WITH_EDITOR
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		if (AssetName.IsEmpty() || !Class)
		{
			return nullptr;
		}
		UPackage* Package = CreatePackage(*PackagePath);
		UObject* Asset = NewObject<UObject>(Package, Class, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		return Asset;
#else
		return nullptr;
#endif
	}
}

FGameplayTag UBoxDataLibrary::MakeGameplayTag(FName TagName, bool bErrorIfNotFound)
{
	if (TagName.IsNone())
	{
		return FGameplayTag();
	}
	return FGameplayTag::RequestGameplayTag(TagName, bErrorIfNotFound);
}

UObject* UBoxDataLibrary::CreateOrLoadAsset(TSubclassOf<UObject> AssetClass, const FString& PackagePath)
{
	if (!AssetClass || PackagePath.IsEmpty())
	{
		BoxDataPrivate::Warn(TEXT("CreateOrLoadAsset"), TEXT("Class 或路径为空"));
		return nullptr;
	}
	if (UObject* Existing = StaticLoadObject(AssetClass, nullptr, *PackagePath))
	{
		return Existing;
	}
	return BoxDataPrivate::NewStandaloneAsset(AssetClass, PackagePath);
}

UDataTable* UBoxDataLibrary::CreateOrLoadDataTable(UScriptStruct* RowStruct, const FString& PackagePath)
{
	if (!RowStruct || PackagePath.IsEmpty())
	{
		BoxDataPrivate::Warn(TEXT("CreateOrLoadDataTable"), TEXT("RowStruct 或路径为空"));
		return nullptr;
	}
	if (UDataTable* Existing = LoadObject<UDataTable>(nullptr, *PackagePath))
	{
		return Existing;
	}
	UDataTable* Table = Cast<UDataTable>(BoxDataPrivate::NewStandaloneAsset(UDataTable::StaticClass(), PackagePath));
	if (Table)
	{
		Table->RowStruct = RowStruct;
	}
	return Table;
}

bool UBoxDataLibrary::SaveAsset(UObject* Asset)
{
	if (!Asset)
	{
		return false;
	}
#if WITH_EDITOR
	UPackage* Package = Asset->GetOutermost();
	FString Filename;
	if (!FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), Filename, FPackageName::GetAssetPackageExtension()))
	{
		BoxDataPrivate::Warn(TEXT("SaveAsset"), Package->GetName());
		return false;
	}
	Asset->MarkPackageDirty();
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GWarn;
	return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
#else
	return false;
#endif
}

bool UBoxDataLibrary::SetTableRowExportText(UDataTable* Table, FName RowName, const FString& ExportText)
{
	if (!Table || !Table->RowStruct || RowName.IsNone())
	{
		BoxDataPrivate::Warn(TEXT("SetTableRowExportText"), TEXT("Table / RowStruct / RowName 无效"));
		return false;
	}
	UScriptStruct* RowStruct = Table->RowStruct;
	if (uint8* Existing = Table->FindRowUnchecked(RowName))
	{
		const TCHAR* Result = RowStruct->ImportText(*ExportText, Existing, Table, PPF_None, GWarn, FString(TEXT("BoxDataLibrary")));
		if (!Result)
		{
			BoxDataPrivate::Warn(TEXT("SetTableRowExportText"), RowName.ToString());
			return false;
		}
		Table->HandleDataTableChanged(RowName);
		Table->MarkPackageDirty();
		return true;
	}

	const int32 Size = RowStruct->GetStructureSize();
	TArray<uint8> Buffer;
	Buffer.AddUninitialized(Size);
	RowStruct->InitializeStruct(Buffer.GetData());
	const TCHAR* Result = RowStruct->ImportText(*ExportText, Buffer.GetData(), Table, PPF_None, GWarn, FString(TEXT("BoxDataLibrary")));
	if (!Result)
	{
		RowStruct->DestroyStruct(Buffer.GetData());
		BoxDataPrivate::Warn(TEXT("SetTableRowExportText"), RowName.ToString());
		return false;
	}
	Table->AddRow(RowName, Buffer.GetData(), RowStruct);
	RowStruct->DestroyStruct(Buffer.GetData());
	Table->MarkPackageDirty();
	return true;
}

FString UBoxDataLibrary::GetTableRowExportText(const UDataTable* Table, FName RowName)
{
	if (!Table || !Table->RowStruct)
	{
		return FString();
	}
	const uint8* Row = const_cast<UDataTable*>(Table)->FindRowUnchecked(RowName);
	if (!Row)
	{
		return FString();
	}
	FString Out;
	Table->RowStruct->ExportText(Out, Row, nullptr, const_cast<UDataTable*>(Table), PPF_None, nullptr);
	return Out;
}

bool UBoxDataLibrary::RemoveTableRow(UDataTable* Table, FName RowName)
{
	if (!Table || !Table->GetRowMap().Contains(RowName))
	{
		return false;
	}
	Table->RemoveRow(RowName);
	Table->MarkPackageDirty();
	return true;
}

TArray<FName> UBoxDataLibrary::GetTableRowNames(const UDataTable* Table)
{
	return Table ? Table->GetRowNames() : TArray<FName>();
}

bool UBoxDataLibrary::HasTableRow(const UDataTable* Table, FName RowName)
{
	return Table && Table->GetRowMap().Contains(RowName);
}
