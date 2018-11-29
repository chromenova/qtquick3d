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
#include <Qt3DSRenderTextureAtlas.h>
#include <Qt3DSContainers.h>
#include <Qt3DSAtomic.h>
#include <Qt3DSFoundation.h>
#include <Qt3DSBroadcastingAllocator.h>
#include <qdemonrendertexture2d.h>
#include <qdemonrendercontext.h>

using namespace render;

namespace {

// a algorithm based on http://clb.demon.fi/files/RectangleBinPack/
struct STextureAtlasBinPackSL
{
public:
    STextureAtlasBinPackSL(QDemonRenderContext &inContext, qint32 width, qint32 height)
        : m_BinWidth(width)
        , m_BinHeight(height)
        , m_SkyLine(inContext.GetAllocator(), "STextureAtlasBinPackSL::m_SkyLine")
    {
        // setup first entry
        SSkylineNode theNode = { 0, 0, width };
        m_SkyLine.push_back(theNode);
    }

    ~STextureAtlasBinPackSL() { m_SkyLine.clear(); }

    /*	insert new rect
     *
     */
    STextureAtlasRect Insert(qint32 width, qint32 height)
    {
        qint32 binHeight;
        qint32 binWidth;
        qint32 binIndex;

        STextureAtlasRect newNode = findPosition(width, height, &binWidth, &binHeight, &binIndex);

        if (binIndex != -1) {
            // adjust skyline nodes
            addSkylineLevelNode(binIndex, newNode);
        }

        return newNode;
    }

private:
    /// Represents a single level (a horizontal line) of the skyline/horizon/envelope.
    struct SSkylineNode
    {
        int x; ///< The starting x-coordinate (leftmost).
        int y; ///< The y-coordinate of the skyline level line.
        int width; /// The line width. The ending coordinate (inclusive) will be x+width-1.
    };

    /*	find position
     *
     */
    STextureAtlasRect findPosition(qint32 width, qint32 height, qint32 *binWidth, qint32 *binHeight,
                                   qint32 *binIndex)
    {
        *binWidth = m_BinWidth;
        *binHeight = m_BinHeight;
        *binIndex = -1;
        STextureAtlasRect newRect;

        for (quint32 i = 0; i < m_SkyLine.size(); ++i) {
            qint32 y = getSkylineLevel(i, width, height);

            if (y >= 0) {
                if ((y + height < *binHeight)
                    || ((y + height == *binHeight) && m_SkyLine[i].width < *binWidth)) {
                    *binHeight = y + height;
                    *binIndex = i;
                    *binWidth = m_SkyLine[i].width;
                    newRect.m_X = m_SkyLine[i].x;
                    newRect.m_Y = y;
                    newRect.m_Width = width;
                    newRect.m_Height = height;
                }
            }
        }

        return newRect;
    }

    /* @brief check if rectangle can be placed into the the bin
     *
     * return skyline hight
     */
    int getSkylineLevel(quint32 binIndex, qint32 width, qint32 height)
    {
        // first check width exceed
        qint32 x = m_SkyLine[binIndex].x;
        if (x + width > m_BinWidth)
            return -1;

        qint32 leftAlign = width;
        quint32 index = binIndex;
        qint32 y = m_SkyLine[index].y;

        while (leftAlign > 0) {
            y = (y > m_SkyLine[index].y) ? y : m_SkyLine[index].y;
            // check hight
            if (y + height > m_BinHeight)
                return -1;

            leftAlign -= m_SkyLine[index].width;
            ++index;

            if (index > m_SkyLine.size())
                return -1;
        }

        return y;
    }

    /* @brief add an new skyline entry
     *
     * return no return
     */
    void addSkylineLevelNode(qint32 binIndex, const STextureAtlasRect &newRect)
    {
        SSkylineNode newNode;

        newNode.x = newRect.m_X;
        newNode.y = newRect.m_Y + newRect.m_Height;
        newNode.width = newRect.m_Width;
        m_SkyLine.insert(m_SkyLine.begin() + binIndex, newNode);

        // iterate over follow up nodes and adjust
        for (quint32 i = binIndex + 1; i < m_SkyLine.size(); ++i) {
            if (m_SkyLine[i].x < m_SkyLine[i - 1].x + m_SkyLine[i - 1].width) {
                int shrink = m_SkyLine[i - 1].x + m_SkyLine[i - 1].width - m_SkyLine[i].x;

                m_SkyLine[i].x += shrink;
                m_SkyLine[i].width -= shrink;

                if (m_SkyLine[i].width <= 0) {
                    m_SkyLine.erase(m_SkyLine.begin() + i);
                    --i;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        mergeSkylineLevelNodes();
    }

    /* @brief merge skyline node
     *
     * return no return
     */
    void mergeSkylineLevelNodes()
    {
        // check if we can merge nodes
        for (quint32 i = 0; i < m_SkyLine.size() - 1; ++i) {
            if (m_SkyLine[i].y == m_SkyLine[i + 1].y) {
                m_SkyLine[i].width += m_SkyLine[i + 1].width;
                m_SkyLine.erase(m_SkyLine.begin() + (i + 1));
                --i;
            }
        }
    }

    qint32 m_BinWidth;
    qint32 m_BinHeight;

    nvvector<SSkylineNode> m_SkyLine;
};

struct STextureAtlasEntry
{
    STextureAtlasEntry()
        : m_X(0)
        , m_Y(0)
        , m_Width(0)
        , m_Height(0)
        , m_pBuffer(QDemonDataRef<quint8>())
    {
    }
    STextureAtlasEntry(float x, float y, float w, float h, QDemonDataRef<quint8> buffer)
        : m_X(x)
        , m_Y(y)
        , m_Width(w)
        , m_Height(h)
        , m_pBuffer(buffer)
    {
    }
    STextureAtlasEntry(const STextureAtlasEntry &entry)
    {
        m_X = entry.m_X;
        m_Y = entry.m_Y;
        m_Width = entry.m_Width;
        m_Height = entry.m_Height;
        m_pBuffer = entry.m_pBuffer;
    }
    ~STextureAtlasEntry() {}

    float m_X, m_Y;
    float m_Width, m_Height;
    QDemonDataRef<quint8> m_pBuffer;
};

struct STextureAtlas : public ITextureAtlas
{
    NVFoundationBase &m_Foundation;
    volatile qint32 mRefCount;
    QDemonScopedRefCounted<QDemonRenderContext> m_RenderContext;

    STextureAtlas(NVFoundationBase &inFnd, QDemonRenderContext &inRenderContext, qint32 width,
                  qint32 height)
        : m_Foundation(inFnd)
        , mRefCount(0)
        , m_RenderContext(inRenderContext)
        , m_Width(width)
        , m_Height(height)
        , m_Spacing(1)
        , m_AtlasEntrys(inFnd.getAllocator(), "STextureAtlas::m_SkyLine")
    {
        m_pBinPack =
            QDEMON_NEW(inFnd.getAllocator(), STextureAtlasBinPackSL)(inRenderContext, width, height);
    }

    virtual ~STextureAtlas()
    {
        RelaseEntries();

        if (m_pBinPack)
            NVDelete(m_Foundation.getAllocator(), m_pBinPack);
    }

    void RelaseEntries() override
    {
        nvvector<STextureAtlasEntry>::iterator it;

        for (it = m_AtlasEntrys.begin(); it != m_AtlasEntrys.end(); it++) {
            QDEMON_FREE(m_Foundation.getAllocator(), it->m_pBuffer.begin());
        }

        m_AtlasEntrys.clear();
    }
    qint32 GetWidth() const override { return m_Width; }
    qint32 GetHeight() const override { return m_Height; }

    qint32 GetAtlasEntryCount() const override { return m_AtlasEntrys.size(); }

    TTextureAtlasEntryAndBuffer GetAtlasEntryByIndex(quint32 index) override
    {
        if (index >= m_AtlasEntrys.size())
            return eastl::make_pair(STextureAtlasRect(), QDemonDataRef<quint8>());

        return eastl::make_pair(STextureAtlasRect((qint32)m_AtlasEntrys[index].m_X,
                                                  (qint32)m_AtlasEntrys[index].m_Y,
                                                  (qint32)m_AtlasEntrys[index].m_Width,
                                                  (qint32)m_AtlasEntrys[index].m_Height),
                                m_AtlasEntrys[index].m_pBuffer);
    }

    QDEMON_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    STextureAtlasRect AddAtlasEntry(qint32 width, qint32 height, qint32 pitch,
                                    qint32 dataWidth, QDemonConstDataRef<quint8> bufferData) override
    {
        STextureAtlasRect rect;

        // pitch is the number of bytes per line in bufferData
        // dataWidth is the relevant data width in bufferData. Rest is padding that can be ignored.
        if (m_pBinPack) {
            qint32 paddedWith, paddedPitch, paddedHeight;
            // add spacing around the character
            paddedWith = width + 2 * m_Spacing;
            paddedPitch = dataWidth + 2 * m_Spacing;
            paddedHeight = height + 2 * m_Spacing;
            // first get entry in the texture atlas
            rect = m_pBinPack->Insert(paddedWith, paddedHeight);
            if (rect.m_Width == 0)
                return rect;

            // we align the data be to 4 byte
            int alignment = (4 - (paddedPitch % 4)) % 4;
            paddedPitch += alignment;

            // since we do spacing around the character we need to copy line by line
            quint8 *glyphBuffer =
                (quint8 *)QDEMON_ALLOC(m_Foundation.getAllocator(),
                                 paddedHeight * paddedPitch * sizeof(quint8), "STextureAtlas");
            if (glyphBuffer) {
                memset(glyphBuffer, 0, paddedHeight * paddedPitch);

                quint8 *pDst = glyphBuffer + paddedPitch + m_Spacing;
                quint8 *pSrc = const_cast<quint8 *>(bufferData.begin());
                for (qint32 i = 0; i < height; ++i) {
                    memcpy(pDst, pSrc, dataWidth);

                    pDst += paddedPitch;
                    pSrc += pitch;
                }

                // add new entry
                m_AtlasEntrys.push_back(STextureAtlasEntry(
                    (float)rect.m_X, (float)rect.m_Y, (float)paddedWith, (float)paddedHeight,
                    QDemonDataRef<quint8>(glyphBuffer, paddedHeight * paddedPitch * sizeof(quint8))));

                // normalize texture coordinates
                rect.m_NormX = (float)rect.m_X / (float)m_Width;
                rect.m_NormY = (float)rect.m_Y / (float)m_Height;
                rect.m_NormWidth = (float)paddedWith / (float)m_Width;
                rect.m_NormHeight = (float)paddedHeight / (float)m_Height;
            }
        }

        return rect;
    }

private:
    qint32 m_Width; ///< texture atlas width
    qint32 m_Height; ///< texture atlas height
    qint32 m_Spacing; ///< spacing around the entry
    nvvector<STextureAtlasEntry> m_AtlasEntrys; ///< our entries in the atlas
    STextureAtlasBinPackSL *m_pBinPack; ///< our bin packer which actually does most of the work
};

} // namespace

ITextureAtlas &ITextureAtlas::CreateTextureAtlas(NVFoundationBase &inFnd,
                                                 QDemonRenderContext &inRenderContext, qint32 width,
                                                 qint32 height)
{
    return *QDEMON_NEW(inFnd.getAllocator(), STextureAtlas)(inFnd, inRenderContext, width, height);
}
