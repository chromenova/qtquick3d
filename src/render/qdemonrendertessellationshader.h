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
#ifndef QDEMON_RENDER_TESSELLATION_SHADER_H
#define QDEMON_RENDER_TESSELLATION_SHADER_H

#include <QtDemonRender/qdemonrendershader.h>

QT_BEGIN_NAMESPACE

class QDemonRenderContextImpl;

///< This class represents a tessellation control shader
class QDemonRenderTessControlShader : public QDemonRenderShader
{

public:
    /**
         * @brief constructor
         *
         * @param[in] context		Pointer to render context
         * @param[in] fnd			Pointer to foundation
         * @param[in] source		Pointer to shader source code
         * @param[in] binaryProgram	true if this is a binary program
         *
         * @return No return.
         */
    QDemonRenderTessControlShader(QSharedPointer<QDemonRenderContextImpl> context,
                                  QDemonConstDataRef<qint8> source, bool binaryProgram);

    /// @brief destructor
    ~QDemonRenderTessControlShader();

    /**
         * @brief Query if shader compiled succesfuly
         *
         * @return True if shader is valid.
         */
    bool IsValid() override { return (m_ShaderHandle != nullptr); }

    /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
    virtual QDemonRenderBackend::QDemonRenderBackendTessControlShaderObject GetShaderHandle()
    {
        return m_ShaderHandle;
    }

private:
    QDemonRenderBackend::QDemonRenderBackendTessControlShaderObject
    m_ShaderHandle; ///< opaque backend handle
};

///< This class represents a tessellation evaluation shader
class QDemonRenderTessEvaluationShader : public QDemonRenderShader
{

public:
    /**
         * @brief constructor
         *
         * @param[in] context		Pointer to render context
         * @param[in] fnd			Pointer to foundation
         * @param[in] source		Pointer to shader source code
         * @param[in] binaryProgram	true if this is a binary program
         *
         * @return No return.
         */
    QDemonRenderTessEvaluationShader(QSharedPointer<QDemonRenderContextImpl> context,
                                 QDemonConstDataRef<qint8> source, bool binaryProgram);

    /// @brief destructor
    ~QDemonRenderTessEvaluationShader();

    /**
         * @brief Query if shader compiled succesfuly
         *
         * @return True if shader is valid.
         */
    bool IsValid() override { return (m_ShaderHandle != nullptr); }

    /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
    virtual QDemonRenderBackend::QDemonRenderBackendTessEvaluationShaderObject GetShaderHandle()
    {
        return m_ShaderHandle;
    }

private:
    QDemonRenderBackend::QDemonRenderBackendTessEvaluationShaderObject
    m_ShaderHandle; ///< opaque backend handle
};

QT_END_NAMESPACE

#endif
