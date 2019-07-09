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

#ifndef QDEMON_RENDER_LOADED_TEXTURE_H
#define QDEMON_RENDER_LOADED_TEXTURE_H

#include <QtDemonRender/qdemonrenderbasetypes.h>

#include <QtDemonRuntimeRender/qdemonrenderinputstreamfactory.h>

#include <QtGui/QImage>

QT_BEGIN_NAMESPACE
class QDemonInputStreamFactory;

struct QDemonTextureData
{
    void *data = nullptr;
    quint32 dataSizeInBytes = 0;
    QDemonRenderTextureFormat format = QDemonRenderTextureFormat::Unknown;
};
enum class QDemonExtendedTextureFormats
{
    NoExtendedFormat = 0,
    Palettized,
    CustomRGB,
};
// Utility class used for loading image data from disk.
// Supports jpg, png, and dds.
struct QDemonLoadedTexture
{
public:
    QAtomicInt ref;
    qint32 width = 0;
    qint32 height = 0;
    qint32 components = 0;
    void *data = nullptr;
    QImage image;
    quint32 dataSizeInBytes = 0;
    QDemonRenderTextureFormat format = QDemonRenderTextureFormat::RGBA8;
    QDemonExtendedTextureFormats m_ExtendedFormat = QDemonExtendedTextureFormats::NoExtendedFormat;
    // Used for palettized images.
    void *m_palette = nullptr;
    qint32 m_customMasks[3]{ 0, 0, 0 };
    int m_bitCount = 0;
    char m_backgroundColor[3]{ 0, 0, 0 };
    quint8 *m_transparencyTable = nullptr;
    qint32 m_transparentPaletteIndex = -1;

    ~QDemonLoadedTexture();
    void setFormatFromComponents()
    {
        switch (components) {
        case 1: // undefined, but in this context probably luminance
            format = QDemonRenderTextureFormat::Luminance8;
            break;
        case 2:
            format = QDemonRenderTextureFormat::LuminanceAlpha8;
            break;
        case 3:
            format = QDemonRenderTextureFormat::RGB8;
            break;

        default:
            // fallthrough intentional
        case 4:
            format = QDemonRenderTextureFormat::RGBA8;
            break;
        }
    }

    // Returns true if this image has a pixel less than 255.
    bool scanForTransparency();

    static QDemonRef<QDemonLoadedTexture> load(const QString &inPath,
                                               const QDemonRenderTextureFormat &inFormat,
                                               QDemonInputStreamFactory &inFactory,
                                               bool inFlipY = true,
                                               const QDemonRenderContextType &renderContextType = QDemonRenderContextType::NullContext);
    static QDemonRef<QDemonLoadedTexture> loadQImage(const QString &inPath,
                                                     const QDemonRenderTextureFormat &inFormat,
                                                     qint32 flipVertical,
                                                     QDemonRenderContextType renderContextType);
    static QDemonRef<QDemonLoadedTexture> loadHdrImage(QSharedPointer<QIODevice> source, QDemonRenderContextType renderContextType);

};
QT_END_NAMESPACE

#endif
