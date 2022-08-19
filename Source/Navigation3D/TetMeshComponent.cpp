// Fill out your copyright notice in the Description page of Project Settings.


#include "TetMeshComponent.h"
#include "TetGeneratorTest.h"

void UTetMeshComponent::SetRenderData(const TArray<FVector>& InVertices, const TArray<size_t>& InIndices)
{
	Vertices = InVertices;
	Indices = InIndices;
	MarkRenderStateDirty();
	
}

void UTetMeshComponent::GatherRenderData(TArray<FVector>& OutVertices, TArray<size_t> OutIndices)
{
	OutVertices = Vertices;
	OutIndices = Indices;
}

void UTetMeshComponent::GetSliceData(FVector& Normal, float& T) const
{
	FScopeLock ScopeLock(&SliceCritialSection);
	Normal = SliceNormal;
	T = SliceT;
}
void UTetMeshComponent::SetSliceData(const FVector& Normal, float T)
{
	FScopeLock ScopeLock(&SliceCritialSection);
	SliceNormal = Normal;
	SliceT = T;
	
}

FPrimitiveSceneProxy* UTetMeshComponent::CreateSceneProxy()
{
	class FTetVisProxy final : public FPrimitiveSceneProxy
	{
	public:
		FTetVisProxy(const UTetMeshComponent* inComponent)
			: FPrimitiveSceneProxy(inComponent)
		{
			
			
		}


		SIZE_T GetTypeHash() const override
		{
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}
		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			UMaterial* mat = UMaterial::GetDefaultMaterial(MD_Surface);
			FMaterialRenderProxy* MaterialProxy =  mat->GetRenderProxy();

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					FMeshBatch& Mesh = Collector.AllocateMesh();
					FMeshBatchElement& BatchElement = Mesh.Elements[0];

					//BatchElement.ind
				}
			}

		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View);
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}

		virtual uint32 GetMemoryFootprint(void) const override { return sizeof * this + GetAllocatedSize(); }
		uint32 GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }
	private:
		const UTetMeshComponent* TetVisComponent;

		FBoxCenterAndExtent Bounds;
		TArray<FVector> Vertices;
		TArray<size_t> Indices;
	};


	return new FTetVisProxy(this);
}

FBoxSphereBounds UTetMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds();
}

