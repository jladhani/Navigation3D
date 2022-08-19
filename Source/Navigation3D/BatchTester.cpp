// Fill out your copyright notice in the Description page of Project Settings.


#include "BatchTester.h"
#include "IcarusVolumeCustom.h"
#include "Maze.h"
#include "CompactMaze.h"
#include <DrawDebugHelpers.h>
#include <Nav3DVolume.h>
#include <Nav3DComponent.h>
#include <chrono>
#include "ProceduralMeshComponent.h"
// Sets default values
ABatchTester::ABatchTester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");
	Nav3DComponent = CreateDefaultSubobject<UNav3DComponent>("Nav3dComponent");
	Visualizer = CreateDefaultSubobject<UProceduralMeshComponent>("Visualizer");
	Visualizer->SetupAttachment(RootComponent);
	AddOwnedComponent(Nav3DComponent);

	SequenceSettings.Add(50);
	SequenceSettings.Add(100);
	SequenceSettings.Add(200);


	//Nav3DComponent->Attach()
}

// Called when the game starts or when spawned
void ABatchTester::BeginPlay()
{
	Super::BeginPlay();
	StartBatchTesting();
	
}

// Called every frame
void ABatchTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsTesting) BatchTestSingleStep();
}

void ABatchTester::BatchTest()
{
	//AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask,)

}

void ABatchTester::StartBatchTesting()
{
	randomStream = FRandomStream(0);
	IsTesting = true;
	VoxelSize = SequenceSettings[1];
}


void ABatchTester::SaveSVO(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/SVO/";
	FString FullPath = SaveDirectory + name + ".SVOMeshData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileWriter(*FullPath, 0));
	Nav3DVolume->Serialize2(*Ar.Get());
}

void ABatchTester::LoadSVO(const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/SVO/";
	FString FullPath = SaveDirectory + name + ".SVOMeshData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	Nav3DVolume->Serialize2(*Ar.Get());
}

void ABatchTester::BatchTestSingleStep()
{
	if (TestID >= AmountOfTests) return;
	check(IcarusVolume && Nav3DVolume);
	if (IcarusVolume == nullptr) return;
	float Complexity = (float)TestID / (float)AmountOfTests;

	int mazeComplexity = FMath::RoundToInt((float)MaxMazeSize * Complexity);
	TSharedPtr<CompactMaze> GeneratedMaze = CompactMaze::GenerateMaze(IcarusVolume->GetBounds().GetBox(), IcarusVolume->GetActorLocation(), randomStream.RandRange(0, 10000000), mazeComplexity);

	///Icarus Test
	IcarusVolume->VoxelSize = VoxelSize;
	IcarusVolume->SetOcclusionInterface(GeneratedMaze);
	IcarusVolume->Rebuild();

	double TotalArea = IcarusVolume->CalculateTotalArea();
	double IncorrectArea = IcarusVolume->CalculateIncorrectArea();
	double TotalFreeSpace = GeneratedMaze->GetTotalOpenVolume();

	double IcarusOpenSpace = TotalArea - IncorrectArea;

	FVector Ex = IcarusVolume->GetBounds().BoxExtent * 2.f;
	double Test = Ex.X * Ex.Y * Ex.Z;


	NavData navDataIcarus;
	IcarusPerfData perf = IcarusVolume->GatherData();
	navDataIcarus.TestId = TestID;
	navDataIcarus.VoxelSize = VoxelSize;
	navDataIcarus.SceneComplexity = Complexity;
	navDataIcarus.DataStructure = TETMESH;
	navDataIcarus.WorldType = ESceneType::MAZE;
	navDataIcarus.OpenSpaceCovered = (IcarusOpenSpace / TotalFreeSpace) * 100.f;
	navDataIcarus.IncorrectArea = IncorrectArea / TotalArea * 100.f;
	navDataIcarus.NumberOfNodes = perf.NumberOfNodes;
	navDataIcarus.NumberOfConnections = perf.NumberOfConnections;
	navDataIcarus.GenerationTime = perf.GenerationTime;
	navDataIcarus.MemoryUsage = perf.MemoryUsage;
	Collector.SupplyData(navDataIcarus);

	///Nav3d Test

	Nav3DVolume->IsOccludedDel.BindRaw(GeneratedMaze.Get(), &OcclusionInterface::IsOccupied);
	Nav3DVolume->VoxelSize = VoxelSize;
	auto t1 = std::chrono::steady_clock::now();

	Nav3DVolume->BuildOctree();
	auto t2 = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration<float>(t2 - t1);
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();


	NavData navDataNav3D;
	navDataNav3D.TestId = TestID;
	navDataNav3D.DataStructure = SVO;
	navDataNav3D.WorldType = ESceneType::MAZE;
	navDataNav3D.SceneComplexity = Complexity;
	navDataNav3D.VoxelSize = VoxelSize;
	navDataNav3D.GenerationTime = time;
	navDataNav3D.OpenSpaceCovered = Nav3DVolume->GetOpenVolume() / TotalFreeSpace * 100.f;
	navDataNav3D.IncorrectArea = 0;
	navDataNav3D.NumberOfConnections = Nav3DVolume->GetConnections();
	navDataNav3D.NumberOfNodes = Nav3DVolume->GetNumNodes();
	navDataNav3D.MemoryUsage = Nav3DVolume->NumBytes;

	Collector.SupplyData(navDataNav3D);

	//continue;
	//Pathfinding

	FVector StartLocation = GeneratedMaze->GetRandomLocation(randomStream);
	FVector GoalLocation = GeneratedMaze->GetRandomLocation(randomStream);

	struct PathSettings
	{
		FVector Startlocation;
		FVector GoalLocation;
	};
	TArray<PathSettings> PathGenerationSettings;
	PathGenerationSettings.Reserve(PathsPerTest);
	for (size_t i = 0; i < PathsPerTest; i++)
	{
		PathGenerationSettings.Add({ GeneratedMaze->GetRandomLocation(randomStream) ,GeneratedMaze->GetRandomLocation(randomStream) });
	}


	Nav3DComponent->FindVolume();
	ANav3DVolume* vol = const_cast<ANav3DVolume*>(Nav3DComponent->GetCurrentVolume());

	FNav3DOctreeEdge StartEdge;
	FNav3DOctreeEdge TargetEdge;

	vol->GetEdge(StartLocation, StartEdge);
	vol->GetEdge(GoalLocation, TargetEdge);

	FNav3DPathFindingConfig Config;
	Config.Heuristic = ENav3DHeuristic::Euclidean;
	Config.EstimateWeight = 5.f;
	Config.NodeSizePreference = 1.0f;
	Config.PathPruning = ENav3DPathPruning::WithClearance;
	Config.PathSmoothing = 5;

	FNav3DPath OutPath;
	//Nav3DComponent->ExecutePathFinding(StartEdge, TargetEdge, StartLocation, GoalLocation, Config, OutPath);
	for (int32 i = 0; i < OutPath.Points.Num() - 1; i++)
	{
		//DrawDebugLine(GetWorld(), OutPath.Points[i].Location, OutPath.Points[i + 1].Location, FColor::Green, true, -1, 0u, 15.f);
	}


	for (auto& settings : PathGenerationSettings)
	{
		TArray<FVector> Path;
		auto patht1 = std::chrono::steady_clock::now();
		//IcarusVolume->FindPath(settings.Startlocation, settings.GoalLocation, Path);
		auto patht2 = std::chrono::steady_clock::now();
		auto pathDuration = std::chrono::duration<float>(patht2 - patht1);

		float PathLenght = 0;
		if (Path.Num() > 0) {
			for (size_t j = 0; j < Path.Num() - 1; j++)
			{
				PathLenght += FVector::Dist(Path[j], Path[j + 1]) / 100.f;
				//DrawDebugLine(GetWorld(), Path[j], Path[j + 1], FColor::Red, true,-1,0u,15.f);
				//DrawDebugLine(GetWorld(), Path[j], Path[j + 1], FColor::Red, true,-1,0u,15.f);

			}
		}
		PathData pathData;
		pathData.PathID = PathID;
		pathData.GenerationTime = std::chrono::duration_cast<std::chrono::milliseconds>(pathDuration).count();
		pathData.Distance = PathLenght;

		Collector.SupplyPathData(pathData);
	}
	PathID++;

	


	TestID++;

	if (TestID >= AmountOfTests)
	{
		Collector.Save("TestData",true,true);
		IsTesting = false;
	}
	UE_LOG(LogTemp, Warning, TEXT("Test %d Done"), TestID);
	//UE_LOG(LogTemp, Warning, TEXT("%f"), perf.GenerationTime);
}

