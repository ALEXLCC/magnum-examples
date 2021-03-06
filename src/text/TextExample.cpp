/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iomanip>
#include <sstream>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Math/Complex.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/Renderer.h>
#include <Magnum/Shaders/DistanceFieldVector.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>

#include "configure.h"

namespace Magnum { namespace Examples {

using namespace Magnum::Math::Literals;

class TextExample: public Platform::Application {
    public:
        explicit TextExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        void updateText();

        PluginManager::Manager<Text::AbstractFont> _manager;
        std::unique_ptr<Text::AbstractFont> _font;

        Text::DistanceFieldGlyphCache _cache;
        Mesh _text;
        Buffer _vertices, _indices;
        std::unique_ptr<Text::Renderer2D> _text2;
        Shaders::DistanceFieldVector2D _shader;

        Matrix3 _transformation;
        Matrix3 _projection;
};

TextExample::TextExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Text Example")), _manager(MAGNUM_PLUGINS_FONT_DIR), _cache(Vector2i(2048), Vector2i(512), 22), _text{NoCreate} {
    /* Load FreeTypeFont plugin */
    _font = _manager.loadAndInstantiate("FreeTypeFont");
    if(!_font) std::exit(1);

    /* Open the font and fill glyph cache */
    Utility::Resource rs("fonts");
    if(!_font->openSingleData(rs.getRaw("DejaVuSans.ttf"), 110.0f)) {
        Error() << "Cannot open font file";
        std::exit(1);
    }

    _font->fillGlyphCache(_cache, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-+,.!°ěäЗдравстуймиγειασουτνκόμ ");
    std::tie(_text, std::ignore) = Text::Renderer2D::render(*_font, _cache, 0.1295f,
        "Hello, world!\n"
        "Ahoj, světe!\n"
        "Здравствуй, мир!\n"
        "γεια σου, τον κόσμο!\n"
        "Hej Världen!",
        _vertices, _indices, BufferUsage::StaticDraw, Text::Alignment::MiddleCenter);

    _text2.reset(new Text::Renderer2D(*_font, _cache, 0.035f, Text::Alignment::TopRight));
    _text2->reserve(40, BufferUsage::DynamicDraw, BufferUsage::StaticDraw);

    Renderer::enable(Renderer::Feature::Blending);
    Renderer::setBlendFunction(Renderer::BlendFunction::SourceAlpha, Renderer::BlendFunction::OneMinusSourceAlpha);
    Renderer::setBlendEquation(Renderer::BlendEquation::Add, Renderer::BlendEquation::Add);

    _transformation = Matrix3::rotation(Deg(-10.0f));
    _projection = Matrix3::scaling(Vector2::yScale(Vector2(defaultFramebuffer.viewport().size()).aspectRatio()));
    updateText();
}

void TextExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    _projection = Matrix3::scaling(Vector2::yScale(Vector2(size).aspectRatio()));
}

void TextExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);

    _shader.setVectorTexture(_cache.texture());

    _shader.setTransformationProjectionMatrix(_projection * _transformation)
        .setColor(Color3::fromHSV(216.0_degf, 0.85f, 1.0f))
        .setOutlineColor(Color3{0.95f})
        .setOutlineRange(0.45f, 0.35f)
        .setSmoothness(0.025f/ _transformation.uniformScaling());
    _text.draw(_shader);

    _shader.setTransformationProjectionMatrix(_projection*
        Matrix3::translation(1.0f/ _projection.rotationScaling().diagonal()))
        .setColor(Color3{1.0f})
        .setOutlineRange(0.5f, 1.0f)
        .setSmoothness(0.075f);
    _text2->mesh().draw(_shader);

    swapBuffers();
}

void TextExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    if(event.offset().y() > 0)
        _transformation = Matrix3::rotation(Deg(1.0f))*Matrix3::scaling(Vector2(1.1f))* _transformation;
    else
        _transformation = Matrix3::rotation(Deg(-1.0f))*Matrix3::scaling(Vector2(1.0f/1.1f))* _transformation;

    updateText();

    event.setAccepted();
    redraw();
}

void TextExample::updateText() {
    std::ostringstream out;
    out << std::setprecision(2)
        << "Rotation: "
        << Float(Deg(Complex::fromMatrix(_transformation.rotation()).angle()))
        << "°\nScale: "
        << _transformation.uniformScaling();
    _text2->render(out.str());
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TextExample)
