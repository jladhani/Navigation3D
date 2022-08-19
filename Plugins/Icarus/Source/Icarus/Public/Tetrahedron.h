#pragma once
#include "CoreMinimal.h"


struct ICARUS_API Tetrahedron
{
	union
	{
		struct  
		{
			FVector V1;
			FVector V2;
			FVector V3;
			FVector V4;
		};
		FVector Vertices[4];
	};
	Tetrahedron(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4);

	FBox GetBounds() const;

	bool IsTouchingBox(const FBox& box);
	FVector GetFaceNormal(int face) const;

	FVector& operator[](size_t idx) {
		return Vertices[idx];
	}

	const FVector& operator[](size_t idx) const {
		return Vertices[idx];
	}

private:

};