#undef check
#pragma warning( disable : 4103 4456 4459 4458 4541)
PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
#include <IcarusTest.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Mesh_triangulation_3.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>
#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/make_mesh_3.h>

#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/make_mesh_3.h>
#include <CGAL/Image_3.h>



#include <Containers/Map.h>
#include <map>

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
// Function
FT sphere_function(const Point& p)
{
	return CGAL::squared_distance(p, Point(CGAL::ORIGIN)) - 1;
}

CGAL::Image_3 randomImage2()
{
	const int dim = 400;
	const unsigned char number_of_spheres = 50;
	const int max_radius_of_spheres = 10;
	const int radius_of_big_sphere = 80;
	_image* image = _createImage(dim, dim, dim, 1,
		1.f, 1.f, 1.f, 1,
		WK_FIXED, SGN_UNSIGNED);

	unsigned char* ptr = static_cast<unsigned char*>(image->data);
	std::fill(ptr, ptr + dim * dim * dim, '\0');

	std::ptrdiff_t center = dim / 2;
	std::size_t i, j, k;
	i = j = k = center;

	std::ptrdiff_t radius = radius_of_big_sphere;


	for (std::ptrdiff_t ii = -radius; ii <= radius; ++ii)
	{
		for (std::ptrdiff_t jj = -radius; jj <= radius; ++jj)
		{
			for (std::ptrdiff_t kk = -radius; kk <= radius; ++kk)
			{
				if (ii * ii + jj * jj + kk * kk > radius * radius) continue;
				using CGAL::IMAGEIO::static_evaluate;
				static_evaluate<unsigned char>(image, i + ii, j + jj, k + kk) = 1;
			}
		}
	}
	return CGAL::Image_3(image);
}

void Generate2(TArray<FVector>& outVertices, TArray<size_t>& outIndices)
{
	outVertices.Empty();
	outIndices.Empty();

	CGAL::Image_3 image = randomImage2();

	Mesh_domain domain = Mesh_domain::create_labeled_image_mesh_domain(image);
	Mesh_criteria criteria(facet_distance = 10);

	C3t3 c3t3 = CGAL::make_mesh_3<C3t3>(domain, criteria);
	Tr& tr = c3t3.triangulation();

	//TArray<FVector> points3;
	std::map<Tr::Vertex_handle, size_t> Mapper;
	//TArray<FVector> Vertices;
	//TArray<size_t> Indices;
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
	//std::map<Vertex_handle> V;
	for (C3t3::Vertices_in_complex_iterator vit = c3t3.vertices_in_complex_begin(), end = c3t3.vertices_in_complex_end(); vit != end; ++vit)
	{
		Tr::Point p = vit->point();
		FVector newPoint(p.x(), p.y(), p.z());
		points.Add(newPoint);
	}
	outIndices.Reserve(c3t3.number_of_cells() * 4);

	for (C3t3::Cells_in_complex_iterator cit = c3t3.cells_in_complex_begin(), end = c3t3.cells_in_complex_end(); cit != end; ++cit)
	{
		//cit->vertex(0)->info();
		Vertex_handle v1 = cit->vertex(0);
		Vertex_handle v2 = cit->vertex(1);
		Vertex_handle v3 = cit->vertex(2);
		Vertex_handle v4 = cit->vertex(3);
		outIndices.Add(Mapper[v1]);
		outIndices.Add(Mapper[v2]);
		outIndices.Add(Mapper[v3]);
		outIndices.Add(Mapper[v4]);






	}


	// Output
	//std::ofstream medit_file("out.mesh");
	//c3t3.output_to_medit(medit_file);
	return;
}

void SphereGenerator::Generate(TArray<FVector>& outVertices, TArray<size_t>& outIndices)
{
	Generate2(outVertices, outIndices);
	return;
	outVertices.Empty();
	outIndices.Empty();


	Mesh_domain domain =
		Mesh_domain::create_implicit_mesh_domain(sphere_function,
			K::Sphere_3(CGAL::ORIGIN, 2.));
	// Mesh criteria
	Mesh_criteria criteria(facet_angle = 25, facet_distance = 0.025,
		cell_radius_edge_ratio = 2);
	// Mesh generation
	C3t3 c3t3 = CGAL::make_mesh_3<C3t3>(domain, criteria);
	
	Tr& tr = c3t3.triangulation();

	//TArray<FVector> points3;
	std::map<Tr::Vertex_handle, size_t> Mapper;
	//TArray<FVector> Vertices;
	//TArray<size_t> Indices;
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
	//std::map<Vertex_handle> V;
	for (C3t3::Vertices_in_complex_iterator vit = c3t3.vertices_in_complex_begin(), end = c3t3.vertices_in_complex_end(); vit != end; ++vit)
	{
		Tr::Point p = vit->point();
		FVector newPoint(p.x(),p.y(), p.z());
		points.Add(newPoint);
	}
	outIndices.Reserve(c3t3.number_of_cells() * 4);

	for (C3t3::Cells_in_complex_iterator cit = c3t3.cells_in_complex_begin(), end = c3t3.cells_in_complex_end(); cit != end; ++cit)
	{
		//cit->vertex(0)->info();
		Vertex_handle v1 = cit->vertex(0);
		Vertex_handle v2 = cit->vertex(1);
		Vertex_handle v3 = cit->vertex(2);
		Vertex_handle v4 = cit->vertex(3);
		outIndices.Add(Mapper[v1]);
		outIndices.Add(Mapper[v2]);
		outIndices.Add(Mapper[v3]);
		outIndices.Add(Mapper[v4]);






	}
	

	// Output
	//std::ofstream medit_file("out.mesh");
	//c3t3.output_to_medit(medit_file);
	return;
}