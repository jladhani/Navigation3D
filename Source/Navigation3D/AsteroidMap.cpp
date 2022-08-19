#include "AsteroidMap.h"
#include "DrawDebugHelpers.h"
#include "SurfaceMesh.h"
#include "IcarusVolumeCustom.h"
#include "Tetrahedron.h"

TSharedPtr<AsteroidMap> AsteroidMap::GenerateAsteroidMap(FBox Bounds, FVector Location, int seed, float complexity)
{
	FRandomStream stream(seed);
	complexity = FMath::Clamp(complexity, 0.f, 1.f);
	const float fillPrecentage = 0.5f;
	const float MaxRatio = 10;
	const float c = 0.914;

	TSharedPtr<AsteroidMap> Result(new AsteroidMap());
	Result->Bounds = Bounds;
	TArray<Asteroid>& Asteroids = Result->Asteroids;

	Result->Octree = MakeUnique<TOctree2< OctreeNode, FOctreeSemantics>>(
		FVector(0, 0, 0), Bounds.GetExtent().GetMax());

	TUniquePtr<TOctree2<OctreeNode, FOctreeSemantics>>& Octree = Result->Octree;
	FVector Dimentions = Bounds.Max - Bounds.Min;
	float TotalVolume = Dimentions.X * Dimentions.Y * Dimentions.Z;
	float CurrentFillVolume = 0;
	float DesiredFillVolume = TotalVolume * complexity * fillPrecentage;

	float InitialAsteroidVolume = TotalVolume * 0.05f;

	while (CurrentFillVolume < DesiredFillVolume)
	{
		FVector Location = FVector(stream.FRandRange(Bounds.Min.X, Bounds.Max.X), stream.FRandRange(Bounds.Min.Y, Bounds.Max.Y), stream.FRandRange(Bounds.Min.Y, Bounds.Max.Y));
		FRotator Rotation = FRotator(stream.FRandRange(0.f, 360.f), stream.FRandRange(0.f, 360.f), 0);
		float Radius = 50;
		float Lenght = Radius * stream.FRandRange(0.f, MaxRatio);
		Asteroid NewAsteroid = Asteroid(Location, Rotation, Lenght, Radius);


		float DesiredAsteroidVolume = Asteroids.Num() > 0 ? InitialAsteroidVolume* (1.f / (FMath::Pow(Asteroids.Num(), c))) : InitialAsteroidVolume;

		NewAsteroid.ScaleOnVolume(DesiredAsteroidVolume);
		FBox BTest = NewAsteroid.GetBounds();
		bool rsa = BTest.IsInside(Bounds);
		//Check if asteroid is in bounds
		if (!Bounds.IsInside(BTest))
		{
			continue;
		}
		
		TArray<int> CollisionCheck;
		bool HadCollision = false;
		int failCounter = 0;
		do
		{
			failCounter++;
			HadCollision = false;
			Octree->FindElementsWithBoundsTest(FBoxCenterAndExtent(NewAsteroid.GetBounds()), [&HadCollision, &NewAsteroid, &Asteroids](const OctreeNode& Node) {
				FVector Normal;
				float Depth = 0;
				if (NewAsteroid.IsOverlapping(Asteroids[Node.Index], Normal, Depth))
				{
					NewAsteroid.Location += Normal * Depth;
					HadCollision = true;
				}
				//return Depth < 0;
			});
		} while (HadCollision == true && failCounter < 1000);

		if (HadCollision == true) continue;

		if (!Bounds.IsInside(NewAsteroid.GetBounds())) continue;
		int newId = Asteroids.Add(NewAsteroid);
		FBox NewBounds = NewAsteroid.GetBounds();
		Octree->AddElement({ newId, FBoxCenterAndExtent(NewBounds) });
		float Vol = NewAsteroid.GetVolume();
		CurrentFillVolume += NewAsteroid.GetVolume();
	}
	return Result;
}

TSharedPtr<AsteroidMap> AsteroidMap::LoadAsteroidMap(const FString& Name)
{
	TSharedPtr<AsteroidMap> Result(new AsteroidMap());
	if (Result->Load(Name)) return Result;
	else return TSharedPtr<AsteroidMap>();
}

double AsteroidMap::GetTotalOpenVolume()
{
	FVector BoundSize = Bounds.Max - Bounds.Min;
	double totalVolume = BoundSize.X * BoundSize.Y * BoundSize.Z;

	for (Asteroid& A : Asteroids)
	{
		totalVolume -= (double)A.GetVolume();
	}

	return totalVolume;
}

float AsteroidMap::IntersectTet(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4)
{
	Tetrahedron tet(v1, v2, v3, v4);
	TArray<int> CollisionsIndices;
	Octree->FindElementsWithBoundsTest(FBoxCenterAndExtent(tet.GetBounds()), [&CollisionsIndices,this,tet](const OctreeNode& Node)
		{
			if (Asteroids[Node.Index].IsOverlapping(tet))
			{
				CollisionsIndices.Add(Node.Index);
			}
		});

	double Overlap = 0;
	for (int Idx : CollisionsIndices)
	{
		AsteroidMesh& Mesh = GetCachedAsteroidMesh(Idx);
		auto SurfMesh = Icarus::SurfaceMesh::FromTriangles(Mesh.Vertices, Mesh.Indices);
		auto TetMesh = Icarus::SurfaceMesh::FromTetrahedron(tet);

		SurfMesh->Intersect2(TetMesh);
		Overlap += SurfMesh->CalculateVolume();
	}

	return Overlap;
}

bool AsteroidMap::IntersectTetTest(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4)
{
	Tetrahedron tet(v1, v2, v3, v4);
	TArray<int> CollisionsIndices;
	Octree->FindElementsWithBoundsTest(FBoxCenterAndExtent(tet.GetBounds()), [&CollisionsIndices, this, tet](const OctreeNode& Node)
		{
			if (Asteroids[Node.Index].IsOverlapping(tet))
			{
				CollisionsIndices.Add(Node.Index);
			}
		});

	double Overlap = 0;
	for (int Idx : CollisionsIndices)
	{
		AsteroidMesh& Mesh = GetCachedAsteroidMesh(Idx);

		auto SurfMesh = Icarus::SurfaceMesh::FromTriangles(Mesh.Vertices, Mesh.Indices);
		auto TetMesh = Icarus::SurfaceMesh::FromTetrahedron(tet);
		auto NewMesh = SurfMesh->Intersect(TetMesh);

		Overlap += NewMesh->CalculateVolume();
		if (Overlap > 0) return true;
	}

	return false;
}

bool AsteroidMap::IsOccupied(const FBox& TestBounds)
{
	TArray<int> CollisionsIndices;

	Octree->FindElementsWithBoundsTest(FBoxCenterAndExtent(TestBounds), [&CollisionsIndices](const OctreeNode& Node)
		{
			CollisionsIndices.Add(Node.Index);
		});


	for (int index : CollisionsIndices) {
		if (Asteroids[index].IsOverlapping(TestBounds))
		{
			return true;
		}
	}
	return false;
}

FVector AsteroidMap::GetRandomLocation(FRandomStream& RandomStream) const
{

	while (true)
	{
		FVector Location = FVector(RandomStream.FRandRange(Bounds.Min.X, Bounds.Max.X), RandomStream.FRandRange(Bounds.Min.Y, Bounds.Max.Y), RandomStream.FRandRange(Bounds.Min.Z, Bounds.Max.Z));
		bool Valid = true;
		Octree->FindElementsWithBoundsTest(FBoxCenterAndExtent(Location, FVector::ZeroVector), [this, &Valid, &Location](const OctreeNode& Node)
			{
				if (Asteroids[Node.Index].IsOverlapping(FBox(Location,Location)))
				{
					Valid = false;
				}
			});
		if (Valid) return Location;
	}
	return FVector(0, 0, 0);
}

void AsteroidMap::Save(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/AsteroidMap/";
	FString FullPath = SaveDirectory + name + ".AsteroidMap";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileWriter(*FullPath, 0));
	Serialize(*Ar.Get());
}

bool AsteroidMap::Load(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/AsteroidMap/";
	FString FullPath = SaveDirectory + name + ".AsteroidMap";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	if (Ar)Serialize(*Ar.Get());
	return Ar.IsValid();
}


void AsteroidMap::Serialize(FArchive& Ar)
{
	auto NumberOfCapsules = Asteroids.Num();
	Ar.Serialize(&NumberOfCapsules, sizeof(NumberOfCapsules));
	Ar.Serialize(&Bounds, sizeof(Bounds));

	if (!Ar.IsSaving())
	{
		Asteroids.SetNumUninitialized(NumberOfCapsules);
	}

	for (size_t i = 0; i < NumberOfCapsules; i++)
	{
		Ar.Serialize(&Asteroids[i], sizeof(Asteroid));
	}
	if (!Ar.IsSaving())
	{
		Octree= MakeUnique<TOctree2< OctreeNode, FOctreeSemantics>>(
			FVector(0, 0, 0), Bounds.GetExtent().GetMax()
			);
		for (size_t i = 0; i < Asteroids.Num(); i++)
		{
			Octree->AddElement({(int)i,  FBoxCenterAndExtent(Asteroids[i].GetBounds())});
		}
		CachedAsteroidMeshes.Empty();
	}
}

void DrawDebugWireMesh(UWorld* world,const TArray<FVector>& Vertices, const TArray<int32>& Indices)
{
	for (size_t i = 0; i < Indices.Num(); i+=3)
	{
		const FVector& V1 = Vertices[Indices[i]];
		const FVector& V2 = Vertices[Indices[i+1]];
		const FVector& V3 = Vertices[Indices[i+2]];
		DrawDebugLine(world, V1, V2, FColor::Red);
		DrawDebugLine(world, V2, V3, FColor::Red);
		DrawDebugLine(world, V3, V1, FColor::Red);
	}
}
using namespace Icarus;

void AsteroidMap::DrawDebug(UWorld* world)
{
	for (size_t i = 0; i < Asteroids.Num(); i++)
	{
		auto& P = GetCachedAsteroidMesh(i);	
		DrawDebugMesh(world, P.Vertices, P.Indices, FColor::Blue);
		//DrawDebugWireMesh(world, P.Vertices, P.Indices);
	}

	DrawDebugBox(world, FVector(0, 0, 0), Bounds.GetExtent(), FColor::Red);
}

AsteroidMap::AsteroidMesh& AsteroidMap::GetCachedAsteroidMesh(int Idx)
{
	auto PairPtr = CachedAsteroidMeshes.Find(Idx);
	if (PairPtr == nullptr)
	{
		Asteroid& A = Asteroids[Idx];
		TArray<FVector> Vertices;
		TArray<int32> Indices;
		auto newMesh = SurfaceMesh::FromCapsule(A.Length, A.Radius);
		newMesh->GetVerticesAndIndices(Vertices, Indices);
		for (auto& V : Vertices) V = A.Rotation.RotateVector(V) + A.Location;;
		return CachedAsteroidMeshes.Add(Idx, AsteroidMesh{Vertices,Indices});
		
		//auto DebugMesh = SurfaceMesh::FromTriangles(Vertices, Indices);
		//return CachedAsteroidMeshes.Add(Idx, DebugMesh);
	}
	return *PairPtr;
}

bool Asteroid::IsOverlapping(const Asteroid& Other, FVector& OutNormal, float& OutPenDepth) const
{
	FVector AV1 = Rotation.RotateVector(FVector(0.5f, 0, 0) * Length) + Location;
	FVector AV2 = Rotation.RotateVector(FVector(-0.5f, 0, 0) * Length) + Location;
	FVector BV1 = Other.Rotation.RotateVector(FVector(0.5f, 0, 0) * Other.Length) + Other.Location;
	FVector BV2 = Other.Rotation.RotateVector(FVector(-0.5f, 0, 0) * Other.Length) + Other.Location;

	FVector AResult = {};
	FVector BResult = {};

	FMath::SegmentDistToSegmentSafe(AV1, AV2, BV1, BV2, AResult, BResult);

	OutPenDepth = (Radius + Other.Radius)-FVector::Dist(AResult, BResult);
	if (OutPenDepth > 0)
	{
		FVector t1 = AResult - BResult;
		OutNormal = (AResult - BResult).GetSafeNormal();
	}
	else if(OutPenDepth == 0)
	{
		OutNormal = FVector::CrossProduct(AV1-AV2,BV1-BV2).GetSafeNormal();
	}

	return OutPenDepth > 0;
}

bool Asteroid::IsOverlapping(const Tetrahedron& Tet) const
{
	TArray<FVector> Vertices;
	Vertices.SetNumUninitialized(4);
	for (size_t i = 0; i < 4; i++) Vertices[i] = Tet.Vertices[i];
	

	TArray<FVector> Edges =
	{
		Tet.V2 - Tet.V1,
		Tet.V3 - Tet.V1,
		Tet.V4 - Tet.V1,
		Tet.V2 - Tet.V3,
		Tet.V2 - Tet.V4,
		Tet.V3 - Tet.V4
	};
	TArray<FVector> FaceNormals =
	{
		Tet.GetFaceNormal(0),
		Tet.GetFaceNormal(1),
		Tet.GetFaceNormal(2),
		Tet.GetFaceNormal(3),
	};
	return IsOverlappingSAT(FaceNormals,Edges,Vertices);
}

bool Asteroid::IsOverlapping(const FBox& Box) const
{
	FVector Center = Box.GetCenter();
	FVector Extent = Box.GetExtent();

	FPlane Planes[6] = {
		FPlane(Center + FVector(Extent.X,0,0), FVector(Extent.X,0,0).GetSafeNormal()),
		FPlane(Center + FVector(-Extent.X,0,0), FVector(-Extent.X,0,0).GetSafeNormal()),
		FPlane(Center + FVector(0,Extent.Y,0), FVector(0,Extent.Y,0).GetSafeNormal()),
		FPlane(Center + FVector(0,-Extent.Y,0), FVector(0,-Extent.Y,0).GetSafeNormal()),
		FPlane(Center + FVector(0,0,Extent.Z), FVector(0,0,Extent.Z).GetSafeNormal()),
		FPlane(Center + FVector(0,0,-Extent.Z), FVector(0,0,-Extent.Z).GetSafeNormal())
	};
	FVector v1 = Location + GetDirection() *Length* 0.5f;
	FVector v2 = Location + GetDirection() *Length* -0.5f;

	FVector p1;
	float dist = BIG_NUMBER;
	for (const FPlane& p : Planes)
	{
		FVector PlaneCentre = (Center + p.GetNormal() * Extent);

		float t = FMath::GetTForSegmentPlaneIntersect(v1, v2, p);
		FVector IntersectionPoint = v1 + t * (v2 - v1);
		FVector CapsulePoint = v1 + ((v2 - v1) * FMath::Clamp(t, 0.0f, 1.f));

		FVector PlaneProjection = FVector::PointPlaneProject(CapsulePoint, p);

		FVector ClampDirs = (FVector(1, 1, 1) - p.GetNormal()) * Extent;
		FVector localPoint = PlaneProjection - PlaneCentre;
		if (ClampDirs.X > 0.01) localPoint.X = FMath::Clamp(localPoint.X, -ClampDirs.X, ClampDirs.X);
		if (ClampDirs.Y > 0.01) localPoint.Y = FMath::Clamp(localPoint.Y, -ClampDirs.Y, ClampDirs.Y);
		if (ClampDirs.Z > 0.01) localPoint.Z = FMath::Clamp(localPoint.Z, -ClampDirs.Z, ClampDirs.Z);

		FVector PlanePoint = localPoint + PlaneCentre;

		float newDist = FVector::Dist(CapsulePoint, PlanePoint);
		if (newDist < dist) dist = newDist;
		if (newDist < Radius) break;
	}



	return dist < Radius;
	/*
	TArray<FVector> Vertices =
	{
		Box.Min,
		FVector(Box.Min.X,Box.Min.Y,Box.Max.Z),
		FVector(Box.Min.X,Box.Max.Y,Box.Min.Z),
		FVector(Box.Max.X,Box.Min.Y,Box.Min.Z),
		Box.Max,
		FVector(Box.Max.X,Box.Max.Y,Box.Min.Z),
		FVector(Box.Max.X,Box.Min.Y,Box.Max.Z),
		FVector(Box.Min.X,Box.Max.Y,Box.Max.Z)
	};
	TArray<FVector> Edges =
	{
		FVector(1,0,0),
		FVector(0,1,0),
		FVector(0,0,1)
	};

	TArray<FVector> FaceNormals =
	{
		FVector(1,0,0),
		FVector(0,1,0),
		FVector(0,0,1)
	};
	return IsOverlappingSAT(FaceNormals, Edges, Vertices);*/
}

bool Asteroid::IsOverlappingSAT(const TArray<FVector>& FaceNormals, const TArray<FVector>& Edges, const TArray<FVector>& Vertices) const
{
	constexpr float Maxfloat = std::numeric_limits<float>::max();
	constexpr float Minfloat = std::numeric_limits<float>::lowest();
	TArray<FVector> AllAxis;
	AllAxis.Reserve(1 + FaceNormals.Num() + Edges.Num());

	FVector CapsuleDir = GetDirection();

	//Add Normals
	AllAxis.Add(CapsuleDir.GetSafeNormal());
	for (const FVector& Normal : FaceNormals) AllAxis.Add(Normal.GetSafeNormal());

	//Add Cross of edges
	for (const FVector& Edge : Edges) AllAxis.Add(FVector::CrossProduct(Edge,CapsuleDir).GetSafeNormal());

	FVector CapsuleVertices[2]
	{
		Location + GetDirection() * Length * 0.5f,
		Location + GetDirection() * Length * -0.5f
	};

	for (const FVector& Axis : AllAxis)
	{
		if (Axis == FVector::ZeroVector) continue;

		float capsProjMin = Maxfloat, objProjMin = Maxfloat;
		float capsProjMax = Minfloat, objProjMax = Minfloat;

		for (size_t i = 0; i < 2; i++)
		{
			float val = FVector::DotProduct(Axis, CapsuleVertices[i]);
			if (val-Radius < capsProjMin) capsProjMin = val - Radius;
			if (val+Radius > capsProjMax) capsProjMax = val + Radius;
		}
		for (const FVector& Vertex : Vertices)
		{
			float val = FVector::DotProduct(Axis, Vertex);
			if (val < objProjMin) objProjMin = val;
			if (val > objProjMax) objProjMax = val;
		}
		bool DoOverlap = FMath::Max(capsProjMin, objProjMin) <= FMath::Min(capsProjMax, objProjMax);
		if (!DoOverlap) return false;
	}


	return true;
}

FBox Asteroid::GetBounds() const
{
	FVector Normal = Rotation.RotateVector(FVector(1.f, 0, 0));
	FVector V1 = Normal * Length * 0.5f;
	FVector V2 = -V1;

	FBox Result(ForceInit);
	FVector RadiusV = FVector(Radius);
	Result += V1 + RadiusV + Location;
	Result += V1 - RadiusV + Location;
	Result += V2 + RadiusV + Location;
	Result += V2 - RadiusV + Location;
	return Result;
}

int Asteroid::ScaleOnVolume(float DesiredVolume, float Tolerance /*= 0.005f*/)
{
	if (DesiredVolume <= 0)
	{
		Length = 0;
		Radius = 0;
		return 0;
	}

	float Ratio =  Length / Radius;
	float Volume = GetVolume();
	while (DesiredVolume > Volume)
	{
		Radius = Radius * Radius;
		Length = Radius * Ratio;
		Volume = GetVolume();
	}

	float LastStep = Radius;
	float VolumeTolerance = Tolerance * DesiredVolume;
	Volume = GetVolume();
	int InterationCount = 0;
	while (FMath::Abs(Volume - DesiredVolume) > VolumeTolerance)
	{
		float Step = LastStep * 0.5f;
		if (Volume > DesiredVolume) Radius -= Step;
		else Radius += Step;
		Length = Radius * Ratio;
		Volume = GetVolume();
		LastStep = Step;
		InterationCount++;
	}
	return InterationCount;
}

