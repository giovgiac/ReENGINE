/*
 * Vertex.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Vertex.hpp"

namespace Re
{
    namespace Graphics
    {
		void CalculateAverageNormals(const boost::container::vector<u32>& indices, boost::container::vector<Vertex>& vertices)
		{
			for (usize i = 0; i < indices.size(); i += 3)
			{
				// Calculate indices to traverse the vertices.
				u32 in0 = indices[i];
				u32 in1 = indices[i + 1];
				u32 in2 = indices[i + 2];

				// Calculate lines to find normal, using the cross-product.
				Re::Math::Vector3 v1 = vertices[in1]._position - vertices[in0]._position;
				Re::Math::Vector3 v2 = vertices[in2]._position - vertices[in0]._position;
				Re::Math::Vector3 normal = Re::Math::Vector3::Cross(v1, v2).Normalize();

				// Add the normal to the vertices it belongs.
				vertices[in0]._normal += normal;
				vertices[in1]._normal += normal;
				vertices[in2]._normal += normal;
			}

			for (usize i = 0; i < vertices.size(); ++i)
			{
				// Normalize the normal of each vertex.
				vertices[i]._normal.Normalize();
			}
		}
    }
}
