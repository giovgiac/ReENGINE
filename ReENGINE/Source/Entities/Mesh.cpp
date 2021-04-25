/*
 * Mesh.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Mesh.hpp"

#include "Components/RenderComponent.hpp"
#include "Graphics/Material.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Vertex.hpp"
#include "Math/Vector3.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/container/deque.hpp>
#include <boost/container/vector.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

static Assimp::Importer assetImporter;

namespace Re
{
    namespace Entities
    {
        Mesh::Mesh()
            : _filename(""), _transformComponent(nullptr)
        {
            // Create default components.
            _transformComponent = AddComponent<Components::TransformComponent>();
        }

        Mesh::Mesh(const utf8* filename)
            : Mesh()
        {
            // Set values as specified.
            _filename = filename;
        }

        void Mesh::Initialize()
        {
            Entity::Initialize();

            // Load the mesh.
            Load();
        }

        void Mesh::Update(f32 deltaTime)
        {
            Entity::Update(deltaTime);

            // TEST CODE: Set a smooth rotation on the mesh object (for testing purposes).
            static f32 rotationSpeed = 16.0f;
            Math::Rotator rotation = _transformComponent->GetRotation();
            if (rotation._yaw >= 360.0f)
                _transformComponent->SetRotation(rotation._pitch, rotation._roll, 0.0f);
            //_transformComponent->Rotate(0.0f, 0.0f, rotationSpeed * deltaTime);
        }

        void Mesh::Load()
        {
            if (_filename == "") return;

            // Import the scene from file, so that meshes, materials and textures can be imported.
            const aiScene* scene = assetImporter.ReadFile(_filename, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
            if (scene)
            {
                // Go through each material and copy them into the ReENGINE format.
                boost::container::vector<boost::shared_ptr<Graphics::Material>> materials(scene->mNumMaterials);
                for (usize i = 0; i < scene->mNumMaterials; ++i)
                {
                    aiMaterial* material = scene->mMaterials[i];

                    if (material->GetTextureCount(aiTextureType_DIFFUSE))
                    {
                        aiString texturePath;
                        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
                        {
                            // Attempt to remove directory information from the texture path and create texture.
                            usize id = std::string(texturePath.data).rfind("\\");
                            std::string textureFilename = "Textures/" + std::string(texturePath.data).substr(id + 1);
                            boost::shared_ptr<Graphics::Texture> texture = boost::make_shared<Graphics::Texture>(textureFilename.c_str());

                            // Create the material using the retrieved parameters and texture.
                            materials[i] = boost::make_shared<Graphics::Material>(1.0f, 0.0f, texture);
                        }
                        else
                        {
                            // Resort back to the default material.
                            materials[i] = boost::make_shared<Graphics::Material>();
                        }
                    }
                    else
                    {
                        // Resort back to the default material.
                        materials[i] = boost::make_shared<Graphics::Material>();
                    }
                }

                // Go through every single mesh in every node and create render components for them.
                boost::container::deque<aiNode*> nodes = { scene->mRootNode };
                while (!nodes.empty())
                {
                    // Retrieve and remove first element from deque.
                    aiNode* node = nodes.front();
                    nodes.pop_front();

                    // Go through each mesh at node and create a render component for it.
                    for (usize i = 0; i < node->mNumMeshes; ++i)
                    {
                        // Set a variable to easily access the mesh.
                        aiMesh* currentMesh = scene->mMeshes[node->mMeshes[i]];
                        bool shouldCalculateNormals = false;

                        // Create the vectors to store the relevant mesh data.
                        boost::container::vector<Graphics::Vertex> vertices;
                        boost::container::vector<u32> indices;

                        // Resize vectors to fit data appropriately.
                        vertices.resize(currentMesh->mNumVertices);
                        
                        // Go through each vertex and copy data across.
                        for (usize j = 0; j < currentMesh->mNumVertices; ++j)
                        {
                            // Copy vertex position data.
                            vertices[j]._position = Math::Vector3(currentMesh->mVertices[j].x, currentMesh->mVertices[j].y, currentMesh->mVertices[j].z);

                            // Copy vertex normal data.
                            if (currentMesh->HasNormals())
                            {
                                vertices[j]._normal = Math::Vector3(currentMesh->mNormals[j].x, currentMesh->mNormals[j].y, currentMesh->mNormals[j].z);
                            }
                            else
                            {
                                vertices[j]._normal = Math::Vector3(0.0f);
                                shouldCalculateNormals = true;
                            }

                            // Copy vertex UV data.
                            if (currentMesh->HasTextureCoords(0))
                            {
                                vertices[j]._textureCoordinate = Math::Vector(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y);
                            }
                            else
                            {
                                vertices[j]._textureCoordinate = Math::Vector::Zero();
                            }
                        }

                        // Go through each face to get indices and copy data across.
                        for (usize j = 0; j < currentMesh->mNumFaces; ++j)
                        {
                            aiFace face = currentMesh->mFaces[j];
                            for (usize k = 0; k < face.mNumIndices; ++k)
                            {
                                // Copy index data.
                                indices.push_back(face.mIndices[k]);
                            }
                        }

                        // Calculate normals if required.
                        if (shouldCalculateNormals)
                            Graphics::CalculateAverageNormals(vertices, indices);

                        // Create the render component that represents this mesh.
                        AddComponent<Components::RenderComponent>(vertices, indices, materials[currentMesh->mMaterialIndex]);
                    }

                    // Go through each node attached to this node to load their meshes as well.
                    for (usize i = 0; i < node->mNumChildren; ++i)
                    {
                        nodes.push_front(node->mChildren[i]);
                    }
                }
            }
        }

        Components::TransformComponent* Mesh::GetTransform() const
        {
            return _transformComponent;
        }
    }
}
