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

#include <QtDemonRuntimeRender/qdemonrendererimpl.h>
#include <QtDemonRuntimeRender/qdemonrenderlight.h>
#include <QtDemonRuntimeRender/qdemonrendercontextcore.h>
#include <QtDemonRuntimeRender/qdemonrendershadercache.h>
#include <QtDemonRuntimeRender/qdemonrenderdynamicobjectsystem.h>
#include <QtDemonRuntimeRender/qdemonrendershadercodegeneratorv2.h>
#include <QtDemonRuntimeRender/qdemonrenderdefaultmaterialshadergenerator.h>
#include <QtDemonRuntimeRender/qdemonvertexpipelineimpl.h>

// This adds support for the depth buffers in the shader so we can do depth
// texture-based effects.
#define QDEMON_RENDER_SUPPORT_DEPTH_TEXTURE 1

QT_BEGIN_NAMESPACE

void QDemonTextShader::render(const QDemonRef<QDemonRenderTexture2D> &inTexture,
                              const QDemonTextScaleAndOffset &inScaleAndOffset,
                              const QVector4D &inTextColor,
                              const QMatrix4x4 &inMVP,
                              const QVector2D &inCameraVec,
                              const QDemonRef<QDemonRenderContext> &inRenderContext,
                              const QDemonRef<QDemonRenderInputAssembler> &inInputAssemblerBuffer,
                              quint32 count,
                              const QDemonTextTextureDetails &inTextTextureDetails,
                              const QVector3D &inBackgroundColor)
{
    inRenderContext->setCullingEnabled(false);
    inRenderContext->setActiveShader(shader);
    mvp.set(inMVP);
    sampler.set(inTexture.data());
    textColor.set(inTextColor);
    dimensions.set(QVector4D(inScaleAndOffset.textScale.x(),
                             inScaleAndOffset.textScale.y(),
                             inScaleAndOffset.textOffset.x(),
                             inScaleAndOffset.textOffset.y()));
    cameraProperties.set(inCameraVec);
    QDemonTextureDetails theTextureDetails = inTexture->getTextureDetails();
    float theWidthScale = (float)inTextTextureDetails.textWidth / (float)theTextureDetails.width;
    float theHeightScale = (float)inTextTextureDetails.textHeight / (float)theTextureDetails.height;
    backgroundColor.set(inBackgroundColor);

    textDimensions.set(QVector3D(theWidthScale, theHeightScale, inTextTextureDetails.flipY ? 1.0f : 0.0f));
    inRenderContext->setInputAssembler(inInputAssemblerBuffer);
    inRenderContext->draw(QDemonRenderDrawMode::Triangles, count, 0);
}

void QDemonTextShader::renderPath(const QDemonRef<QDemonRenderPathFontItem> &inPathFontItem,
                                  const QDemonRef<QDemonRenderPathFontSpecification> &inPathFontSpec,
                                  const QDemonTextScaleAndOffset &inScaleAndOffset,
                                  const QVector4D &inTextColor,
                                  const QMatrix4x4 &inViewProjection,
                                  const QMatrix4x4 &inModel,
                                  const QVector2D &,
                                  const QDemonRef<QDemonRenderContext> &inRenderContext,
                                  const QDemonTextTextureDetails &inTextTextureDetails,
                                  const QVector3D &inBackgroundColor)
{
    QDemonRenderBoolOp::Enum theDepthFunction = inRenderContext->getDepthFunction();
    bool isDepthEnabled = inRenderContext->isDepthTestEnabled();
    bool isStencilEnabled = inRenderContext->isStencilTestEnabled();
    bool isDepthWriteEnabled = inRenderContext->isDepthWriteEnabled();
    QDemonRenderStencilFunctionArgument theArg(QDemonRenderBoolOp::NotEqual, 0, 0xFF);
    QDemonRenderStencilOperationArgument theOpArg(QDemonRenderStencilOp::Keep, QDemonRenderStencilOp::Keep, QDemonRenderStencilOp::Zero);
    QDemonRef<QDemonRenderDepthStencilState> depthStencilState = inRenderContext->createDepthStencilState(isDepthEnabled,
                                                                                                          isDepthWriteEnabled,
                                                                                                          theDepthFunction,
                                                                                                          false,
                                                                                                          theArg,
                                                                                                          theArg,
                                                                                                          theOpArg,
                                                                                                          theOpArg);

    inRenderContext->setActiveShader(nullptr);
    inRenderContext->setCullingEnabled(false);

    inRenderContext->setDepthStencilState(depthStencilState);

    // setup transform
    QMatrix4x4 offsetMatrix;
    offsetMatrix.translate(inScaleAndOffset.textOffset.x() - (float)inTextTextureDetails.textWidth / 2.0f,
                           inScaleAndOffset.textOffset.y() - (float)inTextTextureDetails.textHeight / 2.0f,
                           0.0);

    QMatrix4x4 pathMatrix = inPathFontItem->getTransform();

    inRenderContext->setPathProjectionMatrix(inViewProjection);
    inRenderContext->setPathModelViewMatrix(inModel * offsetMatrix * pathMatrix);

    // first pass
    inPathFontSpec->stencilFillPathInstanced(inPathFontItem);

    // second pass
    inRenderContext->setActiveProgramPipeline(programPipeline);
    textColor.set(inTextColor);
    backgroundColor.set(inBackgroundColor);

    inRenderContext->setStencilTestEnabled(true);
    inPathFontSpec->coverFillPathInstanced(inPathFontItem);

    inRenderContext->setStencilTestEnabled(isStencilEnabled);
    inRenderContext->setDepthFunction(theDepthFunction);

    inRenderContext->setActiveProgramPipeline(nullptr);
}

void QDemonTextShader::render2D(const QDemonRef<QDemonRenderTexture2D> &inTexture,
                                const QVector4D &inTextColor,
                                const QMatrix4x4 &inMVP,
                                const QDemonRef<QDemonRenderContext> &inRenderContext,
                                const QDemonRef<QDemonRenderInputAssembler> &inInputAssemblerBuffer,
                                quint32 count,
                                const QVector2D &inVertexOffsets)
{
    // inRenderContext.SetCullingEnabled( false );
    inRenderContext->setBlendingEnabled(true);
    inRenderContext->setDepthWriteEnabled(false);
    inRenderContext->setDepthTestEnabled(false);

    inRenderContext->setActiveShader(shader);

    QDemonRenderBlendFunctionArgument blendFunc(QDemonRenderSrcBlendFunc::SrcAlpha,
                                                QDemonRenderDstBlendFunc::OneMinusSrcAlpha,
                                                QDemonRenderSrcBlendFunc::One,
                                                QDemonRenderDstBlendFunc::One);
    QDemonRenderBlendEquationArgument blendEqu(QDemonRenderBlendEquation::Add, QDemonRenderBlendEquation::Add);

    inRenderContext->setBlendFunction(blendFunc);
    inRenderContext->setBlendEquation(blendEqu);

    mvp.set(inMVP);
    sampler.set(inTexture.data());
    textColor.set(inTextColor);
    vertexOffsets.set(inVertexOffsets);

    inRenderContext->setInputAssembler(inInputAssemblerBuffer);
    inRenderContext->draw(QDemonRenderDrawMode::Triangles, count, 0);
}

static inline void addVertexDepth(QDemonShaderVertexCodeGenerator &vertexShader)
{
    // near plane, far plane
    vertexShader.addInclude("viewProperties.glsllib");
    vertexShader.addVarying("vertex_depth", "float");
    // the w coordinate is the unormalized distance to the object from the camera
    // We want the normalized distance, with 0 representing the far plane and 1 representing
    // the near plane, of the object in the vertex depth variable.

    vertexShader << "\tvertex_depth = calculateVertexDepth( camera_properties, gl_Position );"
                 << "\n";
}

// Helper implements the vertex pipeline for mesh subsets when bound to the default material.
// Should be completely possible to use for custom materials with a bit of refactoring.
struct QDemonSubsetMaterialVertexPipeline : public QDemonVertexPipelineImpl
{
    QDemonRendererImpl &renderer;
    QDemonSubsetRenderable &renderable;
    TessModeValues::Enum tessMode;

    QDemonSubsetMaterialVertexPipeline(QDemonRendererImpl &inRenderer, QDemonSubsetRenderable &inRenderable, bool inWireframeRequested)
        : QDemonVertexPipelineImpl(inRenderer.getDemonContext()->getDefaultMaterialShaderGenerator(),
                                   inRenderer.getDemonContext()->getShaderProgramGenerator(),
                                   false)
        , renderer(inRenderer)
        , renderable(inRenderable)
        , tessMode(TessModeValues::NoTess)
    {
        if (inRenderer.getContext()->isTessellationSupported())
            tessMode = inRenderable.tessellationMode;

        if (inRenderer.getContext()->isGeometryStageSupported() && tessMode != TessModeValues::NoTess)
            m_wireframe = inWireframeRequested;
    }

    void initializeTessControlShader()
    {
        if (tessMode == TessModeValues::NoTess || programGenerator()->getStage(ShaderGeneratorStages::TessControl) == nullptr) {
            return;
        }

        QDemonShaderStageGeneratorInterface &tessCtrlShader(*programGenerator()->getStage(ShaderGeneratorStages::TessControl));

        tessCtrlShader.addUniform("tessLevelInner", "float");
        tessCtrlShader.addUniform("tessLevelOuter", "float");

        setupTessIncludes(ShaderGeneratorStages::TessControl, tessMode);

        tessCtrlShader.append("void main() {\n");

        tessCtrlShader.append("\tctWorldPos[0] = varWorldPos[0];");
        tessCtrlShader.append("\tctWorldPos[1] = varWorldPos[1];");
        tessCtrlShader.append("\tctWorldPos[2] = varWorldPos[2];");

        if (tessMode == TessModeValues::TessPhong || tessMode == TessModeValues::TessNPatch) {
            tessCtrlShader.append("\tctNorm[0] = varObjectNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = varObjectNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = varObjectNormal[2];");
        }
        if (tessMode == TessModeValues::TessNPatch) {
            tessCtrlShader.append("\tctTangent[0] = varTangent[0];");
            tessCtrlShader.append("\tctTangent[1] = varTangent[1];");
            tessCtrlShader.append("\tctTangent[2] = varTangent[2];");
        }

        tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
        tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
    }
    void initializeTessEvaluationShader()
    {
        if (tessMode == TessModeValues::NoTess || programGenerator()->getStage(ShaderGeneratorStages::TessEval) == nullptr) {
            return;
        }

        QDemonShaderStageGeneratorInterface &tessEvalShader(*programGenerator()->getStage(ShaderGeneratorStages::TessEval));

        setupTessIncludes(ShaderGeneratorStages::TessEval, tessMode);

        if (tessMode == TessModeValues::TessLinear)
            renderer.getDemonContext()->getDefaultMaterialShaderGenerator()->addDisplacementImageUniforms(tessEvalShader,
                                                                                                          m_displacementIdx,
                                                                                                          m_displacementImage);

        tessEvalShader.addUniform("model_view_projection", "mat4");
        tessEvalShader.addUniform("normal_matrix", "mat3");

        tessEvalShader.append("void main() {");

        if (tessMode == TessModeValues::TessNPatch) {
            tessEvalShader.append("\tctNorm[0] = varObjectNormalTC[0];");
            tessEvalShader.append("\tctNorm[1] = varObjectNormalTC[1];");
            tessEvalShader.append("\tctNorm[2] = varObjectNormalTC[2];");

            tessEvalShader.append("\tctTangent[0] = varTangentTC[0];");
            tessEvalShader.append("\tctTangent[1] = varTangentTC[1];");
            tessEvalShader.append("\tctTangent[2] = varTangentTC[2];");
        }

        tessEvalShader.append("\tvec4 pos = tessShader( );\n");
    }

    void finalizeTessControlShader()
    {
        QDemonShaderStageGeneratorInterface &tessCtrlShader(*programGenerator()->getStage(ShaderGeneratorStages::TessControl));
        // add varyings we must pass through
        typedef TStrTableStrMap::const_iterator TParamIter;
        for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
            tessCtrlShader << "\t" << iter.value() << "TC[gl_InvocationID] = " << iter.value() << "[gl_InvocationID];\n";
        }
    }

    void finalizeTessEvaluationShader()
    {
        QDemonShaderStageGeneratorInterface &tessEvalShader(*programGenerator()->getStage(ShaderGeneratorStages::TessEval));

        QByteArray outExt;
        if (programGenerator()->getEnabledStages() & ShaderGeneratorStages::Geometry)
            outExt = "TE";

        // add varyings we must pass through
        typedef TStrTableStrMap::const_iterator TParamIter;
        if (tessMode == TessModeValues::TessNPatch) {
            for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
                tessEvalShader << "\t" << iter.key() << outExt << " = gl_TessCoord.z * " << iter.key() << "TC[0] + ";
                tessEvalShader << "gl_TessCoord.x * " << iter.key() << "TC[1] + ";
                tessEvalShader << "gl_TessCoord.y * " << iter.key() << "TC[2];\n";
            }

            // transform the normal
            if (m_generationFlags & GenerationFlagValues::WorldNormal)
                tessEvalShader << "\n\tvarNormal" << outExt << " = normalize(normal_matrix * teNorm);\n";
            // transform the tangent
            if (m_generationFlags & GenerationFlagValues::TangentBinormal) {
                tessEvalShader << "\n\tvarTangent" << outExt << " = normalize(normal_matrix * teTangent);\n";
                // transform the binormal
                tessEvalShader << "\n\tvarBinormal" << outExt << " = normalize(normal_matrix * teBinormal);\n";
            }
        } else {
            for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
                tessEvalShader << "\t" << iter.key() << outExt << " = gl_TessCoord.x * " << iter.key() << "TC[0] + ";
                tessEvalShader << "gl_TessCoord.y * " << iter.key() << "TC[1] + ";
                tessEvalShader << "gl_TessCoord.z * " << iter.key() << "TC[2];\n";
            }

            // displacement mapping makes only sense with linear tessellation
            if (tessMode == TessModeValues::TessLinear && m_displacementImage) {
                QDemonDefaultMaterialShaderGeneratorInterface::ImageVariableNames
                        theNames = renderer.getDemonContext()->getDefaultMaterialShaderGenerator()->getImageVariableNames(m_displacementIdx);
                tessEvalShader << "\tpos.xyz = defaultMaterialFileDisplacementTexture( " << theNames.m_imageSampler
                               << ", displaceAmount, " << theNames.m_imageFragCoords << outExt;
                tessEvalShader << ", varObjectNormal" << outExt << ", pos.xyz );"
                               << "\n";
                tessEvalShader << "\tvarWorldPos" << outExt << "= (model_matrix * pos).xyz;"
                               << "\n";
                tessEvalShader << "\tvarViewVector" << outExt << "= normalize(camera_position - "
                               << "varWorldPos" << outExt << ");"
                               << "\n";
            }

            // transform the normal
            tessEvalShader << "\n\tvarNormal" << outExt << " = normalize(normal_matrix * varObjectNormal" << outExt << ");\n";
        }

        tessEvalShader.append("\tgl_Position = model_view_projection * pos;\n");
    }

    void beginVertexGeneration(quint32 displacementImageIdx, QDemonRenderableImage *displacementImage) override
    {
        m_displacementIdx = displacementImageIdx;
        m_displacementImage = displacementImage;

        TShaderGeneratorStageFlags theStages(QDemonShaderProgramGeneratorInterface::defaultFlags());
        if (tessMode != TessModeValues::NoTess) {
            theStages |= ShaderGeneratorStages::TessControl;
            theStages |= ShaderGeneratorStages::TessEval;
        }
        if (m_wireframe) {
            theStages |= ShaderGeneratorStages::Geometry;
        }
        programGenerator()->beginProgram(theStages);
        if (tessMode != TessModeValues::NoTess) {
            initializeTessControlShader();
            initializeTessEvaluationShader();
        }
        if (m_wireframe) {
            initializeWireframeGeometryShader();
        }
        // Open up each stage.
        QDemonShaderStageGeneratorInterface &vertexShader(vertex());
        vertexShader.addIncoming("attr_pos", "vec3");
        vertexShader << "void main()"
                     << "\n"
                     << "{"
                     << "\n";
        vertexShader << "\tvec3 uTransform;"
                     << "\n";
        vertexShader << "\tvec3 vTransform;"
                     << "\n";

        if (displacementImage) {
            generateUVCoords();
            materialGenerator()->generateImageUVCoordinates(*this, displacementImageIdx, 0, *displacementImage);
            if (!hasTessellation()) {
                vertexShader.addUniform("displaceAmount", "float");
                // we create the world position setup here
                // because it will be replaced with the displaced position
                setCode(GenerationFlagValues::WorldPosition);
                vertexShader.addUniform("model_matrix", "mat4");

                vertexShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
                QDemonDefaultMaterialShaderGeneratorInterface::ImageVariableNames theVarNames = materialGenerator()->getImageVariableNames(
                        displacementImageIdx);

                vertexShader.addUniform(theVarNames.m_imageSampler, "sampler2D");

                vertexShader << "\tvec3 displacedPos = defaultMaterialFileDisplacementTexture( " << theVarNames.m_imageSampler
                             << ", displaceAmount, " << theVarNames.m_imageFragCoords << ", attr_norm, attr_pos );"
                             << "\n";
                addInterpolationParameter("varWorldPos", "vec3");
                vertexShader.append("\tvec3 local_model_world_position = (model_matrix * "
                                    "vec4(displacedPos, 1.0)).xyz;");
                assignOutput("varWorldPos", "local_model_world_position");
            }
        }
        // for tessellation we pass on the position in object coordinates
        // Also note that gl_Position is written in the tess eval shader
        if (hasTessellation())
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
        else {
            vertexShader.addUniform("model_view_projection", "mat4");
            if (displacementImage)
                vertexShader.append("\tgl_Position = model_view_projection * vec4(displacedPos, 1.0);");
            else
                vertexShader.append("\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
        }

        if (hasTessellation()) {
            generateWorldPosition();
            generateWorldNormal();
            generateObjectNormal();
            generateVarTangentAndBinormal();
        }
    }

    void beginFragmentGeneration() override
    {
        fragment().addUniform("material_diffuse", "vec4");
        fragment() << "void main()"
                   << "\n"
                   << "{"
                   << "\n";
        // We do not pass object opacity through the pipeline.
        fragment() << "\tfloat object_opacity = material_diffuse.a;"
                   << "\n";
    }

    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue) override
    {
        vertex() << "\t" << inVarName << " = " << inVarValue << ";\n";
    }
    void doGenerateUVCoords(quint32 inUVSet = 0) override
    {
        Q_ASSERT(inUVSet == 0 || inUVSet == 1);

        if (inUVSet == 0) {
            vertex().addIncoming("attr_uv0", "vec2");
            vertex() << "\tvarTexCoord0 = attr_uv0;"
                     << "\n";
        } else if (inUVSet == 1) {
            vertex().addIncoming("attr_uv1", "vec2");
            vertex() << "\tvarTexCoord1 = attr_uv1;"
                     << "\n";
        }
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void doGenerateWorldNormal() override
    {
        QDemonShaderStageGeneratorInterface &vertexGenerator(vertex());
        vertexGenerator.addIncoming("attr_norm", "vec3");
        vertexGenerator.addUniform("normal_matrix", "mat3");
        if (hasTessellation() == false) {
            vertexGenerator.append("\tvec3 world_normal = normalize(normal_matrix * attr_norm).xyz;");
            vertexGenerator.append("\tvarNormal = world_normal;");
        }
    }
    void doGenerateObjectNormal() override
    {
        addInterpolationParameter("varObjectNormal", "vec3");
        vertex().append("\tvarObjectNormal = attr_norm;");
    }
    void doGenerateWorldPosition() override
    {
        vertex().append("\tvec3 local_model_world_position = (model_matrix * vec4(attr_pos, 1.0)).xyz;");
        assignOutput("varWorldPos", "local_model_world_position");
    }

    void doGenerateVarTangentAndBinormal() override
    {
        vertex().addIncoming("attr_textan", "vec3");
        vertex().addIncoming("attr_binormal", "vec3");

        bool hasNPatchTessellation = tessMode == TessModeValues::TessNPatch;

        if (!hasNPatchTessellation) {
            vertex() << "\tvarTangent = normal_matrix * attr_textan;"
                     << "\n"
                     << "\tvarBinormal = normal_matrix * attr_binormal;"
                     << "\n";
        } else {
            vertex() << "\tvarTangent = attr_textan;"
                     << "\n"
                     << "\tvarBinormal = attr_binormal;"
                     << "\n";
        }
    }

    void doGenerateVertexColor() override
    {
        vertex().addIncoming("attr_color", "vec3");
        vertex().append("\tvarColor = attr_color;");
    }

    void endVertexGeneration() override
    {

        if (hasTessellation()) {
            // finalize tess control shader
            finalizeTessControlShader();
            // finalize tess evaluation shader
            finalizeTessEvaluationShader();

            tessControl().append("}");
            tessEval().append("}");
        }
        if (m_wireframe) {
            // finalize geometry shader
            finalizeWireframeGeometryShader();
            geometry().append("}");
        }
        vertex().append("}");
    }

    void endFragmentGeneration() override { fragment().append("}"); }

    void addInterpolationParameter(const QByteArray &inName, const QByteArray &inType) override
    {
        m_interpolationParameters.insert(inName, inType);
        vertex().addOutgoing(inName, inType);
        fragment().addIncoming(inName, inType);
        if (hasTessellation()) {
            QByteArray nameBuilder(inName);
            nameBuilder.append("TC");
            tessControl().addOutgoing(nameBuilder, inType);

            nameBuilder = inName;
            if (programGenerator()->getEnabledStages() & ShaderGeneratorStages::Geometry) {
                nameBuilder.append("TE");
                geometry().addOutgoing(inName, inType);
            }
            tessEval().addOutgoing(nameBuilder, inType);
        }
    }

    QDemonShaderStageGeneratorInterface &activeStage() override { return vertex(); }
};

QDemonRef<QDemonRenderShaderProgram> QDemonRendererImpl::generateShader(QDemonSubsetRenderable &inRenderable,
                                                                        const TShaderFeatureSet &inFeatureSet)
{
    // build a string that allows us to print out the shader we are generating to the log.
    // This is time consuming but I feel like it doesn't happen all that often and is very
    // useful to users
    // looking at the log file.
    const char *logPrefix("mesh subset pipeline-- ");

    m_generatedShaderString.clear();
    m_generatedShaderString = logPrefix;

    QDemonShaderDefaultMaterialKey theKey(inRenderable.shaderDescription);
    theKey.toString(m_generatedShaderString, m_defaultMaterialShaderKeyProperties);
    QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
    QDemonRef<QDemonRenderShaderProgram> cachedProgram = theCache->getProgram(m_generatedShaderString, inFeatureSet);
    if (cachedProgram)
        return cachedProgram;

    QDemonSubsetMaterialVertexPipeline pipeline(*this,
                                                inRenderable,
                                                m_defaultMaterialShaderKeyProperties.m_wireframeMode.getValue(theKey));
    return m_demonContext->getDefaultMaterialShaderGenerator()->generateShader(inRenderable.material,
                                                                               inRenderable.shaderDescription,
                                                                               pipeline,
                                                                               inFeatureSet,
                                                                               m_currentLayer->lights,
                                                                               inRenderable.firstImage,
                                                                               inRenderable.renderableFlags.hasTransparency(),
                                                                               logPrefix);
}

// --------------  Special cases for shadows  -------------------

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getParaboloidDepthShader(TessModeValues::Enum inTessMode)
{
    if (!m_demonContext->getRenderContext()->isTessellationSupported() || inTessMode == TessModeValues::NoTess) {
        return getParaboloidDepthNoTessShader();
    } else if (inTessMode == TessModeValues::TessLinear) {
        return getParaboloidDepthTessLinearShader();
    } else if (inTessMode == TessModeValues::TessPhong) {
        return getParaboloidDepthTessPhongShader();
    } else if (inTessMode == TessModeValues::TessNPatch) {
        return getParaboloidDepthTessNPatchShader();
    }

    return getParaboloidDepthNoTessShader();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getParaboloidDepthNoTessShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_paraboloidDepthShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "paraboloid depth shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthVertex(vertexShader);
            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getParaboloidDepthTessLinearShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_paraboloidDepthTessLinearShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "paraboloid depth tess linear shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            // vertexShader.AddOutgoing("world_pos", "vec4");
            vertexShader.addUniform("model_view_projection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            // vertexShader.Append("\tworld_pos = attr_pos;");
            vertexShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            // tessCtrlShader.AddOutgoing( "outUVTC", "vec2" );
            // tessCtrlShader.AddOutgoing( "outNormalTC", "vec3" );
            tessCtrlShader.append("void main() {\n");
            // tessCtrlShader.Append("\tctWorldPos[0] = outWorldPos[0];");
            // tessCtrlShader.Append("\tctWorldPos[1] = outWorldPos[1];");
            // tessCtrlShader.Append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(tessEvalShader);
            tessEvalShader.append("}");

            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
        }
        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getParaboloidDepthTessPhongShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_paraboloidDepthTessPhongShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "paraboloid depth tess phong shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            // vertexShader.AddOutgoing("world_pos", "vec4");
            vertexShader.addUniform("model_view_projection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            // vertexShader.Append("\tworld_pos = attr_pos;");
            vertexShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            // tessCtrlShader.AddOutgoing( "outUVTC", "vec2" );
            // tessCtrlShader.AddOutgoing( "outNormalTC", "vec3" );
            tessCtrlShader.append("void main() {\n");
            // tessCtrlShader.Append("\tctWorldPos[0] = outWorldPos[0];");
            // tessCtrlShader.Append("\tctWorldPos[1] = outWorldPos[1];");
            // tessCtrlShader.Append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(tessEvalShader);
            tessEvalShader.append("}");

            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
        }
        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getParaboloidDepthTessNPatchShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_paraboloidDepthTessNPatchShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "paraboloid depth tess NPatch shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            // vertexShader.AddOutgoing("world_pos", "vec4");
            vertexShader.addUniform("model_view_projection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            // vertexShader.Append("\tworld_pos = attr_pos;");
            vertexShader.append("}");

            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            // tessCtrlShader.AddOutgoing( "outUVTC", "vec2" );
            // tessCtrlShader.AddOutgoing( "outNormalTC", "vec3" );
            tessCtrlShader.append("void main() {\n");
            // tessCtrlShader.Append("\tctWorldPos[0] = outWorldPos[0];");
            // tessCtrlShader.Append("\tctWorldPos[1] = outWorldPos[1];");
            // tessCtrlShader.Append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(tessEvalShader);
            tessEvalShader.append("}");

            QDemonShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
        }
        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getCubeShadowDepthShader(TessModeValues::Enum inTessMode)
{
    if (!m_demonContext->getRenderContext()->isTessellationSupported() || inTessMode == TessModeValues::NoTess) {
        return getCubeDepthNoTessShader();
    } else if (inTessMode == TessModeValues::TessLinear) {
        return getCubeDepthTessLinearShader();
    } else if (inTessMode == TessModeValues::TessPhong) {
        return getCubeDepthTessPhongShader();
    } else if (inTessMode == TessModeValues::TessNPatch) {
        return getCubeDepthTessNPatchShader();
    }

    return getCubeDepthNoTessShader();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getCubeDepthNoTessShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_cubemapDepthShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "cubemap face depth shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram();
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );

            QDemonShaderProgramGeneratorInterface::outputCubeFaceDepthVertex(vertexShader);
            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QDemonShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram();
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getCubeDepthTessLinearShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_cubemapDepthTessLinearShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "cubemap face depth linear tess shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QDemonShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addUniform("model_matrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tworld_pos = model_matrix * pos;");
            tessEvalShader.append("\tworld_pos /= world_pos.w;");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getCubeDepthTessPhongShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_cubemapDepthTessPhongShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "cubemap face depth phong tess shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QDemonShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addUniform("model_matrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tworld_pos = model_matrix * pos;");
            tessEvalShader.append("\tworld_pos /= world_pos.w;");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getCubeDepthTessNPatchShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_cubemapDepthTessNPatchShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "cubemap face depth npatch tess shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QDemonShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("\toutNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addUniform("model_matrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tctNorm[0] = outNormalTC[0];");
            tessEvalShader.append("\tctNorm[1] = outNormalTC[1];");
            tessEvalShader.append("\tctNorm[2] = outNormalTC[2];");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tworld_pos = model_matrix * pos;");
            tessEvalShader.append("\tworld_pos /= world_pos.w;");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getOrthographicDepthShader(TessModeValues::Enum inTessMode)
{
    if (!m_demonContext->getRenderContext()->isTessellationSupported() || inTessMode == TessModeValues::NoTess) {
        return getOrthographicDepthNoTessShader();
    } else if (inTessMode == TessModeValues::TessLinear) {
        return getOrthographicDepthTessLinearShader();
    } else if (inTessMode == TessModeValues::TessPhong) {
        return getOrthographicDepthTessPhongShader();
    } else if (inTessMode == TessModeValues::TessNPatch) {
        return getOrthographicDepthTessNPatchShader();
    }

    return getOrthographicDepthNoTessShader();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getOrthographicDepthNoTessShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_orthographicDepthShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "orthographic depth shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");
            vertexShader.addOutgoing("outDepth", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("   gl_Position = model_view_projection * vec4( attr_pos, 1.0 );");
            vertexShader.append("   outDepth.x = gl_Position.z / gl_Position.w;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfloat depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getOrthographicDepthTessLinearShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_orthographicDepthTessLinearShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "orthographic depth tess linear shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfloat depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;");
            tessEvalShader.append("\toutDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getOrthographicDepthTessPhongShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_orthographicDepthTessPhongShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "orthographic depth tess phong shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfloat depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;");
            tessEvalShader.append("\toutDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getOrthographicDepthTessNPatchShader()
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthShader = m_orthographicDepthTessNPatchShader;

    if (theDepthShader.hasValue() == false) {
        QByteArray name = "orthographic depth tess npatch shader";

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");
            fragmentShader.addUniform("model_view_projection", "mat4");
            fragmentShader.addUniform("camera_properties", "vec2");
            fragmentShader.addUniform("camera_position", "vec3");
            fragmentShader.addUniform("camera_direction", "vec3");
            fragmentShader.addInclude("depthpass.glsllib");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            // fragmentShader.Append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("\tfloat depth = (outDepth.x - camera_properties.x) / "
                                  "(camera_properties.y - camera_properties.x);");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.addUniform("model_matrix", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;");
            tessEvalShader.append("\toutDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader.getValue();
}

// ---------------------------------

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getDepthPrepassShader(bool inDisplaced)
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthPrePassShader = (!inDisplaced)
            ? m_depthPrepassShader
            : m_depthPrepassShaderDisplaced;

    if (theDepthPrePassShader.hasValue() == false) {
        // check if we do displacement mapping
        QByteArray name = "depth prepass shader";
        if (inDisplaced)
            name.append(" displacement");

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");

            vertexShader.append("void main() {");

            if (inDisplaced) {
                getDemonContext()->getDefaultMaterialShaderGenerator()->addDisplacementMappingForDepthPass(vertexShader);
            } else {
                vertexShader.append("\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
            }
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram();
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthPrePassShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthPrePassShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }
    return theDepthPrePassShader.getValue();
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getDepthTessPrepassShader(TessModeValues::Enum inTessMode, bool inDisplaced)
{
    if (!m_demonContext->getRenderContext()->isTessellationSupported() || inTessMode == TessModeValues::NoTess) {
        return getDepthPrepassShader(inDisplaced);
    } else if (inTessMode == TessModeValues::TessLinear) {
        return getDepthTessLinearPrepassShader(inDisplaced);
    } else if (inTessMode == TessModeValues::TessPhong) {
        return getDepthTessPhongPrepassShader();
    } else if (inTessMode == TessModeValues::TessNPatch) {
        return getDepthTessNPatchPrepassShader();
    }

    return getDepthPrepassShader(inDisplaced);
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getDepthTessLinearPrepassShader(bool inDisplaced)
{
    QDemonOption<QDemonRef<QDemonRenderableDepthPrepassShader>> &theDepthPrePassShader = (!inDisplaced)
            ? m_depthTessLinearPrepassShader
            : m_depthTessLinearPrepassShaderDisplaced;

    if (theDepthPrePassShader.hasValue() == false) {
        // check if we do displacement mapping
        QByteArray name = "depth tess linear prepass shader";
        if (inDisplaced)
            name.append(" displacement");

        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            if (inDisplaced) {
                vertexShader.addIncoming("attr_uv0", "vec2");
                vertexShader.addIncoming("attr_norm", "vec3");

                vertexShader.addUniform("displacementMap_rot", "vec4");
                vertexShader.addUniform("displacementMap_offset", "vec3");

                vertexShader.addOutgoing("outNormal", "vec3");
                vertexShader.addOutgoing("outUV", "vec2");
            }
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");
            vertexShader.addUniform("model_matrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            if (inDisplaced) {
                vertexShader.append("\toutNormal = attr_norm;");
                vertexShader.append("\tvec3 uTransform = vec3( displacementMap_rot.x, "
                                    "displacementMap_rot.y, displacementMap_offset.x );");
                vertexShader.append("\tvec3 vTransform = vec3( displacementMap_rot.z, "
                                    "displacementMap_rot.w, displacementMap_offset.y );");
                vertexShader.addInclude("defaultMaterialLighting.glsllib"); // getTransformedUVCoords is in the
                // lighting code addition.
                vertexShader << "\tvec2 uv_coords = attr_uv0;"
                             << "\n";
                vertexShader << "\toutUV = getTransformedUVCoords( vec3( uv_coords, 1.0), "
                                "uTransform, vTransform );\n";
            }
            vertexShader.append("\toutWorldPos = (model_matrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.addOutgoing("outUVTC", "vec2");
            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("\tctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");

            if (inDisplaced) {
                tessCtrlShader.append("\toutUVTC[gl_InvocationID] = outUV[gl_InvocationID];");
                tessCtrlShader.append("\toutNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];");
            }

            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            if (inDisplaced) {
                tessEvalShader.addUniform("displacementSampler", "sampler2D");
                tessEvalShader.addUniform("displaceAmount", "float");
                tessEvalShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
            }
            tessEvalShader.addOutgoing("outUV", "vec2");
            tessEvalShader.addOutgoing("outNormal", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");

            if (inDisplaced) {
                tessEvalShader << "\toutUV = gl_TessCoord.x * outUVTC[0] + gl_TessCoord.y * "
                                  "outUVTC[1] + gl_TessCoord.z * outUVTC[2];"
                               << "\n";
                tessEvalShader << "\toutNormal = gl_TessCoord.x * outNormalTC[0] + gl_TessCoord.y * "
                                  "outNormalTC[1] + gl_TessCoord.z * outNormalTC[2];"
                               << "\n";
                tessEvalShader << "\tvec3 displacedPos = defaultMaterialFileDisplacementTexture( "
                                  "displacementSampler , displaceAmount, outUV , outNormal, pos.xyz );"
                               << "\n";
                tessEvalShader.append("\tgl_Position = model_view_projection * vec4(displacedPos, 1.0);");
            } else
                tessEvalShader.append("\tgl_Position = model_view_projection * pos;");

            tessEvalShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
        }

        QDemonShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, TShaderFeatureSet());

        if (depthShaderProgram) {
            theDepthPrePassShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            theDepthPrePassShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }
    return theDepthPrePassShader;
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getDepthTessPhongPrepassShader()
{
    if (m_depthTessPhongPrepassShader.hasValue() == false) {
        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QByteArray name = "depth tess phong prepass shader";
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");
            vertexShader.addUniform("model_matrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutWorldPos = (model_matrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("\tctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;\n");
            tessEvalShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
        }

        QDemonShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, TShaderFeatureSet());

        if (depthShaderProgram) {
            m_depthTessPhongPrepassShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            m_depthTessPhongPrepassShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }
    return m_depthTessPhongPrepassShader;
}

QDemonRef<QDemonRenderableDepthPrepassShader> QDemonRendererImpl::getDepthTessNPatchPrepassShader()
{
    if (m_depthTessNPatchPrepassShader.hasValue() == false) {
        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QByteArray name = "depth tess npatch prepass shader";
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
            QDemonShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessControl));
            QDemonShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::TessEval));
            QDemonShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("model_view_projection", "mat4");
            vertexShader.addUniform("model_matrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutWorldPos = (model_matrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("\tctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tctTangent[0] = outNormal[0];"); // we don't care for the tangent
            tessCtrlShader.append("\tctTangent[1] = outNormal[1];");
            tessCtrlShader.append("\tctTangent[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("\toutNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("model_view_projection", "mat4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tctNorm[0] = outNormalTC[0];");
            tessEvalShader.append("\tctNorm[1] = outNormalTC[1];");
            tessEvalShader.append("\tctNorm[2] = outNormalTC[2];");
            tessEvalShader.append("\tctTangent[0] = outNormalTC[0];"); // we don't care for the tangent
            tessEvalShader.append("\tctTangent[1] = outNormalTC[1];");
            tessEvalShader.append("\tctTangent[2] = outNormalTC[2];");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = model_view_projection * pos;\n");
            tessEvalShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex | ShaderGeneratorStages::TessControl
                                               | ShaderGeneratorStages::TessEval | ShaderGeneratorStages::Fragment));
        }

        QDemonShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, TShaderFeatureSet());

        if (depthShaderProgram) {
            m_depthTessNPatchPrepassShader = QDemonRef<QDemonRenderableDepthPrepassShader>(
                    new QDemonRenderableDepthPrepassShader(depthShaderProgram, getContext()));
        } else {
            m_depthTessNPatchPrepassShader = QDemonRef<QDemonRenderableDepthPrepassShader>();
        }
    }
    return m_depthTessNPatchPrepassShader;
}

QDemonRef<QDemonDefaultAoPassShader> QDemonRendererImpl::getDefaultAoPassShader(TShaderFeatureSet inFeatureSet)
{
    if (m_defaultAoPassShader.hasValue() == false) {
        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QByteArray name = "fullscreen AO pass shader";
        QDemonRef<QDemonRenderShaderProgram> aoPassShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!aoPassShaderProgram) {
            getProgramGenerator()->beginProgram();
            QDemonShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("uv_coords", "vec2");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0 );");
            theVertexGenerator.append("\tuv_coords = attr_uv;");
            theVertexGenerator.append("}");

            // fragmentGenerator.AddInclude( "SSAOCustomMaterial.glsllib" );
            theFragmentGenerator.addInclude("viewProperties.glsllib");
            theFragmentGenerator.addInclude("screenSpaceAO.glsllib");
            if (m_context->getRenderContextType() == QDemonRenderContextType::GLES2) {
                theFragmentGenerator << "\tuniform vec4 ao_properties;"
                                     << "\n"
                                     << "\tuniform vec4 ao_properties2;"
                                     << "\n"
                                     << "\tuniform vec4 shadow_properties;"
                                     << "\n"
                                     << "\tuniform vec4 aoScreenConst;"
                                     << "\n"
                                     << "\tuniform vec4 UvToEyeConst;"
                                     << "\n";
            } else {
                theFragmentGenerator << "layout (std140) uniform cbAoShadow { "
                                     << "\n"
                                     << "\tvec4 ao_properties;"
                                     << "\n"
                                     << "\tvec4 ao_properties2;"
                                     << "\n"
                                     << "\tvec4 shadow_properties;"
                                     << "\n"
                                     << "\tvec4 aoScreenConst;"
                                     << "\n"
                                     << "\tvec4 UvToEyeConst;"
                                     << "\n"
                                     << "};"
                                     << "\n";
            }
            theFragmentGenerator.addUniform("camera_direction", "vec3");
            theFragmentGenerator.addUniform("depth_sampler", "sampler2D");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator << "\tfloat aoFactor;"
                                 << "\n";
            theFragmentGenerator << "\tvec3 screenNorm;"
                                 << "\n";

            // We're taking multiple depth samples and getting the derivatives at each of them
            // to get a more
            // accurate view space normal vector.  When we do only one, we tend to get bizarre
            // values at the edges
            // surrounding objects, and this also ends up giving us weird AO values.
            // If we had a proper screen-space normal map, that would also do the trick.
            if (m_context->getRenderContextType() == QDemonRenderContextType::GLES2) {
                theFragmentGenerator.addUniform("depth_sampler_size", "vec2");
                theFragmentGenerator.append("\tivec2 iCoords = ivec2( gl_FragCoord.xy );");
                theFragmentGenerator.append("\tfloat depth = getDepthValue( "
                                            "texture2D(depth_sampler, vec2(iCoords)"
                                            " / depth_sampler_size), camera_properties );");
                theFragmentGenerator.append("\tdepth = depthValueToLinearDistance( depth, camera_properties );");
                theFragmentGenerator.append("\tdepth = (depth - camera_properties.x) / "
                                            "(camera_properties.y - camera_properties.x);");
                theFragmentGenerator.append("\tfloat depth2 = getDepthValue( "
                                            "texture2D(depth_sampler, vec2(iCoords+ivec2(1))"
                                            " / depth_sampler_size), camera_properties );");
                theFragmentGenerator.append("\tdepth2 = depthValueToLinearDistance( depth, camera_properties );");
                theFragmentGenerator.append("\tfloat depth3 = getDepthValue( "
                                            "texture2D(depth_sampler, vec2(iCoords-ivec2(1))"
                                            " / depth_sampler_size), camera_properties );");
            } else {
                theFragmentGenerator.append("\tivec2 iCoords = ivec2( gl_FragCoord.xy );");
                theFragmentGenerator.append("\tfloat depth = getDepthValue( "
                                            "texelFetch(depth_sampler, iCoords, 0), "
                                            "camera_properties );");
                theFragmentGenerator.append("\tdepth = depthValueToLinearDistance( depth, camera_properties );");
                theFragmentGenerator.append("\tdepth = (depth - camera_properties.x) / "
                                            "(camera_properties.y - camera_properties.x);");
                theFragmentGenerator.append("\tfloat depth2 = getDepthValue( "
                                            "texelFetch(depth_sampler, iCoords+ivec2(1), 0), "
                                            "camera_properties );");
                theFragmentGenerator.append("\tdepth2 = depthValueToLinearDistance( depth, camera_properties );");
                theFragmentGenerator.append("\tfloat depth3 = getDepthValue( "
                                            "texelFetch(depth_sampler, iCoords-ivec2(1), 0), "
                                            "camera_properties );");
            }
            theFragmentGenerator.append("\tdepth3 = depthValueToLinearDistance( depth, camera_properties );");
            theFragmentGenerator.append("\tvec3 tanU = vec3(10, 0, dFdx(depth));");
            theFragmentGenerator.append("\tvec3 tanV = vec3(0, 10, dFdy(depth));");
            theFragmentGenerator.append("\tscreenNorm = normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("\ttanU = vec3(10, 0, dFdx(depth2));");
            theFragmentGenerator.append("\ttanV = vec3(0, 10, dFdy(depth2));");
            theFragmentGenerator.append("\tscreenNorm += normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("\ttanU = vec3(10, 0, dFdx(depth3));");
            theFragmentGenerator.append("\ttanV = vec3(0, 10, dFdy(depth3));");
            theFragmentGenerator.append("\tscreenNorm += normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("\tscreenNorm = -normalize(screenNorm);");

            theFragmentGenerator.append("\taoFactor = \
                                        SSambientOcclusion( depth_sampler, screenNorm, ao_properties, ao_properties2, \
                                                            camera_properties, aoScreenConst, UvToEyeConst );");

            theFragmentGenerator.append("\tgl_FragColor = vec4(aoFactor, aoFactor, aoFactor, 1.0);");

            theFragmentGenerator.append("}");
        }

        aoPassShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), inFeatureSet);

        if (aoPassShaderProgram) {
            m_defaultAoPassShader = QDemonRef<QDemonDefaultAoPassShader>(
                    new QDemonDefaultAoPassShader(aoPassShaderProgram, getContext()));
        } else {
            m_defaultAoPassShader = QDemonRef<QDemonDefaultAoPassShader>();
        }
    }
    return m_defaultAoPassShader;
}

QDemonRef<QDemonDefaultAoPassShader> QDemonRendererImpl::getFakeDepthShader(TShaderFeatureSet inFeatureSet)
{
    if (m_fakeDepthShader.hasValue() == false) {
        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QByteArray name = "depth display shader";
        QDemonRef<QDemonRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QDemonShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("uv_coords", "vec2");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0 );");
            theVertexGenerator.append("\tuv_coords = attr_uv;");
            theVertexGenerator.append("}");

            theFragmentGenerator.addUniform("depth_sampler", "sampler2D");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator.append("\tivec2 iCoords = ivec2( gl_FragCoord.xy );");
            theFragmentGenerator.append("\tfloat depSample = texelFetch(depth_sampler, iCoords, 0).x;");
            theFragmentGenerator.append("\tgl_FragColor = vec4( depSample, depSample, depSample, 1.0 );");
            theFragmentGenerator.append("\treturn;");
            theFragmentGenerator.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), inFeatureSet);

        if (depthShaderProgram) {
            m_fakeDepthShader = QDemonRef<QDemonDefaultAoPassShader>(new QDemonDefaultAoPassShader(depthShaderProgram, getContext()));
        } else {
            m_fakeDepthShader = QDemonRef<QDemonDefaultAoPassShader>();
        }
    }
    return m_fakeDepthShader;
}

QDemonRef<QDemonDefaultAoPassShader> QDemonRendererImpl::getFakeCubeDepthShader(TShaderFeatureSet inFeatureSet)
{
    if (!m_fakeCubemapDepthShader.hasValue()) {
        QDemonRef<QDemonShaderCacheInterface> theCache = m_demonContext->getShaderCache();
        QByteArray name = "cube depth display shader";
        QDemonRef<QDemonRenderShaderProgram> cubeShaderProgram = theCache->getProgram(name, TShaderFeatureSet());
        if (!cubeShaderProgram) {
            getProgramGenerator()->beginProgram();
            QDemonShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
            QDemonShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("sample_dir", "vec3");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0 );");
            theVertexGenerator.append("\tsample_dir = vec3(4.0 * (attr_uv.x - 0.5), -1.0, 4.0 * (attr_uv.y - 0.5));");
            theVertexGenerator.append("}");
            theFragmentGenerator.addUniform("depth_cube", "samplerCube");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator.append("\tfloat smpDepth = texture( depth_cube, sample_dir ).x;");
            theFragmentGenerator.append("\tgl_FragColor = vec4(smpDepth, smpDepth, smpDepth, 1.0);");
            theFragmentGenerator.append("}");
        }

        cubeShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QDemonShaderCacheProgramFlags(), inFeatureSet);

        if (cubeShaderProgram) {
            m_fakeCubemapDepthShader = QDemonRef<QDemonDefaultAoPassShader>(
                    new QDemonDefaultAoPassShader(cubeShaderProgram, getContext()));
        } else {
            m_fakeCubemapDepthShader = QDemonRef<QDemonDefaultAoPassShader>();
        }
    }
    return m_fakeCubemapDepthShader.getValue();
}

QDemonTextRenderHelper QDemonRendererImpl::getTextShader(bool inUsePathRendering)
{
    QScopedPointer<QDemonTextShader> &thePtr = (!inUsePathRendering) ? m_textShader : m_textPathShader;
    if (thePtr)
        return QDemonTextRenderHelper(thePtr.get(), m_quadInputAssembler);

    QDemonRef<QDemonRenderShaderProgram> theShader = nullptr;
    QDemonRef<QDemonRenderProgramPipeline> thePipeline = nullptr;

    if (!inUsePathRendering) {
        getProgramGenerator()->beginProgram();
        QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
        QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        // xy of text dimensions are scaling factors, zw are offset factors.
        vertexGenerator.addUniform("text_dimensions", "vec4");
        vertexGenerator.addUniform("model_view_projection", "mat4");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator << "\tvec3 textPos = vec3(attr_pos.x * text_dimensions.x + text_dimensions.z"
                        << ", attr_pos.y * text_dimensions.y + text_dimensions.w"
                        << ", attr_pos.z);"
                        << "\n";

        vertexGenerator.append("\tgl_Position = model_view_projection * vec4(textPos, 1.0);");
        vertexGenerator.append("\tuv_coords = attr_uv;");

        fragmentGenerator.addUniform("text_textcolor", "vec4");
        fragmentGenerator.addUniform("text_textdimensions", "vec3");
        fragmentGenerator.addUniform("text_image", "sampler2D");
        fragmentGenerator.addUniform("text_backgroundcolor", "vec3");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("\tvec2 theCoords = uv_coords;");
        // Enable rendering from a sub-rect

        fragmentGenerator << "\ttheCoords.x = theCoords.x * text_textdimensions.x;"
                          << "\n"
                          << "\ttheCoords.y = theCoords.y * text_textdimensions.y;"
                          << "\n"
                          // flip the y uv coord if the dimension's z variable is set
                          << "\tif ( text_textdimensions.z > 0.0 ) theCoords.y = 1.0 - theCoords.y;"
                          << "\n";
        fragmentGenerator.append("\tvec4 c = texture2D(text_image, theCoords);");
        fragmentGenerator.append("\tfragOutput = vec4(mix(text_backgroundcolor.rgb, "
                                 "text_textcolor.rgb, c.rgb), c.a) * text_textcolor.a;");

        vertexGenerator.append("}");
        fragmentGenerator.append("}");
        const char *shaderName = "text shader";
        theShader = getProgramGenerator()->compileGeneratedShader(shaderName, QDemonShaderCacheProgramFlags(), TShaderFeatureSet(), false);
    } else {
        getProgramGenerator()->beginProgram(TShaderGeneratorStageFlags(ShaderGeneratorStages::Fragment));
        QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

        fragmentGenerator.addUniform("text_textcolor", "vec4");
        fragmentGenerator.addUniform("text_backgroundcolor", "vec3");

        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("\tfragOutput = vec4(mix(text_backgroundcolor.rgb, "
                                 "text_textcolor.rgb, text_textcolor.a), text_textcolor.a );");
        fragmentGenerator.append("}");

        const char *shaderName = "text path shader";
        theShader = getProgramGenerator()->compileGeneratedShader(shaderName, QDemonShaderCacheProgramFlags(), TShaderFeatureSet(), true);

        // setup program pipeline
        if (theShader) {
            thePipeline = getContext()->createProgramPipeline();
            if (thePipeline) {
                thePipeline->setProgramStages(theShader, QDemonRenderShaderTypeFlags(QDemonRenderShaderTypeValue::Fragment));
            }
        }
    }

    if (theShader) {
        generateXYQuad();
        thePtr.reset(new QDemonTextShader(theShader, thePipeline));
    }
    return QDemonTextRenderHelper(thePtr.get(), m_quadInputAssembler);
}

QDemonRef<QDemonTextDepthShader> QDemonRendererImpl::getTextDepthShader()
{
    if (m_textDepthPrepassShader.hasValue() == false) {
        getProgramGenerator()->beginProgram();

        QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
        QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        // xy of text dimensions are scaling factors, zw are offset factors.
        vertexGenerator.addUniform("text_dimensions", "vec4");
        vertexGenerator.addUniform("model_view_projection", "mat4");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator << "\tvec3 textPos = vec3(attr_pos.x * text_dimensions.x + text_dimensions.z"
                        << ", attr_pos.y * text_dimensions.y + text_dimensions.w"
                        << ", attr_pos.z);"
                        << "\n";

        vertexGenerator.append("\tgl_Position = model_view_projection * vec4(textPos, 1.0);");
        vertexGenerator.append("\tuv_coords = attr_uv;");

        fragmentGenerator.addUniform("text_textdimensions", "vec3");
        fragmentGenerator.addUniform("text_image", "sampler2D");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("\tvec2 theCoords = uv_coords;");
        // Enable rendering from a sub-rect

        fragmentGenerator << "\ttheCoords.x = theCoords.x * text_textdimensions.x;"
                          << "\n"
                          << "\ttheCoords.y = theCoords.y * text_textdimensions.y;"
                          << "\n"
                          // flip the y uv coord if the dimension's z variable is set
                          << "\tif ( text_textdimensions.z > 0.0 ) theCoords.y = 1.0 - theCoords.y;"
                          << "\n";
        fragmentGenerator.append("\tfloat alpha_mask = texture2D( text_image, theCoords ).r;");
        fragmentGenerator.append("\tif ( alpha_mask < .05 ) discard;");
        vertexGenerator.append("}");
        fragmentGenerator.append("}");
        const char *shaderName = "text depth shader";
        QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()->compileGeneratedShader(shaderName,
                                                                                                       QDemonShaderCacheProgramFlags(),
                                                                                                       TShaderFeatureSet());
        if (theShader == nullptr) {
            m_textDepthPrepassShader = QDemonRef<QDemonTextDepthShader>();
        } else {
            generateXYQuad();
            m_textDepthPrepassShader = QDemonRef<QDemonTextDepthShader>(new QDemonTextDepthShader(theShader, m_quadInputAssembler));
        }
    }
    return m_textDepthPrepassShader;
}

QDemonTextRenderHelper QDemonRendererImpl::getTextWidgetShader()
{
    if (m_textWidgetShader)
        return QDemonTextRenderHelper(m_textWidgetShader.get(), m_quadInputAssembler);

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    // xy of text dimensions are scaling factors, zw are offset factors.
    vertexGenerator.addUniform("text_dimensions", "vec4");
    vertexGenerator.addUniform("model_view_projection", "mat4");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator << "\tvec3 textPos = vec3(attr_pos.x * text_dimensions.x + text_dimensions.z"
                    << ", attr_pos.y * text_dimensions.y + text_dimensions.w"
                    << ", attr_pos.z);"
                    << "\n";

    vertexGenerator.append("\tgl_Position = model_view_projection * vec4(textPos, 1.0);");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("text_textcolor", "vec4");
    fragmentGenerator.addUniform("text_textdimensions", "vec3");
    fragmentGenerator.addUniform("text_image", "sampler2D");
    fragmentGenerator.addUniform("text_backgroundcolor", "vec3");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec2 theCoords = uv_coords;");
    // Enable rendering from a sub-rect

    fragmentGenerator << "\ttheCoords.x = theCoords.x * text_textdimensions.x;"
                      << "\n"
                      << "\ttheCoords.y = theCoords.y * text_textdimensions.y;"
                      << "\n"
                      // flip the y uv coord if the dimension's z variable is set
                      << "\tif ( text_textdimensions.z > 0.0 ) theCoords.y = 1.0 - theCoords.y;"
                      << "\n";
    fragmentGenerator.append("\tfloat alpha_mask = texture2D( text_image, theCoords ).r * text_textcolor.a;");
    fragmentGenerator.append("\tfragOutput = vec4(mix(text_backgroundcolor.rgb, "
                             "text_textcolor.rgb, alpha_mask), 1.0 );");
    fragmentGenerator.append("}");
    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()->compileGeneratedShader("text widget shader",
                                                                                                   QDemonShaderCacheProgramFlags(),
                                                                                                   TShaderFeatureSet());

    if (theShader) {
        generateXYQuad();
        m_textWidgetShader.reset(new QDemonTextShader(theShader));
    }
    return QDemonTextRenderHelper(m_textWidgetShader.get(), m_quadInputAssembler);
}

QDemonTextRenderHelper QDemonRendererImpl::getOnscreenTextShader()
{
    if (m_textOnscreenShader)
        return QDemonTextRenderHelper(m_textOnscreenShader.get(), m_quadStripInputAssembler);

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addUniform("model_view_projection", "mat4");
    vertexGenerator.addUniform("vertex_offsets", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");

    vertexGenerator.append("\tvec3 pos = attr_pos + vec3(vertex_offsets, 0.0);");
    vertexGenerator.append("\tgl_Position = model_view_projection * vec4(pos, 1.0);");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("text_textcolor", "vec4");
    fragmentGenerator.addUniform("text_image", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tfloat alpha = texture2D( text_image, uv_coords ).a;");
    fragmentGenerator.append("\tfragOutput = vec4(text_textcolor.r, text_textcolor.g, text_textcolor.b, alpha);");
    fragmentGenerator.append("}");

    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("onscreen texture shader",
                                                                                      QDemonShaderCacheProgramFlags(),
                                                                                      TShaderFeatureSet());

    if (theShader) {
        generateXYQuadStrip();
        m_textOnscreenShader.reset(new QDemonTextShader(theShader));
    }
    return QDemonTextRenderHelper(m_textOnscreenShader.get(), m_quadStripInputAssembler);
}

QDemonRef<QDemonRenderShaderProgram> QDemonRendererImpl::getTextAtlasEntryShader()
{
    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addUniform("model_view_projection", "mat4");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");

    vertexGenerator.append("\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("text_image", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tfloat alpha = texture2D( text_image, uv_coords ).a;");
    fragmentGenerator.append("\tfragOutput = vec4(alpha, alpha, alpha, alpha);");
    fragmentGenerator.append("}");

    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("texture atlas entry shader",
                                                                                      QDemonShaderCacheProgramFlags(),
                                                                                      TShaderFeatureSet());

    return theShader;
}

QDemonTextRenderHelper QDemonRendererImpl::getShader(QDemonTextRenderable & /*inRenderable*/, bool inUsePathRendering)
{
    return getTextShader(inUsePathRendering);
}

QDemonRef<QDemonLayerSceneShader> QDemonRendererImpl::getSceneLayerShader()
{
    if (m_sceneLayerShader.hasValue())
        return m_sceneLayerShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));

    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    // xy of text dimensions are scaling factors, zw are offset factors.
    vertexGenerator.addUniform("layer_dimensions", "vec2");
    vertexGenerator.addUniform("model_view_projection", "mat4");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator << "\tvec3 layerPos = vec3(attr_pos.x * layer_dimensions.x / 2.0"
                    << ", attr_pos.y * layer_dimensions.y / 2.0"
                    << ", attr_pos.z);"
                    << "\n";

    vertexGenerator.append("\tgl_Position = model_view_projection * vec4(layerPos, 1.0);");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("layer_image", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec2 theCoords = uv_coords;\n");
    fragmentGenerator.append("\tvec4 theLayerTexture = texture2D( layer_image, theCoords );\n");
    fragmentGenerator.append("\tif( theLayerTexture.a == 0.0 ) discard;\n");
    fragmentGenerator.append("\tfragOutput = theLayerTexture;\n");
    fragmentGenerator.append("}");
    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()->compileGeneratedShader("layer shader",
                                                                                                   QDemonShaderCacheProgramFlags(),
                                                                                                   TShaderFeatureSet());
    QDemonRef<QDemonLayerSceneShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonLayerSceneShader>(new QDemonLayerSceneShader(theShader));
    m_sceneLayerShader = retval;
    return m_sceneLayerShader.getValue();
}

QDemonRef<QDemonLayerProgAABlendShader> QDemonRendererImpl::getLayerProgAABlendShader()
{
    if (m_layerProgAAShader.hasValue())
        return m_layerProgAAShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");
    fragmentGenerator.addUniform("accumulator", "sampler2D");
    fragmentGenerator.addUniform("last_frame", "sampler2D");
    fragmentGenerator.addUniform("blend_factors", "vec2");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec4 accum = texture2D( accumulator, uv_coords );");
    fragmentGenerator.append("\tvec4 lastFrame = texture2D( last_frame, uv_coords );");
    fragmentGenerator.append("\tgl_FragColor = accum*blend_factors.y + lastFrame*blend_factors.x;");
    fragmentGenerator.append("}");
    QDemonRef<QDemonRenderShaderProgram>
            theShader = getProgramGenerator()->compileGeneratedShader("layer progressiveAA blend shader",
                                                                      QDemonShaderCacheProgramFlags(),
                                                                      TShaderFeatureSet());
    QDemonRef<QDemonLayerProgAABlendShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonLayerProgAABlendShader>(new QDemonLayerProgAABlendShader(theShader));
    m_layerProgAAShader = retval;
    return m_layerProgAAShader.getValue();
}

QDemonRef<QDemonShadowmapPreblurShader> QDemonRendererImpl::getCubeShadowBlurXShader()
{
    if (m_cubeShadowBlurXShader.hasValue())
        return m_cubeShadowBlurXShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    // vertexGenerator.AddIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_pos.xy;");
    vertexGenerator.append("}");

    // This with the ShadowBlurYShader design for a 2-pass 5x5 (sigma=1.0)
    // Weights computed using -- http://dev.theomader.com/gaussian-kernel-calculator/
    fragmentGenerator.addUniform("camera_properties", "vec2");
    fragmentGenerator.addUniform("depthCube", "samplerCube");
    // fragmentGenerator.AddUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("layout(location = 0) out vec4 frag0;");
    fragmentGenerator.append("layout(location = 1) out vec4 frag1;");
    fragmentGenerator.append("layout(location = 2) out vec4 frag2;");
    fragmentGenerator.append("layout(location = 3) out vec4 frag3;");
    fragmentGenerator.append("layout(location = 4) out vec4 frag4;");
    fragmentGenerator.append("layout(location = 5) out vec4 frag5;");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tfloat ofsScale = camera_properties.x / 2500.0;");
    fragmentGenerator.append("\tvec3 dir0 = vec3(1.0, -uv_coords.y, -uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir1 = vec3(-1.0, -uv_coords.y, uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir2 = vec3(uv_coords.x, 1.0, uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir3 = vec3(uv_coords.x, -1.0, -uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir4 = vec3(uv_coords.x, -uv_coords.y, 1.0);");
    fragmentGenerator.append("\tvec3 dir5 = vec3(-uv_coords.x, -uv_coords.y, -1.0);");
    fragmentGenerator.append("\tfloat depth0;");
    fragmentGenerator.append("\tfloat depth1;");
    fragmentGenerator.append("\tfloat depth2;");
    fragmentGenerator.append("\tfloat outDepth;");
    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir0).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir0 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir0 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir0 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir0 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag0 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir1).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir1 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir1 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir1 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir1 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag1 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir2).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir2 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir2 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir2 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir2 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag2 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir3).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir3 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir3 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir3 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir3 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag3 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir4).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir4 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir4 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir4 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir4 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag4 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir5).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir5 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir5 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir5 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir5 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag5 = vec4(outDepth);");

    fragmentGenerator.append("}");

    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("cubemap shadow blur X shader",
                                                                                      QDemonShaderCacheProgramFlags(),
                                                                                      TShaderFeatureSet());
    QDemonRef<QDemonShadowmapPreblurShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonShadowmapPreblurShader>(new QDemonShadowmapPreblurShader(theShader));
    m_cubeShadowBlurXShader = retval;
    return m_cubeShadowBlurXShader.getValue();
}

QDemonRef<QDemonShadowmapPreblurShader> QDemonRendererImpl::getCubeShadowBlurYShader()
{
    if (m_cubeShadowBlurYShader.hasValue())
        return m_cubeShadowBlurYShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    // vertexGenerator.AddIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_pos.xy;");
    vertexGenerator.append("}");

    // This with the ShadowBlurXShader design for a 2-pass 5x5 (sigma=1.0)
    // Weights computed using -- http://dev.theomader.com/gaussian-kernel-calculator/
    fragmentGenerator.addUniform("camera_properties", "vec2");
    fragmentGenerator.addUniform("depthCube", "samplerCube");
    // fragmentGenerator.AddUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("layout(location = 0) out vec4 frag0;");
    fragmentGenerator.append("layout(location = 1) out vec4 frag1;");
    fragmentGenerator.append("layout(location = 2) out vec4 frag2;");
    fragmentGenerator.append("layout(location = 3) out vec4 frag3;");
    fragmentGenerator.append("layout(location = 4) out vec4 frag4;");
    fragmentGenerator.append("layout(location = 5) out vec4 frag5;");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tfloat ofsScale = camera_properties.x / 2500.0;");
    fragmentGenerator.append("\tvec3 dir0 = vec3(1.0, -uv_coords.y, -uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir1 = vec3(-1.0, -uv_coords.y, uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir2 = vec3(uv_coords.x, 1.0, uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir3 = vec3(uv_coords.x, -1.0, -uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir4 = vec3(uv_coords.x, -uv_coords.y, 1.0);");
    fragmentGenerator.append("\tvec3 dir5 = vec3(-uv_coords.x, -uv_coords.y, -1.0);");
    fragmentGenerator.append("\tfloat depth0;");
    fragmentGenerator.append("\tfloat depth1;");
    fragmentGenerator.append("\tfloat depth2;");
    fragmentGenerator.append("\tfloat outDepth;");
    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir0).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir0 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir0 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir0 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir0 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag0 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir1).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir1 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir1 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir1 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir1 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag1 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir2).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir2 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir2 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir2 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir2 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag2 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir3).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir3 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir3 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir3 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir3 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag3 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir4).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir4 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir4 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir4 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir4 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag4 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir5).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir5 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir5 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir5 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir5 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag5 = vec4(outDepth);");

    fragmentGenerator.append("}");

    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("cubemap shadow blur Y shader",
                                                                                      QDemonShaderCacheProgramFlags(),
                                                                                      TShaderFeatureSet());
    QDemonRef<QDemonShadowmapPreblurShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonShadowmapPreblurShader>(new QDemonShadowmapPreblurShader(theShader));
    m_cubeShadowBlurYShader = retval;
    return m_cubeShadowBlurYShader.getValue();
}

QDemonRef<QDemonShadowmapPreblurShader> QDemonRendererImpl::getOrthoShadowBlurXShader()
{
    if (m_orthoShadowBlurXShader.hasValue())
        return m_orthoShadowBlurXShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_uv.xy;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("camera_properties", "vec2");
    fragmentGenerator.addUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec2 ofsScale = vec2( camera_properties.x / 7680.0, 0.0 );");
    fragmentGenerator.append("\tfloat depth0 = texture(depthSrc, uv_coords).x;");
    fragmentGenerator.append("\tfloat depth1 = texture(depthSrc, uv_coords + ofsScale).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthSrc, uv_coords - ofsScale).x;");
    fragmentGenerator.append("\tfloat depth2 = texture(depthSrc, uv_coords + 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthSrc, uv_coords - 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tfloat outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfragOutput = vec4(outDepth);");
    fragmentGenerator.append("}");

    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("shadow map blur X shader",
                                                                                      QDemonShaderCacheProgramFlags(),
                                                                                      TShaderFeatureSet());
    QDemonRef<QDemonShadowmapPreblurShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonShadowmapPreblurShader>(new QDemonShadowmapPreblurShader(theShader));
    m_orthoShadowBlurXShader = retval;
    return m_orthoShadowBlurXShader.getValue();
}

QDemonRef<QDemonShadowmapPreblurShader> QDemonRendererImpl::getOrthoShadowBlurYShader()
{
    if (m_orthoShadowBlurYShader.hasValue())
        return m_orthoShadowBlurYShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_uv.xy;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("camera_properties", "vec2");
    fragmentGenerator.addUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec2 ofsScale = vec2( 0.0, camera_properties.x / 7680.0 );");
    fragmentGenerator.append("\tfloat depth0 = texture(depthSrc, uv_coords).x;");
    fragmentGenerator.append("\tfloat depth1 = texture(depthSrc, uv_coords + ofsScale).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthSrc, uv_coords - ofsScale).x;");
    fragmentGenerator.append("\tfloat depth2 = texture(depthSrc, uv_coords + 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthSrc, uv_coords - 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tfloat outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfragOutput = vec4(outDepth);");
    fragmentGenerator.append("}");

    QDemonRef<QDemonRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("shadow map blur Y shader",
                                                                                      QDemonShaderCacheProgramFlags(),
                                                                                      TShaderFeatureSet());
    QDemonRef<QDemonShadowmapPreblurShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonShadowmapPreblurShader>(new QDemonShadowmapPreblurShader(theShader));
    m_orthoShadowBlurYShader = retval;
    return m_orthoShadowBlurYShader.getValue();
}

#ifdef ADVANCED_BLEND_SW_FALLBACK
QDemonRef<QDemonAdvancedModeBlendShader> QDemonRendererImpl::getAdvancedBlendModeShader(AdvancedBlendModes::Enum blendMode)
{
    // Select between blend equations.
    if (blendMode == AdvancedBlendModes::Overlay) {
        return getOverlayBlendModeShader();
    } else if (blendMode == AdvancedBlendModes::ColorBurn) {
        return getColorBurnBlendModeShader();
    } else if (blendMode == AdvancedBlendModes::ColorDodge) {
        return getColorDodgeBlendModeShader();
    }
    return {};
}

QDemonRef<QDemonAdvancedModeBlendShader> QDemonRendererImpl::getOverlayBlendModeShader()
{
    if (m_advancedModeOverlayBlendShader.hasValue())
        return m_advancedModeOverlayBlendShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("base_layer", "sampler2D");
    fragmentGenerator.addUniform("blend_layer", "sampler2D");

    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec4 base = texture2D(base_layer, uv_coords);");
    fragmentGenerator.append("\tif (base.a != 0.0) base.rgb /= base.a;");
    fragmentGenerator.append("\telse base = vec4(0.0);");
    fragmentGenerator.append("\tvec4 blend = texture2D(blend_layer, uv_coords);");
    fragmentGenerator.append("\tif (blend.a != 0.0) blend.rgb /= blend.a;");
    fragmentGenerator.append("\telse blend = vec4(0.0);");

    fragmentGenerator.append("\tvec4 res = vec4(0.0);");
    fragmentGenerator.append("\tfloat p0 = base.a * blend.a;");
    fragmentGenerator.append("\tfloat p1 = base.a * (1.0 - blend.a);");
    fragmentGenerator.append("\tfloat p2 = blend.a * (1.0 - base.a);");
    fragmentGenerator.append("\tres.a = p0 + p1 + p2;");

    QDemonRef<QDemonRenderShaderProgram> theShader;
    fragmentGenerator.append("\tfloat f_rs_rd = (base.r < 0.5? (2.0 * base.r * blend.r) : "
                             "(1.0 - 2.0 * (1.0 - base.r) * (1.0 - blend.r)));");
    fragmentGenerator.append("\tfloat f_gs_gd = (base.g < 0.5? (2.0 * base.g * blend.g) : "
                             "(1.0 - 2.0 * (1.0 - base.g) * (1.0 - blend.g)));");
    fragmentGenerator.append("\tfloat f_bs_bd = (base.b < 0.5? (2.0 * base.b * blend.b) : "
                             "(1.0 - 2.0 * (1.0 - base.b) * (1.0 - blend.b)));");
    fragmentGenerator.append("\tres.r = f_rs_rd * p0 + base.r * p1 + blend.r * p2;");
    fragmentGenerator.append("\tres.g = f_gs_gd * p0 + base.g * p1 + blend.g * p2;");
    fragmentGenerator.append("\tres.b = f_bs_bd * p0 + base.b * p1 + blend.b * p2;");
    fragmentGenerator.append("\tgl_FragColor = vec4(res.rgb * res.a, res.a);");
    fragmentGenerator.append("}");
    theShader = getProgramGenerator()->compileGeneratedShader("advanced overlay shader",
                                                              QDemonShaderCacheProgramFlags(),
                                                              TShaderFeatureSet());

    QDemonRef<QDemonAdvancedModeBlendShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonAdvancedModeBlendShader>(new QDemonAdvancedModeBlendShader(theShader));
    m_advancedModeOverlayBlendShader = retval;
    return m_advancedModeOverlayBlendShader.getValue();
}

QDemonRef<QDemonAdvancedModeBlendShader> QDemonRendererImpl::getColorBurnBlendModeShader()
{
    if (m_advancedModeColorBurnBlendShader.hasValue())
        return m_advancedModeColorBurnBlendShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("base_layer", "sampler2D");
    fragmentGenerator.addUniform("blend_layer", "sampler2D");

    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec4 base = texture2D(base_layer, uv_coords);");
    fragmentGenerator.append("\tif (base.a != 0.0) base.rgb /= base.a;");
    fragmentGenerator.append("\telse base = vec4(0.0);");
    fragmentGenerator.append("\tvec4 blend = texture2D(blend_layer, uv_coords);");
    fragmentGenerator.append("\tif (blend.a != 0.0) blend.rgb /= blend.a;");
    fragmentGenerator.append("\telse blend = vec4(0.0);");

    fragmentGenerator.append("\tvec4 res = vec4(0.0);");
    fragmentGenerator.append("\tfloat p0 = base.a * blend.a;");
    fragmentGenerator.append("\tfloat p1 = base.a * (1.0 - blend.a);");
    fragmentGenerator.append("\tfloat p2 = blend.a * (1.0 - base.a);");
    fragmentGenerator.append("\tres.a = p0 + p1 + p2;");

    QDemonRef<QDemonRenderShaderProgram> theShader;
    fragmentGenerator.append("\tfloat f_rs_rd = ((base.r == 1.0) ? 1.0 : "
                             "(blend.r == 0.0) ? 0.0 : 1.0 - min(1.0, ((1.0 - base.r) / blend.r)));");
    fragmentGenerator.append("\tfloat f_gs_gd = ((base.g == 1.0) ? 1.0 : "
                             "(blend.g == 0.0) ? 0.0 : 1.0 - min(1.0, ((1.0 - base.g) / blend.g)));");
    fragmentGenerator.append("\tfloat f_bs_bd = ((base.b == 1.0) ? 1.0 : "
                             "(blend.b == 0.0) ? 0.0 : 1.0 - min(1.0, ((1.0 - base.b) / blend.b)));");
    fragmentGenerator.append("\tres.r = f_rs_rd * p0 + base.r * p1 + blend.r * p2;");
    fragmentGenerator.append("\tres.g = f_gs_gd * p0 + base.g * p1 + blend.g * p2;");
    fragmentGenerator.append("\tres.b = f_bs_bd * p0 + base.b * p1 + blend.b * p2;");
    fragmentGenerator.append("\tgl_FragColor =  vec4(res.rgb * res.a, res.a);");
    fragmentGenerator.append("}");

    theShader = getProgramGenerator()->compileGeneratedShader("advanced colorBurn shader",
                                                              QDemonShaderCacheProgramFlags(),
                                                              TShaderFeatureSet());
    QDemonRef<QDemonAdvancedModeBlendShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonAdvancedModeBlendShader>(new QDemonAdvancedModeBlendShader(theShader));
    m_advancedModeColorBurnBlendShader = retval;
    return m_advancedModeColorBurnBlendShader.getValue();
}

QDemonRef<QDemonAdvancedModeBlendShader> QDemonRendererImpl::getColorDodgeBlendModeShader()
{
    if (m_advancedModeColorDodgeBlendShader.hasValue())
        return m_advancedModeColorDodgeBlendShader.getValue();

    getProgramGenerator()->beginProgram();

    QDemonShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Vertex));
    QDemonShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(ShaderGeneratorStages::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("base_layer", "sampler2D");
    fragmentGenerator.addUniform("blend_layer", "sampler2D");

    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec4 base = texture2D(base_layer, uv_coords);");
    fragmentGenerator.append("\tif (base.a != 0.0) base.rgb /= base.a;");
    fragmentGenerator.append("\telse base = vec4(0.0);");
    fragmentGenerator.append("\tvec4 blend = texture2D(blend_layer, uv_coords);");
    fragmentGenerator.append("\tif (blend.a != 0.0) blend.rgb /= blend.a;");
    fragmentGenerator.append("\telse blend = vec4(0.0);");

    fragmentGenerator.append("\tvec4 res = vec4(0.0);");
    fragmentGenerator.append("\tfloat p0 = base.a * blend.a;");
    fragmentGenerator.append("\tfloat p1 = base.a * (1.0 - blend.a);");
    fragmentGenerator.append("\tfloat p2 = blend.a * (1.0 - base.a);");
    fragmentGenerator.append("\tres.a = p0 + p1 + p2;");

    QDemonRef<QDemonRenderShaderProgram> theShader;
    fragmentGenerator.append("\tfloat f_rs_rd = ((base.r == 0.0) ? 0.0 : "
                             "(blend.r == 1.0) ? 1.0 : min(base.r / (1.0 - blend.r), 1.0));");
    fragmentGenerator.append("\tfloat f_gs_gd = ((base.g == 0.0) ? 0.0 : "
                             "(blend.g == 1.0) ? 1.0 : min(base.g / (1.0 - blend.g), 1.0));");
    fragmentGenerator.append("\tfloat f_bs_bd = ((base.b == 0.0) ? 0.0 : "
                             "(blend.b == 1.0) ? 1.0 : min(base.b / (1.0 - blend.b), 1.0));");
    fragmentGenerator.append("\tres.r = f_rs_rd * p0 + base.r * p1 + blend.r * p2;");
    fragmentGenerator.append("\tres.g = f_gs_gd * p0 + base.g * p1 + blend.g * p2;");
    fragmentGenerator.append("\tres.b = f_bs_bd * p0 + base.b * p1 + blend.b * p2;");

    fragmentGenerator.append("\tgl_FragColor =  vec4(res.rgb * res.a, res.a);");
    fragmentGenerator.append("}");
    theShader = getProgramGenerator()->compileGeneratedShader("advanced colorDodge shader",
                                                              QDemonShaderCacheProgramFlags(),
                                                              TShaderFeatureSet());
    QDemonRef<QDemonAdvancedModeBlendShader> retval;
    if (theShader)
        retval = QDemonRef<QDemonAdvancedModeBlendShader>(new QDemonAdvancedModeBlendShader(theShader));
    m_advancedModeColorDodgeBlendShader = retval;
    return m_advancedModeColorDodgeBlendShader.getValue();
}
#endif
QT_END_NAMESPACE
