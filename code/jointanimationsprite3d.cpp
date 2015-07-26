/************************************************************************
*    FILE NAME:       jointanimationsprite3d.cpp
*
*    DESCRIPTION:     3D joint animation sprite class
************************************************************************/

// Physical component dependency
#include <3d/jointanimationsprite3d.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <managers/meshmanager.h>
#include <managers/shader.h>
#include <3d/joint.h>
#include <3d/jointanimationmanager.h>
#include <3d/meshanimation.h>
#include <3d/jointanimation.h>
#include <3d/mesh3d.h>
#include <3d/objectdata3d.h>
#include <3d/objectdatalist3d.h>
#include <utilities/statcounter.h>
#include <utilities/exceptionhandling.h>
#include <utilities/highresolutiontimer.h>

// Required namespace(s)
using namespace std;

/************************************************************************
*    desc:  Constructer                                                             
************************************************************************/
CJointAnimSprite3D::CJointAnimSprite3D()
                  : pCurrentAnim(NULL),
                    time(1.f),
                    pTransToAnim(NULL),
                    transTime(0.f),
                    transFrameCount(0.f),
                    updateAnim(true)
{
}   // Constructer

CJointAnimSprite3D::CJointAnimSprite3D( CObjectData3D * pObjData )
                  : pCurrentAnim(NULL),
                    time(1.f),
                    pTransToAnim(NULL),
                    transTime(0.f),
                    transFrameCount(0.f),
                    updateAnim(true)
{
    pObjectData = pObjData;

    Init();

}   // Constructer


/************************************************************************
*    desc:  Destructer                                                             
************************************************************************/
CJointAnimSprite3D::~CJointAnimSprite3D()
{
}   // Destructer


/************************************************************************
*    desc:  Init the sprite
************************************************************************/
void CJointAnimSprite3D::Init( const std::string & group, const std::string & type )
{
    pObjectData = CObjectDataList3D::Instance().GetData( group, type );

    Init();

}   // Init


/************************************************************************
*    desc:  Init the sprite
************************************************************************/
void CJointAnimSprite3D::Init()
{
    // Load the mesh
    pMesh = CMeshMgr::Instance().GetJointAnimMesh3D( pObjectData );

    // this open and parse the XML file:
    XMLNode animLstNode = XMLNode::openFileHelper( GetVisualData().GetAnimFile().c_str(), "jointAnimLst" );

    // Load the animations
    for( int i = 0; i < animLstNode.nChildNode(); ++i )
    {
        LoadAnimFromRSA( string(animLstNode.getChildNode(i).getAttribute("name")),
                              atof(animLstNode.getChildNode(i).getAttribute("fps")),
                              string(animLstNode.getChildNode(i).getAttribute("file")));
    }

    // Set the shader effect and technique
    SetEffectAndTechnique( GetVisualData().GetEffect(),
                           GetVisualData().GetTechnique() );

    // Set the color
    SetMaterialColor( GetVisualData().GetColor() );

    // Set up specular lighting
    SetSpecularProperties( GetVisualData().GetSpecularShine(),
                           GetVisualData().GetSpecularIntensity() );

}   // Init


/************************************************************************
*    desc:  Load the mesh animation from RSA file
*  
*    param: string & name - name of animation
*           float - default animation speed
*           string & animFilePath - path to file
************************************************************************/
void CJointAnimSprite3D::LoadAnimFromRSA( string & name, float fps, string & animFilePath )
{
    CMeshAnim * pMeshAnim = NULL;

    // See if this joint animation has already been loaded
    // Meshes are shared so multiple animation loads will be attemped
    jointLinkLstMapIter = spJointLinkLstMap.find( name );

    // If it's not found, load the joint animation and add it to the list
    if( jointLinkLstMapIter == spJointLinkLstMap.end() )
    {
        pMeshAnim = CJointAnimMgr::Instance().LoadFromFile( animFilePath );

        // Make sure we are dealing with the same number of joints
        if( pMeshAnim->jCount != pMesh->GetJointCount() )
            throw NExcept::CCriticalException("Joint Animation Sprite Error!",
                boost::str( boost::format("Number of joints don't match (%s).\n\n%s\nLine: %s") % animFilePath.c_str() % __FUNCTION__ % __LINE__ ));

        // Make sure that all joints saved to the animation file have the same name and parent name
        for( int i = 0; i < pMeshAnim->jCount; ++i )
        {
            bool found = false;
            for( unsigned int j = 0; j < pMesh->GetJointCount(); ++j )
            {
                if( (pMeshAnim->pJointAnim[i].name == pMesh->GetJoint(j)->name) &&
                    (pMeshAnim->pJointAnim[i].parent == pMesh->GetJoint(j)->parent) )
                {
                    found = true;
                    break;
                }
            }

            if( !found )
                throw NExcept::CCriticalException("Joint Animation Sprite Error!",
                    boost::str( boost::format("Key frame joint not found (%s).\n\n%s\nLine: %s") % animFilePath.c_str() % __FUNCTION__ % __LINE__ ));
        }

        // Allocate a temporary multi-link list class
        CJointLinkLst<CJointNode *> * pJointMultiLink = new CJointLinkLst<CJointNode *>;
        spJointLinkLstMap.insert( name, pJointMultiLink );

        pJointMultiLink->SetFrameCount( pMeshAnim->fCount );
        pJointMultiLink->SetSpeed( fps / 1000.f );

        // Create the multi-link list
        for( unsigned int i = 0; i < pMesh->GetJointCount(); ++i )
        {
            CJointNode * pTmpNode = new CJointNode();

            pTmpNode->id = pMesh->GetJoint(i)->name;
            pTmpNode->parentId = pMesh->GetJoint(i)->parent;
            pTmpNode->headPos = pMesh->GetJoint(i)->headPos;
            pTmpNode->tailPos = pMesh->GetJoint(i)->tailPos;
            pTmpNode->orientationMatrix = pMesh->GetJoint(i)->matrix;

            for( int j = 0; j < pMeshAnim->jCount; ++j )
            {
                if( pTmpNode->id == pMeshAnim->pJointAnim[j].name )
                {
                    pTmpNode->pJointAnim = &pMeshAnim->pJointAnim[j];
                    break;
                }
            }

            pJointMultiLink->AddNode( pTmpNode );
        }

        // Allocate out list of DirectX matrixes. One for each joint
        pDXMatrix.reset( new D3DXMATRIX[pMesh->GetJointCount()] );
    }

}	// LoadAnimFromRSA


/************************************************************************
*    desc:  Set the active animation
*  
*    param: string & name - name of animation
*           float frameCount - number of frames to transition the animation
************************************************************************/
void CJointAnimSprite3D::SetAnimation( const std::string & name, float frameCount )
{
    // Set the current animation if not set
    if( (pCurrentAnim == NULL) || (frameCount < 1.f) )
    {
        jointLinkLstMapIter = spJointLinkLstMap.find( name );

        if( jointLinkLstMapIter != spJointLinkLstMap.end() )
            // NOTE: This class does not own this pointer.
            pCurrentAnim = jointLinkLstMapIter->second;
        else
            throw NExcept::CCriticalException("Joint Animation Sprite Error!",
                boost::str( boost::format("Animation name not found (%s).\n\n%s\nLine: %s") % name.c_str() % __FUNCTION__ % __LINE__ ));

        time = 1.f;
    }
    else if( (animNameStr != name) )
    {
        // Only reset the time if we are not transitioning
        if( pTransToAnim == NULL )
            transTime = 0.f;

        transFrameCount = frameCount;

        jointLinkLstMapIter = spJointLinkLstMap.find( name );

        if( jointLinkLstMapIter != spJointLinkLstMap.end() )
            // NOTE: This class does not own this pointer.
            pTransToAnim = jointLinkLstMapIter->second;
        else
            throw NExcept::CCriticalException("Joint Animation Sprite Error!",
                boost::str( boost::format("Animation name not found (%s).\n\n%s\nLine: %s") % name.c_str() % __FUNCTION__ % __LINE__ ));
    }

    animNameStr = name;

}	// SetAnimation


/************************************************************************
*    desc:  Inc the animation timer
************************************************************************/
void CJointAnimSprite3D::IncAnimationTime()
{
    if( pCurrentAnim != NULL )
    {
        // Don't increment the time during a transition
        if( pTransToAnim == NULL )
        {
            // increment the time
            time += CHighResTimer::Instance().GetElapsedTime() * pCurrentAnim->GetSpeed();

            // Reset the time if we go over
            if( time > pCurrentAnim->GetFrameCount() )
                time = 1.0f + CHighResTimer::Instance().GetElapsedTime() * pCurrentAnim->GetSpeed();
        }

        // Inc the trans time during a transition
        else
            transTime += CHighResTimer::Instance().GetElapsedTime() * pTransToAnim->GetSpeed();

        updateAnim = true;
    }

}	// UpdateAnimation


/************************************************************************
*    desc:  Update the animation
************************************************************************/
void CJointAnimSprite3D::UpdateAnimation()
{
    if( pCurrentAnim != NULL && updateAnim )
    {
        updateAnim = false;

        if( pTransToAnim != NULL )
        {
            if( transTime < transFrameCount )
            {
                pTransToAnim->CalcTweenPosRot( 1.0001f );
                pCurrentAnim->CalcTweenPosRot( time );
                pCurrentAnim->TransitionTweensPosRot( pTransToAnim, transTime, transFrameCount );
            }
            else
            {
                // NOTE: This class does not own these pointers.
                pCurrentAnim = pTransToAnim;
                time = 1.f + CHighResTimer::Instance().GetElapsedTime() * pCurrentAnim->GetSpeed();

                pCurrentAnim->TransformJoints( time );

                pTransToAnim = NULL;
                transTime = 0.f;
                transFrameCount = 0.f;
            }
        }
        else
            pCurrentAnim->TransformJoints( time );
    }

}	// UpdateAnimation


/************************************************************************
*    desc:  Update the shader prior to rendering
*
*    param: CMatrix & matrix - passed in matrix class
************************************************************************/
void CJointAnimSprite3D::UpdateShader( CMatrix & matrix )
{
    // Update the animation matrixes
    UpdateAnimation();

    // Do the standard shader update
    CVisualSprite3D::UpdateShader( matrix );

    // Init the joint matrix array and send it up to the shader
    InitJointMatrixArry();

}	// UpdateShader


/************************************************************************
*    desc:  Update the shader prior to rendering
************************************************************************/
void CJointAnimSprite3D::UpdateShadowMapShader()
{
    // Update the animation matrixes
    UpdateAnimation();

    // Do the standard shader update
    CVisualSprite3D::UpdateShadowMapShader();

    // Init the joint matrix array and send it up to the shader
    InitJointMatrixArry();

}	// UpdateShadowMapShader


/************************************************************************
*    desc:  Init the joint matrix array and send it up to the shader
************************************************************************/
void CJointAnimSprite3D::InitJointMatrixArry()
{
    for( unsigned int i = 0; i < pMesh->GetJointCount(); ++i )
        pDXMatrix[i] = pCurrentAnim->GetNode(i)->matrix();

    CShader::Instance().SetEffectValue( CShader::Instance().GetActiveEffectData(),
        "jointMatrix", pMesh->GetJointCount(), pDXMatrix.get() );

}	// InitJointMatrixArry


/************************************************************************
*    desc:  Get the active animation string name
*  
*    ret: string & name - name of animation
************************************************************************/
string & CJointAnimSprite3D::GetAnimation()
{
    return animNameStr;
}


