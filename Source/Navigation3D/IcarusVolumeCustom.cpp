// Fill out your copyright notice in the Description page of Project Settings.


#include "IcarusVolumeCustom.h"
#include "TetMeshVisualizer.h"
#include <IcarusNavGenerator.h>
#include <Tetrahedron.h>
#include <DrawDebugHelpers.h>
#include "Maze.h"
#include "Nav3DHelpers.h"
#include "SurfaceMesh.h"
#include <set>
#include "OcclusionInterface.h"

AIcarusVolumeCustom::AIcarusVolumeCustom()
{
	TetMeshVisualizer = CreateDefaultSubobject<UTetMeshVisualizer>(TEXT("TetMeshVisualizer"));
	TetMeshVisualizer->SetupAttachment(RootComponent);

	//Visualizer = CreateDefaultSubobject<UProceduralMeshComponent>("Visualizer");
	//Visualizer->SetupAttachment(RootComponent);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AIcarusVolumeCustom::BeginPlay()
{
	Super::BeginPlay();
	return;

}

void AIcarusVolumeCustom::Start()
{

	
}

void AIcarusVolumeCustom::Tick(float DeltaTime)
{

	if (false) {
		for (auto& Node : Nodes)
		{
			auto& connections = Node.getChildren();
			for (auto& connection : connections)
			{
				DrawDebugLine(GetWorld(), Node.Location, static_cast<TetNode*>(connection.first)->Location, FColor::Red);
			}
		}
	}

	return;
}

bool AIcarusVolumeCustom::IsOccluded(FBoxCenterAndExtent WorldBounds)
{
	return OcclusionObject->IsOccupied(WorldBounds.GetBox());



	/*
	//DrawDebugBox(GetWorld(), WorldBounds.Center, WorldBounds.Extent, FColor::Blue, false, 20.f);
	ECollisionChannel CollisionChannel = ECollisionChannel::ECC_WorldStatic;
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.bFindInitialOverlaps = true;
	CollisionQueryParams.bTraceComplex = false;
	CollisionQueryParams.TraceTag = "IcarusRasterize";
	bool test = GetWorld()->OverlapBlockingTestByChannel(
		WorldBounds.Center,
		FQuat::Identity,
		CollisionChannel,
		FCollisionShape::MakeBox(WorldBounds.Extent),
		CollisionQueryParams
	);
	return test;*/
}

void AIcarusVolumeCustom::OnBuildComplete(const TArray<FVector>& Vertices, const TArray<size_t>& Indices)
{
	TArray<int32> IndicesInt32;
	IndicesInt32.Reserve(Indices.Num());
	for (size_t indice : Indices) IndicesInt32.Add(static_cast<int32>(indice));
	//TetMeshVisualizer->SetTetMesh(Vertices, IndicesInt32);
	auto surfMesh = Icarus::SurfaceMesh::Create();
	for (int32 idx = 0; idx < /*Indices.Num()*/0; idx += 4)
	{
		Tetrahedron t(Vertices[Indices[idx]], Vertices[Indices[idx+1]], Vertices[Indices[idx+2]], Vertices[Indices[idx+3]]);
		auto ToUnion = Icarus::SurfaceMesh::FromTetrahedron(t);
		auto result = surfMesh->Union(ToUnion);
		TArray<FVector> SurfVertices;
		TArray<int32> SurfIndices;
		surfMesh->GetVerticesAndIndices(SurfVertices, SurfIndices);
		if(result)surfMesh = result;
	}

	//TArray<FVector> SurfVertices;
	//TArray<int32> SurfIndices;
	//surfMesh->GetVerticesAndIndices(SurfVertices, SurfIndices);

	//Visualizer->CreateMeshSection(0, SurfVertices, SurfIndices, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);

	TetVertices = Vertices;
	TetIndices = Indices;
	NumberOfTets = Vertices.Num();

	BuildNodeTree();
}

bool AIcarusVolumeCustom::FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPath, float AbortTimer, bool& IsAborted)
{
	int32 NodeStartIdx = -1;
	int32 NodeGoalIdx = -1;

	NodeOctree->FindElementsWithBoundsTest(FBoxCenterAndExtent(Start, FVector(100, 100, 100)), [this, Start, &NodeStartIdx](const OctreeNode& node) {
		FVector v1 = TetVertices[TetIndices[node.Index * 4]];
		FVector v2 = TetVertices[TetIndices[node.Index * 4 + 1]];
		FVector v3 = TetVertices[TetIndices[node.Index * 4 + 2]];
		FVector v4 = TetVertices[TetIndices[node.Index * 4 + 3]];
		if (Nav3DHelpers::TetrahedronPointTest(v1, v2, v3, v4, Start))NodeStartIdx = (int32)node.Index;
	});
	NodeOctree->FindElementsWithBoundsTest(FBoxCenterAndExtent(Goal, FVector(0, 0, 0)), [this, Goal, &NodeGoalIdx](const OctreeNode& node) {
		FVector v1 = TetVertices[TetIndices[node.Index * 4]];
		FVector v2 = TetVertices[TetIndices[node.Index * 4 + 1]];
		FVector v3 = TetVertices[TetIndices[node.Index * 4 + 2]];
		FVector v4 = TetVertices[TetIndices[node.Index * 4 + 3]];
		if (Nav3DHelpers::TetrahedronPointTest(v1, v2, v3, v4, Goal))NodeGoalIdx = (int32)node.Index;
	});

	if (NodeStartIdx == -1 || NodeGoalIdx == -1) return false;


	std::vector<TetNode*> path;
	PathFinder<TetNode> myFinder;
	myFinder.setStart(Nodes[NodeStartIdx]);
	myFinder.setGoal(Nodes[NodeGoalIdx]);
	
	bool result = myFinder.findPath<AStar>(path,AbortTimer,IsAborted);
	if (result)
	{
		OutPath.Empty();
		OutPath.Reserve(path.size()+2);
		OutPath.Add(Start);
		for (TetNode* node : path) OutPath.Add(static_cast<TetNode*>(node)->Location);
		OutPath.Add(Goal);
	}
	return result;
}

IcarusPerfData AIcarusVolumeCustom::GatherData() const
{
	IcarusPerfData data = Super::GatherData();
	
	//Calculate Connections
	struct Connection
	{
		const TetNode* t1;
		const TetNode* t2;
		bool operator==(const Connection& s) const
		{
			return t1 == s.t1 && t2 == s.t2;
		};
		bool operator<(const Connection& rhs)
		{
			return GetHash() < rhs.GetHash();
		}
		uint32 GetHash() const {
			return FCrc::MemCrc32(this, sizeof(Connection));
		}
	};
	auto cmp = [](const Connection& c1, const Connection& c2) {return c1.t1 < c2.t1; };
	std::set<Connection, decltype(cmp)> Connections(cmp);
	for (const TetNode& Node : Nodes)
	{
		for (auto Neigbour : Node.getChildren())
		{
			const TetNode* t1 = &Node;
			Connection c1{ &Node, static_cast<TetNode*>(Neigbour.first)};
			Connection c2{ static_cast<TetNode*>(Neigbour.first),&Node};
			if (Connections.count(c2) == 0 && Connections.count(c1) == 0)
			{
				Connections.insert(c1);
			}
		}
	}
	data.NumberOfConnections = Connections.size();
	data.NumberOfNodes = Nodes.Num();
	
	data.MemoryUsage = TetVertices.Num() * sizeof(FVector) + TetIndices.Num() * sizeof(int32) + Nodes.Num() * sizeof(TetNode) + Connections.size() * sizeof(TetNode*);
	return data;
}

float AIcarusVolumeCustom::CalculateIncorrectArea() const
{

	

	float Area = 0;
	for (size_t i = 0; i < TetIndices.Num(); i += 4)
	{
		Area += OcclusionObject->IntersectTet(TetVertices[TetIndices[i]], TetVertices[TetIndices[i + 1]], TetVertices[TetIndices[i + 2]], TetVertices[TetIndices[i + 3]]);
		//Tetrahedron tet(TetVertices[TetIndices[i]], TetVertices[TetIndices[i + 1]], TetVertices[TetIndices[i + 2]], TetVertices[TetIndices[i + 3]]);
		//auto tetMesh = Icarus::SurfaceMesh::FromTetrahedron(tet);
		//Area += tetMesh->CalculateVolume();
	}
	return Area;
}

float AIcarusVolumeCustom::CalculateTotalArea() const
{
	float Area = 0;
	for (size_t i = 0; i < TetIndices.Num(); i+=4)
	{
		Tetrahedron tet(TetVertices[TetIndices[i]], TetVertices[TetIndices[i+1]], TetVertices[TetIndices[i+2]], TetVertices[TetIndices[i+3]]);
		auto tetMesh = Icarus::SurfaceMesh::FromTetrahedron(tet);
		Area += tetMesh->CalculateVolume();
	}
	return Area;
}

void AIcarusVolumeCustom::Show()
{
	TetMeshVisualizer->SetVisibility(true);

	TArray<int32> IndicesInt32;
	IndicesInt32.Reserve(TetIndices.Num());
	for (size_t indice : TetIndices) IndicesInt32.Add(static_cast<int32>(indice));
	TetMeshVisualizer->SetTetMesh(TetVertices, IndicesInt32);
}


void AIcarusVolumeCustom::Hide()
{
	TetMeshVisualizer->SetVisibility(false);
}

void AIcarusVolumeCustom::Save(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/TetMesh/";
	FString FullPath = SaveDirectory + name + ".tetMeshData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileWriter(*FullPath, 0));
	Serialize(*Ar.Get());
}

void AIcarusVolumeCustom::Load(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/TetMesh/";
	FString FullPath = SaveDirectory + name + ".tetMeshData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	if(Ar)Serialize(*Ar.Get());
}

void AIcarusVolumeCustom::Serialize(FArchive& Ar)
{
	size_t NumberOfVertices = TetVertices.Num();
	size_t NumberOfIndicies = TetIndices.Num();
	Ar.Serialize(&NumberOfVertices,sizeof(NumberOfVertices));
	Ar.Serialize(&NumberOfIndicies, sizeof(NumberOfIndicies));

	if (!Ar.IsSaving())
	{
		TetVertices.SetNumUninitialized(NumberOfVertices);
		TetIndices.SetNumUninitialized(NumberOfIndicies);
	}
	for (size_t i = 0; i < NumberOfVertices; i++)
	{
		Ar.Serialize(&TetVertices[i], sizeof(TetVertices[i]));
	}
	for (size_t i = 0; i < NumberOfIndicies; i++)
	{
		Ar.Serialize(&TetIndices[i], sizeof(TetIndices[i]));
	}

	Ar.Serialize(&NumberOfTets, sizeof(NumberOfTets));
	Ar << PerfData.GenerationTime;

	if (!Ar.IsSaving())BuildNodeTree();
}

void AIcarusVolumeCustom::BuildNodeTree()
{
	AStar::getInstance().clear();

	size_t NumNodes = TetIndices.Num() / 4;
	Nodes.Empty();
	Nodes.Init(TetNode(), NumNodes);
	TotalNodes = NumNodes;

	//Fill Nodes
	for (size_t idx = 0; idx < Nodes.Num(); idx++)
	{
		TetNode& node = Nodes[idx];
		node.Idx = idx;
		size_t baseI = idx * 4;
		node.Location = (TetVertices[TetIndices[baseI]] + TetVertices[TetIndices[baseI + 1]] + TetVertices[TetIndices[baseI + 2]] + TetVertices[TetIndices[baseI + 3]]) / 4.f;

	}

	//Fill Octree
	FBoxSphereBounds OctreeBounds = GetBounds();

	NodeOctree = MakeUnique<TOctree2< OctreeNode, FOctreeSemantics>>(
		FVector(0, 0, 0), OctreeBounds.BoxExtent.GetMax()
		);
	TArray<FBoxCenterAndExtent> CachedBounds;
	CachedBounds.Reserve(TotalNodes);
	for (int32 i = 0; i < TotalNodes; i++)
	{
		FBox Box(ForceInit);

		int32 indiceBaseIdx = i * 4;
		for (size_t j = 0; j < 4; j++)
		{
			Box += TetVertices[TetIndices[indiceBaseIdx + j]];
		}
		FBoxCenterAndExtent Bounds(Box);
		CachedBounds.Add(Bounds);
		NodeOctree->AddElement(OctreeNode{ i,Bounds });
	}
	//Find Neigbours
	for (size_t i = 0; i < TotalNodes; i++)
	{
		TArray<size_t> MyIndices;
		MyIndices.Add(TetIndices[i * 4]);
		MyIndices.Add(TetIndices[i * 4 + 1]);
		MyIndices.Add(TetIndices[i * 4 + 2]);
		MyIndices.Add(TetIndices[i * 4 + 3]);

		FBoxCenterAndExtent& Bounds = CachedBounds[i];
		Bounds.Extent * 1.5f;

		NodeOctree->FindElementsWithBoundsTest(Bounds, [&MyIndices,this,i](const OctreeNode& Node) {
			if (i == Node.Index) return;
			int32 baseIndex = Node.Index * 4;
			int neighbourVertices = 0;
			for (size_t j = 0; j < 4; j++)
			{
				if (MyIndices.Contains(TetIndices[baseIndex + j]))neighbourVertices++;
			}
			if (neighbourVertices == 3)
			{
				Nodes[i].addChild(&Nodes[Node.Index], Nodes[i].distanceTo(&Nodes[Node.Index]));
			}
		});
	}


	//Validate
	bool Validated = true;

	for (auto& Node : Nodes)
	{
		auto& neigbours = Node.getChildren();
		for (auto& neigbour : neigbours)
		{
			auto& test = neigbour.first->getChildren();
			TetNode* N = &Node;
			bool found = false;
			for (auto& B : test)
			{
				if (B.first == N)
				{
					found = true;
					break;
				}
			}
			if (found == false) Validated = false;

		}
		if (Validated == false) break;
	}
	int test = 0;
}
