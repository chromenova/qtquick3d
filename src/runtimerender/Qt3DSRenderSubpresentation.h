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
#pragma once
#ifndef QDEMON_RENDER_SUBPRESENTATION_H
#define QDEMON_RENDER_SUBPRESENTATION_H

#include <Qt3DSOffscreenRenderManager.h>
#include <Qt3DSRenderSubPresentationHelper.h>
#include <Qt3DSRenderPresentation.h>
#include <Qt3DSSimpleTypes.h>
#include <QDemonRefCounted>
#include <Qt3DSAtomic.h>
#include <StringTable.h>
#include <qdemonrenderbasetypes.h>

namespace qt3ds {
namespace render {

    class CSubPresentationRenderer;

    struct CSubPresentationPickQuery : public IGraphObjectPickQuery
    {
        CSubPresentationRenderer &m_Renderer;

        CSubPresentationPickQuery(CSubPresentationRenderer &renderer)
            : m_Renderer(renderer)
        {
        }
        Qt3DSRenderPickResult Pick(const QVector2D &inMouseCoords,
                                          const QVector2D &inViewportDimensions,
                                          bool inPickEverything) override;
    };

    class CSubPresentationRenderer : public IOffscreenRenderer
    {
    public:
        IQt3DSRenderContext &m_RenderContext;
        SPresentation &m_Presentation;
        volatile qint32 mRefCount;
        SOffscreenRendererEnvironment m_LastRenderedEnvironment;
        CSubPresentationPickQuery m_PickQuery;
        CRegisteredString m_OffscreenRendererType;

        CSubPresentationRenderer(IQt3DSRenderContext &inRenderContext, SPresentation &inPresentation);

        QDEMON_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext.GetAllocator())

        SOffscreenRendererEnvironment GetDesiredEnvironment(QVector2D inPresScale) override;
        virtual SOffscreenRenderFlags
        NeedsRender(const SOffscreenRendererEnvironment &inEnvironment, QVector2D inPresScale,
                    const SRenderInstanceId instanceId) override;
        void Render(const SOffscreenRendererEnvironment &inEnvironment,
                    QDemonRenderContext & /*inRenderContext*/,
                    QVector2D inPresScale, SScene::RenderClearCommand inClearBuffer,
                    const SRenderInstanceId instanceId) override;
        void RenderWithClear(const SOffscreenRendererEnvironment &inEnvironment,
                             QDemonRenderContext &inRenderContext,
                             QVector2D inPresScale, SScene::RenderClearCommand inClearBuffer,
                             QVector3D inClearColor,
                             const SRenderInstanceId instanceId) override;
        IGraphObjectPickQuery *GetGraphObjectPickQuery(const SRenderInstanceId) override { return &m_PickQuery; }
        bool Pick(const QVector2D & /*inMouseCoords*/, const QVector2D & /*inViewportDimensions*/,
                  const SRenderInstanceId) override
        {
            return false;
        }
        void addCallback(IOffscreenRendererCallback *cb) override
        {

        }
        // Used for RTTI purposes so we can safely static-cast an offscreen renderer to a
        // CSubPresentationRenderer
        static const char *GetRendererName() { return "SubPresentation"; }
        CRegisteredString GetOffscreenRendererType() override { return m_OffscreenRendererType; }

        Qt3DSRenderPickResult DoGraphQueryPick(const QVector2D &inMouseCoords,
                                              const QVector2D &inViewportDimensions,
                                              bool inPickEverything);
    };
}
}

#endif
