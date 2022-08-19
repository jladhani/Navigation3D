#include "DataCollector.h"


int DataCollector::SupplyPathData(const PathData& pathData)
{
	return CollectedPathData.Add(pathData);
}

int DataCollector::SupplyData(const NavData& data)
{
	FScopeLock lock(&Mutex);
	return CollectedNavData.Add(data);
}

void DataCollector::Save(FString FileName, bool Nav, bool Path)
{
	SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Results/";
	if(Nav)SaveMainData(FileName);
	if(Path)SavePathData(FileName);
}

void DataCollector::Clear()
{
	CollectedNavData.Empty();
}

bool DataCollector::FindTetMeshRef(int testId, ESceneType worldType, NavData*& out)
{
	bool succes = false;
	for (NavData& navData : CollectedNavData)
	{
		if (navData.TestId == testId && navData.WorldType == worldType && navData.DataStructure == TETMESH)
		{
			succes = true;
			out = &navData;
		}
	}
	return succes;
}

void DataCollector::SaveMainData(const FString& FileName)
{


	CollectedNavData.Sort([](const NavData& a, const NavData b) {
		return a.TestId < b.TestId;
		});

	FString FullPath = SaveDirectory + FileName + ".csv";

	FString FinalString = "";

	FinalString += "ID" + DELIMITER + "Type" + DELIMITER + "WorldType" + DELIMITER + "Scene Complexity" + DELIMITER + "Voxel Size" + DELIMITER + "OpenFreeCovered" + DELIMITER + "IncorectArea" + DELIMITER + "NumberOfConnections" + DELIMITER + "NumberOfNodes" + DELIMITER + "GenerationTime" + DELIMITER + "MemoryUsage" + DELIMITER + "PathListID\n";


	int id = 0;
	for (size_t i = 0; i < 2; i++)
	{

		for (const NavData& navData : CollectedNavData)
		{
			if (i == 0 && navData.DataStructure == TETMESH) continue;
			if (i == 1 && navData.DataStructure == SVO) continue;

			float MemoryMB = (float)navData.MemoryUsage / (1024 * 1024);

			FinalString += FString::FromInt(navData.TestId) + DELIMITER;
			FinalString += FString(navData.DataStructure == TETMESH ? "TetMesh" : "SVO") + DELIMITER;
			FinalString += FString(navData.WorldType == ESceneType::MAZE ? "Maze" : "AsteroidField") + DELIMITER;
			FinalString += FString::SanitizeFloat(navData.SceneComplexity, 3) + DELIMITER;
			FinalString += FString::FromInt(navData.VoxelSize) + DELIMITER;
			FinalString += FString::SanitizeFloat(navData.OpenSpaceCovered) + DELIMITER;
			FinalString += FString::SanitizeFloat(isfinite(navData.IncorrectArea) ? navData.IncorrectArea : 0.f) + DELIMITER;
			FinalString += FString::FromInt(navData.NumberOfConnections) + DELIMITER;
			FinalString += FString::FromInt(navData.NumberOfNodes) + DELIMITER;
			FinalString += FString::SanitizeFloat(navData.GenerationTime) + DELIMITER;
			FinalString += FString::SanitizeFloat(MemoryMB, 2)+ DELIMITER;
			FinalString += FString::FromInt(id++) + "\n";

		}

	}
	FinalString.ReplaceInline(TEXT("."), *FLOATCOMMA);
	FFileHelper::SaveStringToFile(FinalString, *FullPath);
}

void DataCollector::SavePathData(const FString& FileName)
{

	CollectedPathData.Sort([](const PathData& a, const PathData b) {
		return (a.TestID * 1000 + a.PathID) < (b.TestID * 1000 + b.PathID);
		});

	FString FullPath = SaveDirectory + FileName + "Paths" + ".csv";
	FString FinalString = "";
	FinalString += "TestID" + DELIMITER + "PathID"  +DELIMITER + + "Type" + DELIMITER + "WorldType" + DELIMITER + "Voxel Size" + DELIMITER + "Length of Path" + DELIMITER + "Generation Time" + DELIMITER + "Status\n";
	
	for (PathData& Data : CollectedPathData)
	{
		FinalString += FString::FromInt(Data.TestID) + DELIMITER;
		FinalString += FString::FromInt(Data.PathID) + DELIMITER;
		FinalString += FString(Data.DataStructure == TETMESH ? "TetMesh" : "SVO") + DELIMITER;
		FinalString += FString(Data.WorldType == ESceneType::MAZE ? "Maze" : "AsteroidField") + DELIMITER;
		FinalString += FString::FromInt(Data.VoxelSize) + DELIMITER;


		FinalString += FString::SanitizeFloat(Data.Distance) + DELIMITER;
		FinalString += FString::SanitizeFloat(Data.GenerationTime)+ DELIMITER;
		switch (Data.Result)
		{
		default:
			FinalString += DELIMITER;
			break;
		case SUCCES:
			FinalString += "Success" + DELIMITER;
			break;
		case TOOKTOOLONG:
			FinalString += "Took too long" + DELIMITER;
			break;
		case FAILED:
			FinalString += "Failed" + DELIMITER;
			break;
		}
		FinalString += "\n";
	}


	FinalString.ReplaceInline(TEXT("."), *FLOATCOMMA);
	FFileHelper::SaveStringToFile(FinalString, *FullPath);
	

	

}

void NavData::Save(const FString& name)
{

	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Results/";
	FString FullPath = SaveDirectory + name + ".result";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileWriter(*FullPath, 0));
	Serialize(*Ar.Get());
}

bool NavData::Load(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Results/";
	FString FullPath = SaveDirectory + name + ".result";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	if (Ar)Serialize(*Ar.Get());
	return Ar.IsValid();
}

void NavData::Serialize(FArchive& Ar)
{
	Ar << TestId;
	Ar.Serialize(&DataStructure, sizeof(DataStructure));


	int OldSceneType = static_cast<int>(WorldType);
	Ar.Serialize(&OldSceneType, sizeof(OldSceneType));
	WorldType = static_cast<ESceneType>(OldSceneType);

	Ar << SceneComplexity;
	Ar << VoxelSize;
	Ar << OpenSpaceCovered;
	Ar << IncorrectArea;
	Ar << NumberOfConnections;
	Ar << NumberOfNodes;
	Ar << GenerationTime;
	Ar << MemoryUsage;
	
	int32 Num = PathFindingData.Num();
	Ar << Num;

	if (Ar.IsLoading())
	{
		PathFindingData.SetNum(Num);
	}

	Ar.Serialize(PathFindingData.GetData(), sizeof(UPathfindingData) * Num);

}
