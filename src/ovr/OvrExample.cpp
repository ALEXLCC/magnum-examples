/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015
              Jonathan Hale <squareys@googlemail.com>

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

#include <memory.h>
#include <Corrade/Containers/Array.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Mesh.h>
#include <Corrade/Utility/utilities.h>
#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Framebuffer.h>
#include <Magnum/Renderbuffer.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Context.h>

#include "Types.h"
#include "HmdCamera.h"
#include "CubeDrawable.h"

#include <Magnum/LibOvrIntegration/LibOvrIntegration.h>
#include <Magnum/LibOvrIntegration/Context.h>
#include <Magnum/LibOvrIntegration/Hmd.h>
#include <Magnum/LibOvrIntegration/HmdEnum.h>

namespace Magnum {

namespace Examples {

using namespace LibOvrIntegration;

/**
 * @brief The Application class of OVRExample.
 * @author Jonathan Hale (Squareys)
 */
class OvrExample: public Platform::Application {
public:
    explicit OvrExample(const Arguments& arguments);
    virtual ~OvrExample();

private:
    void drawEvent() override;

    LibOvrIntegration::Context _ovrContext;
    std::unique_ptr<Hmd> _hmd;

    std::unique_ptr<Buffer> _indexBuffer, _vertexBuffer;
    std::unique_ptr<Mesh> _mesh;
    std::unique_ptr<Shaders::Phong> _shader;

    Scene3D _scene;
    Object3D _cameraObject;
    Object3D _eyes[2];
    SceneGraph::DrawableGroup3D _drawables;
    std::unique_ptr<HmdCamera> _cameras[2];

    std::unique_ptr<Object3D> _cubes[4];
    std::unique_ptr<CubeDrawable> _cubeDrawables[4];

    std::unique_ptr<Framebuffer> _mirrorFramebuffer;
    Texture2D* _mirrorTexture;

    LayerEyeFov* _layer;
};

OvrExample::OvrExample(const Arguments& arguments) : Platform::Application(arguments, nullptr),
    _indexBuffer(nullptr), _vertexBuffer(nullptr), _mesh(nullptr),
    _shader(nullptr), _scene(), _cameraObject(&_scene)
{

    /* connect to an Hmd, or create a debug hmd with DK2 type in case none is connected. */
    _hmd = LibOvrIntegration::Context::get().createHmd(0, HmdType::DK2);

    /* get the hmd display resolution */
    Vector2i resolution = _hmd->resolution() / 2;

    /* create a context with the hmd display resolution */
    Configuration conf;
    conf.setTitle("Magnum OculusVR Example")
        .setSize(resolution)
        .setSampleCount(16);

    if(!tryCreateContext(conf))
        createContext(conf.setSampleCount(0));

    /* the oculus sdk compositor does some "magic" to reduce latency. For
     * that to work, vsync needs to be turned off. */
    if(!setSwapInterval(0))
        Error() << "Could not turn off vsync.";

    Renderer::enable(Renderer::Feature::DepthTest);

    _hmd->setEnabledCaps(HmdCapability::LowPersistence | HmdCapability::DynamicPrediction | HmdCapability::NoVSync);
    _hmd->configureTracking(HmdTrackingCapability::Orientation | HmdTrackingCapability::MagYawCorrection |
                            HmdTrackingCapability::Position, {});
    _hmd->configureRendering();

    /* setup mirroring of oculus sdk compositor results to a texture which can later be blitted
     * onto the defaultFramebuffer. */
    _mirrorTexture = &_hmd->createMirrorTexture(TextureFormat::RGBA, resolution);
    _mirrorFramebuffer.reset(new Framebuffer(Range2Di::fromSize({}, resolution)));
    _mirrorFramebuffer->attachTexture(Framebuffer::ColorAttachment(0), *_mirrorTexture, 0)
                      .mapForRead(Framebuffer::ColorAttachment(0));

    /* Setup cube mesh. */
    const Trade::MeshData3D cube = Primitives::Cube::solid();

    _vertexBuffer.reset(new Buffer());
    _vertexBuffer->setData(
                MeshTools::interleave(cube.positions(0), cube.normals(0)),
                BufferUsage::StaticDraw);

    Containers::Array<char> indexData;
    Mesh::IndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) =
            MeshTools::compressIndices(cube.indices());

    _indexBuffer.reset(new Buffer());
    _indexBuffer->setData(indexData, BufferUsage::StaticDraw);

    _mesh.reset(new Mesh());
    _mesh->setPrimitive(cube.primitive()).setCount(cube.indices().size()).addVertexBuffer(
                *_vertexBuffer, 0, Shaders::Phong::Position {},
                Shaders::Phong::Normal {}).setIndexBuffer(*_indexBuffer, 0, indexType,
                                                          indexStart, indexEnd);

    /* setup shader */
    _shader.reset(new Shaders::Phong());

    /* setup scene */
    _cubes[0] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[0]->rotateY(Deg(45.0f));
    _cubes[0]->translate({0.0f, 0.0f, -3.0f});
    _cubeDrawables[0] = std::unique_ptr<CubeDrawable>(new CubeDrawable(_mesh.get(), _shader.get(), {1.0f, 1.0f, 0.0f}, _cubes[0].get(), &_drawables));

    _cubes[1] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[1]->rotateY(Deg(45.0f));
    _cubes[1]->translate({5.0f, 0.0f, 0.0f});
    _cubeDrawables[1] = std::unique_ptr<CubeDrawable>(new CubeDrawable(_mesh.get(), _shader.get(), {1.0f, 0.0f, 0.0f}, _cubes[1].get(), &_drawables));

    _cubes[2] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[2]->rotateY(Deg(45.0f));
    _cubes[2]->translate({-10.0f, 0.0f, 0.0f});
    _cubeDrawables[2] = std::unique_ptr<CubeDrawable>(new CubeDrawable(_mesh.get(), _shader.get(), {0.0f, 0.0f, 1.0f}, _cubes[2].get(), &_drawables));

    _cubes[3] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[3]->rotateY(Deg(45.0f));
    _cubes[3]->translate({0.0f, 0.0f, 7.0f});
    _cubeDrawables[3] = std::unique_ptr<CubeDrawable>(new CubeDrawable(_mesh.get(), _shader.get(), {0.0f, 1.0f, 1.0f}, _cubes[3].get(), &_drawables));

    /* setup compositor layers */
    _layer = &LibOvrIntegration::Context::get().compositor().addLayerEyeFov();
    _layer->setFov(*_hmd.get());
    _layer->setHighQuality(true);

    /* setup cameras */
    _eyes[0].setParent(&_cameraObject);
    _eyes[1].setParent(&_cameraObject);

    for(int eye = 0; eye < 2; ++eye) {
        /* projection matrix is set in the camera, since it requires some hmd-specific fov etc. */
        _cameras[eye].reset(new HmdCamera(*_hmd, eye, _eyes[eye]));

        _layer->setColorTexture(eye, _cameras[eye]->textureSet());
        _layer->setViewport(eye, {{}, _cameras[eye]->viewport()});
    }

}

OvrExample::~OvrExample() {
}

void OvrExample::drawEvent() {
    /* get orientation and position of the hmd. */
    std::array<DualQuaternion, 2> poses = _hmd->pollEyePoses().eyePoses();

    /* draw the scene for both cameras */
    for(int eye = 0; eye < 2; ++eye) {
        /* set the transformation according to rift trackers */
        _eyes[eye].setTransformation(poses[eye].toMatrix());
        /* render each eye. */
        _cameras[eye]->draw(_drawables);
    }

    /* set the layers eye poses to the poses chached in the _hmd. */
    _layer->setRenderPoses(*_hmd.get());

    /* let the libOVR sdk compositor do its magic! */
    LibOvrIntegration::Context::get().compositor().submitFrame(*_hmd.get());

    /* blit mirror texture to defaultFramebuffer. */
    const Vector2i size = _mirrorTexture->imageSize(0);
    Framebuffer::blit(*_mirrorFramebuffer,
                      defaultFramebuffer,
                      {{0, size.y()}, {size.x(), 0}},
                      {{}, size},
                      FramebufferBlit::Color, FramebufferBlitFilter::Nearest);

    if(_hmd->isDebugHmd()) {
        /* provide some rotation, but only without real devices to avoid vr sickness ;) */
        _cameraObject.rotateY(Deg(0.1f));
    }

    swapBuffers();
    redraw();
}

}
}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::OvrExample)
