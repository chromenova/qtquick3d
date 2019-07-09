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
    property bool uShadowMappingEnabled: false
    property real bump_amount: 0.5
    property real uTranslucentFalloff: 0.0
    property real uDiffuseLightWrap: 0.0
    property real uOpacity: 100.0
    property real transmission_weight: 0.2
    property real reflection_weight: 0.8
    property vector2d texture_tiling: Qt.vector2d(1.0, 1.0)
    property vector3d paper_color: Qt.vector3d(0.531, 0.531, 0.531)

    shaderInfo: DemonCustomMaterialShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: DemonCustomMaterialShaderInfo.Transmissive | DemonCustomMaterialShaderInfo.Diffuse
        layers: 1
    }

    property DemonCustomMaterialTexture uBakedShadowTexture: DemonCustomMaterialTexture {
            enabled: uShadowMappingEnabled
            type: DemonCustomMaterialTexture.LightmapShadow
            image: DemonImage {
                id: shadowImage
                source: "maps/shadow.png"
            }
    }
    property DemonCustomMaterialTexture diffuse_texture: DemonCustomMaterialTexture {
        type: DemonCustomMaterialTexture.Diffuse
        enabled: true
        image: DemonImage {
            tilingModeHorizontal: DemonImage.Repeat
            tilingModeVertical: DemonImage.Repeat
            source: "maps/paper_diffuse.png"
        }
    }
    property DemonCustomMaterialTexture bump_texture: DemonCustomMaterialTexture {
        type: DemonCustomMaterialTexture.Bump
        enabled: true
        image: DemonImage {
            tilingModeHorizontal: DemonImage.Repeat
            tilingModeVertical: DemonImage.Repeat
            source: "maps/paper_diffuse.png"
        }
    }
    property DemonCustomMaterialTexture transmission_texture: DemonCustomMaterialTexture {
        type: DemonCustomMaterialTexture.Bump
        enabled: true
        image: DemonImage {
            tilingModeHorizontal: DemonImage.Repeat
            tilingModeVertical: DemonImage.Repeat
            source: "maps/paper_trans.png"
        }
    }

    DemonCustomMaterialShader {
        id: paperOfficeFragShader
        stage: DemonCustomMaterialShader.Fragment
        shader: "shaders/paperOffice.frag"
    }

    passes: [ DemonCustomMaterialPass {
            shaders: paperOfficeFragShader
        }
    ]
}
