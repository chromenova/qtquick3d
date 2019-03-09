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
#ifndef QDEMON_RENDER_RENDER_ATOMIC_COUNTER_BUFFER_H
#define QDEMON_RENDER_RENDER_ATOMIC_COUNTER_BUFFER_H

#include <QtDemonRender/qtdemonrenderglobal.h>
#include <QtDemonRender/qdemonrenderdatabuffer.h>
#include <QtCore/QString>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE

// forward declaration
class QDemonRenderContext;
class AtomicCounterBufferEntry;

typedef QHash<QByteArray, AtomicCounterBufferEntry *> TRenderAtomiCounterBufferEntryMap;

///< Constant (uniform) buffer representation
class Q_DEMONRENDER_EXPORT QDemonRenderAtomicCounterBuffer : public QDemonRenderDataBuffer
{
public:
    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to context
     * @param[in] bufferName	Name of the buffer. Must match the name used in programs
     * @param[in] size			Size of the buffer
     * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
     * @param[in] data			A pointer to the buffer data that is allocated by the
     * application.
     *
     * @return No return.
     */
    QDemonRenderAtomicCounterBuffer(const QDemonRef<QDemonRenderContext> &context,
                                    const QByteArray &bufferName,
                                    size_t size,
                                    QDemonRenderBufferUsageType usageType,
                                    QDemonDataRef<quint8> data);

    ///< destructor
    virtual ~QDemonRenderAtomicCounterBuffer() override;

    /**
     * @brief bind the buffer bypasses the context state
     *
     * @return no return.
     */
    void bind() override;

    /**
     * @brief bind the buffer to a shader program
     *
     * @param[in] index			Index of the constant buffer within the program
     *
     * @return no return.
     */
    void bindToShaderProgram(quint32 index);

    /**
     * @brief update the buffer to hardware
     *
     * @return no return.
     */
    void update();

    /**
     * @brief update a piece of memory directly within the storage buffer
     *
     * Note: When you use this function you should know what you are doing.
     *		 The memory layout within C++ must exactly match the memory layout in the
     *shader.
     *		 We use std140 (430) layout which guarantees a specific layout behavior across
     *all HW vendors.
     *		 How the memory layout is computed can be found in the GL spec.
     *
     * @param[in] offset	offset into storage buffer
     * @param[in] data		pointer to data
     *
     * @return no return
     */
    void updateData(qint32 offset, QDemonDataRef<quint8> data);

    /**
     * @brief add a parameter to the atomic counter buffer
     *
     * @param[in] name		Name of the parameter (must match the name in the shader
     * program)
     * @param[in] offset	Offset in bytes into the buffer
     *
     * @return no return
     */
    void addParam(const QByteArray &name, quint32 offset);

    /**
     * @brief Check if the buffer contains this param
     *
     * @param[in] name		Name of the parameter (must match the name in the shader
     * program)
     *
     * @return no return
     */
    bool containsParam(const QByteArray &name);

    /**
     * @brief get the buffer name
     *
     * @return the buffer name
     */
    QByteArray bufferName() const { return m_name; }

private:
    QByteArray m_name; ///< buffer name
    TRenderAtomiCounterBufferEntryMap m_atomicCounterBufferEntryMap; ///< holds the entries of a atomic counter buffer
    bool m_dirty; ///< true if buffer is dirty
};

QT_END_NAMESPACE

#endif
