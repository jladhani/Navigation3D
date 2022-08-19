#include <IcarusVolume.h>
#include <IcarusNavGenerator.h>
#include <chrono>

DECLARE_CYCLE_STAT(TEXT("Rebuild - ICARUS"), STAT_ICARUS_Rebuild, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Occlude - ICARUS"), STAT_ICARUS_Occlude, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("BuildTets - ICARUS"), STAT_ICARUS_BuildTets, STATGROUP_Game);

using namespace std::chrono;

IcarusPerfData AIcarusVolume::GatherData() const
{
	return PerfData;
}

void AIcarusVolume::Rebuild()
{

	if (VoxelSize == 0)
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_ICARUS_Rebuild);
	auto t1 = std::chrono::steady_clock::now();
	//calculate grid size
	auto Bounds = GetBounds();
	FVector Extent = GetBounds().BoxExtent;
	FVector Size = FVector(12800);
	
	FIntVector VoxelAxisCount;
	VoxelAxisCount.X = FMath::CeilToInt(Size.X / VoxelSize);
	VoxelAxisCount.Y = FMath::CeilToInt(Size.Y / VoxelSize);
	VoxelAxisCount.Z = FMath::CeilToInt(Size.Z / VoxelSize);

	FVector AdjustedSize = FVector(VoxelSize * VoxelAxisCount.X, VoxelSize * VoxelAxisCount.Y, VoxelSize * VoxelAxisCount.Z);
	

	FBox AdjustedBounds = FBoxCenterAndExtent(Bounds.Origin, AdjustedSize / 2.f).GetBox();
	
	bool IsValidData = false;
	bool* TestData = (bool*)FMemory::Malloc(VoxelAxisCount.X * VoxelAxisCount.Y * VoxelAxisCount.Z * sizeof(bool));
	TSharedPtr<CompactBinaryVoxelGrid> VoxelGrid(new CompactBinaryVoxelGrid(VoxelAxisCount.X, VoxelAxisCount.Y, VoxelAxisCount.Z,FVector(VoxelSize)));
	VoxelGrid->Bounds = AdjustedBounds;
	{
		SCOPE_CYCLE_COUNTER(STAT_ICARUS_Occlude);

		for (size_t z = 0; z < VoxelAxisCount.Z; z++)
		{
			for (size_t y = 0; y < VoxelAxisCount.Y; y++)
			{
				for (size_t x = 0; x < VoxelAxisCount.X; x++)
				{
					FVector MinBounds = AdjustedBounds.Min + FVector(x * VoxelSize, y * VoxelSize, z * VoxelSize);
					FVector MaxBounds = MinBounds + FVector(VoxelSize, VoxelSize, VoxelSize);
					FBoxCenterAndExtent b(FBox(MinBounds, MaxBounds));
					bool test = !IsOccluded(b);
					VoxelGrid->SetState(x, y, z, test);
					TestData[z * VoxelAxisCount.X * VoxelAxisCount.Y + y * VoxelAxisCount.X + x] = test;
					
					if (!IsValidData && test)
					{
						IsValidData = true;
					}
				}
			}
		};
	}
	TArray<FVector> Vertices;
	TArray<size_t> Indices;
	if (!IsValidData)
	{
		auto t2 = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration<float>(t2 - t1);
		auto time = duration_cast<milliseconds>(duration).count();
		PerfData.GenerationTime = time;
		OnBuildComplete(Vertices, Indices);
		return;
	}



	{
		SCOPE_CYCLE_COUNTER(STAT_ICARUS_BuildTets);

		IcarusNavGenerator::Build(VoxelGrid, Vertices, Indices, Param1);

		FVector offset = FVector(VoxelSize*0.5f, VoxelSize * 0.5f, VoxelSize * 0.5f);
		for (FVector& v : Vertices) v += offset;

	}
	//Respace Vertices
	for (FVector& Vertex : Vertices)
	{
		//Vertex -= AdjustedSize / 2.f;
		//Vertex /= 2.f;
		Vertex -= AdjustedSize / 2.f;
	}




	auto t2 = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration<float>(t2 - t1);
	auto time = duration_cast<milliseconds>(duration).count();
	PerfData.GenerationTime = time;
	OnBuildComplete(Vertices, Indices);
	
}
