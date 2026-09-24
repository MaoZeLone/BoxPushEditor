#include "Data/BoxInstanceParams.h"

#include "BoxPushTags.h"
#include "Data/InteractableDef.h"
#include "Data/InteractableLogic.h"
#include "Match/BoxBoard.h"
#include "Algo/Reverse.h"
#include "UObject/UnrealType.h"

namespace BoxInstanceParams
{
	const FBoxInstanceOverride* FindRow(const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key)
	{
		return Overrides.FindByPredicate([CompId, Key](const FBoxInstanceOverride& Row)
		{
			return Row.CompId == CompId && Row.Key == Key;
		});
	}

	void RemoveRow(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key)
	{
		Overrides.RemoveAll([CompId, Key](const FBoxInstanceOverride& Row)
		{
			return Row.CompId == CompId && Row.Key == Key;
		});
	}

	bool ResolveBool(const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, bool Default, bool bAllow)
	{
		if (!bAllow)
		{
			return Default;
		}
		const FBoxInstanceOverride* Row = FindRow(Overrides, CompId, Key);
		return Row && Row->Kind == EBoxParamKind::Bool ? Row->BoolValue : Default;
	}

	int32 ResolveInt(const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, int32 Default, bool bAllow)
	{
		if (!bAllow)
		{
			return Default;
		}
		const FBoxInstanceOverride* Row = FindRow(Overrides, CompId, Key);
		return Row && Row->Kind == EBoxParamKind::Int ? Row->IntValue : Default;
	}

	FName ResolveName(const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, FName Default, bool bAllow)
	{
		if (!bAllow)
		{
			return Default;
		}
		const FBoxInstanceOverride* Row = FindRow(Overrides, CompId, Key);
		return Row && Row->Kind == EBoxParamKind::Name ? Row->NameValue : Default;
	}

	FGameplayTag ResolveTag(const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, FGameplayTag Default, bool bAllow)
	{
		if (!bAllow)
		{
			return Default;
		}
		const FBoxInstanceOverride* Row = FindRow(Overrides, CompId, Key);
		return Row && Row->Kind == EBoxParamKind::Tag ? Row->TagValue : Default;
	}

	const FProperty* FindOverrideProperty(const UInteractableLogicComp* Comp, FName Key)
	{
		if (!Comp || Key.IsNone())
		{
			return nullptr;
		}
		const FProperty* Prop = Comp->GetClass()->FindPropertyByName(Key);
		if (!Prop || Prop->GetMetaData(TEXT("InstanceOverride")).IsEmpty())
		{
			return nullptr;
		}
		return Prop;
	}

	bool AllowFlag(const UInteractableLogicComp* Comp, const FProperty* Prop)
	{
		const FString AllowName = Prop->GetMetaData(TEXT("InstanceOverride"));
		const FBoolProperty* Allow = FindFProperty<FBoolProperty>(Comp->GetClass(), *AllowName);
		return Allow && Allow->GetPropertyValue_InContainer(Comp);
	}

	bool FillDefault(const UInteractableLogicComp* Comp, const FProperty* Prop, FBoxShownParam& Out)
	{
		Out = FBoxShownParam();
		Out.CompId = Comp->CompId;
		Out.Key = Prop->GetFName();
		const FString Label = Prop->GetMetaData(TEXT("DisplayName"));
		Out.Label = Label.IsEmpty() ? FText::FromName(Out.Key) : FText::FromString(Label);
		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
		{
			Out.Kind = EBoxParamKind::Bool;
			Out.BoolValue = BoolProp->GetPropertyValue_InContainer(Comp);
			return true;
		}
		if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			Out.Kind = EBoxParamKind::Int;
			const FString Min = Prop->GetMetaData(TEXT("ClampMin"));
			Out.IntMin = Min.IsEmpty() ? 0 : FCString::Atoi(*Min);
			Out.IntValue = IntProp->GetPropertyValue_InContainer(Comp);
			return true;
		}
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			Out.Kind = EBoxParamKind::Name;
			Out.NameValue = NameProp->GetPropertyValue_InContainer(Comp);
			return true;
		}
		if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			if (StructProp->Struct == FGameplayTag::StaticStruct())
			{
				Out.Kind = EBoxParamKind::Tag;
				Out.TagValue = *StructProp->ContainerPtrToValuePtr<FGameplayTag>(Comp);
				return true;
			}
		}
		return false;
	}

	bool RowAllowed(const UInteractableLogicComp* Comp, FName Key)
	{
		const FProperty* Prop = FindOverrideProperty(Comp, Key);
		return Prop && AllowFlag(Comp, Prop);
	}

	void AddBool(TArray<FBoxShownParam>& Out, const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, const FText& Label, bool Default, bool bAllow)
	{
		if (!bAllow)
		{
			return;
		}
		FBoxShownParam Param;
		Param.CompId = CompId;
		Param.Key = Key;
		Param.Label = Label;
		Param.Kind = EBoxParamKind::Bool;
		Param.BoolValue = ResolveBool(Overrides, CompId, Key, Default, true);
		Out.Add(Param);
	}

	void AddInt(TArray<FBoxShownParam>& Out, const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, const FText& Label, int32 Default, int32 Min, bool bAllow)
	{
		if (!bAllow)
		{
			return;
		}
		FBoxShownParam Param;
		Param.CompId = CompId;
		Param.Key = Key;
		Param.Label = Label;
		Param.Kind = EBoxParamKind::Int;
		Param.IntMin = Min;
		Param.IntValue = ResolveInt(Overrides, CompId, Key, Default, true);
		Out.Add(Param);
	}

	void AddName(TArray<FBoxShownParam>& Out, const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, const FText& Label, FName Default, bool bAllow)
	{
		if (!bAllow)
		{
			return;
		}
		FBoxShownParam Param;
		Param.CompId = CompId;
		Param.Key = Key;
		Param.Label = Label;
		Param.Kind = EBoxParamKind::Name;
		Param.NameValue = ResolveName(Overrides, CompId, Key, Default, true);
		Out.Add(Param);
	}

	void AddTag(TArray<FBoxShownParam>& Out, const TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, const FText& Label, FGameplayTag Default, bool bAllow)
	{
		if (!bAllow)
		{
			return;
		}
		FBoxShownParam Param;
		Param.CompId = CompId;
		Param.Key = Key;
		Param.Label = Label;
		Param.Kind = EBoxParamKind::Tag;
		Param.TagValue = ResolveTag(Overrides, CompId, Key, Default, true);
		Out.Add(Param);
	}

	void Collect(const UInteractableDef* Def, const TArray<FBoxInstanceOverride>& Overrides, TArray<FBoxShownParam>& Out)
	{
		Out.Reset();
		if (!Def)
		{
			return;
		}
		for (const UInteractableLogicComp* Comp : Def->LogicComps)
		{
			if (!Comp)
			{
				continue;
			}
			TArray<const FProperty*> Props;
			for (TFieldIterator<FProperty> It(Comp->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
			{
				Props.Add(*It);
			}
			Algo::Reverse(Props);
			for (const FProperty* Prop : Props)
			{
				if (!FindOverrideProperty(Comp, Prop->GetFName()) || !AllowFlag(Comp, Prop))
				{
					continue;
				}
				FBoxShownParam Default;
				if (!FillDefault(Comp, Prop, Default))
				{
					continue;
				}
				if (Default.Kind == EBoxParamKind::Bool)
				{
					AddBool(Out, Overrides, Default.CompId, Default.Key, Default.Label, Default.BoolValue, true);
				}
				else if (Default.Kind == EBoxParamKind::Int)
				{
					AddInt(Out, Overrides, Default.CompId, Default.Key, Default.Label, Default.IntValue, Default.IntMin, true);
				}
				else if (Default.Kind == EBoxParamKind::Name)
				{
					AddName(Out, Overrides, Default.CompId, Default.Key, Default.Label, Default.NameValue, true);
				}
				else if (Default.Kind == EBoxParamKind::Tag)
				{
					AddTag(Out, Overrides, Default.CompId, Default.Key, Default.Label, Default.TagValue, true);
				}
			}
		}
	}

	bool FindDefault(const UInteractableDef* Def, FName CompId, FName Key, FBoxShownParam& Out)
	{
		if (!Def)
		{
			return false;
		}
		for (const UInteractableLogicComp* Comp : Def->LogicComps)
		{
			if (!Comp || Comp->CompId != CompId)
			{
				continue;
			}
			const FProperty* Prop = FindOverrideProperty(Comp, Key);
			if (!Prop || !AllowFlag(Comp, Prop))
			{
				return false;
			}
			return FillDefault(Comp, Prop, Out);
		}
		return false;
	}

	void UpsertBool(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, bool Value, bool Default)
	{
		RemoveRow(Overrides, CompId, Key);
		if (Value == Default)
		{
			return;
		}
		FBoxInstanceOverride Row;
		Row.CompId = CompId;
		Row.Key = Key;
		Row.Kind = EBoxParamKind::Bool;
		Row.BoolValue = Value;
		Overrides.Add(Row);
	}

	void UpsertInt(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, int32 Value, int32 Default)
	{
		RemoveRow(Overrides, CompId, Key);
		if (Value == Default)
		{
			return;
		}
		FBoxInstanceOverride Row;
		Row.CompId = CompId;
		Row.Key = Key;
		Row.Kind = EBoxParamKind::Int;
		Row.IntValue = Value;
		Overrides.Add(Row);
	}

	void UpsertName(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, FName Value, FName Default)
	{
		RemoveRow(Overrides, CompId, Key);
		if (Value == Default)
		{
			return;
		}
		FBoxInstanceOverride Row;
		Row.CompId = CompId;
		Row.Key = Key;
		Row.Kind = EBoxParamKind::Name;
		Row.NameValue = Value;
		Overrides.Add(Row);
	}

	void UpsertTag(TArray<FBoxInstanceOverride>& Overrides, FName CompId, FName Key, FGameplayTag Value, FGameplayTag Default)
	{
		RemoveRow(Overrides, CompId, Key);
		if (Value == Default)
		{
			return;
		}
		FBoxInstanceOverride Row;
		Row.CompId = CompId;
		Row.Key = Key;
		Row.Kind = EBoxParamKind::Tag;
		Row.TagValue = Value;
		Overrides.Add(Row);
	}

	bool SameRow(const FBoxInstanceOverride& A, const FBoxInstanceOverride& B)
	{
		return A.CompId == B.CompId && A.Key == B.Key && A.Kind == B.Kind
			&& A.BoolValue == B.BoolValue && A.IntValue == B.IntValue
			&& A.NameValue == B.NameValue && A.TagValue == B.TagValue;
	}

	bool SameOverrides(const TArray<FBoxInstanceOverride>& A, const TArray<FBoxInstanceOverride>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!SameRow(A[Index], B[Index]))
			{
				return false;
			}
		}
		return true;
	}

	void AppendIssues(const FBoxLevelInstance& Inst, TArray<FLevelValidationIssue>& Out)
	{
		const UInteractableDef* Def = Inst.LoadDefinition();
		for (const FBoxInstanceOverride& Row : Inst.ParamOverrides)
		{
			const UInteractableLogicComp* Comp = nullptr;
			if (Def)
			{
				for (const UInteractableLogicComp* Candidate : Def->LogicComps)
				{
					if (Candidate && Candidate->CompId == Row.CompId)
					{
						Comp = Candidate;
						break;
					}
				}
			}
			if (Comp && RowAllowed(Comp, Row.Key))
			{
				continue;
			}
			FLevelValidationIssue Issue;
			Issue.bError = false;
			Issue.Message = FText::FromString(FString::Printf(TEXT("实例 %s 的重载 %s.%s 未开放，已忽略"),
				*Inst.InstanceId.ToString(), *Row.CompId.ToString(), *Row.Key.ToString()));
			Issue.Cells.Add(Inst.Cell);
			Out.Add(Issue);
		}
	}

	bool BlocksPlayer(const FBoxRuntimeInstance& Inst)
	{
		const UBlockingLogic* Blocking = Inst.Def ? Inst.Def->FindLogic<UBlockingLogic>() : nullptr;
		if (!Blocking)
		{
			return false;
		}
		return ResolveBool(Inst.Overrides, Blocking->CompId, GET_MEMBER_NAME_CHECKED(UBlockingLogic, bBlocksPlayer), Blocking->bBlocksPlayer, Blocking->bAllowBlocksPlayer);
	}

	bool BlocksPush(const FBoxRuntimeInstance& Inst)
	{
		const UBlockingLogic* Blocking = Inst.Def ? Inst.Def->FindLogic<UBlockingLogic>() : nullptr;
		if (!Blocking)
		{
			return false;
		}
		return ResolveBool(Inst.Overrides, Blocking->CompId, GET_MEMBER_NAME_CHECKED(UBlockingLogic, bBlocksPush), Blocking->bBlocksPush, Blocking->bAllowBlocksPush);
	}

	int32 PushSteps(const FBoxRuntimeInstance& Inst)
	{
		const UPushableLogic* Pushable = Inst.Def ? Inst.Def->FindLogic<UPushableLogic>() : nullptr;
		if (!Pushable)
		{
			return 1;
		}
		return FMath::Max(1, ResolveInt(Inst.Overrides, Pushable->CompId, GET_MEMBER_NAME_CHECKED(UPushableLogic, Steps), Pushable->Steps, Pushable->bAllowSteps));
	}

	bool StopOnBox(const FBoxRuntimeInstance& Inst)
	{
		const USlideLogic* Slide = Inst.Def ? Inst.Def->FindLogic<USlideLogic>() : nullptr;
		if (!Slide)
		{
			return true;
		}
		return ResolveBool(Inst.Overrides, Slide->CompId, GET_MEMBER_NAME_CHECKED(USlideLogic, bStopOnBox), Slide->bStopOnBox, Slide->bAllowStopOnBox);
	}

	int32 DelayMoves(const FBoxRuntimeInstance& Inst)
	{
		const UReturnLogic* Return = Inst.Def ? Inst.Def->FindLogic<UReturnLogic>() : nullptr;
		if (!Return)
		{
			return 0;
		}
		return FMath::Max(0, ResolveInt(Inst.Overrides, Return->CompId, GET_MEMBER_NAME_CHECKED(UReturnLogic, DelayMoves), Return->DelayMoves, Return->bAllowDelayMoves));
	}

	FGameplayTag RequiredType(const FBoxRuntimeInstance& Inst)
	{
		const UGoalLogic* Goal = Inst.Def ? Inst.Def->FindLogic<UGoalLogic>() : nullptr;
		const FGameplayTag Fallback = TAG_Type_Interactable_Box;
		if (!Goal)
		{
			return Fallback;
		}
		const FGameplayTag Resolved = ResolveTag(Inst.Overrides, Goal->CompId, GET_MEMBER_NAME_CHECKED(UGoalLogic, RequiredType), Goal->RequiredType, Goal->bAllowRequiredType);
		return Resolved.IsValid() ? Resolved : Fallback;
	}

	FGameplayTag AcceptType(const FBoxRuntimeInstance& Inst)
	{
		const UPedalLogic* Pedal = Inst.Def ? Inst.Def->FindLogic<UPedalLogic>() : nullptr;
		if (!Pedal)
		{
			return FGameplayTag();
		}
		return ResolveTag(Inst.Overrides, Pedal->CompId, GET_MEMBER_NAME_CHECKED(UPedalLogic, AcceptType), Pedal->AcceptType, Pedal->bAllowAcceptType);
	}
}
