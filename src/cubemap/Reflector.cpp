/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Reflector.h"

#include <sstream>
#include <Containers/Array.h>
#include <Utility/Resource.h>
#include <CubeMapTexture.h>
#include <Texture.h>
#include <TextureFormat.h>
#include <MeshTools/CompressIndices.h>
#include <MeshTools/Interleave.h>
#include <Primitives/UVSphere.h>
#include <SceneGraph/Camera3D.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>
#include <Trade/MeshData3D.h>

#include "ReflectorShader.h"

namespace Magnum { namespace Examples {

Reflector::Reflector(Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group) {
    CubeMapResourceManager& resourceManager = CubeMapResourceManager::instance();

    /* Sphere mesh */
    if(!(sphere = resourceManager.get<Mesh>("sphere"))) {
        Mesh* mesh = new Mesh;
        Buffer* buffer = new Buffer;
        Buffer* indexBuffer = new Buffer;

        Trade::MeshData3D sphereData = Primitives::UVSphere::solid(16, 32, Primitives::UVSphere::TextureCoords::Generate);
        MeshTools::interleave(*mesh, *buffer, Buffer::Usage::StaticDraw, sphereData.positions(0), sphereData.textureCoords2D(0));
        MeshTools::compressIndices(*mesh, *indexBuffer, Buffer::Usage::StaticDraw, sphereData.indices());
        mesh->setPrimitive(sphereData.primitive())
            .addVertexBuffer(*buffer, 0, ReflectorShader::Position(), ReflectorShader::TextureCoords());

        resourceManager.set("sphere-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set("sphere-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set(sphere.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Tarnish texture */
    if(!(tarnishTexture = resourceManager.get<Texture2D>("tarnish-texture"))) {
        Resource<Trade::AbstractImporter> importer = resourceManager.get<Trade::AbstractImporter>("jpeg-importer");
        Utility::Resource rs("data");
        importer->openData(rs.getRaw("tarnish.jpg"));

        std::optional<Trade::ImageData2D> image = importer->image2D(0);
        CORRADE_INTERNAL_ASSERT(image);
        auto texture = new Texture2D;
        texture->setWrapping(Sampler::Wrapping::ClampToEdge)
            .setMagnificationFilter(Sampler::Filter::Linear)
            .setMinificationFilter(Sampler::Filter::Linear, Sampler::Mipmap::Linear)
            .setStorage(Math::log2(image->size().min())+1, TextureFormat::RGB8, image->size())
            .setSubImage(0, {}, *image)
            .generateMipmap();

        resourceManager.set<Texture2D>(tarnishTexture.key(), texture, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Reflector shader */
    if(!(shader = resourceManager.get<AbstractShaderProgram, ReflectorShader>("reflector-shader")))
        resourceManager.set<AbstractShaderProgram>(shader.key(), new ReflectorShader, ResourceDataState::Final, ResourcePolicy::Resident);

    /* Texture (created in CubeMap class) */
    texture = resourceManager.get<CubeMapTexture>("texture");
}

void Reflector::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) {
    shader->setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix())
        .setReflectivity(2.0f)
        .setDiffuseColor(Color3(0.3f))
        .setCameraMatrix(static_cast<Object3D&>(camera.object()).absoluteTransformation().rotation())
        .use();

    texture->bind(ReflectorShader::TextureLayer);
    tarnishTexture->bind(ReflectorShader::TarnishTextureLayer);

    sphere->draw();
}

}}
