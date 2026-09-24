#include "Input/BoxInputConfig.h"

#include "BoxPushTags.h"

namespace
{
	FBoxInputAction MakeBind(const UInputAction* Action, FName TagName)
	{
		FBoxInputAction Bind;
		Bind.InputAction = Action;
		if (!TagName.IsNone())
		{
			Bind.InputTag = FGameplayTag::RequestGameplayTag(TagName, false);
		}
		return Bind;
	}

	const UInputAction* FindAction(const TArray<FBoxInputAction>& Actions, const FGameplayTag& InputTag, bool bLogIfNotFound, const UObject* Owner, const TCHAR* ListName)
	{
		for (const FBoxInputAction& Action : Actions)
		{
			if (Action.InputAction && Action.InputTag == InputTag)
			{
				return Action.InputAction;
			}
		}
		if (bLogIfNotFound)
		{
			UE_LOG(LogTemp, Warning, TEXT("BoxInputConfig [%s] 找不到 %s InputAction，Tag: [%s]"),
				*GetNameSafe(Owner), ListName, *InputTag.ToString());
		}
		return nullptr;
	}
}

void UBoxInputConfig::PostLoad()
{
	Super::PostLoad();
	if (SplitAbilityBindsOutOfNative())
	{
#if WITH_EDITOR
		MarkPackageDirty();
#endif
	}
}

bool UBoxInputConfig::SplitAbilityBindsOutOfNative()
{
	TArray<FBoxInputAction> KeptNative;
	KeptNative.Reserve(NativeInputActions.Num());
	bool bChanged = false;
	for (const FBoxInputAction& Row : NativeInputActions)
	{
		if (Row.InputTag.IsValid() && Row.InputTag != TAG_Input_Move)
		{
			if (!FindAction(AbilityInputActions, Row.InputTag, false, this, TEXT("Ability")))
			{
				AbilityInputActions.Add(Row);
			}
			bChanged = true;
			continue;
		}
		KeptNative.Add(Row);
	}
	if (bChanged)
	{
		NativeInputActions = MoveTemp(KeptNative);
	}
	return bChanged;
}

const UInputAction* UBoxInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound) const
{
	return FindAction(NativeInputActions, InputTag, bLogIfNotFound, this, TEXT("Native"));
}

const UInputAction* UBoxInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound) const
{
	return FindAction(AbilityInputActions, InputTag, bLogIfNotFound, this, TEXT("Ability"));
}

void UBoxInputConfig::AddNativeInputAction(const UInputAction* Action, FName TagName)
{
	NativeInputActions.Add(MakeBind(Action, TagName));
	MarkPackageDirty();
}

void UBoxInputConfig::AddAbilityInputAction(const UInputAction* Action, FName TagName)
{
	AbilityInputActions.Add(MakeBind(Action, TagName));
	MarkPackageDirty();
}

void UBoxInputConfig::ResetNativeInputActions()
{
	NativeInputActions.Reset();
	MarkPackageDirty();
}

void UBoxInputConfig::ResetAbilityInputActions()
{
	AbilityInputActions.Reset();
	MarkPackageDirty();
}
