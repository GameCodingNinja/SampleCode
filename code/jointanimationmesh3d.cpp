
/************************************************************************
*    FILE NAME:       jointanimationmesh3d.cpp
*
*    DESCRIPTION:     3D joint animation mesh class
************************************************************************/ 

// Physical component dependency
#include <3d/jointanimationmesh3d.h>

// Boost lib dependencies
#include <boost/format.hpp>

// DirectX lib dependencies
#include <d3dx9.h>

// Game lib dependencies
#include <system/xdevice.h>
#include <managers/texturemanager.h>
#include <3d/joint.h>
#include <3d/objectdata3d.h>
#include <misc/spritebinaryfileheader.h>
#include <common/texture.h>
#include <common/xjface.h>
#include <common/xvertexbuffer.h>
#include <common/jpoint.h>
#include <utilities/smartpointers.h>
#include <utilities/exceptionhandling.h>


// Turn off function unsafe warnings
#pragma warning(disable : 4996)

/************************************************************************
*    desc:  Constructer                                                             
************************************************************************/
CJointAnimMesh3D::CJointAnimMesh3D()
                : jointCount(0)
{
    vertexForamtMask = D3DFVF_XYZB2|D3DFVF_LASTBETA_UBYTE4|D3DFVF_NORMAL|D3DFVF_TEX1;
    vertexDataSize = sizeof(CJVertex);

}   // Constructer


/************************************************************************
*    desc:  Destructer                                                             
************************************************************************/
CJointAnimMesh3D::~CJointAnimMesh3D()
{
}   // Destructer


/************************************************************************
*    desc:  Load the mesh from RSS file
*  
*    param: const CObjectData3D * pObjData - object data pointer
*    
*    return: bool - true on success or false on fail
************************************************************************/
void CJointAnimMesh3D::LoadFromRSS( const CObjectData3D * pObjData )
{
    // Open the file
    NSmart::scoped_filehandle_ptr<FILE> pFile( fopen( pObjData->GetVisualData().GetFile().c_str(), "rb") );
    if( pFile.isNull() )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Error Loading file (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    // Read in the file header
    CSpriteBinaryFileHeader fileHeader;
    fread( &fileHeader, 1, sizeof(fileHeader), pFile.get() );

    // Check to make sure we're loading in the right kind of file
    if( fileHeader.file_header != SPRITE_FILE_HEADER )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("File header mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    // Get the counts
    faceGrpCount = fileHeader.face_group_count;	
    jointCount = fileHeader.joint_count;

    // Set up a variable to test where we are in the file
    int tag_check;

    // Read in new tag
    fread( &tag_check, 1, sizeof(tag_check), pFile.get() );

    // Check and make sure we are where we should be
    if( tag_check != VERT_LIST )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Tag check mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    // Allocate local smart pointer arrays
    boost::scoped_array<CJPoint> pVert( new CJPoint[fileHeader.vert_count] );
    boost::scoped_array<CUV> pUV( new CUV[fileHeader.uv_count] );
    boost::scoped_array<CNormal> pVNormal( new CNormal[fileHeader.vert_norm_count] );
    boost::scoped_array<CBinaryJoint> pBinaryJoint( new CBinaryJoint[jointCount] );

    // Allocate class smart pointer arrays
    spXVertBuf.reset( new CXVertBuff[faceGrpCount] );
    pJoint.reset( new CJoint[fileHeader.joint_count] );
    
    // Load in the verts
    fread( pVert.get(), fileHeader.vert_count, sizeof(CJPoint), pFile.get() );

    // Read in new tag
    fread( &tag_check, 1, sizeof(tag_check), pFile.get() );

    // Check and make sure we are where we should be
    if( tag_check != UV_LIST )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Tag check mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    // Load in the uvs
    fread( pUV.get(), fileHeader.uv_count, sizeof(CUV), pFile.get() );

    // Read in new tag
    fread( &tag_check, 1, sizeof(tag_check), pFile.get() );

    // Check and make sure we are where we should be
    if( tag_check != VERT_NORM_LIST )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Tag check mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    // Load in the uvs
    fread( pVNormal.get(), fileHeader.vert_norm_count, sizeof(CNormal), pFile.get() );

    // Read in new tag
    fread( &tag_check, 1, sizeof(tag_check), pFile.get() );

    // Check and make sure we are where we should be
    if( tag_check != FACE_GROUP )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Tag check mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    // Use these variables to calculate the radius of the object
    double maxDistance = 0.0;
    double curDistance = 0.0;

    // temporary texture index array container
    std::vector<std::vector<int>> textIndexVec;

    // Add faces to the vector
    for( unsigned int i = 0; i < faceGrpCount; ++i )
    {
        CBinaryFaceGroup group;

        // Read in new tag
        fread( &tag_check, 1, sizeof(tag_check), pFile.get() );

        // Check and make sure we are where we should be
        if( tag_check != FACE_LIST )
            throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
                boost::str( boost::format("Tag check mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

        // Get the number faces in the group as well as the material index
        fread( &group, 1, sizeof(group), pFile.get() );

        // Texture or face count can't be zero
        if( (group.textureCount <= 0) || (group.groupFaceCount <= 0) )
            throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
                boost::str( boost::format("Texture or face count error (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

        spXVertBuf[i].SetFaceCount( group.groupFaceCount );

        // Collect all the texture indexes for this face
        std::vector<int> indexVec;
        for( int j = 0; j < group.textureCount; ++j )
        {
            int textIndex;
            fread( &textIndex, 1, sizeof(textIndex), pFile.get() );
            indexVec.push_back(textIndex);
        }

        textIndexVec.push_back(indexVec);

        // create the vertex buffer
        if( D3D_OK != CXDevice::Instance().GetXDevice()->CreateVertexBuffer( spXVertBuf[i].GetFaceCount() * sizeof(CXJFace),
                                                                             D3DUSAGE_WRITEONLY,
                                                                             0,
                                                                             D3DPOOL_MANAGED,
                                                                             spXVertBuf[i].GetDblPtrVertBuffer(),
                                                                             NULL ) )
        {
            throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
                boost::str( boost::format("Error creating vertex buffer (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));
        }

        // Make a face pointer
        CXJFace * pFace;

        // Lock the vertex buffer for copying
        spXVertBuf[i].GetVertBuffer()->Lock( 0, 0, (void **)&pFace, 0 );

        // Allocate and load in this group of faces
        boost::scoped_array<CBinaryFace> pBinFace( new CBinaryFace[spXVertBuf[i].GetFaceCount()] );
        fread( pBinFace.get(), spXVertBuf[i].GetFaceCount(), sizeof(CBinaryFace), pFile.get() );

        // Load the data from the xml file
        for( unsigned int j = 0; j < spXVertBuf[i].GetFaceCount(); ++j )
        {
            for( int k = 0; k < 3; ++k )
            {
                // Get the index into each of the lists
                int vIndex = pBinFace[j].vert[k];
                int uvIndex = pBinFace[j].uv[k];
                int vnIndex = pBinFace[j].normal[k];

                // Set the vertex values
                pFace[j].vert[k].vert = pVert[vIndex];

                // Set the vertex normal values
                pFace[j].vert[k].vnorm = pVNormal[vnIndex];
    
                // Set the uv values
                pFace[j].vert[k].uv = pUV[uvIndex];

                // Take this oppertunity to calculate this objects radius
                curDistance = pFace[j].vert[k].vert.GetLengthSquared();

                // Have we found the longest distance?
                if( curDistance > maxDistance )
                    maxDistance = curDistance;
            }
        }

        // Unlock the buffer so it can be used
        spXVertBuf[i].GetVertBuffer()->Unlock();
    }
        
    // Read in new tag
    fread( &tag_check, 1, sizeof(tag_check), pFile.get() );

    // Check and make sure we are where we should be
    if( tag_check != MAT_LIST )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Tag check mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    // Load all the textures associated with this mesh
    std::vector<NText::CTextureFor3D> textureLstVec;
    for(int mIndex = 0; mIndex < fileHeader.text_count; ++mIndex)
    {
        CBinaryTexture binaryTexture;

        // Read in the texture type and path
        fread( &binaryTexture, 1, sizeof(binaryTexture), pFile.get() );

        // Keep a local copy of the texture pointer and it's type
        // The Material manager owns the texture pointers and will free them
        std::string texturePath = binaryTexture.path;
        NText::CTextureFor3D texture;
        texture.pTexture = CTextureMgr::Instance().LoadFor3D( pObjData->GetGroup(), texturePath );
        texture.type = binaryTexture.type;

        textureLstVec.push_back(texture);

        if( NULL == texture.pTexture )
            throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
                boost::str( boost::format("Texture is NILL (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));
    }

    // associate the textures with the faces
    for( unsigned int i = 0; i < faceGrpCount; ++i )
        for( size_t j = 0; j < textIndexVec[i].size(); ++j )
            spXVertBuf[i].SetTexture( textureLstVec[ textIndexVec[i][j] ] );

    // compute the radius
    radiusSqrt = (float)sqrt( maxDistance );
    radius = maxDistance;

    // Read in new tag
    fread( &tag_check, 1, sizeof(tag_check), pFile.get() );
    
    // Check and make sure we are where we should be
    if( tag_check != JOINT_LIST )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Tag check mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

    int jointVertCountTest(0);

    // Read in all the joint information
    fread( pBinaryJoint.get(), jointCount, sizeof(CBinaryJoint), pFile.get() );

    // Load the joint info
    for( unsigned int i = 0; i < jointCount; ++i )
    {
        pJoint[i].name = pBinaryJoint[i].name;
        pJoint[i].parent = pBinaryJoint[i].parentName;

        pJoint[i].headPos = pBinaryJoint[i].headPos;
        pJoint[i].tailPos = pBinaryJoint[i].tailPos;

        // Add up the joint count to make sure they equal the vert count
        jointVertCountTest += pBinaryJoint[i].vert_count;

        pJoint[i].matrix = pBinaryJoint[i].orientation;
    }

    // jointVertCountTest should match vertCount
    if( jointVertCountTest != fileHeader.vert_count )
        throw NExcept::CCriticalException("Joint Animated Mesh Load Error!",
            boost::str( boost::format("Joint count mismatch (%s).\n\n%s\nLine: %s") % pObjData->GetVisualData().GetFile() % __FUNCTION__ % __LINE__ ));

}	// LoadFromRSS


/************************************************************************
*    desc:  Get the joint
*
*    param: index - index into array
*
*    ret: CJoint & - pointer to the joint
************************************************************************/
CJoint * CJointAnimMesh3D::GetJoint( unsigned int index )
{
    if( (index >= jointCount) || (pJoint.get() == NULL) )
        throw NExcept::CCriticalException("Joint Animated Mesh Error!",
            boost::str( boost::format("Get joint index exceeds count (%d of %d).\n\n%s\nLine: %s") % index % jointCount % __FUNCTION__ % __LINE__ ));

    return &pJoint[index];

}	// GetJointArray


/************************************************************************
*    desc:  Get the joint count
*
*    ret: unsigned int - number of joints
************************************************************************/
unsigned int CJointAnimMesh3D::GetJointCount()
{
    return jointCount;

}	// GetJointCount

