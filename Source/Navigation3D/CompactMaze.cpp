#include "CompactMaze.h"

#include "Math/RandomStream.h"
#include "SurfaceMesh.h"
#include "Tetrahedron.h"
#include <DrawDebugHelpers.h>
#include <chrono>

//This assumes that normal > 0 on one axis!
bool* CompactMaze::GetFaceFromVoxel(FIntVector VoxelLocation, FIntVector Normal)
{
	FaceDirection FaceDir;
	bool bIsPosive;
	if (Normal.X != 0) FaceDir = FaceDirection::YZ;
	else if (Normal.Y != 0) FaceDir = FaceDirection::XZ;
	else FaceDir = FaceDirection::XY;
	bIsPosive = (Normal.X + Normal.Y + Normal.Z) > 0;

	//Extract xy
	int x = 0;
	int y = 0;
	int depth = 0;
	switch (FaceDir)
	{
	case XY:
		x = VoxelLocation.X;
		y = VoxelLocation.Y;
		depth = VoxelLocation.Z;
		break;
	case XZ:
		x = VoxelLocation.X;
		y = VoxelLocation.Z;
		depth = VoxelLocation.Y;
		break;
	case YZ:
		x = VoxelLocation.Y;
		y = VoxelLocation.Z;
		depth = VoxelLocation.X;
		break;
	default:
		break;
	}

	//offset depth
	if (bIsPosive) depth++;
	return GetFace(FaceDir, depth, x, y);
}

bool* CompactMaze::GetFace(FaceDirection direction,int depth, int x, int y)
{
	//Get plane
	int GridSize = GetGridSize();
	int PlaneByteSize = GridSize* GridSize;
	int NumberOfSlices = GridSize + 1;
	int DirectionByteSize = NumberOfSlices * PlaneByteSize;

	//Start with slice direction offset
	int Offset = DirectionByteSize * direction;

	//Offset plane
	Offset += depth * PlaneByteSize;

	//Offset xy on plane
	Offset += x + y * GridSize;


	return FaceStates + Offset;
}

FVector CompactMaze::GetVoxelLocation(FIntVector idx)
{
	FVector FloatingIdx = FVector(idx);
	//float VoxelSize = (Bounds.Max.X - Bounds.Min.Y) / GetGridSize();

	return Bounds.Min + FloatingIdx * VoxelSize + FVector(VoxelSize/2);
}

void CompactMaze::GenerateCollisionBoxes()
{
	int GridSize = GetGridSize();


	
	struct Edge
	{
		int FaceIdxA;
		int FaceIdxB;
	};

	struct Corner
	{
		int FaceIdxA;
		int FaceIdxB;
		int FaceIdxC;
	};

	constexpr size_t NumberOfFaces = 6;
	constexpr size_t NumberOfEdges = 12;
	constexpr size_t NumberOfCorners = 8;

	FIntVector Faces[NumberOfFaces] =
	{
		FIntVector(0, 0, 1),
		FIntVector(0, 0, -1),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0),
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0)
	};
	Edge Edges[NumberOfEdges] =
	{
		{0,2},
		{0,3},
		{0,4},
		{0,5},
		{2,4},
		{2,5},
		{3,4},
		{3,5},
		{1,2},
		{1,3},
		{1,4},
		{1,5},
	};

	Corner Corners[NumberOfCorners] =
	{
		{0,2,4},
		{0,4,3},
		{0,3,5},
		{0,5,2},
		{1,2,4},
		{1,4,3},
		{1,3,5},
		{1,5,2}
	};

	auto substractHelper = [](FBox& box, FIntVector dir, float cutDistance)
	{
		FVector substract = FVector(dir) * cutDistance;

		box.Min += FVector(FMath::Max(0.f, substract.X), FMath::Max(0.f, substract.Y), FMath::Max(0.f, substract.Z));
		box.Max += FVector(FMath::Min(0.f, substract.X), FMath::Min(0.f, substract.Y), FMath::Min(0.f, substract.Z));
	};
	CollisionBoxes.Reserve(FMath::Pow((GridSize + 1)* GridSize* GridSize, 3));

	for (int z = 0; z < GridSize; z++)
	{
		for (int y = 0; y < GridSize; y++)
		{
			for (int x = 0; x < GridSize; x++)
			{
				FIntVector VoxelLocation(x, y, z);
				FBox Voxel = GetVoxelBounds(FIntVector(x, y, z)).GetBox();
				float VoxelWidth = Voxel.Max.X - Voxel.Min.X;
				float faceWidth = VoxelWidth * faceWidthPrecentile;
				FBox uncutFaces[NumberOfFaces];
				FBox cutFaces[NumberOfFaces];

				//generate sides
				for (size_t i = 0; i < NumberOfFaces; i++)
				{
					Voxel = GetVoxelBounds(FIntVector(x, y, z)).GetBox();
					bool faceState = *GetFaceFromVoxel(VoxelLocation, Faces[i]);
					substractHelper(Voxel, Faces[i], (VoxelWidth - faceWidth));
					uncutFaces[i] = Voxel;
					
					//remove edges
					for (size_t j = 0; j < NumberOfFaces; j++)
					{
						FIntVector v1Abs(FMath::Abs(Faces[i].X), FMath::Abs(Faces[i].Y), FMath::Abs(Faces[i].Z));
						FIntVector v2Abs(FMath::Abs(Faces[j].X), FMath::Abs(Faces[j].Y), FMath::Abs(Faces[j].Z));
						if (v1Abs != v2Abs)
						{
							substractHelper(Voxel, Faces[j]*-1, faceWidth);
						}
					}
					cutFaces[i] = Voxel;
					if (!faceState) {
						CollisionBoxes.Add(Voxel);
						//NodeOctree->AddElement(OctreeNode{ NodeIdxCounter++,FBoxCenterAndExtent(Voxel) });
						
					}
				}


				//generate edges
				for (size_t i = 0; i < NumberOfEdges; i++)
				{
					int FaceIdxA = Edges[i].FaceIdxA;
					int FaceIdxB = Edges[i].FaceIdxB;

					bool faceStateA = *GetFaceFromVoxel(VoxelLocation, Faces[FaceIdxA]);
					bool faceStateB = *GetFaceFromVoxel(VoxelLocation, Faces[FaceIdxB]);
					if (!faceStateA || !faceStateB) {
						Voxel = GetVoxelBounds(FIntVector(x, y, z)).GetBox();
						substractHelper(Voxel, Faces[FaceIdxA], (VoxelWidth - faceWidth));
						substractHelper(Voxel, Faces[FaceIdxB], (VoxelWidth - faceWidth));

						//remove edges
						for (size_t j = 0; j < NumberOfFaces; j++)
						{
							FIntVector v1AbsA(FMath::Abs(Faces[FaceIdxA].X), FMath::Abs(Faces[FaceIdxA].Y), FMath::Abs(Faces[FaceIdxA].Z));
							FIntVector v1AbsB(FMath::Abs(Faces[FaceIdxB].X), FMath::Abs(Faces[FaceIdxB].Y), FMath::Abs(Faces[FaceIdxB].Z));

							FIntVector v2Abs(FMath::Abs(Faces[j].X), FMath::Abs(Faces[j].Y), FMath::Abs(Faces[j].Z));
							if (v1AbsA != v2Abs && v1AbsB != v2Abs)
							{
								substractHelper(Voxel, Faces[j] * -1, faceWidth);
							}
						}

						CollisionBoxes.Add(Voxel);
						//NodeOctree->AddElement(OctreeNode{ NodeIdxCounter++,FBoxCenterAndExtent(Voxel) });
					}
				}

				

				//generate corners
				for (size_t i = 0; i < NumberOfCorners; i++)
				{
					int FaceIdxA = Corners[i].FaceIdxA;
					int FaceIdxB = Corners[i].FaceIdxB;
					int FaceIdxC = Corners[i].FaceIdxC;

					bool faceStateA = *GetFaceFromVoxel(VoxelLocation, Faces[FaceIdxA]);
					bool faceStateB = *GetFaceFromVoxel(VoxelLocation, Faces[FaceIdxB]);
					bool faceStateC = *GetFaceFromVoxel(VoxelLocation, Faces[FaceIdxC]);

					if (!faceStateA || !faceStateB || !faceStateC) {
						Voxel = GetVoxelBounds(FIntVector(x, y, z)).GetBox();
						substractHelper(Voxel, Faces[FaceIdxA], (VoxelWidth - faceWidth));
						substractHelper(Voxel, Faces[FaceIdxB], (VoxelWidth - faceWidth));
						substractHelper(Voxel, Faces[FaceIdxC], (VoxelWidth - faceWidth));
						CollisionBoxes.Add(Voxel);
						NodeOctree->AddElement(OctreeNode{ NodeIdxCounter++,FBoxCenterAndExtent(Voxel) });
					}
				}

			}
		}
	}
	CollisionBoxes.Shrink();
}

CompactMaze::CompactMaze(FBox Bounds, FVector Location, int seed, int complexity)
	: Bounds(Bounds)
	, Seed(seed)
	, Complexity(complexity)
	, VoxelSize((Bounds.Max.X-Bounds.Min.X) / GetGridSize())
{

	NodeOctree = MakeUnique<TOctree2< OctreeNode, FOctreeSemantics>>(
		FVector(0, 0, 0), Bounds.GetExtent().GetMax()
		);
}

CompactMaze::CompactMaze()
{

}

CompactMaze::~CompactMaze()
{
	if (FaceStates)FMemory::Free(FaceStates);

}

TSharedPtr<CompactMaze> CompactMaze::LoadMaze(const FString& name)
{
	TSharedPtr<CompactMaze> GeneratedMaze = TSharedPtr<CompactMaze>(new CompactMaze());
	if (GeneratedMaze->Load(name)) {
		return GeneratedMaze;
	}
	else return TSharedPtr<CompactMaze>();
}
TSharedPtr<CompactMaze> CompactMaze::GenerateMaze(FBox Bounds, FVector Location, int seed, int complexity)
{

	auto GridTo1D = [](FIntVector location, size_t width)
	{
		return location.Z * width * width + location.Y * width + location.X;
	};
	auto IsWithinBounds = [](FIntVector location, size_t width)
	{
		return location.X < width&& location.X >= 0 && location.Y < width&& location.Y >= 0 && location.Z < width&& location.Z >= 0;
	};
	FRandomStream RandomStream(seed);

	TSharedPtr<CompactMaze> GeneratedMaze = TSharedPtr<CompactMaze>(new CompactMaze(Bounds, Location, seed, complexity));



	int GridSize = GeneratedMaze->GetGridSize();
	int PlaneByteSize = GridSize * GridSize;
	int NumberOfSlices = GridSize + 1;
	int DirectionByteSize = NumberOfSlices * PlaneByteSize;
	int FaceStatesSize = (PlaneByteSize * NumberOfSlices) * 3;

	GeneratedMaze->FaceStates = (bool*)FMemory::MallocZeroed(FaceStatesSize * sizeof(bool));
	GeneratedMaze->NumFaceStates = FaceStatesSize;
	int TotalVoxels = GridSize * GridSize * GridSize;

	enum NodeState : uint8 {
		UNVISITED = 0,
		VISITED = 1
	};
	NodeState* VoxelGridState = (NodeState*)FMemory::MallocZeroed(TotalVoxels * sizeof(NodeState));
	TArray<FIntVector> VisitedStack;

	FIntVector StartLocation = FIntVector(RandomStream.RandRange(0, GridSize - 1), RandomStream.RandRange(0, GridSize - 1), RandomStream.RandRange(0, GridSize - 1));
	VoxelGridState[GridTo1D(StartLocation, GridSize)] = VISITED;
	VisitedStack.Push(StartLocation);

	while (VisitedStack.Num() > 0)
	{
		//Get Top of Stack
		FIntVector Top = VisitedStack.Top();

		//Gather viable neigbours
		TArray<FIntVector> PossibleNeigbours;
		TArray<FIntVector> Neigbours;
		PossibleNeigbours.Add(FIntVector(1, 0, 0));
		PossibleNeigbours.Add(FIntVector(-1, 0, 0));
		PossibleNeigbours.Add(FIntVector(0, 1, 0));
		PossibleNeigbours.Add(FIntVector(0, -1, 0));
		PossibleNeigbours.Add(FIntVector(0, 0, 1));
		PossibleNeigbours.Add(FIntVector(0, 0, -1));

		for (FIntVector& possibleNeigbour : PossibleNeigbours)
		{
			FIntVector NeigbourLocation = possibleNeigbour + Top;
			if (IsWithinBounds(NeigbourLocation, GridSize))
			{
				auto t2 = GridTo1D(possibleNeigbour, GridSize);
				auto state = VoxelGridState[GridTo1D(possibleNeigbour, GridSize)];
				if (VoxelGridState[GridTo1D(NeigbourLocation, GridSize)] == UNVISITED)
				{
					Neigbours.Add(possibleNeigbour);
				}
			}
		}

		if (Neigbours.Num() > 0) {

			FIntVector Neigbour = Neigbours[RandomStream.RandRange(0, Neigbours.Num() - 1)];
			FIntVector NeigbourLocation = Neigbour + Top;
			
			bool* state = GeneratedMaze->GetFaceFromVoxel(Top, Neigbour);
			*state = true;
			VoxelGridState[GridTo1D(NeigbourLocation, GridSize)] = VISITED;

			VisitedStack.Push(NeigbourLocation);

		}
		else
		{
			VisitedStack.Pop(false);
		}

	}


	FMemory::Free(VoxelGridState);
	GeneratedMaze->GenerateCollisionBoxes();
	return GeneratedMaze;
}


FVector CompactMaze::GetRandomLocation(FRandomStream& RandomStream) const
{
	while (true)
	{
		FVector Location = FVector(RandomStream.FRandRange(Bounds.Min.X, Bounds.Max.X), RandomStream.FRandRange(Bounds.Min.Y, Bounds.Max.Y), RandomStream.FRandRange(Bounds.Min.Z, Bounds.Max.Z));
		bool Valid = true;
		NodeOctree->FindElementsWithBoundsTest(FBoxCenterAndExtent(Location, FVector::ZeroVector), [this,&Valid, &Location](const OctreeNode& Node)
			{
				if (CollisionBoxes[Node.Index].IsInside(Location)) Valid = false;
			});
		if (Valid) return Location;
	}



	return FVector();
}

double CompactMaze::GetTotalOpenVolume()
{
	FVector BoundSize = Bounds.Max - Bounds.Min;
	double totalVolume = BoundSize.X * BoundSize.Y * BoundSize.Z;

	for (const auto& box : CollisionBoxes)
	{
		FVector BoxSize = box.Max - box.Min;
		totalVolume -= BoxSize.X * BoxSize.Y * BoxSize.Z;
	}

	return totalVolume;
}
bool CompactMaze::IsOccupied(const FBox& TestBounds)
{
	bool occupied = false;
	NodeOctree->FindFirstElementWithBoundsTest(FBoxCenterAndExtent(TestBounds), [&occupied](const OctreeNode& Node) {
		occupied = true;
		return false;
		});
	return occupied;



	return false;
}

static int failed = 0;
static int succes = 0;
static float accumt1 = 0;
static float accumt2 = 0;
static FCriticalSection m_mutex;

float CompactMaze::IntersectTet(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4)
{
	
	Tetrahedron tet(v1, v2, v3, v4);

	TArray<FBox> Intersections;
	IntersectBox(tet.GetBounds(), Intersections);

	float totalOverlap = 0;
	for (FBox& box : Intersections)
	{
		if (tet.IsTouchingBox(box)) {
			auto tetMesh = Icarus::SurfaceMesh::FromTetrahedron(tet);
			auto boxMesh = Icarus::SurfaceMesh::FromBox(box);
			bool didSucceed = boxMesh->Intersect2(tetMesh);
			float volume = boxMesh->CalculateVolume();
			totalOverlap += volume;
		}
	}
	return totalOverlap;
}

bool CompactMaze::IntersectTetTest(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4)
{
	Tetrahedron tet(v1, v2, v3, v4);

	TArray<FBox> Intersections;
	IntersectBox(tet.GetBounds(), Intersections);

	float totalOverlap = 0;
	for (FBox& box : Intersections)
	{
		if (tet.IsTouchingBox(box)) {
			auto tetMesh = Icarus::SurfaceMesh::FromTetrahedron(tet);
			auto boxMesh = Icarus::SurfaceMesh::FromBox(box);
			bool didSucceed = boxMesh->Intersect2(tetMesh);
			float volume = boxMesh->CalculateVolume();
			totalOverlap += volume;
			if (totalOverlap > 0) return true;
		}
	}
	return false;
}

void CompactMaze::IntersectBox(const FBox& BoxBounds, TArray<FBox>& OutIntersections)
{
	NodeOctree->FindElementsWithBoundsTest(BoxBounds, [&OutIntersections](const OctreeNode& Node) {
		OutIntersections.Add(Node.BoxCenterAndExtent.GetBox());
	});
}

std::shared_ptr<Icarus::SurfaceMesh> CompactMaze::GetSurfaceMesh() const
{
	return std::shared_ptr<Icarus::SurfaceMesh>();
}


void CompactMaze::DrawDebug(UWorld* world)
{

	int NumSlices = GetGridSize() + 1;

	float DistanceBetweenSlices = (Bounds.Max - Bounds.Min).X / GetGridSize();
	float DistanceBetweenVoxels = (Bounds.Max - Bounds.Min).X / GetGridSize();
	
	const int GridSize = GetGridSize();
	//XY
	for (int depth = 0; depth < NumSlices; depth++)
	{
		for (int y = 0; y < GridSize; y++)
		{
			for (int x = 0; x < GridSize; x++)
			{
				if (*GetFace(XY, depth, x, y))
				{
					DrawDebugLine(world, GetVoxelLocation(FIntVector(x, y, depth-1)), GetVoxelLocation(FIntVector(x, y, depth)), FColor::Green);
				}

			}
		}
	}
	//XZ
	for (int depth = 0; depth < NumSlices; depth++)
	{
		for (int y = 0; y < GridSize; y++)
		{
			for (int x = 0; x < GridSize; x++)
			{
				if (*GetFace(XZ, depth, x, y))
				{
					DrawDebugLine(world, GetVoxelLocation(FIntVector(x, depth - 1, y)), GetVoxelLocation(FIntVector(x, depth, y)), FColor::Green);
				}

			}
		}
	}
	//YZ
	for (int depth = 0; depth < NumSlices; depth++)
	{
		for (int y = 0; y < GridSize; y++)
		{
			for (int x = 0; x < GridSize; x++)
			{
				if (*GetFace(YZ, depth, x, y))
				{
					DrawDebugLine(world, GetVoxelLocation(FIntVector(depth - 1,x, y)), GetVoxelLocation(FIntVector(depth, x,  y)), FColor::Green);
				}
			}
		}
	}



	//DrawCollisionBoxes
	for (auto& box : CollisionBoxes)
	{
		DrawDebugBox(world, box.GetCenter(), box.GetExtent(), FColor::Red);
	}

	
	return;
	//Draw grid
	for (int y = 0; y < NumSlices; y++)
	{
		for (int x = 0; x < NumSlices; x++)
		{
			//Z
			FVector start = FVector(x * DistanceBetweenSlices, y * DistanceBetweenSlices, 0) + Bounds.Min;
			FVector end = FVector(start.X, start.Y, Bounds.Max.Z);
			DrawDebugLine(world, start, end, FColor::Blue);

			//Y
			start = FVector(x * DistanceBetweenSlices,0, y * DistanceBetweenSlices) + Bounds.Min;
			end = FVector(start.X, Bounds.Max.Y, start.Z);
			DrawDebugLine(world, start, end, FColor::Blue);

			//X
			start = FVector(0, x * DistanceBetweenSlices, y * DistanceBetweenSlices) + Bounds.Min;
			end = FVector(Bounds.Max.X, start.Y, start.Z);
			DrawDebugLine(world, start, end, FColor::Blue);
		}
	}
}

void CompactMaze::Save(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Maze/";
	//SaveDirectory = "D:/Data/Maze/";
	FString FullPath = SaveDirectory + name + ".mazeData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileWriter(*FullPath, 0));
	Serialize(*Ar.Get());
}

bool CompactMaze::Load(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/Maze/";
	//SaveDirectory = "D:/Data/Maze/";
	FString FullPath = SaveDirectory + name + ".mazeData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	if(Ar)Serialize(*Ar.Get());
	return Ar.IsValid();
}

void CompactMaze::Serialize(FArchive& Ar)
{
	//FBox Bounds, FVector Location, int seed, int complexity

	Ar.Serialize(&Bounds, sizeof(Bounds));
	Ar.Serialize(&Seed, sizeof(Seed));
	Ar.Serialize(&Complexity, sizeof(Complexity));


	auto NumberOfCollisionBoxes = CollisionBoxes.Num();
	Ar.Serialize(&NumberOfCollisionBoxes, sizeof(NumberOfCollisionBoxes));

	if (!Ar.IsSaving())
	{
		CollisionBoxes.SetNumUninitialized(NumberOfCollisionBoxes);
	}

	for (size_t i = 0; i < NumberOfCollisionBoxes; i++)
	{
		Ar.Serialize(&CollisionBoxes[i], sizeof(FBox));
	}
	Ar.Serialize(&NumFaceStates, sizeof(NumFaceStates));

	if (!Ar.IsSaving())
	{
		if (FaceStates) FMemory::Free(FaceStates);
		FaceStates = (bool*)FMemory::Malloc(NumFaceStates * sizeof(bool));
	}

	Ar.Serialize(FaceStates, NumFaceStates * sizeof(bool));
	if (!Ar.IsSaving())
	{
		NodeOctree = MakeUnique<TOctree2< OctreeNode, FOctreeSemantics>>(
			FVector(0, 0, 0), Bounds.GetExtent().GetMax()
			);
		for (const FBox& box : CollisionBoxes)
		{
			
			NodeOctree->AddElement(OctreeNode{ NodeIdxCounter++,FBoxCenterAndExtent(box) });
		}
	}
	Ar.Close();
}



FBoxCenterAndExtent CompactMaze::GetVoxelBounds(FIntVector location) const
{
	const FVector Offset = -Bounds.GetExtent();
	const FVector HalfVoxel = FVector(VoxelSize) / 2.f;
	const FVector Centre = Offset + FVector(location) * VoxelSize + HalfVoxel;
	return FBoxCenterAndExtent(Centre, FVector(VoxelSize / 2.f));
}

