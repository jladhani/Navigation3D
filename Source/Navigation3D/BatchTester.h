// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataCollector.h"
#include "GameFramework/Actor.h"
#include "BatchTester.generated.h"

class UProceduralMeshComponent;
class AIcarusVolumeCustom;
class ANav3DVolume;
class UNav3DComponent;
class CompactMaze;


UCLASS()
class NAVIGATION3D_API ABatchTester : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABatchTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere)
		AIcarusVolumeCustom* IcarusVolume;
	UPROPERTY(EditAnywhere)
	ANav3DVolume* Nav3DVolume;
	UPROPERTY(VisibleAnywhere)
		UProceduralMeshComponent* Visualizer;
	UPROPERTY(EditAnywhere)
		UNav3DComponent* Nav3DComponent;
	UPROPERTY(EditAnywhere, Category = "Settings")
		int AmountOfTests = 100;

	UPROPERTY(EditAnywhere, Category = "Settings")
		int MaxMazeSize = 100;

	UPROPERTY(EditAnywhere, Category = "Settings")
		int MaxAsteroidFill = 50;

	UPROPERTY(EditAnywhere, Category = "Settings")
		int PathsPerTest = 10;

	UPROPERTY(EditAnywhere, Category = "Settings")
		FVector BoundsSize = FVector(12800, 12800, 12800);

	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Settings")
		TArray<int> SequenceSettings;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void BatchTest();

	void StartBatchTesting();
	void BatchTestSingleStep();

	void SaveSVO(const FString& name);
	void LoadSVO(const FString& name);

	bool IsTesting = false;
	size_t TestID;

	int VoxelSize;
	FRandomStream randomStream;
	DataCollector Collector;
	int PathID = 0;



	TSharedPtr<CompactMaze> TestMaze;
};
