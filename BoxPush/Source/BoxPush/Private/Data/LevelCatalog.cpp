#include "Data/LevelCatalog.h"

#include "Data/InteractableDef.h"
#include "Data/InteractableLogic.h"

namespace
{
	void ForEachCatalogRow(const UDataTable* Catalog, TFunctionRef<void(const FName&, const FLevelCatalogRow&)> Visitor)
	{
		if (!Catalog)
		{
			return;
		}
		Catalog->ForeachRow<FLevelCatalogRow>(TEXT("BoxPushCatalog"), Visitor);
	}

	void AddIssue(TArray<FLevelValidationIssue>& OutIssues, bool bError, const FString& Text, const TArray<FIntPoint>& Cells = {})
	{
		FLevelValidationIssue Issue;
		Issue.bError = bError;
		Issue.Message = FText::FromString(Text);
		Issue.Cells = Cells;
		OutIssues.Add(Issue);
	}
}

TArray<FLevelCatalogRow> ULevelCatalogLibrary::GetListedRows(const UDataTable* Catalog)
{
	TArray<FLevelCatalogRow> Rows;
	ForEachCatalogRow(Catalog, [&Rows](const FName& RowName, const FLevelCatalogRow& Row)
	{
		if (Row.bListed)
		{
			FLevelCatalogRow Copy = Row;
			if (Copy.LevelId.IsNone())
			{
				Copy.LevelId = RowName;
			}
			Rows.Add(Copy);
		}
	});
	Rows.Sort([](const FLevelCatalogRow& A, const FLevelCatalogRow& B)
	{
		return A.SortOrder < B.SortOrder;
	});
	return Rows;
}

bool ULevelCatalogLibrary::FindRow(const UDataTable* Catalog, FName LevelId, FLevelCatalogRow& OutRow)
{
	if (!Catalog || LevelId.IsNone())
	{
		return false;
	}
	if (const FLevelCatalogRow* Found = Catalog->FindRow<FLevelCatalogRow>(LevelId, TEXT("BoxPushFindRow")))
	{
		OutRow = *Found;
		if (OutRow.LevelId.IsNone())
		{
			OutRow.LevelId = LevelId;
		}
		return true;
	}
	return false;
}

FName ULevelCatalogLibrary::GetNextListedLevelId(const UDataTable* Catalog, FName CurrentLevelId)
{
	const TArray<FLevelCatalogRow> Listed = GetListedRows(Catalog);
	for (int32 Index = 0; Index < Listed.Num() - 1; ++Index)
	{
		if (Listed[Index].LevelId == CurrentLevelId)
		{
			return Listed[Index + 1].LevelId;
		}
	}
	return NAME_None;
}

void ULevelCatalogLibrary::ValidateCatalog(const UDataTable* Catalog, TArray<FLevelValidationIssue>& OutIssues)
{
	if (!Catalog)
	{
		AddIssue(OutIssues, true, TEXT("总关卡表为空"));
		return;
	}

	TSet<FName> SeenIds;
	ForEachCatalogRow(Catalog, [&](const FName& RowName, const FLevelCatalogRow& Row)
	{
		const FName Id = Row.LevelId.IsNone() ? RowName : Row.LevelId;
		if (Id != RowName)
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("行名 %s 与 LevelId %s 不一致"), *RowName.ToString(), *Id.ToString()));
		}
		if (SeenIds.Contains(Id))
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("总表 LevelId 重复：%s"), *Id.ToString()));
		}
		SeenIds.Add(Id);

		if (Row.LevelAsset.IsNull())
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("%s 未指定 LevelAsset"), *Id.ToString()));
			return;
		}

		if (const ULevelData* Level = Row.LevelAsset.LoadSynchronous())
		{
			if (Level->LevelId != Id)
			{
				AddIssue(OutIssues, true, FString::Printf(TEXT("%s 的 DA.LevelId 与总表不一致"), *Id.ToString()));
			}
		}
	});
}

void ULevelCatalogLibrary::ValidateLevelForListing(const ULevelData* Level, const UDataTable* Catalog, TArray<FLevelValidationIssue>& OutIssues)
{
	if (!Level)
	{
		AddIssue(OutIssues, true, TEXT("关卡资产为空"));
		return;
	}

	Level->Validate(OutIssues);

	if (Catalog)
	{
		FLevelCatalogRow Row;
		if (!FindRow(Catalog, Level->LevelId, Row))
		{
			AddIssue(OutIssues, true, TEXT("总表里没有这一关的行"));
		}
	}
}

bool ULevelCatalogLibrary::IsPushableDefinition(const UInteractableDef* Def)
{
	return Def && Def->FindLogic<UPushableLogic>() != nullptr;
}

bool ULevelCatalogLibrary::IsGoalDefinition(const UInteractableDef* Def)
{
	return Def && Def->FindLogic<UGoalLogic>() != nullptr;
}
