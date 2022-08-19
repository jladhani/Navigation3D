#pragma once

#include "Math/Vector.h"
#include "Containers/Array.h"
class ICARUS_API SphereGenerator
{
public:
	static void Generate(TArray<FVector>& outVertices, TArray<size_t>& outIndices);
};