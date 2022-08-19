// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types.h"



/**
 * 
 */
struct NAVIGATION3D_API Path
{
	Path();
	~Path();
	TArray<FVector> Points;
	float CalculateLength() const;
	static void GetAllPaths(StructureType Structure, ESceneType Scene, int VoxelSetting, TArray< Path>& Out);
	void Save(const FString& Name);
	bool Load(const FString& Name);
	static bool FileExist(const FString& Name, ESceneType Scene, StructureType Structure, int VoxelSetting);
	void Serialize(FArchive& Ar);
	FVector StartLocation;
	FVector TargetLocation;
	float GenerationTime = 0;
	int TestId;
	int PathId;
	int VoxelSetting;
	StructureType Structure;
	ESceneType Scene;
	PathResult Result;
};