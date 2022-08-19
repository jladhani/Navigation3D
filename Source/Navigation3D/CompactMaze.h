// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <IcarusNavGenerator.h>
#include "OcclusionInterface.h"
/**
 *
 */
#include <memory>
#include "IcarusVolumeCustom.h"


namespace Icarus {
	class SurfaceMesh;
};

class NAVIGATION3D_API CompactMaze : public OcclusionInterface
{
	friend class MazeGenerator;

public:
	~CompactMaze();
	static TSharedPtr<CompactMaze> GenerateMaze(FBox Bounds, FVector Location, int seed, int complexity);
	static TSharedPtr<CompactMaze> LoadMaze(const FString& name);

	//OcclusionInterface
	bool IsOccupied(const FBox& TestBounds) override;
	FVector GetRandomLocation(FRandomStream& RandomStream) const override;
	double GetTotalOpenVolume() override;
	float IntersectTet(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4) override;

	void IntersectBox(const FBox& Bounds, TArray<FBox>& OutIntersections);

	std::shared_ptr<Icarus::SurfaceMesh> GetSurfaceMesh() const;

	void DrawDebug(UWorld* world) override;


	void Save(const FString& name) override;

	bool Load(const FString& name) override;

	void Serialize(FArchive& Ar);


	bool IntersectTetTest(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4) override;

private:
	
	enum FaceDirection { XY = 0, XZ = 1, YZ = 2 };

	bool* GetFaceFromVoxel(FIntVector VoxelLocation, FIntVector Normal);
	bool* GetFace(FaceDirection direciton,int depth, int x, int y);

	int GetGridSize() const { return Complexity + 2; }
	FVector GetVoxelLocation(FIntVector idx);

	void GenerateCollisionBoxes();

	bool* FaceStates = nullptr;
	size_t NumFaceStates;

	CompactMaze();;
	CompactMaze(FBox Bounds, FVector Location, int seed, int complexity);

	FBoxCenterAndExtent GetVoxelBounds(FIntVector location) const;

	FBox Bounds;
	int Seed;
	int Complexity;
	float OpenSpace;
	float VoxelSize;
	float faceWidthPrecentile = 0.05f;

	TArray<FBox> CollisionBoxes;
	int32 NodeIdxCounter = 0;
	TUniquePtr<TOctree2<OctreeNode, FOctreeSemantics>> NodeOctree;
};
