// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TetGeneratorTest.generated.h"

class UTetMeshVisualizer;




UCLASS()
class NAVIGATION3D_API ATetGeneratorTest : public AActor
{
	GENERATED_BODY()
	

	TArray<FVector> Vertices;
	TArray<size_t> Indices;

	TArray<FVector> TriangleVertices;
	TArray<int32> TriangleIndices;


	UPROPERTY(VisibleAnywhere)
	UTetMeshVisualizer* TetMeshVisualizer;

	

public:	
	// Sets default values for this actor's properties
	ATetGeneratorTest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

};
