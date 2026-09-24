#include "Data/InteractableLogic.h"

#include "BoxPushTags.h"
#include "Data/BoxInstanceParams.h"
#include "Data/BoxTypeDisplay.h"
#include "Data/InteractableDef.h"
#include "Match/BoxBoard.h"

UGoalLogic::UGoalLogic()
{
	RequiredType = TAG_Type_Interactable_Box;
}

void UGoalLogic::PostLoad()
{
	Super::PostLoad();
	if (!RequiredType.IsValid())
	{
		RequiredType = TAG_Type_Interactable_Box;
	}
}

UPedalLogic::UPedalLogic()
{
	AcceptType = TAG_Type_Interactable_Box;
}

void UPedalLogic::PostLoad()
{
	Super::PostLoad();
	if (!AcceptType.IsValid())
	{
		AcceptType = TAG_Type_Interactable_Box;
	}
}

bool UInteractableLogicComp::IsWinSatisfied(const UBoxBoard& Board, const FBoxRuntimeInstance& Self) const
{
	(void)Board;
	(void)Self;
	return true;
}

bool UGoalLogic::IsWinSatisfied(const UBoxBoard& Board, const FBoxRuntimeInstance& Self) const
{
	if (!Self.Def)
	{
		return false;
	}

	const FGameplayTag Required = BoxInstanceParams::RequiredType(Self);
	for (const FBoxRuntimeInstance& Other : Board.GetInstances())
	{
		if (Other.InstanceId == Self.InstanceId || Other.Cell != Self.Cell || !Other.Def)
		{
			continue;
		}
		if (!Other.Def->FindLogic<UPushableLogic>())
		{
			continue;
		}
		if (UBoxTypeDisplayLibrary::MatchesType(Other.Def->Type, Required))
		{
			return true;
		}
	}
	return false;
}
