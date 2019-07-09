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

#ifndef QDEMONQMLUTILITIES_P_H
#define QDEMONQMLUTILITIES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtdemonassetimportglobal.h>

#include <QString>
#include <QColor>
#include <QTextStream>

QT_BEGIN_NAMESPACE

namespace QDemonQmlUtilities {

class PropertyMap
{
public:
    enum Type {
        Node,
        Model,
        Camera,
        Light,
        DefaultMaterial,
        Image
    };

    typedef QHash<QString, QVariant> PropertiesMap;

    static PropertyMap *instance();

    PropertiesMap *propertiesForType(Type type);
    QVariant getDefaultValue(Type type, const QString &property);
    bool isDefaultValue(Type type, const QString &property, const QVariant &value);


private:
    PropertyMap();
    ~PropertyMap();

    QHash<Type, PropertiesMap *> m_properties;

};

QString Q_DEMONASSETIMPORT_EXPORT insertTabs(int n);
QString Q_DEMONASSETIMPORT_EXPORT qmlComponentName(const QString &name);
QString Q_DEMONASSETIMPORT_EXPORT colorToQml(const QColor &color);
QString Q_DEMONASSETIMPORT_EXPORT sanitizeQmlId(const QString &id);
QString Q_DEMONASSETIMPORT_EXPORT sanitizeQmlSourcePath(const QString &source, bool removeParentDirectory = false);

void Q_DEMONASSETIMPORT_EXPORT writeQmlPropertyHelper(QTextStream &output, int tabLevel, PropertyMap::Type type, const QString &propertyName, const QVariant &value);

}


QT_BEGIN_NAMESPACE

#endif // QDEMONQMLUTILITIES_P_H
