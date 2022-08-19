// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IcarusVolume.h"
#include "PathFinder/PathFinder.h"
#include "PathFinder/AStar.h"

#include "IcarusVolumeCustom.generated.h"

class OcclusionInterface;

struct OctreeNode
{
	int32 Index;
	FBoxCenterAndExtent BoxCenterAndExtent;
};
struct FOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static FBoxCenterAndExtent GetBoundingBox(const OctreeNode& Element)
	{
		return Element.BoxCenterAndExtent;
	}

	FORCEINLINE static bool AreElementsEqual(const OctreeNode* A, const OctreeNode* B)
	{
		return A->Index == B->Index;
	}
	FORCEINLINE static void SetElementId(const OctreeNode& Element, FOctreeElementId2 Id) {}
};

class TetNode : public AStarNode
{

public:
	float distanceTo(AStarNode* node) const override
	{
		TetNode* other = static_cast<TetNode*>(node);
		return FVector::Dist(Location, other->Location);
	}
	FVector Location;
	size_t Idx;
};

class UProceduralMeshComponent;
class UTetMeshVisualizer;
/**
 * 
 */
UCLASS()
class NAVIGATION3D_API AIcarusVolumeCustom : public AIcarusVolume
{
	GENERATED_BODY()
public:
	AIcarusVolumeCustom();

	virtual void BeginPlay() override;
	void Start();
	virtual void Tick(float DeltaTime);
	virtual bool IsOccluded(FBoxCenterAndExtent WorldBounds);
	virtual void OnBuildComplete(const TArray<FVector>& Vertices, const TArray<size_t>& Indices);

	void SetOcclusionInterface(TSharedPtr<OcclusionInterface> NewMaze) { OcclusionObject = NewMaze; }
	TSharedPtr<OcclusionInterface> GetOcclusionInterface() const { return OcclusionObject; }
	bool FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPath,float AbortTimer, bool& IsAborted);

	virtual IcarusPerfData GatherData() const override;

	float CalculateIncorrectArea() const;
	float CalculateTotalArea() const;

	UFUNCTION(BlueprintCallable)
	void Show();

	UFUNCTION(BlueprintCallable)
		void Hide();

	void Save(const FString& name);
	UFUNCTION(BlueprintCallable)
	void Load(const FString& name);

	void Serialize(FArchive& Ar);
	UPROPERTY(VisibleAnywhere)
		UTetMeshVisualizer* TetMeshVisualizer;
	UPROPERTY(EditAnywhere)
	bool bDrawDebug = false;

	UPROPERTY(VisibleAnywhere)
		UProceduralMeshComponent* Visualizer;

	UPROPERTY(VisibleAnywhere)
		int NumberOfTets = 0;
	UPROPERTY(EditAnywhere)
		int StartNode = 0;
	UPROPERTY(EditAnywhere)
		int EndNode = 100;
	UPROPERTY(VisibleAnywhere)
		int TotalNodes = 100;

	TArray<FVector> TetVertices;
	TArray<size_t> TetIndices;
private:
	void BuildNodeTree();

	TArray<TetNode> Nodes;


	TUniquePtr<TOctree2<OctreeNode, FOctreeSemantics>> NodeOctree;

	TSharedPtr<OcclusionInterface> OcclusionObject;

};
