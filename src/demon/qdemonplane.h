#ifndef QDEMONPLANE_H
#define QDEMONPLANE_H

#include <QtDemon/qtdemonglobal.h>
#include <QtGui/QVector3D>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

/**
\brief Representation of a plane.

 Plane equation used: n.dot(v) + d = 0
*/
class Q_DEMON_EXPORT QDemonPlane
{
public:
    /**
    \brief Constructor
    */
    Q_ALWAYS_INLINE QDemonPlane();

    /**
    \brief Constructor from a normal and a distance
    */
    Q_ALWAYS_INLINE QDemonPlane(float nx, float ny, float nz, float distance)
        : n(nx, ny, nz)
        , d(distance)
    {
    }

    /**
    \brief Constructor from a normal and a distance
    */
    Q_ALWAYS_INLINE QDemonPlane(const QVector3D &normal, float distance)
        : n(normal)
        , d(distance)
    {
    }

    /**
    \brief Constructor from a point on the plane and a normal
    */
    Q_ALWAYS_INLINE QDemonPlane(const QVector3D &point, const QVector3D &normal)
        : n(normal)
        , d(-QVector3D::dotProduct(point, n)) // p satisfies normal.dot(p) + d = 0
    {
    }

    /**
    \brief Constructor from three points
    */
    Q_ALWAYS_INLINE QDemonPlane(const QVector3D &p0, const QVector3D &p1, const QVector3D &p2)
    {
        n = QVector3D::crossProduct(p1 - p0, p2 - p0).normalized();
        d = QVector3D::dotProduct(-p0, n);
    }

    Q_ALWAYS_INLINE float distance(const QVector3D &p) const
    {
        return QVector3D::dotProduct(p, n) + d;
    }

    Q_ALWAYS_INLINE bool contains(const QVector3D &p) const
    {
        return qAbs(distance(p)) < (1.0e-7f);
    }

    /**
    \brief projects p into the plane
    */
    Q_ALWAYS_INLINE QVector3D project(const QVector3D &p) const
    {
        return p - n * distance(p);
    }

    /**
    \brief find an arbitrary point in the plane
    */
    Q_ALWAYS_INLINE QVector3D pointInPlane() const { return -n * d; }

    /**
    \brief equivalent plane with unit normal
    */

    Q_ALWAYS_INLINE void normalize()
;

    QVector3D n; //!< The normal to the plane
    float d; //!< The distance from the origin
};

QT_END_NAMESPACE

#endif // QDEMONPLANE_H
