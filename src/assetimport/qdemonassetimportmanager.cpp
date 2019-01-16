/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include "qdemonassetimportmanager.h"

#include "qdemonassetimporter_p.h"
#include "qdemonassetimporterfactory_p.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QDemonAssetImportManager::QDemonAssetImportManager(QObject *parent) : QObject(parent)
{
    // load assetimporters
    const QStringList keys = QDemonAssetImporterFactory::keys();
    for (const auto &key : keys) {
        auto importer = QDemonAssetImporterFactory::create(key, QStringList());
        m_assetImporters.append(importer);
        // Add to extension map
        for (const auto &extension : importer->inputExtensions()) {
            m_extensionsMap.insert(extension, importer);
        }
    }
}

QDemonAssetImportManager::~QDemonAssetImportManager()
{
    for (auto importer : m_assetImporters) {
        delete importer;
    }
}

bool QDemonAssetImportManager::importFile(const QString &filename, const QDir &outputPath, QString *error)
{
    QFileInfo fileInfo(filename);

    // Is this a real file?
    if (!fileInfo.exists()) {
        if (error)
            *error = QStringLiteral("file does not exist");
        return false;
    }

    // Do we have a importer to load the file?
    const auto extension = fileInfo.completeSuffix();
    if (!m_extensionsMap.keys().contains(extension)) {
        if (error)
            *error = QStringLiteral("unsupported file extension %1").arg(extension);
        return false;
    }

    auto importer = m_extensionsMap[extension];
    QStringList generatedFiles;
    auto errorString = importer->import(fileInfo.absoluteFilePath(), outputPath, QVariantMap(), &generatedFiles);

    if (!errorString.isEmpty()) {
        if (error) {
            *error = QStringLiteral("%1").arg(errorString);
        }

        return false;
    }

    // debug output
    for (const auto &file : generatedFiles)
        qDebug() << "generated file: " << file;

    return true;
}

QT_END_NAMESPACE