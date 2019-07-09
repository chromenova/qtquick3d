/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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


#include "qdemonrendercustommaterialrendercontext.h"

#include <QtDemonRender/QDemonRenderTexture2D>

QT_BEGIN_NAMESPACE

QDemonCustomMaterialRenderContext::QDemonCustomMaterialRenderContext(const QDemonRenderLayer &inLayer,
                                                                     const QDemonLayerRenderData &inData,
                                                                     const QVector<QDemonRenderLight *> &inLights,
                                                                     const QDemonRenderCamera &inCamera,
                                                                     const QDemonRenderModel &inModel,
                                                                     const QDemonRenderSubset &inSubset,
                                                                     const QMatrix4x4 &inMvp,
                                                                     const QMatrix4x4 &inWorld,
                                                                     const QMatrix3x3 &inNormal,
                                                                     const QDemonRenderCustomMaterial &inMaterial,
                                                                     const QDemonRef<QDemonRenderTexture2D> &inDepthTex,
                                                                     const QDemonRef<QDemonRenderTexture2D> &inAoTex,
                                                                     QDemonShaderDefaultMaterialKey inMaterialKey,
                                                                     QDemonRenderableImage *inFirstImage,
                                                                     float inOpacity)
    : layer(inLayer)
    , layerData(inData)
    , lights(inLights)
    , camera(inCamera)
    , model(inModel)
    , subset(inSubset)
    , modelViewProjection(inMvp)
    , modelMatrix(inWorld)
    , normalMatrix(inNormal)
    , material(inMaterial)
    , depthTexture(inDepthTex)
    , aoTexture(inAoTex)
    , materialKey(inMaterialKey)
    , firstImage(inFirstImage)
    , opacity(inOpacity)
{
}

QDemonCustomMaterialRenderContext::~QDemonCustomMaterialRenderContext() = default;

QT_END_NAMESPACE
