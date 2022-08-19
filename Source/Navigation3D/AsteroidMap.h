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
#

namespace Icarus {
	class SurfaceMesh;
};
struct Tetrahedron;

struct NAVIGATION3D_API Asteroid
{
	FVector Location;
	FRotator Rotation;
	float Length;
	float Radius;
	Asteroid(const FVector& Location, const FRotator& Rotation, float Length, float Radius)
		: Location(Location)
		, Rotation(Rotation)
		, Length(Length)
		, Radius(Radius) {}

	bool IsOverlapping(const Asteroid& Other, FVector& OutNormal, float& OutPenDepth) const;
	bool IsOverlapping(const Tetrahedron& Other) const;
	bool IsOverlapping(const FBox& Other) const;
	bool IsOverlappingSAT(const TArray<FVector>& FaceNormals, const TArray<FVector>& Edges, const TArray<FVector>& Vertices) const;
	float GetVolume()
	{
		return PI * Radius * Radius * ((4 / 3) * Radius + Length);
	}

	FBox GetBounds() const;
	FVector GetDirection() const { return Rotation.RotateVector(FVector(1, 0, 0)); }

	int ScaleOnVolume(float DesiredVolume, float Tolerance = 0.005f);
};

class NAVIGATION3D_API AsteroidMap : public OcclusionInterface
{
public:

	static TSharedPtr<AsteroidMap> GenerateAsteroidMap(FBox Bounds, FVector Location, int seed, float complexity);
	static TSharedPtr<AsteroidMap> LoadAsteroidMap(const FString& Name);;

	double GetTotalOpenVolume() override;
	float IntersectTet(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4) override;

	bool IntersectTetTest(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4) override;
	bool IsOccupied(const FBox& TestBounds) override;
	FVector GetRandomLocation(FRandomStream& RandomStream) const override;
	void Save(const FString& name) override;
	bool Load(const FString& name) override;
	void Serialize(FArchive& Ar);
	void DrawDebug(UWorld* world) override;

private:



	FBox Bounds;
	TArray<Asteroid> Asteroids;

	struct AsteroidMesh
	{
		TArray<FVector> Vertices;
		TArray<int32> Indices;
	};
	AsteroidMesh& GetCachedAsteroidMesh(int Idx);
	TMap<int, AsteroidMesh> CachedAsteroidMeshes;
	TUniquePtr<TOctree2<OctreeNode, FOctreeSemantics>> Octree;
};