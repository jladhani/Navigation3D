// Fill out your copyright notice in the Description page of Project Settings.


#include "TetTest.h"
#include <DrawDebugHelpers.h>
#include "Tetrahedron.h"
// Sets default values
ATetTest::ATetTest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATetTest::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATetTest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	



	FVector globalMinBox = BoxMin + BoxLocation;
	FVector globalMaxBox = BoxMax + BoxLocation;

	FBox Box = FBox(globalMinBox, globalMaxBox);
	Tetrahedron tet(TetA, TetB, TetC, TetD);

	FColor Color = tet.IsTouchingBox(Box) ? FColor::Green : FColor::Red;



	FBoxCenterAndExtent bx = FBoxCenterAndExtent(Box);
	DrawDebugBox(GetWorld(), bx.Center, bx.Extent, Color);
	DrawDebugLine(GetWorld(), tet.V1, tet.V2, Color);
	DrawDebugLine(GetWorld(), tet.V1, tet.V3, Color);
	DrawDebugLine(GetWorld(), tet.V1, tet.V4, Color);
	DrawDebugLine(GetWorld(), tet.V2, tet.V3, Color);
	DrawDebugLine(GetWorld(), tet.V3, tet.V4, Color);
	DrawDebugLine(GetWorld(), tet.V4, tet.V2, Color);
}

