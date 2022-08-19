// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <IcarusNavGenerator.h>
/**
 * 
 */

#include <memory>
namespace Icarus {
	class SurfaceMesh;
}
class NAVIGATION3D_API Maze : public TSharedFromThis<Maze>
{
	friend class MazeGenerator;

public:
	~Maze();
	static TSharedPtr<Maze> GenerateMaze(FBox Bounds, FVector Location, int seed, int complexity);
	TSharedPtr<CompactBinaryVoxelGrid> GetVoxelGrid() const { return BinaryVoxelGrid; }

	bool IsOccupied(const FBox& TestBounds);

	FVector GetRandomLocation(FRandomStream& RandomStream) const;

	double GetTotalOpenVolume();
	float IntersectTet(FVector v1, FVector v2, FVector v3, FVector v4);

	void IntersectBox(FBox Bounds, TArray<FBox>& OutIntersections);

	std::shared_ptr<Icarus::SurfaceMesh> GetSurfaceMesh() const;

private:
	Maze(FBox Bounds, FVector Location, int seed, int complexity);

	TSharedPtr<CompactBinaryVoxelGrid> BinaryVoxelGrid;
	FBoxCenterAndExtent GetVoxelBounds(FIntVector location) const;

	FBox Bounds;
	int Seed;
	int Complexity;
	float OpenSpace;
};
