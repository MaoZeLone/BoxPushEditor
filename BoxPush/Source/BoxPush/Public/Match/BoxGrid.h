#pragma once

#include "CoreMinimal.h"

namespace BoxGrid
{
	inline constexpr float CellSize = 200.f;
	inline constexpr float OriginZ = 200.f;

	inline FVector CellToWorld(FIntPoint Cell, float Z = 0.f)
	{
		return FVector((Cell.X + 0.5f) * CellSize, (Cell.Y + 0.5f) * CellSize, OriginZ + Z);
	}

	inline FVector BoardCenter(int32 Width, int32 Height)
	{
		return FVector(Width * CellSize * 0.5f, Height * CellSize * 0.5f, OriginZ);
	}

	inline FIntPoint Neighbor(FIntPoint Cell, FIntPoint Dir)
	{
		return Cell + Dir;
	}

	inline const FIntPoint North{0, 1};
	inline const FIntPoint South{0, -1};
	inline const FIntPoint East{1, 0};
	inline const FIntPoint West{-1, 0};
}

/** 实例朝向。0 上、1 右、2 下、3 左，相对归位后的俯视，和 WASD 一致，不跟当前镜头转。 */
namespace BoxFacing
{
	inline int32 Normalize(int32 YawSteps)
	{
		return (YawSteps % 4 + 4) % 4;
	}

	inline FIntPoint ToDir(int32 YawSteps)
	{
		switch (Normalize(YawSteps))
		{
		case 0: return BoxGrid::North;
		case 1: return BoxGrid::West;
		case 2: return BoxGrid::South;
		default: return BoxGrid::East;
		}
	}

	inline FRotator ToRotator(int32 YawSteps)
	{
		return FRotator(0.f, 90.f + Normalize(YawSteps) * 90.f, 0.f);
	}

	inline int32 FromDir(FIntPoint Dir)
	{
		if (Dir.X == 0 && Dir.Y > 0)
		{
			return 0;
		}
		if (Dir.X < 0 && Dir.Y == 0)
		{
			return 1;
		}
		if (Dir.X == 0 && Dir.Y < 0)
		{
			return 2;
		}
		return 3;
	}
}

/** 对局正交俯视。位置在棋盘中心正上方，朝下看。 */
namespace BoxPlayCamera
{
	inline const FVector Offset(0.f, 0.f, 3000.f);
	inline const FRotator Rotation(-90.f, 90.f, 0.f);
	inline constexpr float FieldOfView = 50.f;

	inline FVector ViewLocation(int32 Width, int32 Height)
	{
		return BoxGrid::BoardCenter(Width, Height) + Offset;
	}
}
