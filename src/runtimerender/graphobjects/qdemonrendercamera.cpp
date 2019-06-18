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

#include "qdemonrendercamera.h"

#include <QtDemonRuntimeRender/qdemonrendererutil.h>

#include <QtDemonRender/qdemonrendertexture2d.h>
#include <QtDemonRender/qdemonrendercontext.h>
#include <QtDemon/qdemonutils.h>

#include <QtGui/QVector2D>

#include <qmath.h>

QT_BEGIN_NAMESPACE

namespace {

float getAspectRatio(const QRectF &inViewport)
{
    return inViewport.height() != 0 ? inViewport.width() / inViewport.height() : 0.0f;
}

float getAspectRatio(const QVector2D &inDimensions)
{
    return inDimensions.y() != 0 ? inDimensions.x() / inDimensions.y() : 0.0f;
}

bool isCameraVerticalAdjust(QDemonRenderCamera::ScaleModes inMode, float inDesignAspect, float inActualAspect)
{
    return (inMode == QDemonRenderCamera::ScaleModes::Fit && inActualAspect >= inDesignAspect) || inMode == QDemonRenderCamera::ScaleModes::FitVertical;
}

#if 0
bool isCameraHorizontalAdjust(CameraScaleModes inMode, float inDesignAspect, float inActualAspect)
{
    return (inMode == CameraScaleModes::Fit && inActualAspect < inDesignAspect) || inMode == CameraScaleModes::FitHorizontal;
}
#endif

bool isFitTypeScaleMode(QDemonRenderCamera::ScaleModes inMode)
{
    return inMode == QDemonRenderCamera::ScaleModes::Fit || inMode == QDemonRenderCamera::ScaleModes::FitHorizontal || inMode == QDemonRenderCamera::ScaleModes::FitVertical;
}

struct QDemonPinCameraResult
{
    QRectF m_viewport;
    QRectF m_virtualViewport;
    QDemonPinCameraResult(QRectF v, QRectF vv) : m_viewport(v), m_virtualViewport(vv) {}
};
// Scale and transform the projection matrix to respect the camera anchor attribute
// and the scale mode.
QDemonPinCameraResult pinCamera(const QRectF &inViewport,
                                QVector2D inDesignDims,
                                QMatrix4x4 &ioPerspectiveMatrix,
                                QDemonRenderCamera::ScaleModes inScaleMode,
                                QDemonRenderCamera::ScaleAnchors inPinLocation)
{
    QRectF viewport(inViewport);
    QRectF idealViewport(inViewport.x(), inViewport.y(), inDesignDims.x(), inDesignDims.y());
    float designAspect = getAspectRatio(inDesignDims);
    float actualAspect = getAspectRatio(inViewport);
    if (isFitTypeScaleMode(inScaleMode)) {
        idealViewport.setWidth(viewport.width());
        idealViewport.setHeight(viewport.height());
    }
    // We move the viewport such that the left, top of the presentation sits against the left top
    // edge
    // We only need to translate in X *if* our actual aspect > design aspect
    // And then we only need to account for whatever centering would happen.

    bool pinLeft = inPinLocation == QDemonRenderCamera::ScaleAnchors::SouthWest || inPinLocation == QDemonRenderCamera::ScaleAnchors::West
            || inPinLocation == QDemonRenderCamera::ScaleAnchors::NorthWest;
    bool pinRight = inPinLocation == QDemonRenderCamera::ScaleAnchors::SouthEast || inPinLocation == QDemonRenderCamera::ScaleAnchors::East
            || inPinLocation == QDemonRenderCamera::ScaleAnchors::NorthEast;
    bool pinTop = inPinLocation == QDemonRenderCamera::ScaleAnchors::NorthWest || inPinLocation == QDemonRenderCamera::ScaleAnchors::North
            || inPinLocation == QDemonRenderCamera::ScaleAnchors::NorthEast;
    bool pinBottom = inPinLocation == QDemonRenderCamera::ScaleAnchors::SouthWest || inPinLocation == QDemonRenderCamera::ScaleAnchors::South
            || inPinLocation == QDemonRenderCamera::ScaleAnchors::SouthEast;

    if (inScaleMode == QDemonRenderCamera::ScaleModes::SameSize) {
        // In this case the perspective transform does not center the view,
        // it places it in the lower-left of the viewport.
        float idealWidth = inDesignDims.x();
        float idealHeight = inDesignDims.y();
        if (pinRight)
            idealViewport.setX(idealViewport.x() - ((idealWidth - inViewport.width())));
        else if (!pinLeft)
            idealViewport.setX(idealViewport.x() - ((idealWidth - inViewport.width()) / 2.0f));

        if (pinTop)
            idealViewport.setY(idealViewport.y() - ((idealHeight - inViewport.height())));
        else if (!pinBottom)
            idealViewport.setY(idealViewport.y() - ((idealHeight - inViewport.height()) / 2.0f));
    } else {
        // In this case our perspective matrix will center the view and we need to decenter
        // it as necessary
        // if we are wider than we are high
        if (isCameraVerticalAdjust(inScaleMode, designAspect, actualAspect)) {
            if (pinLeft || pinRight) {
                float idealWidth = inViewport.height() * designAspect;
                qint32 halfOffset = (qint32)((idealWidth - inViewport.width()) / 2.0f);
                halfOffset = pinLeft ? halfOffset : -1 * halfOffset;
                idealViewport.setX(idealViewport.x() + halfOffset);
            }
        } else {
            if (pinTop || pinBottom) {
                float idealHeight = inViewport.width() / designAspect;
                qint32 halfOffset = (qint32)((idealHeight - inViewport.height()) / 2.0f);
                halfOffset = pinBottom ? halfOffset : -1 * halfOffset;
                idealViewport.setY(idealViewport.y() + halfOffset);
            }
        }
    }

    ioPerspectiveMatrix = QDemonRenderContext::applyVirtualViewportToProjectionMatrix(ioPerspectiveMatrix, viewport, idealViewport);
    return QDemonPinCameraResult(viewport, idealViewport);
}
}

QDemonRenderCamera::QDemonRenderCamera()
    : QDemonRenderNode(QDemonRenderGraphObject::Type::Camera)
    , clipNear(10)
    , clipFar(10000)
    , fov(60)
    , fovHorizontal(false)
    , scaleMode(QDemonRenderCamera::ScaleModes::Fit)
    , scaleAnchor(QDemonRenderCamera::ScaleAnchors::Center)
{
    TORAD(fov);
    projection = QMatrix4x4();
    position = QVector3D(0, 0, -600);
}

// Code for testing
QDemonCameraGlobalCalculationResult QDemonRenderCamera::calculateGlobalVariables(const QRectF &inViewport, const QVector2D &inDesignDimensions)
{
    bool wasDirty = QDemonRenderNode::calculateGlobalVariables();
    return QDemonCameraGlobalCalculationResult{ wasDirty, calculateProjection(inViewport, inDesignDimensions) };
}

bool QDemonRenderCamera::calculateProjection(const QRectF &inViewport, const QVector2D &inDesignDimensions)
{
    bool retval = false;
    if (flags.testFlag(Flag::Orthographic))
        retval = computeFrustumOrtho(inViewport, inDesignDimensions);
    else
        retval = computeFrustumPerspective(inViewport, inDesignDimensions);
    if (retval) {
        float *writePtr(projection.data());
        frustumScale.setX(writePtr[0]);
        frustumScale.setY(writePtr[5]);
        pinCamera(inViewport, inDesignDimensions, projection, scaleMode, scaleAnchor);
    }
    return retval;
}

//==============================================================================
/**
 *	Compute the projection matrix for a perspective camera
 *	@return true if the computed projection matrix is valid
 */
bool QDemonRenderCamera::computeFrustumPerspective(const QRectF &inViewport, const QVector2D &inDesignDimensions)
{
    projection = QMatrix4x4();
    float theAngleInRadians = verticalFov(inViewport) / 2.0f;
    float theDeltaZ = clipFar - clipNear;
    float theSine = sinf(theAngleInRadians);
    float designAspect = getAspectRatio(inDesignDimensions);
    float theAspectRatio = designAspect;
    if (isFitTypeScaleMode(scaleMode))
        theAspectRatio = getAspectRatio(inViewport);

    if (!qFuzzyIsNull(theDeltaZ) && !qFuzzyIsNull(theSine) && !qFuzzyIsNull(theAspectRatio)) {
        float *writePtr(projection.data());
        writePtr[10] = -(clipFar + clipNear) / theDeltaZ;
        writePtr[11] = -1;
        writePtr[14] = -2 * clipNear * clipFar / theDeltaZ;
        writePtr[15] = 0;

        if (isCameraVerticalAdjust(scaleMode, designAspect, theAspectRatio)) {
            float theCotangent = cosf(theAngleInRadians) / theSine;
            writePtr[0] = theCotangent / theAspectRatio;
            writePtr[5] = theCotangent;
        } else {
            float theCotangent = cosf(theAngleInRadians) / theSine;
            writePtr[0] = theCotangent / designAspect;
            writePtr[5] = theCotangent * (theAspectRatio / designAspect);
        }
        return true;
    }

    Q_ASSERT(false);
    return false;
}

//==============================================================================
/**
 *	Compute the projection matrix for a orthographic camera
 *	@return true if the computed projection matrix is valid
 */
bool QDemonRenderCamera::computeFrustumOrtho(const QRectF &inViewport, const QVector2D &inDesignDimensions)
{
    projection = QMatrix4x4();

    float theDeltaZ = clipFar - clipNear;
    float halfWidth = inDesignDimensions.x() / 2.0f;
    float halfHeight = inDesignDimensions.y() / 2.0f;
    float designAspect = getAspectRatio(inDesignDimensions);
    float theAspectRatio = designAspect;
    if (isFitTypeScaleMode(scaleMode))
        theAspectRatio = getAspectRatio(inViewport);
    if (!qFuzzyIsNull(theDeltaZ)) {
        float *writePtr(projection.data());
        writePtr[10] = -2.0f / theDeltaZ;
        writePtr[11] = 0.0f;
        writePtr[14] = -(clipNear + clipFar) / theDeltaZ;
        writePtr[15] = 1.0f;
        if (isCameraVerticalAdjust(scaleMode, designAspect, theAspectRatio)) {
            writePtr[0] = 1.0f / (halfHeight * theAspectRatio);
            writePtr[5] = 1.0f / halfHeight;
        } else {
            writePtr[0] = 1.0f / halfWidth;
            writePtr[5] = 1.0f / (halfWidth / theAspectRatio);
        }
        return true;
    }

    Q_ASSERT(false);
    return false;
}

float QDemonRenderCamera::getOrthographicScaleFactor(const QRectF &inViewport, const QVector2D &inDesignDimensions) const
{
    if (scaleMode == QDemonRenderCamera::ScaleModes::SameSize)
        return 1.0f;
    QMatrix4x4 temp;
    float designAspect = getAspectRatio(inDesignDimensions);
    float theAspectRatio = getAspectRatio(inViewport);
    if (scaleMode == QDemonRenderCamera::ScaleModes::Fit) {
        if (theAspectRatio >= designAspect)
            return inViewport.width() < inDesignDimensions.x() ? theAspectRatio / designAspect : 1.0f;

        return inViewport.height() < inDesignDimensions.y() ? designAspect / theAspectRatio : 1.0f;
    } else if (scaleMode == QDemonRenderCamera::ScaleModes::FitVertical) {
        return (float)inDesignDimensions.y() / (float)inViewport.height();
    } else {
        return (float)inDesignDimensions.x() / (float)inViewport.width();
    }
}

QMatrix3x3 QDemonRenderCamera::getLookAtMatrix(const QVector3D &inUpDir, const QVector3D &inDirection) const
{
    QVector3D theDirection(inDirection);

    theDirection.normalize();

    const QVector3D &theUpDir(inUpDir);

    // gram-shmidt orthogonalization
    QVector3D theCrossDir = QVector3D::crossProduct(theDirection, theUpDir);
    theCrossDir.normalize();
    QVector3D theFinalDir = QVector3D::crossProduct(theCrossDir, theDirection);
    theFinalDir.normalize();
    float multiplier = 1.0f;
    if (flags.testFlag(Flag::LeftHanded))
        multiplier = -1.0f;

    theDirection *= multiplier;
    float matrixData[9] = { theCrossDir.x(), theFinalDir.x(), theDirection.x(),
                            theCrossDir.y(), theFinalDir.y(), theDirection.y(),
                            theCrossDir.z(), theFinalDir.z(), theDirection.z()
                          };

    QMatrix3x3 theResultMatrix(matrixData);
    return theResultMatrix;
}

void QDemonRenderCamera::lookAt(const QVector3D &inCameraPos, const QVector3D &inUpDir, const QVector3D &inTargetPos)
{
    QVector3D theDirection = inTargetPos - inCameraPos;
    if (flags.testFlag(Flag::LeftHanded))
        theDirection.setZ(theDirection.z() * -1.0f);
    rotation = getRotationVectorFromRotationMatrix(getLookAtMatrix(inUpDir, theDirection));
    position = inCameraPos;
    markDirty(TransformDirtyFlag::TransformIsDirty);
}

void QDemonRenderCamera::calculateViewProjectionMatrix(QMatrix4x4 &outMatrix) const
{
    QMatrix4x4 globalInverse = mat44::getInverse(globalTransform);
    outMatrix = projection * globalInverse;
}

QDemonCuboidRect QDemonRenderCamera::getCameraBounds(const QRectF &inViewport, const QVector2D &inDesignDimensions) const
{
    QMatrix4x4 unused;
    QDemonPinCameraResult theResult = pinCamera(inViewport, inDesignDimensions, unused, scaleMode, scaleAnchor);
    // find the normalized edges of the view frustum given the renormalization that happens when
    // pinning the camera.
    QDemonCuboidRect normalizedCuboid(-1, 1, 1, -1);
    QVector2D translation(theResult.m_viewport.x() - theResult.m_virtualViewport.x(),
                          theResult.m_viewport.y() - theResult.m_virtualViewport.y());
    if (scaleMode == QDemonRenderCamera::ScaleModes::SameSize) {
        // the cuboid ranges are the actual divided by the ideal in this case
        float xRange = 2.0f * (theResult.m_viewport.width() / theResult.m_virtualViewport.width());
        float yRange = 2.0f * (theResult.m_viewport.height() / theResult.m_virtualViewport.height());
        normalizedCuboid = QDemonCuboidRect(-1, -1 + yRange, -1 + xRange, -1);
        translation.setX(translation.x() / (theResult.m_virtualViewport.width() / 2.0f));
        translation.setY(translation.y() / (theResult.m_virtualViewport.height() / 2.0f));
        normalizedCuboid.translate(translation);
    }
    // fit.  This means that two parameters of the normalized cuboid will be -1, 1.
    else {
        // In this case our perspective matrix will center the view and we need to decenter
        // it as necessary
        float actualAspect = getAspectRatio(inViewport);
        float designAspect = getAspectRatio(inDesignDimensions);
        // if we are wider than we are high
        float idealWidth = inViewport.width();
        float idealHeight = inViewport.height();

        if (isCameraVerticalAdjust(scaleMode, designAspect, actualAspect)) {
            // then we just need to setup the left, right parameters of the cuboid because we know
            // the top
            // bottom are -1,1 due to how fit works.
            idealWidth = (float)QDemonRendererUtil::nextMultipleOf4((quint32)(inViewport.height() * designAspect + .5f));
            // halfRange should always be greater than 1.0f.
            float halfRange = inViewport.width() / idealWidth;
            normalizedCuboid.left = -halfRange;
            normalizedCuboid.right = halfRange;
            translation.setX(translation.x() / (idealWidth / 2.0f));
        } else {
            idealHeight = (float)QDemonRendererUtil::nextMultipleOf4((quint32)(inViewport.width() / designAspect + .5f));
            float halfRange = inViewport.height() / idealHeight;
            normalizedCuboid.bottom = -halfRange;
            normalizedCuboid.top = halfRange;
            translation.setY(translation.y() / (idealHeight / 2.0f));
        }
        normalizedCuboid.translate(translation);
    }
    // Given no adjustment in the virtual rect, then this is what we would have.

    return normalizedCuboid;
}

void QDemonRenderCamera::setupOrthographicCameraForOffscreenRender(QDemonRenderTexture2D &inTexture, QMatrix4x4 &outVP)
{
    QDemonTextureDetails theDetails(inTexture.textureDetails());
    // TODO:
    Q_UNUSED(theDetails);
    QDemonRenderCamera theTempCamera;
    setupOrthographicCameraForOffscreenRender(inTexture, outVP, theTempCamera);
}

void QDemonRenderCamera::setupOrthographicCameraForOffscreenRender(QDemonRenderTexture2D &inTexture, QMatrix4x4 &outVP, QDemonRenderCamera &outCamera)
{
    QDemonTextureDetails theDetails(inTexture.textureDetails());
    QDemonRenderCamera theTempCamera;
    theTempCamera.flags.setFlag(Flag::Orthographic);
    theTempCamera.markDirty(TransformDirtyFlag::TransformIsDirty);
    QVector2D theDimensions((float)theDetails.width, (float)theDetails.height);
    theTempCamera.calculateGlobalVariables(QRect(0, 0, theDetails.width, theDetails.height), theDimensions);
    theTempCamera.calculateViewProjectionMatrix(outVP);
    outCamera = theTempCamera;
}

QDemonRenderRay QDemonRenderCamera::unproject(const QVector2D &inViewportRelativeCoords,
                                              const QRectF &inViewport,
                                              const QVector2D &inDesignDimensions) const
{
    QDemonRenderRay theRay;
    QMatrix4x4 tempVal;
    QDemonPinCameraResult result = pinCamera(inViewport, inDesignDimensions, tempVal, scaleMode, scaleAnchor);
    QVector2D globalCoords = toAbsoluteCoords(inViewport, inViewportRelativeCoords);
    QVector2D normalizedCoords = absoluteToNormalizedCoordinates(result.m_virtualViewport, globalCoords);
    QVector3D &outOrigin(theRay.origin);
    QVector3D &outDir(theRay.direction);
    QVector2D inverseFrustumScale(1.0f / frustumScale.x(), 1.0f / frustumScale.y());
    QVector2D scaledCoords(inverseFrustumScale.x() * normalizedCoords.x(), inverseFrustumScale.y() * normalizedCoords.y());

    if (flags.testFlag(Flag::Orthographic)) {
        outOrigin.setX(scaledCoords.x());
        outOrigin.setY(scaledCoords.y());
        outOrigin.setZ(0.0f);

        outDir.setX(0.0f);
        outDir.setY(0.0f);
        outDir.setZ(-1.0f);
    } else {
        outOrigin.setX(0.0f);
        outOrigin.setY(0.0f);
        outOrigin.setZ(0.0f);

        outDir.setX(scaledCoords.x());
        outDir.setY(scaledCoords.y());
        outDir.setZ(-1.0f);
    }

    outOrigin = mat44::transform(globalTransform, outOrigin);
    QMatrix3x3 theNormalMatrix = calculateNormalMatrix();

    outDir = mat33::transform(theNormalMatrix, outDir);
    outDir.normalize();
    /*
    char printBuf[2000];
    sprintf_s( printBuf, "normCoords %f %f outDir %f %f %f\n"
            , normalizedCoords.x, normalizedCoords.y, outDir.x, outDir.y, outDir.z );
    OutputDebugStringA( printBuf );
    */

    return theRay;
}

QVector3D QDemonRenderCamera::unprojectToPosition(const QVector3D &inGlobalPos, const QDemonRenderRay &inRay) const
{
    QVector3D theCameraDir = getDirection();
    QVector3D theObjGlobalPos = inGlobalPos;
    float theDistance = -1.0f * QVector3D::dotProduct(theObjGlobalPos, theCameraDir);
    QDemonPlane theCameraPlane(theCameraDir, theDistance);
    return inRay.intersect(theCameraPlane);
}

float QDemonRenderCamera::verticalFov(float aspectRatio) const
{
    if (fovHorizontal)
        return 2.0f * qAtan(qTan(qreal(fov) / 2.0) / qreal(aspectRatio));
    else
        return fov;
}

float QDemonRenderCamera::verticalFov(const QRectF &inViewport) const
{
    return verticalFov(getAspectRatio(inViewport));
}

QT_END_NAMESPACE
