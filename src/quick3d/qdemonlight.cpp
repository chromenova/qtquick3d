/****************************************************************************
**
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

#include "qdemonlight.h"
#include "qdemonobject_p.h"

#include <QtDemonRuntimeRender/qdemonrenderlight.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype DemonLight
    \inqmlmodule QtDemon
    \brief Lets you define the lighting for a 3D item
*/

QDemonLight::QDemonLight() : m_diffuseColor(Qt::white), m_specularColor(Qt::white), m_ambientColor(Qt::black) {}

QDemonLight::~QDemonLight() {}

QDemonObject::Type QDemonLight::type() const
{
    return QDemonObject::Light;
}

QDemonLight::QDemonRenderLightTypes QDemonLight::lightType() const
{
    return m_lightType;
}

QColor QDemonLight::diffuseColor() const
{
    return m_diffuseColor;
}

QColor QDemonLight::specularColor() const
{
    return m_specularColor;
}

QColor QDemonLight::ambientColor() const
{
    return m_ambientColor;
}

float QDemonLight::brightness() const
{
    return m_brightness;
}

float QDemonLight::linearFade() const
{
    return m_linearFade;
}

float QDemonLight::exponentialFade() const
{
    return m_exponentialFade;
}

float QDemonLight::areaWidth() const
{
    return m_areaWidth;
}

float QDemonLight::areaHeight() const
{
    return m_areaHeight;
}

bool QDemonLight::castShadow() const
{
    return m_castShadow;
}

float QDemonLight::shadowBias() const
{
    return m_shadowBias;
}

float QDemonLight::shadowFactor() const
{
    return m_shadowFactor;
}

int QDemonLight::shadowMapResolution() const
{
    return m_shadowMapResolution;
}

float QDemonLight::shadowMapFar() const
{
    return m_shadowMapFar;
}

float QDemonLight::shadowMapFieldOfView() const
{
    return m_shadowMapFieldOfView;
}

float QDemonLight::shadowFilter() const
{
    return m_shadowFilter;
}

QDemonNode *QDemonLight::scope() const
{
    return m_scope;
}

void QDemonLight::setLightType(QDemonLight::QDemonRenderLightTypes lightType)
{
    if (m_lightType == lightType)
        return;

    m_lightType = lightType;
    emit lightTypeChanged(m_lightType);
    update();
}

void QDemonLight::setDiffuseColor(QColor diffuseColor)
{
    if (m_diffuseColor == diffuseColor)
        return;

    m_diffuseColor = diffuseColor;
    emit diffuseColorChanged(m_diffuseColor);
    update();
}

void QDemonLight::setSpecularColor(QColor specularColor)
{
    if (m_specularColor == specularColor)
        return;

    m_specularColor = specularColor;
    emit specularColorChanged(m_specularColor);
    update();
}

void QDemonLight::setAmbientColor(QColor ambientColor)
{
    if (m_ambientColor == ambientColor)
        return;

    m_ambientColor = ambientColor;
    emit ambientColorChanged(m_ambientColor);
    update();
}

void QDemonLight::setBrightness(float brightness)
{
    if (qFuzzyCompare(m_brightness, brightness))
        return;

    m_brightness = brightness;
    emit brightnessChanged(m_brightness);
    update();
}

void QDemonLight::setLinearFade(float linearFade)
{
    if (qFuzzyCompare(m_linearFade, linearFade))
        return;

    m_linearFade = linearFade;
    emit linearFadeChanged(m_linearFade);
    update();
}

void QDemonLight::setExponentialFade(float exponentialFade)
{
    if (qFuzzyCompare(m_exponentialFade, exponentialFade))
        return;

    m_exponentialFade = exponentialFade;
    emit exponentialFadeChanged(m_exponentialFade);
    update();
}

void QDemonLight::setAreaWidth(float areaWidth)
{
    if (qFuzzyCompare(m_areaWidth, areaWidth))
        return;

    m_areaWidth = areaWidth;
    emit areaWidthChanged(m_areaWidth);
    update();
}

void QDemonLight::setAreaHeight(float areaHeight)
{
    if (qFuzzyCompare(m_areaHeight, areaHeight))
        return;

    m_areaHeight = areaHeight;
    emit areaHeightChanged(m_areaHeight);
    update();
}

void QDemonLight::setCastShadow(bool castShadow)
{
    if (m_castShadow == castShadow)
        return;

    m_castShadow = castShadow;
    emit castShadowChanged(m_castShadow);
    update();
}

void QDemonLight::setShadowBias(float shadowBias)
{
    if (qFuzzyCompare(m_shadowBias, shadowBias))
        return;

    m_shadowBias = shadowBias;
    emit shadowBiasChanged(m_shadowBias);
    update();
}

void QDemonLight::setShadowFactor(float shadowFactor)
{
    if (qFuzzyCompare(m_shadowFactor, shadowFactor))
        return;

    m_shadowFactor = shadowFactor;
    emit shadowFactorChanged(m_shadowFactor);
    update();
}

void QDemonLight::setShadowMapResolution(int shadowMapResolution)
{
    if (m_shadowMapResolution == shadowMapResolution)
        return;

    m_shadowMapResolution = shadowMapResolution;
    emit shadowMapResolutionChanged(m_shadowMapResolution);
    update();
}

void QDemonLight::setShadowMapFar(float shadowMapFar)
{
    if (qFuzzyCompare(m_shadowMapFar, shadowMapFar))
        return;

    m_shadowMapFar = shadowMapFar;
    emit shadowMapFarChanged(m_shadowMapFar);
    update();
}

void QDemonLight::setShadowMapFieldOfView(float shadowMapFieldOfView)
{
    if (qFuzzyCompare(m_shadowMapFieldOfView, shadowMapFieldOfView))
        return;

    m_shadowMapFieldOfView = shadowMapFieldOfView;
    emit shadowMapFieldOfViewChanged(m_shadowMapFieldOfView);
    update();
}

void QDemonLight::setShadowFilter(float shadowFilter)
{
    if (qFuzzyCompare(m_shadowFilter, shadowFilter))
        return;

    m_shadowFilter = shadowFilter;
    emit shadowFilterChanged(m_shadowFilter);
    update();
}

void QDemonLight::setScope(QDemonNode *scope)
{
    if (m_scope == scope)
        return;

    m_scope = scope;
    emit scopeChanged(m_scope);
    update();
}

QDemonRenderGraphObject *QDemonLight::updateSpatialNode(QDemonRenderGraphObject *node)
{
    if (!node)
        node = new QDemonRenderLight();

    QDemonNode::updateSpatialNode(node);

    QDemonRenderLight *light = static_cast<QDemonRenderLight *>(node);

    light->m_lightType = QDemonRenderLight::Type(m_lightType);
    light->m_diffuseColor = QVector3D(m_diffuseColor.redF(), m_diffuseColor.greenF(), m_diffuseColor.blueF());
    light->m_specularColor = QVector3D(m_specularColor.redF(), m_specularColor.greenF(), m_specularColor.blueF());
    light->m_ambientColor = QVector3D(m_ambientColor.redF(), m_ambientColor.greenF(), m_ambientColor.blueF());

    light->m_brightness = m_brightness;
    light->m_linearFade = m_linearFade;
    light->m_exponentialFade = m_exponentialFade;

    light->m_areaWidth = m_areaWidth;
    light->m_areaHeight = m_areaHeight;

    light->m_castShadow = m_castShadow;
    light->m_shadowBias = m_shadowBias;
    light->m_shadowFactor = m_shadowFactor;
    light->m_shadowMapRes = m_shadowMapResolution;
    light->m_shadowMapFar = m_shadowMapFar;
    light->m_shadowMapFov = m_shadowMapFieldOfView;
    light->m_shadowFilter = m_shadowFilter;

    if (m_scope)
        light->m_scope = static_cast<QDemonRenderNode*>(QDemonObjectPrivate::get(m_scope)->spatialNode);
    else
        light->m_scope = nullptr;

    return node;
}

QT_END_NAMESPACE
