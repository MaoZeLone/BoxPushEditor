#pragma once

#include "CoreMinimal.h"
#include "Data/BoxPushLevelTypes.h"
#include "Data/InteractableState.h"

class UInteractableVisualComp;

namespace BoxPushDefValidation
{
	void AddIssue(TArray<FLevelValidationIssue>& OutIssues, bool bError, const FString& Text);

	void ValidateVisualTree(
		const TArray<UInteractableVisualComp*>& Comps,
		TArray<FLevelValidationIssue>& OutIssues);

	void ValidateVisualTree(
		const TArray<UInteractableVisualComp*>& Comps,
		const TArray<FInteractableStateDef>& States,
		TArray<FLevelValidationIssue>& OutIssues);

	void ValidateStates(
		const TArray<FInteractableStateDef>& States,
		TArray<FLevelValidationIssue>& OutIssues);

	void ValidateTransitions(
		const TArray<FInteractableStateDef>& States,
		const TArray<FInteractableTransitionDef>& Transitions,
		TArray<FLevelValidationIssue>& OutIssues);
}
