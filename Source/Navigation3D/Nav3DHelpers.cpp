#include "Nav3DHelpers.h"

namespace Nav3DHelpers
{

	bool TetrahedronPointTest(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4, const FVector& point)
	{
		struct Plane
		{
			FVector Location;
			FVector Normal;
		};
		auto CreatePlaneHelper = [](const FVector& v1, const FVector& v2, const FVector& v3, const FVector& ExtraVert) {
			FVector b1 = (v2 - v1).GetSafeNormal();
			FVector b2 = (v3 - v1).GetSafeNormal();
			FVector Normal = FVector::CrossProduct(b1, b2);
			FVector VertDir = (ExtraVert - v1).GetSafeNormal();
			if (FVector::DotProduct(Normal, VertDir) > 0) Normal = -Normal;
			return Plane{ v1,Normal };
		};

		Plane p1 = CreatePlaneHelper(v1, v2, v3, v4);
		Plane p2 = CreatePlaneHelper(v2, v3, v4, v1);
		Plane p3 = CreatePlaneHelper(v3, v4, v1, v2);
		Plane p4 = CreatePlaneHelper(v4, v1, v2, v3);

		auto IsBehindPlane = [](const Plane& plane, const FVector& point)
		{
			return FVector::DotProduct(point - plane.Location, plane.Normal) < 0;
		};

		return IsBehindPlane(p1, point) && IsBehindPlane(p2, point) && IsBehindPlane(p2, point) && IsBehindPlane(p2, point);
	};

}