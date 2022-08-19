// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <chrono>
#include "Types.h"
#include "Tetrahedron.h"
#include "DrawDebugHelpers.h"
#include "ParellelTester.generated.h"


class TestRunnable;
class ContextDispenser;
class DataCollector;


USTRUCT(BlueprintType)
struct FTestSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int TotalTests = 1000;
	UPROPERTY(BlueprintReadWrite)
	int StartIndex = 1;
	UPROPERTY(BlueprintReadWrite)
	int StopIndex = TotalTests;
	UPROPERTY(BlueprintReadWrite)
	int NumberOfThreads = 12;
	UPROPERTY(BlueprintReadWrite)
	TArray<int> VoxelSizes;

	UPROPERTY(BlueprintReadWrite)
	bool bGenerateEnv;
	UPROPERTY(BlueprintReadWrite)
	bool bGenerateTetMesh;
	UPROPERTY(BlueprintReadWrite)
	bool bGenerateSVO;
	UPROPERTY(BlueprintReadWrite)
	bool bReflectTetMesh;
	UPROPERTY(BlueprintReadWrite)
	bool bReflectSVO;
	UPROPERTY(BlueprintReadWrite)
	bool bPathFindTetMesh;
	UPROPERTY(BlueprintReadWrite)
	bool bPathFindSVO;
	UPROPERTY(BlueprintReadWrite)
	TEnumAsByte <ESceneType> SceneType = ESceneType::MAZE;

};

UCLASS()
class NAVIGATION3D_API AParellelTester : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AParellelTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UFUNCTION(BlueprintCallable)
	void StartTesting(FTestSettings Settings);

	UFUNCTION(BlueprintCallable)
		void StopTesting();

	UFUNCTION(BlueprintCallable)
	TArray<FString> GetAllStatus();

	UFUNCTION(BlueprintCallable)
		bool IsRunning() const {return bIsRunning;};

	void SingleTestDone(TestRunnable* runnable);

	void PostTesting();

	UPROPERTY(EditAnywhere, Category = "Settings")
		int AmountOfTests = 1000;

	UPROPERTY(EditAnywhere, Category = "Settings")
		int MaxMazeSize = 100;

	UPROPERTY(EditAnywhere, Category = "Settings")
		FVector BoundsSize = FVector(12800, 12800, 12800);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
		TArray<int> SequenceSettings;
	UFUNCTION(BlueprintCallable)
	void LoadAndCollectData(int Setting, ESceneType SceneSetting);

	UFUNCTION(BlueprintCallable)
		void LoadAndCollectAllData();

	UFUNCTION(BlueprintCallable)
		void TestSomething();

	static const int PathFindingFailTime = 5000;

private:
	std::chrono::steady_clock::time_point StartTime;
	
	FCriticalSection Mutex;
	TArray<TestRunnable*> TestRunnables;
	ContextDispenser* Dispenser;
	DataCollector* Collector;
	bool bIsRunning = false;
	bool bIsDone = false;
	 TSharedPtr<class OcclusionInterface> DebugDrawOI;
	int NumberOfThreads = 10;
};