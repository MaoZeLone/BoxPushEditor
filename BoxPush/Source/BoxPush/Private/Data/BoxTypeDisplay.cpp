#include "Data/BoxTypeDisplay.h"

#include "BoxPushTags.h"
#include "Data/BoxAssetPaths.h"
#include "Engine/DataTable.h"

const UDataTable* UBoxTypeDisplayLibrary::LoadTable()
{
	return LoadObject<UDataTable>(nullptr, *BoxAssetPaths::TypeDisplay());
}

FText UBoxTypeDisplayLibrary::GetDisplayName(FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		return FText::FromString(TEXT("游戏"));
	}

	FText Found;
	if (const UDataTable* Table = LoadTable())
	{
		Table->ForeachRow<FBoxTypeDisplayRow>(TEXT("BoxTypeDisplay"), [&](const FName& RowName, const FBoxTypeDisplayRow& Row)
		{
			if (Found.IsEmpty() && (Row.Tag == Tag || RowName == Tag.GetTagName()))
			{
				Found = Row.DisplayName;
			}
		});
	}
	if (!Found.IsEmpty())
	{
		return Found;
	}

	FString Leaf = Tag.ToString();
	int32 Dot = INDEX_NONE;
	if (Leaf.FindLastChar(TEXT('.'), Dot))
	{
		Leaf = Leaf.Mid(Dot + 1);
	}
	return FText::FromString(Leaf);
}

bool UBoxTypeDisplayLibrary::MatchesType(FGameplayTag AssetType, FGameplayTag Filter)
{
	if (!Filter.IsValid())
	{
		return true;
	}
	return AssetType.MatchesTag(Filter);
}

void UBoxTypeDisplayLibrary::ApplyOfficialDefaults(UDataTable* Table)
{
#if WITH_EDITOR
	if (!Table)
	{
		return;
	}

	Table->EmptyTable();
	auto Add = [Table](FGameplayTag Tag, const TCHAR* Name)
	{
		FBoxTypeDisplayRow Row;
		Row.Tag = Tag;
		Row.DisplayName = FText::FromString(Name);
		Table->AddRow(Tag.GetTagName(), Row);
	};

	Add(TAG_Type_Terrain, TEXT("地形"));
	Add(TAG_Type_Terrain_Floor, TEXT("地板"));
	Add(TAG_Type_Terrain_Wall, TEXT("墙"));
	Add(TAG_Type_Terrain_Empty, TEXT("空洞"));
	Add(TAG_Type_Character, TEXT("角色"));
	Add(TAG_Type_Interactable, TEXT("交互物"));
	Add(TAG_Type_Interactable_Box, TEXT("箱子"));
	Add(TAG_Type_Interactable_Target, TEXT("目标"));
	Add(TAG_Type_Interactable_Pedal, TEXT("踏板"));
#endif
}

FGameplayTag UBoxTypeDisplayLibrary::InferInteractableType(FName DefinitionId)
{
	const FString Id = DefinitionId.ToString();
	if (DefinitionId == TEXT("Target"))
	{
		return TAG_Type_Interactable_Target;
	}
	if (DefinitionId == TEXT("Pedal"))
	{
		return TAG_Type_Interactable_Pedal;
	}
	if (DefinitionId == TEXT("Box") || Id.StartsWith(TEXT("Box_")))
	{
		return TAG_Type_Interactable_Box;
	}
	return TAG_Type_Interactable;
}

FGameplayTag UBoxTypeDisplayLibrary::Terrain()
{
	return TAG_Type_Terrain;
}

FGameplayTag UBoxTypeDisplayLibrary::TerrainFloor()
{
	return TAG_Type_Terrain_Floor;
}

FGameplayTag UBoxTypeDisplayLibrary::TerrainWall()
{
	return TAG_Type_Terrain_Wall;
}

FGameplayTag UBoxTypeDisplayLibrary::TerrainEmpty()
{
	return TAG_Type_Terrain_Empty;
}

FGameplayTag UBoxTypeDisplayLibrary::Character()
{
	return TAG_Type_Character;
}

FGameplayTag UBoxTypeDisplayLibrary::InteractableTarget()
{
	return TAG_Type_Interactable_Target;
}

FGameplayTag UBoxTypeDisplayLibrary::InteractablePedal()
{
	return TAG_Type_Interactable_Pedal;
}

FGameplayTag UBoxTypeDisplayLibrary::ToolEraser()
{
	return TAG_Type_Tool_Eraser;
}

void UBoxTypeDisplayLibrary::CollectPaletteChain(FGameplayTag Tag, TArray<FGameplayTag>& OutChain)
{
	OutChain.Reset();
	if (!Tag.IsValid())
	{
		return;
	}

	TArray<FGameplayTag> Parents;
	Tag.GetGameplayTagParents().GetGameplayTagArray(Parents);
	Parents.Sort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		return A.ToString().Len() < B.ToString().Len();
	});
	for (const FGameplayTag& Parent : Parents)
	{
		if (Parent.ToString() == TEXT("Type"))
		{
			continue;
		}
		OutChain.Add(Parent);
	}
}
