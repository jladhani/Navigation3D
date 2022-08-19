// Fill out your copyright notice in the Description page of Project Settings.


#include "MyTestActor.h"
#include <Tetrahedron.h>
#include <SurfaceMesh.h>
#include "ProceduralMeshComponent.h"
#include "Maze.h"

// Sets default values
AMyTestActor::AMyTestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Visualizer = CreateDefaultSubobject<UProceduralMeshComponent>("Visualizer");
	Visualizer->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AMyTestActor::BeginPlay()
{
	Super::BeginPlay();

	TSharedPtr<Maze> GeneratedMaze = Maze::GenerateMaze(FBox(FVector(0,0,0),FVector(5000,5000,5000)), FVector(0,0,0), 0, 1);

	FBox b1(FVector(0, 0, 0), FVector(100, 100, 100));
	FBox b2(FVector(110, 0, 0), FVector(200, 100, 100));
	FBox b3(FVector(110, 0, 105), FVector(200, 100, 200));

	Tetrahedron t1(FVector(0, 0, 0), FVector(0, 100, 0), FVector(100, 0, 0), FVector(0, 0, -100));
	Tetrahedron t2(FVector(0, 0, 30), FVector(100, 0, 20), FVector(0, 100, 30), FVector(0, 0, -70));
	Tetrahedron t3(FVector(0, 0, 50), FVector(20, 100, 0), FVector(0, 100, 0), FVector(70, 0, 100));


	auto SurfMesh = Icarus::SurfaceMesh::FromTetrahedron(t1);
	auto SurfMesh2 = Icarus::SurfaceMesh::FromTetrahedron(t2);
	auto SurfMesh3 = Icarus::SurfaceMesh::FromTetrahedron(t3);
	//auto MazeMesh = GeneratedMaze->GetSurfaceMesh();
	
	auto Uni = SurfMesh->Union(SurfMesh2);

	auto Uni2 = Uni->Union(SurfMesh3);


	TArray<FVector> Vertices;
	TArray<int32> Indices;

	SurfMesh->GetVerticesAndIndices(Vertices, Indices);

	Visualizer->CreateMeshSection(0, Vertices, Indices, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);


	float test = 0;

}

// Called every frame
void AMyTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

