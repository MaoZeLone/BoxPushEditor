#include "Data/BoxActionSet.h"

FPrimaryAssetId UBoxActionSet::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ActionSet"), GetFName());
}

void UBoxActionSet::ApplyOfficialDefaults()
{
	ResetGrantedActions();
	AddGrantedAction(TEXT("Ability.Move"), TEXT("InputTag.Move"));
	AddGrantedAction(TEXT("Ability.Push"), NAME_None);
	AddGrantedAction(TEXT("Ability.Undo"), TEXT("InputTag.Undo"));
	AddGrantedAction(TEXT("Ability.Redo"), TEXT("InputTag.Redo"));
	AddGrantedAction(TEXT("Ability.Restart"), TEXT("InputTag.Restart"));
	AddGrantedAction(TEXT("Ability.Pause"), TEXT("InputTag.Pause"));
}

void UBoxActionSet::AddGrantedAction(FName AbilityTag, FName InputTag)
{
	FBoxGrantedAction Row;
	if (!AbilityTag.IsNone())
	{
		Row.AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTag, false);
	}
	if (!InputTag.IsNone())
	{
		Row.InputTag = FGameplayTag::RequestGameplayTag(InputTag, false);
	}
	GrantedActions.Add(Row);
	MarkPackageDirty();
}

void UBoxActionSet::ResetGrantedActions()
{
	GrantedActions.Reset();
	MarkPackageDirty();
}

bool UBoxActionSet::HasGranted(FGameplayTag AbilityTag) const
{
	return FindByAbilityTag(AbilityTag) != nullptr;
}

const FBoxGrantedAction* UBoxActionSet::FindByAbilityTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return nullptr;
	}
	for (const FBoxGrantedAction& Row : GrantedActions)
	{
		if (Row.AbilityTag.MatchesTag(AbilityTag))
		{
			return &Row;
		}
	}
	return nullptr;
}

const FBoxGrantedAction* UBoxActionSet::FindByInputTag(FGameplayTag InputTag) const
{
	if (!InputTag.IsValid())
	{
		return nullptr;
	}
	for (const FBoxGrantedAction& Row : GrantedActions)
	{
		if (Row.InputTag.IsValid() && Row.InputTag.MatchesTag(InputTag))
		{
			return &Row;
		}
	}
	return nullptr;
}
