
/************************************************************************
*    FILE NAME:       point.h
*
*    DESCRIPTION:     Point class
************************************************************************/  

#ifndef __point_h__
#define __point_h__

#include <complex>

#pragma warning(disable : 4244)

template <class type>
class CPoint
{
public:

    // point values. 
    type x, y, z;

    /************************************************************************                                                             
    *    desc:  Constructor
    ************************************************************************/
    CPoint() : x(0), y(0), z(0)
    {
    }	// Constructor

    // Needs to be done like this to avoid recursion
    template <typename U>
    CPoint( const CPoint<U> & obj ) : x(obj.x), y(obj.y), z(obj.z)
    {
    }

    CPoint( type _x, type _y, type _z ) : x(_x), y(_y), z(_z)
    {
    }

    CPoint( type _x, type _y ) : x(_x), y(_y), z(0)
    {
    }	// Constructor


    /************************************************************************                                                             
    *    desc:  Clear the x
    ************************************************************************/
    void ClearX()
    {
        x = 0.0f;
    }

    /************************************************************************                                                             
    *    desc:  Clear the y
    ************************************************************************/
    void ClearY()
    {
        y = 0.0f;
    }

    /************************************************************************                                                             
    *    desc:  Clear the z
    ************************************************************************/
    void ClearZ()
    {
        z = 0.0f;
    }

    /************************************************************************                                                             
    *    desc:  Clear the values
    ************************************************************************/
    void Clear()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    /************************************************************************                                                             
    *    desc:  The equality operator 
    *
    *    param:  CPoint & point - point to equate
    *
    *    return: bool - true or false
    ************************************************************************/
    bool operator == ( const CPoint & point ) const
    {
        if( (point.x == x) && (point.y == y) && (point.z == z) )
            return true;

        return false;

    }   // operator ==

    /************************************************************************                                                             
    *    desc:  The inequality operator 
    *
    *    param:  CPoint & point - point to check
    *
    *    return: bool - true or false
    ************************************************************************/
    bool operator != ( const CPoint & point ) const
    {
        if( ( point.x != x ) || ( point.y != y ) || ( point.z != z ) )
            return true;

        return false;

    }   // operator !=

    /************************************************************************                                                             
    *    desc:  The subtraction operator 
    *
    *    param:  CPoint & point - point to subtract
    *
    *    return: CPoint - subtracted point
    ************************************************************************/
    CPoint operator - ( const CPoint & point ) const
    {
        CPoint tmp;
        tmp.x = x - point.x;
        tmp.y = y - point.y;
        tmp.z = z - point.z;

        return tmp;

    }   // operator -

    /************************************************************************                                                             
    *    desc:  The subtraction operator 
    *
    *    param:  type * value - value to subtract
    *
    *    return: CPoint - subtracted point
    ************************************************************************/
    CPoint operator - ( type value ) const
    {
        CPoint tmp;
        tmp.x = x - value;
        tmp.y = y - value;
        tmp.z = z - value;

        return tmp;

    }   // operator -

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  CPoint & point - point to add
    *
    *    return: CPoint - added point
    ************************************************************************/
    CPoint operator + ( const CPoint & point ) const
    {
        CPoint tmp;
        tmp.x = x + point.x;
        tmp.y = y + point.y;
        tmp.z = z + point.z;

        return tmp;

    }   // operator +

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  type value - value to add
    *
    *    return: CPoint - added point
    ************************************************************************/
    CPoint operator + ( type value ) const
    {
        CPoint tmp;
        tmp.x = x + value;
        tmp.y = y + value;
        tmp.z = z + value;

        return tmp;

    }   // operator +

    /************************************************************************                                                             
    *    desc:  The division operator 
    *
    *    param:  CPoint & point - point to divide by
    *
    *    return: CPoint - divided by point
    ************************************************************************/
    CPoint operator / ( const CPoint & point ) const
    {
        CPoint tmp;
        tmp.x = x / point.x;
        tmp.y = y / point.y;
        tmp.z = z / point.z;

        return tmp;

    }   // operator /

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  type value - value to divide by
    *
    *    return: CPoint - divided by point
    ************************************************************************/
    CPoint operator / ( type value ) const
    {
        CPoint tmp;
        tmp.x = x / value;
        tmp.y = y / value;
        tmp.z = z / value;

        return tmp;

    }   // operator /

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  CPoint & point - point to add
    *
    *    return: CPoint - added point
    ************************************************************************/
    CPoint operator += ( const CPoint & point )
    {
        x += point.x;
        y += point.y;
        z += point.z;

        return *this;

    }   // operator +=

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  type value - value to add
    *
    *    return: CPoint - added point
    ************************************************************************/
    CPoint operator += ( type value )
    {
        x += value;
        y += value;
        z += value;

        return *this;

    }   // operator +=

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  CPoint & point - point to add
    *
    *    return: CPoint - added point
    ************************************************************************/
    CPoint operator -= ( const CPoint & point )
    {
        x -= point.x;
        y -= point.y;
        z -= point.z;

        return *this;

    }   // operator -=

    /************************************************************************                                                             
    *    desc:  The addition operator 
    *
    *    param:  type value - value to add
    *
    *    return: CPoint - added point
    ************************************************************************/
    CPoint operator -= ( type value )
    {
        x -= value;
        y -= value;
        z -= value;

        return *this;

    }   // operator -=

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  CPoint & point * point to multiply
    *
    *    return: CPoint - multiplied point
    ************************************************************************/
    CPoint operator * ( const CPoint & point ) const
    {
        CPoint tmp;
        tmp.x = x * point.x;
        tmp.y = y * point.y;
        tmp.z = z * point.z;

        return tmp;

    }   // operator *

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  type value - value to multiply
    *
    *    return: CPoint - multiplied point
    ************************************************************************/
    CPoint operator * ( type value ) const
    {
        CPoint tmp;
        tmp.x = x * value;
        tmp.y = y * value;
        tmp.z = z * value;

        return tmp;

    }   // operator *

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  CMatrix & matrix - matrix to multiply
    *
    *    return: CPoint - multiplied point
    ************************************************************************/
    CPoint operator * ( type * pMat ) const
    {
        CPoint tmp;

        tmp.x = x * pMat[0] + y * pMat[4] + z * pMat[8];
        tmp.y = x * pMat[1] + y * pMat[5] + z * pMat[9];
        tmp.z = x * pMat[2] + y * pMat[6] + z * pMat[10];

        return tmp;

    }   // operator *=

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  CPoint & point - point to multiply
    *
    *    return: CPoint - multiplied point
    ************************************************************************/
    CPoint operator *= ( const CPoint & point )
    {
        x *= point.x;
        y *= point.y;
        z *= point.z;

        return *this;

    }   // operator *=

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  type value - value to multiply
    *
    *    return: CPoint - multiplied point
    ************************************************************************/
    CPoint operator *= ( type value )
    {
        x *= value;
        y *= value;
        z *= value;

        return *this;

    }   // operator *=

    /************************************************************************                                                             
    *    desc:  The multiplication operator 
    *
    *    param:  CMatrix & matrix - matrix to multiply
    *
    *    return: CPoint - multiplied point
    ************************************************************************/
    CPoint operator *= ( type * pMat )
    {
        CPoint tmp;

        tmp.x = x * pMat[0] + y * pMat[4] + z * pMat[8];
        tmp.y = x * pMat[1] + y * pMat[5] + z * pMat[9];
        tmp.z = x * pMat[2] + y * pMat[6] + z * pMat[10];

        x = tmp.x;
        y = tmp.y;
        z = tmp.z;

        return *this;

    }   // operator *=

    /************************************************************************                                                             
    *    desc:  The division operator 
    *
    *    param:  CPoint & point - point to divide
    *
    *    return: CPoint - divided point
    ************************************************************************/
    CPoint operator /= ( const CPoint & point )
    {
        x /= point.x;
        y /= point.y;
        z /= point.z;

        return *this;

    }   // operator *=

    /************************************************************************                                                             
    *    desc:  The division operator 
    *
    *    param:  type value - value to divide
    *
    *    return: CPoint - divided point
    ************************************************************************/
    CPoint operator /= ( type value )
    {
        x /= value;
        y /= value;
        z /= value;

        return *this;

    }   // operator *=

    /************************************************************************                                                             
    *    desc:  Set the point data
    ************************************************************************/
    void Set( type _x, type _y, type _z )
    {
        x = _x;
        y = _y;
        z = _z;

    }   // Set

    void Set( type value )
    {
        x = value;
        y = value;
        z = value;

    }   // Set

    /************************************************************************                                                             
    *    desc:  Cap the value
    ************************************************************************/
    void Cap( type value )
    {
        if( value > 0.0f )
        {
            if( x > value )
            {
                x -= value;
            }
            else if ( x < 0.0f )
            {
                x += value;
            }

            if( y > value )
            {
                y -= value;
            }
            else if ( y < 0.0f )
            {
                y += value;
            }

            if( z > value )
            {
                z -= value;
            }
            else if ( z < 0.0f )
            {
                z += value;
            }
        }
        else
        {
            if( x > value )
            {
                x += value;
            }
            else if ( x < 0.0f )
            {
                x -= value;
            }

            if( y > value )
            {
                y += value;
            }
            else if ( y < 0.0f )
            {
                y -= value;
            }

            if( z > value )
            {
                z += value;
            }
            else if ( z < 0.0f )
            {
                z -= value;
            }
        }

    }   // Cap

    /************************************************************************                                                             
    *    desc:  Does this point not have any data? 
    *
    *    return: bool
    ************************************************************************/
    bool IsEmpty() const
    {
        // This looks at the bits of the data as an int
        return ( IsXEmpty() && IsYEmpty() && IsZEmpty() );

    }   // IsEmpty

    bool IsXEmpty() const
    {
        // This looks at the bits of the data as an int
        return ( (0 == x) || (*(int*)&x == 0) );

    }   // IsEmpty

    bool IsYEmpty() const
    {
        // This looks at the bits of the data as an int
        return ( (0 == y) || (*(int*)&y == 0) );

    }   // IsEmpty

    bool IsZEmpty() const
    {
        // This looks at the bits of the data as an int
        return ( (0 == z) || (*(int*)&z == 0) );

    }   // IsEmpty

    /************************************************************************                                                             
    *    desc:  Invert the values of this point
    ************************************************************************/
    void Invert()
    {
        x = -x;
        y = -y;
        z = -z;

    }   // Invert

    /************************************************************************                                                             
    *    desc:  Invert a copy of this point and return it
    ************************************************************************/
    CPoint GetInvert()
    {
        CPoint tmp(*this);
        tmp.Invert();

        return tmp;

    }   // Invert

    /************************************************************************                                                             
    *    desc:  Invert the values of this point
    ************************************************************************/
    void InvertX()
    {
        x = -x;

    }   // Invert

    /************************************************************************                                                             
    *    desc:  Invert the values of this point
    ************************************************************************/
    void InvertY()
    {
        y = -y;

    }   // Invert

    /************************************************************************                                                             
    *    desc:  Invert the values of this point
    ************************************************************************/
    void InvertZ()
    {
        z = -z;

    }   // Invert

    /************************************************************************                                                             
    *    desc:  Get the length of the point from the origin 
    *
    *    ret:	float - length of point
    ************************************************************************/
    type GetLength() const
    {
        return std::sqrt( GetLengthSquared() );

    }   // GetLengthSquared


    /************************************************************************                                                             
    *    desc:  Get the length between two points 
    *
    *    param: CPoint & point - point
    *
    *    ret:	float - distance between two points
    ************************************************************************/
    type GetLength( const CPoint & point ) const
    {
        return std::sqrt( GetLengthSquared( point ) );

    }   // GetLengthSquared


    /************************************************************************                                                             
    *    desc:  Get the length of the point from the origin along the x and y
    *			axis
    *
    *    ret:	float - length of point
    ************************************************************************/
    type GetLength2D() const
    {
        return std::sqrt( GetLengthSquared2D() );

    }   // GetLength2D

    /************************************************************************                                                             
    *    desc:  Get the squared length between two points 
    *
    *    param: CPoint & point - point
    *
    *    ret:	float - squared distance between two points
    ************************************************************************/
    type GetLengthSquared( const CPoint & point ) const
    {
        return (*this - point).GetLengthSquared();

    }   // GetLengthSquared

    /************************************************************************                                                             
    *    desc:  Get the squared length of the point from the origin 
    *
    *    return: type - length of point
    ************************************************************************/
    type GetLengthSquared() const
    {
        return( x * x ) +  ( y * y ) + ( z * z );

    }   // GetLength

    /************************************************************************                                                             
    *    desc:  Get the squared length of the point from the origin 
    *
    *    return: type - length of point
    ************************************************************************/
    type GetLengthSquared2D() const
    {
        return( x * x ) +  ( y * y );

    }   // GetLength

    /************************************************************************                                                             
    *    desc:  Get the dot product 
    *
    *    param:  CPoint & point
    *
    *    return: type - distance between two points
    ************************************************************************/
    type GetDotProduct( const CPoint & point ) const
    {
            return ( x * point.x ) + ( y * point.y ) + ( z * point.z );

    }   // GetDotProduct

    /************************************************************************                                                             
    *    desc:  normalize this point
    ************************************************************************/
    void Normalize()
    {
        type length = GetLength();

        if( length != 0.0f )
        {
            x /= length;
            y /= length;
            z /= length;
        }

    }	// Normalize

    void Normalize2D()
    {
        type length = GetLength2D();

        if( length != 0.0f )
        {
            x /= length;
            y /= length;
        }

    }	// Normalize

    /************************************************************************                                                             
    *    desc:  Get the cross product
    ************************************************************************/
    CPoint GetCrossProduct( const CPoint & point ) const
    {
        CPoint tmp;
        tmp.x = (y * point.z) - (point.y * z);
        tmp.y = (z * point.x) - (point.z * x);
        tmp.z = (x * point.y) - (point.x * y);
        tmp.Normalize();

        return tmp;

    }	// GetCrossProduct

    /************************************************************************                                                             
    *    desc:  Create a displacement vector
    *
    *    param:  CPoint & point
    *            type amount - amount of displacement
    *
    *    return: CPoint - the dislacement vector
    ************************************************************************/
    CPoint GetDisplacement( const CPoint & point, type amount ) const
    {
        CPoint displacement( *this - point );
        displacement.Normalize();
        CPoint tmp = *this + (displacement * amount);

        return tmp;

    }	// GetDisplacement


    /***********************************************************************************
    *	NEGATIVE OPERATOR
    ***********************************************************************************/
    /************************************************************************
    *    desc:  Return a copy of the point with its signs flipped
    *
    *	 ret:	CPoint - point with signs flipped
    ************************************************************************/
    CPoint operator - () const
    {
        return CPoint(-x,-y,-z);

    }	// operator -

};

#endif  // __point_h__

