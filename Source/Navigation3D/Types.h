#pragma once
//#include "CoreMinimal.h"

enum StructureType : int
{
	TETMESH = 1,
	SVO = 2
};

UENUM(BlueprintType)
enum class ESceneType : uint8
{
	MAZE = 0,
	ASTEROID = 1
};

enum PathResult
{
	SUCCES = 0,
	TOOKTOOLONG = 1,
	FAILED = 2,
};

struct PathData
{
	int TestID;
	int PathID;
	float Distance;
	float GenerationTime;
	PathResult Result;
	int VoxelSize;
	StructureType DataStructure;
	ESceneType WorldType;
};