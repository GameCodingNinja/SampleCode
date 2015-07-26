
/************************************************************************
*    FILE NAME:       matrix.cpp
*
*    DESCRIPTION:     4x4 Matrix math class
************************************************************************/

// Physical component dependency
#include <common/matrix.h>

// Standard lib dependencies
#include <math.h>
#include <memory.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <utilities/exceptionhandling.h>
#include <utilities/genfunc.h>
#include <common/defs.h>

// Turn off the data type conversion warning (ie. int to float, float to int etc.)
// We do this all the time in 3D. Don't need to be bugged by it all the time.
#pragma warning(disable : 4244)

enum
{
    NO_ROT = 0,
    ROT_Z = 1,
    ROT_Y = 2,
    ROT_X = 4,
    ROT_ALL = ROT_Z | ROT_Y | ROT_X
};


/************************************************************************
*    desc:  Constructor                                                             
************************************************************************/
CMatrix::CMatrix()
{
    InitilizeMatrix();

}   // Constructor


/************************************************************************
*    desc:  Copy Constructor
*
*    param: const CMatrix & obj - Matrix object
************************************************************************/
CMatrix::CMatrix( const CMatrix & obj )
{
    *this = obj;

}   // Constructor


/************************************************************************
*    desc:  Copy Constructor
*
*    param: float mat[16] - Matrix object
************************************************************************/
CMatrix::CMatrix( float mat[mMax] )
{
    memcpy( matrix, mat, sizeof(matrix) );

}   // Constructor

/************************************************************************                                                             
*    desc:  Reset the matrix to the identity matrix
************************************************************************/
void CMatrix::InitilizeMatrix()
{
    InitIdentityMatrix( matrix );

}   // InitilizeMatrix


/************************************************************************
*    desc:  Initializes a specific matrix to the identity matrix
*
*    param: float mat[16] - Matrix array
************************************************************************/
void CMatrix::InitIdentityMatrix( float mat[16] )
{
    // Initializes a specific matrix to the identity matrix:
    mat[0]  = 1.0f;   mat[1] = 0.0f;   mat[2]  = 0.0f;   mat[3] = 0.0f;
    mat[4]  = 0.0f;   mat[5] = 1.0f;   mat[6]  = 0.0f;   mat[7] = 0.0f;
    mat[8]  = 0.0f;   mat[9] = 0.0f;   mat[10] = 1.0f;  mat[11] = 0.0f;
    mat[12] = 0.0f;  mat[13] = 0.0f;   mat[14] = 0.0f;  mat[15] = 1.0f;

}   // InitIdentityMatrix


/************************************************************************
*    desc:  Clear translation data from the matrix
************************************************************************/
void CMatrix::ClearTranlate()
{
    // Initializes a specific matrix to the identity matrix:
    matrix[12] = 0.0f;  matrix[13] = 0.0f;   matrix[14] = 0.0f;

}   // ClearTranlate


/************************************************************************
*    desc:  Merge matrix into master matrix
*
*    parm:  float newMatrix[16] - Matrix array
************************************************************************/  
void CMatrix::MergeMatrix( const float mat[mMax] )
{
    float temp[mMax];

    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        { 
            temp[(i*4)+j] = (matrix[i*4]	 * mat[j])
                          + (matrix[(i*4)+1] * mat[4+j])
                          + (matrix[(i*4)+2] * mat[8+j])
                          + (matrix[(i*4)+3] * mat[12+j]);
        }
    }

    // Copy temp to master Matrix
    memcpy( matrix, temp, sizeof(temp) );

}  // MergeMatrix


/************************************************************************
*    desc:  Merge matrix into master matrix
*
*    parm:  float newMatrix[16] - Matrix array
************************************************************************/  
void CMatrix::ReverseMergeMatrix( const float mat[mMax] )
{
    float temp[mMax];

    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        { 
            temp[(i*4)+j] = (mat[i*4]     * matrix[j])
                          + (mat[(i*4)+1] * matrix[4+j])
                          + (mat[(i*4)+2] * matrix[8+j])
                          + (mat[(i*4)+3] * matrix[12+j]);
        }
    }

    // Copy temp to master Matrix
    memcpy( matrix, temp, sizeof(temp) );

}  // MergeMatrix


/************************************************************************
*    desc:  Merge source matrix into destination matrix.
*
*    param: float Dest[16] - Destination Matric
*           float Source[16] - Source Matrix
************************************************************************/
void CMatrix::MergeMatrices( float dest[mMax], const float source[mMax] )
{
    float temp[mMax];

    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            temp[ (i*4)+j ] = ( source[ i*4 ]	  * dest[ j ] )
                            + ( source[ (i*4)+1 ] * dest[ 4+j ] )
                            + ( source[ (i*4)+2 ] * dest[ 8+j ] )
                            + ( source[ (i*4)+3 ] * dest[ 12+j ] );
        }
    }

    // Copy Temp to Dest
    memcpy( dest, temp, sizeof(temp) );

}   // MergeMatrices


/************************************************************************
*    desc:  Generate 3D rotation matrix.
*
*    param: const CPoint & point - rotation point
************************************************************************/
void CMatrix::Rotate( const CPoint & point )
{
    Rotate( CRadian(point) );

}   // Rotate


/************************************************************************
*    desc:  Generate 3D rotation matrix.
*
*    param: const CRadian & point - rotation point
*
*	 NOTE:	There is some mysterious problem with my new rotate function
*			that can only be seen in the game template at the moment.
*			In the template, the robots' hands are getting flattened and
*			I can't figure out why. I've tried everything to reproduce this
*			rotationally screwed up matrix but couldn't
************************************************************************/
void CMatrix::Rotate( const CRadian & radian )
{
    int flags = NO_ROT;
    float rMatrix[ 16 ];

    // init the rotation matrix
    InitIdentityMatrix( rMatrix );

    // Apply Z rotation
    if( radian.z != 0.0f )
    {
        RotateZRad( rMatrix, radian.z, flags );
        flags |= ROT_Z;
    }
    
    // Apply Y rotation
    if( radian.y != 0.0f )
    {
        RotateYRad( rMatrix, radian.y, flags );
        flags |= ROT_Y;
    }

    // Apply X rotation
    if( radian.x != 0.0f )
    {
        RotateXRad( rMatrix, radian.x, flags );
        flags |= ROT_X;
    }

    // Merg the rotation into the master matrix
    MergeMatrix( rMatrix );

}   // Rotate


/************************************************************************
*    desc:  Get the Z rotation of the matrix
*
*	 NOTE:	If the matrix is scaled or there are more rotations besides
*			z, then the result might not be correct
*
*    param: bool inDegrees = true - whether to return the rotation in degrees
*									or radians
*
*	 ret:	float - rotation value
************************************************************************/
float CMatrix::GetZRot( bool inDegrees ) const
{
    if( inDegrees )
        return -atan2( matrix[m10], matrix[m00] ) * RAD_TO_DEG;

    return -atan2( matrix[m10], matrix[m00] );

}	// GetZRot


/************************************************************************
*    desc:  Create 3D translation matrix
*
*    param: const CPoint & point - translation point
************************************************************************/
void CMatrix::Translate( const CPoint & point )
{
    matrix[12] += point.x;
    matrix[13] += point.y;
    matrix[14] += point.z;

}   // Translate


/************************************************************************
*    desc:  Function designed to transform a vertex using
*           the master matrix
*
*    param: CPoint & dest - destination point
*           CPoint & source - source point
************************************************************************/
void CMatrix::Transform( CPoint & dest, const CPoint & source ) const
{
    // Transform vertex by master matrix:
    dest.x = ( source.x * matrix[ 0 ] )
           + ( source.y * matrix[ 4 ] )
           + ( source.z * matrix[ 8 ] )
           + matrix[ 12 ];

    dest.y = ( source.x * matrix[ 1 ] )
           + ( source.y * matrix[ 5 ] )
           + ( source.z * matrix[ 9 ] )
           + matrix[ 13 ];

    dest.z = ( source.x * matrix[ 2 ] )
           + ( source.y * matrix[ 6 ] )
           + ( source.z * matrix[ 10 ] )
           + matrix[ 14 ];

}   // Transform


/************************************************************************
*    desc:  Function designed to transform a vertex using
*           the master matrix
*
*    param: CPoint & dest - destination point
*           CPoint & source - source point
************************************************************************/
void CMatrix::Transform( CPoint * pDest, const CPoint * pSource ) const
{
    // Transform vertex by master matrix:
    pDest->x = ( pSource->x * matrix[ 0 ] )
           + ( pSource->y * matrix[ 4 ] )
           + ( pSource->z * matrix[ 8 ] )
           + matrix[ 12 ];

    pDest->y = ( pSource->x * matrix[ 1 ] )
           + ( pSource->y * matrix[ 5 ] )
           + ( pSource->z * matrix[ 9 ] )
           + matrix[ 13 ];

    pDest->z = ( pSource->x * matrix[ 2 ] )
           + ( pSource->y * matrix[ 6 ] )
           + ( pSource->z * matrix[ 10 ] )
           + matrix[ 14 ];

}   // Transform


/************************************************************************
*    desc:  Function designed to transform a vertex using
*           the master matrix
*
*    param: CPoint & dest - destination point
*           CPoint & source - source point
************************************************************************/
void CMatrix::Transform( CRect<float> & dest, const CRect<float> & source ) const
{
    // Transform vertex by master matrix:
    dest.x1 = ( source.x1 * matrix[ 0 ] )
            + ( source.y1 * matrix[ 4 ] )
            + matrix[ 12 ];

    dest.y1 = ( source.x1 * matrix[ 1 ] )
            + ( source.y1 * matrix[ 5 ] )
            + matrix[ 13 ];

    dest.x2 = ( source.x2 * matrix[ 0 ] )
            + ( source.y2 * matrix[ 4 ] )
            + matrix[ 12 ];

    dest.y2 = ( source.x2 * matrix[ 1 ] )
            + ( source.y2 * matrix[ 5 ] )
            + matrix[ 13 ];

}   // Transformm


/************************************************************************
*    desc:  Function designed to transform a normal. Normals don't have
*           a position, only direction so we only use the rotation
*           portion of the matrix.
*
*    param: CTransNormal & dest - destination point
*           CNormal & source - source point
************************************************************************/
void CMatrix::Transform( CNormal & dest, const CNormal & source ) const
{
    // Transform vertex by master matrix:
    dest.x = ( source.x * matrix[ 0 ])
           + ( source.y * matrix[ 4 ])
           + ( source.z * matrix[ 8 ]);

    dest.y = ( source.x * matrix[ 1 ])
           + ( source.y * matrix[ 5 ])
           + ( source.z * matrix[ 9 ]);

    dest.z = ( source.x * matrix[ 2 ])
           + ( source.y * matrix[ 6 ])
           + ( source.z * matrix[ 10 ]);

}   // Transform


/************************************************************************
*    desc:  Function designed to transform a normal. Normals don't have
*           a position, only direction so we only use the rotation
*           portion of the matrix.
*
*    param: CPoint & dest   - destination point
*           CPoint & source - source point
************************************************************************/
void CMatrix::Transform3x3( CPoint & dest, const CPoint & source ) const
{
    // Transform vertex by master matrix:
    dest.x = ( source.x * matrix[ 0 ])
           + ( source.y * matrix[ 4 ])
           + ( source.z * matrix[ 8 ]);

    dest.y = ( source.x * matrix[ 1 ])
           + ( source.y * matrix[ 5 ])
           + ( source.z * matrix[ 9 ]);

    dest.z = ( source.x * matrix[ 2 ])
           + ( source.y * matrix[ 6 ])
           + ( source.z * matrix[ 10 ]);

}   // Transform


/************************************************************************
*    desc:  Get the transpose of a matrix
************************************************************************/
CMatrix CMatrix::GetTransposeMatrix() const
{
    CMatrix tmp;

    tmp[m00] = matrix[m00];

    tmp[m01] = matrix[m10];
    tmp[m10] = matrix[m01];

    tmp[m11] = matrix[m11];

    tmp[m02] = matrix[m20];
    tmp[m20] = matrix[m02];

    tmp[m12] = matrix[m21];
    tmp[m21] = matrix[m12];

    tmp[m03] = matrix[m30];
    tmp[m30] = matrix[m03];

    tmp[m22] = matrix[m22];

    tmp[m31] = matrix[m13];
    tmp[m13] = matrix[m31];

    tmp[m32] = matrix[m23];
    tmp[m23] = matrix[m32];

    tmp[m33] = matrix[m33];

    return tmp;

}	// GetTransposeMatrix


/************************************************************************
*    desc:  Function designed to transform a vertex using
*           the master matrix
*
*    param: CQuad & dest - destination point
*           CQuad & source - source point
************************************************************************/
void CMatrix::Transform( CQuad & dest, const CQuad & source ) const
{
    // Transform vertex by master matrix:
    for( int i = 0; i < 4; ++i )
        Transform( dest.point[i], source.point[i] );

}   // Transform


/************************************************************************
*    desc:   Get matrix point in space
*
*    ret:    CPoint - point to be returned
************************************************************************/
CPoint CMatrix::GetMatrixPoint()
{
    CPoint source;
    CPoint dest;
    float tempMat[16];

    // Copy over the matrix to restore after operation
    memcpy( tempMat, matrix, sizeof(tempMat) );

    // Get the translation part of the matrix and invert the sign
    source.x = -matrix[m30];
    source.y = matrix[m31];
    source.z = -matrix[m32];

    // Inverse the matrix
    Inverse();

    // Transform only by the 3x3 part of the matrix 
    Transform3x3( dest, source );

    // Restore the matrix back to what it was
    memcpy( matrix, tempMat, sizeof(matrix) );

    return dest;

}   // GetMatrixPoint


/************************************************************************
*    desc:   Get matrix rotation
*
*    ret:    CRadian - radian to be returned
************************************************************************/
CRadian CMatrix::GetMatrixRotation()
{
    CRadian tmp;

    // singularity at north pole
    if( matrix[m10] > 0.998f )
    { 
        tmp.x = M_PI_2;
        tmp.y = atan2( matrix[m02], matrix[m22] );
        tmp.z = 0;
        return tmp;
    }

    // singularity at south pole
    if( matrix[m10] < -0.998f )
    { 
        tmp.x = -M_PI_2;
        tmp.y = atan2( matrix[m02], matrix[m22] );
        tmp.z = 0;
        return tmp;
    }

    tmp.x = asin( matrix[m10] );
    tmp.y = atan2( -matrix[m20], matrix[m00] );
    tmp.z = atan2( -matrix[m12], matrix[m11] );
    
    return tmp;

}   // GetObjPoint


/************************************************************************
*    desc: Function designed to merge scaling matrix with master matrix.
*
*    NOTE: To scale down, value needs to be inbetween 0.0 to 1.0.
*          Scale up is any value greater then 1.0
*
*    param:  float scale - scale value
************************************************************************/   
void CMatrix::Scale( const CPoint & point )
{
    // Initialize scaling matrix:
    matrix[0]  *= point.x;
    matrix[5]  *= point.y;
    matrix[10] *= point.z;

}   // Scale


/************************************************************************
*    desc: Function designed to merge scaling matrix with master matrix.
*
*    NOTE: You can scale on all 3 axises but I can't think of reason
*          you would want to do that. Plus allowing scaling on all 3
*          axises would introduce unneeded complexity into the bounding
*          sphere checks for collision and such.
*
*    NOTE: To scale down, value needs to be inbetween 0.0 to 1.0.
*          Scale up is any value greater then 1.0
*
*    param:  float scale - scale value
************************************************************************/   
void CMatrix::Scale( float scale )
{
    // Initialize scaling matrix:
    matrix[0]  *= scale;
    matrix[5]  *= scale;
    matrix[10] *= scale;

}   // Scale


/************************************************************************
*    desc:  Inverses this matrix. Assumes that the last column is [0 0 0 1]
*
*    ret: bool - true on success
************************************************************************/
bool CMatrix::Inverse()
{
    const float EPSILON_E5((float)(1E-5));

    float det =  ( matrix[m00] * ( matrix[m11] * matrix[m22] - matrix[m12] * matrix[m21] ) -
                   matrix[m01] * ( matrix[m10] * matrix[m22] - matrix[m12] * matrix[m20] ) +
                   matrix[m02] * ( matrix[m10] * matrix[m21] - matrix[m11] * matrix[m20] ) );

    // test determinate to see if it's 0
    if( fabs(det) < EPSILON_E5 )
    {
        return false;
    }

    float det_inv  = 1.0f / det;

    float tmp[ 16 ];

    // Initialize translation matrix
    InitIdentityMatrix( tmp );

    tmp[m00] =  det_inv * ( matrix[m11] * matrix[m22] - matrix[m12] * matrix[m21] );
    tmp[m01] = -det_inv * ( matrix[m01] * matrix[m22] - matrix[m02] * matrix[m21] );
    tmp[m02] =  det_inv * ( matrix[m01] * matrix[m12] - matrix[m02] * matrix[m11] );
    tmp[m03] = 0.0f; // always 0

    tmp[m10] = -det_inv * ( matrix[m10] * matrix[m22] - matrix[m12] * matrix[m20] );
    tmp[m11] =  det_inv * ( matrix[m00] * matrix[m22] - matrix[m02] * matrix[m20] );
    tmp[m12] = -det_inv * ( matrix[m00] * matrix[m12] - matrix[m02] * matrix[m10] );
    tmp[m13] = 0.0f; // always 0

    tmp[m20] =  det_inv * ( matrix[m10] * matrix[m21] - matrix[m11] * matrix[m20] );
    tmp[m21] = -det_inv * ( matrix[m00] * matrix[m21] - matrix[m01] * matrix[m20] );
    tmp[m22] =  det_inv * ( matrix[m00] * matrix[m11] - matrix[m01] * matrix[m10] );
    tmp[m23] = 0.0f; // always 0

    tmp[m30] = -( matrix[m30] * matrix[m00] + matrix[m31] * matrix[m10] + matrix[m32] * matrix[m20] );
    tmp[m31] = -( matrix[m30] * matrix[m01] + matrix[m31] * matrix[m11] + matrix[m32] * matrix[m21] );
    tmp[m32] = -( matrix[m30] * matrix[m02] + matrix[m31] * matrix[m12] + matrix[m32] * matrix[m22] );
    tmp[m33] = 1.0f; // always 0

    // Copy Temp to Dest
    memcpy( matrix, tmp, sizeof(tmp) );

    return true;

}	// Inverse


/************************************************************************
*    desc:  Inverse the Z. 
************************************************************************/
void CMatrix::InverseZ()
{
    matrix[14] = -matrix[14];

}	// InverseZ


/************************************************************************
*    desc:  Set the quaternion
*
*    param: CQuaternion & quat - set the quat
************************************************************************/
void CMatrix::Set( const CQuaternion & quat )
{
    float temp[mMax];

    // Initialize translation matrix
    InitIdentityMatrix( temp );

    float x2 = quat.x * quat.x;
    float y2 = quat.y * quat.y;
    float z2 = quat.z * quat.z;
    float xy = quat.x * quat.y;
    float xz = quat.x * quat.z;
    float yz = quat.y * quat.z;
    float wx = quat.w * quat.x;
    float wy = quat.w * quat.y;
    float wz = quat.w * quat.z;

    temp[m00] = 1.0f - 2.0f * (y2 + z2);
    temp[m01] = 2.0f * (xy - wz);
    temp[m02] = 2.0f * (xz + wy);

    temp[m10] = 2.0f * (xy + wz);
    temp[m11] = 1.0f - 2.0f * (x2 + z2);
    temp[m12] = 2.0f * (yz - wx), 0.0f;

    temp[m20] = 2.0f * (xz - wy);
    temp[m21] = 2.0f * (yz + wx);
    temp[m22] = 1.0f - 2.0f * (x2 + y2);

    // Merge matrix with master matrix:
    MergeMatrix( temp );

}	// Set


/************************************************************************
*    desc:  Create the matrix based on where the camera is looking
*
*    param: pos - pos if camera/light
*    param: target - spot the camera/light is pointing to
*    param: cameraUp - The camera up of the camera
************************************************************************/
void CMatrix::LookAt( const CPoint & pos, const CPoint & target, const CPoint & cameraUp )
{
    CPoint zAxis = target - pos;
    zAxis.Normalize();

    // We are assuming Y is always pointing up
    CPoint yUp(CPoint(0, 1, 0));
    CPoint xAxis = cameraUp.GetCrossProduct( zAxis );
    xAxis.Normalize();

    CPoint yAxis = zAxis.GetCrossProduct( xAxis );

    matrix[m00] = xAxis.x;		matrix[m01] = yAxis.x;		matrix[m02] = zAxis.x;		matrix[m03] = 0.f;
    matrix[m10] = xAxis.y;		matrix[m11] = yAxis.y;		matrix[m12] = zAxis.y;		matrix[m13] = 0.f;
    matrix[m20] = xAxis.z;		matrix[m21] = yAxis.z;		matrix[m22] = zAxis.z;		matrix[m23] = 0.f;

    matrix[m30] = -pos.GetDotProduct(xAxis);
    matrix[m31] = -pos.GetDotProduct(yAxis);
    matrix[m32] = -pos.GetDotProduct(zAxis);
    matrix[m33] = 1.f;

}	// LookAt


/************************************************************************                                                             
*    desc:	Multiply the matrices only using the rotation/scale portion 
*
*    param:	const CMatrix & obj - matrix to multiply
************************************************************************/
void CMatrix::Multiply3x3( const CMatrix & obj )
{
    float tmp[mMax] = { 1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1 };

    for( int i = 0; i < 3; ++i )
    {
        for( int j = 0; j < 3; ++j )
        { 
            tmp[(i*4)+j] =   ( matrix[i*4]	   * obj[j]   )
                           + ( matrix[(i*4)+1] * obj[4+j] )
                           + ( matrix[(i*4)+2] * obj[8+j] );
        }
    }

    // Copy the tmp matrix to the class's matrix
    memcpy( matrix, tmp, sizeof(matrix) );

}	// Multiply3x3


/************************************************************************
*    desc:  Rotate the matrix along the z axis
*
*    param: float dest[mMax] - destination matrix
*			float value      - amount to rotate by
*			int rotFlags     - bitmask representing the applied rotations
************************************************************************/  
void CMatrix::RotateZRad( float dest[mMax], float value, int rotFlags )
{
    float cosZ = cos(value);
    float sinZ = sin(value);

    dest[0] = cosZ;
    dest[1] = sinZ;
    dest[4] = -sinZ;
    dest[5] = cosZ;

}	// RotateZRad


/************************************************************************
*    desc:  Rotate the matrix along the y axis
*
*    param: float dest[mMax] - destination matrix
*			float value      - amount to rotate by
*			int rotFlags     - bitmask representing the applied rotations
************************************************************************/  
void CMatrix::RotateYRad( float dest[mMax], float value, int rotFlags )
{
    float cosY = cos(value);
    float sinY = sin(value);

    switch( rotFlags )
    {
        case ROT_Z:
        {
            float tmp0, tmp1, tmp8, tmp9;
            tmp0 = dest[0] * cosY;
            tmp1 = dest[1] * cosY;
            tmp8 = dest[0] * sinY;
            tmp9 = dest[1] * sinY;
            dest[0] = tmp0;
            dest[1] = tmp1;
            dest[2] = -sinY;
            dest[8] = tmp8;
            dest[9] = tmp9;
            dest[10] = cosY;
            break;
        }
        case NO_ROT:
        {
            dest[0]  =  cosY;
            dest[2]  = -sinY;
            dest[8]  =  sinY;
            dest[10] =  cosY;
            break;
        }
    }

}	// RotateYRad


/************************************************************************
*    desc:  Rotate the matrix along the x axis
*
*    param: float dest[mMax] - destination matrix
*			float value      - amount to rotate by
*			int rotFlags     - bitmask representing the applied rotations
************************************************************************/  
void CMatrix::RotateXRad( float dest[mMax], float value, int rotFlags )
{
    float cosX = cos(value);
    float sinX = sin(value);

    switch( rotFlags )
    {
        case ROT_Z:
        {
            float tmp4, tmp5, tmp8, tmp9;
            tmp4 = dest[4] * cosX;
            tmp5 = dest[5] * cosX;
            tmp8 = dest[4] * -sinX;
            tmp9 = dest[5] * -sinX;
            dest[4] = tmp4;
            dest[5] = tmp5;
            dest[6] = sinX;
            dest[8] = tmp8;
            dest[9] = tmp9;
            dest[10] = cosX;
            break;
        }

        case ROT_Y:
        {
            float tmp4, tmp6, tmp8, tmp10;
            tmp4 = dest[8] * sinX;
            tmp6 = dest[10] * sinX;
            tmp8 = dest[8] * cosX;
            tmp10 = dest[10] * cosX;
            dest[4] = tmp4;
            dest[5] = cosX;
            dest[6] = tmp6;
            dest[8] = tmp8;
            dest[9] = -sinX;
            dest[10] = tmp10;
            break;
        }

        case ROT_Z | ROT_Y:
        {
            float tmp4, tmp5, tmp6, tmp8, tmp9, tmp10;
            tmp4 = ( dest[4] * cosX ) + ( dest[8] * sinX );
            tmp5 = ( dest[5] * cosX ) + ( dest[9] * sinX );
            tmp6 = dest[10] * sinX;
            tmp8 = ( dest[4] * -sinX ) + ( dest[8] * cosX );
            tmp9 = ( dest[5] * -sinX ) + ( dest[9] * cosX );
            tmp10 = dest[10] * cosX;
            dest[4] = tmp4;
            dest[5] = tmp5;
            dest[6] = tmp6;
            dest[8] = tmp8;
            dest[9] = tmp9;
            dest[10] = tmp10;
            break;
        }

        case NO_ROT:
        {
            dest[5]  =  cosX;
            dest[6]  =  sinX;
            dest[9]  = -sinX;
            dest[10] =  cosX;
            break;
        }
    }

}	// RotateXRad


/************************************************************************                                                             
*    desc:  The multiplication operator 
*
*    param:  CMatrix & newMatrix - matrix to multiply
*
*    return: CMatrix - multiplied matrix
************************************************************************/
CMatrix CMatrix::operator * ( const CMatrix & obj ) const
{
    float tmp2[mMax];

    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        { 
            tmp2[(i*4)+j] =   (matrix[i*4]	   * obj[j])
                            + (matrix[(i*4)+1] * obj[4+j])
                            + (matrix[(i*4)+2] * obj[8+j])
                            + (matrix[(i*4)+3] * obj[12+j]);
        }
    }

    return CMatrix(tmp2);

}   // operator *


/************************************************************************                                                             
*    desc:  The multiplication operator 
*
*    param:  CMatrix & newMatrix - matrix to multiply
*
*    return: CMatrix - multiplied matrix
************************************************************************/
CMatrix CMatrix::operator *= ( const CMatrix & obj )
{
    float tmp2[mMax];

    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        { 
            tmp2[(i*4)+j] =   (matrix[i*4]	   * obj[j])
                            + (matrix[(i*4)+1] * obj[4+j])
                            + (matrix[(i*4)+2] * obj[8+j])
                            + (matrix[(i*4)+3] * obj[12+j]);
        }
    }

    // Copy the tmp matrix to the class's matrix
    memcpy( matrix, tmp2, sizeof(tmp2) );

    return *this;

}   // operator *=


/************************************************************************                                                             
*    desc:  The = operator 
*
*    param:  const float mat[4][4] - matrix to copy over
*
*    return: CMatrix - multiplied matrix
************************************************************************/
CMatrix CMatrix::operator = ( const float mat[4][4] )
{
    for( int i = 0; i < 4; ++i )
        for( int j = 0; j < 4; ++j )
            matrix[i*4 + j] = mat[i][j];

    return *this;

}   // operator =


/************************************************************************                                                             
*    desc:  The [] operator 
*
*    param:  uint index - index into matrix array
*
*    return: float
************************************************************************/
const float CMatrix::operator [] ( uint index ) const
{
    if( index >= mMax )
        throw NExcept::CCriticalException("Index out of range",
                    boost::str( boost::format("Index exceeds allowable range (%d,%d).\n\n%s\nLine: %s") % index % mMax % __FUNCTION__ % __LINE__ ));

    return matrix[index];

}   // operator []

float & CMatrix::operator [] ( uint index )
{
    if( index >= mMax )
        throw NExcept::CCriticalException("Index out of range",
                    boost::str( boost::format("Index exceeds allowable range (%d,%d).\n\n%s\nLine: %s") % index % mMax % __FUNCTION__ % __LINE__ ));

    return matrix[index];

}   // operator []


/************************************************************************                                                             
*    desc:  The () operator 
*
*    return: float
************************************************************************/
const float * CMatrix::operator () () const
{
    return matrix;

}   // operator ()