// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "TetMeshVisualizer.generated.h"

UENUM(Blueprintable)
enum CuttDirection
{
	PositiveX UMETA(DisplayName = "+X"),
	PositiveY UMETA(DisplayName = "+Y"),
	PositiveZ UMETA(DisplayName = "+Z"),
	NegativeX UMETA(DisplayName = "-X"),
	NegativeY UMETA(DisplayName = "-Y"),
	NegativeZ UMETA(DisplayName = "-Z")
};

/**
 * 
 */
UCLASS()
class NAVIGATION3D_API UTetMeshVisualizer : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	UTetMeshVisualizer(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay();
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
	
	
	void SetTetMesh(const TArray<FVector>& NewVertices, const TArray<int32> NewIndices);

	UPROPERTY(EditAnywhere, Category = TetMesh, meta = (UIMin = "0.0", UIMax = "1.0"))
		float CuttPlaneAlpha = 1.f;

	UPROPERTY(EditAnywhere, Category = TetMesh)
		bool bShowMesh = true;

	UPROPERTY(EditAnywhere, Category = TetMesh)
		bool bHighlightEdges = true;

	UPROPERTY(EditAnywhere, Category = TetMesh)
		bool bHighlightEdges2 = true;

	UPROPERTY(EditAnywhere, Category = TetMesh)
		TEnumAsByte<CuttDirection> CuttDirection;

	UPROPERTY(EditAnywhere, Category = TetMesh)
		UMaterial* Mat;
protected:

private:
	void UpdateCutOff();
	bool IsCuttOff(const FVector& location) const;
	void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

	TArray<FVector> Vertices;
	TArray<int32> Indices;
	TArray<int32> CuttTriIndices;

	FBox Bounds;
}; 
