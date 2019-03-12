/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
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
#ifndef QDEMON_RENDER_WIDGETS_H
#define QDEMON_RENDER_WIDGETS_H

#include <QtCore/qpair.h>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMatrix3x3>
#include <QtGui/QVector3D>

#include <QtDemon/QDemonOption>
#include <QtDemon/QDemonBounds3>
#include <QtDemon/qdemondataref.h>

#include <QtDemonRender/qdemonrendervertexbuffer.h>
#include <QtDemonRender/qdemonrenderindexbuffer.h>

#include <QtDemonRuntimeRender/qdemonrendertext.h>
#include <QtDemonRuntimeRender/qdemonrenderer.h>

#include <QtDemonRuntimeRender/qdemonrendershadercodegenerator.h>

QT_BEGIN_NAMESPACE

class QDemonRenderContext;

struct QDemonWidgetRenderInformation
{
    // Just the rotation component of the nodeparenttocamera.
    QMatrix3x3 m_normalMatrix;
    // The node parent's global transform multiplied by the inverse camera global transfrom;
    // basically the MV from model-view-projection
    QMatrix4x4 m_nodeParentToCamera;
    // Projection that accounts for layer scaling
    QMatrix4x4 m_layerProjection;
    // Pure camera projection without layer scaling
    QMatrix4x4 m_pureProjection;
    // A look at matrix that will rotate objects facing directly up
    // the Z axis such that the point to the camera.
    QMatrix3x3 m_lookAtMatrix;
    // Conversion from world to camera position so world points not in object
    // local space can be converted to camera space without going through the node's
    // inverse global transform
    QMatrix4x4 m_cameraGlobalInverse;
    // Offset to add to the node's world position in camera space to move to the ideal camera
    // location so that scale will work.  This offset should be added *after* translation into
    // camera space
    QVector3D m_worldPosOffset;
    // Position in camera space to center the widget around
    QVector3D m_position;
    // Scale factor to scale the widget by.
    float m_scale = 1.0f;

    // The camera used to render this object.
    QDemonRenderCamera *m_camera = nullptr;
    QDemonWidgetRenderInformation(const QMatrix3x3 &inNormal,
                                  const QMatrix4x4 &inNodeParentToCamera,
                                  const QMatrix4x4 &inLayerProjection,
                                  const QMatrix4x4 &inProjection,
                                  const QMatrix3x3 &inLookAt,
                                  const QMatrix4x4 &inCameraGlobalInverse,
                                  const QVector3D &inWorldPosOffset,
                                  const QVector3D &inPos,
                                  float inScale,
                                  QDemonRenderCamera &inCamera)
        : m_normalMatrix(inNormal)
        , m_nodeParentToCamera(inNodeParentToCamera)
        , m_layerProjection(inLayerProjection)
        , m_pureProjection(inProjection)
        , m_lookAtMatrix(inLookAt)
        , m_cameraGlobalInverse(inCameraGlobalInverse)
        , m_worldPosOffset(inWorldPosOffset)
        , m_position(inPos)
        , m_scale(inScale)
        , m_camera(&inCamera)
    {
    }
    QDemonWidgetRenderInformation() = default;
};
typedef QPair<QDemonShaderVertexCodeGenerator &, QDemonShaderFragmentCodeGenerator &> TShaderGeneratorPair;

enum class RenderWidgetModes
{
    Local,
    Global,
};

class QDemonRenderWidgetInterface
{
public:
    QAtomicInt ref;
    virtual ~QDemonRenderWidgetInterface();
    QDemonRenderNode *m_node = nullptr;

    QDemonRenderWidgetInterface(QDemonRenderNode &inNode) : m_node(&inNode) {}
    QDemonRenderWidgetInterface() = default;
    virtual void render(QDemonRendererImpl &inWidgetContext, QDemonRenderContext &inRenderContext) = 0;
    QDemonRenderNode &getNode() { return *m_node; }

    // Pure widgets.
    static QDemonRef<QDemonRenderWidgetInterface> createBoundingBoxWidget(QDemonRenderNode &inNode,
                                                                          const QDemonBounds3 &inBounds,
                                                                          const QVector3D &inColor);
    static QDemonRef<QDemonRenderWidgetInterface> createAxisWidget(QDemonRenderNode &inNode);
};
QT_END_NAMESPACE

#endif
