#include "qdemonutils.h"

#include <QtCore/QDir>

#include <cmath>

QT_BEGIN_NAMESPACE

float vec2::magnitude(const QVector2D &v)
{
    return ::sqrtf(v.x() * v.x() + v.y() * v.y());
}

QVector3D vec3::minimum(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(qMin(v1.x(), v2.x()),
                     qMin(v1.y(), v2.y()),
                     qMin(v1.z(), v2.z()));
}

QVector3D vec3::maximum(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(qMax(v1.x(), v2.x()),
                     qMax(v1.y(), v2.y()),
                     qMax(v1.z(), v2.z()));
}

bool vec3::isFinite(const QVector3D &v)
{
    return qIsFinite(v.x()) && qIsFinite(v.y()) && qIsFinite(v.z());
}

float vec3::magnitude(const QVector3D &v)
{
    return sqrtf(v.x() * v.x() + v.y() * v.y() + v.z() * v.z());
}

float vec3::magnitudeSquared(const QVector3D &v)
{
    return v.x() * v.x() + v.y() * v.y() + v.z() * v.z();
}


// This special normalize function normalizes a vector in place
// and returns the magnnitude (needed for compatiblity)
float vec3::normalize(QVector3D &v)
{
    const float m = vec3::magnitude(v);
    if (m > 0)
        v /= m;
    return m;
}

QVector3D mat33::transform(const QMatrix3x3 &m, const QVector3D &v)
{
    const QVector3D c0 = QVector3D(m(0,0), m(1,0), m(2,0));
    const QVector3D c1 = QVector3D(m(0,1), m(1,1), m(2,1));
    const QVector3D c2 = QVector3D(m(0,2), m(1,2), m(2,2));
    return c0 * v.x() + c1 * v.y() + c2 * v.z();
}

QMatrix3x3 mat33::getInverse(const QMatrix3x3 &m)
{
    //return column0.dot(column1.cross(column2));
    const QVector3D c0(m(0, 0), m(0, 1), m(0, 2));
    const QVector3D c1(m(1, 0), m(1, 1), m(1, 2));
    const QVector3D c2(m(2, 0), m(2, 1), m(2, 2));

    const float det = QVector3D::dotProduct(c0, QVector3D::crossProduct(c1, c2));
    QMatrix3x3 inverse;

    if (det != 0) {
        const float invDet = 1.0f / det;

        inverse(0, 0) = invDet * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2));
        inverse(0, 1) = invDet * -(m(0, 1) * m(2, 2) - m(2, 1) * m(0, 2));
        inverse(0, 2) = invDet * (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1));

        inverse(1, 0) = invDet * -(m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0));
        inverse(1, 1) = invDet * (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0));
        inverse(1, 2) = invDet * -(m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0));

        inverse(2, 0) = invDet * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
        inverse(2, 1) = invDet * -(m(0, 0) * m(2, 1) - m(0, 1) * m(2, 0));
        inverse(2, 2) = invDet * (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1));

        return inverse;
    } else {
        return QMatrix3x3();
    }
}

QMatrix3x3 mat44::getUpper3x3(const QMatrix4x4 &m)
{
    const float values[9] = {
        m(0, 0), m(0, 1), m(0, 2),
        m(1, 0), m(1, 1), m(1, 2),
        m(2, 0), m(2, 1), m(2, 2)
    };
    return QMatrix3x3(values);
}

QMatrix4x4 mat44::getInverse(const QMatrix4x4 &m)
{
    // inverse algorithm was written for row-major matrixes.
    const float *myPtr(m.constData());
#define mat(idx) myPtr[idx]

    float a00 = mat(0), a01 = mat(1), a02 = mat(2), a03 = mat(3), a10 = mat(4), a11 = mat(5),
            a12 = mat(6), a13 = mat(7), a20 = mat(8), a21 = mat(9), a22 = mat(10), a23 = mat(11),
            a30 = mat(12), a31 = mat(13), a32 = mat(14), a33 = mat(15),
#undef mat

            b00 = a00 * a11 - a01 * a10, b01 = a00 * a12 - a02 * a10, b02 = a00 * a13 - a03 * a10,
            b03 = a01 * a12 - a02 * a11, b04 = a01 * a13 - a03 * a11, b05 = a02 * a13 - a03 * a12,
            b06 = a20 * a31 - a21 * a30, b07 = a20 * a32 - a22 * a30, b08 = a20 * a33 - a23 * a30,
            b09 = a21 * a32 - a22 * a31, b10 = a21 * a33 - a23 * a31, b11 = a22 * a33 - a23 * a32,

            d = (b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06), invDet;

    // Calculate the determinant
    if (d != 0) {
        Q_ASSERT(false);
        return QMatrix4x4();
    }
    invDet = 1 / d;

    QMatrix4x4 retval;
    float *destPtr = retval.data();

#define dest(idx) destPtr[idx]
    dest(0) = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
    dest(1) = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet;
    dest(2) = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
    dest(3) = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet;
    dest(4) = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet;
    dest(5) = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
    dest(6) = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet;
    dest(7) = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
    dest(8) = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
    dest(9) = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet;
    dest(10) = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
    dest(11) = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet;
    dest(12) = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet;
    dest(13) = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
    dest(14) = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet;
    dest(15) = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;
#undef dest

    return retval;
}

QVector3D mat44::rotate(const QMatrix4x4 &m, const QVector3D &v)
{
    const QVector4D tmp = mat44::rotate(m, QVector4D(v.x(), v.y(), v.z(), 1.0f));
    return QVector3D(tmp.x(), tmp.y(), tmp.z());
}

QVector4D mat44::rotate(const QMatrix4x4 &m, const QVector4D &v)
{
    const QVector4D column0(m(0, 0), m(0, 1), m(0, 2), m(0, 3));
    const QVector4D column1(m(1, 0), m(1, 1), m(1, 2), m(1, 3));
    const QVector4D column2(m(2, 0), m(2, 1), m(2, 2), m(2, 3));
    return column0 * v.x() + column1 * v.y() + column2 * v.z();
}

QVector3D mat44::transform(const QMatrix4x4 &m, const QVector3D &v)
{
    const QVector4D tmp = mat44::transform(m, QVector4D(v.x(), v.y(), v.z(), 1.0f));
    return QVector3D(tmp.x(), tmp.y(), tmp.z());
}

QVector4D mat44::transform(const QMatrix4x4 &m, const QVector4D &v)
{
    const QVector4D column0(m(0, 0), m(0, 1), m(0, 2), m(0, 3));
    const QVector4D column1(m(1, 0), m(1, 1), m(1, 2), m(1, 3));
    const QVector4D column2(m(2, 0), m(2, 1), m(2, 2), m(2, 3));
    const QVector4D column3(m(3, 0), m(3, 1), m(3, 2), m(3, 3));
    return column0 * v.x() + column1 * v.y() + column2 * v.z() + column3 * v.w();
}

//QVector3D mat44::scale(const QMatrix4x4 &m, const QVector3D &v)
//{
//    const QVector4D v4(v.x(), v.y(), v.z(), 1.0f);
//    mat44::scale(m, v4);
//}

//QVector4D mat44::scale(const QMatrix4x4 &m, const QVector4D &v)
//{
//    QVector4D column0(m(0, 0), m(0, 1), m(0, 2), m(0, 3));
//    QVector4D column1(m(1, 0), m(1, 1), m(1, 2), m(1, 3));
//    QVector4D column2(m(2, 0), m(2, 1), m(2, 2), m(2, 3));
//    QVector4D column3(m(3, 0), m(3, 1), m(3, 2), m(3, 3));
//    column0 *= v.x();
//    column1 *= v.y();
//    column2 *= v.z();
//    column3 *= v.w();
//}

bool quant::isFinite(const QQuaternion &q)
{
    return qIsFinite(q.x()) && qIsFinite(q.y()) && qIsFinite(q.z()) && qIsFinite(q.scalar());
}

float quant::magnitude(const QQuaternion &q)
{
    return std::sqrt(q.x() * q.x() + q.y() * q.y() + q.z() * q.z() + q.scalar() * q.scalar());
}

bool quant::isSane(const QQuaternion &q)
{
    const float unitTolerance = float(1e-2);
    return isFinite(q) && qAbs(magnitude(q) - 1) < unitTolerance;
}

bool quant::isUnit(const QQuaternion &q)
{
    const float unitTolerance = float(1e-4);
    return isFinite(q) && qAbs(magnitude(q) - 1) < unitTolerance;
}

QVector3D quant::rotated(const QQuaternion &q, const QVector3D &v)
{
    const float vx = 2.0f * v.x();
    const float vy = 2.0f * v.y();
    const float vz = 2.0f * v.z();
    const float w2 = q.scalar() * q.scalar() - 0.5f;
    const float dot2 = (q.x() * vx + q.y() * vy + q.z() * vz);
    return QVector3D((vx * w2 + (q.y() * vz - q.z() * vy) * q.scalar() + q.x() * dot2),
                     (vy * w2 + (q.z() * vx - q.x() * vz) * q.scalar() + q.y() * dot2),
                     (vz * w2 + (q.x() * vy - q.y() * vx) * q.scalar() + q.z() * dot2));
}

QVector3D quant::inverseRotated(const QQuaternion &q, const QVector3D &v)
{
    const float vx = 2.0f * v.x();
    const float vy = 2.0f * v.y();
    const float vz = 2.0f * v.z();
    const float w2 = q.scalar() * q.scalar() - 0.5f;
    const float dot2 = (q.x() * vx + q.y() * vy + q.z() * vz);
    return QVector3D((vx * w2 - (q.y() * vz - q.z() * vy) * q.scalar() + q.x() * dot2),
                     (vy * w2 - (q.z() * vx - q.x() * vz) * q.scalar() + q.y() * dot2),
                     (vz * w2 - (q.x() * vy - q.y() * vx) * q.scalar() + q.z() * dot2));
}

void memZero(void *ptr, size_t size)
{
    memset(ptr, 0, size);
}

void memSet(void *ptr, quint8 value, size_t size)
{
    memset(ptr, value, size);
}


const char *nonNull(const char *src) { return src == nullptr ? "" : src; }


float radToDeg(const float a)
{
    return 57.29577951308232286465f * a;
}

double radToDeg(const double a)
{
    return 57.29577951308232286465 * a;
}

float degToRad(const float a)
{
    return 0.01745329251994329547f * a;
}

double degToRad(const double a)
{
    return 0.01745329251994329547 * a;
}

qint64 IOStream::positionHelper(const QIODevice &device, qint64 offset, IOStream::SeekPosition::Enum seekPosition)
{
    qint64 startPos = 0;

    switch (seekPosition) {
    case IOStream::SeekPosition::Begin:
        startPos = 0;
        break;
    case IOStream::SeekPosition::Current:
        startPos = device.pos();
        break;
    case IOStream::SeekPosition::End:
        startPos = device.size();
        break;
    default:
        break;
    }

    startPos += offset;
    if (device.size() == 0 && offset != 0) {
        Q_ASSERT(false);
        return 0;
    }
    if (startPos < 0) {
        Q_ASSERT(false);
        startPos = 0;
    }
    if (startPos >= device.size() && startPos != 0) {
        Q_ASSERT(false);
        startPos = device.size() - 1;
    }
    return startPos;
}


QString CFileTools::NormalizePathForQtUsage(const QString &path)
{
    // path can be a file path or a qrc URL string.

    QString filePath = QDir::cleanPath(path);

    if (filePath.startsWith(QLatin1String("qrc:/")))
        return filePath.mid(3);
    else
        return filePath;
}

namespace  {
bool IsAbsolute(const char *inPath, size_t inLen)
{
    if (inLen > 2 && inPath[1] == ':')
        return true;
    else if (inLen > 1 && (inPath[0] == '\\' || inPath[0] == '/'))
        return true;
    return false;
}
inline uint StrLen(const char *inType)
{
    uint retval = 0;
    while (inType && *inType) {
        ++retval;
        ++inType;
    }
    return retval;
}
//// State machine where you can add a character
//// and it will tell you how many characters to erase
//struct SPathStateMachine
//{
//    struct States
//    {
//        enum Enum {
//            NoState = 0, // Don't care
//            Slash, // Last char was either a forward or backward slash
//            Period, // Last char was a period
//            TwoPeriods, // Last two characters were periods
//        };
//    };
//    struct Actions
//    {
//        enum Enum {
//            NoAction = 0,
//            DeleteBack1Slash,
//            DeleteBack2Slashes,
//        };
//    };

//    States::Enum m_State;

//    SPathStateMachine()
//        : m_State(States::NoState)
//    {
//    }

//    Actions::Enum AnalyzeChar(QCharRef inChar)
//    {
//        switch (inChar.toLatin1()) {
//        case '\\':
//        case '/':
//            switch (m_State) {
//            case States::NoState:
//                m_State = States::Slash;
//                break;
//            case States::Period:
//                m_State = States::Slash;
//                return Actions::DeleteBack1Slash;

//            case States::TwoPeriods:
//                m_State = States::Slash;
//                return Actions::DeleteBack2Slashes;
//            case States::Slash:
//                return Actions::DeleteBack1Slash;
//            }
//            break;
//        case '.':
//            switch (m_State) {
//            case States::Slash:
//            case States::NoState:
//                m_State = States::Period;
//                break;
//            case States::Period:
//                m_State = States::TwoPeriods;
//                break;
//            case States::TwoPeriods:
//                break;
//            }
//            break;
//        default:
//            m_State = States::NoState;
//            break;
//        }
//        return Actions::NoAction;
//    }
//};
//inline bool DoDeleteBack1Slash(int &idx, QString &ioPath)
//{
//    int slashLoc = ioPath.indexOf('/', idx - 1);
//    if ((slashLoc != -1) && (slashLoc > 2)
//        // and the next *two* characters aren't both dots.
//        && ((ioPath[slashLoc - 1] != '.') || (ioPath[slashLoc - 2] != '.'))) {

//        ioPath.remove(ioPath.begin() + slashLoc, ioPath.begin() + idx);
//        idx = slashLoc;
//        return true;
//    }
//    return false;
//}

//void NormalizePath(QString &ioPath)
//{
//    int pathLen = ioPath.size();
//    SPathStateMachine theStateMachine;
//    for (int idx = 0; idx < pathLen; ++idx) {
//        QCharRef currentChar = ioPath[idx];
//        if (currentChar == '\\')
//            currentChar = QChar::fromLatin1('/');
//        SPathStateMachine::Actions::Enum action = theStateMachine.AnalyzeChar(currentChar);
//        switch (action) {
//        case SPathStateMachine::Actions::DeleteBack2Slashes:
//            if (DoDeleteBack1Slash(idx, ioPath))
//                DoDeleteBack1Slash(idx, ioPath);
//            pathLen = ioPath.size();
//            break;

//        case SPathStateMachine::Actions::DeleteBack1Slash:
//            DoDeleteBack1Slash(idx, ioPath);
//            pathLen = ioPath.size();
//            break;
//        default:
//            break;
//        }
//    }
//}

}
// ### Fix to Normalize Path
void CFileTools::CombineBaseAndRelative(const char *inBase, const char *inRelative, QString &outString)
{
    if (IsAbsolute(inRelative, StrLen(inRelative))) {
        outString = QString::fromLocal8Bit(nonNull(inRelative));
    } else {
        if (inRelative && *inRelative) {
            if (inRelative[0] == '#')
                outString = QString::fromLocal8Bit(inRelative);
            else {
                if (IsAbsolute(inRelative, strlen(inRelative))) {
                    outString = QString::fromLocal8Bit(inRelative);
                } else {
                    outString = inBase ? QString::fromLocal8Bit(inBase) : QString();
                    if (outString.size())
                        outString.append(QStringLiteral("/"));
                    outString.append(inRelative ? QString::fromLocal8Bit(inRelative) : QString());
                }
                //NormalizePath(outString);
            }
        }
    }
}


QT_END_NAMESPACE
