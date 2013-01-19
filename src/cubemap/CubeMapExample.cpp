/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "CubeMapExample.h"

#include <PluginManager/PluginManager.h>
#include <Math/Constants.h>
#include <AbstractShaderProgram.h>
#include <DefaultFramebuffer.h>
#include <Extensions.h>
#include <IndexedMesh.h>
#include <Renderer.h>
#include <Texture.h>
#include <SceneGraph/Camera3D.h>
#include <Trade/AbstractImporter.h>

#include "CubeMap.h"
#include "Reflector.h"
#include "configure.h"

using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

CubeMapExample::CubeMapExample(int& argc, char** argv): GlutApplication(argc, argv, "Cube map example") {
    MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::ARB::texture_storage);
    MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::ARB::invalidate_subdata);

    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setFeature(Renderer::Feature::FaceCulling, true);

    /* Set up perspective camera */
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(3.0f));
    (camera = new SceneGraph::Camera3D<>(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        ->setPerspective(deg(55.0f), 1.0f, 0.001f, 100.0f);

    /* Load TGA importer plugin */
    PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    Trade::AbstractImporter* importer;
    if(manager.load("TgaImporter") != AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
        Error() << "Cannot load TGAImporter plugin from" << manager.pluginDirectory();
        std::exit(1);
    }
    resourceManager.set<Trade::AbstractImporter>("tga-importer", importer, ResourceDataState::Final, ResourcePolicy::Manual);

    /* Add objects to scene */
    (new CubeMap(argc == 2 ? argv[1] : "", &scene, &drawables))
        ->scale(Vector3(20.0f));

    (new Reflector(&scene, &drawables))
        ->scale(Vector3(0.5f))
        ->translate(Vector3::xAxis(-0.5f));

    (new Reflector(&scene, &drawables))
        ->scale(Vector3(0.3f))
        ->rotate(deg(37.0f), Vector3::xAxis())
        ->translate(Vector3::xAxis(0.3f));

    /* We don't need the importer anymore */
    resourceManager.free<Trade::AbstractImporter>();
}

void CubeMapExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    camera->setViewport(size);
}

void CubeMapExample::drawEvent() {
    defaultFramebuffer.clear(DefaultFramebuffer::Clear::Depth);
    defaultFramebuffer.invalidate({DefaultFramebuffer::InvalidationAttachment::Color});

    camera->draw(drawables);
    swapBuffers();
}

void CubeMapExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Up)
        cameraObject->rotate(deg(-10.0f), cameraObject->transformation().right().normalized());

    else if(event.key() == KeyEvent::Key::Down)
        cameraObject->rotate(deg(10.0f), cameraObject->transformation().right().normalized());

    else if(event.key() == KeyEvent::Key::Left || event.key() == KeyEvent::Key::Right) {
        GLfloat translationY = cameraObject->transformation().translation().y();
        cameraObject->translate(Vector3::yAxis(-translationY))
            ->rotateY(event.key() == KeyEvent::Key::Left ? deg(10.0f) : deg(-10.0f))
            ->translate(Vector3::yAxis(translationY));

    } else return;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::CubeMapExample)
