
/************************************************************************
*    FILE NAME:       quaternion.h
*
*    DESCRIPTION:     3D quaternion class
************************************************************************/  

#ifndef __quaternion_h__
#define __quaternion_h__

// Game lib dependencies
#include <common/radian.h>
#include <common/defs.h>

// Turn off the data type conversion warning (ie. int to float, float to int etc.)
// We do this all the time in 3D. Don't need to be bugged by it all the time.
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)

class CQuaternion
{
public:

    // Quaternion values. 
    double x, y, z, w, length;

    // Fast init if class varaibles.
    CQuaternion():length(0)
    {
        Identity();
    }

    // Copy constructor
    CQuaternion( const CQuaternion & obj )
    {
        *this = obj;
    }

    // Constructor
    CQuaternion( double _x, double _y, double _z, double _w ):x(_x),y(_y),z(_z),w(_w)
    {
    }

    /************************************************************************                                                             
    *    desc:  Set to it's identity
    ************************************************************************/
    void Identity()
    {
        x = 0.;
        y = 0.;
        z = 0.;
        w = 1.;
    }

    /************************************************************************                                                             
    *    desc:  Set a rotation point - Assuming the angles are in radians
    *
    *    param:  CPoint & point - point to set as rotation
    ************************************************************************/
    void SetRotation( const CRadian & radian )
    {
        /*double a = radian.x / 2.0;
        double h = radian.y / 2.0;
        double b = radian.z / 2.0;

        // attitude
        double c2 = cos(a);
        double s2 = sin(a);

        // heading
        double c1 = cos(h);
        double s1 = sin(h);

        // bank
        double c3 = cos(b);
        double s3 = sin(b);

        double c1c2 = c1 * c2;
        double s1s2 = s1 * s2;

        w = (c1c2 * c3) - (s1s2 * s3);
        x = (s1 * c2 * c3) + (c1 * s2 * s3);
        y = (c1c2 * s3) + (s1s2 * c3);
        z = (c1 * s2 * c3) - (s1 * c2 * s3);*/

        double p = radian.x / 2.0;
        double y = radian.y / 2.0;
        double r = radian.z / 2.0;
 
        double sinp = sin(p);
        double siny = sin(y);
        double sinr = sin(r);
        double cosp = cos(p);
        double cosy = cos(y);
        double cosr = cos(r);
 
        x = sinr * cosp * cosy - cosr * sinp * siny;
        y = cosr * sinp * cosy + sinr * cosp * siny;
        z = cosr * cosp * siny - sinr * sinp * cosy;
        w = cosr * cosp * cosy + sinr * sinp * siny;

        Normalize();

    }	// SetRotation

    /************************************************************************                                                             
    *    desc:  Set a rotation point - Assuming the angles are in degrees
    *
    *    param:  CPoint & point - point to set as rotation
    ************************************************************************/
    void SetRotation( const CPoint & point )
    {
        // convert to a radian and pass it on
        SetRotation( CRadian( point.x * DEG_TO_RAD,
                              point.y * DEG_TO_RAD,
                              point.z * DEG_TO_RAD) );
    }	// SetRotation

    /************************************************************************                                                             
    *    desc:  Get the Euler angles from the quad
    ************************************************************************/
    CRadian GetRotation() const
    {
        CRadian tmp;

        double test = x * y + z * w;

        // singularity at north pole
        if( test > 0.499 )
        {
            // attitude
            tmp.x = M_PI_2;

            // heading
            tmp.y = 2 * atan2(x, w);

            // bank
            tmp.z = 0;
        }
        // singularity at south pole
        else if( test < -0.499 )
        {
            // attitude
            tmp.x = -M_PI_2;

            // heading
            tmp.y = -2 * atan2(x, w);

            // bank
            tmp.z = 0;
        }
        else
        {
            double sqx = x * x;
            double sqy = y * y;
            double sqz = z * z;

            // attitude
            tmp.x = asin(2*test);

            // heading
            tmp.y = atan2( 2*y*w-2*x*z, 1 - 2*sqy - 2*sqz );

            // bank
            tmp.z = atan2( 2*x*w - 2*y*z, 1 - 2*sqx - 2*sqz );
        }

        return tmp;

    }   // GetRotation

    /************************************************************************                                                             
    *    desc:  The equality operator 
    *
    *    param:  CQuaternion & quat - point to check
    *
    *    return: bool - true or false
    ************************************************************************/
    bool operator == ( const CQuaternion & quat ) const
    {
        if( (quat.x == x) && (quat.y == y) && (quat.z == z) && (quat.w == w) )
            return true;

        return false;

    }   // operator ==

    /************************************************************************                                                             
    *    desc:  The inequality operator 
    *
    *    param:  CQuaternion & quat - quat to check
    *
    *    return: bool - true or false
    ************************************************************************/
    bool operator != ( const CQuaternion & quat ) const
    {
        if( ( quat.x != x ) || ( quat.y != y ) || ( quat.z != z ) || ( quat.w != w ) )
            return true;

        return false;

    }   // operator !=

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  CQuaternion & quat * quat to multiply
    *
    *    return: CQuaternion - multiplied quat
    ************************************************************************/
    CQuaternion operator * ( const CQuaternion & quat ) const
    {
        CQuaternion tmp;

        tmp.x = (w * quat.x) + (x * quat.w) + (y * quat.z) - (z * quat.y);
        tmp.y = (w * quat.y) + (y * quat.w) + (z * quat.x) - (x * quat.z);
        tmp.z = (w * quat.z) + (x * quat.y) - (y * quat.x) + (z * quat.w);
        tmp.w = (w * quat.w) - (x * quat.x) - (y * quat.y) - (z * quat.z);

        return tmp;

    }   // operator *

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  CQuaternion & quat * quat to multiply
    *
    *    return: CQuaternion - multiplied quat
    ************************************************************************/
    CQuaternion operator *= ( const CQuaternion & quat )
    {
        x = (w * quat.x) + (x * quat.w) + (y * quat.z) - (z * quat.y);
        y = (w * quat.y) + (y * quat.w) + (z * quat.x) - (x * quat.z);
        z = (w * quat.z) + (x * quat.y) - (y * quat.x) + (z * quat.w);
        w = (w * quat.w) - (x * quat.x) - (y * quat.y) - (z * quat.z);

        return *this;

    }   // operator *

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  CQuaternion & quat * quat to add
    *
    *    return: CQuaternion - added quat
    ************************************************************************/
    CQuaternion operator + ( const CQuaternion & quat ) const
    {
        CQuaternion tmp;
        tmp.x = x + quat.x;
        tmp.y = y + quat.y;
        tmp.z = z + quat.z;
        tmp.w = w + quat.w;

        return tmp;

    }   // operator +

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  CQuaternion & quat * quat to add
    *
    *    return: CQuaternion - added point
    ************************************************************************/
    CQuaternion operator += ( const CQuaternion & quat )
    {
        x += quat.x;
        y += quat.y;
        z += quat.z;
        w += quat.w;

        return *this;

    }   // operator +=

    /************************************************************************                                                             
    *    desc:  The subtraction operator 
    *
    *    param:  CQuaternion & quat * quat to subtract
    *
    *    return: CQuaternion - subtracted quat
    ************************************************************************/
    CQuaternion operator - ( const CQuaternion & quat ) const
    {
        CQuaternion tmp;
        tmp.x = x - quat.x;
        tmp.y = y - quat.y;
        tmp.z = z - quat.z;
        tmp.w = w - quat.w;

        return tmp;

    }   // operator -

    /************************************************************************                                                             
    *    desc:  The subtraction operator 
    *
    *    param:  CQuaternion & quat - point to add
    *
    *    return: CQuaternion - subtracted
    ************************************************************************/
    CQuaternion operator -= ( const CQuaternion & quat )
    {
        x -= quat.x;
        y -= quat.y;
        z -= quat.z;
        w -= quat.w;

        return *this;

    }   // operator -=

    /************************************************************************                                                             
    *    desc:  Set the quaternion data
    ************************************************************************/
    void Set( const CPoint & point )
    {
        x = point.x;
        y = point.y;
        z = point.z;

        Normalize();

    }   // Set

    /************************************************************************                                                             
    *    desc:  Set the quaternion data
    ************************************************************************/
    void Set( double _x, double _y, double _z, double _w )
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;

        Normalize();

    }   // Set

    /************************************************************************                                                             
    *    desc:  Invert the values of this quaternion
    ************************************************************************/
    CQuaternion GetConjugate() const
    {
        CQuaternion tmp;
        tmp.x = -x;
        tmp.y = -y;
        tmp.z = -z;
        tmp.w = w;

        return tmp;

    }   // GetConjugate

    /************************************************************************                                                             
    *    desc:  Invert the values of this quaternion
    ************************************************************************/
    CQuaternion GetInverse() const
    {
        CQuaternion tmp;

        double norm2_inv = 1.0 / GetLengthSquared();

        tmp.x = -x * norm2_inv;
        tmp.y = -y * norm2_inv;
        tmp.z = -z * norm2_inv;
        tmp.w =  w * norm2_inv;

        return tmp;

    }   // GetInverse

    /************************************************************************                                                             
    *    desc:  Get the length of the points 
    *
    *    return: float - length of point
    ************************************************************************/
    double GetLengthSquared() const
    {
        return (x * x) +  (y * y) + (z * z) + (w * w);

    }   // GetLengthSquared

    /************************************************************************                                                             
    *    desc:  Get the point 
    *
    *    return: CPoint - get the point
    ************************************************************************/
    CPoint GetPoint() const
    {
        CPoint tmp;

        tmp.x = x * length;
        tmp.y = y * length;
        tmp.z = z * length;

        return tmp;

    }   // GetPoint

    /************************************************************************                                                             
    *    desc:  normalize this point
    ************************************************************************/
    void Normalize()
    {
        length = sqrt( GetLengthSquared() );

        if( length != 0.0f )
        {
            x /= length;
            y /= length;
            z /= length;
            w /= length;
        }

    }	// Normalize
};

#endif  // __quaternion_h__

