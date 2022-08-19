// Fill out your copyright notice in the Description page of Project Settings.


#include "TetGeneratorTest.h"
#include "IcarusTest.h"
#include  "TetMeshVisualizer.h"
#include <DrawDebugHelpers.h>



// Sets default values
ATetGeneratorTest::ATetGeneratorTest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	TetMeshVisualizer = CreateDefaultSubobject<UTetMeshVisualizer>(TEXT("TetMeshVisualizer"));

	RootComponent = TetMeshVisualizer;
}

// Called when the game starts or when spawned
void ATetGeneratorTest::BeginPlay()
{
	Super::BeginPlay();


	SphereGenerator::Generate(Vertices, Indices);
	TArray<int32> IndicesInt32;
	for (size_t& indice : Indices)
	{
		IndicesInt32.Add(static_cast<int32>(indice));
	}


	TetMeshVisualizer->SetTetMesh(Vertices, IndicesInt32);


}

// Called every frame
void ATetGeneratorTest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
}



