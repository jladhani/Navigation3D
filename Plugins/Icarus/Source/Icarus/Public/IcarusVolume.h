#pragma once


#include "GameFramework/Volume.h"
#include "IcarusVolume.generated.h"

class CompactBinaryVoxelGrid;


struct IcarusPerfData
{
	//Coverage
	float OpenSpaceCovered;
	float IncorrectArea;

	//Complexity
	int NumberOfConnections;
	int NumberOfNodes;

	//Performance
	float GenerationTime;
	int MemoryUsage;
};

/**
 *  Volume contains the octree and methods required for 3D navigation
 */
UCLASS(Blueprintable, meta = (DisplayName = "Icarus Volume"))
class ICARUS_API AIcarusVolume : public AActor
{
	GENERATED_BODY()
	
public:
	AIcarusVolume() {};

	virtual bool IsOccluded(FBoxCenterAndExtent WorldBounds) { return false; };
	virtual void OnBuildComplete(const TArray<FVector>& Vertices, const TArray<size_t>& Indices) {};

	virtual IcarusPerfData GatherData() const;
	void Rebuild();

	FBoxSphereBounds GetBounds() { return FBoxSphereBounds(FBox(FVector(-6400),FVector(6400))); }

	UPROPERTY(EditAnywhere)
	float VoxelSize = 10;

	UPROPERTY(EditAnywhere)
		float Param1 = 50;
	

protected:
	IcarusPerfData PerfData;

private:


};