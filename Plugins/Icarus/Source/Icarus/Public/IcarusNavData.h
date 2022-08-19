#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IcarusNavData.generated.h"


struct IcarusNode
{
	size_t ID;
	TArray<size_t> connections;
};

UCLASS()
class ICARUS_API AIcarusNavData : public AActor
{
	GENERATED_BODY()
public:


	void SetData(const TArray<FVector>& InVertices, const TArray<size_t>& InIndices, const TArray<IcarusNode>& InNodes) {};
	const TArray<FVector>& GetVertices() const { return Vertices; };
	const TArray<size_t>& GetIndices() const { return Indices; };
	const TArray<IcarusNode>& GetNodes() const{ return Nodes; };
private:
	TArray<FVector> Vertices;
	TArray<size_t> Indices;
	TArray<IcarusNode> Nodes;

};