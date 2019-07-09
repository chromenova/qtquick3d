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

#ifndef QDEMON_RENDER_IMAGE_TEXTURE_DATA_H
#define QDEMON_RENDER_IMAGE_TEXTURE_DATA_H

#include <QtDemonRender/qdemonrendertexture2d.h>

QT_BEGIN_NAMESPACE
// forward declararion
class QDemonRenderPrefilterTexture;

enum class QDemonImageTextureFlagValue
{
    HasTransparency = 1,
    InvertUVCoords = 1 << 1,
    PreMultiplied = 1 << 2,
};

struct QDemonRenderImageTextureFlags : public QFlags<QDemonImageTextureFlagValue>
{
    bool hasTransparency() const { return this->operator&(QDemonImageTextureFlagValue::HasTransparency); }
    void setHasTransparency(bool inValue) { setFlag(QDemonImageTextureFlagValue::HasTransparency, inValue); }

    bool isInvertUVCoords() const { return this->operator&(QDemonImageTextureFlagValue::InvertUVCoords); }
    void setInvertUVCoords(bool inValue) { setFlag(QDemonImageTextureFlagValue::InvertUVCoords, inValue); }

    bool isPreMultiplied() const { return this->operator&(QDemonImageTextureFlagValue::PreMultiplied); }
    void setPreMultiplied(bool inValue) { setFlag(QDemonImageTextureFlagValue::PreMultiplied, inValue); }
};

struct QDemonRenderImageTextureData
{
    QDemonRef<QDemonRenderTexture2D> m_texture;
    QDemonRenderImageTextureFlags m_textureFlags;
    QDemonRef<QDemonRenderPrefilterTexture> m_bsdfMipMap;

    QDemonRenderImageTextureData();
    ~QDemonRenderImageTextureData();

    bool operator!=(const QDemonRenderImageTextureData &inOther)
    {
        return m_texture != inOther.m_texture || m_textureFlags != inOther.m_textureFlags || m_bsdfMipMap != inOther.m_bsdfMipMap;
    }
};
QT_END_NAMESPACE

#endif
