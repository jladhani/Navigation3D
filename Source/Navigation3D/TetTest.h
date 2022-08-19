// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TetTest.generated.h"

UCLASS()
class NAVIGATION3D_API ATetTest : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATetTest();

	UPROPERTY(EditAnywhere)
	int Debug = 0;
	UPROPERTY(EditAnywhere)
	FVector BoxLocation = FVector(0,0,0);

	UPROPERTY(EditAnywhere)
		FVector BoxMin = FVector(-100);
	UPROPERTY(EditAnywhere)
		FVector BoxMax = FVector(100);

	UPROPERTY(EditAnywhere)
		FVector TetA = FVector(-100,0,0);

	UPROPERTY(EditAnywhere)
		FVector TetB = FVector(-100, 100, 0);

	UPROPERTY(EditAnywhere)
		FVector TetC = FVector(100, 100, 0);

	UPROPERTY(EditAnywhere)
		FVector TetD = FVector(0, 0, 100);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
