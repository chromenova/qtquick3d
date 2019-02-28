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


#include <QtDemonRuntimeRender/qdemonrenderer.h>
#include <QtDemonRuntimeRender/qdemonrendererimpl.h>
#include <QtDemonRuntimeRender/qdemonrendercontextcore.h>
#include <QtDemonRuntimeRender/qdemonrendercamera.h>
#include <QtDemonRuntimeRender/qdemonrenderlight.h>
#include <QtDemonRuntimeRender/qdemonrenderimage.h>
#include <QtDemonRuntimeRender/qdemonrenderbuffermanager.h>
#include <QtDemonRuntimeRender/qdemonoffscreenrendermanager.h>
#include <QtDemonRuntimeRender/qdemonrendercontextcore.h>
#include <QtDemonRuntimeRender/qdemontextrenderer.h>
#include <QtDemonRuntimeRender/qdemonrenderscene.h>
#include <QtDemonRuntimeRender/qdemonrenderpresentation.h>
#include <QtDemonRuntimeRender/qdemonrendereffect.h>
#include <QtDemonRuntimeRender/qdemonrendereffectsystem.h>
#include <QtDemonRuntimeRender/qdemonrenderresourcemanager.h>
#include <QtDemonRuntimeRender/qdemonrendertexttexturecache.h>
#include <QtDemonRuntimeRender/qdemonrendertexttextureatlas.h>
#include <QtDemonRuntimeRender/qdemonrendermaterialhelpers.h>
#include <QtDemonRuntimeRender/qdemonrendercustommaterialsystem.h>
#include <QtDemonRuntimeRender/qdemonrenderrenderlist.h>
#include <QtDemonRuntimeRender/qdemonrenderpath.h>
#include <QtDemonRuntimeRender/qdemonrendershadercodegeneratorv2.h>
#include <QtDemonRuntimeRender/qdemonrenderdefaultmaterialshadergenerator.h>

#include <QtDemonRender/qdemonrenderframebuffer.h>
#include <QtDemon/QDemonDataRef>
#include <QtDemon/qdemonutils.h>

#include <stdlib.h>
#include <algorithm>

#ifdef _WIN32
#pragma warning(disable : 4355)
#endif

// Quick tests you can run to find performance problems

//#define QDEMON_RENDER_DISABLE_HARDWARE_BLENDING 1
//#define QDEMON_RENDER_DISABLE_LIGHTING 1
//#define QDEMON_RENDER_DISABLE_TEXTURING 1
//#define QDEMON_RENDER_DISABLE_TRANSPARENCY 1
//#define QDEMON_RENDER_DISABLE_FRUSTUM_CULLING 1

// If you are fillrate bound then sorting opaque objects can help in some circumstances
//#define QDEMON_RENDER_DISABLE_OPAQUE_SORT 1

QT_BEGIN_NAMESPACE

struct QDemonRenderableImage;
struct QDemonShaderGeneratorGeneratedShader;
struct QDemonSubsetRenderable;

static QDemonRenderInstanceId combineLayerAndId(const QDemonRenderLayer *layer, const QDemonRenderInstanceId id)
{
    uint64_t x = (uint64_t)layer;
    x += 31u * (uint64_t)id;
    return (QDemonRenderInstanceId)x;
}

QDemonRendererImpl::QDemonRendererImpl(QDemonRenderContextInterface *ctx)
    : m_demonContext(ctx)
    , m_context(ctx->getRenderContext())
    , m_bufferManager(ctx->getBufferManager())
    , m_offscreenRenderManager(ctx->getOffscreenRenderManager())
    #ifdef ADVANCED_BLEND_SW_FALLBACK
    , m_layerBlendTexture(ctx->getResourceManager())
    , m_blendFb(nullptr)
    #endif
    , m_currentLayer(nullptr)
    , m_pickRenderPlugins(true)
    , m_layerCachingEnabled(true)
    , m_layerGPuProfilingEnabled(false)
{
}
QDemonRendererImpl::~QDemonRendererImpl()
{
    m_layerShaders.clear();
    m_shaders.clear();
    m_instanceRenderMap.clear();
    m_constantBuffers.clear();
}

void QDemonRendererImpl::childrenUpdated(QDemonGraphNode &inParent)
{
    if (inParent.type == QDemonGraphObjectTypes::Layer) {
        TInstanceRenderMap::iterator theIter = m_instanceRenderMap.find(static_cast<QDemonRenderInstanceId>(&inParent));
        if (theIter != m_instanceRenderMap.end()) {
            theIter.value()->camerasAndLights.clear();
            theIter.value()->renderableNodes.clear();
        }
    } else if (inParent.parent)
        childrenUpdated(*inParent.parent);
}

float QDemonRendererImpl::getTextScale(const QDemonText &inText)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inText);
    if (theData)
        return theData->m_textScale;
    return 1.0f;
}

static inline QDemonRenderLayer *getNextLayer(QDemonRenderLayer &inLayer)
{
    if (inLayer.nextSibling && inLayer.nextSibling->type == QDemonGraphObjectTypes::Layer)
        return static_cast<QDemonRenderLayer *>(inLayer.nextSibling);
    return nullptr;
}

static inline void maybePushLayer(QDemonRenderLayer &inLayer, QVector<QDemonRenderLayer *> &outLayerList)
{
    inLayer.calculateGlobalVariables();
    if (inLayer.flags.isGloballyActive() && inLayer.flags.isLayerRenderToTarget())
        outLayerList.push_back(&inLayer);
}
static void buildRenderableLayers(QDemonRenderLayer &inLayer, QVector<QDemonRenderLayer *> &renderableLayers,
                                  bool inRenderSiblings)
{
    maybePushLayer(inLayer, renderableLayers);
    if (inRenderSiblings) {
        for (QDemonRenderLayer *theNextLayer = getNextLayer(inLayer); theNextLayer;
             theNextLayer = getNextLayer(*theNextLayer))
            maybePushLayer(*theNextLayer, renderableLayers);
    }
}

bool QDemonRendererImpl::prepareLayerForRender(QDemonRenderLayer &inLayer,
                                               const QVector2D &inViewportDimensions,
                                               bool inRenderSiblings,
                                               const QDemonRenderInstanceId id)
{
    (void)inViewportDimensions;
    QVector<QDemonRenderLayer *> renderableLayers;
    // Found by fair roll of the dice.
    renderableLayers.reserve(4);

    buildRenderableLayers(inLayer, renderableLayers, inRenderSiblings);

    bool retval = false;

    for (QVector<QDemonRenderLayer *>::reverse_iterator iter = renderableLayers.rbegin(),
         end = renderableLayers.rend();
         iter != end; ++iter) {
        // Store the previous state of if we were rendering a layer.
        QDemonRenderLayer *theLayer = *iter;
        QSharedPointer<QDemonLayerRenderData> theRenderData = getOrCreateLayerRenderDataForNode(*theLayer, id);

        if (theRenderData) {
            theRenderData->prepareForRender();
            retval = retval || theRenderData->layerPrepResult->flags.wasDirty();
        } else {
            Q_ASSERT(false);
        }
    }

    return retval;
}

void QDemonRendererImpl::renderLayer(QDemonRenderLayer &inLayer,
                                     const QVector2D &inViewportDimensions,
                                     bool clear,
                                     QVector3D clearColor,
                                     bool inRenderSiblings,
                                     const QDemonRenderInstanceId id)
{
    (void)inViewportDimensions;
    QVector<QDemonRenderLayer *> renderableLayers;
    // Found by fair roll of the dice.
    renderableLayers.reserve(4);

    buildRenderableLayers(inLayer, renderableLayers, inRenderSiblings);

    QSharedPointer<QDemonRenderContext> theRenderContext(m_demonContext->getRenderContext());
    QSharedPointer<QDemonRenderFrameBuffer> theFB = theRenderContext->getRenderTarget();
    auto iter = renderableLayers.rbegin();
    const auto end = renderableLayers.rend();
    for (;iter != end; ++iter) {
        QDemonRenderLayer *theLayer = *iter;
        QSharedPointer<QDemonLayerRenderData> theRenderData = getOrCreateLayerRenderDataForNode(*theLayer, id);
        QDemonLayerRenderPreparationResult &prepRes(*theRenderData->layerPrepResult);
        LayerBlendTypes::Enum layerBlend = prepRes.getLayer()->getLayerBlend();
#ifdef ADVANCED_BLEND_SW_FALLBACK
        if ((layerBlend == LayerBlendTypes::Overlay ||
             layerBlend == LayerBlendTypes::ColorBurn ||
             layerBlend == LayerBlendTypes::ColorDodge) &&
                !theRenderContext->isAdvancedBlendHwSupported() &&
                !theRenderContext->isAdvancedBlendHwSupportedKHR()) {
            // Create and set up FBO and texture for advanced blending SW fallback
            QRect viewport = theRenderContext->getViewport();
            m_layerBlendTexture.ensureTexture(viewport.width() + viewport.x(),
                                              viewport.height() + viewport.y(),
                                              QDemonRenderTextureFormats::RGBA8);
            if (m_blendFb == nullptr)
                m_blendFb = theRenderContext->createFrameBuffer();
            m_blendFb->attach(QDemonRenderFrameBufferAttachments::Color0, m_layerBlendTexture.getTexture());
            theRenderContext->setRenderTarget(m_blendFb);
            theRenderContext->setScissorTestEnabled(false);
            QVector4D color(0.0f, 0.0f, 0.0f, 0.0f);
            if (clear) {
                color.setX(clearColor.x());
                color.setY(clearColor.y());
                color.setZ(clearColor.z());
                color.setW(1.0f);
            }
            QVector4D origColor = theRenderContext->getClearColor();
            theRenderContext->setClearColor(color);
            theRenderContext->clear(QDemonRenderClearValues::Color);
            theRenderContext->setClearColor(origColor);
            theRenderContext->setRenderTarget(theFB);
            break;
        } else {
            m_layerBlendTexture.releaseTexture();
        }
#endif
    }

    for (iter = renderableLayers.rbegin(); iter != end; ++iter) {
        // Store the previous state of if we were rendering a layer.
        QDemonRenderLayer *theLayer = *iter;
        QSharedPointer<QDemonLayerRenderData> theRenderData = getOrCreateLayerRenderDataForNode(*theLayer, id);

        if (theRenderData) {
            if (theRenderData->layerPrepResult->isLayerVisible())
                theRenderData->runnableRenderToViewport(theFB);
        } else {
            Q_ASSERT(false);
        }
    }
}

QDemonRenderLayer *QDemonRendererImpl::getLayerForNode(const QDemonGraphNode &inNode) const
{
    if (inNode.type == QDemonGraphObjectTypes::Layer)
        return &const_cast<QDemonRenderLayer &>(static_cast<const QDemonRenderLayer &>(inNode));

    if (inNode.parent)
        return getLayerForNode(*inNode.parent);

    return nullptr;
}

QSharedPointer<QDemonLayerRenderData> QDemonRendererImpl::getOrCreateLayerRenderDataForNode(const QDemonGraphNode &inNode, const QDemonRenderInstanceId id)
{
    const QDemonRenderLayer *theLayer = getLayerForNode(inNode);
    if (theLayer) {
        TInstanceRenderMap::const_iterator theIter = m_instanceRenderMap.find(combineLayerAndId(theLayer, id));
        if (theIter != m_instanceRenderMap.end())
            return QSharedPointer<QDemonLayerRenderData>(theIter.value());

        QSharedPointer<QDemonLayerRenderData> theRenderData = QSharedPointer<QDemonLayerRenderData>(new QDemonLayerRenderData(
                                                                                              const_cast<QDemonRenderLayer &>(*theLayer), sharedFromThis()));
        m_instanceRenderMap.insert(combineLayerAndId(theLayer, id), theRenderData);

        // create a profiler if enabled
        if (isLayerGpuProfilingEnabled() && theRenderData)
            theRenderData->createGpuProfiler();

        return theRenderData;
    }
    return nullptr;
}

QDemonRenderCamera *QDemonRendererImpl::getCameraForNode(const QDemonGraphNode &inNode) const
{
    QSharedPointer<QDemonLayerRenderData> theLayer =
            const_cast<QDemonRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theLayer)
        return theLayer->camera;
    return nullptr;
}

QDemonOption<QDemonCuboidRect> QDemonRendererImpl::getCameraBounds(const QDemonGraphObject &inObject)
{
    if (QDemonGraphObjectTypes::IsNodeType(inObject.type)) {
        const QDemonGraphNode &theNode = static_cast<const QDemonGraphNode &>(inObject);
        QSharedPointer<QDemonLayerRenderData> theLayer = getOrCreateLayerRenderDataForNode(theNode);
        if (theLayer->getOffscreenRenderer() == false) {
            QDemonRenderCamera *theCamera = theLayer->camera;
            if (theCamera)
                return theCamera->getCameraBounds(
                            theLayer->layerPrepResult->getLayerToPresentationViewport(),
                            theLayer->layerPrepResult->getPresentationDesignDimensions());
        }
    }
    return QDemonOption<QDemonCuboidRect>();
}

void QDemonRendererImpl::drawScreenRect(QRectF inRect, const QVector3D &inColor)
{
    QDemonRenderCamera theScreenCamera;
    theScreenCamera.markDirty(NodeTransformDirtyFlag::TransformIsDirty);
    QRectF theViewport(m_context->getViewport());
    theScreenCamera.flags.setOrthographic(true);
    theScreenCamera.calculateGlobalVariables(theViewport, QVector2D(theViewport.width(), theViewport.height()));
    generateXYQuad();
    if (!m_screenRectShader) {
        QSharedPointer<QDemonShaderProgramGeneratorInterface> theGenerator(getProgramGenerator());
        theGenerator->beginProgram();
        QDemonShaderStageGeneratorInterface &vertexGenerator(
                    *theGenerator->getStage(ShaderGeneratorStages::Vertex));
        QDemonShaderStageGeneratorInterface &fragmentGenerator(
                    *theGenerator->getStage(ShaderGeneratorStages::Fragment));
        // TODO: Move out and change type!
        vertexGenerator.addIncoming(QLatin1String("attr_pos"), QLatin1String("vec3"));
        vertexGenerator.addUniform(QLatin1String("model_view_projection"), QLatin1String("mat4"));
        vertexGenerator.addUniform(QLatin1String("rectangle_dims"), QLatin1String("vec3"));
        vertexGenerator.append(QLatin1String("void main() {"));
        vertexGenerator.append(QLatin1String("\tgl_Position = model_view_projection * vec4(attr_pos * rectangle_dims, 1.0);"));
        vertexGenerator.append(QLatin1String("}"));
        fragmentGenerator.addUniform(QLatin1String("output_color"), QLatin1String("vec3"));
        fragmentGenerator.append(QLatin1String("void main() {"));
        fragmentGenerator.append(QLatin1String("\tgl_FragColor.rgb = output_color;"));
        fragmentGenerator.append(QLatin1String("\tgl_FragColor.a = 1.0;"));
        fragmentGenerator.append(QLatin1String("}"));
        // No flags enabled
        m_screenRectShader = theGenerator->compileGeneratedShader(QLatin1String("DrawScreenRect"), QDemonShaderCacheProgramFlags(), TShaderFeatureSet());
    }
    if (m_screenRectShader) {
        // Fudge the rect by one pixel to ensure we see all the corners.
        if (inRect.width() > 1)
            inRect.setWidth(inRect.width() - 1);
        if (inRect.height() > 1)
            inRect.setHeight(inRect.height() - 1);
        inRect.setX(inRect.x() + 1);
        inRect.setY(inRect.y() + 1);
        // Figure out the rect center.
        QDemonGraphNode theNode;

        const QPointF &center = inRect.center();
        QVector2D rectGlobalCenter = {float(center.x()), float(center.y())};
        QVector2D rectCenter(toNormalizedRectRelative(theViewport, rectGlobalCenter));
        theNode.position.setX(rectCenter.x());
        theNode.position.setY(rectCenter.y());
        theNode.markDirty(NodeTransformDirtyFlag::TransformIsDirty);
        theNode.calculateGlobalVariables();
        QMatrix4x4 theViewProjection;
        theScreenCamera.calculateViewProjectionMatrix(theViewProjection);
        QMatrix4x4 theMVP;
        QMatrix3x3 theNormal;
        theNode.calculateMVPAndNormalMatrix(theViewProjection, theMVP, theNormal);
        m_context->setBlendingEnabled(false);
        m_context->setDepthWriteEnabled(false);
        m_context->setDepthTestEnabled(false);
        m_context->setCullingEnabled(false);
        m_context->setActiveShader(m_screenRectShader);
        m_screenRectShader->setPropertyValue("model_view_projection", theMVP);
        m_screenRectShader->setPropertyValue("output_color", inColor);
        m_screenRectShader->setPropertyValue("rectangle_dims", QVector3D(inRect.width() / 2.0f, inRect.height() / 2.0f, 0.0f));
    }
    if (!m_rectInputAssembler) {
        Q_ASSERT(m_quadVertexBuffer);
        quint8 indexData[] = { 0, 1, 1, 2, 2, 3, 3, 0 };

        m_rectIndexBuffer = m_context->createIndexBuffer(
                    QDemonRenderBufferUsageType::Static,
                    QDemonRenderComponentTypes::UnsignedInteger8, sizeof(indexData),
                    toConstDataRef(indexData, sizeof(indexData)));

        QDemonRenderVertexBufferEntry theEntries[] = {
            QDemonRenderVertexBufferEntry("attr_pos",
            QDemonRenderComponentTypes::Float32, 3),
        };

        // create our attribute layout
        m_rectAttribLayout = m_context->createAttributeLayout(toConstDataRef(theEntries, 1));

        quint32 strides = m_quadVertexBuffer->getStride();
        quint32 offsets = 0;
        m_rectInputAssembler = m_context->createInputAssembler(
                    m_rectAttribLayout, toConstDataRef(&m_quadVertexBuffer, 1), m_rectIndexBuffer,
                    toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    }

    m_context->setInputAssembler(m_rectInputAssembler);
    m_context->draw(QDemonRenderDrawMode::Lines, m_rectIndexBuffer->getNumIndices(), 0);
}

void QDemonRendererImpl::setupWidgetLayer()
{
    QSharedPointer<QDemonRenderContext> theContext = m_demonContext->getRenderContext();

    if (!m_widgetTexture) {
        QSharedPointer<QDemonResourceManagerInterface> theManager = m_demonContext->getResourceManager();
        m_widgetTexture = theManager->allocateTexture2D(m_beginFrameViewport.width(),
                                                        m_beginFrameViewport.height(),
                                                        QDemonRenderTextureFormats::RGBA8);
        m_widgetFbo = theManager->allocateFrameBuffer();
        m_widgetFbo->attach(QDemonRenderFrameBufferAttachments::Color0,
                            QDemonRenderTextureOrRenderBuffer(m_widgetTexture));
        theContext->setRenderTarget(m_widgetFbo);

        // QDemonRenderRect theScissorRect( 0, 0, m_BeginFrameViewport.m_Width,
        // m_BeginFrameViewport.m_Height );
        // QDemonRenderContextScopedProperty<QDemonRenderRect> __scissorRect( theContext,
        // &QDemonRenderContext::GetScissorRect, &QDemonRenderContext::SetScissorRect, theScissorRect );
        QDemonRenderContextScopedProperty<bool> __scissorEnabled(
                    *theContext, &QDemonRenderContext::isScissorTestEnabled,
                    &QDemonRenderContext::setScissorTestEnabled, false);
        m_context->setClearColor(QVector4D(0, 0, 0, 0));
        m_context->clear(QDemonRenderClearValues::Color);

    } else
        theContext->setRenderTarget(m_widgetFbo);
}

void QDemonRendererImpl::beginFrame()
{
    for (quint32 idx = 0, end = m_lastFrameLayers.size(); idx < end; ++idx)
        m_lastFrameLayers[idx]->resetForFrame();
    m_lastFrameLayers.clear();
    m_beginFrameViewport = m_demonContext->getRenderList()->getViewport();
}
void QDemonRendererImpl::endFrame()
{
    if (m_widgetTexture) {
        // Releasing the widget FBO can set it as the active frame buffer.
        QDemonRenderContextScopedProperty<QSharedPointer<QDemonRenderFrameBuffer>> __fbo(
                    *m_context, &QDemonRenderContext::getRenderTarget, &QDemonRenderContext::setRenderTarget);
        QDemonTextureDetails theDetails = m_widgetTexture->getTextureDetails();
        m_context->setBlendingEnabled(true);
        // Colors are expected to be non-premultiplied, so we premultiply alpha into them at
        // this point.
        m_context->setBlendFunction(QDemonRenderBlendFunctionArgument(
                                        QDemonRenderSrcBlendFunc::One, QDemonRenderDstBlendFunc::OneMinusSrcAlpha,
                                        QDemonRenderSrcBlendFunc::One, QDemonRenderDstBlendFunc::OneMinusSrcAlpha));
        m_context->setBlendEquation(QDemonRenderBlendEquationArgument(
                                        QDemonRenderBlendEquation::Add, QDemonRenderBlendEquation::Add));

        m_context->setDepthTestEnabled(false);
        m_context->setScissorTestEnabled(false);
        m_context->setViewport(m_beginFrameViewport);
        QDemonRenderCamera theCamera;
        theCamera.markDirty(NodeTransformDirtyFlag::TransformIsDirty);
        theCamera.flags.setOrthographic(true);
        QVector2D theTextureDims((float)theDetails.width, (float)theDetails.height);
        theCamera.calculateGlobalVariables(QRect(0, 0, theDetails.width, theDetails.height), theTextureDims);
        QMatrix4x4 theViewProj;
        theCamera.calculateViewProjectionMatrix(theViewProj);
        renderQuad(theTextureDims, theViewProj, *m_widgetTexture);

        QSharedPointer<QDemonResourceManagerInterface> theManager(m_demonContext->getResourceManager());
        theManager->release(m_widgetFbo);
        theManager->release(m_widgetTexture);
        m_widgetTexture = nullptr;
        m_widgetFbo = nullptr;
    }
}

inline bool pickResultLessThan(const QDemonRenderPickResult &lhs, const QDemonRenderPickResult &rhs)
{
    return floatLessThan(lhs.m_cameraDistanceSq, rhs.m_cameraDistanceSq);
}

inline float clampUVCoord(float inUVCoord, QDemonRenderTextureCoordOp::Enum inCoordOp)
{
    if (inUVCoord > 1.0f || inUVCoord < 0.0f) {
        switch (inCoordOp) {
        default:
            Q_ASSERT(false);
            break;
        case QDemonRenderTextureCoordOp::ClampToEdge:
            inUVCoord = qMin(inUVCoord, 1.0f);
            inUVCoord = qMax(inUVCoord, 0.0f);
            break;
        case QDemonRenderTextureCoordOp::Repeat: {
            float multiplier = inUVCoord > 0.0f ? 1.0f : -1.0f;
            float clamp = std::fabs(inUVCoord);
            clamp = clamp - std::floor(clamp);
            if (multiplier < 0)
                inUVCoord = 1.0f - clamp;
            else
                inUVCoord = clamp;
        } break;
        case QDemonRenderTextureCoordOp::MirroredRepeat: {
            float multiplier = inUVCoord > 0.0f ? 1.0f : -1.0f;
            float clamp = std::fabs(inUVCoord);
            if (multiplier > 0.0f)
                clamp -= 1.0f;
            quint32 isMirrored = ((quint32)clamp) % 2 == 0;
            float remainder = clamp - std::floor(clamp);
            inUVCoord = remainder;
            if (isMirrored) {
                if (multiplier > 0.0f)
                    inUVCoord = 1.0f - inUVCoord;
            } else {
                if (multiplier < 0.0f)
                    inUVCoord = 1.0f - remainder;
            }
        } break;
        }
    }
    return inUVCoord;
}

static QPair<QVector2D, QVector2D> getMouseCoordsAndViewportFromSubObject(QVector2D inLocalHitUVSpace,
                                                                          QDemonRenderPickSubResult &inSubResult)
{
    QMatrix4x4 theTextureMatrix(inSubResult.m_textureMatrix);
    QVector3D theNewUVCoords(mat44::transform(theTextureMatrix, (QVector3D(inLocalHitUVSpace.x(), inLocalHitUVSpace.y(), 0))));
    theNewUVCoords.setX(clampUVCoord(theNewUVCoords.x(), inSubResult.m_horizontalTilingMode));
    theNewUVCoords.setY(clampUVCoord(theNewUVCoords.y(), inSubResult.m_verticalTilingMode));
    QVector2D theViewportDimensions = QVector2D(float(inSubResult.m_viewportWidth), float(inSubResult.m_viewportHeight));
    QVector2D theMouseCoords(theNewUVCoords.x() * theViewportDimensions.x(), (1.0f - theNewUVCoords.y()) * theViewportDimensions.y());

    return QPair<QVector2D, QVector2D>(theMouseCoords, theViewportDimensions);
}

QDemonPickResultProcessResult QDemonRendererImpl::processPickResultList(bool inPickEverything)
{
    if (m_lastPickResults.empty())
        return QDemonPickResultProcessResult();
    // Things are rendered in a particular order and we need to respect that ordering.
    std::stable_sort(m_lastPickResults.begin(), m_lastPickResults.end(), pickResultLessThan);

    // We need to pick against sub objects basically somewhat recursively
    // but if we don't hit any sub objects and the parent isn't pickable then
    // we need to move onto the next item in the list.
    // We need to keep in mind that theQuery->Pick will enter this method in a later
    // stack frame so *if* we get to sub objects we need to pick against them but if the pick
    // completely misses *and* the parent object locally pickable is false then we need to move
    // onto the next object.

    quint32 numToCopy = (quint32)m_lastPickResults.size();
    quint32 numCopyBytes = numToCopy * sizeof(QDemonRenderPickResult);
    QDemonRenderPickResult *thePickResults = reinterpret_cast<QDemonRenderPickResult *>(::malloc(numCopyBytes));
    ::memcpy(thePickResults, m_lastPickResults.data(), numCopyBytes);
    m_lastPickResults.clear();
    bool foundValidResult = false;
    QDemonPickResultProcessResult thePickResult(thePickResults[0]);
    for (size_t idx = 0; idx < numToCopy && foundValidResult == false; ++idx) {
        thePickResult = thePickResults[idx];
        // Here we do a hierarchy.  Picking against sub objects takes precedence.
        // If picking against the sub object doesn't return a valid result *and*
        // the current object isn't globally pickable then we move onto the next object returned
        // by the pick query.
        if (thePickResult.m_hitObject != nullptr && thePickResult.m_firstSubObject != nullptr && m_pickRenderPlugins) {
            QVector2D theUVCoords(thePickResult.m_localUVCoords.x(), thePickResult.m_localUVCoords.y());
            QSharedPointer<QDemonOffscreenRendererInterface> theSubRenderer(thePickResult.m_firstSubObject->m_subRenderer);
            QPair<QVector2D, QVector2D> mouseAndViewport = getMouseCoordsAndViewportFromSubObject(theUVCoords,
                                                                                                  *thePickResult.m_firstSubObject);
            QVector2D theMouseCoords = mouseAndViewport.first;
            QVector2D theViewportDimensions = mouseAndViewport.second;
            QDemonGraphObjectPickQueryInterface *theQuery = theSubRenderer->getGraphObjectPickQuery(this);
            if (theQuery) {
                QDemonRenderPickResult theInnerPickResult = theQuery->pick(theMouseCoords, theViewportDimensions, inPickEverything);
                if (theInnerPickResult.m_hitObject) {
                    thePickResult = theInnerPickResult;
                    thePickResult.m_offscreenRenderer = theSubRenderer;
                    foundValidResult = true;
                    thePickResult.m_wasPickConsumed = true;
                } else if (QDemonGraphObjectTypes::IsNodeType(thePickResult.m_hitObject->type)) {
                    const QDemonGraphNode *theNode = static_cast<const QDemonGraphNode *>(thePickResult.m_hitObject);
                    if (theNode->flags.isGloballyPickable() == true) {
                        foundValidResult = true;
                        thePickResult.m_wasPickConsumed = true;
                    }
                }
            } else {
                // If the sub renderer doesn't consume the pick then we return the picked object
                // itself.  So no matter what, if we get to here the pick was consumed.
                thePickResult.m_wasPickConsumed = true;
                bool wasPickConsumed = theSubRenderer->pick(theMouseCoords, theViewportDimensions, this);
                if (wasPickConsumed) {
                    thePickResult.m_hitObject = nullptr;
                    foundValidResult = true;
                }
            }
        } else {
            foundValidResult = true;
            thePickResult.m_wasPickConsumed = true;
        }
    }
    return thePickResult;
}

QDemonRenderPickResult QDemonRendererImpl::pick(QDemonRenderLayer &inLayer,
                                                const QVector2D &inViewportDimensions,
                                                const QVector2D &inMouseCoords,
                                                bool inPickSiblings,
                                                bool inPickEverything,
                                                const QDemonRenderInstanceId id)
{
    m_lastPickResults.clear();

    QDemonRenderLayer *theLayer = &inLayer;
    // Stepping through how the original runtime did picking it picked layers in order
    // stopping at the first hit.  So objects on the top layer had first crack at the pick
    // vector itself.
    do {
        if (theLayer->flags.isActive()) {
            TInstanceRenderMap::iterator theIter = m_instanceRenderMap.find(combineLayerAndId(theLayer, id));
            if (theIter != m_instanceRenderMap.end()) {
                m_lastPickResults.clear();
                getLayerHitObjectList(*theIter.value(), inViewportDimensions, inMouseCoords, inPickEverything, m_lastPickResults);
                QDemonPickResultProcessResult retval(processPickResultList(inPickEverything));
                if (retval.m_wasPickConsumed)
                    return std::move(retval);
            } else {
                // Q_ASSERT( false );
            }
        }

        if (inPickSiblings)
            theLayer = getNextLayer(*theLayer);
        else
            theLayer = nullptr;
    } while (theLayer != nullptr);

    return QDemonRenderPickResult();
}

static inline QDemonOption<QVector2D> intersectRayWithNode(const QDemonGraphNode &inNode,
                                                           QDemonRenderableObject &inRenderableObject,
                                                           const QDemonRenderRay &inPickRay)
{
    if (inRenderableObject.renderableFlags.IsText()) {
        QDemonTextRenderable &theRenderable = static_cast<QDemonTextRenderable &>(inRenderableObject);
        if (&theRenderable.text == &inNode)
            return inPickRay.getRelativeXY(inRenderableObject.globalTransform,
                                           inRenderableObject.bounds);
    } else if (inRenderableObject.renderableFlags.isDefaultMaterialMeshSubset()) {
        QDemonSubsetRenderable &theRenderable = static_cast<QDemonSubsetRenderable &>(inRenderableObject);
        if (&theRenderable.modelContext.model == &inNode)
            return inPickRay.getRelativeXY(inRenderableObject.globalTransform,
                                           inRenderableObject.bounds);
    } else if (inRenderableObject.renderableFlags.isCustomMaterialMeshSubset()) {
        QDemonCustomMaterialRenderable &theRenderable =
                static_cast<QDemonCustomMaterialRenderable &>(inRenderableObject);
        if (&theRenderable.modelContext.model == &inNode)
            return inPickRay.getRelativeXY(inRenderableObject.globalTransform,
                                           inRenderableObject.bounds);
    } else {
        Q_ASSERT(false);
    }
    return QDemonEmpty();
}

static inline QDemonRenderPickSubResult constructSubResult(QDemonRenderImage &inImage)
{
    QDemonTextureDetails theDetails = inImage.m_textureData.m_texture->getTextureDetails();
    return QDemonRenderPickSubResult(inImage.m_lastFrameOffscreenRenderer,
                                     inImage.m_textureTransform, inImage.m_horizontalTilingMode,
                                     inImage.m_verticalTilingMode, theDetails.width,
                                     theDetails.height);
}

QDemonOption<QVector2D> QDemonRendererImpl::facePosition(QDemonGraphNode &inNode, QDemonBounds3 inBounds,
                                                         const QMatrix4x4 &inGlobalTransform,
                                                         const QVector2D &inViewportDimensions,
                                                         const QVector2D &inMouseCoords,
                                                         QDemonDataRef<QDemonGraphObject *> inMapperObjects,
                                                         QDemonRenderBasisPlanes::Enum inPlane)
{
    QSharedPointer<QDemonLayerRenderData> theLayerData = getOrCreateLayerRenderDataForNode(inNode);
    if (theLayerData == nullptr)
        return QDemonEmpty();
    // This function assumes the layer was rendered to the scene itself.  There is another
    // function
    // for completely offscreen layers that don't get rendered to the scene.
    bool wasRenderToTarget(theLayerData->layer.flags.isLayerRenderToTarget());
    if (wasRenderToTarget == false || theLayerData->camera == nullptr
            || theLayerData->layerPrepResult.hasValue() == false
            || theLayerData->lastFrameOffscreenRenderer != nullptr)
        return QDemonEmpty();

    QVector2D theMouseCoords(inMouseCoords);
    QVector2D theViewportDimensions(inViewportDimensions);

    for (quint32 idx = 0, end = inMapperObjects.size(); idx < end; ++idx) {
        QDemonGraphObject &currentObject = *inMapperObjects[idx];
        if (currentObject.type == QDemonGraphObjectTypes::Layer) {
            // The layer knows its viewport so it can take the information directly.
            // This is extremely counter intuitive but a good sign.
        } else if (currentObject.type == QDemonGraphObjectTypes::Image) {
            QDemonRenderImage &theImage = static_cast<QDemonRenderImage &>(currentObject);
            QDemonRenderModel *theParentModel = nullptr;
            if (theImage.m_parent
                    && theImage.m_parent->type == QDemonGraphObjectTypes::DefaultMaterial) {
                QDemonRenderDefaultMaterial *theMaterial =
                        static_cast<QDemonRenderDefaultMaterial *>(theImage.m_parent);
                if (theMaterial) {
                    theParentModel = theMaterial->parent;
                }
            }
            if (theParentModel == nullptr) {
                Q_ASSERT(false);
                return QDemonEmpty();
            }
            QDemonBounds3 theModelBounds = theParentModel->getBounds(
                        getDemonContext()->getBufferManager(), getDemonContext()->getPathManager(), false);

            if (theModelBounds.isEmpty()) {
                Q_ASSERT(false);
                return QDemonEmpty();
            }
            QDemonOption<QVector2D> relativeHit =
                    facePosition(*theParentModel, theModelBounds, theParentModel->globalTransform,
                                 theViewportDimensions, theMouseCoords, QDemonDataRef<QDemonGraphObject *>(),
                                 QDemonRenderBasisPlanes::XY);
            if (relativeHit.isEmpty()) {
                return QDemonEmpty();
            }
            QDemonRenderPickSubResult theResult = constructSubResult(theImage);
            QVector2D hitInUVSpace = (*relativeHit) + QVector2D(.5f, .5f);
            QPair<QVector2D, QVector2D> mouseAndViewport =
                    getMouseCoordsAndViewportFromSubObject(hitInUVSpace, theResult);
            theMouseCoords = mouseAndViewport.first;
            theViewportDimensions = mouseAndViewport.second;
        }
    }

    QDemonOption<QDemonRenderRay> theHitRay = theLayerData->layerPrepResult->getPickRay(theMouseCoords, theViewportDimensions, false);
    if (theHitRay.hasValue() == false)
        return QDemonEmpty();

    // Scale the mouse coords to change them into the camera's numerical space.
    QDemonRenderRay thePickRay = *theHitRay;
    QDemonOption<QVector2D> newValue = thePickRay.getRelative(inGlobalTransform, inBounds, inPlane);
    return newValue;
}

QDemonRenderPickResult QDemonRendererImpl::pickOffscreenLayer(QDemonRenderLayer &/*inLayer*/,
                                                              const QVector2D & /*inViewportDimensions*/,
                                                              const QVector2D & /*inMouseCoords*/,
                                                              bool /*inPickEverything*/)
{
    return QDemonRenderPickResult();
}

QVector3D QDemonRendererImpl::unprojectToPosition(QDemonGraphNode &inNode, QVector3D &inPosition,
                                                  const QVector2D &inMouseVec) const
{
    // Translate mouse into layer's coordinates
    QSharedPointer<QDemonLayerRenderData> theData =
            const_cast<QDemonRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theData == nullptr || theData->camera == nullptr) {
        return QVector3D(0, 0, 0);
    } // Q_ASSERT( false ); return QVector3D(0,0,0); }

    QSize theWindow = m_demonContext->getWindowDimensions();
    QVector2D theDims((float)theWindow.width(), (float)theWindow.height());

    QDemonLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    QDemonRenderRay theRay = thePrepResult.getPickRay(inMouseVec, theDims, true);

    return theData->camera->unprojectToPosition(inPosition, theRay);
}

QVector3D QDemonRendererImpl::unprojectWithDepth(QDemonGraphNode &inNode, QVector3D &,
                                                 const QVector3D &inMouseVec) const
{
    // Translate mouse into layer's coordinates
    QSharedPointer<QDemonLayerRenderData> theData = const_cast<QDemonRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theData == nullptr || theData->camera == nullptr) {
        return QVector3D(0, 0, 0);
    } // Q_ASSERT( false ); return QVector3D(0,0,0); }

    // Flip the y into gl coordinates from window coordinates.
    QVector2D theMouse(inMouseVec.x(), inMouseVec.y());
    float theDepth = inMouseVec.z();

    QDemonLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    QSize theWindow = m_demonContext->getWindowDimensions();
    QDemonRenderRay theRay = thePrepResult.getPickRay(theMouse, QVector2D((float)theWindow.width(), (float)theWindow.height()), true);
    QVector3D theTargetPosition = theRay.m_origin + theRay.m_direction * theDepth;
    if (inNode.parent != nullptr && inNode.parent->type != QDemonGraphObjectTypes::Layer)
        theTargetPosition = mat44::transform(mat44::getInverse(inNode.parent->globalTransform), theTargetPosition);
    // Our default global space is right handed, so if you are left handed z means something
    // opposite.
    if (inNode.flags.isLeftHanded())
        theTargetPosition.setZ(theTargetPosition.z() * -1);
    return theTargetPosition;
}

QVector3D QDemonRendererImpl::projectPosition(QDemonGraphNode &inNode, const QVector3D &inPosition) const
{
    // Translate mouse into layer's coordinates
    QSharedPointer<QDemonLayerRenderData> theData =
            const_cast<QDemonRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theData == nullptr || theData->camera == nullptr) {
        return QVector3D(0, 0, 0);
    }

    QMatrix4x4 viewProj;
    theData->camera->calculateViewProjectionMatrix(viewProj);
    QVector4D projPos = mat44::transform(viewProj, QVector4D(inPosition, 1.0f));
    projPos.setX(projPos.x() / projPos.w());
    projPos.setY(projPos.y() / projPos.w());

    QRectF theViewport = theData->layerPrepResult->getLayerToPresentationViewport();
    QVector2D theDims((float)theViewport.width(), (float)theViewport.height());
    projPos.setX(projPos.x() + 1.0);
    projPos.setY(projPos.y() + 1.0);
    projPos.setX(projPos.x() * 0.5);
    projPos.setY(projPos.y() * 0.5);
    QVector3D cameraToObject = theData->camera->getGlobalPos() - inPosition;
    projPos.setZ(sqrtf(QVector3D::dotProduct(cameraToObject, cameraToObject)));
    QVector3D mouseVec = QVector3D(projPos.x(), projPos.y(), projPos.z());
    mouseVec.setX(mouseVec.x() * theDims.x());
    mouseVec.setY(mouseVec.y() * theDims.y());

    mouseVec.setX(mouseVec.x() + theViewport.x());
    mouseVec.setY(mouseVec.y() + theViewport.y());

    // Flip the y into window coordinates so it matches the mouse.
    QSize theWindow = m_demonContext->getWindowDimensions();
    mouseVec.setY(theWindow.height() - mouseVec.y());

    return mouseVec;
}

QDemonOption<QDemonLayerPickSetup> QDemonRendererImpl::getLayerPickSetup(QDemonRenderLayer &inLayer,
                                                                    const QVector2D &inMouseCoords,
                                                                    const QSize &inPickDims)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (theData == nullptr || theData->camera == nullptr) {
        Q_ASSERT(false);
        return QDemonEmpty();
    }
    QSize theWindow = m_demonContext->getWindowDimensions();
    QVector2D theDims((float)theWindow.width(), (float)theWindow.height());
    // The mouse is relative to the layer
    QDemonOption<QVector2D> theLocalMouse = getLayerMouseCoords(*theData, inMouseCoords, theDims, false);
    if (theLocalMouse.hasValue() == false) {
        return QDemonEmpty();
    }

    QDemonLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    if (thePrepResult.getCamera() == nullptr) {
        return QDemonEmpty();
    }
    // Perform gluPickMatrix and pre-multiply it into the view projection
    QMatrix4x4 theTransScale;
    QDemonRenderCamera &theCamera(*thePrepResult.getCamera());

    QRectF layerToPresentation = thePrepResult.getLayerToPresentationViewport();
    // Offsetting is already taken care of in the camera's projection.
    // All we need to do is to scale and translate the image.
    layerToPresentation.setX(0);
    layerToPresentation.setY(0);
    QVector2D theMouse(*theLocalMouse);
    // The viewport will need to center at this location
    QVector2D viewportDims((float)inPickDims.width(), (float)inPickDims.height());
    QVector2D bottomLeft = QVector2D(theMouse.x() - viewportDims.x() / 2.0f, theMouse.y() - viewportDims.y() / 2.0f);
    // For some reason, and I haven't figured out why yet, the offsets need to be backwards for
    // this to work.
    // bottomLeft.x = layerToPresentation.m_Width - bottomLeft.x;
    // bottomLeft.y = layerToPresentation.m_Height - bottomLeft.y;
    // Virtual rect is relative to the layer.
    QRectF thePickRect(bottomLeft.x(), bottomLeft.y(), viewportDims.x(), viewportDims.y());
    QMatrix4x4 projectionPremult;
    projectionPremult = QDemonRenderContext::applyVirtualViewportToProjectionMatrix(projectionPremult, layerToPresentation, thePickRect);
    projectionPremult = mat44::getInverse(projectionPremult);

    QMatrix4x4 globalInverse = mat44::getInverse(theCamera.globalTransform);
    QMatrix4x4 theVP = theCamera.projection * globalInverse;
    // For now we won't setup the scissor, so we may be off by inPickDims at most because
    // GetLayerMouseCoords will return
    // false if the mouse is too far off the layer.
    return QDemonLayerPickSetup(projectionPremult, theVP, QRect(0, 0, (quint32)layerToPresentation.width(),
                                                                           (quint32)layerToPresentation.height()));
}

QDemonOption<QRectF> QDemonRendererImpl::getLayerRect(QDemonRenderLayer &inLayer)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (theData == nullptr || theData->camera == nullptr) {
        Q_ASSERT(false);
        return QDemonEmpty();
    }
    QDemonLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    return thePrepResult.getLayerToPresentationViewport();
}

// This doesn't have to be cheap.
void QDemonRendererImpl::runLayerRender(QDemonRenderLayer &inLayer, const QMatrix4x4 &inViewProjection)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (theData == nullptr || theData->camera == nullptr) {
        Q_ASSERT(false);
        return;
    }
    theData->prepareAndRender(inViewProjection);
}

void QDemonRendererImpl::addRenderWidget(QDemonRenderWidgetInterface &inWidget)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inWidget.getNode());
    if (theData)
        theData->addRenderWidget(inWidget);
}

void QDemonRendererImpl::renderLayerRect(QDemonRenderLayer &inLayer, const QVector3D &inColor)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (theData)
        theData->m_boundingRectColor = inColor;
}

QDemonScaleAndPosition QDemonRendererImpl::getWorldToPixelScaleFactor(const QDemonRenderCamera &inCamera,
                                                                 const QVector3D &inWorldPoint,
                                                                 QDemonLayerRenderData &inRenderData)
{
    if (inCamera.flags.isOrthographic() == true) {
        // There are situations where the camera can scale.
        return QDemonScaleAndPosition(inWorldPoint,
                                      inCamera.getOrthographicScaleFactor(
                                          inRenderData.layerPrepResult->getLayerToPresentationViewport(),
                                          inRenderData.layerPrepResult->getPresentationDesignDimensions()));
    } else {
        QVector3D theCameraPos(0, 0, 0);
        QVector3D theCameraDir(0, 0, -1);
        QDemonRenderRay theRay(theCameraPos, inWorldPoint - theCameraPos);
        QDemonPlane thePlane(theCameraDir, -600);
        QVector3D theItemPosition(inWorldPoint);
        QDemonOption<QVector3D> theIntersection = theRay.intersect(thePlane);
        if (theIntersection.hasValue())
            theItemPosition = *theIntersection;
        // The special number comes in from physically measuring how off we are on the screen.
        float theScaleFactor = (1.0f / inCamera.projection(1, 1));
        QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inCamera);
        quint32 theHeight = theData->layerPrepResult->getTextureDimensions().height();
        float theScaleMultiplier = 600.0f / ((float)theHeight / 2.0f);
        theScaleFactor *= theScaleMultiplier;

        return QDemonScaleAndPosition(theItemPosition, theScaleFactor);
    }
}

QDemonScaleAndPosition QDemonRendererImpl::getWorldToPixelScaleFactor(QDemonRenderLayer &inLayer,
                                                                 const QVector3D &inWorldPoint)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (theData == nullptr || theData->camera == nullptr) {
        Q_ASSERT(false);
        return QDemonScaleAndPosition();
    }
    return getWorldToPixelScaleFactor(*theData->camera, inWorldPoint, *theData);
}

void QDemonRendererImpl::releaseLayerRenderResources(QDemonRenderLayer &inLayer, const QDemonRenderInstanceId id)
{
    TInstanceRenderMap::iterator theIter = m_instanceRenderMap.find(combineLayerAndId(&inLayer, id));
    if (theIter != m_instanceRenderMap.end()) {
        TLayerRenderList::iterator theLastFrm = std::find(
                    m_lastFrameLayers.begin(), m_lastFrameLayers.end(), theIter.value());
        if (theLastFrm != m_lastFrameLayers.end()) {
            theIter.value()->resetForFrame();
            m_lastFrameLayers.erase(theLastFrm);
        }
        m_instanceRenderMap.erase(theIter);
    }
}

void QDemonRendererImpl::renderQuad(const QVector2D inDimensions, const QMatrix4x4 &inMVP,
                                    QDemonRenderTexture2D &inQuadTexture)
{
    m_context->setCullingEnabled(false);
    QSharedPointer<QDemonLayerSceneShader> theShader = getSceneLayerShader();
    QDemonRenderContext &theContext(*m_context);
    theContext.setActiveShader(theShader->shader);
    theShader->mvp.set(inMVP);
    theShader->dimensions.set(inDimensions);
    theShader->sampler.set(&inQuadTexture);

    generateXYQuad();
    theContext.setInputAssembler(m_quadInputAssembler);
    theContext.draw(QDemonRenderDrawMode::Triangles, m_quadIndexBuffer->getNumIndices(), 0);
}

void QDemonRendererImpl::renderQuad()
{
    m_context->setCullingEnabled(false);
    generateXYQuad();
    m_context->setInputAssembler(m_quadInputAssembler);
    m_context->draw(QDemonRenderDrawMode::Triangles, m_quadIndexBuffer->getNumIndices(), 0);
}

void QDemonRendererImpl::renderPointsIndirect()
{
    m_context->setCullingEnabled(false);
    generateXYZPoint();
    m_context->setInputAssembler(m_pointInputAssembler);
    m_context->drawIndirect(QDemonRenderDrawMode::Points, 0);
}

void QDemonRendererImpl::layerNeedsFrameClear(QDemonLayerRenderData &inLayer)
{
    m_lastFrameLayers.push_back(&inLayer);
}

void QDemonRendererImpl::beginLayerDepthPassRender(QDemonLayerRenderData &inLayer)
{
    m_currentLayer = &inLayer;
}

void QDemonRendererImpl::endLayerDepthPassRender() { m_currentLayer = nullptr; }

void QDemonRendererImpl::beginLayerRender(QDemonLayerRenderData &inLayer)
{
    m_currentLayer = &inLayer;
    // Remove all of the shaders from the layer shader set
    // so that we can only apply the camera and lighting properties to
    // shaders that are in the layer.
    m_layerShaders.clear();
}
void QDemonRendererImpl::endLayerRender() { m_currentLayer = nullptr; }

void QDemonRendererImpl::prepareImageForIbl(QDemonRenderImage &inImage)
{
    if (inImage.m_textureData.m_texture && inImage.m_textureData.m_texture->getNumMipmaps() < 1)
        inImage.m_textureData.m_texture->generateMipmaps();
}

bool nodeContainsBoneRoot(QDemonGraphNode &childNode, qint32 rootID)
{
    for (QDemonGraphNode *childChild = childNode.firstChild; childChild != nullptr;
         childChild = childChild->nextSibling) {
        if (childChild->skeletonId == rootID)
            return true;
    }

    return false;
}

void fillBoneIdNodeMap(QDemonGraphNode &childNode, QHash<long, QDemonGraphNode *> &ioMap)
{
    if (childNode.skeletonId >= 0)
        ioMap[childNode.skeletonId] = &childNode;
    for (QDemonGraphNode *childChild = childNode.firstChild; childChild != nullptr;
         childChild = childChild->nextSibling)
        fillBoneIdNodeMap(*childChild, ioMap);
}

bool QDemonRendererImpl::prepareTextureAtlasForRender()
{
    QSharedPointer<QDemonTextTextureAtlasInterface> theTextureAtlas = m_demonContext->getTextureAtlas();
    if (theTextureAtlas == nullptr)
        return false;

    // this is a one time creation
    if (!theTextureAtlas->isInitialized()) {
        QDemonRenderContext &theContext(*m_context);
        QSharedPointer<QDemonRenderVertexBuffer> mVertexBuffer;
        QSharedPointer<QDemonRenderInputAssembler> mInputAssembler;
        QSharedPointer<QDemonRenderAttribLayout> mAttribLayout;
        // temporay FB
        QDemonRenderContextScopedProperty<QSharedPointer<QDemonRenderFrameBuffer>> __fbo(
                    *m_context, &QDemonRenderContext::getRenderTarget, &QDemonRenderContext::setRenderTarget);

        QDemonTextRendererInterface &theTextRenderer(*m_demonContext->getOnscreenTextRenderer());
        TTextTextureAtlasDetailsAndTexture theResult = theTextureAtlas->prepareTextureAtlas();
        if (!theResult.first.entryCount) {
            Q_ASSERT(theResult.first.entryCount);
            return false;
        }

        // generate the index buffer we need
        generateXYQuad();

        QDemonRenderVertexBufferEntry theEntries[] = {
            QDemonRenderVertexBufferEntry("attr_pos",
            QDemonRenderComponentTypes::Float32, 3),
            QDemonRenderVertexBufferEntry(
            "attr_uv", QDemonRenderComponentTypes::Float32, 2, 12),
        };

        // create our attribute layout
        mAttribLayout = m_context->createAttributeLayout(toConstDataRef(theEntries, 2));

        QSharedPointer<QDemonRenderFrameBuffer> theAtlasFB(m_demonContext->getResourceManager()->allocateFrameBuffer());
        theAtlasFB->attach(QDemonRenderFrameBufferAttachments::Color0, theResult.second);
        m_demonContext->getRenderContext()->setRenderTarget(theAtlasFB);

        // this texture contains our single entries
        QSharedPointer<QDemonRenderTexture2D> theTexture = nullptr;
        if (m_context->getRenderContextType() == QDemonRenderContextValues::GLES2) {
            theTexture = m_demonContext->getResourceManager()->allocateTexture2D(32, 32, QDemonRenderTextureFormats::RGBA8);
        } else {
            theTexture = m_demonContext->getResourceManager()->allocateTexture2D(32, 32, QDemonRenderTextureFormats::Alpha8);
        }
        m_context->setClearColor(QVector4D(0, 0, 0, 0));
        m_context->clear(QDemonRenderClearValues::Color);
        m_context->setDepthTestEnabled(false);
        m_context->setScissorTestEnabled(false);
        m_context->setCullingEnabled(false);
        m_context->setBlendingEnabled(false);
        m_context->setViewport(QRect(0, 0, theResult.first.textWidth, theResult.first.textHeight));

        QDemonRenderCamera theCamera;
        theCamera.clipNear = -1.0;
        theCamera.clipFar = 1.0;
        theCamera.markDirty(NodeTransformDirtyFlag::TransformIsDirty);
        theCamera.flags.setOrthographic(true);
        QVector2D theTextureDims((float)theResult.first.textWidth,
                                 (float)theResult.first.textHeight);
        theCamera.calculateGlobalVariables(QRect(0, 0, theResult.first.textWidth, theResult.first.textHeight), theTextureDims);
        // We want a 2D lower left projection
        float *writePtr(theCamera.projection.data());
        writePtr[12] = -1;
        writePtr[13] = -1;

        // generate render stuff
        // We dynamicall update the vertex buffer
        float tempBuf[20];
        float *bufPtr = tempBuf;
        quint32 bufSize = 20 * sizeof(float); // 4 vertices  3 pos 2 tex
        QDemonDataRef<quint8> vertData((quint8 *)bufPtr, bufSize);
        mVertexBuffer = theContext.createVertexBuffer(QDemonRenderBufferUsageType::Dynamic,
                                                      20 * sizeof(float),
                                                      3 * sizeof(float) + 2 * sizeof(float),
                                                      vertData);
        quint32 strides = mVertexBuffer->getStride();
        quint32 offsets = 0;
        mInputAssembler = theContext.createInputAssembler(
                    mAttribLayout, toConstDataRef(&mVertexBuffer, 1), m_quadIndexBuffer,
                    toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));

        QSharedPointer<QDemonRenderShaderProgram> theShader = getTextAtlasEntryShader();
        QDemonTextShader theTextShader(theShader);

        if (theShader) {
            theContext.setActiveShader(theShader);
            theTextShader.mvp.set(theCamera.projection);

            // we are going through all entries and render to the FBO
            for (quint32 i = 0; i < theResult.first.entryCount; i++) {
                QDemonTextTextureAtlasEntryDetails theDetails =
                        theTextRenderer.renderAtlasEntry(i, *theTexture);
                // update vbo
                // we need to mirror coordinates
                float x1 = (float)theDetails.x;
                float x2 = (float)theDetails.x + theDetails.textWidth;
                float y1 = (float)theDetails.y;
                float y2 = (float)theDetails.y + theDetails.textHeight;

                float box[4][5] = {
                    { x1, y1, 0, 0, 1 },
                    { x1, y2, 0, 0, 0 },
                    { x2, y2, 0, 1, 0 },
                    { x2, y1, 0, 1, 1 },
                };

                QDemonDataRef<quint8> vertData((quint8 *)box, bufSize);
                mVertexBuffer->updateBuffer(vertData, false);

                theTextShader.sampler.set(theTexture.data());

                theContext.setInputAssembler(mInputAssembler);
                theContext.draw(QDemonRenderDrawMode::Triangles, m_quadIndexBuffer->getNumIndices(),
                                0);
            }
        }

        m_demonContext->getResourceManager()->release(theTexture);
        m_demonContext->getResourceManager()->release(theAtlasFB);

        return true;
    }

    return theTextureAtlas->isInitialized();
}

QDemonOption<QVector2D> QDemonRendererImpl::getLayerMouseCoords(QDemonLayerRenderData &inLayerRenderData,
                                                                const QVector2D &inMouseCoords,
                                                                const QVector2D &inViewportDimensions,
                                                                bool forceImageIntersect) const
{
    if (inLayerRenderData.layerPrepResult.hasValue())
        return inLayerRenderData.layerPrepResult->getLayerMouseCoords(
                    inMouseCoords, inViewportDimensions, forceImageIntersect);
    return QDemonEmpty();
}

void QDemonRendererImpl::getLayerHitObjectList(QDemonLayerRenderData &inLayerRenderData,
                                               const QVector2D &inViewportDimensions,
                                               const QVector2D &inPresCoords, bool inPickEverything,
                                               TPickResultArray &outIntersectionResult)
{
    // This function assumes the layer was rendered to the scene itself.  There is another
    // function
    // for completely offscreen layers that don't get rendered to the scene.
    bool wasRenderToTarget(inLayerRenderData.layer.flags.isLayerRenderToTarget());
    if (wasRenderToTarget && inLayerRenderData.camera != nullptr) {
        QDemonOption<QDemonRenderRay> theHitRay;
        if (inLayerRenderData.layerPrepResult.hasValue())
            theHitRay = inLayerRenderData.layerPrepResult->getPickRay(
                        inPresCoords, inViewportDimensions, false);
        if (inLayerRenderData.lastFrameOffscreenRenderer == nullptr) {
            if (theHitRay.hasValue()) {
                // Scale the mouse coords to change them into the camera's numerical space.
                QDemonRenderRay thePickRay = *theHitRay;
                for (quint32 idx = inLayerRenderData.opaqueObjects.size(), end = 0; idx > end;
                     --idx) {
                    QDemonRenderableObject *theRenderableObject =
                            inLayerRenderData.opaqueObjects[idx - 1];
                    if (inPickEverything
                            || theRenderableObject->renderableFlags.getPickable())
                        intersectRayWithSubsetRenderable(thePickRay, *theRenderableObject,
                                                         outIntersectionResult);
                }
                for (quint32 idx = inLayerRenderData.transparentObjects.size(), end = 0;
                     idx > end; --idx) {
                    QDemonRenderableObject *theRenderableObject =
                            inLayerRenderData.transparentObjects[idx - 1];
                    if (inPickEverything
                            || theRenderableObject->renderableFlags.getPickable())
                        intersectRayWithSubsetRenderable(thePickRay, *theRenderableObject,
                                                         outIntersectionResult);
                }
            }
        } else {
            QDemonGraphObjectPickQueryInterface *theQuery =
                    inLayerRenderData.lastFrameOffscreenRenderer->getGraphObjectPickQuery(this);
            if (theQuery) {
                QDemonRenderPickResult theResult =
                        theQuery->pick(inPresCoords, inViewportDimensions, inPickEverything);
                if (theResult.m_hitObject) {
                    theResult.m_offscreenRenderer =
                            inLayerRenderData.lastFrameOffscreenRenderer;
                    outIntersectionResult.push_back(theResult);
                }
            } else
                inLayerRenderData.lastFrameOffscreenRenderer->pick(inPresCoords,
                                                                     inViewportDimensions,
                                                                     this);
        }
    }
}

static inline QDemonRenderPickSubResult constructSubResult(QDemonRenderableImage &inImage)
{
    return constructSubResult(inImage.m_image);
}

void QDemonRendererImpl::intersectRayWithSubsetRenderable(
        const QDemonRenderRay &inRay, QDemonRenderableObject &inRenderableObject,
        TPickResultArray &outIntersectionResultList)
{
    QDemonOption<QDemonRenderRayIntersectionResult> theIntersectionResultOpt(inRay.intersectWithAABB(
                                                                      inRenderableObject.globalTransform, inRenderableObject.bounds));
    if (theIntersectionResultOpt.hasValue() == false)
        return;
    QDemonRenderRayIntersectionResult &theResult(*theIntersectionResultOpt);

    // Leave the coordinates relative for right now.
    const QDemonGraphObject *thePickObject = nullptr;
    if (inRenderableObject.renderableFlags.isDefaultMaterialMeshSubset())
        thePickObject = &static_cast<QDemonSubsetRenderable *>(&inRenderableObject)->modelContext.model;
    else if (inRenderableObject.renderableFlags.IsText())
        thePickObject = &static_cast<QDemonTextRenderable *>(&inRenderableObject)->text;
    else if (inRenderableObject.renderableFlags.isCustomMaterialMeshSubset())
        thePickObject = &static_cast<QDemonCustomMaterialRenderable *>(&inRenderableObject)->modelContext.model;
    else if (inRenderableObject.renderableFlags.isPath())
        thePickObject = &static_cast<QDemonPathRenderable *>(&inRenderableObject)->m_path;

    if (thePickObject != nullptr) {
        outIntersectionResultList.push_back(QDemonRenderPickResult(
                                                *thePickObject, theResult.m_rayLengthSquared, theResult.m_relXY));

        // For subsets, we know we can find images on them which may have been the result
        // of rendering a sub-presentation.
        if (inRenderableObject.renderableFlags.isDefaultMaterialMeshSubset()) {
            QDemonRenderPickSubResult *theLastResult = nullptr;
            for (QDemonRenderableImage *theImage =
                 static_cast<QDemonSubsetRenderable *>(&inRenderableObject)->firstImage;
                 theImage != nullptr; theImage = theImage->m_nextImage) {
                if (theImage->m_image.m_lastFrameOffscreenRenderer != nullptr
                        && theImage->m_image.m_textureData.m_texture != nullptr) {
                    QDemonRenderPickSubResult *theSubResult = new QDemonRenderPickSubResult(constructSubResult(*theImage));
                    if (theLastResult == nullptr)
                        outIntersectionResultList.back().m_firstSubObject = theSubResult;
                    else
                        theLastResult->m_nextSibling = theSubResult;
                    theLastResult = theSubResult;
                }
            }
        }
    }
}

QSharedPointer<QDemonRenderShaderProgram> QDemonRendererImpl::compileShader(const QString &inName,
                                                                            const char *inVert,
                                                                            const char *inFrag)
{
    getProgramGenerator()->beginProgram();
    getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex)->append(QString::fromLatin1(inVert));
    getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment)->append(QString::fromLatin1(inFrag));
    return getProgramGenerator()->compileGeneratedShader(inName);
}

const float MINATTENUATION = 0;
const float MAXATTENUATION = 1000;

float clampFloat(float value, float min, float max)
{
    return value < min ? min : ((value > max) ? max : value);
}

float translateConstantAttenuation(float attenuation) { return attenuation * .01f; }

float translateLinearAttenuation(float attenuation)
{
    attenuation = clampFloat(attenuation, MINATTENUATION, MAXATTENUATION);
    return attenuation * 0.0001f;
}

float translateQuadraticAttenuation(float attenuation)
{
    attenuation = clampFloat(attenuation, MINATTENUATION, MAXATTENUATION);
    return attenuation * 0.0000001f;
}

QSharedPointer<QDemonShaderGeneratorGeneratedShader> QDemonRendererImpl::getShader(QDemonSubsetRenderable &inRenderable,
                                                                                   TShaderFeatureSet inFeatureSet)
{
    if (m_currentLayer == nullptr) {
        Q_ASSERT(false);
        return nullptr;
    }
    TShaderMap::iterator theFind = m_shaders.find(inRenderable.shaderDescription);
    QSharedPointer<QDemonShaderGeneratorGeneratedShader> retval = nullptr;
    if (theFind == m_shaders.end()) {
        // Generate the shader.
        QSharedPointer<QDemonRenderShaderProgram> theShader(generateShader(inRenderable, inFeatureSet));
        if (theShader) {
            QSharedPointer<QDemonShaderGeneratorGeneratedShader> theGeneratedShader = QSharedPointer<QDemonShaderGeneratorGeneratedShader>(new QDemonShaderGeneratorGeneratedShader(
                                                                                                                                     m_generatedShaderString, theShader));
            m_shaders.insert(inRenderable.shaderDescription, theGeneratedShader);
            retval = theGeneratedShader;
        } else {
            // We still insert something because we don't to attempt to generate the same bad shader
            // twice.
            m_shaders.insert(inRenderable.shaderDescription, nullptr);
        }
    } else {
        retval = theFind.value();
    }

    if (retval != nullptr) {
        if (!m_layerShaders.contains(*retval)) {
            m_layerShaders.insert(*retval);
        }
        if (m_currentLayer && m_currentLayer->camera) {
            QDemonRenderCamera &theCamera(*m_currentLayer->camera);
            if (m_currentLayer->cameraDirection.hasValue() == false)
                m_currentLayer->cameraDirection = theCamera.getScalingCorrectDirection();
        }
    }
    return retval;
}
static QVector3D g_fullScreenRectFace[] = {
    QVector3D(-1, -1, 0), QVector3D(-1, 1, 0), QVector3D(1, 1, 0), QVector3D(1, -1, 0),
};

static QVector2D g_fullScreenRectUVs[] = { QVector2D(0, 0), QVector2D(0, 1), QVector2D(1, 1),
                                           QVector2D(1, 0) };

void QDemonRendererImpl::generateXYQuad()
{
    if (m_quadInputAssembler)
        return;

    QDemonRenderVertexBufferEntry theEntries[] = {
        QDemonRenderVertexBufferEntry("attr_pos",
        QDemonRenderComponentTypes::Float32, 3),
        QDemonRenderVertexBufferEntry("attr_uv",
        QDemonRenderComponentTypes::Float32, 2, 12),
    };

    float tempBuf[20];
    float *bufPtr = tempBuf;
    QVector3D *facePtr(g_fullScreenRectFace);
    QVector2D *uvPtr(g_fullScreenRectUVs);
    for (int j = 0; j < 4; j++, ++facePtr, ++uvPtr, bufPtr += 5) {
        bufPtr[0] = facePtr->x();
        bufPtr[1] = facePtr->y();
        bufPtr[2] = facePtr->z();
        bufPtr[3] = uvPtr->x();
        bufPtr[4] = uvPtr->y();
    }
    m_quadVertexBuffer = m_context->createVertexBuffer(
                QDemonRenderBufferUsageType::Static, 20 * sizeof(float),
                3 * sizeof(float) + 2 * sizeof(float), toU8DataRef(tempBuf, 20));

    quint8 indexData[] = {
        0, 1, 2, 0, 2, 3,
    };
    m_quadIndexBuffer = m_context->createIndexBuffer(
                QDemonRenderBufferUsageType::Static, QDemonRenderComponentTypes::UnsignedInteger8,
                sizeof(indexData), toU8DataRef(indexData, sizeof(indexData)));

    // create our attribute layout
    m_quadAttribLayout = m_context->createAttributeLayout(toConstDataRef(theEntries, 2));

    // create input assembler object
    quint32 strides = m_quadVertexBuffer->getStride();
    quint32 offsets = 0;
    m_quadInputAssembler = m_context->createInputAssembler(
                m_quadAttribLayout, toConstDataRef(&m_quadVertexBuffer, 1), m_quadIndexBuffer,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
}

void QDemonRendererImpl::generateXYZPoint()
{
    if (m_pointInputAssembler)
        return;

    QDemonRenderVertexBufferEntry theEntries[] = {
        QDemonRenderVertexBufferEntry("attr_pos",
        QDemonRenderComponentTypes::Float32, 3),
        QDemonRenderVertexBufferEntry("attr_uv",
        QDemonRenderComponentTypes::Float32, 2, 12),
    };

    float tempBuf[5];
    tempBuf[0] = tempBuf[1] = tempBuf[2] = 0.0;
    tempBuf[3] = tempBuf[4] = 0.0;

    m_pointVertexBuffer = m_context->createVertexBuffer(
                QDemonRenderBufferUsageType::Static, 5 * sizeof(float),
                3 * sizeof(float) + 2 * sizeof(float), toU8DataRef(tempBuf, 5));

    // create our attribute layout
    m_pointAttribLayout = m_context->createAttributeLayout(toConstDataRef(theEntries, 2));

    // create input assembler object
    quint32 strides = m_pointVertexBuffer->getStride();
    quint32 offsets = 0;
    m_pointInputAssembler = m_context->createInputAssembler(
                m_pointAttribLayout, toConstDataRef(&m_pointVertexBuffer, 1), nullptr,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
}

QPair<QSharedPointer<QDemonRenderVertexBuffer>, QSharedPointer<QDemonRenderIndexBuffer >> QDemonRendererImpl::getXYQuad()
{
    if (!m_quadInputAssembler)
        generateXYQuad();

    return QPair<QSharedPointer<QDemonRenderVertexBuffer>, QSharedPointer<QDemonRenderIndexBuffer >>(m_quadVertexBuffer, m_quadIndexBuffer);
}

QDemonLayerGlobalRenderProperties QDemonRendererImpl::getLayerGlobalRenderProperties()
{
    QDemonLayerRenderData &theData = *m_currentLayer;
    QDemonRenderLayer &theLayer = theData.layer;
    if (theData.cameraDirection.hasValue() == false)
        theData.cameraDirection = theData.camera->getScalingCorrectDirection();

    return QDemonLayerGlobalRenderProperties { theLayer, *theData.camera, *theData.cameraDirection, theData.lights,
                theData.lightDirections, theData.shadowMapManager, theData.m_layerDepthTexture,
                theData.m_layerSsaoTexture, theLayer.lightProbe, theLayer.lightProbe2,
                theLayer.probeHorizon, theLayer.probeBright, theLayer.probe2Window,
                theLayer.probe2Pos, theLayer.probe2Fade, theLayer.probeFov };
}

void QDemonRendererImpl::generateXYQuadStrip()
{
    if (m_quadStripInputAssembler)
        return;

    QDemonRenderVertexBufferEntry theEntries[] = {
        QDemonRenderVertexBufferEntry("attr_pos",
        QDemonRenderComponentTypes::Float32, 3),
        QDemonRenderVertexBufferEntry("attr_uv",
        QDemonRenderComponentTypes::Float32, 2, 12),
    };

    // this buffer is filled dynmically
    m_quadStripVertexBuffer =
            m_context->createVertexBuffer(QDemonRenderBufferUsageType::Dynamic, 0,
                                          3 * sizeof(float) + 2 * sizeof(float) // stride
                                          ,
                                          QDemonDataRef<quint8>());

    // create our attribute layout
    m_quadStripAttribLayout = m_context->createAttributeLayout(toConstDataRef(theEntries, 2));

    // create input assembler object
    quint32 strides = m_quadStripVertexBuffer->getStride();
    quint32 offsets = 0;
    m_quadStripInputAssembler = m_context->createInputAssembler(
                m_quadStripAttribLayout, toConstDataRef(&m_quadStripVertexBuffer, 1), nullptr,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
}

void QDemonRendererImpl::updateCbAoShadow(const QDemonRenderLayer *pLayer, const QDemonRenderCamera *pCamera,
                                          QDemonResourceTexture2D &inDepthTexture)
{
    if (m_context->getConstantBufferSupport()) {
        QString theName = QString::fromLocal8Bit("cbAoShadow");
        QSharedPointer<QDemonRenderConstantBuffer> pCB = m_context->getConstantBuffer(theName);

        if (!pCB) {
            // the  size is determined automatically later on
            pCB = m_context->createConstantBuffer(
                        theName.toLocal8Bit().constData(), QDemonRenderBufferUsageType::Static, 0, QDemonDataRef<quint8>());
            if (!pCB) {
                Q_ASSERT(false);
                return;
            }
            m_constantBuffers.insert(theName, pCB);

            // Add paramters. Note should match the appearance in the shader program
            pCB->addParam(QString::fromLocal8Bit("ao_properties"),
                          QDemonRenderShaderDataTypes::Vec4, 1);
            pCB->addParam(QString::fromLocal8Bit("ao_properties2"),
                          QDemonRenderShaderDataTypes::Vec4, 1);
            pCB->addParam(QString::fromLocal8Bit("shadow_properties"),
                          QDemonRenderShaderDataTypes::Vec4, 1);
            pCB->addParam(QString::fromLocal8Bit("aoScreenConst"),
                          QDemonRenderShaderDataTypes::Vec4, 1);
            pCB->addParam(QString::fromLocal8Bit("UvToEyeConst"),
                          QDemonRenderShaderDataTypes::Vec4, 1);
        }

        // update values
        QVector4D aoProps(pLayer->aoStrength * 0.01f, pLayer->aoDistance * 0.4f,
                          pLayer->aoSoftness * 0.02f, pLayer->aoBias);
        pCB->updateParam("ao_properties", QDemonDataRef<quint8>((quint8 *)&aoProps, 1));
        QVector4D aoProps2((float)pLayer->aoSamplerate, (pLayer->aoDither) ? 1.0f : 0.0f, 0.0f,
                           0.0f);
        pCB->updateParam("ao_properties2", QDemonDataRef<quint8>((quint8 *)&aoProps2, 1));
        QVector4D shadowProps(pLayer->shadowStrength * 0.01f, pLayer->shadowDist,
                              pLayer->shadowSoftness * 0.01f, pLayer->shadowBias);
        pCB->updateParam("shadow_properties", QDemonDataRef<quint8>((quint8 *)&shadowProps, 1));

        float R2 = pLayer->aoDistance * pLayer->aoDistance * 0.16f;
        float rw = 100, rh = 100;

        if (inDepthTexture.getTexture() && inDepthTexture.getTexture()) {
            rw = (float)inDepthTexture.getTexture()->getTextureDetails().width;
            rh = (float)inDepthTexture.getTexture()->getTextureDetails().height;
        }
        float fov = (pCamera) ? pCamera->verticalFov(rw / rh) : 1.0f;
        float tanHalfFovY = tanf(0.5f * fov * (rh / rw));
        float invFocalLenX = tanHalfFovY * (rw / rh);

        QVector4D aoScreenConst(1.0f / R2, rh / (2.0f * tanHalfFovY), 1.0f / rw, 1.0f / rh);
        pCB->updateParam("aoScreenConst", QDemonDataRef<quint8>((quint8 *)&aoScreenConst, 1));
        QVector4D UvToEyeConst(2.0f * invFocalLenX, -2.0f * tanHalfFovY, -invFocalLenX,
                               tanHalfFovY);
        pCB->updateParam("UvToEyeConst", QDemonDataRef<quint8>((quint8 *)&UvToEyeConst, 1));

        // update buffer to hardware
        pCB->update();
    }
}

// widget context implementation

QSharedPointer<QDemonRenderVertexBuffer> QDemonRendererImpl::getOrCreateVertexBuffer(QString &inStr,
                                                                      quint32 stride,
                                                                      QDemonConstDataRef<quint8> bufferData)
{
    QSharedPointer<QDemonRenderVertexBuffer> retval = getVertexBuffer(inStr);
    if (retval) {
        // we update the buffer
        retval->updateBuffer(bufferData, false);
        return retval;
    }
    retval = m_context->createVertexBuffer(QDemonRenderBufferUsageType::Dynamic,
                                           bufferData.size(), stride, bufferData);
    m_widgetVertexBuffers.insert(inStr, retval);
    return retval;
}
QSharedPointer<QDemonRenderIndexBuffer> QDemonRendererImpl::getOrCreateIndexBuffer(QString &inStr,
                                                                                   QDemonRenderComponentTypes::Enum componentType,
                                                                                   size_t size, QDemonConstDataRef<quint8> bufferData)
{
    QSharedPointer<QDemonRenderIndexBuffer> retval = getIndexBuffer(inStr);
    if (retval) {
        // we update the buffer
        retval->updateBuffer(bufferData, false);
        return retval;
    }

    retval = m_context->createIndexBuffer(QDemonRenderBufferUsageType::Dynamic,
                                          componentType, size, bufferData);
    m_widgetIndexBuffers.insert(inStr, retval);
    return retval;
}

QSharedPointer<QDemonRenderAttribLayout> QDemonRendererImpl::createAttributeLayout(QDemonConstDataRef<QDemonRenderVertexBufferEntry> attribs)
{
    // create our attribute layout
    QSharedPointer<QDemonRenderAttribLayout> theAttribLayout = m_context->createAttributeLayout(attribs);
    return theAttribLayout;
}

QSharedPointer<QDemonRenderInputAssembler> QDemonRendererImpl::getOrCreateInputAssembler(QString &inStr,
                                                                                         QSharedPointer<QDemonRenderAttribLayout> attribLayout,
                                                                                         QDemonConstDataRef<QSharedPointer<QDemonRenderVertexBuffer>> buffers,
                                                                                         const QSharedPointer<QDemonRenderIndexBuffer> indexBuffer,
                                                                                         QDemonConstDataRef<quint32> strides,
                                                                                         QDemonConstDataRef<quint32> offsets)
{
    QSharedPointer<QDemonRenderInputAssembler> retval = getInputAssembler(inStr);
    if (retval)
        return retval;

    retval =
            m_context->createInputAssembler(attribLayout, buffers, indexBuffer, strides, offsets);
    m_widgetInputAssembler.insert(inStr, retval);
    return retval;
}

QSharedPointer<QDemonRenderVertexBuffer> QDemonRendererImpl::getVertexBuffer(const QString &inStr)
{
    TStrVertBufMap::iterator theIter = m_widgetVertexBuffers.find(inStr);
    if (theIter != m_widgetVertexBuffers.end())
        return theIter.value();
    return nullptr;
}

QSharedPointer<QDemonRenderIndexBuffer> QDemonRendererImpl::getIndexBuffer(const QString &inStr)
{
    TStrIndexBufMap::iterator theIter = m_widgetIndexBuffers.find(inStr);
    if (theIter != m_widgetIndexBuffers.end())
        return theIter.value();
    return nullptr;
}

QSharedPointer<QDemonRenderInputAssembler> QDemonRendererImpl::getInputAssembler(const QString &inStr)
{
    TStrIAMap::iterator theIter = m_widgetInputAssembler.find(inStr);
    if (theIter != m_widgetInputAssembler.end())
        return theIter.value();
    return nullptr;
}

QSharedPointer<QDemonRenderShaderProgram> QDemonRendererImpl::getShader(const QString &inStr)
{
    TStrShaderMap::iterator theIter = m_widgetShaders.find(inStr);
    if (theIter != m_widgetShaders.end())
        return theIter.value();
    return nullptr;
}

QSharedPointer<QDemonRenderShaderProgram> QDemonRendererImpl::compileAndStoreShader(const QString &inStr)
{
    QSharedPointer<QDemonRenderShaderProgram> newProgram = getProgramGenerator()->compileGeneratedShader(inStr);
    if (newProgram)
        m_widgetShaders.insert(inStr, newProgram);
    return newProgram;
}

QSharedPointer<QDemonShaderProgramGeneratorInterface> QDemonRendererImpl::getProgramGenerator()
{
    return m_demonContext->getShaderProgramGenerator();
}

QDemonTextDimensions QDemonRendererImpl::measureText(const QDemonTextRenderInfo &inText)
{
    if (m_demonContext->getTextRenderer() != nullptr)
        return m_demonContext->getTextRenderer()->measureText(inText, 0);
    return QDemonTextDimensions();
}

void QDemonRendererImpl::renderText(const QDemonTextRenderInfo &inText, const QVector3D &inTextColor,
                                    const QVector3D &inBackgroundColor, const QMatrix4x4 &inMVP)
{
    if (m_demonContext->getTextRenderer() != nullptr) {
        QDemonTextRendererInterface &theTextRenderer(*m_demonContext->getTextRenderer());
        QSharedPointer<QDemonRenderTexture2D> theTexture = m_demonContext->getResourceManager()->allocateTexture2D(
                    32, 32, QDemonRenderTextureFormats::RGBA8);
        QDemonTextTextureDetails theTextTextureDetails =
                theTextRenderer.renderText(inText, *theTexture);
        QDemonTextRenderHelper theTextHelper(getTextWidgetShader());
        if (theTextHelper.shader != nullptr) {
            m_demonContext->getRenderContext()->setBlendingEnabled(false);
            QDemonTextScaleAndOffset theScaleAndOffset(*theTexture, theTextTextureDetails, inText);
            theTextHelper.shader->render(theTexture, theScaleAndOffset,
                                           QVector4D(inTextColor, 1.0f), inMVP, QVector2D(0, 0),
                                           getContext(), theTextHelper.quadInputAssembler,
                                           theTextHelper.quadInputAssembler->getIndexCount(),
                                           theTextTextureDetails, inBackgroundColor);
        }
        m_demonContext->getResourceManager()->release(theTexture);
    }
}

void QDemonRendererImpl::renderText2D(float x, float y, QDemonOption<QVector3D> inColor, const QString &text)
{
    if (m_demonContext->getOnscreenTextRenderer() != nullptr) {
        generateXYQuadStrip();

        if (prepareTextureAtlasForRender()) {
            TTextRenderAtlasDetailsAndTexture theRenderTextDetails;
            QSharedPointer<QDemonTextTextureAtlasInterface> theTextureAtlas = m_demonContext->getTextureAtlas();
            QSize theWindow = m_demonContext->getWindowDimensions();

            QDemonTextRenderInfo theInfo;
            theInfo.text = text;
            theInfo.fontSize = 20;
            // text scale 2% of screen we don't scale Y though because it becomes unreadable
            theInfo.scaleX = (theWindow.width() / 100.0f) * 1.5f / (theInfo.fontSize);
            theInfo.scaleY = 1.0f;

            theRenderTextDetails = theTextureAtlas->renderText(theInfo);

            if (theRenderTextDetails.first.vertices.size()) {
                QDemonTextRenderHelper theTextHelper(getOnscreenTextShader());
                if (theTextHelper.shader != nullptr) {
                    // setup 2D projection
                    QDemonRenderCamera theCamera;
                    theCamera.clipNear = -1.0;
                    theCamera.clipFar = 1.0;

                    theCamera.markDirty(NodeTransformDirtyFlag::TransformIsDirty);
                    theCamera.flags.setOrthographic(true);
                    QVector2D theWindowDim((float)theWindow.width(), (float)theWindow.height());
                    theCamera.calculateGlobalVariables(QRect(0, 0, theWindow.width(), theWindow.height()), theWindowDim);
                    // We want a 2D lower left projection
                    float *writePtr(theCamera.projection.data());
                    writePtr[12] = -1;
                    writePtr[13] = -1;

                    // upload vertices
                    m_quadStripVertexBuffer->updateBuffer(theRenderTextDetails.first.vertices,
                                                          false);

                    theTextHelper.shader->render2D(theRenderTextDetails.second, QVector4D(inColor, 1.0f),
                                                   theCamera.projection, getContext(),
                                                   theTextHelper.quadInputAssembler,
                                                   theRenderTextDetails.first.vertexCount, QVector2D(x, y));
                }
                // we release the memory here
                ::free(theRenderTextDetails.first.vertices.begin());
            }
        }
    }
}

void QDemonRendererImpl::renderGpuProfilerStats(float x, float y,
                                                QDemonOption<QVector3D> inColor)
{
    if (!isLayerGpuProfilingEnabled())
        return;

    char messageLine[1024];
    TInstanceRenderMap::const_iterator theIter;

    float startY = y;

    for (theIter = m_instanceRenderMap.begin(); theIter != m_instanceRenderMap.end(); theIter++) {
        float startX = x;
        const QSharedPointer<QDemonLayerRenderData> theLayerRenderData = theIter.value();
        const QDemonRenderLayer *theLayer = &theLayerRenderData->layer;

        if (theLayer->flags.isActive() && theLayerRenderData->m_layerProfilerGpu) {
            const QDemonRenderProfilerInterface::TStrIDVec &idList =
                    theLayerRenderData->m_layerProfilerGpu->getTimerIDs();
            if (!idList.empty()) {
                startY -= 22;
                startX += 20;
                renderText2D(startX, startY, inColor, theLayer->id);
                QDemonRenderProfilerInterface::TStrIDVec::const_iterator theIdIter = idList.begin();
                for (theIdIter = idList.begin(); theIdIter != idList.end(); theIdIter++) {
                    startY -= 22;
                    sprintf(messageLine, "%s: %.3f ms", theIdIter->toLatin1().constData(),
                            theLayerRenderData->m_layerProfilerGpu->getElapsedTime(*theIdIter));
                    renderText2D(startX + 20, startY, inColor, QString::fromLocal8Bit(messageLine));
                }
            }
        }
    }
}

// Given a node and a point in the node's local space (most likely its pivot point), we return
// a normal matrix so you can get the axis out, a transformation from node to camera
// a new position and a floating point scale factor so you can render in 1/2 perspective mode
// or orthographic mode if you would like to.
QDemonWidgetRenderInformation QDemonRendererImpl::getWidgetRenderInformation(QDemonGraphNode &inNode,
                                                                             const QVector3D &inPos,
                                                                             RenderWidgetModes::Enum inWidgetMode)
{
    QSharedPointer<QDemonLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inNode);
    QDemonRenderCamera *theCamera = theData->camera;
    if (theCamera == nullptr || theData->layerPrepResult.hasValue() == false) {
        Q_ASSERT(false);
        return QDemonWidgetRenderInformation();
    }
    QMatrix4x4 theGlobalTransform;
    if (inNode.parent != nullptr && inNode.parent->type != QDemonGraphObjectTypes::Layer && !inNode.flags.isIgnoreParentTransform())
        theGlobalTransform = inNode.parent->globalTransform;
    QMatrix4x4 theCameraInverse = mat44::getInverse(theCamera->globalTransform);
    QMatrix4x4 theNodeParentToCamera;
    if (inWidgetMode == RenderWidgetModes::Local)
        theNodeParentToCamera = theCameraInverse * theGlobalTransform;
    else
        theNodeParentToCamera = theCameraInverse;

    float normalMatData[9] = {
        theNodeParentToCamera(0,0), theNodeParentToCamera(0, 1), theNodeParentToCamera(0,2),
        theNodeParentToCamera(1,0), theNodeParentToCamera(1, 1), theNodeParentToCamera(1,2),
        theNodeParentToCamera(2,0), theNodeParentToCamera(2, 1), theNodeParentToCamera(2,2)
    };

    QMatrix3x3 theNormalMat(normalMatData);
    theNormalMat = mat33::getInverse(theNormalMat).transposed();
    QVector3D column0(theNormalMat(0, 0), theNormalMat(0, 1), theNormalMat(0, 2));
    QVector3D column1(theNormalMat(1, 0), theNormalMat(1, 1), theNormalMat(1, 2));
    QVector3D column2(theNormalMat(2, 0), theNormalMat(2, 1), theNormalMat(2, 2));
    column0.normalize();
    column1.normalize();
    column2.normalize();
    float normalizedMatData[9] = {
        column0.x(), column0.y(), column0.z(),
        column1.x(), column1.y(), column1.z(),
        column2.x(), column2.y(), column2.z()
    };

    theNormalMat = QMatrix3x3(normalizedMatData);

    QMatrix4x4 theTranslation;
    theTranslation(3, 0) = inNode.position.x();
    theTranslation(3, 1) = inNode.position.y();
    theTranslation(3, 2) = inNode.position.z();
    theTranslation(3, 2) *= -1.0f;

    theGlobalTransform = theGlobalTransform * theTranslation;

    QMatrix4x4 theNodeToParentPlusTranslation = theCameraInverse * theGlobalTransform;
    QVector3D thePos = mat44::transform(theNodeToParentPlusTranslation, inPos);
    QDemonScaleAndPosition theScaleAndPos = getWorldToPixelScaleFactor(*theCamera, thePos, *theData);
    QMatrix3x3 theLookAtMatrix;
    if (theCamera->flags.isOrthographic() == false) {
        QVector3D theNodeToCamera = theScaleAndPos.position;
        theNodeToCamera.normalize();
        QVector3D theOriginalAxis = QVector3D(0, 0, -1);
        QVector3D theRotAxis = QVector3D::crossProduct(theOriginalAxis, theNodeToCamera);
        theRotAxis.normalize();
        float theAxisLen = vec3::magnitude(theRotAxis);
        if (theAxisLen > .05f) {
            float theRotAmount = std::acos(QVector3D::dotProduct(theOriginalAxis, theNodeToCamera));
            QQuaternion theQuat(theRotAmount, theRotAxis);
            theLookAtMatrix = theQuat.toRotationMatrix();
        }
    }
    QVector3D thePosInWorldSpace = mat44::transform(theGlobalTransform, inPos);
    QVector3D theCameraPosInWorldSpace = theCamera->getGlobalPos();
    QVector3D theCameraOffset = thePosInWorldSpace - theCameraPosInWorldSpace;
    QVector3D theDir = theCameraOffset;
    theDir.normalize();
    // Things should be 600 units from the camera, as that is how all of our math is setup.
    theCameraOffset = 600.0f * theDir;
    return QDemonWidgetRenderInformation(
                theNormalMat, theNodeParentToCamera, theCamera->projection, theCamera->projection,
                theLookAtMatrix, theCameraInverse, theCameraOffset, theScaleAndPos.position,
                theScaleAndPos.scale, *theCamera);
}

QDemonOption<QVector2D> QDemonRendererImpl::getLayerMouseCoords(QDemonRenderLayer &inLayer,
                                                                const QVector2D &inMouseCoords,
                                                                const QVector2D &inViewportDimensions,
                                                                bool forceImageIntersect) const
{
    QSharedPointer<QDemonLayerRenderData> theData =
            const_cast<QDemonRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inLayer);
    return getLayerMouseCoords(*theData, inMouseCoords, inViewportDimensions,
                               forceImageIntersect);
}

bool QDemonRendererInterface::isGlEsContext(QDemonRenderContextType inContextType)
{
    QDemonRenderContextType esContextTypes(QDemonRenderContextValues::GLES2
                                           | QDemonRenderContextValues::GLES3
                                           | QDemonRenderContextValues::GLES3PLUS);

    if ((inContextType & esContextTypes))
        return true;

    return false;
}

bool QDemonRendererInterface::isGlEs3Context(QDemonRenderContextType inContextType)
{
    if (inContextType == QDemonRenderContextValues::GLES3
            || inContextType == QDemonRenderContextValues::GLES3PLUS)
        return true;

    return false;
}

bool QDemonRendererInterface::isGl2Context(QDemonRenderContextType inContextType)
{
    if (inContextType == QDemonRenderContextValues::GL2)
        return true;

    return false;
}

QSharedPointer<QDemonRendererInterface> QDemonRendererInterface::createRenderer(QDemonRenderContextInterface *inContext)
{
    return QSharedPointer<QDemonRendererImpl>(new QDemonRendererImpl(inContext));
}
QT_END_NAMESPACE