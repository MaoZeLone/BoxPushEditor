#include "AbilitySystem/BoxAbilityTagRelationshipMapping.h"
#include "BoxPushTags.h"

namespace
{
	FBoxAbilityTagRelationship MakeBlockedBy(const FGameplayTag& AbilityTag, const FGameplayTag& BlockedBy)
	{
		FBoxAbilityTagRelationship Row;
		Row.AbilityTag = AbilityTag;
		Row.ActivationBlockedTags.AddTag(BlockedBy);
		return Row;
	}
}

void UBoxAbilityTagRelationshipMapping::ApplyOfficialDefaults()
{
	Relationships.Reset();
	Relationships.Add(MakeBlockedBy(TAG_Ability_Move, TAG_Status_StepLocked));
	Relationships.Add(MakeBlockedBy(TAG_Ability_Push, TAG_Status_StepLocked));
	Relationships.Add(MakeBlockedBy(TAG_Ability_Undo, TAG_Status_StepLocked));
	Relationships.Add(MakeBlockedBy(TAG_Ability_Redo, TAG_Status_StepLocked));
	Relationships.Add(MakeBlockedBy(TAG_Ability_Restart, TAG_Status_StepLocked));
	MarkPackageDirty();
}

bool UBoxAbilityTagRelationshipMapping::CanActivate(FGameplayTag AbilityTag, const FGameplayTagContainer& OwnerTags) const
{
	FGameplayTagContainer Required;
	FGameplayTagContainer Blocked;
	GetActivationRequirements(AbilityTag, Required, Blocked);
	if (Required.Num() > 0 && !OwnerTags.HasAll(Required))
	{
		return false;
	}
	if (Blocked.Num() > 0 && OwnerTags.HasAny(Blocked))
	{
		return false;
	}
	return true;
}

void UBoxAbilityTagRelationshipMapping::GetActivationRequirements(
	FGameplayTag AbilityTag,
	FGameplayTagContainer& OutRequired,
	FGameplayTagContainer& OutBlocked) const
{
	OutRequired.Reset();
	OutBlocked.Reset();
	if (!AbilityTag.IsValid())
	{
		return;
	}
	for (const FBoxAbilityTagRelationship& Row : Relationships)
	{
		if (!Row.AbilityTag.IsValid())
		{
			continue;
		}
		if (AbilityTag.MatchesTag(Row.AbilityTag))
		{
			OutRequired.AppendTags(Row.ActivationRequiredTags);
			OutBlocked.AppendTags(Row.ActivationBlockedTags);
		}
	}
}

void UBoxAbilityTagRelationshipMapping::GetBlockAndCancelTags(
	const FGameplayTagContainer& AbilityTags,
	FGameplayTagContainer& OutTagsToBlock,
	FGameplayTagContainer& OutTagsToCancel) const
{
	OutTagsToBlock.Reset();
	OutTagsToCancel.Reset();
	for (const FBoxAbilityTagRelationship& Row : Relationships)
	{
		if (Row.AbilityTag.IsValid() && AbilityTags.HasTag(Row.AbilityTag))
		{
			OutTagsToBlock.AppendTags(Row.AbilityTagsToBlock);
			OutTagsToCancel.AppendTags(Row.AbilityTagsToCancel);
		}
	}
}
