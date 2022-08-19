#include "Surfacemesh.h"
#include "Tetrahedron.h"

#include <map>

#include <CGAL/Complex_2_in_triangulation_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/IO/facets_in_complex_2_to_triangle_mesh.h>
#include <CGAL/Aff_transformation_3.h>

#pragma warning( disable : 4103 4456 4459 4458 4541)
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/transform.h>


using namespace Icarus;


using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point3 = Kernel::Point_3;
using Mesh = CGAL::Surface_mesh<Point3>;
using vertex_descriptor = Mesh::Vertex_index;
using face_descriptor = Mesh::Face_index;
typedef Kernel::Vector_3 Vector_3;

// default triangulation for Surface_mesher
typedef CGAL::Surface_mesher::Surface_mesh_default_triangulation_3_generator<Kernel>::Type Tr2;
// c2t3
typedef CGAL::Complex_2_in_triangulation_3<Tr2> C2t3;
typedef Tr2::Geom_traits GT;
typedef GT::Sphere_3 Sphere_3;
typedef GT::FT FT;
typedef FT(*Function3)(Point3);
typedef CGAL::Implicit_surface_3<GT, Function3> Surface_3;

typedef CGAL::Aff_transformation_3<Kernel> Aff_transformation_3;

namespace Icarus {
	class SurfaceMesh_Data
	{
	public:
		Mesh meshData;
	};
};


void SurfaceMesh::Translate(const FVector& V)
{
	Aff_transformation_3 transl(CGAL::TRANSLATION, Vector_3(V.X, V.Y, V.Z));
	CGAL::Polygon_mesh_processing::transform(transl, data->meshData);
}


std::shared_ptr<SurfaceMesh> SurfaceMesh::FromCapsule(float Length, float Radius)
{

	if (Radius == 0) return std::shared_ptr<SurfaceMesh>();

	Tr2 tr;            // 3D-Delaunay triangulation
	C2t3 c2t3(tr);   // 2D-complex in 3D-Delaunay triangulation

	FVector Normal = FVector(1, 0, 0);
	float HalfLength = FMath::Abs(Length)/2.f;
	float RadiusSq = Radius * Radius;
	auto func = [RadiusSq, HalfLength](Point3 p) {
		float t = FMath::Clamp((float)p.x(), -HalfLength, HalfLength);
		float X = p.x() - t;
		float dsq = X* X + p.y() * p.y() + p.z() * p.z();
		FT result = dsq - RadiusSq;
		return result;
	};

	float BoundingDistance = HalfLength + Radius;
	BoundingDistance = BoundingDistance * BoundingDistance;
	// defining the surface
	Surface_3 surface(func,             // pointer to function
		Sphere_3(CGAL::ORIGIN, BoundingDistance)); // bounding sphere
// Note that "2." above is the *squared* radius of the bounding sphere!
// defining meshing criteria
	CGAL::Surface_mesh_default_criteria_3<Tr2> criteria(10.,  // angular bound
		Radius*0.5,  // radius bound
		Radius*0.5); // distance bound
// meshing surface
	CGAL::make_surface_mesh(c2t3, surface, criteria, CGAL::Non_manifold_tag());
	Mesh sm;
	CGAL::facets_in_complex_2_to_triangle_mesh(c2t3, sm);


	std::shared_ptr<SurfaceMesh> result = std::shared_ptr<SurfaceMesh>(new SurfaceMesh());
	result->data->meshData = sm;
	return result;
}






Point3 FVectorToPoint3(const FVector& v)
{
	return Point3(v.X, v.Y, v.Z);
}

FVector Point3ToFVector(const Point3& p)
{
	return FVector(p.x(), p.y(), p.z());
}

SurfaceMesh::SurfaceMesh()
{
	data = new SurfaceMesh_Data();
}

SurfaceMesh::~SurfaceMesh()
{
	delete data;
}

float SurfaceMesh::CalculateVolume() const
{
	return CGAL::Polygon_mesh_processing::volume(data->meshData);
}

void SurfaceMesh::GetVerticesAndIndices(TArray<FVector>& OutVertices, TArray<int32>& OutIndices)
{
	Mesh& mesh = data->meshData;
	std::map<Mesh::vertex_index, int32> Mapper;
	for (auto it = mesh.vertices_begin(); it != mesh.vertices_end(); it++)
	{
		Mesh::vertex_index vi = *it;
		int32 idx = OutVertices.Add(Point3ToFVector(mesh.point(vi)));
		Mapper.insert(std::make_pair(vi, idx));
		idx++;
	}
	int32 SwapStartIdx = OutIndices.Num();
	for (auto it = mesh.faces_begin(); it != mesh.faces_end(); it++)
	{
		Mesh::face_index fi = *it;
		Mesh::Halfedge_index hf = mesh.halfedge(fi);
		for (Mesh::Halfedge_index hi : halfedges_around_face(hf, mesh))
		{
			Mesh::Vertex_index vi = target(hi, mesh);
			OutIndices.Push(Mapper[vi]);
		}
	}
	for (size_t i = SwapStartIdx; i < OutIndices.Num(); i+=3)
	{
		OutIndices.Swap(i, i + 1);
	}
}

std::shared_ptr<SurfaceMesh> SurfaceMesh::Intersect(std::shared_ptr<SurfaceMesh> other)
{
	auto outSurfMesh = Create();

	bool valid_intersect = CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(data->meshData, other->data->meshData,outSurfMesh->data->meshData);
	if (!valid_intersect) return std::shared_ptr<SurfaceMesh>();
	return outSurfMesh;
}
bool SurfaceMesh::Intersect2(std::shared_ptr<SurfaceMesh> other)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(data->meshData, other->data->meshData, data->meshData);
}

std::shared_ptr<SurfaceMesh> SurfaceMesh::Union(std::shared_ptr<SurfaceMesh> other)
{
	auto outSurfMesh = Create();
	bool valid_intersect = CGAL::Polygon_mesh_processing::corefine_and_compute_union(data->meshData, other->data->meshData, outSurfMesh->data->meshData);
	if (!valid_intersect) return std::shared_ptr<SurfaceMesh>();
	return outSurfMesh;
}

bool SurfaceMesh::Union2(std::shared_ptr<SurfaceMesh> other)
{


	return CGAL::Polygon_mesh_processing::corefine_and_compute_union(data->meshData, other->data->meshData, data->meshData);
}

std::shared_ptr<SurfaceMesh> SurfaceMesh::FromBox(const FBox& box)
{
	auto surfMesh = Create();
	Mesh& mesh = surfMesh->data->meshData;

	FVector size = box.Max - box.Min;

	vertex_descriptor a = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(0, 0, 0)));
	vertex_descriptor b = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(1, 0, 0)));
	vertex_descriptor c = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(1, 1, 0)));
	vertex_descriptor d = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(0, 1, 0)));
	vertex_descriptor e = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(0, 0, 1)));
	vertex_descriptor f = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(1, 0, 1)));
	vertex_descriptor g = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(1, 1, 1)));
	vertex_descriptor h = mesh.add_vertex(FVectorToPoint3(box.Min + size * FVector(0, 1, 1)));

	bool IsSucces = true;

	IsSucces == IsSucces && mesh.add_face(a, f, e) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(a, b, f) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(b, g, f) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(b, c, g) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(c, h, g) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(c, d, h) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(d, e, h) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(d, a, e) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(e, g, h) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(e, f, g) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(a, c, b) != Mesh::null_face();
	IsSucces == IsSucces && mesh.add_face(a, d, c) != Mesh::null_face();
	
	return IsSucces ? surfMesh : std::shared_ptr<SurfaceMesh>();
}
/*
std::shared_ptr<SurfaceMesh> SurfaceMesh::FromTetrahedron(const Tetrahedron& tet)
{
	auto surfMesh = Create();
	TArray<vertex_descriptor> VertexDescriptors;
	VertexDescriptors.Reserve(4);

	Mesh& mesh = surfMesh->data->meshData;

	//vertex_descriptor u = mesh.add_vertex(FVectorToPoint3(tet.V1));
	//vertex_descriptor v = mesh.add_vertex(FVectorToPoint3(tet.V2));
	//vertex_descriptor w = mesh.add_vertex(FVectorToPoint3(tet.V3));
	//vertex_descriptor x = mesh.add_vertex(FVectorToPoint3(tet.V4));

	FVector Center = (tet.V1 + tet.V2 + tet.V3 + tet.V4) / 4.f;

	vertex_descriptor verts[4] =
	{
		mesh.add_vertex(FVectorToPoint3(tet.V1)),
		mesh.add_vertex(FVectorToPoint3(tet.V2)),
		mesh.add_vertex(FVectorToPoint3(tet.V3)),
		mesh.add_vertex(FVectorToPoint3(tet.V4))
	};

	struct Face
	{
		int v1;
		int v2;
		int v3;
	};

	Face faces[4] =
	{
		Face{0,1,2},
		Face{2,3,4},
		Face{0,1,3},
		Face{0,2,3},
	};
	
	for (size_t faceId = 0; faceId < 4; faceId++)
	{
		vertex_descriptor v1Desc = verts[faces[faceId].v1];
		vertex_descriptor v2Desc = verts[faces[faceId].v2];
		vertex_descriptor v3Desc = verts[faces[faceId].v3];

		FVector v1 = tet.Vertices[faces[faceId].v1];
		FVector v2 = tet.Vertices[faces[faceId].v2] - v1;
		FVector v3 = tet.Vertices[faces[faceId].v3] - v1;

		FVector localCenter = Center - v1;
		FVector Cross = FVector::CrossProduct(v2, v3);
		face_descriptor fd;
		if (FVector::DotProduct(Cross, localCenter) > 0) fd = mesh.add_face(v1Desc, v2Desc, v3Desc);
		else fd = mesh.add_face(v1Desc, v2Desc, v3Desc);
		if (fd == Mesh::null_face()) return std::shared_ptr<SurfaceMesh>();
	}

	/*
	face_descriptor f1 = mesh.add_face(u, v, w);
	face_descriptor f2 = mesh.add_face(u, x, v);
	face_descriptor f3 = mesh.add_face(u, w, x);
	face_descriptor f4 = mesh.add_face(w, v, x);

	if (f1 == Mesh::null_face() || f2 == Mesh::null_face() || f3 == Mesh::null_face() || f4 == Mesh::null_face())
	{
		return std::shared_ptr<SurfaceMesh>();
	}
	*/
	//return surfMesh;
//}

std::shared_ptr<SurfaceMesh> SurfaceMesh::FromTetrahedron(const Tetrahedron& tet)
{
	auto surfMesh = Create();
	TArray<vertex_descriptor> VertexDescriptors;
	VertexDescriptors.Reserve(4);

	Mesh& mesh = surfMesh->data->meshData;

	vertex_descriptor u = mesh.add_vertex(FVectorToPoint3(tet.V1));
	vertex_descriptor v = mesh.add_vertex(FVectorToPoint3(tet.V2));
	vertex_descriptor w = mesh.add_vertex(FVectorToPoint3(tet.V3));
	vertex_descriptor x = mesh.add_vertex(FVectorToPoint3(tet.V4));


	FVector Cross = FVector::CrossProduct(tet.V2 - tet.V1, tet.V3 - tet.V1);
	bool needsFlip = FVector::DotProduct(Cross, tet.V4 - tet.V1) < 0;
	
	face_descriptor f1;
	face_descriptor f2;
	face_descriptor f3;
	face_descriptor f4;

	if (needsFlip) {
		f1 = mesh.add_face(u, v, w);
		f2 = mesh.add_face(u, x, v);
		f3 = mesh.add_face(u, w, x);
		f4 = mesh.add_face(w, v, x);
	}
	else
	{
		f1 = mesh.add_face(u, w, v);
		f2 = mesh.add_face(u, v, x);
		f3 = mesh.add_face(u, x, w);
		f4 = mesh.add_face(w, x, v);
	}

	if (f1 == Mesh::null_face() || f2 == Mesh::null_face() || f3 == Mesh::null_face() || f4 == Mesh::null_face())
	{
		return std::shared_ptr<SurfaceMesh>();
	}
	
	return surfMesh;
}

std::shared_ptr<SurfaceMesh> SurfaceMesh::FromTriangles(const TArray<FVector>& vertices, const TArray<int32>& indices)
{
	auto surfMesh = Create();
	TArray<vertex_descriptor> VertexDescriptors;
	VertexDescriptors.Reserve(vertices.Num());

	Mesh& mesh = surfMesh->data->meshData;

	for (const FVector& vertex : vertices)
	{
		VertexDescriptors.Add(mesh.add_vertex(FVectorToPoint3(vertex)));
	}

	int32 NumFaces = indices.Num() / 3;
	for (int32 faceId = 0; faceId < NumFaces; faceId++)
	{
		int32 id = faceId * 3;
		face_descriptor faceDescriptor = mesh.add_face(VertexDescriptors[indices[id+1]], VertexDescriptors[indices[id]], VertexDescriptors[indices[id + 2]]);
		if (faceDescriptor == Mesh::null_face())
		{
			return std::shared_ptr<SurfaceMesh>();
		}
	}
	return surfMesh;
}

std::shared_ptr<SurfaceMesh> SurfaceMesh::Create()
{
	return std::shared_ptr<SurfaceMesh>(new SurfaceMesh());
}

