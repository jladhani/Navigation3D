// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "TetMeshComponent.generated.h"



/**
 * 
 */
UCLASS()
class NAVIGATION3D_API UTetMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	

public:
	void SetRenderData(const TArray<FVector>& Vertices, const TArray<size_t>& Indices);
	void GatherRenderData(TArray<FVector>& OutVertices, TArray<size_t> OutIndices);
	void GetSliceData(FVector& Normal, float& T) const;
	void SetSliceData(const FVector& Normal, float T);

protected:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:

	mutable FCriticalSection SliceCritialSection;
	FVector SliceNormal;
	float SliceT;

	TArray<FVector> Vertices;
	TArray<size_t> Indices;
};
