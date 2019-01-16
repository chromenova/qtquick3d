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
#ifndef QDEMON_RENDER_IMAGE_H
#define QDEMON_RENDER_IMAGE_H

#include <QtDemonRuntimeRender/qdemonrendergraphobject.h>
#include <QtDemonRuntimeRender/qdemonrendernode.h>
#include <QtDemonRuntimeRender/qdemonrenderimagetexturedata.h>
#include <QtDemonRuntimeRender/qtdemonruntimerenderglobal.h>
#include <QtDemonRuntimeRender/qdemonrenderplugingraphobject.h>
// TODO: Add back later
//#include <QtDemonRuntimeRender/qdemonrenderplugin.h>
#include <QtDemonRender/qdemonrendertexture2d.h>

#include <QtGui/QVector2D>

QT_BEGIN_NAMESPACE
class IQDemonRenderContext;
class IOffscreenRenderManager;
class IOffscreenRenderer;
struct ImageMappingModes
{
    enum Enum {
        Normal = 0, // UV mapping
        Environment = 1,
        LightProbe = 2,
    };
};

struct Q_DEMONRUNTIMERENDER_EXPORT SImage : public SGraphObject
{
    // Complete path to the file;
    //*not* relative to the presentation directory
    QString m_ImagePath;
    QString m_ImageShaderName; ///< for custom materials we don't generate the name

    // Presentation id.
    QString m_OffscreenRendererId; // overrides source path if available
    SRenderPlugin *m_RenderPlugin; // Overrides everything if available.
    QSharedPointer<IOffscreenRenderer> m_LastFrameOffscreenRenderer;
    SGraphObject *m_Parent;

    SImageTextureData m_TextureData;

    NodeFlags m_Flags; // only dirty, transform dirty, and active apply

    QVector2D m_Scale;
    QVector2D m_Pivot;
    float m_Rotation; // Radians.
    QVector2D m_Position;
    ImageMappingModes::Enum m_MappingMode;
    QDemonRenderTextureCoordOp::Enum m_HorizontalTilingMode;
    QDemonRenderTextureCoordOp::Enum m_VerticalTilingMode;

    // Setting any of the above variables means this object is dirty.
    // Setting any of the vec2 properties means this object's transform is dirty

    QMatrix4x4 m_TextureTransform;

    SImage();
    // Renders the sub presentation
    // Or finds the image.
    // and sets up the texture transform
    bool ClearDirty(IBufferManager &inBufferManager, IOffscreenRenderManager &inRenderManager,
                    /*IRenderPluginManager &pluginManager,*/ bool forIbl = false);

    void CalculateTextureTransform();

    // Generic method used during serialization
    // to remap string and object pointers
    template <typename TRemapperType>
    void Remap(TRemapperType &inRemapper)
    {
        SGraphObject::Remap(inRemapper);
        inRemapper.Remap(m_ImagePath);
        inRemapper.Remap(m_OffscreenRendererId);
        // Null out objects that should be null when loading from file.
        inRemapper.NullPtr(m_LastFrameOffscreenRenderer);
        inRemapper.NullPtr(m_TextureData.m_Texture);
        inRemapper.Remap(m_RenderPlugin);
        inRemapper.Remap(m_Parent);
    }
};
QT_END_NAMESPACE

#endif