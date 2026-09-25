#include "Data/InteractableState.h"
#include "Data/InteractableStateRuntime.h"
#include "Data/BoxInstanceParams.h"
#include "Match/BoxBoard.h"

void ExecuteInteractableStateActions(const TArray<FInteractableStateActionDef>& Actions, const FBoxStateActionContext& Context)
{
	for (int32 Index = 0; Index < Actions.Num(); ++Index)
	{
		const FInteractableStateActionDef& Action = Actions[Index];
		switch (Action.Type)
		{
		case EBoxStateActionType::FireEvent:
			if (!Action.EventId.IsNone())
			{
				FName EventId = Action.EventId;
				if (Action.bAllowOverride && Context.Instance)
				{
					EventId = BoxInstanceParams::ResolveOverriddenName(
						Context.Instance->Overrides,
						Context.StateId,
						BoxInstanceParams::FireEventOverrideKey(Context.Phase, Index),
						Action.EventId);
				}
				if (EventId.IsNone())
				{
					break;
				}
				if (Context.FiredEvents)
				{
					Context.FiredEvents->Add(EventId);
				}
				if (Context.BroadcastQueue)
				{
					Context.BroadcastQueue->Add(EventId);
				}
			}
			break;
		default:
			break;
		}
	}
}
