#pragma once
#include "CoreMinimal.h"
#include <memory>

struct Tetrahedron;

namespace Icarus
{
	class ICARUS_API SurfaceMesh
	{
	public:
		~SurfaceMesh();


		std::shared_ptr<SurfaceMesh> Intersect(std::shared_ptr<SurfaceMesh> other);
		bool Intersect2(std::shared_ptr<SurfaceMesh> other);

		std::shared_ptr<SurfaceMesh> Union(std::shared_ptr<SurfaceMesh> other);
		bool Union2(std::shared_ptr<SurfaceMesh> other);

		void GetVerticesAndIndices(TArray<FVector>& OutVertices, TArray<int32>& OutIndices);

		float CalculateVolume() const;

		void Translate(const FVector& V);

		static std::shared_ptr<SurfaceMesh> FromCapsule(float Length, float Radius);
		static std::shared_ptr<SurfaceMesh> FromBox(const FBox& box);
		static std::shared_ptr<SurfaceMesh> FromTetrahedron(const Tetrahedron& box);
		static std::shared_ptr<SurfaceMesh> FromTriangles(const TArray<FVector>& vertices, const TArray<int32>& indices);
		static std::shared_ptr<SurfaceMesh> Create();
	private:
		SurfaceMesh();

		class SurfaceMesh_Data* data;


	};
}