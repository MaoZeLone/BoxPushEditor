#include "Data/BoxPushLevelTypes.h"

#include "Engine/AssetManager.h"
#include "Data/BoxAssetPaths.h"
#include "Data/InteractableDef.h"

namespace
{
	UInteractableDef* LoadDefinitionById(FName DefinitionId)
	{
		if (DefinitionId.IsNone())
		{
			return nullptr;
		}

		if (UAssetManager::IsInitialized())
		{
			UAssetManager& Manager = UAssetManager::Get();
			const FSoftObjectPath Path = Manager.GetPrimaryAssetPath(FPrimaryAssetId(TEXT("InteractableDef"), DefinitionId));
			if (Path.IsValid())
			{
				if (UInteractableDef* Def = Cast<UInteractableDef>(Path.TryLoad()))
				{
					return Def;
				}
			}
		}

		const FString Name = DefinitionId.ToString();
		const FString AssetPath = BoxAssetPaths::InteractableObject(Name);
		return LoadObject<UInteractableDef>(nullptr, *AssetPath);
	}
}

const UInteractableDef* FBoxLevelInstance::LoadDefinition() const
{
	if (UInteractableDef* Loaded = Definition.LoadSynchronous())
	{
		return Loaded;
	}
	return LoadDefinitionById(DefinitionId);
}

FName FBoxLevelInstance::GetResolvedDefinitionId() const
{
	if (const UInteractableDef* Def = Definition.Get())
	{
		return Def->DefinitionId.IsNone() ? DefinitionId : Def->DefinitionId;
	}
	if (!DefinitionId.IsNone())
	{
		return DefinitionId;
	}
	const FString AssetName = Definition.ToSoftObjectPath().GetAssetName();
	if (AssetName.StartsWith(TEXT("DA_")))
	{
		return FName(*AssetName.RightChop(3));
	}
	return AssetName.IsEmpty() ? NAME_None : FName(*AssetName);
}

void FBoxLevelInstance::BackfillDefinitionIfNeeded()
{
	if (!Definition.IsNull())
	{
		return;
	}
	if (UInteractableDef* Def = LoadDefinitionById(DefinitionId))
	{
		Definition = Def;
	}
}
