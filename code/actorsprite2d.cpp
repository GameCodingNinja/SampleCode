
/************************************************************************
*    FILE NAME:       actorsprite2d.cpp
*
*    DESCRIPTION:     Class template
************************************************************************/

// Physical component dependency
#include <2d/actorsprite2d.h>

// Game lib dependencies
#include <utilities/deletefuncs.h>
#include <utilities/exceptionhandling.h>
#include <utilities/settings.h>
#include <common/actordata.h>
#include <common/size.h>
#include <2d/sprite2d.h>
#include <2d/iaibase2d.h>
#include <objectdata/objectdatamanager.h>
#include <objectdata/objectdata2d.h>

// SDL/OpenGL lib dependencies
#include <SDL.h>

// Boost lib dependencies
#include <boost/format.hpp>

/************************************************************************
*    desc:  Constructor
************************************************************************/
CActorSprite2D::CActorSprite2D( const CActorData & actorData, int id ) :
    m_radius(0),
    m_scaledRadius(0),
    m_projectionType(CSettings::Instance().GetProjectionType()),
    m_id(id),
    m_collisionGroup(0),
    m_collisionMask(0)
{
    Create( actorData );

}   // constructor


/************************************************************************
*    desc:  destructor
************************************************************************/
CActorSprite2D::~CActorSprite2D()
{
    NDelFunc::DeleteVectorPointers( m_pSpriteVec );

}   // destructor


/************************************************************************
*    desc:  Set the AI pointer. This class owns the pointer
************************************************************************/
void CActorSprite2D::SetAI( iAIBase2D * pAIBase )
{
    m_upAI.reset(pAIBase);

    // Handle any initialization in a seperate function
    m_upAI->Init();

}   // SetAI


/************************************************************************
*    desc:  React to what the player is doing
************************************************************************/
void CActorSprite2D::HandleEvent( const SDL_Event & rEvent )
{
    if( m_upAI )
        m_upAI->HandleEvent( rEvent );

}   // HandleEvent


/************************************************************************
*    desc:  Create the actor's sprites
************************************************************************/
void CActorSprite2D::Create( const CActorData & actorData )
{
    const auto & spriteDataVec = actorData.GetSpriteData();

    m_pSpriteVec.reserve( spriteDataVec.size() );

    CSize<float> largestSize;

    for( auto & iter: spriteDataVec )
    {
        // Allocate the sprite and add it to the map for easy access
        m_pSpriteVec.push_back( new CSprite2D(CObjectDataMgr::Instance().GetData2D( iter.GetGroup(), iter.GetObjectName() )) );
        m_pSpriteMap.emplace( iter.GetObjectName(), m_pSpriteVec.back() );

        // Copy over the transform
        iter.CopyTransform( m_pSpriteVec.back() );

        // Find the largest size width and height of the different sprites for the controls size
        const CSize<float> & size = m_pSpriteVec.back()->GetObjectData().GetSize();
        const CPoint<CWorldValue> & pos = m_pSpriteVec.back()->GetPos();
        const CPoint<float> & scale = m_pSpriteVec.back()->GetScale();

        const float width( (size.w + std::fabs(pos.x)) * scale.x );
        const float height( (size.h + std::fabs(pos.y)) * scale.y );

        // Find the largest size to use as radius
        if( width > largestSize.w )
            largestSize.w = width;

        if( height > largestSize.h )
            largestSize.h = height;
    }

    // Convert the largest width and height to a radius
    largestSize /= 2;
    m_radius = largestSize.GetLength();
    m_scaledRadius = m_radius;
    
    // Set the collision filter info
    m_collisionGroup = actorData.GetCollisionGroup();
    m_collisionMask = actorData.GetCollisionMask();
    m_collisionRadiusScalar = actorData.GetCollisionRadiusScalar();
    
    // Init the radius for collision
    m_collisionRadius = m_scaledRadius * m_collisionRadiusScalar;

}   // Create


/************************************************************************
*    desc:  Do the Physics
************************************************************************/
void CActorSprite2D::Physics()
{

}   // Physics


/************************************************************************
*    desc:  Update the actor
************************************************************************/
void CActorSprite2D::Update()
{
    if( m_upAI )
        m_upAI->Update();

    for( auto iter : m_pSpriteVec )
        iter->Update();

}   // Update


/************************************************************************
*    desc:  Transform the actor
************************************************************************/
void CActorSprite2D::DoTransform()
{
    Transform();

    for( auto iter : m_pSpriteVec )
        iter->Transform( GetMatrix(), WasWorldPosTranformed() );

}   // Transform

void CActorSprite2D::DoTransform( const CObject2D & object )
{
    Transform( object.GetMatrix(), object.WasWorldPosTranformed() );

    for( auto iter : m_pSpriteVec )
        iter->Transform( GetMatrix(), WasWorldPosTranformed() );

}   // Transform


/************************************************************************
*    desc:  Render the actor
************************************************************************/
void CActorSprite2D::Render( const CMatrix & matrix )
{
    // Render in reverse order
    if( InView() )
        for( auto it = m_pSpriteVec.rbegin(); it != m_pSpriteVec.rend(); ++it )
            (*it)->Render( matrix );

}   // Render


/************************************************************************
*    desc:  Get the sprite
************************************************************************/
CSprite2D & CActorSprite2D::GetSprite( size_t index )
{
    return *m_pSpriteVec.at(index);

}   // GetSprite


/************************************************************************
*    desc:  Get the sprite group
************************************************************************/
CSprite2D & CActorSprite2D::GetSprite( const std::string & name )
{
    auto iter = m_pSpriteMap.find( name );
    if( iter == m_pSpriteMap.end() )
        throw NExcept::CCriticalException( "Actor Sprite Access Error!",
            boost::str( boost::format("Sprite name does not exist (%s).\n\n%s\nLine: %s")
                % name % __FUNCTION__ % __LINE__ ) );

    return *iter->second;

}   // GetSprite


/************************************************************************
*    desc:  Render the actor
************************************************************************/
bool CActorSprite2D::InView()
{
    if( m_projectionType == NDefs::EPT_ORTHOGRAPHIC )
        return InOrthographicView();

    else if( m_projectionType == NDefs::EPT_PERSPECTIVE )
        return InPerspectiveView();

    return true;

}   // InView


/************************************************************************
 *    desc:  Check if an object is within the orthographic view frustum
 ************************************************************************/
bool CActorSprite2D::InOrthographicView()
{
    // Check against the right side of the screen
    if( std::fabs(m_transPos.x) > (CSettings::Instance().GetDefaultSizeHalf().w + m_scaledRadius) )
        return false;

    // Check against the top of the screen
    if( std::fabs(m_transPos.y) > (CSettings::Instance().GetDefaultSizeHalf().h + m_scaledRadius) )
        return false;

    // If we made it this far, the object is in view
    return true;

}   // InOrthographicView


/************************************************************************
 *    desc:  Check if an object is within the perspective view frustum
 ************************************************************************/
bool CActorSprite2D::InPerspectiveView()
{
    const CSize<float> & aspectRatio = CSettings::Instance().GetScreenAspectRatio();

    // Check the right and left sides of the screen
    if( std::fabs(m_transPos.x) > ((std::fabs(m_pos.z) * aspectRatio.w) + m_scaledRadius) )
        return false;

    // Check the top and bottom sides of the screen
    if( std::fabs(m_transPos.y) > ((std::fabs(m_pos.z) * aspectRatio.h) + m_scaledRadius) )
        return false;

    // if we made it this far, the object was not culled
    return true;

}   // InPerspectiveView


/************************************************************************
*    desc:  Get the unique id number
************************************************************************/
int CActorSprite2D::GetId()
{
    return m_id;

}   // GetId


/************************************************************************
*    desc:  Apply the scale to the radius
************************************************************************/
void CActorSprite2D::ApplyScale()
{
    CObject2D::ApplyScale();

    // Find the largest actor scale
    float scale = GetScale().x;
    if( GetScale().y > scale )
        scale = GetScale().y;

    m_scaledRadius = m_radius * scale;
    
    // Init the radius for collision
    m_collisionRadius = m_scaledRadius * m_collisionRadiusScalar;

}   // ApplyScale


/************************************************************************
*    desc:  Get the collision group
************************************************************************/
uint CActorSprite2D::GetCollisionGroup() const
{
    return m_collisionGroup;
}


/************************************************************************
*    desc:  Get the collision radius
************************************************************************/
float CActorSprite2D::GetCollisionRadius() const
{
    return m_collisionRadius;
}


/***************************************************************************
*    desc:  Check for broad phase collision against other actor sprite
****************************************************************************/
bool CActorSprite2D::IsCollision( CActorSprite2D * pActor )
{
    bool result(false);
    
    if( m_collisionMask & pActor->GetCollisionGroup() )
    {
        result = CheckBroadPhase( pActor );
    }
    
    return result;
    
}   // IsCollision


/***************************************************************************
*    desc:  Check for broad phase collision against other actor sprite
****************************************************************************/
bool CActorSprite2D::CheckBroadPhase( CActorSprite2D * pActor )
{
    const float radius = m_collisionRadius + pActor->GetCollisionRadius();
    const float length = m_transPos.GetLength2D( pActor->GetTransPos() );
    
    return (length < radius);
    
}   // CheckBroadPhase
