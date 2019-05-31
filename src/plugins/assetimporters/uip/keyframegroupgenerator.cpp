#include "keyframegroupgenerator.h"

#include <private/qdemonqmlutilities_p.h>

#include "propertymap.h"

QT_BEGIN_NAMESPACE

KeyframeGroupGenerator::KeyframeGroupGenerator()
{

}

KeyframeGroupGenerator::~KeyframeGroupGenerator()
{
    for (auto keyframeGroupMap : m_targetKeyframeMap.values())
        for (auto keyframeGroup : keyframeGroupMap.values())
            delete keyframeGroup;
}

void KeyframeGroupGenerator::addAnimation(const AnimationTrack &animation)
{
    auto keyframeGroupMap = m_targetKeyframeMap.find(animation.m_target);
    QStringList propertyParts = animation.m_property.split(".");
    QString property = propertyParts.at(0);
    QString field = QStringLiteral("x");
    if (propertyParts.count() > 1)
        field = propertyParts.last();

    if (keyframeGroupMap != m_targetKeyframeMap.end()) {
        // Check if the property name already exists in the keyframes
        auto keyframeGroup = keyframeGroupMap.value().find(property);
        if (keyframeGroup != keyframeGroupMap.value().end()) {
            auto keyframeGroupInstance = keyframeGroup.value();
            // verify the keyframe lists are the same size
            if (keyframeGroupInstance->keyframes.count() == animation.m_keyFrames.count()) {
                for (int i = 0; i < keyframeGroupInstance->keyframes.count(); ++i)
                    keyframeGroupInstance->keyframes[i]->setValue(animation.m_keyFrames[i].value, field);
            } else {
                qWarning() << "Keyframe lists are not the same size, bad things are going to happen";
            }
        } else {
            // if not then add a new property
            keyframeGroupMap.value().insert(property, new KeyframeGroup(animation, property, field));
        }
    } else {
        // Add a new KeyframeGroupMap
        auto keyframeGroupMap = KeyframeGroupMap();
        keyframeGroupMap.insert(property, new KeyframeGroup(animation, property, field));
        m_targetKeyframeMap.insert(animation.m_target, keyframeGroupMap);
    }
}

void KeyframeGroupGenerator::generateKeyframeGroups(QTextStream &output, int tabLevel)
{
    for (auto groupMaps : m_targetKeyframeMap.values())
        for (auto keyframeGroup : groupMaps.values())
            keyframeGroup->generateKeyframeGroupQml(output, tabLevel);
}

KeyframeGroupGenerator::KeyframeGroup::KeyFrame::KeyFrame(const AnimationTrack::KeyFrame &keyframe, ValueType type, const QString &field)
{
    valueType = type;
    time = keyframe.time;
    setValue(keyframe.value, field);
    c2time = keyframe.c2time;
    c2value = keyframe.c2value;
    c1time = keyframe.c1time;
    c1value = keyframe.c1value;

}

void KeyframeGroupGenerator::KeyframeGroup::KeyFrame::setValue(float newValue, const QString &field)
{
    if (field == QStringLiteral("x"))
        value.setX(newValue);
    else if (field == QStringLiteral("y"))
        value.setY(newValue);
    else if (field == QStringLiteral("z"))
        value.setZ(newValue);
    else if (field == QStringLiteral("w"))
        value.setW(newValue);
    else
        value.setX(newValue);
}

QString KeyframeGroupGenerator::KeyframeGroup::KeyFrame::valueToString() const
{
    if (valueType == ValueType::Float)
        return QString::number(double(value.x()));
    if (valueType == ValueType::Vector2D)
        return QString(QStringLiteral("Qt.vector2d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(")"));
    if (valueType == ValueType::Vector3D)
        return QString(QStringLiteral("Qt.vector3d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(")"));
    if (valueType == ValueType::Vector4D)
        return QString(QStringLiteral("Qt.vector4d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(", ") + QString::number(double(value.w())) +
                       QStringLiteral(")"));
    if (valueType == ValueType::Color)
        return QString(QStringLiteral("Qt.rgba(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(", ") + QString::number(double(value.w())) +
                       QStringLiteral(")"));
    Q_UNREACHABLE();
    return QString();
}

KeyframeGroupGenerator::KeyframeGroup::KeyframeGroup(const AnimationTrack &animation, const QString &p, const QString &field)
{
    type = KeyframeGroup::AnimationType(animation.m_type);
    target = animation.m_target;
    property = getQmlPropertyName(p); // convert to qml property
    isDynamic = animation.m_dynamic;
    for (const auto &keyframe : animation.m_keyFrames)
        keyframes.append(new KeyFrame(keyframe, getPropertyValueType(p), field));
}

KeyframeGroupGenerator::KeyframeGroup::~KeyframeGroup()
{
    for (auto keyframe : keyframes)
        delete keyframe;
}

void KeyframeGroupGenerator::KeyframeGroup::generateKeyframeGroupQml(QTextStream &output, int tabLevel) const
{
    output << QDemonQmlUtilities::insertTabs(tabLevel) << QStringLiteral("KeyframeGroup {") << endl;
    output << QDemonQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("target: ") << target->qmlId() << endl;
    output << QDemonQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("property: ") << QStringLiteral("\"") << property << QStringLiteral("\"") <<  endl;

    for (auto keyframe : keyframes) {
        output << QDemonQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("Keyframe {") << endl;

        output << QDemonQmlUtilities::insertTabs(tabLevel + 2) << QStringLiteral("frame: ") << keyframe->time << endl;
        output << QDemonQmlUtilities::insertTabs(tabLevel + 2) << QStringLiteral("value: ") << keyframe->valueToString() << endl;

        // ### Only linear supported at the moment, add support for EaseInOut and Bezier

        output << QDemonQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("}") << endl;
    }

    output << QDemonQmlUtilities::insertTabs(tabLevel) << QStringLiteral("}") << endl << endl;
}

KeyframeGroupGenerator::KeyframeGroup::KeyFrame::ValueType KeyframeGroupGenerator::KeyframeGroup::getPropertyValueType(const QString &propertyName) {

    PropertyMap *propertyMap = PropertyMap::instance();
    const auto properties = propertyMap->propertiesForType(target->type());
    if (properties->contains(propertyName)) {
        switch (properties->value(propertyName).type) {
        case Q3DS::PropertyType::FloatRange:
        case Q3DS::PropertyType::LongRange:
        case Q3DS::PropertyType::Float:
        case Q3DS::PropertyType::Long:
        case Q3DS::PropertyType::FontSize:
            return KeyFrame::ValueType::Float;
        case Q3DS::PropertyType::Float2:
            return KeyFrame::ValueType::Vector2D;
        case Q3DS::PropertyType::Vector:
        case Q3DS::PropertyType::Scale:
        case Q3DS::PropertyType::Rotation:
            return KeyFrame::ValueType::Vector3D;
        case Q3DS::PropertyType::Color:
            return KeyFrame::ValueType::Color;
        default:
            return KeyFrame::ValueType::Unhandled;
        }
    }
    return KeyFrame::ValueType::Unhandled;
}

QString KeyframeGroupGenerator::KeyframeGroup::getQmlPropertyName(const QString &propertyName)
{
    PropertyMap *propertyMap = PropertyMap::instance();
    const auto properties = propertyMap->propertiesForType(target->type());
    if (properties->contains(propertyName)) {
        return properties->value(propertyName).name;
    }

    return propertyName;
}

QT_END_NAMESPACE
