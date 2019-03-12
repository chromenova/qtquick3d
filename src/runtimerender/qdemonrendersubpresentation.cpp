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
#include <qdemonrendersubpresentation.h>
#include <qdemonrenderrenderlist.h>
#ifdef _WIN32
#pragma warning(disable : 4355) // this used in initializer list.  I have never seen this result in
// a physical error
#endif
QT_BEGIN_NAMESPACE

QDemonRenderPickResult CSubPresentationPickQuery::pick(const QVector2D &inMouseCoords, const QVector2D &inViewportDimensions, bool inPickEverything)
{
    return m_renderer.doGraphQueryPick(inMouseCoords, inViewportDimensions, inPickEverything);
}

QDemonSubPresentationRenderer::QDemonSubPresentationRenderer(QDemonRenderContextInterface *inRenderContext,
                                                             QDemonRef<QDemonRenderPresentation> inPresentation)
    : m_renderContext(inRenderContext)
    , m_presentation(inPresentation)
    , m_pickQuery(*this)
    , m_offscreenRendererType(QString::fromLocal8Bit(getRendererName()))
{
}

QDemonOffscreenRendererEnvironment QDemonSubPresentationRenderer::getDesiredEnvironment(QVector2D /*inPresScale*/)
{
    // If we aren't using a clear color, then we are expected to blend with the background
    bool hasTransparency = m_presentation->scene->useClearColor ? false : true;
    QDemonRenderTextureFormat format = hasTransparency ? QDemonRenderTextureFormat::RGBA8
                                                              : QDemonRenderTextureFormat::RGB8;
    return QDemonOffscreenRendererEnvironment((quint32)(m_presentation->presentationDimensions.x()),
                                              (quint32)(m_presentation->presentationDimensions.y()),
                                              format,
                                              QDemonOffscreenRendererDepthValues::Depth16,
                                              false,
                                              QDemonRenderLayer::AAMode::NoAA);
}

QDemonOffscreenRenderFlags QDemonSubPresentationRenderer::needsRender(const QDemonOffscreenRendererEnvironment & /*inEnvironment*/,
                                                                      QVector2D /*inPresScale*/,
                                                                      const QDemonRenderInstanceId instanceId)
{
    bool hasTransparency = m_presentation->scene->useClearColor ? false : true;
    QRect theViewportSize(m_renderContext->getRenderList()->getViewport());
    bool wasDirty = m_presentation->scene->prepareForRender(QVector2D((float)theViewportSize.width(),
                                                                      (float)theViewportSize.height()),
                                                            m_renderContext,
                                                            instanceId);
    return QDemonOffscreenRenderFlags(hasTransparency, wasDirty);
}

// Returns true if the rendered result image has transparency, or false
// if it should be treated as a completely opaque image.
void QDemonSubPresentationRenderer::render(const QDemonOffscreenRendererEnvironment &inEnvironment,
                                           QDemonRenderContext &inRenderContext,
                                           QVector2D,
                                           QDemonRenderScene::RenderClearCommand inClearColorBuffer,
                                           const QDemonRenderInstanceId instanceId)
{
    QDemonSubPresentationHelper theHelper(m_renderContext, QSize((quint32)inEnvironment.width, (quint32)inEnvironment.height));
    QRect theViewportSize(inRenderContext.viewport());
    m_presentation->scene->render(QVector2D((float)theViewportSize.width(), (float)theViewportSize.height()),
                                  m_renderContext,
                                  inClearColorBuffer,
                                  instanceId);
    m_lastRenderedEnvironment = inEnvironment;
}

void QDemonSubPresentationRenderer::renderWithClear(const QDemonOffscreenRendererEnvironment &inEnvironment,
                                                    QDemonRenderContext &inRenderContext,
                                                    QVector2D inPresScale,
                                                    QDemonRenderScene::RenderClearCommand inClearBuffer,
                                                    QVector3D inClearColor,
                                                    const QDemonRenderInstanceId id)
{
    Q_UNUSED(inEnvironment);
    Q_UNUSED(inPresScale);
    QRect theViewportSize(inRenderContext.viewport());
    m_presentation->scene->renderWithClear(QVector2D((float)theViewportSize.width(), (float)theViewportSize.height()),
                                           m_renderContext,
                                           inClearBuffer,
                                           inClearColor,
                                           id);
}

// You know the viewport dimensions because
QDemonRenderPickResult QDemonSubPresentationRenderer::doGraphQueryPick(const QVector2D &inMouseCoords,
                                                                       const QVector2D &inViewportDimensions,
                                                                       bool inPickEverything)
{
    QDemonRenderPickResult thePickResult;

    if (m_presentation->scene && m_presentation->scene->firstChild) {
        thePickResult = m_renderContext->getRenderer()->pick(*m_presentation->scene->firstChild,
                                                             inViewportDimensions,
                                                             QVector2D(inMouseCoords.x(), inMouseCoords.y()),
                                                             true,
                                                             inPickEverything);
    }
    return thePickResult;
}
QT_END_NAMESPACE
