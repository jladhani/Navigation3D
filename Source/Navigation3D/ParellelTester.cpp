// Fill out your copyright notice in the Description page of Project Settings.


#include "ParellelTester.h"
#include "Hal/Runnable.h"
#include "CompactMaze.h"
#include "AsteroidMap.h"
#include "DataCollector.h"
#include "IcarusVolumeCustom.h"
#include <Nav3DVolume.h>
#include "Nav3DComponent.h"
#include "Path.h"
#include <chrono>
#include <thread>
#include "Path.h"
struct TestContext
{
	//FRandomStream randomStream;
	ESceneType SceneType;

	bool bGenerateEnv;

	bool bGenerateTetMesh;
	bool bGenerateSVO;

	bool bReflectTetMesh;
	bool bReflectSVO;

	bool bPathFindTetMesh;
	bool bPathFindSVO;


	int TestID;
	int MaxMazeSize;
	FBox Bounds;
	float Complexity;
	float VoxelSize;
	UWorld* World;
};

class ContextDispenser
{
public:
	ContextDispenser(const TArray<TestContext>& Input) : Contexts(Input) {};

	bool GetContext(TestContext& Output)
	{
		FScopeLock lock(&Mutex);
		if (Contexts.Num() == 0) return false;
		Output = Contexts.Pop();
		UE_LOG(LogTemp, Verbose, TEXT("%i tasks left"), Contexts.Num());
		return true;
	}

private:
	FCriticalSection Mutex;
	TArray<TestContext> Contexts;
};

class TestRunnable : public FRunnable
{
public:
	// Custom constructor for setting up our thread with its target
	TestRunnable(AParellelTester* Owner, ContextDispenser* taskDispenser, AIcarusVolumeCustom* IcarusVolume, ANav3DVolume* Nav3DVolume, DataCollector* Collector)
		: Dispenser(taskDispenser)
		, IcarusVolume(IcarusVolume)
		, Nav3DVolume(Nav3DVolume)
		, Owner(Owner)
		, Collector(Collector)
	{
	};
	virtual ~TestRunnable() {};

	void Start()
	{
		if(!Thread) Thread = FRunnableThread::Create(this, TEXT("ParallelTester"), 320000);
	}

	// FRunnable functions
	virtual uint32 Run() override;
	virtual void Stop() override {};
	virtual void Exit() override {};
	// FRunnable

	//FOnTestCompleteSignature OnCompleteDelegate;
	const FString& GetStatus()const { return Status; }

	void StopTesting() { bStopTesting = true; };

protected:
	void Reset();
	bool GenerateScene(TestContext& context);

	bool GenerateSVO(TestContext& context);
	bool GenerateTetMesh(TestContext& context);

	bool ReflectSVO(TestContext& Context);
	bool ReflectTetmesh(TestContext& Context);

	bool PathFindSVO(TestContext& Context);
	bool PathFindTetMesh(TestContext& Context);

	TSharedPtr<OcclusionInterface> GetScene(TestContext& Context);
	AIcarusVolumeCustom* GetIcarus(TestContext& Context);
	ANav3DVolume* GetNav3D(TestContext& Context);
	NavData GetNavData(TestContext& Context, StructureType structure);

	FString GetStructurePath(const TestContext& Context, StructureType structure, ESceneType scene);
	FString GetResultsPath(const TestContext& Context, StructureType structure);

	FRunnableThread* Thread = nullptr;
	ContextDispenser* Dispenser;
	bool bLoadedIcarus = false;
	AIcarusVolumeCustom* IcarusVolume;
	bool bLoadedNav3D = false;
	ANav3DVolume* Nav3DVolume;
	AParellelTester* Owner;
	DataCollector* Collector;
	FString Status;
	bool bStopTesting = false;
	TSharedPtr<OcclusionInterface> CachedScene = nullptr;
};

void SaveSVO(ANav3DVolume* Nav3DVolume, const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/SVO/";
	FString FullPath = SaveDirectory + name + ".SVOMeshData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileWriter(*FullPath, 0));
	Nav3DVolume->Serialize2(*Ar.Get());
}

void LoadSVO(ANav3DVolume* Nav3DVolume, const FString& name)
{
	IFileManager* FileManager = &IFileManager::Get();
	FString SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "Data/SVO/";
	FString FullPath = SaveDirectory + name + ".SVOMeshData";
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>(FileManager->CreateFileReader(*FullPath, 0));
	Nav3DVolume->Serialize2(*Ar.Get());
}

uint32 TestRunnable::Run()
{
	TestContext Context;
	while (Dispenser->GetContext(Context) && bStopTesting == false)
	{
		Status = FString::SanitizeFloat(Context.VoxelSize, 0) + "_" + FString::FromInt(Context.TestID);
		if (Context.bGenerateEnv) {
			GenerateScene(Context);
		}

		if (Context.bGenerateTetMesh) {
			GenerateTetMesh(Context);
		}
		if (Context.bGenerateSVO) {
			GenerateSVO(Context);
		}
		
		if (Context.bReflectTetMesh)
		{
			ReflectTetmesh(Context);
		}

		if (Context.bReflectSVO)
		{
			ReflectSVO(Context);
		}
		
		if (Context.bPathFindSVO)
		{
			PathFindSVO(Context);
		}

		if (Context.bPathFindTetMesh)
		{
			PathFindTetMesh(Context);
		}
		
		Reset();
		UE_LOG(LogTemp, Warning, TEXT("Test %d Done"), Context.TestID);
	}
	//IcarusVolume->Destroy();
	//Nav3DVolume->Destroy();
	Owner->SingleTestDone(this);
	return 1;
}

void TestRunnable::Reset()
{
	CachedScene = nullptr;
	//Nav3DVolume->Initialise();
	bLoadedIcarus = false;
	bLoadedNav3D = false;
}

bool TestRunnable::GenerateScene(TestContext& Context)
{
	TSharedPtr<OcclusionInterface> GeneratedStructure;
	if (Context.SceneType == ESceneType::MAZE) {
		FRandomStream randomStream = FRandomStream(Context.TestID);
		int mazeComplexity = FMath::RoundToInt((float)Context.MaxMazeSize * Context.Complexity);
		GeneratedStructure = CompactMaze::GenerateMaze(Context.Bounds, FVector(0, 0, 0), randomStream.RandRange(0, 10000000), mazeComplexity);
		if (GeneratedStructure)GeneratedStructure->Save("Maze" + FString::FromInt(Context.TestID));
	}
	else if (Context.SceneType == ESceneType::ASTEROID) {
		FRandomStream randomStream = FRandomStream(Context.TestID);
		GeneratedStructure = AsteroidMap::GenerateAsteroidMap(Context.Bounds, FVector(0, 0, 0), randomStream.RandRange(0, 10000000), Context.Complexity);
		if (GeneratedStructure)GeneratedStructure->Save("AsteroidMap" + FString::FromInt(Context.TestID));
	}
	
	CachedScene = GeneratedStructure;
	return CachedScene != nullptr;
}

bool TestRunnable::GenerateSVO(TestContext& Context)
{

	TSharedPtr<OcclusionInterface> OclusionInterface = GetScene(Context);
	if (OclusionInterface) {
		Nav3DVolume->IsOccludedDel.BindRaw(OclusionInterface.Get(), &OcclusionInterface::IsOccupied);
		Nav3DVolume->VoxelSize = Context.VoxelSize;
		Nav3DVolume->Initialise();
		Nav3DVolume->UpdateVolume2(Context.World);
		auto t1 = std::chrono::steady_clock::now();

		Nav3DVolume->BuildOctree();
		auto t2 = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration<float>(t2 - t1);
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		Nav3DVolume->CachedGenerationTime = time;

		SaveSVO(Nav3DVolume, GetStructurePath(Context, SVO, Context.SceneType));
		bLoadedNav3D = true;
	}
	return OclusionInterface != nullptr;
}

bool TestRunnable::GenerateTetMesh(TestContext& Context)
{
	TSharedPtr<OcclusionInterface> OclusionInterface = GetScene(Context);
	if (OclusionInterface) {
		IcarusVolume->VoxelSize = Context.VoxelSize;
		IcarusVolume->Param1 = Context.VoxelSize * 0.49f;
		IcarusVolume->SetOcclusionInterface(OclusionInterface);
		IcarusVolume->Rebuild();
		IcarusVolume->Save(GetStructurePath(Context, TETMESH, Context.SceneType));
		bLoadedIcarus = true;
	}
	return OclusionInterface != nullptr;
}

bool TestRunnable::ReflectSVO(TestContext& Context)
{
	TSharedPtr<OcclusionInterface> OclusionInterface = GetScene(Context);
	ANav3DVolume* LoadedNav3DVolume = GetNav3D(Context);
	if (OclusionInterface == nullptr || LoadedNav3DVolume == nullptr) return false;
	LoadedNav3DVolume->IsOccludedDel.BindRaw(OclusionInterface.Get(), &OcclusionInterface::IsOccupied);

	double TotalFreeSpace = OclusionInterface->GetTotalOpenVolume();



	NavData navDataNav3D;
	navDataNav3D.TestId = Context.TestID;
	navDataNav3D.DataStructure = SVO;
	navDataNav3D.WorldType = Context.SceneType;
	navDataNav3D.SceneComplexity = Context.Complexity;
	navDataNav3D.VoxelSize = Context.VoxelSize;
	navDataNav3D.OpenSpaceCovered = LoadedNav3DVolume->GetOpenVolume() / TotalFreeSpace * 100.f;
	navDataNav3D.IncorrectArea = 0;
	navDataNav3D.NumberOfConnections = LoadedNav3DVolume->GetConnections();
	navDataNav3D.NumberOfNodes = LoadedNav3DVolume->GetNumNodes();
	navDataNav3D.MemoryUsage = LoadedNav3DVolume->NumBytes;
	
	navDataNav3D.GenerationTime = LoadedNav3DVolume->CachedGenerationTime;

	navDataNav3D.Save(GetResultsPath(Context, SVO));
	return true;
}

bool TestRunnable::ReflectTetmesh(TestContext& Context)
{
	NavData navDataIcarus;
	navDataIcarus.Load(GetResultsPath(Context, TETMESH));
	AIcarusVolumeCustom* LoadedIcarusVolume = GetIcarus(Context);

	
	TSharedPtr<OcclusionInterface> OclusionInterface = GetScene(Context);

	if (OclusionInterface == nullptr || LoadedIcarusVolume == nullptr) return false;

	LoadedIcarusVolume->SetOcclusionInterface(OclusionInterface);
	IcarusPerfData perf = LoadedIcarusVolume->GatherData();

	navDataIcarus.TestId = Context.TestID;
	navDataIcarus.VoxelSize = Context.VoxelSize;
	navDataIcarus.SceneComplexity = Context.Complexity;
	navDataIcarus.DataStructure = TETMESH;
	navDataIcarus.WorldType = Context.SceneType;
	double TotalArea = LoadedIcarusVolume->CalculateTotalArea();
	double IncorrectArea = LoadedIcarusVolume->CalculateIncorrectArea();
	double TotalFreeSpace = OclusionInterface->GetTotalOpenVolume();
	double IcarusOpenSpace = TotalArea - IncorrectArea;
	navDataIcarus.OpenSpaceCovered = (IcarusOpenSpace / TotalFreeSpace) * 100.f;
	navDataIcarus.IncorrectArea = IncorrectArea / TotalArea * 100.f;
	navDataIcarus.NumberOfNodes = perf.NumberOfNodes;
	navDataIcarus.NumberOfConnections = perf.NumberOfConnections;
	navDataIcarus.GenerationTime = perf.GenerationTime;
	
	int32 NumTetVertices = LoadedIcarusVolume->TetVertices.Num();
	int32 NumTetIndices = LoadedIcarusVolume->TetIndices.Num();
	int32 NumNodes = navDataIcarus.NumberOfNodes;
	int32 NumConnections = navDataIcarus.NumberOfConnections;

	navDataIcarus.MemoryUsage = NumTetVertices * sizeof(FVector) + NumTetIndices * sizeof(int32) + NumNodes * (sizeof(TetNode)-sizeof(FVector)) + NumConnections * sizeof(TetNode*);

	navDataIcarus.Save(GetResultsPath(Context, TETMESH));
	return true;
}
const int NumberOfPathsForTest = 50;
bool TestRunnable::PathFindSVO(TestContext& Context)
{
	FString BaseName = "Path" + FString::FromInt(Context.TestID) + "_";
	bool NeedsGeneration = false;
	for (size_t i = 0; i < NumberOfPathsForTest; i++)
	{
		if (!Path::FileExist(BaseName + FString::FromInt(i), Context.SceneType, SVO, Context.VoxelSize))
		{
			NeedsGeneration = true;
			break;
		}
	}
	if (!NeedsGeneration) return true;

	FRandomStream RandomStream(Context.TestID);
	TSharedPtr<OcclusionInterface> Scene = GetScene(Context);
	ANav3DVolume* Structure = GetNav3D(Context);

	Structure->UpdateVolume2(Context.World);

	for (size_t i = 0; i < NumberOfPathsForTest; i++)
	{

		FVector StartLocation = Scene->GetRandomLocation(RandomStream);
		FVector TargetLocation = Scene->GetRandomLocation(RandomStream);
		FString Name = "Path" + FString::FromInt(Context.TestID) + "_" + FString::FromInt(i);
		if (Path::FileExist(Name, Context.SceneType, SVO, Context.VoxelSize)) continue;


		FNav3DOctreeEdge StartEdge;
		FNav3DOctreeEdge TargetEdge;

		bool ValidPoints = Structure->GetEdge(StartLocation, StartEdge) &&
			Structure->GetEdge(TargetLocation, TargetEdge);

		FNav3DPathFindingConfig Config;
		Config.Heuristic = ENav3DHeuristic::Euclidean;
		Config.EstimateWeight = 5.f;
		Config.NodeSizePreference = 1.0f;
		Config.PathPruning = ENav3DPathPruning::WithClearance;
		Config.PathSmoothing = 5;

		FNav3DPath OutPath;
		bool HasAborted = false;
		auto t1 = std::chrono::steady_clock::now();
		if(ValidPoints) UNav3DComponent::ExecutePathFinding2(Structure, StartEdge, TargetEdge, StartLocation, TargetLocation, Config, OutPath, AParellelTester::PathFindingFailTime, HasAborted);
		auto t2 = std::chrono::steady_clock::now();
		auto pathDuration = std::chrono::duration<float>(t2 - t1);

		Path path;
		OutPath.GetPath(path.Points);
		path.StartLocation = StartLocation;
		path.TargetLocation = TargetLocation;
		path.TestId = Context.TestID;
		path.PathId = i;
		path.VoxelSetting = Context.VoxelSize;
		path.GenerationTime = std::chrono::duration_cast<std::chrono::microseconds>(pathDuration).count();;
		path.Structure = SVO;
		path.Scene = (Context.SceneType == ESceneType::MAZE ? ESceneType::MAZE : ESceneType::ASTEROID);
		if(HasAborted) path.Result = TOOKTOOLONG;
		else if(OutPath.Points.Num() == 0) path.Result = FAILED;
		else path.Result = SUCCES;
		path.Save(Name);
	}

	return true;
}

bool TestRunnable::PathFindTetMesh(TestContext& Context)
{
	FString BaseName = "Path" + FString::FromInt(Context.TestID) + "_";
	bool NeedsGeneration = false;
	for (size_t i = 0; i < NumberOfPathsForTest; i++)
	{
		if (!Path::FileExist(BaseName + FString::FromInt(i), Context.SceneType, TETMESH, Context.VoxelSize))
		{
			NeedsGeneration = true;
			break;
		}
	}
	if (!NeedsGeneration) return true;


	FRandomStream(Context.TestID);
	FRandomStream RandomStream(Context.TestID);
	TSharedPtr<OcclusionInterface> Scene = GetScene(Context);
	AIcarusVolumeCustom* Icarus = GetIcarus(Context);


	for (size_t i = 0; i < NumberOfPathsForTest; i++)
	{
		FVector StartLocation = Scene->GetRandomLocation(RandomStream);
		FVector TargetLocation = Scene->GetRandomLocation(RandomStream);
		FString Name = "Path" + FString::FromInt(Context.TestID) + "_" + FString::FromInt(i);

		Path path;
		if (Path::FileExist(Name, Context.SceneType, TETMESH, Context.VoxelSize)) continue;

		bool HasAborted = false;
		auto patht1 = std::chrono::steady_clock::now();
		bool Succes = Icarus->FindPath(StartLocation, TargetLocation, path.Points, AParellelTester::PathFindingFailTime, HasAborted);
		auto patht2 = std::chrono::steady_clock::now();
		auto pathDuration = std::chrono::duration<float>(patht2 - patht1);

		

		path.TestId = Context.TestID;
		path.PathId = i;
		path.StartLocation = StartLocation;
		path.TargetLocation = TargetLocation;

		path.VoxelSetting = Context.VoxelSize;
		path.GenerationTime = std::chrono::duration_cast<std::chrono::microseconds>(pathDuration).count();;
		path.Structure = TETMESH;
		path.Scene = (Context.SceneType == ESceneType::MAZE ? ESceneType::MAZE : ESceneType::ASTEROID);
		if (HasAborted) path.Result = TOOKTOOLONG;
		else if (!Succes) path.Result = FAILED;
		else path.Result = SUCCES;
		path.Save(Name);
	}

	return true;
}

TSharedPtr<OcclusionInterface> TestRunnable::GetScene(TestContext& Context)
{
	if (CachedScene == nullptr)
	{
		int mazeComplexity = FMath::RoundToInt((float)Context.MaxMazeSize * Context.Complexity);
		if (Context.SceneType == ESceneType::MAZE) {
			FString Path = "Maze";
			Path += FString::FromInt(Context.TestID);
			CachedScene = CompactMaze::LoadMaze(Path);
		}
		else if (Context.SceneType == ESceneType::ASTEROID) {
			FString Path = "AsteroidMap";
			Path += FString::FromInt(Context.TestID);
			CachedScene = AsteroidMap::LoadAsteroidMap(Path);
		}
	}
	return CachedScene;
}

AIcarusVolumeCustom* TestRunnable::GetIcarus(TestContext& Context)
{
	if (!bLoadedIcarus)
	{
		IcarusVolume->Load(GetStructurePath(Context,TETMESH, Context.SceneType));
		bLoadedIcarus = true;
	}
	return IcarusVolume;
}

ANav3DVolume* TestRunnable::GetNav3D(TestContext& Context)
{
	if (!bLoadedNav3D)
	{
		LoadSVO(Nav3DVolume, GetStructurePath(Context,SVO, Context.SceneType));
		bLoadedNav3D = true;
	}
	return Nav3DVolume;
}

NavData TestRunnable::GetNavData(TestContext& Context, StructureType structure)
{
	return NavData();
}

FString TestRunnable::GetStructurePath(const TestContext& Context, StructureType structure, ESceneType scene)
{
	int roundedVoxelSize = FMath::RoundToInt(Context.VoxelSize);
	FString Path = "";
	Path += FString::FromInt(roundedVoxelSize) + "/";
	Path += scene == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";
	Path += structure == SVO ? "SVO" :  "TetMesh";
	Path += FString::FromInt(Context.TestID);
	return Path;
}

FString TestRunnable::GetResultsPath(const TestContext& Context, StructureType structure)
{
	int roundedVoxelSize = FMath::RoundToInt(Context.VoxelSize);
	FString Path = "";
	Path += FString::FromInt(roundedVoxelSize) + "/";
	Path += Context.SceneType == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";
	Path += "Results";
	Path += FString::FromInt(Context.TestID);
	Path += structure == SVO ? "S" : "I";
	return Path;
}

// Sets default values
AParellelTester::AParellelTester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AParellelTester::BeginPlay()
{
	Super::BeginPlay();



/*
	FString PathStr = "AsteroidMap";
	PathStr += FString::FromInt(22);
	auto AsteroidField = AsteroidMap::LoadAsteroidMap(PathStr);
	AIcarusVolumeCustom* IcVolume = GetWorld()->SpawnActor<AIcarusVolumeCustom>();
	Path DrawPath;
	DrawPath.Structure = StructureType::TETMESH;
	DrawPath.Scene = ESceneType::ASTEROID;
	DrawPath.VoxelSetting = 200;
	DrawPath.Load("Path22_13");

	IcVolume->Load("200/AsteroidMap/TetMesh22");
	TArray<FVector> NewPath;
	bool isAborted = false;
	IcVolume->FindPath(DrawPath.StartLocation, DrawPath.TargetLocation, NewPath, 5000, isAborted);

	if (NewPath.Num() > 0) {
		for (size_t i = 0; i < NewPath.Num() - 1; i++)
		{
			DrawDebugLine(GetWorld(), NewPath[i], NewPath[i + 1], FColor::Purple, true);
		}
		DrawDebugPoint(GetWorld(), DrawPath.StartLocation, 20, FColor::Purple, true);
		DrawDebugPoint(GetWorld(), DrawPath.TargetLocation, 20, FColor::Purple, true);

	}

	int test = 0;
	*/
}

// Called every frame
void AParellelTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(DebugDrawOI)DebugDrawOI->DrawDebug(GetWorld());

	if (bIsRunning == true)
	{
		FScopeLock lock(&Mutex);
		if (TestRunnables.Num() == 0)
		{
			PostTesting();
		}
	}

}


void AParellelTester::StartTesting(FTestSettings Settings)
{
	if (bIsRunning == true) return;
	bIsRunning = true;
	StartTime = std::chrono::steady_clock::now();
	NumberOfThreads = Settings.NumberOfThreads;
	SequenceSettings = Settings.VoxelSizes;

	TArray<TestContext> ContextList;
	ContextList.Reserve(SequenceSettings.Num() * Settings.TotalTests);

	FRandomStream MainRandomStream(10);

	for (size_t j = 0; j < SequenceSettings.Num(); j++)
	{
		for (size_t i = Settings.StartIndex; i < Settings.StopIndex +1; i++)
		{

			float Complexity = (float)i / (float)(Settings.TotalTests);
			TestContext NewContext;
			NewContext.bGenerateEnv = Settings.bGenerateEnv;
			NewContext.bGenerateTetMesh = Settings.bGenerateTetMesh;
			NewContext.bGenerateSVO = Settings.bGenerateSVO;
			NewContext.bReflectSVO = Settings.bReflectSVO;
			NewContext.bReflectTetMesh = Settings.bReflectTetMesh;
			NewContext.bPathFindSVO = Settings.bPathFindSVO;
			NewContext.bPathFindTetMesh = Settings.bPathFindTetMesh;
			NewContext.SceneType = Settings.SceneType;
			NewContext.TestID = i;
			NewContext.MaxMazeSize = MaxMazeSize;
			NewContext.Bounds = FBox(-(BoundsSize / 2.f), BoundsSize / 2.f);
			NewContext.Complexity = Complexity;
			NewContext.VoxelSize = SequenceSettings[j];
			NewContext.World = GetWorld();
			ContextList.Add(NewContext);
		}

	}
	Dispenser = new ContextDispenser(ContextList);

	Collector = new DataCollector();
	for (size_t i = 0; i < NumberOfThreads; i++)
	{
		AIcarusVolumeCustom* NewIcarusVolume = GetWorld()->SpawnActor<AIcarusVolumeCustom>();
		NewIcarusVolume->SetActorLocation(FVector(0, 0, 0));

		ANav3DVolume* NewNav3DVolume = GetWorld()->SpawnActor<ANav3DVolume>();
		NewNav3DVolume->SetActorLocation(FVector(0, 0, 0));
		NewNav3DVolume->bEnableCoverMap = false;
		NewNav3DVolume->VolumeSize = 12800;
		NewNav3DVolume->VoxelSize = Settings.VoxelSizes[0];
		NewNav3DVolume->UpdateVolume();


		TestRunnable* runnable = new TestRunnable(this, Dispenser, NewIcarusVolume, NewNav3DVolume, Collector);
		TestRunnables.Add(runnable);
		runnable->Start();
	}
}

void AParellelTester::StopTesting()
{
	FScopeLock lock(&Mutex);
	for (auto Tester : TestRunnables)
	{
		Tester->StopTesting();
	}
}

TArray<FString> AParellelTester::GetAllStatus()
{
	TArray<FString> Output;
	FScopeLock lock(&Mutex);
	for (auto Tester : TestRunnables)
	{
		Output.Add(Tester->GetStatus());
	}
	return Output;
}

void AParellelTester::SingleTestDone(TestRunnable* runnable)
{
	FScopeLock lock(&Mutex);
	TestRunnables.Remove(runnable);
}

void AParellelTester::PostTesting()
{

	Collector->Save("Data", Collector->HasNavData(), Collector->HasPathData());

	auto EndTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration<float>(EndTime - StartTime);
	float time = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	UE_LOG(LogTemp, Warning, TEXT("Testing done in %f s"), time);
	bIsDone = true;
	bIsRunning = false;
}

void AParellelTester::LoadAndCollectData(int Setting, ESceneType SceneSetting)
{
	if(Collector == nullptr)Collector = new DataCollector();
	Collector->Clear();

	FString Folder = FString::FromInt(Setting) + "/";
	Folder += SceneSetting == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";
	for (size_t i = 1; i < AmountOfTests; i++)
	{
		NavData navDataIcarus;
		NavData navDataNav3D;
		FString ResultsPath = Folder + "Results" + FString::FromInt(i);
		bool isValid1 = navDataIcarus.Load(ResultsPath + "I");
		bool isValid2 = navDataNav3D.Load(ResultsPath + "S");
		if(isValid1)Collector->SupplyData(navDataIcarus);
		if(isValid2)Collector->SupplyData(navDataNav3D);
	}

	TArray<Path> Paths;
	Path::GetAllPaths(TETMESH, SceneSetting, Setting, Paths);
	Path::GetAllPaths(SVO, SceneSetting, Setting, Paths);

	for (const Path& path : Paths)
	{
		PathData pData;
		pData.Distance = path.CalculateLength()/100.f;
		pData.GenerationTime = path.GenerationTime;
		pData.PathID = path.PathId;
		pData.TestID = path.TestId;
		pData.Result = path.Result;
		pData.DataStructure = path.Structure;
		pData.WorldType = path.Scene;
		Collector->SupplyPathData(pData);
	}

	Collector->Save("Data", Collector->HasNavData(), Collector->HasPathData());
}

void AParellelTester::LoadAndCollectAllData()
{
	if (Collector == nullptr)Collector = new DataCollector();
	Collector->Clear();

	int TestIDOffset = 0;
	for (size_t j = 0; j < 2; j++)
	{
		ESceneType SceneSetting = j == 0 ? ESceneType::MAZE : ESceneType::ASTEROID;

		for (int Setting : SequenceSettings)
		{
			FString Folder = FString::FromInt(Setting) + "/";
			Folder += SceneSetting == ESceneType::MAZE ? "Maze/" : "AsteroidMap/";
			for (size_t i = 1; i < AmountOfTests; i++)
			{
				NavData navDataIcarus;
				NavData navDataNav3D;
				FString ResultsPath = Folder + "Results" + FString::FromInt(i);
				bool isValid1 = navDataIcarus.Load(ResultsPath + "I");
				bool isValid2 = navDataNav3D.Load(ResultsPath + "S");
				navDataIcarus.TestId += TestIDOffset;
				navDataNav3D.TestId += TestIDOffset;

				navDataIcarus.WorldType = SceneSetting;
				navDataIcarus.DataStructure = TETMESH;
				navDataNav3D.WorldType = SceneSetting;
				navDataNav3D.DataStructure = SVO;


				if (isValid1)Collector->SupplyData(navDataIcarus);
				if (isValid2)Collector->SupplyData(navDataNav3D);
			}

			TArray<Path> Paths;
			Path::GetAllPaths(TETMESH, SceneSetting, Setting, Paths);
			Path::GetAllPaths(SVO, SceneSetting, Setting, Paths);

			for (const Path& path : Paths)
			{
				PathData pData;
				pData.Distance = path.CalculateLength() / 100.f;
				pData.GenerationTime = path.Structure == StructureType::SVO ? path.GenerationTime/1000.f : path.GenerationTime;
				pData.DataStructure = path.Structure;
				pData.VoxelSize = Setting;
				pData.WorldType = path.Scene;
				pData.PathID = path.PathId;
				pData.TestID = path.TestId + TestIDOffset;
				pData.Result = path.Result;
				Collector->SupplyPathData(pData);
			}
			TestIDOffset += AmountOfTests;
		}
	}
	Collector->Save("Data", Collector->HasNavData(), Collector->HasPathData());
}

void AParellelTester::TestSomething()
{

	TArray<Path> TetPaths;
	TArray<Path> SVOPaths;
	int VoxelSize = 50;
	float Cuttoff = 6400 - VoxelSize/2.f;

	Path::GetAllPaths(TETMESH, ESceneType::MAZE, VoxelSize, TetPaths);
	Path::GetAllPaths(SVO, ESceneType::MAZE, VoxelSize, SVOPaths);

	if (TetPaths.Num() != SVOPaths.Num()) return;
	int TotalFailures = 0;
	int TotalEdgeLocations = 0;
	for (int i = 0; i < TetPaths.Num(); i++)
	{
		if (TetPaths[i].Result == PathResult::FAILED && SVOPaths[i].Result != PathResult::FAILED)
		{
			TotalFailures++;
			FVector Locations[2] = {
				TetPaths[i].StartLocation.GetAbs(),
				TetPaths[i].TargetLocation.GetAbs()
			};
			for (const FVector& Location : Locations)
			{
				if (Location.X > Cuttoff || Location.Y > Cuttoff || Location.Z > Cuttoff)
				{
					TotalEdgeLocations++;
					break;
				}
			}
		}
	}

	float PercentageEdges = static_cast<float>(TotalEdgeLocations) / static_cast<float>(TotalFailures);
	UE_LOG(LogTemp, Warning, TEXT("%f"), PercentageEdges*100.f);

}
