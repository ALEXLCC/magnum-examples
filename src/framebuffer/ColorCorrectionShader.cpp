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

#include "ColorCorrectionShader.h"

#include "Utility/Resource.h"

using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

ColorCorrectionShader::ColorCorrectionShader() {
    Resource rs("shader");
    attachShader(Shader::fromData(Shader::Type::Vertex, rs.get("ColorCorrectionShader.vert")));
    attachShader(Shader::fromData(Shader::Type::Fragment, rs.get("ColorCorrectionShader.frag")));

    bindAttribute(Vertex::Location, "vertex");
    bindFragmentDataLocation(0, "original");
    bindFragmentDataLocation(1, "grayscale");
    bindFragmentDataLocation(2, "corrected");

    link();

    matrixUniform = uniformLocation("matrix");
    textureUniform = uniformLocation("textureData");
    correctionTextureUniform = uniformLocation("correctionTextureData");
}

}}
