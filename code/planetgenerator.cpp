/************************************************************************
*    FILE NAME:       planetgenerator.cpp
*
*    DESCRIPTION:     Generator class to generate the space background
************************************************************************/           

// Physical dependency
#include "planetgenerator.h"

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <2d/spritegroup2d.h>
#include <2d/sector2d.h>
#include <common/uv.h>
#include <managers/instancemeshmanager.h>

// Required namespace(s)
using namespace std;

// Turn off the data type conversion warning (ie. int to float, float to int etc.)
// We do this all the time in 3D. Don't need to be bugged by it all the time.
#pragma warning(disable : 4244)
// disable warning about the use of the "this" pointer in the constructor list
#pragma warning(disable : 4355)

// The sprite count constants
const int PLANET_COUNT_MIN = 1;
const int PLANET_COUNT_MAX = 3;

// The index starts and ends of particular sprites. This way we can easily
// get what we want out of a sector's vector of sprites
const int PLANET_INDEX_START = 0;
const int PLANET_INDEX_END = PLANET_INDEX_START + PLANET_COUNT_MAX;

// Sector constants
const int PLANET_SECTOR_SIZE = 121;
const int PLANET_SECTOR_DIMENSIONS = 3;
const int PLANET_SECTOR_TOTAL = PLANET_SECTOR_DIMENSIONS * PLANET_SECTOR_DIMENSIONS;

// Depths of the dust
const int PLANET_DEPTH_MIN = 50;
const int PLANET_DEPTH_MAX = 90;

// The min and max scale of the dust
const int PLANET_SCALE_MIN = 10;
const int PLANET_SCALE_MAX = 80;

// The diameter of influence a color sector has. This must be an odd number
const int PLANET_COLOR_DIAMETER = PLANET_SECTOR_SIZE * 3;


/************************************************************************
*    desc:  Constructer
************************************************************************/
CPlanetGenerator::CPlanetGenerator()
                : CGenerator(),
                  GetRandPlanetPos( generator, IntDistribution( -(PLANET_SECTOR_SIZE >> 1), (PLANET_SECTOR_SIZE >> 1) ) ),
                  GetRandPlanetDepth( generator, IntDistribution( PLANET_DEPTH_MIN, PLANET_DEPTH_MAX ) ),
                  GetRandPlanetScale( generator, FloatDistribution( PLANET_SCALE_MIN, PLANET_SCALE_MAX ) ),
                  GetRandPlanetCount( generator, IntDistribution( PLANET_COUNT_MIN, PLANET_COUNT_MAX ) ),
                  GetRandPlanetColor( colorGenerator, FloatDistribution( 0, 1 ) ),
                  GetRandHueShift( generator, IntDistribution( -90, 90 ) )
{
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CPlanetGenerator::~CPlanetGenerator()
{
}	// destructer


/************************************************************************
*    desc:  Intialize the world generator
*
*	 param:	CPointInt & focus - which sector the player currently is in
*			uint wSeed		  - seed for generation
************************************************************************/
void CPlanetGenerator::Init( CPointInt & focus, uint wSeed )
{
    CInstanceMesh2D * pInstMesh = CInstanceMeshManager::Instance().GetInstanceMeshPtr( "(space)" );

    CObjectData2D * pPlanetObjData = CObjectDataList2D::Instance().GetData( "(space)", "planet" );
    CObjectData2D * pAtmosObjData = CObjectDataList2D::Instance().GetData( "(space)", "planet_atmosphere" );
    CObjectData2D * pShadowObjData = CObjectDataList2D::Instance().GetData( "(space)", "planet_shadow" );

    pUsedSectorVector.reserve( PLANET_SECTOR_TOTAL );

    for( int i = 0; i < PLANET_SECTOR_TOTAL; ++i )
    {
        // Create a sector
        CSector2D * pSector = new CSector2D();
        pUnusedSectorVector.push_back( pSector );
        spSectorVector.push_back( pSector );

        // Add the dust sprites to the sector
        for( int j = 0; j < PLANET_COUNT_MAX; ++j )
        {
            CSpriteGroup2D * pSpriteGrp = new CSpriteGroup2D( pPlanetObjData, true );
            pSector->AddSprite( pSpriteGrp );
            pSpriteGrp->SetInstanceMesh( pInstMesh );

            pSpriteGrp = new CSpriteGroup2D( pAtmosObjData, true );
            pSector->AddSprite( pSpriteGrp );
            pSpriteGrp->SetInstanceMesh( pInstMesh );

            pSpriteGrp = new CSpriteGroup2D( pShadowObjData, true );
            pSector->AddSprite( pSpriteGrp );
            pSpriteGrp->SetInstanceMesh( pInstMesh );
        }
    }

    // Call the base class's init function as well
    CGenerator::Init( focus, wSeed );

}	// Init */


/************************************************************************
*    desc:  Handle the world generation for space
*
*	 param: CPointInt & newFocus - the new focus position
*			CPointInt & focus    - the old focus position
*			bool forceGenerate   - forcefully generate the entire space
*								   background area	 
************************************************************************/
void CPlanetGenerator::HandleGeneration( const CPointInt & focus, const CPointInt & newFocus, bool forceGenerate )
{
    // Generate the dust
    Generate( focus, newFocus, forceGenerate );

    // Update the sprite vector with the newly generated sprites
    UpdateSpriteVector();

}   // HandleSpaceGeneration */


/************************************************************************
*    desc:  Main generation function. Determines if anything needs to
*			generate
*
*	 param: CPointInt & newFocus - the new focus position
*			CPointInt & focus    - the old focus position
*			bool forceGenerate   - forcefully generate the stars
************************************************************************/
void CPlanetGenerator::Generate( const CPointInt & focus, const CPointInt & newFocus, bool forceGenerate )
{
    // To forcefully generate our debris, we set up th generator as if the last focus was
    // far enough away to regenerate all sectors around the new focus
    if( forceGenerate )
    {
        // Change the old focus so we'll regenerate all sectors
        center = (center + PLANET_SECTOR_DIMENSIONS) * PLANET_SECTOR_SIZE;
        center.z = 0;

        // Move all the sectors to the unused sector vector
        pUnusedSectorVector.insert(pUnusedSectorVector.end(), pUsedSectorVector.begin(), pUsedSectorVector.end());
        pUsedSectorVector.clear();

        // Set the x value's starting point
        int x = center.x - ( PLANET_SECTOR_DIMENSIONS >> 1 ) * PLANET_SECTOR_SIZE;
        
        // Set the value we'll use to index into the unused sector vector
        int index = 0;

        // Set each sector's position to something outside our current focus
        for( int i = 0; i < PLANET_SECTOR_DIMENSIONS; ++i )
        {
            // Set the y value's starting point
            int y = center.y - ( PLANET_SECTOR_DIMENSIONS >> 1 ) * PLANET_SECTOR_SIZE;

            for( int j = 0; j < PLANET_SECTOR_DIMENSIONS; ++j )
            {
                // Set the new sector position
                pUnusedSectorVector[index]->SetPosition( CPointInt( x, y, PLANET_DEPTH_MIN ) );

                y += PLANET_SECTOR_SIZE; 
                ++index;
            }

            x += PLANET_SECTOR_SIZE;
        }
    }

    // Find the amount of sectors we've moved
    CPointInt focusDiff = newFocus - center;

    // If the new focus is the same as the old one, no need to perform any changes
    if( abs(focusDiff.x) > (PLANET_SECTOR_SIZE >> 1) || abs(focusDiff.y) > (PLANET_SECTOR_SIZE) >> 1 )
    {
        // Determine the new center
        CPointInt newCenter;
        int xMult, yMult;

        // Get the number generator sector multiples it takes to get to the player
        xMult = newFocus.x / PLANET_SECTOR_SIZE;
        yMult = newFocus.y / PLANET_SECTOR_SIZE;

        // Find the remainder of the divisions above
        int xMod = abs(newFocus.x) % PLANET_SECTOR_SIZE;
        int yMod = abs(newFocus.y) % PLANET_SECTOR_SIZE;

        // If the remainder is greater than half the sector size rounded down, we
        // add or subtract 1 to the multiple
        if( xMod > (PLANET_SECTOR_SIZE >> 1) )
        {
            if( newFocus.x < 0 )
                xMult -= 1;
            else
                xMult += 1;
        }

        if( yMod > (PLANET_SECTOR_SIZE >> 1) )
        {
            if( newFocus.y < 0 )
                yMult -= 1;
            else
                yMult += 1;
        }

        // Calculate the new center of the generator
        newCenter.x = xMult * PLANET_SECTOR_SIZE;
        newCenter.y = yMult * PLANET_SECTOR_SIZE;

        // Organize the sectors by ones inside the new focus range and ones outside
        // of the new focus range
        OrganizeSectors( newCenter );

        // Go through the sectors and figure out which ones we're not using anymore
        sectorVecIter = pUnusedSectorVector.begin();
        while( sectorVecIter != pUnusedSectorVector.end() )
        {
            // Calculate the sector's new location
            CPointInt newSectorPos = (*sectorVecIter)->GetPosition() - center;
            newSectorPos = newCenter + ( newSectorPos * -1 );
            newSectorPos.z = PLANET_DEPTH_MIN;
            (*sectorVecIter)->SetPosition( newSectorPos );

            // Set the seed for our background generation
            generator.seed( GetSectorSeed( newSectorPos ) );

            // Generate the elements of the background
            GeneratePlanet( sectorVecIter );

            // Move the sector from the unused vector to the used one
            pUsedSectorVector.push_back( (*sectorVecIter) );
            sectorVecIter = pUnusedSectorVector.erase( sectorVecIter );
        }

        // Set the new center
        center = newCenter;
    }

}	// GenerateBG */


/************************************************************************
*    desc:  Generate the space dust
*
*	 param: vector<CSector2D *>::iterator & sectorIter - sector to generate
*														 the dust in
************************************************************************/
void CPlanetGenerator::GeneratePlanet( vector<CSector2D *>::iterator & sectorIter )
{
    // Get a seed to determine if we have debris to create
    int planetCount = GetRandPlanetCount();

    // Set the generator seed back to the sector we're in
    uint colorSectorSeed = GetSectorSeed( GetColorSectorPos( sectorIter ) );

    // Generate the dust
    for( int i = PLANET_INDEX_START; i < PLANET_INDEX_END; ++i )
    {
        // Get the sprite group to randomize 
        int index = i * 3;
        CSpriteGroup2D * pTmpPlanet = (*sectorIter)->GetGroup( index );
        CSpriteGroup2D * pTmpAtmosphere = (*sectorIter)->GetGroup( index + 1 );
        CSpriteGroup2D * pTmpShadow = (*sectorIter)->GetGroup( index + 2 );

        if( planetCount > 0 )
        {
            // Make the planet parts visible
            pTmpPlanet->SetVisible( true );
            pTmpAtmosphere->SetVisible( true );
            pTmpShadow->SetVisible( true );
                
            // Randomize the rotation
            pTmpPlanet->SetRot( CPoint( 0, 0, GetRandRot() ) );
            
            // Randomize the position
            CWorldPoint tmpPos;
            tmpPos.x.i = GetRandPlanetPos() + (*sectorIter)->GetPosition().x;
            tmpPos.y.i = GetRandPlanetPos() + (*sectorIter)->GetPosition().y;
            tmpPos.z.i = GetRandPlanetDepth();
            tmpPos.x.f = GetRandFloatPos();
            tmpPos.y.f = GetRandFloatPos();
            tmpPos.z.f = GetRandFloatPos();
            pTmpPlanet->SetPos( tmpPos + CPoint(0,0,10) );
            pTmpAtmosphere->SetPos( tmpPos );
            pTmpShadow->SetPos( tmpPos - CPoint(0,0,10) );

            // Randomize the scale
            float tmpScale = GetRandPlanetScale();
            pTmpPlanet->SetScale( CPoint( tmpScale, tmpScale, 1 ) );
            pTmpAtmosphere->SetScale( CPoint( tmpScale, tmpScale, 1 ) );
            pTmpShadow->SetScale( CPoint( tmpScale, tmpScale, 1 ) );

            // Reset the color sector seed
            colorGenerator.seed( colorSectorSeed );

            // Now we generate the color
            CColor color;
            color.r = GetRandPlanetColor();
            color.g = GetRandPlanetColor();
            color.b = GetRandPlanetColor();

            float saturationShift = (float)PLANET_DEPTH_MIN / (float)tmpPos.z.i;

            color = color.TransformHSV( GetRandHueShift(), saturationShift, 1 );

            pTmpPlanet->SetColor( color );
            pTmpAtmosphere->SetColor( color );
        }
        else
        {
            pTmpPlanet->SetVisible( false );
            pTmpAtmosphere->SetVisible( false );
            pTmpShadow->SetVisible( false );
        }

        planetCount--;
    }

}	// GenerateDust */


/************************************************************************
*    desc:  Get the position of the color sector
************************************************************************/
CPointInt CPlanetGenerator::GetColorSectorPos( std::vector<CSector2D *>::iterator & sectorIter )
{
    // Find which dust color area this sector belongs to
    int x1, x2, y1, y2;
    CPointInt colorSectorPos;

    // Find the four color sectors surrounding the sector we're looking at
    // (x1,y1) (x1,y2) (x2,y1) (x2,y2)
    x1 = ( (*sectorIter)->GetPosition().x / PLANET_COLOR_DIAMETER ) * PLANET_COLOR_DIAMETER;

    if( (*sectorIter)->GetPosition().x > 0 )
        x2 = x1 + PLANET_COLOR_DIAMETER;
    else
        x2 = x1 - PLANET_COLOR_DIAMETER;

    y1 = ( (*sectorIter)->GetPosition().y / PLANET_COLOR_DIAMETER ) * PLANET_COLOR_DIAMETER;
    
    if( (*sectorIter)->GetPosition().y > 0 )
        y2 = y1 + PLANET_COLOR_DIAMETER;
    else
        y2 = y1 - PLANET_COLOR_DIAMETER;

    // Figure out which of the values are closest to determine which color sector we're in
    int value1, value2;
    value1 = abs( x1 - (*sectorIter)->GetPosition().x );
    value2 = abs( x2 - (*sectorIter)->GetPosition().x );

    if( value1 < value2 )
        colorSectorPos.x = x1;
    else
        colorSectorPos.x = x2;

    value1 = abs( y1 - (*sectorIter)->GetPosition().y );
    value2 = abs( y2 - (*sectorIter)->GetPosition().y );

    if( value1 < value2 )
        colorSectorPos.y = y1;
    else
        colorSectorPos.y = y2;

    // We've found our color sector position so now we use it as a seed to generate the colors
    return colorSectorPos;

}	// GetColorSectorPos


/************************************************************************
*    desc:  Find the sectors out of our focus range and moves them to the
*			unused sector vector
*
*	 param: CPointInt & point - focus point
************************************************************************/
void CPlanetGenerator::OrganizeSectors( const CPointInt & point )
{
    // The full variance equation is: 
    int variance = PLANET_SECTOR_SIZE;
    vector<CSector2D *>::iterator sectorVecIter = pUsedSectorVector.begin();

    // Go through the sectors and figure out which ones we're not using anymore
    while( sectorVecIter != pUsedSectorVector.end() )
    {
        // If we're not using them, we remove them
        if( ( (*sectorVecIter)->GetPosition().x > point.x + variance ) ||
            ( (*sectorVecIter)->GetPosition().x < point.x - variance ) ||
            ( (*sectorVecIter)->GetPosition().y > point.y + variance ) ||
            ( (*sectorVecIter)->GetPosition().y < point.y - variance ) )
        {
            pUnusedSectorVector.push_back( (*sectorVecIter) );
            sectorVecIter = pUsedSectorVector.erase( sectorVecIter );
        }
        else
            ++sectorVecIter;
    }

}	// OrganizeSectors */


/************************************************************************
*    desc:  Clear the contents of the generator
************************************************************************/
void CPlanetGenerator::Clear()
{
    CGenerator::Clear();

}	// Clear */