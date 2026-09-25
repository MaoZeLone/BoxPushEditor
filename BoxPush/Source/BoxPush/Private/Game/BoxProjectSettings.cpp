#include "Game/BoxProjectSettings.h"

#include "Misc/Paths.h"

UBoxProjectSettings::UBoxProjectSettings()
{
	CellSize = 200.f;
	OriginZ = 200.f;
	TileFit = 0.975f;
	StepDuration = 0.36f;
	CameraOffset = FVector(0.f, 0.f, 3000.f);
	CameraRotation = FRotator(-90.f, 90.f, 0.f);
	CameraFOV = 50.f;
}

FName UBoxProjectSettings::GetCategoryName() const
{
	return TEXT("Game");
}

static FString ResolveAgainstProject(const FString& Path)
{
	if (Path.IsEmpty())
	{
		return FString();
	}
	if (FPaths::IsRelative(Path))
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), Path));
	}
	return FPaths::ConvertRelativePathToFull(Path);
}

FString UBoxProjectSettings::ResolveUiPackRoot() const
{
	return ResolveAgainstProject(UiPackRoot.Path);
}
