#pragma once

#include "CoreMinimal.h"
#include "Types.h"


struct UPathfindingData
{
	int id;
	float PathfindingTime;
	float PathLength;
};

struct NavData
{
	int TestId = 0;
	StructureType DataStructure;
	ESceneType WorldType = ESceneType::MAZE;
	float SceneComplexity;
	int VoxelSize;
	//Coverage
	float OpenSpaceCovered = 0;
	float IncorrectArea = 0;

	//Complexity
	int NumberOfConnections = 0;
	int NumberOfNodes = 0;

	//Performance
	float GenerationTime = 0;
	int MemoryUsage = 0;
	TArray<UPathfindingData> PathFindingData;

	void Save(const FString& name);
	bool Load(const FString& name);
	void Serialize(FArchive& Ar);
	

};



class DataCollector
{
public:
	DataCollector() {};
	int SupplyPathData(const PathData& pathData);
	int SupplyData(const NavData& data);
	void Save(FString FileName, bool Nav, bool Path);
	void Clear();

	bool FindTetMeshRef(int testId, ESceneType worldType, NavData*& out);
	bool HasNavData() const { return CollectedNavData.Num() > 0; }
	bool HasPathData() const { return CollectedPathData.Num() > 0; }

	
	FString DELIMITER = ";";
	FString FLOATCOMMA = ",";

private:
	void SaveMainData(const FString& FileName);
	void SavePathData(const FString& FileName);
	FCriticalSection Mutex;
	FString SaveDirectory;
	TArray<NavData> CollectedNavData;
	TArray<PathData> CollectedPathData;
};