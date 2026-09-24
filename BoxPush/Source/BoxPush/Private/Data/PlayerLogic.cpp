#include "Data/PlayerLogic.h"

#include "BoxPushTags.h"

UPlayerPushLogic::UPlayerPushLogic()
{
	PushType = TAG_Type_Interactable_Box;
}

void UPlayerPushLogic::PostLoad()
{
	Super::PostLoad();
	if (!PushType.IsValid())
	{
		PushType = TAG_Type_Interactable_Box;
	}
}
