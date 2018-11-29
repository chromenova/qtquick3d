/****************************************************************************
**
** Copyright (C) 2008-2016 NVIDIA Corporation.
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
#ifndef QDEMON_RENDER_PREFILTER_TEXTURE_H
#define QDEMON_RENDER_PREFILTER_TEXTURE_H
#include <Qt3DSAtomic.h>
#include <qdemonrendertexture2d.h>
#include <Qt3DSRender.h>
#include <Qt3DSTypes.h>
#include <Qt3DSRenderLoadedTexture.h>

namespace qt3ds {
namespace render {

    class Qt3DSRenderPrefilterTexture : public QDemonRefCounted
    {
    public:
        Qt3DSRenderPrefilterTexture(QDemonRenderContext *inQDemonRenderContext, qint32 inWidth, qint32 inHeight,
                                  QDemonRenderTexture2D &inTexture,
                                  QDemonRenderTextureFormats::Enum inDestFormat,
                                  NVFoundationBase &inFnd);
        virtual ~Qt3DSRenderPrefilterTexture();

        virtual void Build(void *inTextureData, qint32 inTextureDataSize,
                           QDemonRenderTextureFormats::Enum inFormat) = 0;

        static Qt3DSRenderPrefilterTexture *Create(QDemonRenderContext *inQDemonRenderContext, qint32 inWidth,
                                                 qint32 inHeight, QDemonRenderTexture2D &inTexture,
                                                 QDemonRenderTextureFormats::Enum inDestFormat,
                                                 NVFoundationBase &inFnd);

    protected:
        NVFoundationBase &m_Foundation; ///< Foundation class for allocations and other base things
        volatile qint32 mRefCount; ///< reference count

        QDemonRenderTexture2D &m_Texture2D;
        QDemonRenderTextureFormats::Enum m_InternalFormat;
        QDemonRenderTextureFormats::Enum m_DestinationFormat;

        qint32 m_Width;
        qint32 m_Height;
        qint32 m_MaxMipMapLevel;
        qint32 m_SizeOfFormat;
        qint32 m_SizeOfInternalFormat;
        qint32 m_InternalNoOfComponent;
        qint32 m_NoOfComponent;
        QDemonRenderContext *m_QDemonRenderContext;
    };

    class Qt3DSRenderPrefilterTextureCPU : public Qt3DSRenderPrefilterTexture
    {
    public:
        Qt3DSRenderPrefilterTextureCPU(QDemonRenderContext *inQDemonRenderContext, qint32 inWidth,
                                     qint32 inHeight, QDemonRenderTexture2D &inTexture,
                                     QDemonRenderTextureFormats::Enum inDestFormat,
                                     NVFoundationBase &inFnd);

        void Build(void *inTextureData, qint32 inTextureDataSize,
                   QDemonRenderTextureFormats::Enum inFormat) override;

        STextureData CreateBsdfMipLevel(STextureData &inCurMipLevel, STextureData &inPrevMipLevel,
                                        qint32 width, qint32 height);
        QDEMON_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

        int wrapMod(int a, int base);
        void getWrappedCoords(int &sX, int &sY, int width, int height);
    };

    class Qt3DSRenderPrefilterTextureCompute : public Qt3DSRenderPrefilterTexture
    {
    public:
        Qt3DSRenderPrefilterTextureCompute(QDemonRenderContext *inQDemonRenderContext, qint32 inWidth,
                                         qint32 inHeight, QDemonRenderTexture2D &inTexture,
                                         QDemonRenderTextureFormats::Enum inDestFormat,
                                         NVFoundationBase &inFnd);
        ~Qt3DSRenderPrefilterTextureCompute();

        void Build(void *inTextureData, qint32 inTextureDataSize,
                   QDemonRenderTextureFormats::Enum inFormat) override;

        QDEMON_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

    private:
        void CreateLevel0Tex(void *inTextureData, qint32 inTextureDataSize,
                             QDemonRenderTextureFormats::Enum inFormat);

        QDemonScopedRefCounted<QDemonRenderShaderProgram> m_BSDFProgram;
        QDemonScopedRefCounted<QDemonRenderShaderProgram> m_UploadProgram_RGBA8;
        QDemonScopedRefCounted<QDemonRenderShaderProgram> m_UploadProgram_RGB8;
        QDemonScopedRefCounted<QDemonRenderTexture2D> m_Level0Tex;
        bool m_TextureCreated;

        void createComputeProgram(QDemonRenderContext *context);
        QDemonRenderShaderProgram *
        getOrCreateUploadComputeProgram(QDemonRenderContext *context,
                                        QDemonRenderTextureFormats::Enum inFormat);
    };
}
}

#endif
