#include "Data/PlayerDef.h"
#include "AbilitySystem/BoxAbilityTagRelationshipMapping.h"
#include "BoxPushTags.h"
#include "Character/BoxPlayerCharacter.h"
#include "Data/BoxActionSet.h"
#include "Data/BoxAssetPaths.h"
#include "Data/BoxPushDefValidation.h"
#include "Input/BoxInputConfig.h"

UPlayerDef::UPlayerDef()
{
	PlayerId = TEXT("Player");
	DisplayName = FText::FromString(TEXT("玩家"));
	Type = TAG_Type_Character;
	PawnClass = ABoxPlayerCharacter::StaticClass();
}

UPlayerDef* UPlayerDef::LoadOfficial()
{
	if (UPlayerDef* Loaded = LoadObject<UPlayerDef>(nullptr, *BoxAssetPaths::PlayerDef()))
	{
		return Loaded;
	}

	static TWeakObjectPtr<UPlayerDef> Fallback;
	if (UPlayerDef* Existing = Fallback.Get())
	{
		return Existing;
	}

	UPlayerDef* Created = NewObject<UPlayerDef>(GetTransientPackage(), TEXT("RuntimePlayerDef"));
	Created->ApplyOfficialDefaults();
	Created->InputConfig = LoadObject<UBoxInputConfig>(nullptr, *BoxAssetPaths::PlayerInputConfig());
	Created->ActionSet = LoadObject<UBoxActionSet>(nullptr, *BoxAssetPaths::PlayerActionSet());
	Created->TagRelationshipMapping = LoadObject<UBoxAbilityTagRelationshipMapping>(
		nullptr, *BoxAssetPaths::PlayerTagRelationships());
	Created->AddToRoot();
	Fallback = Created;
	UE_LOG(LogTemp, Warning, TEXT("DA_Player failed to load. Play uses a runtime player def so the character can still move."));
	return Created;
}

FPrimaryAssetId UPlayerDef::GetPrimaryAssetId() const
{
	const FName Id = PlayerId.IsNone() ? GetFName() : PlayerId;
	return FPrimaryAssetId(TEXT("PlayerDef"), Id);
}

void UPlayerDef::ApplyOfficialDefaults()
{
	PlayerId = TEXT("Player");
	DisplayName = FText::FromString(TEXT("玩家"));
	Type = TAG_Type_Character;
	DesignerNote = TEXT("官方角色。PawnData：PawnClass + InputConfig + ActionSet + TagRelationship。");
	if (UClass* PlayerBp = LoadClass<APawn>(nullptr, *BoxAssetPaths::PlayerBlueprint()))
	{
		PawnClass = PlayerBp;
	}
	else
	{
		PawnClass = ABoxPlayerCharacter::StaticClass();
	}
}

FBoxAtlasFrame UPlayerDef::PickFrame(int32 FacingSteps, bool bPushing, bool bWalking, int32 FrameIndex) const
{
	return Sprite ? Sprite->PickFrame(FacingSteps, bPushing, bWalking, FrameIndex) : FBoxAtlasFrame();
}

float UPlayerDef::GetFrameInterval() const
{
	return Sprite && Sprite->FrameInterval > 0.f ? Sprite->FrameInterval : 0.16f;
}

bool UPlayerDef::CanPerform(FGameplayTag AbilityTag, const FGameplayTagContainer& OwnerTags) const
{
	if (!ActionSet || !ActionSet->HasGranted(AbilityTag))
	{
		return false;
	}
	if (!TagRelationshipMapping)
	{
		return true;
	}
	return TagRelationshipMapping->CanActivate(AbilityTag, OwnerTags);
}

void UPlayerDef::Validate(TArray<FLevelValidationIssue>& OutIssues) const
{
	using namespace BoxPushDefValidation;

	if (PlayerId.IsNone())
	{
		AddIssue(OutIssues, true, TEXT("PlayerId 为空"));
	}
	else
	{
		const FString Expected = FString::Printf(TEXT("DA_%s"), *PlayerId.ToString());
		if (GetName() != Expected)
		{
			AddIssue(OutIssues, true, FString::Printf(TEXT("资产名 %s 应等于 %s"), *GetName(), *Expected));
		}
	}
	if (!PawnClass)
	{
		AddIssue(OutIssues, true, TEXT("PawnClass 为空"));
	}
	if (!InputConfig)
	{
		AddIssue(OutIssues, true, TEXT("未指定 InputConfig"));
	}
	else
	{
		if (!InputConfig->DefaultMappingContext)
		{
			AddIssue(OutIssues, true, TEXT("InputConfig 缺少 DefaultMappingContext"));
		}
		if (!InputConfig->FindNativeInputActionForTag(TAG_Input_Move, false))
		{
			AddIssue(OutIssues, true, TEXT("InputConfig.Native 缺少 InputTag.Move"));
		}
		if (!InputConfig->FindAbilityInputActionForTag(TAG_Input_Undo, false))
		{
			AddIssue(OutIssues, true, TEXT("InputConfig.Ability 缺少 InputTag.Undo"));
		}
		if (!InputConfig->FindAbilityInputActionForTag(TAG_Input_Redo, false))
		{
			AddIssue(OutIssues, true, TEXT("InputConfig.Ability 缺少 InputTag.Redo"));
		}
		if (!InputConfig->FindAbilityInputActionForTag(TAG_Input_Restart, false))
		{
			AddIssue(OutIssues, true, TEXT("InputConfig.Ability 缺少 InputTag.Restart"));
		}
	}
	if (!ActionSet)
	{
		AddIssue(OutIssues, true, TEXT("未指定 ActionSet"));
	}
	else
	{
		auto RequireAction = [&](const FGameplayTag& AbilityTag, const TCHAR* Label)
		{
			if (!ActionSet->HasGranted(AbilityTag))
			{
				AddIssue(OutIssues, true, FString::Printf(TEXT("ActionSet 缺少 %s"), Label));
			}
		};
		RequireAction(TAG_Ability_Move, TEXT("Ability.Move"));
		RequireAction(TAG_Ability_Undo, TEXT("Ability.Undo"));
		RequireAction(TAG_Ability_Redo, TEXT("Ability.Redo"));
		RequireAction(TAG_Ability_Restart, TEXT("Ability.Restart"));

		if (InputConfig)
		{
			for (const FBoxGrantedAction& Row : ActionSet->GrantedActions)
			{
				if (!Row.InputTag.IsValid())
				{
					continue;
				}
				const bool bNative = Row.InputTag.MatchesTagExact(TAG_Input_Move);
				const UInputAction* Bound = bNative
					? InputConfig->FindNativeInputActionForTag(Row.InputTag, false)
					: InputConfig->FindAbilityInputActionForTag(Row.InputTag, false);
				if (!Bound)
				{
					AddIssue(OutIssues, true, FString::Printf(
						TEXT("ActionSet 行 %s 的 InputTag %s 在 InputConfig.%s 里没有"),
						*Row.AbilityTag.ToString(), *Row.InputTag.ToString(), bNative ? TEXT("Native") : TEXT("Ability")));
				}
			}
		}
	}
	if (!TagRelationshipMapping)
	{
		AddIssue(OutIssues, true, TEXT("未指定 TagRelationshipMapping"));
	}
}
