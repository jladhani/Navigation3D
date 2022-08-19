#include <IcarusNavGenerator.h>
#include "HAL/UnrealMemory.h"


#undef check
#pragma warning( disable : 4103 4456 4459 4458 4541)
PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Mesh_triangulation_3.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>
#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/make_mesh_3.h>

#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/make_mesh_3.h>
#include <CGAL/Image_3.h>


#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/Surface_mesh_default_criteria_3.h>
#include <CGAL/Complex_2_in_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>

#include <CGAL/intersections.h>

CompactBinaryVoxelGrid::CompactBinaryVoxelGrid(int64_t SizeX, int64_t SizeY, int64_t SizeZ, FVector VoxelSize,bool InitZero)
	: SizeX(SizeX)
	, SizeY(SizeY)
	, SizeZ(SizeZ)
	, VoxelSize(VoxelSize)
{
	int64_t totalSize = SizeX*SizeY*SizeZ/8 +1;
	if(InitZero)data = (uint8_t*)FMemory::MallocZeroed(totalSize);
	else data = (uint8_t*)FMemory::Malloc(totalSize);
}

CompactBinaryVoxelGrid::~CompactBinaryVoxelGrid()
{
	FMemory::Free(data);
}


bool CompactBinaryVoxelGrid::GetState(int64_t Index)
{
	int64_t byteOffset = Index / 8;
	int64_t bitOffset = Index % 8;

	return (data[byteOffset] & (1 << bitOffset)) != 0;
}

void CompactBinaryVoxelGrid::SetState(int64_t X, int64_t Y, int64_t Z, bool state)
{
	int64_t idx = X + Y * SizeX + Z * SizeY * SizeX;
	int64_t byteOffset = idx / 8;
	//int64_t bitOffset = (idx - (byteOffset * 8)) % 8; //Check if optimalization performance works
	int64_t bitOffset = idx % 8; 

	if (state) data[byteOffset] |= 1 << bitOffset;
	else data[byteOffset] &= ~(1 << bitOffset);
}



// Domain
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::FT FT;
typedef K::Point_3 Point;
typedef FT(Function)(const Point&);
typedef CGAL::Labeled_mesh_domain_3<K> Mesh_domain;

#ifdef CGAL_CONCURRENT_MESH_3
typedef CGAL::Parallel_tag Concurrency_tag;
#else
typedef CGAL::Sequential_tag Concurrency_tag;
#endif
// Triangulation
typedef CGAL::Mesh_triangulation_3<Mesh_domain, CGAL::Default, Concurrency_tag>::type Tr;
typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr> C3t3;
typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr>::Vertex_handle Vertex_handle;
// Criteria
typedef CGAL::Mesh_criteria_3<Tr> Mesh_criteria;

//typedef Mesh::Face_index face_descriptor;
// To avoid verbose function and named parameters call
using namespace CGAL::parameters;




CGAL::Image_3 VoxelGridToImage(TSharedPtr<CompactBinaryVoxelGrid> VoxelGrid)
{
	FVector VoxelSize = VoxelGrid->GetVoxelSize();
	int64_t GridSizeX = VoxelGrid->GetSizeX();
	int64_t GridSizeY = VoxelGrid->GetSizeY();
	int64_t GridSizeZ = VoxelGrid->GetSizeZ();

	_image* image = _createImage(GridSizeX, GridSizeY, GridSizeZ, 1,
		VoxelSize.X, VoxelSize.Y, VoxelSize.Z, 1,
		WK_FIXED, SGN_UNSIGNED);


	for (int64_t z = 0; z < GridSizeZ; z++)
	{
		for (int64_t y = 0; y < GridSizeY; y++)
		{
			for (int64_t x = 0; x < GridSizeX; x++)
			{
				CGAL::IMAGEIO::static_evaluate<unsigned char>(image, x, y, z) = VoxelGrid->GetState(x, y, z) ? 1 : 0;
			}
		}
	}
	int8* imageData = (int8*)image->data;
	return CGAL::Image_3(image);
}



void IcarusNavGenerator::Build(TSharedPtr<CompactBinaryVoxelGrid> VoxelGrid, TArray<FVector>& outVertices, TArray<size_t>& outIndices, float param1)
{
	outVertices.Empty();
	outIndices.Empty();

	CGAL::Image_3 image = VoxelGridToImage(VoxelGrid);

	Mesh_domain domain = Mesh_domain::create_labeled_image_mesh_domain(image);
	Mesh_criteria criteria(facet_distance = param1);

	C3t3 c3t3 = CGAL::make_mesh_3<C3t3>(domain, criteria, CGAL::parameters::features(domain),CGAL::parameters::no_lloyd(), CGAL::parameters::no_odt(), CGAL::parameters::no_perturb(), CGAL::parameters::no_exude());

	
	Tr& tr = c3t3.triangulation();

	std::map<Tr::Vertex_handle, size_t> Mapper;
	outVertices.Reserve(tr.number_of_vertices());

	size_t i = 0;
	for (auto vit = tr.vertices_begin(), end = tr.vertices_end(); vit != end; ++vit)
	{
		auto b = (*vit);
		Tr::Point p = vit->point();
		FVector newPoint(p.x(), p.y(), p.z());
		outVertices.Add(newPoint);
		Mapper.insert(std::pair<Tr::Vertex_handle, size_t>(vit, i++));
	}

	TArray<FVector> points;
	points.Reserve(c3t3.number_of_vertices_in_complex());
	for (C3t3::Vertices_in_complex_iterator vit = c3t3.vertices_in_complex_begin(), end = c3t3.vertices_in_complex_end(); vit != end; ++vit)
	{
		Tr::Point p = vit->point();
		FVector newPoint(p.x(), p.y(), p.z());
		points.Add(newPoint);
	}
	outIndices.Reserve(c3t3.number_of_cells() * 4);

	for (C3t3::Cells_in_complex_iterator cit = c3t3.cells_in_complex_begin(), end = c3t3.cells_in_complex_end(); cit != end; ++cit)
	{
		Vertex_handle v1 = cit->vertex(0);
		Vertex_handle v2 = cit->vertex(1);
		Vertex_handle v3 = cit->vertex(2);
		Vertex_handle v4 = cit->vertex(3);
		outIndices.Add(Mapper[v1]);
		outIndices.Add(Mapper[v2]);
		outIndices.Add(Mapper[v3]);
		outIndices.Add(Mapper[v4]);
	}

	FBox b(ForceInitToZero);
	for (FVector& v : outVertices) b += v;

	return;
}
