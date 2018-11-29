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
#ifndef QDEMONN_RENDERQDEMONDER_TYPES_H
#define QDEMONN_RENDERQDEMONDER_TYPES_H

#include <QtDemonRender/qtdemonrenderglobal.h>
#include <QtDemon/QDemonFlags>
#include <QtDemon/QDemonDataRef>
#include <QtDemonRender/qdemonrenderlogging.h>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>
#include <QtGui/QMatrix4x4>
#include <QtGui/QMatrix3x3>
#include <QFloat16>

QT_BEGIN_NAMESPACE

struct QDemonRenderComponentTypes
{
    enum Enum {
        Unknown = 0,
        UnsignedInteger8,
        Integer8,
        UnsignedInteger16,
        Integer16,
        UnsignedInteger32,
        Integer32,
        UnsignedInteger64,
        Integer64,
        Float16,
        Float32,
        Float64
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case UnsignedInteger8:
            return "UnsignedInteger8";
        case Integer8:
            return "Integer8";
        case UnsignedInteger16:
            return "UnsignedInteger16";
        case Integer16:
            return "Integer16";
        case UnsignedInteger32:
            return "UnsignedInteger32";
        case Integer32:
            return "Integer32";
        case UnsignedInteger64:
            return "UnsignedInteger64";
        case Integer64:
            return "Integer64";
        case Float16:
            return "Float16";
        case Float32:
            return "Float32";
        case Float64:
            return "Float64";
        default:
            break;
        }
        return "Unknown";
    }

    static quint32 getSizeofType(Enum value)
    {
        switch (value) {
        case UnsignedInteger8:
            return sizeof(quint8);
        case Integer8:
            return sizeof(qint8);
        case UnsignedInteger16:
            return sizeof(quint16);
        case Integer16:
            return sizeof(qint16);
        case UnsignedInteger32:
            return sizeof(quint32);
        case Integer32:
            return sizeof(qint32);
        case UnsignedInteger64:
            return sizeof(quint64);
        case Integer64:
            return sizeof(qint64);
        case Float16:
            return sizeof(qfloat16);
        case Float32:
            return sizeof(float);
        case Float64:
            return sizeof(double);
        default:
            break;
    }
    Q_ASSERT(false);
    return 0;
}
};

/**
            Define a set of compile-time trait classes that map the enumerations
            to actual compile time types and sizeof so we can deal with the enumeration
            in generic ways.
    */
template <QDemonRenderComponentTypes::Enum TraitType>
struct QDemonRenderComponentTraits
{
    bool force_compile_error;
};

/**
            Define a compile time mapping from datatype to enumeration.  Not that if you
            use this with a type that isn't a component type you will get a compilation
            error.
    */
template <typename TDataType>
struct QDemonRenderComponentTypeToTypeMap
{
    bool force_compile_error;
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger8>
{
    typedef quint8 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger8>()
        : mComponentSize(sizeof(quint8))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<quint8>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<quint8>()
        : m_ComponentType(QDemonRenderComponentTypes::UnsignedInteger8)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer8>
{
    typedef qint8 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer8>()
        : mComponentSize(sizeof(qint8))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<qint8>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<qint8>()
        : m_ComponentType(QDemonRenderComponentTypes::Integer8)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger16>
{
    typedef quint16 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger16>()
        : mComponentSize(sizeof(quint16))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<quint16>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<quint16>()
        : m_ComponentType(QDemonRenderComponentTypes::UnsignedInteger16)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer16>
{
    typedef qint16 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer16>()
        : mComponentSize(sizeof(qint16))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<qint16>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<qint16>()
        : m_ComponentType(QDemonRenderComponentTypes::Integer16)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger32>
{
    typedef quint32 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger32>()
        : mComponentSize(sizeof(quint32))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<quint32>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<quint32>()
        : m_ComponentType(QDemonRenderComponentTypes::UnsignedInteger32)
    { }
};
template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer32>
{
    typedef qint32 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer32>()
        : mComponentSize(sizeof(qint32))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<qint32>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<qint32>()
        : m_ComponentType(QDemonRenderComponentTypes::Integer32)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger64>
{
    typedef quint64 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::UnsignedInteger64>()
        : mComponentSize(sizeof(quint64))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<quint64>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<quint64>()
        : m_ComponentType(QDemonRenderComponentTypes::UnsignedInteger64)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer64>
{
    typedef qint64 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::Integer64>()
        : mComponentSize(sizeof(qint64))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<qint64>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<qint64>()
        : m_ComponentType(QDemonRenderComponentTypes::Integer64)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::Float16>
{
    typedef qfloat16 TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::Float16>()
        : mComponentSize(sizeof(qfloat16))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<qfloat16>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<qfloat16>()
        : m_ComponentType(QDemonRenderComponentTypes::Float16)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::Float32>
{
    typedef float TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::Float32>()
        : mComponentSize(sizeof(float))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<float>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<float>()
        : m_ComponentType(QDemonRenderComponentTypes::Float32)
    { }
};

template <>
struct QDemonRenderComponentTraits<QDemonRenderComponentTypes::Float64>
{
    typedef double TComponentType;
    quint8 mComponentSize;
    QDemonRenderComponentTraits<QDemonRenderComponentTypes::Float64>()
        : mComponentSize(sizeof(double))
    { }
};

template <>
struct QDemonRenderComponentTypeToTypeMap<double>
{
    QDemonRenderComponentTypes::Enum m_ComponentType;
    QDemonRenderComponentTypeToTypeMap<double>()
        : m_ComponentType(QDemonRenderComponentTypes::Float64)
    { }
};


// Map at compile time from component type to datatype;
template <typename TDataType>
inline QDemonRenderComponentTypes::Enum getComponentTypeForType()
{
    return QDemonRenderComponentTypeToTypeMap<TDataType>().m_ComponentType;
}

struct QDemonRenderContextValues
{
    enum Enum {
        GLES2 = 1 << 0,
        GL2 = 1 << 1,
        GLES3 = 1 << 2,
        GL3 = 1 << 3,
        GLES3PLUS = 1 << 4,
        GL4 = 1 << 5,
        NullContext = 1 << 6,
    };
};

typedef QDemonFlags<QDemonRenderContextValues::Enum, quint32> QDemonRenderContextType;

struct QDemonRenderClearValues
{
    enum Enum {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 3,
        Coverage = 1 << 4,
    };
};

typedef QDemonFlags<QDemonRenderClearValues::Enum, quint32> QDemonRenderClearFlags;

struct QDemonRenderQueryType
{
    enum Enum {
        Unknown = 0,
        Samples, ///< samples query object
        Timer, ///< timer query object
    };
};

struct QDemonRenderQueryResultType
{
    enum Enum {
        Unknown = 0,
        ResultAvailable, ///< Check if query result is available
        Result, ///< Get actual result
    };
};

struct QDemonRenderSyncType
{
    enum Enum {
        Unknown = 0,
        GpuCommandsComplete, ///< sync to Gpu commands finished
    };
};

struct QDemonRenderSyncValues
{
    enum Enum {
        Unknown = 0, ///< for future usage
    };
};

typedef QDemonFlags<QDemonRenderSyncValues::Enum, quint32> QDemonRenderSyncFlags;

struct QDemonRenderCommandFlushValues
{
    enum Enum {
        SyncFlushCommands = 0, ///< sync for flushing command
    };
};

typedef QDemonFlags<QDemonRenderCommandFlushValues::Enum, quint32> QDemonRenderCommandFlushFlags;

struct QDemonRenderBufferBindValues
{
    enum Enum {
        Unknown = 0,
        Vertex = 1 << 0, ///< Bind as vertex buffer
        Index = 1 << 1, ///< Bind as index buffer
        Constant = 1 << 2, ///< Bind as constant buffer
        Storage = 1 << 3, ///< Bind as shader storage buffer
        Atomic_Counter = 1 << 4, ///< Bind as atomic counter buffer
        Draw_Indirect = 1 << 5, ///< Bind as draw indirect buffer
    };
};

typedef QDemonFlags<QDemonRenderBufferBindValues::Enum, quint32> QDemonRenderBufferBindFlags;

struct QDemonRenderBufferUsageType
{
    enum Enum {
        Unknown = 0,
        Static, ///< Rarely updated
        Dynamic, ///< Most likely updated every frame
    };
};

struct QDemonRenderImageAccessType
{
    enum Enum {
        Unknown = 0,
        Read, ///< Read only access
        Write, ///< Write only access
        ReadWrite, ///< Read and write access
    };
};

struct QDemonRenderBufferAccessTypeValues
{
    enum Enum {
        Unknown = 0,
        Read = 1 << 0, ///< Read access
        Write = 1 << 1, ///< Write access
        Invalid = 1 << 2, ///< No sync
        InvalidRange = 1 << 3, ///< No sync

    };
};

typedef QDemonFlags<QDemonRenderBufferAccessTypeValues::Enum, quint32> QDemonRenderBufferAccessFlags;

///< defines a barrier of ordering the memory transactions to a command relative to those issued
///before the barrier
struct QDemonRenderBufferBarrierValues
{
    enum Enum {
        Unknown = 0,
        VertexAttribArray = 1 << 0, ///< Barrier for vertex attributes sourced from a buffer
        ElementArray = 1 << 1, ///< Barrier for indices sourced from a buffer
        UniformBuffer = 1 << 2, ///< Barrier for shader uniforms sourced from a buffer
        TextureFetch = 1 << 3, ///< Barrier for texture fetches within shaders
        ShaderImageAccess = 1 << 4, ///< Barrier for image access using load / store
        CommandBuffer = 1 << 5, ///< Barrier for indirect drawing
        PixelBuffer = 1 << 6, ///< Barrier for pixel buffer access
        TextureUpdate = 1 << 7, ///< Barrier for texture writes
        BufferUpdate = 1 << 8, ///< Barrier for buffer writes
        Framebuffer = 1 << 9, ///< Barrier for framebuffer writes
        TransformFeedback = 1 << 10, ///< Barrier for transform feedback writes
        AtomicCounter = 1 << 11, ///< Barrier for atomic counter writes
        ShaderStorage = 1 << 12, ///< Barrier for shader storage blocks writes
        All = 0xFFFF, ///< Barrier for all of the above
    };
};

typedef QDemonFlags<QDemonRenderBufferBarrierValues::Enum, quint32> QDemonRenderBufferBarrierFlags;

struct QDemonRenderRenderBufferFormats
{
    enum Enum {
        Unknown = 0,
        RGBA4,
        RGB565,
        RGBA5551,
        Depth16,
        Depth24,
        Depth32,
        StencilIndex8,
        CoverageNV
    };

    static const char *toString(Enum value)
    {
        switch (value) {
        case RGBA4:
            return "RGBA4";
        case RGB565:
            return "RGB565";
        case RGBA5551:
            return "RGBA5551";
        case Depth16:
            return "Depth16";
        case Depth24:
            return "Depth24";
        case Depth32:
            return "Depth32";
        case StencilIndex8:
            return "StencilIndex8";
        case CoverageNV:
            return "CoverageNV";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderTextureFormats
{
    enum Enum {
        Unknown = 0,
        R8,
        R16,
        R16F,
        R32I,
        R32UI,
        R32F,
        RG8,
        RGBA8,
        RGB8,
        SRGB8,
        SRGB8A8,
        RGB565,
        RGBA5551,
        Alpha8,
        Luminance8,
        Luminance16,
        LuminanceAlpha8,
        RGBA16F,
        RG16F,
        RG32F,
        RGB32F,
        RGBA32F,
        R11G11B10,
        RGB9E5,
        RGBA_DXT1,
        RGB_DXT1,
        RGBA_DXT3,
        RGBA_DXT5,
        Depth16,
        Depth24,
        Depth32,
        Depth24Stencil8
    };

    static bool isUncompressedTextureFormat(QDemonRenderTextureFormats::Enum value)
    {
        switch (value) {
        case QDemonRenderTextureFormats::R8:
            return true;
        case QDemonRenderTextureFormats::R16:
            return true;
        case QDemonRenderTextureFormats::R16F:
            return true;
        case QDemonRenderTextureFormats::R32I:
            return true;
        case QDemonRenderTextureFormats::R32UI:
            return true;
        case QDemonRenderTextureFormats::R32F:
            return true;
        case QDemonRenderTextureFormats::RG8:
            return true;
        case QDemonRenderTextureFormats::RGBA8:
            return true;
        case QDemonRenderTextureFormats::RGB8:
            return true;
        case QDemonRenderTextureFormats::SRGB8:
            return true;
        case QDemonRenderTextureFormats::SRGB8A8:
            return true;
        case QDemonRenderTextureFormats::RGB565:
            return true;
        case QDemonRenderTextureFormats::RGBA5551:
            return true;
        case QDemonRenderTextureFormats::Alpha8:
            return true;
        case QDemonRenderTextureFormats::Luminance8:
            return true;
        case QDemonRenderTextureFormats::Luminance16:
            return true;
        case QDemonRenderTextureFormats::LuminanceAlpha8:
            return true;
        case QDemonRenderTextureFormats::RGBA16F:
            return true;
        case QDemonRenderTextureFormats::RG16F:
            return true;
        case QDemonRenderTextureFormats::RG32F:
            return true;
        case QDemonRenderTextureFormats::RGB32F:
            return true;
        case QDemonRenderTextureFormats::RGBA32F:
            return true;
        case QDemonRenderTextureFormats::R11G11B10:
            return true;
        case QDemonRenderTextureFormats::RGB9E5:
            return true;
                default:
            break;
        }
        return false;
    }

    static bool isCompressedTextureFormat(QDemonRenderTextureFormats::Enum value)
    {
        switch (value) {
        case QDemonRenderTextureFormats::RGBA_DXT1:
            return true;
        case QDemonRenderTextureFormats::RGB_DXT1:
            return true;
        case QDemonRenderTextureFormats::RGBA_DXT3:
            return true;
        case QDemonRenderTextureFormats::RGBA_DXT5:
            return true;
        default:
            break;
        }
        return false;
    }

    static bool isDepthTextureFormat(QDemonRenderTextureFormats::Enum value)
    {
        switch (value) {
        case QDemonRenderTextureFormats::Depth16:
        return true;
        case QDemonRenderTextureFormats::Depth24:
            return true;
        case QDemonRenderTextureFormats::Depth32:
            return true;
        case QDemonRenderTextureFormats::Depth24Stencil8:
            return true;
        default:
            break;
        }
        return false;
    }

    static const char *toString(Enum value)
    {
        switch (value) {
        case R8:
            return "R8";
        case R16:
            return "R16";
        case R16F:
            return "R16F";
        case R32I:
            return "R32I";
        case R32UI:
            return "R32UI";
        case R32F:
            return "R32F";
        case RG8:
            return "RG8";
        case RGBA8:
            return "RGBA8";
        case RGB8:
            return "RGB8";
        case SRGB8:
            return "SRGB8";
        case SRGB8A8:
            return "SRGB8A8";
        case RGB565:
            return "RGB565";
        case RGBA5551:
            return "RGBA5551";
        case Alpha8:
            return "Alpha8";
        case Luminance8:
            return "Luminance8";
        case Luminance16:
            return "Luminance16";
        case LuminanceAlpha8:
            return "LuminanceAlpha8";
        case RGBA16F:
            return "RGBA16F";
        case RG16F:
            return "RG16F";
        case RG32F:
            return "RG32F";
        case RGB32F:
            return "RGB32F";
        case RGBA32F:
            return "RGBA32F";
        case R11G11B10:
            return "R11G11B10";
        case RGB9E5:
            return "RGB9E5";
        case RGBA_DXT1:
            return "RGBA_DXT1";
        case RGB_DXT1:
            return "RGB_DXT1";
        case RGBA_DXT3:
            return "RGBA_DXT3";
        case RGBA_DXT5:
            return "RGBA_DXT5";
        case Depth16:
            return "Depth16";
        case Depth24:
            return "Depth24";
        case Depth32:
            return "Depth32";
        case Depth24Stencil8:
            return "Depth24Stencil8";
        default:
            break;
        }
        return "Unknown";
    }

    static quint32 getSizeofFormat(Enum value)
    {
        switch (value) {
        case R8:
            return 1;
        case R16F:
            return 2;
        case R16:
            return 2;
        case R32I:
            return 4;
        case R32F:
            return 4;
        case RGBA8:
            return 4;
        case RGB8:
            return 3;
        case RGB565:
            return 2;
        case RGBA5551:
            return 2;
        case Alpha8:
            return 1;
        case Luminance8:
            return 1;
        case LuminanceAlpha8:
            return 1;
        case Depth16:
            return 2;
        case Depth24:
            return 3;
        case Depth32:
            return 4;
        case Depth24Stencil8:
            return 4;
        case RGB9E5:
            return 4;
        case SRGB8:
            return 3;
        case SRGB8A8:
            return 4;
        case RGBA16F:
            return 8;
        case RG16F:
            return 4;
        case RG32F:
            return 8;
        case RGBA32F:
            return 16;
        case RGB32F:
            return 12;
        case R11G11B10:
            return 4;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static quint32 getNumberOfComponent(Enum value)
    {
        switch (value) {
        case R8:
            return 1;
        case R16F:
            return 1;
        case R16:
            return 1;
        case R32I:
            return 1;
        case R32F:
            return 1;
        case RGBA8:
            return 4;
        case RGB8:
            return 3;
        case RGB565:
            return 3;
        case RGBA5551:
            return 4;
        case Alpha8:
            return 1;
        case Luminance8:
            return 1;
        case LuminanceAlpha8:
            return 2;
        case Depth16:
            return 1;
        case Depth24:
            return 1;
        case Depth32:
            return 1;
        case Depth24Stencil8:
            return 2;
        case RGB9E5:
            return 3;
        case SRGB8:
            return 3;
        case SRGB8A8:
            return 4;
        case RGBA16F:
            return 4;
        case RG16F:
            return 2;
        case RG32F:
            return 2;
        case RGBA32F:
            return 4;
        case RGB32F:
            return 3;
        case R11G11B10:
            return 3;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static void decodeToFloat(void *inPtr, quint32 byteOfs, float *outPtr,
                              QDemonRenderTextureFormats::Enum inFmt)
    {
        outPtr[0] = 0.0f;
        outPtr[1] = 0.0f;
        outPtr[2] = 0.0f;
        outPtr[3] = 0.0f;
        quint8 *src = reinterpret_cast<quint8 *>(inPtr);
        // float divisor;		// If we want to support RGBD?
        switch (inFmt) {
        case Alpha8:
            outPtr[0] = (float(src[byteOfs])) / 255.0f;
            break;

        case Luminance8:
        case LuminanceAlpha8:
        case R8:
        case RG8:
        case RGB8:
        case RGBA8:
        case SRGB8:
        case SRGB8A8:
            // NOTE : RGBD Hack here for reference.  Not meant for installation.
            // divisor = (QDemonRenderTextureFormats::getSizeofFormat(inFmt) == 4) ?
            // ((float)src[byteOfs+3]) / 255.0f : 1.0f;
            for (quint32 i = 0; i < QDemonRenderTextureFormats::getSizeofFormat(inFmt); ++i) {
                float val = (float(src[byteOfs + i])) / 255.0f;
                outPtr[i] = (i < 3) ? powf(val, 0.4545454545f) : val;
                // Assuming RGBA8 actually means RGBD (which is stupid, I know)
                // if ( QDemonRenderTextureFormats::getSizeofFormat(inFmt) == 4 ) { outPtr[i] /=
                // divisor; }
            }
            // outPtr[3] = divisor;
            break;

        case R32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            break;
        case RG32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
            break;
        case RGBA32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
            outPtr[2] = reinterpret_cast<float *>(src + byteOfs)[2];
            outPtr[3] = reinterpret_cast<float *>(src + byteOfs)[3];
            break;
        case RGB32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
            outPtr[2] = reinterpret_cast<float *>(src + byteOfs)[2];
            break;

        case R16F:
        case RG16F:
        case RGBA16F:
            for (quint32 i = 0; i < (QDemonRenderTextureFormats::getSizeofFormat(inFmt) >> 1); ++i) {
                // NOTE : This only works on the assumption that we don't have any denormals,
                // Infs or NaNs.
                // Every pixel in our source image should be "regular"
                quint16 h = reinterpret_cast<quint16 *>(src + byteOfs)[i];
                quint32 sign = (h & 0x8000u) << 16u;
                quint32 exponent = (((((h & 0x7c00u) >> 10) - 15) + 127) << 23);
                quint32 mantissa = ((h & 0x3ffu) << 13);
                quint32 result = sign | exponent | mantissa;

                if (h == 0 || h == 0x8000) {
                    result = 0;
                } // Special case for zero and negative zero
                memcpy(reinterpret_cast<quint32 *>(outPtr) + i, &result, 4);
            }
            break;

        case R11G11B10:
            // place holder
            Q_ASSERT(false);
            break;

        default:
            outPtr[0] = 0.0f;
            outPtr[1] = 0.0f;
            outPtr[2] = 0.0f;
            outPtr[3] = 0.0f;
            break;
        }
    }

    static void encodeToPixel(float *inPtr, void *outPtr, quint32 byteOfs,
                              QDemonRenderTextureFormats::Enum inFmt)
    {
        quint8 *dest = reinterpret_cast<quint8 *>(outPtr);
        switch (inFmt) {
        case QDemonRenderTextureFormats::Alpha8:
            dest[byteOfs] = quint8(inPtr[0] * 255.0f);
            break;

        case Luminance8:
        case LuminanceAlpha8:
        case R8:
        case RG8:
        case RGB8:
        case RGBA8:
        case SRGB8:
        case SRGB8A8:
            for (quint32 i = 0; i < QDemonRenderTextureFormats::getSizeofFormat(inFmt); ++i) {
                inPtr[i] = (inPtr[i] > 1.0f) ? 1.0f : inPtr[i];
                if (i < 3)
                    dest[byteOfs + i] = quint8(powf(inPtr[i], 2.2f) * 255.0f);
                else
                    dest[byteOfs + i] = quint8(inPtr[i] * 255.0f);
            }
            break;

        case R32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            break;
        case RG32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
            break;
        case RGBA32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
            reinterpret_cast<float *>(dest + byteOfs)[2] = inPtr[2];
            reinterpret_cast<float *>(dest + byteOfs)[3] = inPtr[3];
            break;
        case RGB32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
            reinterpret_cast<float *>(dest + byteOfs)[2] = inPtr[2];
            break;

        case R16F:
        case RG16F:
        case RGBA16F:
            for (quint32 i = 0; i < (QDemonRenderTextureFormats::getSizeofFormat(inFmt) >> 1); ++i) {
                // NOTE : This also has the limitation of not handling  infs, NaNs and
                // denormals, but it should be
                // sufficient for our purposes.
                if (inPtr[i] > 65519.0f) {
                    inPtr[i] = 65519.0f;
                }
                if (fabs(inPtr[i]) < 6.10352E-5f) {
                    inPtr[i] = 0.0f;
                }
                quint32 f = reinterpret_cast<quint32 *>(inPtr)[i];
                quint32 sign = (f & 0x80000000) >> 16;
                qint32 exponent = (f & 0x7f800000) >> 23;
                quint32 mantissa = (f >> 13) & 0x3ff;
                exponent = exponent - 112;
                if (exponent > 31) {
                    exponent = 31;
                }
                if (exponent < 0) {
                    exponent = 0;
                }
                exponent = exponent << 10;
                reinterpret_cast<quint16 *>(dest + byteOfs)[i] =
                        quint16(sign | quint32(exponent) | mantissa);
            }
            break;

        case R11G11B10:
            // place holder
            Q_ASSERT(false);
            break;

        default:
            dest[byteOfs] = 0;
            dest[byteOfs + 1] = 0;
            dest[byteOfs + 2] = 0;
            dest[byteOfs + 3] = 0;
            break;
        }
    }
};

struct QDemonRenderTextureTargetType
{
    enum Enum {
        Unknown = 0,
        Texture2D,
        Texture2D_MS,
        Texture2D_Array,
        TextureCube,
        TextureCubePosX,
        TextureCubeNegX,
        TextureCubePosY,
        TextureCubeNegY,
        TextureCubePosZ,
        TextureCubeNegZ,
    };
};

struct QDemonRenderTextureUnit
{
    enum Enum {
        TextureUnit_0 = 0,
        TextureUnit_1,
        TextureUnit_2,
        TextureUnit_3,
        TextureUnit_4,
        TextureUnit_5,
        TextureUnit_6,
        TextureUnit_7,
        TextureUnit_8,
        TextureUnit_9,
        TextureUnit_10,
        TextureUnit_11,
        TextureUnit_12,
        TextureUnit_13,
        TextureUnit_14,
        TextureUnit_15,
        TextureUnit_16,
        TextureUnit_17,
        TextureUnit_18,
        TextureUnit_19,
        TextureUnit_20,
        TextureUnit_21,
        TextureUnit_22,
        TextureUnit_23,
        TextureUnit_24,
        TextureUnit_25,
        TextureUnit_26,
        TextureUnit_27,
        TextureUnit_28,
        TextureUnit_29,
        TextureUnit_30,
        TextureUnit_31
    };
};

struct QDemonRenderTextureCompareMode
{
    enum Enum {
        Unknown = 0,
        NoCompare,
        CompareToRef
    };
};

struct QDemonRenderTextureSwizzleMode
{
    enum Enum {
        NoSwizzle = 0,
        L8toR8,
        A8toR8,
        L8A8toRG8,
        L16toR16
    };
};

struct QDemonRenderTextureCompareOp
{
    enum Enum {
        Never,
        Less,
        LessThanOrEqual,
        Equal,
        NotEqual,
        Greater,
        GreaterThanOrEqual,
        AlwaysTrue,
    };
};


struct QDemonRenderTextureMinifyingOp
{
    enum Enum {
        Unknown = 0,
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };
    const char *toString(QDemonRenderTextureMinifyingOp::Enum value)
    {
        switch (value) {
        case Nearest:
            return "Nearest";
        case Linear:
            return "Linear";
        case NearestMipmapNearest:
            return "NearestMipmapNearest";
        case LinearMipmapNearest:
            return "LinearMipmapNearest";
        case NearestMipmapLinear:
            return "NearestMipmapLinear";
        case LinearMipmapLinear:
            return "LinearMipmapLinear";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderTextureMagnifyingOp
{
    enum Enum {
        Unknown = 0,
        Nearest,
        Linear
    };
    const char *toString(QDemonRenderTextureMinifyingOp::Enum value)
    {
        switch (value) {
        case QDemonRenderTextureMinifyingOp::Nearest:
            return "Nearest";
        case QDemonRenderTextureMinifyingOp::Linear:
            return "Linear";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderTextureCoordOp
{
    enum Enum {
        Unknown = 0,
        ClampToEdge,
        MirroredRepeat,
        Repeat
    };
    const char *toString(Enum value)
    {
        switch (value) {
        case ClampToEdge:
            return "ClampToEdge";
        case MirroredRepeat:
            return "MirroredRepeat";
        case Repeat:
            return "Repeat";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderHint
{
    enum Enum {
        Unknown = 0,
        Fastest,
        Nicest,
        Unspecified
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case Fastest:
            return "Fastest";
        case Nicest:
            return "Nicest";
        case Unspecified:
            return "Unspecified";
        default:
            break;
        }
        return "Unknown";
    }
};


class QDemonRenderImplemented
{
protected:
    virtual ~QDemonRenderImplemented() {}
public:
    // Get the handle that binds us to the implementation.
    // For instance, return the GLuint that came back from
    // glGenTextures.
    virtual const void *GetImplementationHandle() const = 0;
};

struct QDemonRenderVertexBufferEntry
{
    const char *m_Name;
    /** Datatype of the this entry points to in the buffer */
    QDemonRenderComponentTypes::Enum m_ComponentType;
    /** Number of components of each data member. 1,2,3, or 4.  Don't be stupid.*/
    quint32 m_NumComponents;
    /** Offset from the beginning of the buffer of the first item */
    quint32 m_FirstItemOffset;
    /** Attribute input slot used for this entry*/
    quint32 m_InputSlot;

    QDemonRenderVertexBufferEntry(const char *nm, QDemonRenderComponentTypes::Enum type,
                                  quint32 numComponents, quint32 firstItemOffset = 0,
                                  quint32 inputSlot = 0)
        : m_Name(nm)
        , m_ComponentType(type)
        , m_NumComponents(numComponents)
        , m_FirstItemOffset(firstItemOffset)
        , m_InputSlot(inputSlot)
    {
    }

    QDemonRenderVertexBufferEntry()
        : m_Name(nullptr)
        , m_ComponentType(QDemonRenderComponentTypes::Unknown)
        , m_NumComponents(0)
        , m_FirstItemOffset(0)
        , m_InputSlot(0)
    {
    }

    QDemonRenderVertexBufferEntry(const QDemonRenderVertexBufferEntry &inOther)
        : m_Name(inOther.m_Name)
        , m_ComponentType(inOther.m_ComponentType)
        , m_NumComponents(inOther.m_NumComponents)
        , m_FirstItemOffset(inOther.m_FirstItemOffset)
        , m_InputSlot(inOther.m_InputSlot)
    {
    }

    QDemonRenderVertexBufferEntry &operator=(const QDemonRenderVertexBufferEntry &inOther)
    {
        if (this != &inOther) {
            m_Name = inOther.m_Name;
            m_ComponentType = inOther.m_ComponentType;
            m_NumComponents = inOther.m_NumComponents;
            m_FirstItemOffset = inOther.m_FirstItemOffset;
            m_InputSlot = inOther.m_InputSlot;
        }
        return *this;
    }
};

class QDemonRenderShaderProgram;

//typedef QDemonConstDataRef<qint8> TConstI8Ref;

struct QDemonRenderVertFragCompilationResult
{
    const char *mShaderName;

    QSharedPointer<QDemonRenderShaderProgram> mShader; ///< contains the program

    QDemonRenderVertFragCompilationResult()
        : mShaderName("")
        , mShader(nullptr)
    {
    }
};

struct QDemonRenderFrameBufferAttachments
{
    enum Enum {
        Unknown = 0,
        Color0,
        Color1,
        Color2,
        Color3,
        Color4,
        Color5,
        Color6,
        Color7,
        Depth,
        Stencil,
        DepthStencil,
        CoverageNV,
        LastAttachment,
    };
};


struct QDemonRenderDrawMode
{
    enum Enum {
        Unknown = 0,
        Points,
        LineStrip,
        LineLoop,
        Lines,
        TriangleStrip,
        TriangleFan,
        Triangles,
        Patches,
    };
};

struct QDemonRenderTextureCubeFaces
{
    enum Enum {
        InvalidFace = 0,
        CubePosX = 1,
        CubeNegX,
        CubePosY,
        CubeNegY,
        CubePosZ,
        CubeNegZ,
    };
};

// enums match the NV path extensions
struct QDemonRenderPathCommands
{
    enum Enum {
        Close = 0,
        MoveTo = 2,
        CubicCurveTo = 12,
    };
};

struct QDemonRenderPathFontTarget
{
    enum Enum {
        StandardFont = 0,
        SystemFont = 1,
        FileFont = 2,
    };
};

struct QDemonRenderPathMissingGlyphs
{
    enum Enum {
        SkipMissing = 0,
        UseMissing = 1,
    };
};

struct QDemonRenderPathFontStyleValues
{
    enum Enum {
        Bold = 1 << 0,
        Italic = 1 << 1,
    };
};

typedef QDemonFlags<QDemonRenderPathFontStyleValues::Enum, quint32> QDemonRenderPathFontStyleFlags;

struct QDemonRenderPathReturnValues
{
    enum Enum {
        FontGlypsAvailable = 0,
        FontTargetUnavailable = 1,
        FontUnavailable = 2,
        FontUnintelligible = 3,
        InvalidEnum = 4,
        OutOfMemory = 5,
    };
};

struct QDemonRenderPathFormatType
{
    enum Enum {
        Byte = 1,
        UByte,
        Short,
        UShort,
        Int,
        Uint,
        Float,
        Utf8,
        Utf16,
        Bytes2,
        Bytes3,
        Bytes4,
    };
};

struct QDemonRenderPathGlyphFontMetricValues
{
    enum Enum {
        GlyphWidth = 1 << 0,
        GlyphHeight = 1 << 1,
        GlyphHorizontalBearingX = 1 << 2,
        GlyphHorizontalBearingY = 1 << 3,
        GlyphHorizontalBearingAdvance = 1 << 4,
        GlyphVerticalBearingX = 1 << 5,
        GlyphVerticalBearingY = 1 << 6,
        GlyphVerticalBearingAdvance = 1 << 7,
        GlyphHasKerning = 1 << 8,

        FontXMinBounds = 1 << 9,
        FontYMinBounds = 1 << 10,
        FontXMaxBounds = 1 << 11,
        FontYMaxBounds = 1 << 12,
        FontUnitsPerEm = 1 << 13,
        FontAscender = 1 << 14,
        FontDescender = 1 << 15,
        FontHeight = 1 << 16,
        FontMaxAdvanceWidth = 1 << 17,
        FontMaxAdvanceHeight = 1 << 18,
        FontUnderlinePosition = 1 << 19,
        FontUnderlineThickness = 1 << 20,
        FontHasKerning = 1 << 21,
        FontNumGlyphIndices = 1 << 22,
    };
};

typedef QDemonFlags<QDemonRenderPathGlyphFontMetricValues::Enum, quint32> QDemonRenderPathGlyphFontMetricFlags;

struct QDemonRenderPathListMode
{
    enum Enum {
        AccumAdjacentPairs = 1,
        AdjacentPairs,
        FirstToRest,
    };
};

struct QDemonRenderPathFillMode
{
    enum Enum {
        Fill = 1,
        CountUp,
        CountDown,
        Invert,
    };
};

struct QDemonRenderPathCoverMode
{
    enum Enum {
        ConvexHull = 1,
        BoundingBox,
        BoundingBoxOfBoundingBox,
        PathFillCover,
        PathStrokeCover,
    };
};

struct QDemonRenderPathTransformType
{
    enum Enum {
        NoTransform = 0,
        TranslateX,
        TranslateY,
        Translate2D,
        Translate3D,
        Affine2D,
        Affine3D,
        TransposeAffine2D,
        TransposeAffine3D,
    };
};

struct QDemonRenderWinding
{
    enum Enum {
        Unknown = 0,
        Clockwise,
        CounterClockwise
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case Clockwise:
            return "Clockwise";
        case CounterClockwise:
            return "CounterClockwise";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderState
{
    enum Enum {
        Unknown = 0,
        Blend,
        CullFace,
        DepthTest,
        StencilTest,
        ScissorTest,
        DepthWrite,
        Multisample
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case Blend:
            return "Blend";
        case CullFace:
            return "CullFace";
        case DepthTest:
            return "DepthTest";
        case StencilTest:
            return "StencilTest";
        case ScissorTest:
            return "ScissorTest";
        case DepthWrite:
            return "DepthWrite";
        case Multisample:
            return "Multisample";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderSrcBlendFunc
{
    enum Enum {
        Unknown = 0,
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate
    };

    static const char *toString(Enum value)
    {
        switch (value) {
        case Zero:
            return "Zero";
        case One:
            return "One";
        case SrcColor:
            return "SrcColor";
        case OneMinusSrcColor:
            return "OneMinusSrcColor";
        case DstColor:
            return "DstColor";
        case OneMinusDstColor:
            return "OneMinusDstColor";
        case SrcAlpha:
            return "SrcAlpha";
        case OneMinusSrcAlpha:
            return "OneMinusSrcAlpha";
        case DstAlpha:
            return "DstAlpha";
        case OneMinusDstAlpha:
            return "OneMinusDstAlpha";
        case ConstantColor:
            return "ConstantColor";
        case OneMinusConstantColor:
            return "OneMinusConstantColor";
        case ConstantAlpha:
            return "ConstantAlpha";
        case OneMinusConstantAlpha:
            return "OneMinusConstantAlpha";
        case SrcAlphaSaturate:
            return "SrcAlphaSaturate";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderDstBlendFunc
{
    enum Enum {
        Unknown = 0,
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha
    };

    static const char *toString(Enum value)
    {
        return QDemonRenderSrcBlendFunc::toString(static_cast<QDemonRenderSrcBlendFunc::Enum>(value));
    }
};

struct QDemonRenderBlendEquation
{
    enum Enum {
        Unknown = 0,
        Add,
        Subtract,
        ReverseSubtract,
        Overlay,
        ColorBurn,
        ColorDodge
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case Add:
            return "Add";
        case Subtract:
            return "Subtract";
        case ReverseSubtract:
            return "ReverseSubtract";
        case Overlay:
            return "Overlay";
        case ColorBurn:
            return "ColorBurn";
        case ColorDodge:
            return "ColorDodge";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderFaces
{
    enum Enum {
        Unknown = 0,
        Front,
        Back,
        FrontAndBack
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case Front:
            return "Front";
        case Back:
            return "Back";
        case FrontAndBack:
            return "FrontAndBack";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonReadFaces
{
    enum Enum {
        Unknown = 0,
        Front,
        Back,
        Color0,
        Color1,
        Color2,
        Color3,
        Color4,
        Color5,
        Color6,
        Color7
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case Front:
            return "Front";
        case Back:
            return "Back";
        case Color0:
            return "Color0";
        case Color1:
            return "Color1";
        case Color2:
            return "Color2";
        case Color3:
            return "Color3";
        case Color4:
            return "Color4";
        case Color5:
            return "Color5";
        case Color6:
            return "Color6";
        case Color7:
            return "Color7";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderBoolOp
{
    enum Enum {
        Unknown = 0,
        Never,
        Less,
        LessThanOrEqual,
        Equal,
        NotEqual,
        Greater,
        GreaterThanOrEqual,
        AlwaysTrue
    };

    static const char *toString(Enum value)
    {
        switch (value) {
        case Never:
            return "Never";
        case Less:
            return "Less";
        case LessThanOrEqual:
            return "LessThanOrEqual";
        case Equal:
            return "Equal";
        case NotEqual:
            return "NotEqual";
        case Greater:
            return "Greater";
        case GreaterThanOrEqual:
            return "GreaterThanOrEqual";
        case AlwaysTrue:
            return "AlwaysTrue";
        default:
            break;
        }
        return "Unknown";
    }
};

#define QDEMON_RENDER_ITERATE_STENCIL_OP                                                               \
    QDEMON_RENDER_HANDLE_STENCIL_OP(Keep)                                                              \
    QDEMON_RENDER_HANDLE_STENCIL_OP(Zero)                                                              \
    QDEMON_RENDER_HANDLE_STENCIL_OP(Replace)                                                           \
    QDEMON_RENDER_HANDLE_STENCIL_OP(Increment)                                                         \
    QDEMON_RENDER_HANDLE_STENCIL_OP(IncrementWrap)                                                     \
    QDEMON_RENDER_HANDLE_STENCIL_OP(Decrement)                                                         \
    QDEMON_RENDER_HANDLE_STENCIL_OP(DecrementWrap)                                                     \
    QDEMON_RENDER_HANDLE_STENCIL_OP(Invert)

struct QDemonRenderStencilOp
{
    enum Enum {
        Unknown = 0,
        Keep,
        Zero,
        Replace,
        Increment,
        IncrementWrap,
        Decrement,
        DecrementWrap,
        Invert
    };
    static const char *toString(Enum value)
    {
        switch (value) {
        case Keep:
            return "Keep";
        case Zero:
            return "Zero";
        case Replace:
            return "Replace";
        case Increment:
            return "Increment";
        case IncrementWrap:
            return "IncrementWrap";
        case Decrement:
            return "Decrement";
        case DecrementWrap:
            return "DecrementWrap";
        case Invert:
            return "Invert";
        default:
            break;
        }
        return "Unknown";
    }
};

struct QDemonRenderBlendFunctionArgument
{
    QDemonRenderSrcBlendFunc::Enum m_SrcRGB;
    QDemonRenderDstBlendFunc::Enum m_DstRGB;
    QDemonRenderSrcBlendFunc::Enum m_SrcAlpha;
    QDemonRenderDstBlendFunc::Enum m_DstAlpha;

    QDemonRenderBlendFunctionArgument(QDemonRenderSrcBlendFunc::Enum srcRGB,
                                      QDemonRenderDstBlendFunc::Enum dstRGB,
                                      QDemonRenderSrcBlendFunc::Enum srcAlpha,
                                      QDemonRenderDstBlendFunc::Enum dstAlpha)
        : m_SrcRGB(srcRGB)
        , m_DstRGB(dstRGB)
        , m_SrcAlpha(srcAlpha)
        , m_DstAlpha(dstAlpha)
    {
    }

    // Default blend system premultiplies values.
    QDemonRenderBlendFunctionArgument()
        : m_SrcRGB(QDemonRenderSrcBlendFunc::SrcAlpha)
        , m_DstRGB(QDemonRenderDstBlendFunc::OneMinusSrcAlpha)
        , m_SrcAlpha(QDemonRenderSrcBlendFunc::One)
        , m_DstAlpha(QDemonRenderDstBlendFunc::OneMinusSrcAlpha)
    {
    }
};

struct QDemonRenderBlendEquationArgument
{
    QDemonRenderBlendEquation::Enum m_RGBEquation;
    QDemonRenderBlendEquation::Enum m_AlphaEquation;

    QDemonRenderBlendEquationArgument(QDemonRenderBlendEquation::Enum rgb,
                                      QDemonRenderBlendEquation::Enum alpha)
        : m_RGBEquation(rgb)
        , m_AlphaEquation(alpha)
    {
    }
    QDemonRenderBlendEquationArgument()
        : m_RGBEquation(QDemonRenderBlendEquation::Add)
        , m_AlphaEquation(QDemonRenderBlendEquation::Add)
    {
    }
};

struct QDemonRenderStencilOperationArgument
{
    QDemonRenderStencilOp::Enum m_StencilFail; // What happens when stencil test fails.
    // These values assume the stencil passed
    QDemonRenderStencilOp::Enum
    m_DepthFail; // What happens when the stencil passes but depth test fail.
    QDemonRenderStencilOp::Enum m_DepthPass; // What happens when the stencil and depth tests pass.

    QDemonRenderStencilOperationArgument(QDemonRenderStencilOp::Enum fail,
                                         QDemonRenderStencilOp::Enum depthFail,
                                         QDemonRenderStencilOp::Enum depthPass)
        : m_StencilFail(fail)
        , m_DepthFail(depthFail)
        , m_DepthPass(depthPass)
    {
    }
    QDemonRenderStencilOperationArgument()
        : m_StencilFail(QDemonRenderStencilOp::Keep)
        , m_DepthFail(QDemonRenderStencilOp::Keep)
        , m_DepthPass(QDemonRenderStencilOp::Keep)
    {
    }
    QDemonRenderStencilOperationArgument(const QDemonRenderStencilOperationArgument &StencilOp)
        : m_StencilFail(StencilOp.m_StencilFail)
        , m_DepthFail(StencilOp.m_DepthFail)
        , m_DepthPass(StencilOp.m_DepthPass)
    {
    }

    QDemonRenderStencilOperationArgument &operator=(const QDemonRenderStencilOperationArgument &rhs)
    {
        // Check for self-assignment!
        if (this == &rhs)
            return *this;

        m_StencilFail = rhs.m_StencilFail;
        m_DepthFail = rhs.m_DepthFail;
        m_DepthPass = rhs.m_DepthPass;

        return *this;
    }

    bool operator==(const QDemonRenderStencilOperationArgument &other) const
    {
        return (m_StencilFail == other.m_StencilFail && m_DepthFail == other.m_DepthFail
                && m_DepthPass == other.m_DepthPass);
    }
};

// see glStencilFuncSeparate
struct QDemonRenderStencilFunctionArgument
{
    QDemonRenderBoolOp::Enum m_Function;
    quint32 m_ReferenceValue;
    quint32 m_Mask;

    QDemonRenderStencilFunctionArgument(QDemonRenderBoolOp::Enum function, quint32 referenceValue,
                                        quint32 mask)
        : m_Function(function)
        , m_ReferenceValue(referenceValue)
        , m_Mask(mask)
    {
    }
    QDemonRenderStencilFunctionArgument()
        : m_Function(QDemonRenderBoolOp::AlwaysTrue)
        , m_ReferenceValue(0)
        , m_Mask(quint32(-1))
    {
    }
    QDemonRenderStencilFunctionArgument(const QDemonRenderStencilFunctionArgument &StencilFunc)
        : m_Function(StencilFunc.m_Function)
        , m_ReferenceValue(StencilFunc.m_ReferenceValue)
        , m_Mask(StencilFunc.m_Mask)
    {
    }

    QDemonRenderStencilFunctionArgument &operator=(const QDemonRenderStencilFunctionArgument &rhs)
    {
        // Check for self-assignment!
        if (this == &rhs)
            return *this;

        m_Function = rhs.m_Function;
        m_ReferenceValue = rhs.m_ReferenceValue;
        m_Mask = rhs.m_Mask;

        return *this;
    }

    bool operator==(const QDemonRenderStencilFunctionArgument &other) const
    {
        return (m_Function == other.m_Function && m_ReferenceValue == other.m_ReferenceValue
                && m_Mask == other.m_Mask);
    }
};

class QDemonRenderFrameBuffer;
class QDemonRenderVertexBuffer;
class QDemonRenderIndexBuffer;
class QDemonRenderShaderProgram;
class QDemonRenderProgramPipeline;
class QDemonRenderTextureBase;
class QDemonRenderTexture2D;
class QDemonRenderTexture2DArray;
class QDemonRenderTextureCube;
class QDemonRenderImage2D;
class QDemonRenderDataBuffer;
class QDemonRenderAttribLayout;
class QDemonRenderInputAssembler;

typedef QDemonRenderFrameBuffer *QDemonRenderFrameBufferPtr;
typedef QDemonRenderVertexBuffer *QDemonRenderVertexBufferPtr;
typedef QDemonRenderIndexBuffer *QDemonRenderIndexBufferPtr;
typedef QDemonRenderTexture2D *QDemonRenderTexture2DPtr;
typedef QDemonRenderTexture2DPtr *QDemonRenderTexture2DHandle;
typedef QDemonRenderTexture2DArray *QDemonRenderTexture2DArrayPtr;
typedef QDemonRenderTextureCube *QDemonRenderTextureCubePtr;
typedef QDemonRenderTextureCubePtr *QDemonRenderTextureCubeHandle;
typedef QDemonRenderImage2D *QDemonRenderImage2DPtr;
typedef QDemonRenderDataBuffer *QDemonRenderDataBufferPtr;
typedef const char *QDemonRenderConstCharPtr;
typedef QDemonRenderShaderProgram *QDemonRenderShaderProgramPtr;
//typedef QDemonDataRef<quint32> QDemonU32List;
//typedef QDemonDataRef<const char *> QDemonConstCharPtrList;

template <typename TDataType>
struct QDemonRenderRectT
{
    typedef TDataType TRectType;
    TDataType m_X;
    TDataType m_Y;
    TDataType m_Width;
    TDataType m_Height;
    QDemonRenderRectT(TDataType x, TDataType y, TDataType w, TDataType h)
        : m_X(x)
        , m_Y(y)
        , m_Width(w)
        , m_Height(h)
    {
    }
    QDemonRenderRectT()
        : m_X(0)
        , m_Y(0)
        , m_Width(0)
        , m_Height(0)
    {
    }
    bool operator==(const QDemonRenderRectT<TDataType> &inOther) const
    {
        return m_X == inOther.m_X && m_Y == inOther.m_Y && m_Width == inOther.m_Width
                && m_Height == inOther.m_Height;
    }
    bool operator!=(const QDemonRenderRectT<TDataType> &inOther) const
    {
        return !(*this == inOther);
    }
    TDataType GetRightExtent() const { return m_X + m_Width; }
    TDataType GetBottomExtent() const { return m_Y + m_Height; }
    // Ensure this rect is inside the bounds of the other rect
    void EnsureInBounds(const QDemonRenderRectT<TDataType> &inOther)
    {
        TDataType rightExtent = NVMin(GetRightExtent(), inOther.GetRightExtent());
        TDataType bottomExtent = NVMin(GetBottomExtent(), inOther.GetBottomExtent());
        m_X = NVMax(m_X, inOther.m_X);
        m_Y = NVMax(m_Y, inOther.m_Y);
        m_Width = NVMax(static_cast<TDataType>(0), rightExtent - m_X);
        m_Height = NVMax(static_cast<TDataType>(0), bottomExtent - m_Y);
    }
};

// Render rects are setup to be in the coordinate space of the gl viewport,
// so x, y are left, bottom with increasing units going to the right and
// up respectively.
struct QDemonRenderRect : public QDemonRenderRectT<qint32>
{
    typedef QDemonRenderRectT<qint32> TBase;
    QDemonRenderRect(qint32 x, qint32 y, qint32 w, qint32 h)
        : TBase(x, y, w, h)
    {
    }
    QDemonRenderRect()
        : TBase()
    {
    }
};

struct QDemonRenderRectF : public QDemonRenderRectT<float>
{
    typedef QDemonRenderRectT<float> TBase;
    QDemonRenderRectF(float x, float y, float w, float h)
        : TBase(x, y, w, h)
    {
    }
    QDemonRenderRectF()
        : TBase()
    {
    }
    QDemonRenderRectF(const QDemonRenderRect &inRect)
        : TBase(float(inRect.m_X), float(inRect.m_Y), float(inRect.m_Width),
                float(inRect.m_Height))
    {
    }
    QDemonRenderRect ToIntegerRect() const
    {
        return QDemonRenderRect(qint32(m_X), qint32(m_Y), qint32(m_Width + .5f),
                                qint32(m_Height + .5f));
    }
    QVector2D BottomLeft() const { return QVector2D(m_X, m_Y); }
    QVector2D Center() const
    {
        QVector2D halfDims = HalfDims();
        return QVector2D(m_X + halfDims.x(), m_Y + halfDims.y());
    }
    QVector2D HalfDims() const { return QVector2D(m_Width / 2.0f, m_Height / 2.0f); }
    // Normalized coordinates are in the range of -1,1 where -1 is the left, bottom edges
    // and 1 is the top,right edges.
    QVector2D AbsoluteToNormalizedCoordinates(QVector2D absoluteCoordinates) const
    {
        QVector2D relativeCoords(ToRectRelative(absoluteCoordinates));
        return RelativeToNormalizedCoordinates(relativeCoords);
    }

    QVector2D RelativeToNormalizedCoordinates(QVector2D rectRelativeCoords) const
    {
        QVector2D halfDims(HalfDims());
        QVector2D retval((rectRelativeCoords.x() / halfDims.x()) - 1.0f,
                         (rectRelativeCoords.y() / halfDims.y()) - 1.0f);
        return retval;
    }

    // Take coordinates in global space and move local space where 0,0 is the center
    // of the rect but return value in pixels, not in normalized -1,1 range
    QVector2D ToNormalizedRectRelative(QVector2D absoluteCoordinates) const
    {
        // normalize them
        QVector2D relativeCoords(ToRectRelative(absoluteCoordinates));
        QVector2D halfDims(HalfDims());
        QVector2D normalized((relativeCoords.x() / halfDims.x()) - 1.0f,
                             (relativeCoords.y() / halfDims.y()) - 1.0f);
        return QVector2D(normalized.x() * halfDims.x(), normalized.y() * halfDims.y());
    }

    // Return coordinates in pixels but relative to this rect.
    QVector2D ToRectRelative(QVector2D absoluteCoordinates) const
    {
        return QVector2D(absoluteCoordinates.x() - m_X, absoluteCoordinates.y() - m_Y);
    }

    QVector2D ToAbsoluteCoords(QVector2D inRelativeCoords) const
    {
        return QVector2D(inRelativeCoords.x() + m_X, inRelativeCoords.y() + m_Y);
    }
};

template <typename TDataType>
struct QDemonRenderGenericVec2
{
    TDataType x;
    TDataType y;
    QDemonRenderGenericVec2(TDataType _x, TDataType _y)
        : x(_x)
        , y(_y)
    {
    }
    QDemonRenderGenericVec2() {}
    bool operator==(const QDemonRenderGenericVec2 &inOther) const
    {
        return x == inOther.x && y == inOther.y;
    }
};

template <typename TDataType>
struct QDemonRenderGenericVec3
{
    TDataType x;
    TDataType y;
    TDataType z;
    QDemonRenderGenericVec3(TDataType _x, TDataType _y, TDataType _z)
        : x(_x)
        , y(_y)
        , z(_z)
    {
    }
    QDemonRenderGenericVec3() {}
    bool operator==(const QDemonRenderGenericVec3 &inOther) const
    {
        return x == inOther.x && y == inOther.y && z == inOther.z;
    }
};

template <typename TDataType>
struct QDemonRenderGenericVec4
{
    TDataType x;
    TDataType y;
    TDataType z;
    TDataType w;
    QDemonRenderGenericVec4(TDataType _x, TDataType _y, TDataType _z, TDataType _w)
        : x(_x)
        , y(_y)
        , z(_z)
        , w(_w)
    {
    }
    QDemonRenderGenericVec4() {}
    bool operator==(const QDemonRenderGenericVec4 &inOther) const
    {
        return x == inOther.x && y == inOther.y && z == inOther.z && w == inOther.w;
    }
};

typedef QDemonRenderGenericVec2<bool> bool_2;
typedef QDemonRenderGenericVec3<bool> bool_3;
typedef QDemonRenderGenericVec4<bool> bool_4;
typedef QDemonRenderGenericVec2<quint32> quint32_2;
typedef QDemonRenderGenericVec3<quint32> quint32_3;
typedef QDemonRenderGenericVec4<quint32> quint32_4;
typedef QDemonRenderGenericVec2<qint32> qint32_2;
typedef QDemonRenderGenericVec3<qint32> qint32_3;
typedef QDemonRenderGenericVec4<qint32> qint32_4;

struct QDemonRenderShaderDataTypes
{
    enum Enum {
        Unknown = 0,
        Integer,            //qint32,
        IntegerVec2,        //qint32_2,
        IntegerVec3,        //qint32_3,
        IntegerVec4,        //qint32_4,
        Boolean,            //bool
        BooleanVec2,        //bool_2,
        BooleanVec3,        //bool_3,
        BooleanVec4,        //bool_4,
        Float,              //float,
        Vec2,               //QVector2D,
        Vec3,               //QVector3D,
        Vec4,               //QVector4D,
        UnsignedInteger,    //quint32,
        UnsignedIntegerVec2,//quint32_2,
        UnsignedIntegerVec3,//quint32_3,
        UnsignedIntegerVec4,//quint32_4,
        Matrix3x3,          //QMatrix3x3,
        Matrix4x4,          //QMatrix4x4,
        Texture2D,          //QDemonRenderTexture2DPtr,
        Texture2DHandle,    //QDemonRenderTexture2DHandle,
        Texture2DArray,     //QDemonRenderTexture2DArrayPtr,
        TextureCube,        //QDemonRenderTextureCubePtr,
        TextureCubeHandle,  //QDemonRenderTextureCubeHandle,
        Image2D,            //QDemonRenderImage2DPtr,
        DataBuffer          //QDemonRenderDataBufferPtr
    };
};

template <typename TDataType>
struct QDemonDataTypeToShaderDataTypeMap
{
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<qint32>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Integer;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<qint32_2>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::IntegerVec2;
    }
};

template <> struct QDemonDataTypeToShaderDataTypeMap<qint32_3>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::IntegerVec3;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<qint32_4>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::IntegerVec4;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<bool>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Boolean;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<bool_2>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::BooleanVec2;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<bool_3>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::BooleanVec3;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<bool_4>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::BooleanVec4;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<float>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Float;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QVector2D>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Vec2;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QVector3D>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Vec3;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QVector4D>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Vec4;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<quint32>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::UnsignedInteger;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<quint32_2>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::UnsignedIntegerVec2;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<quint32_3>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::UnsignedIntegerVec3;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<quint32_4>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::UnsignedIntegerVec4;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QMatrix3x3>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Matrix3x3;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QMatrix4x4>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Matrix4x4;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonConstDataRef<QMatrix4x4>>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Matrix4x4;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonRenderTexture2DPtr>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Texture2D;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonRenderTexture2DHandle>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Texture2DHandle;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonRenderTexture2DArrayPtr>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Texture2DArray;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonRenderTextureCubePtr>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::TextureCube;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonRenderTextureCubeHandle>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::TextureCubeHandle;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonRenderImage2DPtr>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::Image2D;
    }
};

template <>
struct QDemonDataTypeToShaderDataTypeMap<QDemonRenderDataBufferPtr>
{
    static QDemonRenderShaderDataTypes::Enum GetType()
    {
        return QDemonRenderShaderDataTypes::DataBuffer;
    }
};

struct QDemonRenderShaderTypeValue
{
    enum Enum {
        Unknown = 1 << 0,
        Vertex = 1 << 1,
        Fragment = 1 << 2,
        TessControl = 1 << 3,
        TessEvaluation = 1 << 4,
        Geometry = 1 << 5
    };
};

typedef QDemonFlags<QDemonRenderShaderTypeValue::Enum, quint32> QDemonRenderShaderTypeFlags;

struct QDemonRenderTextureTypeValue
{
    enum Enum {
        Unknown = 0,
        Diffuse,
        Specular,
        Environment,
        Bump,
        Normal,
        Displace,
        Emissive,
        Emissive2,
        Anisotropy,
        Translucent,
        LightmapIndirect,
        LightmapRadiosity,
        LightmapShadow
    };

    static const char *toString(QDemonRenderTextureTypeValue::Enum value)
    {
        switch (value) {
        case Unknown:
            return "Unknown";
        case Diffuse:
            return "Diffuse";
        case Specular:
            return "Specular";
        case Environment:
            return "Environment";
        case Bump:
            return "Bump";
        case Normal:
            return "Normal";
        case Displace:
            return "Displace";
        case Emissive:
            return "Emissive";
        case Emissive2:
            return "Emissive2";
        case Anisotropy:
            return "Anisotropy";
        case Translucent:
            return "Translucent";
        case LightmapIndirect:
            return "LightmapIndirect";
        case LightmapRadiosity:
            return "LightmapRadiosity";
        case LightmapShadow:
            return "LightmapShadow";
        }
    }
};

struct QDemonRenderReadPixelFormats
{
    enum Enum {
        Unknown = 0,
        Alpha8,
        RGB565,
        RGB8,
        RGBA4444,
        RGBA5551,
        RGBA8
    };
};

// Now for scoped property access.
template <typename TBaseType, typename TDataType>
struct QDemonRenderGenericScopedProperty
{
    typedef void (TBaseType::*TSetter)(TDataType inType);
    typedef TDataType (TBaseType::*TGetter)() const;

    TBaseType &m_Context;
    TSetter m_Setter;
    TDataType m_InitialValue;
    QDemonRenderGenericScopedProperty(TBaseType &ctx, TGetter getter, TSetter setter)
        : m_Context(ctx)
        , m_Setter(setter)
        , m_InitialValue(((ctx).*getter)())
    {
    }
    QDemonRenderGenericScopedProperty(TBaseType &ctx, TGetter getter, TSetter setter,
                                      const TDataType &inNewValue)
        : m_Context(ctx)
        , m_Setter(setter)
        , m_InitialValue(((ctx).*getter)())
    {
        ((m_Context).*m_Setter)(inNewValue);
    }
    ~QDemonRenderGenericScopedProperty() { ((m_Context).*m_Setter)(m_InitialValue); }
};

QT_END_NAMESPACE

#endif
