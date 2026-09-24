#include "Data/InteractableState.h"
#include "Data/InteractableStateRuntime.h"

void ExecuteInteractableStateActions(const TArray<FInteractableStateActionDef>& Actions, const FBoxStateActionContext& Context)
{
	for (const FInteractableStateActionDef& Action : Actions)
	{
		switch (Action.Type)
		{
		case EBoxStateActionType::FireEvent:
			if (!Action.EventId.IsNone() && Context.FiredEvents)
			{
				Context.FiredEvents->Add(Action.EventId);
			}
			break;
		default:
			break;
		}
	}
}
