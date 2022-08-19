// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneDrawer.h"
#include "CompactMaze.h"
#include "AsteroidMap.h"
#include "DrawDebugHelpers.h"
#include "Nav3DVolume.h"

// Sets default values
ASceneDrawer::ASceneDrawer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASceneDrawer::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASceneDrawer::LoadMaze(const FString& Name)
{
	SceneToRender = CompactMaze::LoadMaze(Name);
}

void ASceneDrawer::LoadAsteroid(const FString& Name)
{
	SceneToRender = AsteroidMap::LoadAsteroidMap(Name);

}

void ASceneDrawer::Clear()
{
	SceneToRender.Reset();
}

// Called every frame
void ASceneDrawer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (SceneToRender)
	{
		SceneToRender->DrawDebug(GetWorld());
	}
	if (bDrawPathValid) {
		FVector Offset(0, 0, 5.f);
		if (DrawPath.Points.Num() > 0)
		{
			for (size_t i = 0; i < DrawPath.Points.Num() - 1; i++)
			{
				DrawDebugLine(GetWorld(), DrawPath.Points[i]+ Offset, DrawPath.Points[i + 1]+ Offset, FColor::Purple);
			}
		}
		DrawDebugPoint(GetWorld(), DrawPath.StartLocation + Offset, 20.f, FColor::Purple);
		DrawDebugPoint(GetWorld(), DrawPath.TargetLocation + Offset, 20.f, FColor::Purple);

	}
	if (bDrawNav3D && Nav3DVolume)
	{
		//Nav3DVolume->DebugDrawLeafOcclusion();
		for (const auto& Voxel : VoxelsToRender)
		{
			DrawDebugBox(GetWorld(), Voxel.Center, Voxel.Extent, FColor::Yellow);
		}
	}
}


void ASceneDrawer::DrawDebugPath(const FString& Name, bool IsSVO, bool IsMaze, int VoxelSetting)
{
	DrawPath.Structure = IsSVO ? StructureType::SVO : StructureType::TETMESH;
	DrawPath.Scene = IsMaze ? ESceneType::MAZE : ESceneType::ASTEROID;
	DrawPath.VoxelSetting = VoxelSetting;
	bDrawPathValid = DrawPath.Load(Name);
}

void ASceneDrawer::ClearDebugPath()
{
	DrawPath.Points.Empty();
	bDrawPathValid = false;
}

void ASceneDrawer::LoadNav3DVolume(FString path)
{
	if (!Nav3DVolume) return;
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/SVO/";
	FString FullPath = SaveDirectory + path + ".SVOMeshData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	Nav3DVolume->Serialize2(*Ar.Get());
	Ar->Close();
}

void ASceneDrawer::SetDrawNav3DVolume(bool draw)
{
	VoxelsToRender.Empty();
	bDrawNav3D = draw;
	if (bDrawNav3D)
	{
		Nav3DVolume->GetLeafOcclusionBoxes(VoxelsToRender);
	}
}

