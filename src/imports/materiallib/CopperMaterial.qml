/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtDemon 1.0

DemonCustomMaterial {
    // These properties names need to match the ones in the shader code!
    property bool uEnvironmentMappingEnabled: false
    property bool uShadowMappingEnabled: false
    property real roughness: 0.0
    property vector3d metal_color: Qt.vector3d(0.805, 0.395, 0.305)

    shaderInfo: DemonCustomMaterialShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: DemonCustomMaterialShaderInfo.Glossy
        layers: 1
    }

    property DemonCustomMaterialTexture uEnvironmentTexture: DemonCustomMaterialTexture {
            enabled: uEnvironmentMappingEnabled
            type: DemonCustomMaterialTexture.Environment
            image: DemonImage {
                id: envImage
                source: "maps/spherical_checker.png"
            }
    }
    property DemonCustomMaterialTexture uBakedShadowTexture: DemonCustomMaterialTexture {
            enabled: uShadowMappingEnabled
            type: DemonCustomMaterialTexture.LightmapShadow
            image: DemonImage {
                id: shadowImage
                source: "maps/shadow.png"
            }
    }

    DemonCustomMaterialShader {
        id: copperFragShader
        stage: DemonCustomMaterialShader.Fragment
        shader: "shaders/copper.frag"
    }

    passes: [ DemonCustomMaterialPass {
            shaders: copperFragShader
        }
    ]
}
