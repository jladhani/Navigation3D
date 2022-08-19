// Fill out your copyright notice in the Description page of Project Settings.


#include "Path.h"

Path::Path()
{
}

Path::~Path()
{
}

float Path::CalculateLength() const
{
	float Length = 0;
	if (Points.Num() > 0) {
		for (size_t i = 0; i < Points.Num() - 1; i++)
		{
			Length += FVector::Distance(Points[i], Points[i + 1]);
		}
	}
	return Length;
}

void Path::GetAllPaths(StructureType Structure, ESceneType Scene, int VoxelSetting, TArray< Path>& Out)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Paths/";
	SaveDirectory += Structure == StructureType::TETMESH ? "TetMesh/" : "SVO/";
	SaveDirectory += FString::FromInt(VoxelSetting) + "/";
	SaveDirectory += Scene == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";
	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *SaveDirectory);

	Out.Reserve(Out.Num() + FoundFiles.Num());
	for (FString& FileName : FoundFiles) {
		int32 idx = Out.Add(Path());
		Path& path = Out[idx];
		path.Scene = Scene;
		path.Structure = Structure;
		FString FullPath = SaveDirectory + FileName;
		TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
		if (Ar)path.Serialize(*Ar.Get());
	}

	return;

}

void Path::Save(const FString& Name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Paths/";
	SaveDirectory += Structure == StructureType::TETMESH ? "TetMesh/" : "SVO/";
	SaveDirectory += FString::FromInt(VoxelSetting) + "/";
	SaveDirectory += Scene == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";

	FString FullPath = SaveDirectory + Name + ".PathData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileWriter(*FullPath, 0));
	if (Ar)Serialize(*Ar.Get());
}

bool Path::Load(const FString& Name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Paths/";
	SaveDirectory += Structure == StructureType::TETMESH ? "TetMesh/" : "SVO/";
	SaveDirectory += FString::FromInt(VoxelSetting) + "/";
	SaveDirectory += Scene == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";

	FString FullPath = SaveDirectory + Name + ".PathData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	if (Ar)Serialize(*Ar.Get());
	return Ar.IsValid();
}

bool Path::FileExist(const FString& Name, ESceneType Scene, StructureType Structure, int VoxelSetting)
{
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Paths/";
	SaveDirectory += Structure == StructureType::TETMESH ? "TetMesh/" : "SVO/";
	SaveDirectory += FString::FromInt(VoxelSetting) + "/";
	SaveDirectory += Scene == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";
	FString FullPath = SaveDirectory + Name + ".PathData";
	return FPaths::FileExists(FullPath);
}

void Path::Serialize(FArchive& Ar)
{
	int NumberOfPoints = Points.Num();
	Ar << NumberOfPoints;

	if (Ar.IsLoading())
	{
		Points.SetNumUninitialized(NumberOfPoints);
	}

	for (size_t i = 0; i < NumberOfPoints; i++)
	{
		Ar << Points[i];
	}

	Ar << GenerationTime;
	Ar << PathId;
	Ar << TestId;
	Ar << VoxelSetting;

	Ar.Serialize(&Result, sizeof(Result));
	Ar.Serialize(&Structure, sizeof(Structure));
	Ar.Serialize(&Scene, sizeof(Scene));
	Ar.Serialize(&StartLocation, sizeof(StartLocation));
	Ar.Serialize(&TargetLocation, sizeof(TargetLocation));
}
