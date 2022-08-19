#pragma once
#include "Templates/SharedPointer.h"
#include <cinttypes>

class ICARUS_API CompactBinaryVoxelGrid : public TSharedFromThis<CompactBinaryVoxelGrid>
{
public:
	CompactBinaryVoxelGrid(int64_t SizeX, int64_t SizeY, int64_t SizeZ, FVector VoxelSize, bool InitZero = false);
	~CompactBinaryVoxelGrid();
	inline bool GetState(int64_t X, int64_t Y, int64_t Z) { return GetState(X + Y * SizeX + Z * SizeY * SizeX); };
	bool GetState(int64_t Index);
	void SetState(FIntVector Coordinates, bool State) { SetState(Coordinates.X, Coordinates.Y, Coordinates.Z, State); };
	void SetState(int64_t X, int64_t Y, int64_t Z, bool state);


	int64_t GetSizeX()const { return SizeX; }
	int64_t GetSizeY()const { return SizeY; }
	int64_t GetSizeZ()const { return SizeZ; }
	FVector GetVoxelSize() const { return VoxelSize; };
	FBox Bounds;
private:
	uint8_t* data;
	int64_t SizeX;
	int64_t SizeY;
	int64_t SizeZ;
	FVector VoxelSize;
};

class ICARUS_API IcarusNavGenerator : public TSharedFromThis<IcarusNavGenerator>
{
public:
	static void Build(TSharedPtr<CompactBinaryVoxelGrid> VoxelGrid, TArray<FVector>& outVertices, TArray<size_t>& outIndices, float param1);
};