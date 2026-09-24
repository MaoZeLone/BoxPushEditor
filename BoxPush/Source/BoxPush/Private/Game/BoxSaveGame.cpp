#include "Game/BoxSaveGame.h"

FBoxLevelRecord UBoxSaveGame::GetRecord(FName LevelId) const
{
	if (const FBoxLevelRecord* Found = Levels.Find(LevelId))
	{
		return *Found;
	}
	return FBoxLevelRecord();
}

void UBoxSaveGame::RecordClear(FName LevelId)
{
	if (LevelId.IsNone())
	{
		return;
	}

	FBoxLevelRecord& Record = Levels.FindOrAdd(LevelId);
	Record.bCleared = true;
}
