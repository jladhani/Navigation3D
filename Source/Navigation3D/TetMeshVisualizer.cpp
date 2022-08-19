// Fill out your copyright notice in the Description page of Project Settings.


#include "TetMeshVisualizer.h"
#include <DrawDebugHelpers.h>


UTetMeshVisualizer::UTetMeshVisualizer(const FObjectInitializer& ObjectInitializer)
	: UProceduralMeshComponent(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTetMeshVisualizer::BeginPlay()
{
	Super::BeginPlay();

}

void UTetMeshVisualizer::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (false) {
		for (size_t i = 0; i < CuttTriIndices.Num(); i += 3)
		{
			FVector v1 = GetComponentTransform().TransformPosition(Vertices[CuttTriIndices[i]]);
			FVector v2 = GetComponentTransform().TransformPosition(Vertices[CuttTriIndices[i + 1]]);
			FVector v3 = GetComponentTransform().TransformPosition(Vertices[CuttTriIndices[i + 2]]);


			const float lineFatness = true ? 5.f : 1.f;
			DrawDebugLine(GetWorld(), v1, v2, FColor::Red, false, -1, 0U, lineFatness);
			DrawDebugLine(GetWorld(), v1, v3, FColor::Red, false, -1, 0U, lineFatness);
			DrawDebugLine(GetWorld(), v1, v3, FColor::Red, false, -1, 0U, lineFatness);
		}
	}
}


void UTetMeshVisualizer::SetTetMesh(const TArray<FVector>& NewVertices, const TArray<int32> NewIndices)
{
	Vertices = NewVertices;
	Indices = NewIndices;
	
	Bounds = FBox(EForceInit::ForceInitToZero);
	for (FVector& vertex : Vertices)Bounds += vertex;

	UpdateCutOff();
}

void TetToTri(const TArray<int32>& TetIndices, TArray<int32>& TriIndices)
{
	check(TetIndices.Num() % 4 == 0);

	struct Tri
	{
		size_t a;
		size_t b;
		size_t c;
		bool operator==(const Tri& other)
		{
			return a == other.a && b == other.b && c == other.c;
		}

		bool Compare(Tri& other)
		{
			return
				(a == other.a || a == other.b || a == other.c) &&
				(b == other.a || b == other.b || b == other.c) &&
				(c == other.a || c == other.b || c == other.c);
		}
	};
	TMultiMap<size_t, Tri> TriVerts;
	//convert tets to tri
	for (size_t i = 0; i < TetIndices.Num(); i += 4)
	{
		size_t a = TetIndices[i];
		size_t b = TetIndices[i + 1];
		size_t c = TetIndices[i + 2];
		size_t d = TetIndices[i + 3];

		TriVerts.Add({ a * b * c }, { a,b,c });
		TriVerts.Add({ b * c * d }, { b,c,d });
		TriVerts.Add({ a * b * d }, { a,b,d });
		TriVerts.Add({ a * c * d }, { a,c,d });
	}

	//Remove duplicate triangles
	TArray<size_t> Keys;
	TriVerts.GetKeys(Keys);
	for (size_t key : Keys)
	{
		TArray<Tri> values;
		TriVerts.MultiFind(key, values);
		for (size_t i = 0; i < values.Num(); i++)
		{
			Tri& tri = values[i];
			for (size_t j = 0; j < values.Num(); j++)
			{
				if (i == j) continue;
				Tri TestTri = values[j];
				if (tri.Compare(TestTri))
				{
					values.RemoveAt(j);
					TriVerts.RemoveSingle(key, TestTri);
					break;
				}
			}
		}
	}

	//Fill triangle buffer
	TriIndices.Empty();
	for (auto& pair : TriVerts)
	{
		TriIndices.Add(static_cast<int32>(pair.Value.a));
		TriIndices.Add(static_cast<int32>(pair.Value.b));
		TriIndices.Add(static_cast<int32>(pair.Value.c));
	}
}

FVector TetCentre(FVector a, FVector b, FVector c, FVector d)
{
	return (a + b + c + d) / 4.f;
}

void DuplicateTriFaceSides(TArray<int32>& Indices) 
{
	check(Indices.Num() % 3 == 0);
	size_t NumberOfIndices = Indices.Num();

	for (size_t i = 0; i < NumberOfIndices; i+=3)
	{
		int32 a = Indices[i];
		int32 b = Indices[i+1];
		int32 c = Indices[i+2];
		Indices.Add(a);
		Indices.Add(c);
		Indices.Add(b);
	}
}

void UTetMeshVisualizer::UpdateCutOff()
{
	TArray <int32> TetIndicesCutt;
	TetIndicesCutt.Reserve(Indices.Num());

	for (size_t i = 0; i < Indices.Num(); i+=4)
	{
		FVector centre = TetCentre(Vertices[Indices[i]], Vertices[Indices[i + 1]], Vertices[Indices[i + 2]], Vertices[Indices[i + 3]]);
		if (!IsCuttOff(centre))
		{
			for (size_t j = 0; j < 4; j++) TetIndicesCutt.Add(Indices[i + j]);
		}
	}
	TArray<int32> TriIndicesCutt;
	TetToTri(TetIndicesCutt, TriIndicesCutt);
	CuttTriIndices = TriIndicesCutt;

	DuplicateTriFaceSides(TriIndicesCutt);
	CreateMeshSection(0, Vertices, TriIndicesCutt, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	SetMaterial(0, Mat);

}

bool UTetMeshVisualizer::IsCuttOff(const FVector& location) const
{
	FVector Center = Bounds.GetCenter();
	FVector ex = Bounds.GetExtent();
	float Height = Bounds.Max.Z - Bounds.Min.Z;

	float offset = Center.Z - Height/2.f;

	float a = (location.Z - offset)/Height;

	return a > CuttPlaneAlpha;
}

void UTetMeshVisualizer::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTetMeshVisualizer, CuttPlaneAlpha))
	{
		UpdateCutOff();
	}
	SetVisibility(bShowMesh);
}
