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

#include <QtDemonRender/qdemonrenderdrawindirectbuffer.h>
#include <QtDemonRender/qdemonrendercontext.h>
#include <QtDemonRender/qdemonrendershaderprogram.h>
#include <QtDemon/qdemonutils.h>

QT_BEGIN_NAMESPACE
QDemonRenderDrawIndirectBuffer::QDemonRenderDrawIndirectBuffer(const QDemonRef<QDemonRenderContext> &context,
                                                               QDemonRenderBufferUsageType usageType,
                                                               QDemonByteView data)
    : QDemonRenderDataBuffer(context, QDemonRenderBufferType::DrawIndirect, usageType, data), m_dirty(true)
{
}

QDemonRenderDrawIndirectBuffer::~QDemonRenderDrawIndirectBuffer()
{
}

void QDemonRenderDrawIndirectBuffer::bind()
{
    if (m_mapped) {
        qCCritical(INVALID_OPERATION, "Attempting to Bind a locked buffer");
        Q_ASSERT(false);
    }

    m_backend->bindBuffer(m_handle, m_type);
}

void QDemonRenderDrawIndirectBuffer::update()
{
    // we only update the buffer if it is dirty and we actually have some data
    if (m_dirty && m_bufferData.size()) {
        m_backend->updateBuffer(m_handle, m_type, m_usageType, m_bufferData);
        m_dirty = false;
    }
}

void QDemonRenderDrawIndirectBuffer::updateData(qint32 offset, QDemonByteView data)
{
    // we only update the buffer if we something
    if (data.size())
        m_backend->updateBufferRange(m_handle, m_type, offset, data);
}

QT_END_NAMESPACE
