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
#ifndef QDEMON_RENDER_CAMERA_H
#define QDEMON_RENDER_CAMERA_H

#include <QtDemonRuntimeRender/qtdemonruntimerenderglobal.h>

#include <QtDemonRuntimeRender/qdemonrendernode.h>
#include <QtDemonRuntimeRender/qdemonrenderray.h>

#include <QtDemonRender/qdemonrenderbasetypes.h>

QT_BEGIN_NAMESPACE

struct QDemonCameraGlobalCalculationResult
{
    bool m_wasDirty;
    bool m_computeFrustumSucceeded /* = true */;
};

struct QDemonCuboidRect
{
    float left;
    float top;
    float right;
    float bottom;
    constexpr QDemonCuboidRect(float l = 0.0f, float t = 0.0f, float r = 0.0f, float b = 0.0f)
        : left(l), top(t), right(r), bottom(b)
    {
    }
    void translate(QVector2D inTranslation)
    {
        left += inTranslation.x();
        right += inTranslation.x();
        top += inTranslation.y();
        bottom += inTranslation.y();
    }
};

struct Q_DEMONRUNTIMERENDER_EXPORT QDemonRenderCamera : public QDemonGraphNode
{
    enum class ScaleModes : quint8
    {
        Fit = 0,
        SameSize,
        FitHorizontal,
        FitVertical,
    };

    enum class ScaleAnchors : quint8
    {
        Center = 0,
        North,
        NorthEast,
        East,
        SouthEast,
        South,
        SouthWest,
        West,
        NorthWest,
    };

    // Setting these variables should set dirty on the camera.
    float clipNear;
    float clipFar;

    float fov; // Radians
    bool fovHorizontal;

    QMatrix4x4 projection;
    ScaleModes scaleMode;
    ScaleAnchors scaleAnchor;
    // Record some values from creating the projection matrix
    // to use during mouse picking.
    QVector2D frustumScale;

    QDemonRenderCamera();

    QMatrix3x3 getLookAtMatrix(const QVector3D &inUpDir, const QVector3D &inDirection) const;
    // Set our position, rotation member variables based on the lookat target
    // Marks this object as dirty.
    // Need to test this when the camera's local transform is null.
    // Assumes parent's local transform is the identity, meaning our local transform is
    // our global transform.
    void lookAt(const QVector3D &inCameraPos, const QVector3D &inUpDir, const QVector3D &inTargetPos);

    QDemonCameraGlobalCalculationResult calculateGlobalVariables(const QRectF &inViewport, const QVector2D &inDesignDimensions);
    bool calculateProjection(const QRectF &inViewport, const QVector2D &inDesignDimensions);
    bool computeFrustumOrtho(const QRectF &inViewport, const QVector2D &inDesignDimensions);
    // Used when rendering the widgets in studio.  This scales the widget when in orthographic
    // mode in order to have
    // constant size on screen regardless.
    // Number is always greater than one
    float getOrthographicScaleFactor(const QRectF &inViewport, const QVector2D &inDesignDimensions) const;
    bool computeFrustumPerspective(const QRectF &inViewport, const QVector2D &inDesignDimensions);
    // Text may be scaled so that it doesn't appear pixellated when the camera itself is doing
    // the scaling.
    float getTextScaleFactor(const QRectF &inViewport, const QVector2D &inDesignDimensions) const;

    void calculateViewProjectionMatrix(QMatrix4x4 &outMatrix) const;

    // If this is an orthographic camera, the cuboid properties are the distance from the center
    // point
    // to the left, top, right, and bottom edges of the view frustum in world units.
    // If this is a perspective camera, the cuboid properties are the FOV angles
    // (left,top,right,bottom)
    // of the view frustum.

    // Return a normalized rect that describes the area the camera is rendering to.
    // This takes into account the various camera properties (scale mode, scale anchor).
    QDemonCuboidRect getCameraBounds(const QRectF &inViewport, const QVector2D &inDesignDimensions) const;

    // Setup a camera VP projection for rendering offscreen.
    static void setupOrthographicCameraForOffscreenRender(QDemonRenderTexture2D &inTexture, QMatrix4x4 &outVP);
    static void setupOrthographicCameraForOffscreenRender(QDemonRenderTexture2D &inTexture, QMatrix4x4 &outVP, QDemonRenderCamera &outCamera);

    // Unproject a point (x,y) in viewport relative coordinates meaning
    // left, bottom is 0,0 and values are increasing right,up respectively.
    QDemonRenderRay unproject(const QVector2D &inLayerRelativeMouseCoords, const QRectF &inViewport, const QVector2D &inDesignDimensions) const;

    // Unproject a given coordinate to a 3d position that lies on the same camera
    // plane as inGlobalPos.
    // Expects CalculateGlobalVariables has been called or doesn't need to be.
    QVector3D unprojectToPosition(const QVector3D &inGlobalPos, const QDemonRenderRay &inRay) const;

    float verticalFov(float aspectRatio) const;
    float verticalFov(const QRectF &inViewport) const;
};

QT_END_NAMESPACE

#endif
