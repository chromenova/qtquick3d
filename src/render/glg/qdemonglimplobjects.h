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

#ifndef QDEMON_RENDER_GL_IMPL_OBJECTS_H
#define QDEMON_RENDER_GL_IMPL_OBJECTS_H

#include <QtDemonRender/qdemonopenglutil.h>
#include <QtDemonRender/qdemonrendertexture2d.h>

QT_BEGIN_NAMESPACE

// The set of all properties as they are currently set in hardware.
struct QDemonGLHardPropertyContext
{
    QDemonRef<QDemonRenderFrameBuffer> m_frameBuffer;
    QDemonRef<QDemonRenderShaderProgram> m_activeShader;
    QDemonRef<QDemonRenderProgramPipeline> m_activeProgramPipeline;
    QDemonRef<QDemonRenderInputAssembler> m_inputAssembler;
    QDemonRenderBlendFunctionArgument m_blendFunction;
    QDemonRenderBlendEquationArgument m_blendEquation;
    bool m_cullingEnabled = true;
    QDemonRenderBoolOp m_depthFunction = QDemonRenderBoolOp::Less;
    bool m_blendingEnabled = true;
    bool m_depthWriteEnabled = true;
    bool m_depthTestEnabled = true;
    bool m_stencilTestEnabled = false;
    bool m_scissorTestEnabled = true;
    bool m_colorWritesEnabled = true;
    bool m_multisampleEnabled = false;
    QRect m_scissorRect;
    QRect m_viewport;
    QVector4D m_clearColor{ 0.0, 0.0, 0.0, 1.0 };
};
QT_END_NAMESPACE
#endif
