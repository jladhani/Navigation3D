// Fill out your copyright notice in the Description page of Project Settings.


#include "Maze.h"
#include "Math/RandomStream.h"
#include "SurfaceMesh.h"
#include "Tetrahedron.h"

Maze::Maze(FBox Bounds, FVector Location, int seed, int complexity)
	: Bounds(Bounds)
	, Seed(seed)
	, Complexity(complexity)
{
}

FBoxCenterAndExtent Maze::GetVoxelBounds(FIntVector location) const
{
	const FVector Offset = -Bounds.GetExtent();
	const FVector HalfVoxel = FVector(BinaryVoxelGrid->GetVoxelSize()) / 2.f;
	const FVector Centre = Offset + FVector(location) * BinaryVoxelGrid->GetVoxelSize() + HalfVoxel;
	return FBoxCenterAndExtent(Centre, FVector(BinaryVoxelGrid->GetVoxelSize() / 2.f));
}

Maze::~Maze()
{
}

TSharedPtr<Maze> Maze::GenerateMaze(FBox Bounds, FVector Location, int seed, int complexity)
{
	auto ConvertSuperGridToVoxelGrid = [](FIntVector location)
	{
		return (location * 2) + FIntVector(1, 1, 1);
	};
	auto IsWithinBounds = [](FIntVector location, size_t width)
	{
		return location.X < width&& location.X >= 0 && location.Y < width&& location.Y >= 0 && location.Z < width&& location.Z >= 0;
	};
	auto SuperGridLocationToID = [](FIntVector location, size_t width)
	{
		return location.Z * width * width + location.Y * width + location.X;
	};


	FRandomStream RandomStream(seed);

	TSharedPtr<Maze> GeneratedMaze = TSharedPtr<Maze>(new Maze(Bounds,Location,seed,complexity));

	int VoxelGridSize = (complexity*2) + 5;
	FVector VoxelSize = (Bounds.GetExtent() * 2.f) / (float)VoxelGridSize;


	TSharedPtr<CompactBinaryVoxelGrid> VoxelGrid = TSharedPtr<CompactBinaryVoxelGrid>(new CompactBinaryVoxelGrid(VoxelGridSize, VoxelGridSize, VoxelGridSize, VoxelSize,true));
	
	enum NodeState : uint8 {
		UNVISITED = 0,
		VISITED = 1
	};

	int SuperGridSize = (VoxelGridSize - 1) / 2;
	NodeState* VoxelGridState = (NodeState*)FMemory::MallocZeroed(SuperGridSize * SuperGridSize * SuperGridSize * sizeof(NodeState));
	TArray<FIntVector> VisitedStack;
	
	FIntVector StartLocation = FIntVector(RandomStream.RandRange(0, SuperGridSize-1), RandomStream.RandRange(0, SuperGridSize-1), RandomStream.RandRange(0, SuperGridSize-1));
	VoxelGrid->SetState(ConvertSuperGridToVoxelGrid(StartLocation), true);
	VisitedStack.Push(StartLocation);
	VoxelGridState[SuperGridLocationToID(StartLocation,SuperGridSize)] = VISITED;

	while (VisitedStack.Num()>0)
	{
		//Get Top of Stack
		FIntVector Top = VisitedStack.Top();

		//Gather viable neigbours
		TArray<FIntVector> possibleNeigbours;
		TArray<FIntVector> Neigbours;
		possibleNeigbours.Add(Top + FIntVector(1, 0, 0));
		possibleNeigbours.Add(Top + FIntVector(-1, 0, 0));
		possibleNeigbours.Add(Top + FIntVector(0, 1, 0));
		possibleNeigbours.Add(Top + FIntVector(0, -1, 0));
		possibleNeigbours.Add(Top + FIntVector(0, 0, 1));
		possibleNeigbours.Add(Top + FIntVector(0, 0, -1));

		for (FIntVector& possibleNeigbour : possibleNeigbours)
		{
			if (IsWithinBounds(possibleNeigbour, SuperGridSize))
			{
				if (VoxelGridState[SuperGridLocationToID(possibleNeigbour, SuperGridSize)] == UNVISITED) Neigbours.Add(possibleNeigbour);
			}
		}

		//Choose random neigbour

		if (Neigbours.Num() > 0) {

			FIntVector RandomNeigbour = Neigbours[RandomStream.RandRange(0, Neigbours.Num() - 1)];
			VoxelGridState[SuperGridLocationToID(RandomNeigbour,SuperGridSize)] = VISITED;
			FIntVector Diff = RandomNeigbour - Top;
			FIntVector TopVoxelGridLocation = ConvertSuperGridToVoxelGrid(Top);
			VoxelGrid->SetState(TopVoxelGridLocation + Diff, true);
			VoxelGrid->SetState(TopVoxelGridLocation + Diff*2, true);
			VisitedStack.Push(RandomNeigbour);

		}
		else
		{
			VisitedStack.Pop();
		}

	}
	FMemory::Free(VoxelGridState);
	GeneratedMaze->BinaryVoxelGrid = VoxelGrid;


	

	return GeneratedMaze;
}

bool Maze::IsOccupied(const FBox& TestBounds)
{
	if (!Bounds.Intersect(TestBounds))
	{
		return false;
	}
	FBox Overlap = Bounds.Overlap(TestBounds);
	FVector Min = Overlap.Min - Bounds.Min;
	FVector Max = Overlap.Max - Bounds.Min;

	FVector l1 = Min / BinaryVoxelGrid->GetVoxelSize();
	FIntVector MinVoxels = FIntVector(FMath::Floor(l1.X), FMath::Floor(l1.Y), FMath::Floor(l1.Z));

	FVector l2 = Max / BinaryVoxelGrid->GetVoxelSize();
	FIntVector MaxVoxels = FIntVector(FMath::CeilToInt(l2.X), FMath::CeilToInt(l2.Y), FMath::CeilToInt(l2.Z));
	FIntVector TotalVoxels = FIntVector(BinaryVoxelGrid->GetSizeX(), BinaryVoxelGrid->GetSizeY(), BinaryVoxelGrid->GetSizeZ());
	MaxVoxels = FIntVector(FMath::Min(MaxVoxels.X, TotalVoxels.X), FMath::Min(MaxVoxels.Y, TotalVoxels.Y ), FMath::Min(MaxVoxels.Z, TotalVoxels.Z));

	for (int32 Z = MinVoxels.Z; Z < MaxVoxels.Z; Z++)
	{
		for (int32 Y = MinVoxels.Y; Y < MaxVoxels.Y; Y++)
		{
			for (int32 X = MinVoxels.X; X < MaxVoxels.X; X++)
			{
				bool test = BinaryVoxelGrid->GetState(X, Y, Z);
				if (!BinaryVoxelGrid->GetState(X, Y, Z)) return true;
			}
		}
	}
	return false;
}

FVector Maze::GetRandomLocation(FRandomStream& RandomStream) const
{
	uint32 TotalNodes = BinaryVoxelGrid->GetSizeX() * BinaryVoxelGrid->GetSizeY() * BinaryVoxelGrid->GetSizeZ();
	TArray<uint32> OpenNodes;
	for (uint32 i = 0; i < TotalNodes; i++)
	{
		if (BinaryVoxelGrid->GetState(i)) OpenNodes.Add(i);
	}

	uint32 RandomNode = OpenNodes[RandomStream.RandRange(0, OpenNodes.Num() - 1)];
	FIntVector NodeLocationIdx;
	NodeLocationIdx.Z = RandomNode % BinaryVoxelGrid->GetSizeZ();
	NodeLocationIdx.Y = (RandomNode / BinaryVoxelGrid->GetSizeZ()) % BinaryVoxelGrid->GetSizeY();
	NodeLocationIdx.X = RandomNode / (BinaryVoxelGrid->GetSizeY() * BinaryVoxelGrid->GetSizeZ());


	FBoxCenterAndExtent Voxel = GetVoxelBounds(NodeLocationIdx);

	return Voxel.Center;// +Voxel.Extent * RandomStream.VRand();
}

double Maze::GetTotalOpenVolume()
{
	double NodeSize = BinaryVoxelGrid->GetVoxelSize().X * BinaryVoxelGrid->GetVoxelSize().Y * BinaryVoxelGrid->GetVoxelSize().Z;
	uint32 TotalNodes = BinaryVoxelGrid->GetSizeX() * BinaryVoxelGrid->GetSizeY() * BinaryVoxelGrid->GetSizeZ();
	double TotalVolume = 0;
	for (uint32 i = 0; i < TotalNodes; i++)
	{
		if (BinaryVoxelGrid->GetState(i)) TotalVolume += NodeSize;
	}
	return TotalVolume;
}

float Maze::IntersectTet(FVector v1, FVector v2, FVector v3, FVector v4)
{
	Tetrahedron tet(v1, v2, v3, v4);

	FBox tetBounds = tet.GetBounds();

	TArray<FBox> Intersections;
	IntersectBox(tetBounds, Intersections);

	float totalOverlap = 0;
	for (FBox& box : Intersections)
	{
		auto tetMesh = Icarus::SurfaceMesh::FromTetrahedron(tet);

		auto boxMesh = Icarus::SurfaceMesh::FromBox(box);
		auto overlapMesh = tetMesh->Intersect(boxMesh);
		totalOverlap += overlapMesh->CalculateVolume();
	}

	return totalOverlap;

}

void Maze::IntersectBox(FBox TestBounds, TArray<FBox>& OutIntersections)
{
	if (!Bounds.Intersect(TestBounds))
	{
		return;
	}
	FBox Overlap = Bounds.Overlap(TestBounds);
	FVector Min = Overlap.Min - Bounds.Min;
	FVector Max = Overlap.Max - Bounds.Min;

	FVector l1 = Min / BinaryVoxelGrid->GetVoxelSize();
	FIntVector MinVoxels = FIntVector(FMath::Floor(l1.X), FMath::Floor(l1.Y), FMath::Floor(l1.Z));

	FVector l2 = Max / BinaryVoxelGrid->GetVoxelSize();
	FIntVector MaxVoxels = FIntVector(FMath::CeilToInt(l2.X), FMath::CeilToInt(l2.Y), FMath::CeilToInt(l2.Z));
	FIntVector TotalVoxels = FIntVector(BinaryVoxelGrid->GetSizeX(), BinaryVoxelGrid->GetSizeY(), BinaryVoxelGrid->GetSizeZ());
	MaxVoxels = FIntVector(FMath::Min(MaxVoxels.X, TotalVoxels.X), FMath::Min(MaxVoxels.Y, TotalVoxels.Y), FMath::Min(MaxVoxels.Z, TotalVoxels.Z));

	for (int32 Z = MinVoxels.Z; Z < MaxVoxels.Z; Z++)
	{
		for (int32 Y = MinVoxels.Y; Y < MaxVoxels.Y; Y++)
		{
			for (int32 X = MinVoxels.X; X < MaxVoxels.X; X++)
			{
				if (!BinaryVoxelGrid->GetState(X, Y, Z)) OutIntersections.Add(GetVoxelBounds(FIntVector(X,Y,Z)).GetBox());
			}
		}
	}
}

std::shared_ptr<Icarus::SurfaceMesh> Maze::GetSurfaceMesh() const
{
	std::shared_ptr<Icarus::SurfaceMesh> mesh = Icarus::SurfaceMesh::Create();
	uint32 TotalNodes = BinaryVoxelGrid->GetSizeX() * BinaryVoxelGrid->GetSizeY() * BinaryVoxelGrid->GetSizeZ();

	int64_t MaxX = BinaryVoxelGrid->GetSizeX();
	int64_t MaxY = BinaryVoxelGrid->GetSizeX();
	int64_t MaxZ = BinaryVoxelGrid->GetSizeX();


	for (int64_t Z = 0; Z < MaxZ; Z++)
	{
		for (int64_t Y = 0; Y < MaxY; Y++)
		{
			for (int64_t X = 0; X < MaxX; X++)
			{
				int64_t Idx = X + Y * MaxX + Z * MaxX * MaxY;
				if (BinaryVoxelGrid->GetState(Idx))
				{
					FBox box = GetVoxelBounds(FIntVector(X, Y, Z)).GetBox();
					std::shared_ptr<Icarus::SurfaceMesh> boxMesh = Icarus::SurfaceMesh::FromBox(box);
					if (boxMesh)
					{
						std::shared_ptr<Icarus::SurfaceMesh> result = mesh->Union(boxMesh);
						if (result) mesh = result;
					}
				}
			}
		}
	}

	return mesh;
}
