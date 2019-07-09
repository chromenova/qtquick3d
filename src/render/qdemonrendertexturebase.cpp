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

#include <QtDemonRender/qdemonrendercontext.h>
#include <QtDemonRender/qdemonrendertexturebase.h>

#include <limits>

QT_BEGIN_NAMESPACE

class QDemonRenderTextureSampler
{
public:
    QDemonRenderTextureMinifyingOp minFilter = QDemonRenderTextureMinifyingOp::Linear;
    QDemonRenderTextureMagnifyingOp magFilter = QDemonRenderTextureMagnifyingOp::Linear;
    QDemonRenderTextureCoordOp wrapS = QDemonRenderTextureCoordOp::ClampToEdge;
    QDemonRenderTextureCoordOp wrapT = QDemonRenderTextureCoordOp::ClampToEdge;
    QDemonRenderTextureCoordOp wrapR = QDemonRenderTextureCoordOp::ClampToEdge;
    QDemonRenderTextureSwizzleMode swizzleMode = QDemonRenderTextureSwizzleMode::NoSwizzle;
    float minLod = -1000.;
    float maxLod = 1000.;
    float lodBias = 0.;
    QDemonRenderTextureCompareMode compareMode = QDemonRenderTextureCompareMode::NoCompare;
    QDemonRenderTextureCompareOp compareOp = QDemonRenderTextureCompareOp::LessThanOrEqual;
    float m_anisotropy = 1.;

    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to context
     * @param[in] fnd			Pointer to foundation
     * @param[in] minFilter		Texture min filter
     * @param[in] magFilter		Texture mag filter
     * @param[in] wrapS			Texture coord generation for S
     * @param[in] wrapT			Texture coord generation for T
     * @param[in] wrapR			Texture coord generation for R
     * @param[in] swizzleMode	Texture swizzle mode
     * @param[in] minLod		Texture min level of detail
     * @param[in] maxLod		Texture max level of detail
     * @param[in] lodBias		Texture level of detail bias (unused)
     * @param[in] compareMode	Texture compare mode
     * @param[in] compareFunc	Texture compare function
     * @param[in] anisoFilter	Aniso filter value [1.0, 16.0]
     * @param[in] borderColor	Texture border color float[4] (unused)
     *
     * @return No return.
     */
    explicit QDemonRenderTextureSampler(const QDemonRef<QDemonRenderContext> &context)
        : m_backend(context->backend())
        , m_handle(nullptr)
    {
        // create backend handle
        m_handle = m_backend->createSampler();
    }

    ~QDemonRenderTextureSampler()
    {
        if (m_handle)
            m_backend->releaseSampler(m_handle);
    }

    /**
     * @brief get the backend object handle
     *
     * @return the backend object handle.
     */
    QDemonRenderBackend::QDemonRenderBackendSamplerObject handle() const { return m_handle; }

private:
    QDemonRef<QDemonRenderBackend> m_backend; ///< pointer to backend
    QDemonRenderBackend::QDemonRenderBackendSamplerObject m_handle = nullptr;
};


QDemonRenderTextureBase::QDemonRenderTextureBase(const QDemonRef<QDemonRenderContext> &context,
                                                 QDemonRenderTextureTargetType texTarget)
    : m_context(context)
    , m_backend(context->backend())
    , m_handle(nullptr)
    , m_textureUnit(std::numeric_limits<quint32>::max())
    , m_samplerParamsDirty(true)
    , m_texStateDirty(false)
    , m_sampleCount(1)
    , m_format(QDemonRenderTextureFormat::Unknown)
    , m_texTarget(texTarget)
    , m_baseLevel(0)
    , m_maxLevel(1000)
    , m_maxMipLevel(0)
    , m_immutable(false)
{
    m_handle = m_backend->createTexture();
    m_sampler = new QDemonRenderTextureSampler(context);
}

QDemonRenderTextureBase::~QDemonRenderTextureBase()
{
    if (m_sampler)
        delete m_sampler;
    if (m_handle)
        m_backend->releaseTexture(m_handle);
}

void QDemonRenderTextureBase::setBaseLevel(qint32 value)
{
    if (m_baseLevel != value) {
        m_baseLevel = value;
        m_texStateDirty = true;
    }
}

void QDemonRenderTextureBase::setMaxLevel(qint32 value)
{
    if (m_maxLevel != value) {
        m_maxLevel = value;
        m_texStateDirty = true;
    }
}

void QDemonRenderTextureBase::setMinFilter(QDemonRenderTextureMinifyingOp value)
{
    if (m_sampler->minFilter != value) {
        m_sampler->minFilter = value;
        m_samplerParamsDirty = true;
    }
}

void QDemonRenderTextureBase::setMagFilter(QDemonRenderTextureMagnifyingOp value)
{
    if (m_sampler->magFilter != value) {
        m_sampler->magFilter = value;
        m_samplerParamsDirty = true;
    }
}

void QDemonRenderTextureBase::setTextureWrapS(QDemonRenderTextureCoordOp value)
{
    if (m_sampler->wrapS != value) {
        m_sampler->wrapS = value;
        m_samplerParamsDirty = true;
    }
}

void QDemonRenderTextureBase::setTextureWrapT(QDemonRenderTextureCoordOp value)
{
    if (m_sampler->wrapT != value) {
        m_sampler->wrapT = value;
        m_samplerParamsDirty = true;
    }
}

void QDemonRenderTextureBase::setTextureCompareMode(QDemonRenderTextureCompareMode value)
{
    if (m_sampler->compareMode != value) {
        m_sampler->compareMode = value;
        m_samplerParamsDirty = true;
    }
}

void QDemonRenderTextureBase::setTextureCompareFunc(QDemonRenderTextureCompareOp value)
{
    if (m_sampler->compareOp != value) {
        m_sampler->compareOp = value;
        m_samplerParamsDirty = true;
    }
}

void QDemonRenderTextureBase::applyTexParams()
{
    if (m_samplerParamsDirty) {
        m_backend->updateSampler(m_sampler->handle(),
                                 m_texTarget,
                                 m_sampler->minFilter,
                                 m_sampler->magFilter,
                                 m_sampler->wrapS,
                                 m_sampler->wrapT,
                                 m_sampler->wrapR,
                                 m_sampler->minLod,
                                 m_sampler->maxLod,
                                 m_sampler->lodBias,
                                 m_sampler->compareMode,
                                 m_sampler->compareOp);

        m_samplerParamsDirty = false;
    }

    if (m_texStateDirty) {
        m_backend->updateTextureObject(m_handle, m_texTarget, m_baseLevel, m_maxLevel);
        m_texStateDirty = false;
    }
}

void QDemonRenderTextureBase::applyTexSwizzle()
{
    QDemonRenderTextureSwizzleMode theSwizzleMode = m_backend->getTextureSwizzleMode(m_format);
    if (theSwizzleMode != m_sampler->swizzleMode) {
        m_sampler->swizzleMode = theSwizzleMode;
        m_backend->updateTextureSwizzle(m_handle, m_texTarget, theSwizzleMode);
    }
}
QT_END_NAMESPACE
