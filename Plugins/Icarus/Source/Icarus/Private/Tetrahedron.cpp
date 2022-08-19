#include "Tetrahedron.h"

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <limits>
#include <DrawDebugHelpers.h>
using Ka = CGAL::Exact_predicates_inexact_constructions_kernel;
typedef CGAL::Surface_mesh<Ka::Point_3> Mesh;
typedef Mesh::Vertex_index vertex_descriptor;
typedef Mesh::Face_index face_descriptor;

namespace PMP = CGAL::Polygon_mesh_processing;

Tetrahedron::Tetrahedron(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4)
	: Vertices()
	, V1(v1)
	, V2 (v2)
	, V3 (v3)
	, V4 (v4)
{

}

FBox Tetrahedron::GetBounds() const
{
	FBox bounds(ForceInit);
	bounds += V1;
	bounds += V2;
	bounds += V3;
	bounds += V4;
	return bounds;
}
bool Tetrahedron::IsTouchingBox(const FBox& box)
{
	constexpr int NumAxis = 25;
	constexpr int NumBoxVertices = 8;
	constexpr int NumTetrahedronVertices = 4;
	constexpr float Maxfloat = std::numeric_limits<float>::max();
	constexpr float Minfloat = std::numeric_limits<float>::lowest();

	FVector* VTest = &V1;

	FVector TetEdges[6]
	{
		V2 - V1,
		V3 - V1,
		V4 - V1,
		V2 - V3,
		V2 - V4,
		V3 - V4
	};

	FVector AllAxis[NumAxis] =
	{
		FVector(1,0,0),
		FVector(0,1,0),
		FVector(0,0,1),
		GetFaceNormal(0),
		GetFaceNormal(1),
		GetFaceNormal(2),
		GetFaceNormal(3),
		FVector::CrossProduct(AllAxis[0], TetEdges[0]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[0], TetEdges[1]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[0], TetEdges[2]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[0], TetEdges[3]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[0], TetEdges[4]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[0], TetEdges[5]).GetSafeNormal(),

		FVector::CrossProduct(AllAxis[1], TetEdges[0]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[1], TetEdges[1]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[1], TetEdges[2]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[1], TetEdges[3]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[1], TetEdges[4]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[1], TetEdges[5]).GetSafeNormal(),

		FVector::CrossProduct(AllAxis[2], TetEdges[0]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[2], TetEdges[1]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[2], TetEdges[2]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[2], TetEdges[3]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[2], TetEdges[4]).GetSafeNormal(),
		FVector::CrossProduct(AllAxis[2], TetEdges[5]).GetSafeNormal(),
	};

	FVector BoxVertices[NumBoxVertices] =
	{
		box.Min,
		FVector(box.Min.X,box.Min.Y,box.Max.Z),
		FVector(box.Min.X,box.Max.Y,box.Min.Z),
		FVector(box.Max.X,box.Min.Y,box.Min.Z),
		box.Max,
		FVector(box.Max.X,box.Max.Y,box.Min.Z),
		FVector(box.Max.X,box.Min.Y,box.Max.Z),
		FVector(box.Min.X,box.Max.Y,box.Max.Z)
	};

	for (size_t i = 0; i < NumAxis; i++)
	{
		float tetProjMin = Maxfloat, boxProjMin = Maxfloat;
		float tetProjMax = Minfloat, boxProjMax = Minfloat;

		const FVector& Axis = AllAxis[i];
		if(Axis == FVector::ZeroVector) continue;

		for (size_t j = 0; j < NumBoxVertices; j++)
		{
			float val = FVector::DotProduct(Axis, BoxVertices[j]);
			if (val < boxProjMin) boxProjMin = val;
			if (val > boxProjMax) boxProjMax = val;
		}
		for (size_t j = 0; j < NumTetrahedronVertices; j++)
		{

			float val = FVector::DotProduct(Axis, Vertices[j]);
			if (val < tetProjMin) tetProjMin = val;
			if (val > tetProjMax) tetProjMax = val;
		}
		bool DoOverlap = FMath::Max(tetProjMin, boxProjMin) <= FMath::Min(tetProjMax, boxProjMax);
		if (!DoOverlap) return false;
	}

	return true;
}

FVector Tetrahedron::GetFaceNormal(int face) const
{
	const FVector& V1 = Vertices[(face + 1) % 4];
	FVector V2 = Vertices[(face + 2) % 4] - V1;
	FVector V3 = Vertices[(face + 3) % 4] - V1;
	FVector FaceVertex = Vertices[face] - V1;
	FVector Normal = FVector::CrossProduct(V2, V3).GetSafeNormal();
	return FVector::DotProduct(Normal, FaceVertex) < 0 ? Normal : -Normal;
}