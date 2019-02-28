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
#include "qdemonrendercustommaterialshadergenerator.h"
#include <QtDemonRuntimeRender/qdemonrenderdefaultmaterialshadergenerator.h>
#include <QtDemonRuntimeRender/qdemonrendercontextcore.h>
#include <QtDemonRuntimeRender/qdemonrendershadercodegeneratorv2.h>
#include <QtDemonRuntimeRender/qdemonrenderableimage.h>
#include <QtDemonRuntimeRender/qdemonrenderimage.h>
#include <QtDemonRender/qdemonrendercontext.h>
#include <QtDemonRuntimeRender/qdemonrenderlight.h>
#include <QtDemonRender/qdemonrendershaderprogram.h>
#include <QtDemonRuntimeRender/qdemonrendercamera.h>
#include <QtDemonRuntimeRender/qdemonrendershadowmap.h>
#include <QtDemonRuntimeRender/qdemonrendercustommaterial.h>
#include <qdemonrendercustommaterialsystem.h>
#include <QtDemonRuntimeRender/qdemonrenderlightconstantproperties.h>
#include <QtDemonRuntimeRender/qdemonrendershaderkeys.h>
#include <QtDemon/qdemonutils.h>

QT_BEGIN_NAMESPACE

uint qHash(const QDemonShaderDefaultMaterialKey &key) {
    return key.hash();
}

namespace {
struct QDemonShaderLightProperties
{
    QSharedPointer<QDemonRenderShaderProgram> m_shader;
    RenderLightTypes::Enum m_lightType;
    QDemonLightSourceShader m_lightData;

    QDemonShaderLightProperties(QSharedPointer<QDemonRenderShaderProgram> inShader)
        : m_shader(inShader)
        , m_lightType(RenderLightTypes::Directional)
    {
    }

    void Set(const QDemonRenderLight *inLight)
    {
        QVector3D dir(0, 0, 1);
        if (inLight->m_lightType == RenderLightTypes::Directional) {
            dir = inLight->getScalingCorrectDirection();
            // we lit in world sapce
            dir *= -1;
            m_lightData.position = QVector4D(dir, 0.0);
        } else if (inLight->m_lightType == RenderLightTypes::Area) {
            dir = inLight->getScalingCorrectDirection();
            m_lightData.position = QVector4D(inLight->getGlobalPos(), 1.0);
        } else {
            dir = inLight->getGlobalPos();
            m_lightData.position = QVector4D(dir, 1.0);
        }

        m_lightType = inLight->m_lightType;

        m_lightData.direction = QVector4D(dir, 0.0);

        float normalizedBrightness = inLight->m_brightness / 100.0f;
        m_lightData.diffuse = QVector4D(inLight->m_diffuseColor * normalizedBrightness, 1.0);
        m_lightData.specular = QVector4D(inLight->m_specularColor * normalizedBrightness, 1.0);

        if (inLight->m_lightType == RenderLightTypes::Area) {
            m_lightData.width = inLight->m_areaWidth;
            m_lightData.height = inLight->m_areaWidth;

            QMatrix3x3 theDirMatrix(mat44::getUpper3x3(inLight->globalTransform));
            m_lightData.right = QVector4D(mat33::transform(theDirMatrix, QVector3D(1, 0, 0)), inLight->m_areaWidth);
            m_lightData.up = QVector4D(mat33::transform(theDirMatrix, QVector3D(0, 1, 0)), inLight->m_areaHeight);
        } else {
            m_lightData.width = 0.0;
            m_lightData.height = 0.0;
            m_lightData.right = QVector4D();
            m_lightData.up = QVector4D();

            // These components only apply to CG lights
            m_lightData.ambient = QVector4D(inLight->m_ambientColor, 1.0);

            m_lightData.constantAttenuation = 1.0;
            m_lightData.linearAttenuation = inLight->m_linearFade;
            m_lightData.quadraticAttenuation = inLight->m_exponentialFade;
            m_lightData.spotCutoff = 180.0;
        }

        if (m_lightType == RenderLightTypes::Point) {
            m_lightData.shadowView = QMatrix4x4();
        } else {
            m_lightData.shadowView = inLight->globalTransform;
        }
    }

    static QDemonShaderLightProperties createLightEntry(QSharedPointer<QDemonRenderShaderProgram> inShader)
    {
        return QDemonShaderLightProperties(inShader);
    }
};

/**
 *	Cached texture property lookups, used one per texture so a shader generator for N
 *	textures will have an array of N of these lookup objects.
 */
struct QDemonShaderTextureProperties
{
    QDemonRenderCachedShaderProperty<QDemonRenderTexture2D *> m_sampler;
    QDemonRenderCachedShaderProperty<QVector3D> m_offsets;
    QDemonRenderCachedShaderProperty<QVector4D> m_rotations;
    QDemonShaderTextureProperties(const QString &sampName, const QString &offName, const QString &rotName, QSharedPointer<QDemonRenderShaderProgram> inShader)
        : m_sampler(sampName, inShader)
        , m_offsets(offName, inShader)
        , m_rotations(rotName, inShader)
    {
    }
    QDemonShaderTextureProperties() {}
};

/* We setup some shared state on the custom material shaders */
struct QDemonShaderGeneratorGeneratedShader
{
    typedef QHash<quint32, QDemonShaderTextureProperties> TCustomMaterialImagMap;

    QSharedPointer<QDemonRenderShaderProgram> m_shader;
    // Specific properties we know the shader has to have.
    QDemonRenderCachedShaderProperty<QMatrix4x4> m_modelMatrix;
    QDemonRenderCachedShaderProperty<QMatrix4x4> m_viewProjMatrix;
    QDemonRenderCachedShaderProperty<QMatrix4x4> m_viewMatrix;
    QDemonRenderCachedShaderProperty<QMatrix3x3> m_normalMatrix;
    QDemonRenderCachedShaderProperty<QVector3D> m_cameraPos;
    QDemonRenderCachedShaderProperty<QMatrix4x4> m_projMatrix;
    QDemonRenderCachedShaderProperty<QMatrix4x4> m_viewportMatrix;
    QDemonRenderCachedShaderProperty<QVector2D> m_camProperties;
    QDemonRenderCachedShaderProperty<QDemonRenderTexture2D *> m_depthTexture;
    QDemonRenderCachedShaderProperty<QDemonRenderTexture2D *> m_aoTexture;
    QDemonRenderCachedShaderProperty<QDemonRenderTexture2D *> m_lightProbe;
    QDemonRenderCachedShaderProperty<QVector4D> m_lightProbeProps;
    QDemonRenderCachedShaderProperty<QVector4D> m_lightProbeOpts;
    QDemonRenderCachedShaderProperty<QVector4D> m_lightProbeRot;
    QDemonRenderCachedShaderProperty<QVector4D> m_lightProbeOfs;
    QDemonRenderCachedShaderProperty<QDemonRenderTexture2D *> m_lightProbe2;
    QDemonRenderCachedShaderProperty<QVector4D> m_lightProbe2Props;
    QDemonRenderCachedShaderProperty<qint32> m_lightCount;
    QDemonRenderCachedShaderProperty<qint32> m_areaLightCount;
    QDemonRenderCachedShaderProperty<qint32> m_shadowMapCount;
    QDemonRenderCachedShaderProperty<qint32> m_shadowCubeCount;
    QDemonRenderCachedShaderProperty<float> m_opacity;
    QDemonRenderCachedShaderBuffer<QDemonRenderShaderConstantBuffer> m_aoShadowParams;
    QDemonRenderCachedShaderBuffer<QDemonRenderShaderConstantBuffer> m_lightsBuffer;
    QDemonRenderCachedShaderBuffer<QDemonRenderShaderConstantBuffer> m_areaLightsBuffer;

    QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader> *m_lightsProperties;
    QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader> *m_areaLightsProperties;

    typedef QDemonRenderCachedShaderPropertyArray<QDemonRenderTexture2D *, QDEMON_MAX_NUM_SHADOWS> ShadowMapPropertyArray;
    typedef QDemonRenderCachedShaderPropertyArray<QDemonRenderTextureCube *, QDEMON_MAX_NUM_SHADOWS> ShadowCubePropertyArray;

    ShadowMapPropertyArray m_shadowMaps;
    ShadowCubePropertyArray m_shadowCubes;

    // Cache the image property name lookups
    TCustomMaterialImagMap m_images; // Images external to custom material usage
    volatile qint32 m_refCount;

    QDemonShaderGeneratorGeneratedShader(QSharedPointer<QDemonRenderShaderProgram> inShader, QSharedPointer<QDemonRenderContext> inContext)
        : m_shader(inShader)
        , m_modelMatrix("model_matrix", inShader)
        , m_viewProjMatrix("model_view_projection", inShader)
        , m_viewMatrix("view_matrix", inShader)
        , m_normalMatrix("normal_matrix", inShader)
        , m_cameraPos("camera_position", inShader)
        , m_projMatrix("view_projection_matrix", inShader)
        , m_viewportMatrix("viewport_matrix", inShader)
        , m_camProperties("camera_properties", inShader)
        , m_depthTexture("depth_sampler", inShader)
        , m_aoTexture("ao_sampler", inShader)
        , m_lightProbe("light_probe", inShader)
        , m_lightProbeProps("light_probe_props", inShader)
        , m_lightProbeOpts("light_probe_opts", inShader)
        , m_lightProbeRot("light_probe_rotation", inShader)
        , m_lightProbeOfs("light_probe_offset", inShader)
        , m_lightProbe2("light_probe2", inShader)
        , m_lightProbe2Props("light_probe2_props", inShader)
        , m_lightCount("uNumLights", inShader)
        , m_areaLightCount("uNumAreaLights", inShader)
        , m_shadowMapCount("uNumShadowMaps", inShader)
        , m_shadowCubeCount("uNumShadowCubes", inShader)
        , m_opacity("object_opacity", inShader)
        , m_aoShadowParams("cbAoShadow", inShader)
        , m_lightsBuffer("cbBufferLights", inShader)
        , m_areaLightsBuffer("cbBufferAreaLights", inShader)
        , m_lightsProperties(nullptr)
        , m_areaLightsProperties(nullptr)
        , m_shadowMaps("shadowMaps[0]", inShader)
        , m_shadowCubes("shadowCubes[0]", inShader)
    {
    }

    ~QDemonShaderGeneratorGeneratedShader()
    {
        delete m_lightsProperties;
        delete m_areaLightsProperties;
    }

    QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader> *getLightProperties(int count)
    {
        if (!m_lightsProperties || m_areaLightsProperties->m_lightCountInt < count) {
            if (m_lightsProperties)
                delete m_lightsProperties;
            m_lightsProperties = new QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader>(QStringLiteral("lights"),
                                                                                                         QStringLiteral("uNumLights"),
                                                                                                         this,
                                                                                                         false,
                                                                                                         count);
        }
        return m_lightsProperties;
    }
    QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader> *getAreaLightProperties(int count)
    {
        if (!m_areaLightsProperties || m_areaLightsProperties->m_lightCountInt < count) {
            if (m_areaLightsProperties)
                delete m_areaLightsProperties;
            m_areaLightsProperties = new QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader>(QStringLiteral("areaLights"),
                                                                                                             QStringLiteral("uNumAreaLights"),
                                                                                                             this,
                                                                                                             false,
                                                                                                             count);
        }
        return m_areaLightsProperties;
    }
};

struct QDemonShaderGenerator : public ICustomMaterialShaderGenerator
{
    typedef QHash<QSharedPointer<QDemonRenderShaderProgram>, QSharedPointer<QDemonShaderGeneratorGeneratedShader>> TProgramToShaderMap;
    typedef QPair<size_t, QSharedPointer<QDemonShaderLightProperties>> TCustomMaterialLightEntry;
    typedef QPair<size_t, QDemonRenderCachedShaderProperty<QDemonRenderTexture2D *>> TShadowMapEntry;
    typedef QPair<size_t, QDemonRenderCachedShaderProperty<QDemonRenderTextureCube *>> TShadowCubeEntry;
    typedef QHash<QString, QSharedPointer<QDemonRenderConstantBuffer>> TStrConstanBufMap;

    QDemonRenderContextInterface *m_renderContext;
    QSharedPointer<QDemonShaderProgramGeneratorInterface> m_programGenerator;

    const QDemonRenderCustomMaterial *m_currentMaterial;
    QDemonShaderDefaultMaterialKey *m_currentKey;
    QDemonDefaultMaterialVertexPipelineInterface *m_currentPipeline;
    TShaderFeatureSet m_currentFeatureSet;
    QVector<QDemonRenderLight *> m_lights;
    QDemonRenderableImage *m_firstImage;
    bool m_hasTransparency;

    QString m_imageStem;
    QString m_imageSampler;
    QString m_imageFragCoords;
    QString m_imageRotScale;
    QString m_imageOffset;

    QString m_generatedShaderString;

    QDemonShaderDefaultMaterialKeyProperties m_defaultMaterialShaderKeyProperties;
    TProgramToShaderMap m_programToShaderMap;

    QVector<TCustomMaterialLightEntry> m_lightEntries;

    TStrConstanBufMap m_constantBuffers; ///< store all constants buffers

    QDemonShaderGenerator(QDemonRenderContextInterface *inRc)
        : m_renderContext(inRc)
        , m_programGenerator(m_renderContext->getShaderProgramGenerator())
        , m_currentMaterial(nullptr)
        , m_currentKey(nullptr)
        , m_currentPipeline(nullptr)
        , m_firstImage(nullptr)
        , m_hasTransparency(false)
    {
    }

    QSharedPointer<QDemonShaderProgramGeneratorInterface> programGenerator() { return m_programGenerator; }
    QDemonDefaultMaterialVertexPipelineInterface &vertexGenerator() { return *m_currentPipeline; }
    QDemonShaderStageGeneratorInterface &fragmentGenerator()
    {
        return *m_programGenerator->getStage(ShaderGeneratorStages::Fragment);
    }
    QDemonShaderDefaultMaterialKey &key() { return *m_currentKey; }
    const QDemonRenderCustomMaterial &material() { return *m_currentMaterial; }
    TShaderFeatureSet featureSet() { return m_currentFeatureSet; }
    bool hasTransparency() { return m_hasTransparency; }

    quint32 convertTextureTypeValue(QDemonImageMapTypes::Enum inType)
    {
        QDemonRenderTextureTypeValue::Enum retVal = QDemonRenderTextureTypeValue::Unknown;

        switch (inType) {
        case QDemonImageMapTypes::LightmapIndirect:
            retVal = QDemonRenderTextureTypeValue::LightmapIndirect;
            break;
        case QDemonImageMapTypes::LightmapRadiosity:
            retVal = QDemonRenderTextureTypeValue::LightmapRadiosity;
            break;
        case QDemonImageMapTypes::LightmapShadow:
            retVal = QDemonRenderTextureTypeValue::LightmapShadow;
            break;
        case QDemonImageMapTypes::Bump:
            retVal = QDemonRenderTextureTypeValue::Bump;
            break;
        case QDemonImageMapTypes::Diffuse:
            retVal = QDemonRenderTextureTypeValue::Diffuse;
            break;
        case QDemonImageMapTypes::Displacement:
            retVal = QDemonRenderTextureTypeValue::Displace;
            break;
        default:
            retVal = QDemonRenderTextureTypeValue::Unknown;
            break;
        }

        Q_ASSERT(retVal != QDemonRenderTextureTypeValue::Unknown);

        return (quint32)retVal;
    }

    ImageVariableNames getImageVariableNames(quint32 imageIdx) override
    {
        // convert to QDemonRenderTextureTypeValue
        QDemonRenderTextureTypeValue::Enum texType = (QDemonRenderTextureTypeValue::Enum)imageIdx;
        m_imageStem = QDemonRenderTextureTypeValue::toString(texType);
        m_imageStem.append(QStringLiteral("_"));
        m_imageSampler = m_imageStem;
        m_imageSampler.append(QStringLiteral("sampler"));
        m_imageFragCoords = m_imageStem;
        m_imageFragCoords.append(QStringLiteral("uv_coords"));
        m_imageRotScale = m_imageStem;
        m_imageRotScale.append(QStringLiteral("rot_scale"));
        m_imageOffset = m_imageStem;
        m_imageOffset.append(QStringLiteral("offset"));

        ImageVariableNames retVal;
        retVal.m_imageSampler = m_imageSampler;
        retVal.m_imageFragCoords = m_imageFragCoords;
        return retVal;
    }

    void setImageShaderVariables(QSharedPointer<QDemonShaderGeneratorGeneratedShader> inShader,
                                 QDemonRenderableImage &inImage)
    {
        // skip displacement and emissive mask maps which are handled differently
        if (inImage.m_mapType == QDemonImageMapTypes::Displacement
                || inImage.m_mapType == QDemonImageMapTypes::Emissive)
            return;

        QDemonShaderGeneratorGeneratedShader::TCustomMaterialImagMap::iterator iter =
                inShader->m_images.find(inImage.m_mapType);
        if (iter == inShader->m_images.end()) {
            ImageVariableNames names =
                    getImageVariableNames(convertTextureTypeValue(inImage.m_mapType));
            inShader->m_images.insert(
                        (quint32)inImage.m_mapType,
                        QDemonShaderTextureProperties(names.m_imageSampler, m_imageOffset,
                                                 m_imageRotScale, inShader->m_shader));
            iter = inShader->m_images.find(inImage.m_mapType);
        }

        QDemonShaderTextureProperties &theShaderProps = iter.value();
        const QMatrix4x4 &textureTransform = inImage.m_image.m_textureTransform;
        const float *dataPtr(textureTransform.constData());
        QVector3D offsets(dataPtr[12], dataPtr[13], 0.0f);
        // Grab just the upper 2x2 rotation matrix from the larger matrix.
        QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

        // The image horizontal and vertical tiling modes need to be set here, before we set texture
        // on the shader.
        // because setting the image on the texture forces the textue to bind and immediately apply
        // any tex params.
        inImage.m_image.m_textureData.m_texture->setTextureWrapS(
                    inImage.m_image.m_horizontalTilingMode);
        inImage.m_image.m_textureData.m_texture->setTextureWrapT(
                    inImage.m_image.m_verticalTilingMode);

        theShaderProps.m_sampler.set(inImage.m_image.m_textureData.m_texture.data());
        theShaderProps.m_offsets.set(offsets);
        theShaderProps.m_rotations.set(rotations);
    }

    void generateImageUVCoordinates(QDemonShaderStageGeneratorInterface &, quint32, quint32, QDemonRenderableImage &) override {}

    ///< get the light constant buffer and generate if necessary
    QSharedPointer<QDemonRenderConstantBuffer> getLightConstantBuffer(const char *name, quint32 inLightCount)
    {
        QSharedPointer<QDemonRenderContext> theContext(m_renderContext->getRenderContext());

        // we assume constant buffer support
        Q_ASSERT(theContext->getConstantBufferSupport());
        // we only create if if we have lights
        if (!inLightCount || !theContext->getConstantBufferSupport())
            return nullptr;

        QString theName = QString::fromLocal8Bit(name);
        QSharedPointer<QDemonRenderConstantBuffer> pCB = theContext->getConstantBuffer(theName);

        if (!pCB) {
            // create with size of all structures + int for light count
            QDemonLightSourceShader s[QDEMON_MAX_NUM_LIGHTS];
            QDemonDataRef<quint8> cBuffer((quint8 *)&s, (sizeof(QDemonLightSourceShader) * QDEMON_MAX_NUM_LIGHTS)
                                          + (4 * sizeof(qint32)));
            pCB = theContext->createConstantBuffer(
                        name, QDemonRenderBufferUsageType::Static,
                        (sizeof(QDemonLightSourceShader) * QDEMON_MAX_NUM_LIGHTS) + (4 * sizeof(qint32)), cBuffer);
            if (!pCB) {
                Q_ASSERT(false);
                return nullptr;
            }
            // init first set
            memset(&s[0], 0x0, sizeof(QDemonLightSourceShader) * QDEMON_MAX_NUM_LIGHTS);
            qint32 cgLights[4] = {0, 0, 0, 0};
            pCB->updateRaw(0, QDemonDataRef<quint8>((quint8 *)&cgLights, sizeof(qint32) * 4));
            pCB->updateRaw(4 * sizeof(qint32),
                           QDemonDataRef<quint8>((quint8 *)&s[0],
                           sizeof(QDemonLightSourceShader) * QDEMON_MAX_NUM_LIGHTS));
            pCB->update(); // update to hardware

            m_constantBuffers.insert(theName, pCB);
        }

        return pCB;
    }

    void generateVertexShader()
    {
        // vertex displacement
        quint32 imageIdx = 0;
        QDemonRenderableImage *displacementImage = nullptr;
        quint32 displacementImageIdx = 0;

        for (QDemonRenderableImage *img = m_firstImage; img != nullptr;
             img = img->m_nextImage, ++imageIdx) {
            if (img->m_mapType == QDemonImageMapTypes::Displacement) {
                displacementImage = img;
                displacementImageIdx = imageIdx;
                break;
            }
        }

        // the pipeline opens/closes up the shaders stages
        vertexGenerator().beginVertexGeneration(displacementImageIdx, displacementImage);
    }

    QSharedPointer<QDemonShaderGeneratorGeneratedShader> getShaderForProgram(QSharedPointer<QDemonRenderShaderProgram> inProgram)
    {
        TProgramToShaderMap::iterator inserter = m_programToShaderMap.find(inProgram);

        if (m_programToShaderMap.find(inProgram) == m_programToShaderMap.end())
            inserter = m_programToShaderMap.insert(inProgram, QSharedPointer<QDemonShaderGeneratorGeneratedShader>(new QDemonShaderGeneratorGeneratedShader(inProgram, m_renderContext->getRenderContext())));

        return inserter.value();
    }

    virtual QSharedPointer<QDemonShaderLightProperties> setLight(QSharedPointer<QDemonRenderShaderProgram> inShader,
                                                                 size_t lightIdx,
                                                                 size_t shadeIdx,
                                                                 const QDemonRenderLight *inLight,
                                                                 QDemonShadowMapEntry *inShadow,
                                                                 qint32 shadowIdx,
                                                                 float shadowDist)
    {
        QSharedPointer<QDemonShaderLightProperties> theLightEntry;
        for (quint32 idx = 0, end = m_lightEntries.size(); idx < end && theLightEntry == nullptr;
             ++idx) {
            if (m_lightEntries[idx].first == lightIdx
                    && m_lightEntries[idx].second->m_shader == inShader
                    && m_lightEntries[idx].second->m_lightType == inLight->m_lightType) {
                theLightEntry = m_lightEntries[idx].second;
            }
        }
        if (theLightEntry == nullptr) {
            // create a new name
            QString lightName;
            if (inLight->m_lightType == RenderLightTypes::Area)
                lightName = QStringLiteral("arealights");
            else
                lightName = QStringLiteral("lights");
            char buf[16];
            qsnprintf(buf, 16, "[%d]", int(shadeIdx));
            lightName.append(QString::fromLocal8Bit(buf));

            QSharedPointer<QDemonShaderLightProperties> theNewEntry(new QDemonShaderLightProperties(QDemonShaderLightProperties::createLightEntry(inShader)));
            m_lightEntries.push_back(TCustomMaterialLightEntry(lightIdx, theNewEntry));
            theLightEntry = theNewEntry;
        }
        theLightEntry->Set(inLight);
        theLightEntry->m_lightData.shadowControls =
                QVector4D(inLight->m_shadowBias, inLight->m_shadowFactor, shadowDist, 0.0);
        theLightEntry->m_lightData.shadowIdx = (inShadow) ? shadowIdx : -1;

        return theLightEntry;
    }

    void setShadowMaps(QSharedPointer<QDemonRenderShaderProgram> inProgram,
                       QDemonShadowMapEntry *inShadow,
                       qint32 &numShadowMaps,
                       qint32 &numShadowCubes,
                       bool shadowMap,
                       QDemonShaderGeneratorGeneratedShader::ShadowMapPropertyArray &shadowMaps,
                       QDemonShaderGeneratorGeneratedShader::ShadowCubePropertyArray &shadowCubes)
    {
        Q_UNUSED(inProgram)
        if (inShadow) {
            if (shadowMap == false && inShadow->m_depthCube
                    && (numShadowCubes < QDEMON_MAX_NUM_SHADOWS)) {
                shadowCubes.m_array[numShadowCubes] = inShadow->m_depthCube.data();
                ++numShadowCubes;
            } else if (shadowMap && inShadow->m_depthMap
                       && (numShadowMaps < QDEMON_MAX_NUM_SHADOWS)) {
                shadowMaps.m_array[numShadowMaps] = inShadow->m_depthMap.data();
                ++numShadowMaps;
            }
        }
    }

    void setGlobalProperties(QSharedPointer<QDemonRenderShaderProgram> inProgram,
                             const QDemonRenderLayer & /*inLayer*/,
                             QDemonRenderCamera &inCamera,
                             QVector3D,
                             QVector<QDemonRenderLight *> &inLights,
                             QVector<QVector3D> &,
                             QSharedPointer<QDemonRenderShadowMap> inShadowMaps)
    {
        QSharedPointer<QDemonShaderGeneratorGeneratedShader> theShader(getShaderForProgram(inProgram));
        m_renderContext->getRenderContext()->setActiveShader(inProgram);

        QDemonRenderCamera &theCamera(inCamera);

        QVector2D camProps(theCamera.clipNear, theCamera.clipFar);
        theShader->m_camProperties.set(camProps);
        theShader->m_cameraPos.set(theCamera.getGlobalPos());

        if (theShader->m_viewMatrix.isValid())
            theShader->m_viewMatrix.set(mat44::getInverse(theCamera.globalTransform));

        if (theShader->m_projMatrix.isValid()) {
            QMatrix4x4 vProjMat;
            inCamera.calculateViewProjectionMatrix(vProjMat);
            theShader->m_projMatrix.set(vProjMat);
        }

        // set lights separate for area lights
        qint32 cgLights = 0, areaLights = 0;
        qint32 numShadowMaps = 0, numShadowCubes = 0;

        // this call setup the constant buffer for ambient occlusion and shadow
        theShader->m_aoShadowParams.set();

        if (m_renderContext->getRenderContext()->getConstantBufferSupport()) {
            QSharedPointer<QDemonRenderConstantBuffer> pLightCb = getLightConstantBuffer("cbBufferLights", inLights.size());
            QSharedPointer<QDemonRenderConstantBuffer> pAreaLightCb = getLightConstantBuffer("cbBufferAreaLights", inLights.size());

            // Split the count between CG lights and area lights
            for (quint32 lightIdx = 0; lightIdx < inLights.size() && pLightCb; ++lightIdx) {
                QDemonShadowMapEntry *theShadow = nullptr;
                if (inShadowMaps && inLights[lightIdx]->m_castShadow)
                    theShadow = inShadowMaps->getShadowMapEntry(lightIdx);

                qint32 shdwIdx = (inLights[lightIdx]->m_lightType
                                  != RenderLightTypes::Directional)
                        ? numShadowCubes
                        : numShadowMaps;
                setShadowMaps(inProgram, theShadow, numShadowMaps, numShadowCubes,
                              inLights[lightIdx]->m_lightType == RenderLightTypes::Directional,
                              theShader->m_shadowMaps, theShader->m_shadowCubes);

                if (inLights[lightIdx]->m_lightType == RenderLightTypes::Area) {
                    QSharedPointer<QDemonShaderLightProperties> theAreaLightEntry =
                            setLight(inProgram, lightIdx, areaLights, inLights[lightIdx], theShadow, shdwIdx, inCamera.clipFar);

                    if (theAreaLightEntry && pAreaLightCb) {
                        pAreaLightCb->updateRaw(
                                    areaLights * sizeof(QDemonLightSourceShader) + (4 * sizeof(qint32)),
                                    QDemonDataRef<quint8>((quint8 *)&theAreaLightEntry->m_lightData,
                                                          sizeof(QDemonLightSourceShader)));
                    }

                    areaLights++;
                } else {
                    QSharedPointer<QDemonShaderLightProperties> theLightEntry =
                            setLight(inProgram, lightIdx, cgLights, inLights[lightIdx], theShadow,
                                     shdwIdx, inCamera.clipFar);

                    if (theLightEntry && pLightCb) {
                        pLightCb->updateRaw(
                                    cgLights * sizeof(QDemonLightSourceShader) + (4 * sizeof(qint32)),
                                    QDemonDataRef<quint8>((quint8 *)&theLightEntry->m_lightData,
                                                          sizeof(QDemonLightSourceShader)));
                    }

                    cgLights++;
                }
            }

            if (pLightCb) {
                pLightCb->updateRaw(0, QDemonDataRef<quint8>((quint8 *)&cgLights, sizeof(qint32)));
                theShader->m_lightsBuffer.set();
            }
            if (pAreaLightCb) {
                pAreaLightCb->updateRaw(0, QDemonDataRef<quint8>((quint8 *)&areaLights,
                                                                 sizeof(qint32)));
                theShader->m_areaLightsBuffer.set();
            }

            theShader->m_lightCount.set(cgLights);
            theShader->m_areaLightCount.set(areaLights);
        } else {
            QVector<QSharedPointer<QDemonShaderLightProperties>> lprop;
            QVector<QSharedPointer<QDemonShaderLightProperties >> alprop;
            for (quint32 lightIdx = 0; lightIdx < inLights.size(); ++lightIdx) {

                QDemonShadowMapEntry *theShadow = nullptr;
                if (inShadowMaps && inLights[lightIdx]->m_castShadow)
                    theShadow = inShadowMaps->getShadowMapEntry(lightIdx);

                qint32 shdwIdx = (inLights[lightIdx]->m_lightType
                                  != RenderLightTypes::Directional)
                        ? numShadowCubes
                        : numShadowMaps;
                setShadowMaps(inProgram, theShadow, numShadowMaps, numShadowCubes,
                              inLights[lightIdx]->m_lightType == RenderLightTypes::Directional,
                              theShader->m_shadowMaps, theShader->m_shadowCubes);

                QSharedPointer<QDemonShaderLightProperties> p = setLight(inProgram, lightIdx, areaLights,
                                                     inLights[lightIdx], theShadow,
                                                     shdwIdx, inCamera.clipFar);
                if (inLights[lightIdx]->m_lightType == RenderLightTypes::Area)
                    alprop.push_back(p);
                else
                    lprop.push_back(p);
            }
            QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader> *lightProperties
                    = theShader->getLightProperties(lprop.size());
            QDemonLightConstantProperties<QDemonShaderGeneratorGeneratedShader> *areaLightProperties
                    = theShader->getAreaLightProperties(alprop.size());

            lightProperties->updateLights(lprop);
            areaLightProperties->updateLights(alprop);

            theShader->m_lightCount.set(lprop.size());
            theShader->m_areaLightCount.set(alprop.size());
        }
        for (int i = numShadowMaps; i < QDEMON_MAX_NUM_SHADOWS; ++i)
            theShader->m_shadowMaps.m_array[i] = nullptr;
        for (int i = numShadowCubes; i < QDEMON_MAX_NUM_SHADOWS; ++i)
            theShader->m_shadowCubes.m_array[i] = nullptr;
        theShader->m_shadowMaps.set(numShadowMaps);
        theShader->m_shadowCubes.set(numShadowCubes);
        theShader->m_shadowMapCount.set(numShadowMaps);
        theShader->m_shadowCubeCount.set(numShadowCubes);
    }

    void setMaterialProperties(QSharedPointer<QDemonRenderShaderProgram> inProgram,
                               const QDemonRenderCustomMaterial &inMaterial,
                               const QVector2D &,
                               const QMatrix4x4 &inModelViewProjection,
                               const QMatrix3x3 &inNormalMatrix,
                               const QMatrix4x4 &inGlobalTransform,
                               QDemonRenderableImage *inFirstImage,
                               float inOpacity,
                               QSharedPointer<QDemonRenderTexture2D> inDepthTexture,
                               QSharedPointer<QDemonRenderTexture2D> inSSaoTexture,
                               QDemonRenderImage *inLightProbe,
                               QDemonRenderImage *inLightProbe2,
                               float inProbeHorizon,
                               float inProbeBright,
                               float inProbe2Window,
                               float inProbe2Pos,
                               float inProbe2Fade,
                               float inProbeFOV)
    {
        QSharedPointer<QDemonCustomMaterialSystemInterface> theMaterialSystem(m_renderContext->getCustomMaterialSystem());
        QSharedPointer<QDemonShaderGeneratorGeneratedShader> theShader(getShaderForProgram(inProgram));

        theShader->m_viewProjMatrix.set(inModelViewProjection);
        theShader->m_normalMatrix.set(inNormalMatrix);
        theShader->m_modelMatrix.set(inGlobalTransform);

        theShader->m_depthTexture.set(inDepthTexture.data());
        theShader->m_aoTexture.set(inSSaoTexture.data());

        theShader->m_opacity.set(inOpacity);

        QDemonRenderImage *theLightProbe = inLightProbe;
        QDemonRenderImage *theLightProbe2 = inLightProbe2;

        if (inMaterial.m_iblProbe && inMaterial.m_iblProbe->m_textureData.m_texture) {
            theLightProbe = inMaterial.m_iblProbe;
        }

        if (theLightProbe) {
            if (theLightProbe->m_textureData.m_texture) {
                QDemonRenderTextureCoordOp::Enum theHorzLightProbeTilingMode =
                        QDemonRenderTextureCoordOp::Repeat;
                QDemonRenderTextureCoordOp::Enum theVertLightProbeTilingMode =
                        theLightProbe->m_verticalTilingMode;
                theLightProbe->m_textureData.m_texture->setTextureWrapS(
                            theHorzLightProbeTilingMode);
                theLightProbe->m_textureData.m_texture->setTextureWrapT(
                            theVertLightProbeTilingMode);

                const QMatrix4x4 &textureTransform = theLightProbe->m_textureTransform;
                // We separate rotational information from offset information so that just maybe the
                // shader
                // will attempt to push less information to the card.
                const float *dataPtr(textureTransform.constData());
                // The third member of the offsets contains a flag indicating if the texture was
                // premultiplied or not.
                // We use this to mix the texture alpha.
                // light_probe_offsets.w is now no longer being used to enable/disable fast IBL,
                // (it's now the only option)
                // So now, it's storing the number of mip levels in the IBL image.
                QVector4D offsets(dataPtr[12], dataPtr[13],
                        theLightProbe->m_textureData.m_textureFlags.isPreMultiplied() ? 1.0f
                                                                                      : 0.0f,
                        (float)theLightProbe->m_textureData.m_texture->getNumMipmaps());
                // Fast IBL is always on;
                // inRenderContext.m_Layer.m_FastIbl ? 1.0f : 0.0f );
                // Grab just the upper 2x2 rotation matrix from the larger matrix.
                QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

                theShader->m_lightProbeRot.set(rotations);
                theShader->m_lightProbeOfs.set(offsets);

                if ((!inMaterial.m_iblProbe) && (inProbeFOV < 180.f)) {
                    theShader->m_lightProbeOpts.set(
                                QVector4D(0.01745329251994329547f * inProbeFOV, 0.0f, 0.0f, 0.0f));
                }

                // Also make sure to add the secondary texture, but it should only be added if the
                // primary
                // (i.e. background) texture is also there.
                if (theLightProbe2 && theLightProbe2->m_textureData.m_texture) {
                    theLightProbe2->m_textureData.m_texture->setTextureWrapS(
                                theHorzLightProbeTilingMode);
                    theLightProbe2->m_textureData.m_texture->setTextureWrapT(
                                theVertLightProbeTilingMode);
                    theShader->m_lightProbe2.set(theLightProbe2->m_textureData.m_texture.data());
                    theShader->m_lightProbe2Props.set(
                                QVector4D(inProbe2Window, inProbe2Pos, inProbe2Fade, 1.0f));

                    const QMatrix4x4 &xform2 = theLightProbe2->m_textureTransform;
                    const float *dataPtr(xform2.constData());

                    theShader->m_lightProbeProps.set(
                                QVector4D(dataPtr[12], dataPtr[13], inProbeHorizon, inProbeBright * 0.01f));
                } else {
                    theShader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
                    theShader->m_lightProbeProps.set(
                                QVector4D(0.0f, 0.0f, inProbeHorizon, inProbeBright * 0.01f));
                }
            } else {
                theShader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, -1.0f, 0.0f));
                theShader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
            }

            theShader->m_lightProbe.set(theLightProbe->m_textureData.m_texture.data());

        } else {
            theShader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, -1.0f, 0.0f));
            theShader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
        }

        // finally apply custom material shader properties
        theMaterialSystem->applyShaderPropertyValues(inMaterial, inProgram);

        // additional textures
        for (QDemonRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_nextImage)
            setImageShaderVariables(theShader, *theImage);
    }

    void setMaterialProperties(QSharedPointer<QDemonRenderShaderProgram> inProgram,
                               const QDemonGraphObject &inMaterial,
                               const QVector2D &inCameraVec,
                               const QMatrix4x4 &inModelViewProjection,
                               const QMatrix3x3 &inNormalMatrix,
                               const QMatrix4x4 &inGlobalTransform,
                               QDemonRenderableImage *inFirstImage,
                               float inOpacity,
                               QDemonLayerGlobalRenderProperties inRenderProperties) override
    {
        const QDemonRenderCustomMaterial &theCustomMaterial(
                    reinterpret_cast<const QDemonRenderCustomMaterial &>(inMaterial));
        Q_ASSERT(inMaterial.type == QDemonGraphObjectTypes::CustomMaterial);

        setGlobalProperties(inProgram, inRenderProperties.layer, inRenderProperties.camera,
                            inRenderProperties.cameraDirection, inRenderProperties.lights,
                            inRenderProperties.lightDirections,
                            inRenderProperties.shadowMapManager);

        setMaterialProperties(inProgram, theCustomMaterial, inCameraVec, inModelViewProjection,
                              inNormalMatrix, inGlobalTransform, inFirstImage, inOpacity,
                              inRenderProperties.depthTexture, inRenderProperties.ssaoTexture,
                              inRenderProperties.lightProbe, inRenderProperties.lightProbe2,
                              inRenderProperties.probeHorizon, inRenderProperties.probeBright,
                              inRenderProperties.probe2Window, inRenderProperties.probe2Pos,
                              inRenderProperties.probe2Fade, inRenderProperties.probeFOV);
    }

    void generateLightmapIndirectFunc(QDemonShaderStageGeneratorInterface &inFragmentShader,
                                      QDemonRenderImage *pEmissiveLightmap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec3 computeMaterialLightmapIndirect()\n{\n";
        inFragmentShader << "  vec4 indirect = vec4( 0.0, 0.0, 0.0, 0.0 );\n";
        if (pEmissiveLightmap) {
            ImageVariableNames names =
                    getImageVariableNames(convertTextureTypeValue(QDemonImageMapTypes::LightmapIndirect));
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  indirect = evalIndirectLightmap( " << m_imageSampler
                             << ", varTexCoord1, ";
            inFragmentShader << m_imageRotScale << ", ";
            inFragmentShader << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return indirect.rgb;\n";
        inFragmentShader << "}\n\n";
    }

    void generateLightmapRadiosityFunc(QDemonShaderStageGeneratorInterface &inFragmentShader,
                                       QDemonRenderImage *pRadiosityLightmap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec3 computeMaterialLightmapRadiosity()\n{\n";
        inFragmentShader << "  vec4 radiosity = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pRadiosityLightmap) {
            ImageVariableNames names =
                    getImageVariableNames(convertTextureTypeValue(QDemonImageMapTypes::LightmapRadiosity));
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  radiosity = evalRadiosityLightmap( " << m_imageSampler
                             << ", varTexCoord1, ";
            inFragmentShader << m_imageRotScale << ", ";
            inFragmentShader << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return radiosity.rgb;\n";
        inFragmentShader << "}\n\n";
    }

    void generateLightmapShadowFunc(QDemonShaderStageGeneratorInterface &inFragmentShader,
                                    QDemonRenderImage *pBakedShadowMap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec4 computeMaterialLightmapShadow()\n{\n";
        inFragmentShader << "  vec4 shadowMask = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pBakedShadowMap) {
            ImageVariableNames names =
                    getImageVariableNames((quint32)QDemonRenderTextureTypeValue::LightmapShadow);
            // Add uniforms
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  shadowMask = evalShadowLightmap( " << m_imageSampler
                             << ", texCoord0, ";
            inFragmentShader << m_imageRotScale << ", ";
            inFragmentShader << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return shadowMask;\n";
        inFragmentShader << "}\n\n";
    }

    void generateLightmapIndirectSetupCode(QDemonShaderStageGeneratorInterface &inFragmentShader,
                                           QDemonRenderableImage *pIndirectLightmap,
                                           QDemonRenderableImage *pRadiosityLightmap)
    {
        if (!pIndirectLightmap && !pRadiosityLightmap)
            return;

        QString finalValue;

        inFragmentShader << "\n";
        inFragmentShader << "void initializeLayerVariablesWithLightmap(void)\n{\n";
        if (pIndirectLightmap) {
            inFragmentShader
                    << "  vec3 lightmapIndirectValue = computeMaterialLightmapIndirect( );\n";
            finalValue.append("vec4(lightmapIndirectValue, 1.0)");
        }
        if (pRadiosityLightmap) {
            inFragmentShader
                    << "  vec3 lightmapRadisoityValue = computeMaterialLightmapRadiosity( );\n";
            if (finalValue.isEmpty())
                finalValue.append("vec4(lightmapRadisoityValue, 1.0)");
            else
                finalValue.append(" + vec4(lightmapRadisoityValue, 1.0)");
        }

        finalValue.append(";\n");

        char buf[16];
        for (quint32 idx = 0; idx < material().m_layerCount; idx++) {
            qsnprintf(buf, 16, "[%d]", idx);
            inFragmentShader << "  layers" << buf << ".base += " << finalValue;
            inFragmentShader << "  layers" << buf << ".layer += " << finalValue;
        }

        inFragmentShader << "}\n\n";
    }

    void generateLightmapShadowCode(QDemonShaderStageGeneratorInterface &inFragmentShader,
                                    QDemonRenderableImage *pBakedShadowMap)
    {
        if (pBakedShadowMap) {
            inFragmentShader << " tmpShadowTerm *= computeMaterialLightmapShadow( );\n\n";
        }
    }

    void applyEmissiveMask(QDemonShaderStageGeneratorInterface &inFragmentShader, QDemonRenderImage *pEmissiveMaskMap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec3 computeMaterialEmissiveMask()\n{\n";
        inFragmentShader << "  vec3 emissiveMask = vec3( 1.0, 1.0, 1.0 );\n";
        if (pEmissiveMaskMap) {
            inFragmentShader << "  texture_coordinate_info tci;\n";
            inFragmentShader << "  texture_coordinate_info transformed_tci;\n";
            inFragmentShader << "  tci = textureCoordinateInfo( texCoord0, tangent, binormal );\n";
            inFragmentShader << "  transformed_tci = transformCoordinate( "
                                "rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), ";
            inFragmentShader << "vec3( 0.000000, 0.000000, 0.000000 ), vec3( 1.000000, 1.000000, "
                                "1.000000 ) ), tci );\n";
            inFragmentShader << "  emissiveMask = fileTexture( "
                             << pEmissiveMaskMap->m_imageShaderName
                             << ", vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_alpha, transformed_tci, ";
            inFragmentShader << "vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), "
                                "wrap_repeat, wrap_repeat, gamma_default ).tint;\n";
        }

        inFragmentShader << "  return emissiveMask;\n";
        inFragmentShader << "}\n\n";
    }

    void generateFragmentShader(QDemonShaderDefaultMaterialKey &, const QString &inShaderPathName)
    {
        QSharedPointer<QDemonDynamicObjectSystemInterface> theDynamicSystem(m_renderContext->getDynamicObjectSystem());
        QString fragSource = theDynamicSystem->getShaderSource(inShaderPathName);

        Q_ASSERT(!fragSource.isEmpty());

        // light maps
        bool hasLightmaps = false;
        QDemonRenderableImage *lightmapShadowImage = nullptr;
        QDemonRenderableImage *lightmapIndirectImage = nullptr;
        QDemonRenderableImage *lightmapRadisoityImage = nullptr;

        for (QDemonRenderableImage *img = m_firstImage; img != nullptr; img = img->m_nextImage) {
            if (img->m_mapType == QDemonImageMapTypes::LightmapIndirect) {
                lightmapIndirectImage = img;
                hasLightmaps = true;
            } else if (img->m_mapType == QDemonImageMapTypes::LightmapRadiosity) {
                lightmapRadisoityImage = img;
                hasLightmaps = true;
            } else if (img->m_mapType == QDemonImageMapTypes::LightmapShadow) {
                lightmapShadowImage = img;
            }
        }

        vertexGenerator().generateUVCoords(0);
        // for lightmaps we expect a second set of uv coordinates
        if (hasLightmaps) {
            vertexGenerator().generateUVCoords(1);
        }

        QDemonDefaultMaterialVertexPipelineInterface &vertexShader(vertexGenerator());
        QDemonShaderStageGeneratorInterface &fragmentShader(fragmentGenerator());

        QString srcString(fragSource);

        if (m_renderContext->getRenderContext()->getRenderContextType()
                == QDemonRenderContextValues::GLES2) {
            QString::size_type pos = 0;
            while ((pos = srcString.indexOf("out vec4 fragColor", pos)) != -1) {
                srcString.insert(pos, QStringLiteral("//"));
                pos += int(strlen("//out vec4 fragColor"));
            }
        }

        fragmentShader << "#define FRAGMENT_SHADER\n\n";

        if (!srcString.contains("void main()"))
            fragmentShader.addInclude("evalLightmaps.glsllib");

        // check dielectric materials
        if (!material().isDielectric())
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 1\n\n";
        else
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 0\n\n";

        fragmentShader << "#define QDEMON_ENABLE_RNM 0\n\n";

        fragmentShader << srcString << "\n";

        if (srcString.contains("void main()")) // If a "main()" is already
            // written, we'll assume that the
            // shader
        { // pass is already written out and we don't need to add anything.
            // Nothing beyond the basics, anyway
            vertexShader.generateWorldNormal();
            vertexShader.generateVarTangentAndBinormal();
            vertexShader.generateWorldPosition();

            vertexShader.generateViewVector();
            return;
        }

        if (material().hasLighting() && lightmapIndirectImage) {
            generateLightmapIndirectFunc(fragmentShader, &lightmapIndirectImage->m_image);
        }
        if (material().hasLighting() && lightmapRadisoityImage) {
            generateLightmapRadiosityFunc(fragmentShader, &lightmapRadisoityImage->m_image);
        }
        if (material().hasLighting() && lightmapShadowImage) {
            generateLightmapShadowFunc(fragmentShader, &lightmapShadowImage->m_image);
        }

        if (material().hasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            generateLightmapIndirectSetupCode(fragmentShader, lightmapIndirectImage,
                                              lightmapRadisoityImage);

        if (material().hasLighting()) {
            applyEmissiveMask(fragmentShader, material().m_emissiveMap2);
        }

        // setup main
        vertexGenerator().beginFragmentGeneration();

        // since we do pixel lighting we always need this if lighting is enabled
        // We write this here because the functions below may also write to
        // the fragment shader
        if (material().hasLighting()) {
            vertexShader.generateWorldNormal();
            vertexShader.generateVarTangentAndBinormal();
            vertexShader.generateWorldPosition();

            if (material().isSpecularEnabled()) {
                vertexShader.generateViewVector();
            }
        }

        fragmentShader << "  initializeBaseFragmentVariables();" << "\n";
        fragmentShader << "  computeTemporaries();" << "\n";
        fragmentShader << "  normal = normalize( computeNormal() );" << "\n";
        fragmentShader << "  initializeLayerVariables();" << "\n";
        fragmentShader << "  float alpha = clamp( evalCutout(), 0.0, 1.0 );" << "\n";

        if (material().isCutOutEnabled()) {
            fragmentShader << "  if ( alpha <= 0.0f )" << "\n";
            fragmentShader << "    discard;" << "\n";
        }

        // indirect / direct lightmap init
        if (material().hasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            fragmentShader << "  initializeLayerVariablesWithLightmap();" << "\n";

        // shadow map
        generateLightmapShadowCode(fragmentShader, lightmapShadowImage);

        // main Body
        fragmentShader << "#include \"customMaterialFragBodyAO.glsllib\"" << "\n";

        // for us right now transparency means we render a glass style material
        if (m_hasTransparency && !material().isTransmissive())
            fragmentShader << " rgba = computeGlass( normal, materialIOR, alpha, rgba );" << "\n";
        if (material().isTransmissive())
            fragmentShader << " rgba = computeOpacity( rgba );" << "\n";

        if (vertexGenerator().hasActiveWireframe()) {
            fragmentShader.append("vec3 edgeDistance = varEdgeDistance * gl_FragCoord.w;");
            fragmentShader.append(
                        "\tfloat d = min(min(edgeDistance.x, edgeDistance.y), edgeDistance.z);");
            fragmentShader.append("\tfloat mixVal = smoothstep(0.0, 1.0, d);"); // line width 1.0

            fragmentShader.append("\trgba = mix( vec4(0.0, 1.0, 0.0, 1.0), rgba, mixVal);");
        }
        fragmentShader << "  rgba.a *= object_opacity;" << "\n";
        if (m_renderContext->getRenderContext()->getRenderContextType()
                == QDemonRenderContextValues::GLES2)
            fragmentShader << "  gl_FragColor = rgba;" << "\n";
        else
            fragmentShader << "  fragColor = rgba;" << "\n";
    }

    QSharedPointer<QDemonRenderShaderProgram> generateCustomMaterialShader(const QString &inShaderPrefix,
                                                            const QString &inCustomMaterialName)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.
        m_generatedShaderString.clear();
        m_generatedShaderString = inShaderPrefix;
        m_generatedShaderString.append(inCustomMaterialName);
        QDemonShaderDefaultMaterialKey theKey(key());
        theKey.toString(m_generatedShaderString, m_defaultMaterialShaderKeyProperties);

        generateVertexShader();
        generateFragmentShader(theKey, inCustomMaterialName);

        vertexGenerator().endVertexGeneration();
        vertexGenerator().endFragmentGeneration();

        return programGenerator()->compileGeneratedShader(m_generatedShaderString, QDemonShaderCacheProgramFlags(), featureSet());
    }

    virtual QSharedPointer<QDemonRenderShaderProgram> generateShader(const QDemonGraphObject &inMaterial,
                                                                     QDemonShaderDefaultMaterialKey inShaderDescription,
                                                                     QDemonShaderStageGeneratorInterface &inVertexPipeline,
                                                                     TShaderFeatureSet inFeatureSet,
                                                                     const QVector<QDemonRenderLight *> &inLights,
                                                                     QDemonRenderableImage *inFirstImage,
                                                                     bool inHasTransparency,
                                                                     const QString &inShaderPrefix,
                                                                     const QString &inCustomMaterialName) override
    {
        Q_ASSERT(inMaterial.type == QDemonGraphObjectTypes::CustomMaterial);
        m_currentMaterial = reinterpret_cast<const QDemonRenderCustomMaterial *>(&inMaterial);
        m_currentKey = &inShaderDescription;
        m_currentPipeline = static_cast<QDemonDefaultMaterialVertexPipelineInterface *>(&inVertexPipeline);
        m_currentFeatureSet = inFeatureSet;
        m_lights = inLights;
        m_firstImage = inFirstImage;
        m_hasTransparency = inHasTransparency;

        return generateCustomMaterialShader(inShaderPrefix, inCustomMaterialName);
    }
};
}

QSharedPointer<ICustomMaterialShaderGenerator> ICustomMaterialShaderGenerator::createCustomMaterialShaderGenerator(QDemonRenderContextInterface *inRc)
{
    return QSharedPointer<ICustomMaterialShaderGenerator>(new QDemonShaderGenerator(inRc));
}

QT_END_NAMESPACE