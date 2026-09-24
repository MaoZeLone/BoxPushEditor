#include "Data/PlayerSpriteDef.h"

bool FBoxAtlasFrame::IsSet() const
{
	return !Texture.IsNull();
}

FPrimaryAssetId UPlayerSpriteDef::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("PlayerSpriteDef"), GetFName());
}

const FPlayerFacingLook& UPlayerSpriteDef::GetFacing(int32 FacingSteps) const
{
	switch ((FacingSteps % 4 + 4) % 4)
	{
	case 0: return North;
	case 1: return ScreenRight;
	case 2: return South;
	default: return ScreenLeft;
	}
}

FBoxAtlasFrame UPlayerSpriteDef::PickFrame(int32 FacingSteps, bool bPushing, bool bWalking, int32 FrameIndex) const
{
	const FPlayerFacingLook& Look = GetFacing(FacingSteps);
	const TArray<FBoxAtlasFrame>* Sequence = nullptr;
	if (bPushing && Look.Push.Num() > 0)
	{
		Sequence = &Look.Push;
	}
	else if (bWalking && Look.Walk.Num() > 0)
	{
		Sequence = &Look.Walk;
	}
	if (Sequence)
	{
		const int32 Index = (FrameIndex % Sequence->Num() + Sequence->Num()) % Sequence->Num();
		if ((*Sequence)[Index].IsSet())
		{
			return (*Sequence)[Index];
		}
	}
	if (Look.Idle.IsSet())
	{
		return Look.Idle;
	}
	return FBoxAtlasFrame();
}
