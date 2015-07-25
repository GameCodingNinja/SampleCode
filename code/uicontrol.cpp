
/************************************************************************
*    FILE NAME:       uicontrol.cpp
*
*    DESCRIPTION:     base class for user interface controls
************************************************************************/

// Physical component dependency
#include <gui/uicontrol.h>

// Game lib dependencies
#include <utilities/xmlParser.h>
#include <utilities/xmlparsehelper.h>
#include <utilities/deletefuncs.h>
#include <utilities/genfunc.h>
#include <utilities/settings.h>
#include <utilities/exceptionhandling.h>
#include <gui/menusprite.h>
#include <gui/menudefs.h>
#include <gui/ismartguibase.h>
#include <objectdata/objectdatamanager2d.h>
#include <objectdata/objectdata2d.h>
#include <system/device.h>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CUIControl::CUIControl()
    : m_type(NUIControl::ECT_NULL),
      m_state(NUIControl::ECS_NULL),
      m_lastState(NUIControl::ECS_NULL),
      m_actionType(NUIControl::ECAT_NULL),
      m_mouseSelectType(NMenu::EAP_UP),
      m_fontSpriteCount(0)
{
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CUIControl::~CUIControl()
{
    NDelFunc::DeleteVectorPointers(m_pSpriteVec);

}   // destructer


/************************************************************************
*    desc:  Load the control info from XML node
*  
*    param: node - XML node
************************************************************************/
void CUIControl::LoadFromNode( const XMLNode & node )
{
    // See if we have a list of strings
    XMLNode stringLstNode = node.getChildNode( "fontStringLst" );
    if( !stringLstNode.isEmpty() )
    {
        for( int i = 0; i < stringLstNode.nChildNode(); ++i )
            m_stringVec.push_back( stringLstNode.getChildNode( "string", i ).getAttribute( "text" ) );
    }

    // Get the file path node to the control specific xml code
    XMLNode filePathNode = node.getChildNode( "filePath" );

    // Load the control specific xml file
    if( !filePathNode.isEmpty() )
    {
        // Get the control's file path
        std::string controlFilePath = filePathNode.getAttribute( "file" );

        // Open and parse the control's XML file:
        XMLNode controlNode = XMLNode::openFileHelper( controlFilePath.c_str(), "UIControl" );

        // Load xml specific control code
        LoadControlFromNode( controlNode );
    }

    // Set the controls name
    if( node.isAttributeSet( "name" ) )
        SetName( std::string( node.getAttribute( "name" ) ) );

    // Set the default state of the control
    if( node.isAttributeSet( "defaultState" ) )
        SetDefaultState( std::string( node.getAttribute( "defaultState" ) ) );

    // Set if mouse selection is the down message
    if( node.isAttributeSet( "mouseSelectDown" ) )
        if( std::string( node.getAttribute( "mouseSelectDown" ) ) == "true" )
            m_mouseSelectType = NMenu::EAP_DOWN;

    // Setup the action
    XMLNode actionNode = node.getChildNode( "action" );
    if( !actionNode.isEmpty() )
    {
        // Set the action type
        if( actionNode.isAttributeSet( "actionType" ) )
            SetActionType( actionNode.getAttribute( "actionType" ) );

        // Set the execution action.
        if( actionNode.isAttributeSet( "executionAction" ) )
            SetExecutionAction( std::string(actionNode.getAttribute( "executionAction" )) );
    }

    // Setup the action
    XMLNode stateScriptNode = node.getChildNode( "stateScript" );
    if( !stateScriptNode.isEmpty() )
    {
        if( stateScriptNode.isAttributeSet( "onActive" ) )
            m_scriptFunction.insert( std::make_pair(NUIControl::ECSF_ON_ACTIVE, stateScriptNode.getAttribute( "onActive" )) );

        if( stateScriptNode.isAttributeSet( "onSelect" ) )
            m_scriptFunction.insert( std::make_pair(NUIControl::ECSF_ON_SELECTED, stateScriptNode.getAttribute( "onSelect" )) );
    }

    // Get the scrolling info
    const XMLNode scrollNode = node.getChildNode( "scroll" );
    if( !scrollNode.isEmpty() )
    {
        m_scrollParam.SetStartDelay( std::atoi(scrollNode.getAttribute( "startDelay" )) );
        m_scrollParam.SetScrollDelay( std::atoi(scrollNode.getAttribute( "scrollDelay" )) );

        if( scrollNode.isAttributeSet( "up" ) && (std::string(scrollNode.getAttribute( "up" )) == "true") )
            m_scrollParam.AddMsg( NMenu::EGE_MENU_UP_ACTION );

        if( scrollNode.isAttributeSet( "down" ) && (std::string(scrollNode.getAttribute( "down" )) == "true") )
            m_scrollParam.AddMsg( NMenu::EGE_MENU_DOWN_ACTION );

        if( scrollNode.isAttributeSet( "left" ) && (std::string(scrollNode.getAttribute( "left" )) == "true") )
            m_scrollParam.AddMsg( NMenu::EGE_MENU_LEFT_ACTION );

        if( scrollNode.isAttributeSet( "right" ) && (std::string(scrollNode.getAttribute( "right" )) == "true") )
            m_scrollParam.AddMsg( NMenu::EGE_MENU_RIGHT_ACTION );
    }

    // Get the size modifier info
    m_sizeModifier = NParseHelper::LoadSize( node );

    // Init to the default state
    RevertToDefaultState();

}   // LoadFromNode


/************************************************************************
*    desc:  Load the control specific info from XML node
************************************************************************/
void CUIControl::LoadControlFromNode( const XMLNode & controlNode )
{
    // Get the object type string
    const XMLNode listGroupNode = controlNode.getChildNode( "group" );
    m_group = listGroupNode.getAttribute( "name" );

    // Get the list of object data associated with this button
    const XMLNode objDataLstNode = controlNode.getChildNode( "spriteList" );

    if( !objDataLstNode.isEmpty() )
    {
        m_pSpriteVec.reserve( objDataLstNode.nChildNode() );

        for( int i = 0; i < objDataLstNode.nChildNode(); ++i )
        {
            const XMLNode objDataNode = objDataLstNode.getChildNode( "sprite", i );

            // Load the sprite from node
            LoadSpriteFromNode( objDataNode );
        }
    }

}   // LoadControlFromNode


/************************************************************************
*    desc:  Load a sprite from an XML node
************************************************************************/
void CUIControl::LoadSpriteFromNode( const XMLNode & node )
{
    // Get the type of object
    std::string objectName = node.getAttribute( "objectName" );

    m_pSpriteVec.push_back( new CMenuSprite( CObjectDataMgr2D::Instance().GetData( m_group, objectName ) ) );

    // Load the transform data
    m_pSpriteVec.back()->LoadTransFromNode( node );

    // Init the script functions
    m_pSpriteVec.back()->InitScriptFunctions( node );

    // See if this sprite is used for rendering a font string
    const XMLNode fontNode = node.getChildNode( "font" );
    if( !fontNode.isEmpty() )
    {
        float kerning(0);
        float spaceCharKerning(0);
        float lineWrapWidth(-1);
        float lineWrapHeight(0);
        NDefs::EHorzAlignment horzAlign(NDefs::EHA_HORZ_CENTER);
        NDefs::EVertAlignment vertAlign(NDefs::EVA_VERT_CENTER);

        // Get the must have font related items
        std::string fontName = fontNode.getAttribute( "fontName" );

        // Get the attributes node
        const XMLNode attrNode = fontNode.getChildNode( "attributes" );
        if( !attrNode.isEmpty() )
        {
            if( attrNode.isAttributeSet( "kerning" ) )
                kerning = std::atof(attrNode.getAttribute( "kerning" ));

            if( attrNode.isAttributeSet( "spaceCharKerning" ) )
                spaceCharKerning = atof(attrNode.getAttribute( "spaceCharKerning" ));

            if( attrNode.isAttributeSet( "lineWrapWidth" ) )
                lineWrapWidth = atof(attrNode.getAttribute( "lineWrapWidth" ));

            if( attrNode.isAttributeSet( "lineWrapHeight" ) )
                lineWrapHeight = atof(attrNode.getAttribute( "lineWrapHeight" ));
        }

        // Get the alignment node
        const XMLNode alignmentNode = fontNode.getChildNode( "alignment" );
        if( !alignmentNode.isEmpty() )
        {
            // Set the default alignment
            horzAlign = NParseHelper::LoadHorzAlignment( alignmentNode, NDefs::EHA_HORZ_CENTER );
            vertAlign = NParseHelper::LoadVertAlignment( alignmentNode, NDefs::EVA_VERT_CENTER );
        }

        // Set the font info
        m_pSpriteVec.back()->GetVisualComponent().SetFontInfo( 
            fontName, kerning, spaceCharKerning, lineWrapWidth, lineWrapHeight, horzAlign, vertAlign );

        // Create the font by setting the font string to the first font sprite
        if( !m_stringVec.empty() && (m_fontSpriteCount < m_stringVec.size()) )
        {
            m_pSpriteVec.back()->GetVisualComponent().CreateFontString( m_stringVec.at(m_fontSpriteCount) );
        }

        // set the color if it is different
        m_pSpriteVec.back()->GetVisualComponent().SetColor( NParseHelper::LoadColor( fontNode, m_pSpriteVec.back()->GetVisualComponent().GetColor() ) );

        ++m_fontSpriteCount;
    }
    else
    {
        // Find the largest size width and height of the different sprites for this controls size
        if( m_pSpriteVec.back()->GetObjectData().GetSize().GetW() > m_size.GetW() )
            m_size.w = m_pSpriteVec.back()->GetObjectData().GetSize().GetW();

        if( m_pSpriteVec.back()->GetObjectData().GetSize().GetH() > m_size.h )
            m_size.h = m_pSpriteVec.back()->GetObjectData().GetSize().GetH();
    }

}   // LoadSpriteFromNode


/************************************************************************
*    desc:  Update the control
************************************************************************/
void CUIControl::Update()
{
    m_scriptComponent.Update();

    for( size_t i = 0; i < m_pSpriteVec.size(); ++i )
        m_pSpriteVec[i]->Update();

}   // Update


/************************************************************************
*    desc:  Transform the control
************************************************************************/
void CUIControl::DoTransform( const CObject & object )
{
    Transform( object.GetMatrix(), object.WasWorldPosTranformed() );

    for( size_t i = 0; i < m_pSpriteVec.size(); ++i )
        m_pSpriteVec[i]->Transform( GetMatrix(), WasWorldPosTranformed() );

    // Transform collision
    if( WasWorldPosTranformed() && !m_size.IsEmpty() )
    {
        CMatrix matrix, finalMatrix;
        matrix.Scale( CSettings::Instance().GetScreenRatio().GetH() );
        finalMatrix = GetMatrix() * matrix;
        finalMatrix.InvertY();

        // Get half the screen size to convert to screen coordinates
        CSize<float> screenHalf = CSettings::Instance().GetSizeHalf();

        // Create the rect of the control based on half it's size
        float halfwidth = (m_size.GetW() + m_sizeModifier.GetW()) * 0.5f;
        float halfHeight = (m_size.GetH() + m_sizeModifier.GetH()) * 0.5f;

        CQuad quad;
        quad.point[0].x = -halfwidth;
        quad.point[0].y = -halfHeight;
        quad.point[1].x = halfwidth;
        quad.point[1].y = -halfHeight;
        quad.point[2].x = halfwidth;
        quad.point[2].y = halfHeight;
        quad.point[3].x = -halfwidth;
        quad.point[3].y = halfHeight;

        finalMatrix.Transform( m_collisionQuad, quad );
        
        // Convert the translated rect to screen coordinates
        m_collisionQuad.point[0].x += screenHalf.GetW();
        m_collisionQuad.point[0].y += screenHalf.GetH();
        m_collisionQuad.point[1].x += screenHalf.GetW();
        m_collisionQuad.point[1].y += screenHalf.GetH();
        m_collisionQuad.point[2].x += screenHalf.GetW();
        m_collisionQuad.point[2].y += screenHalf.GetH();
        m_collisionQuad.point[3].x += screenHalf.GetW();
        m_collisionQuad.point[3].y += screenHalf.GetH();

        finalMatrix.Transform( m_collisionCenter, CPoint<float>() );

        // Convert to screen coordinates
        m_collisionCenter.x += screenHalf.GetW();
        m_collisionCenter.y += screenHalf.GetH();
    }

}   // DoTransform


/************************************************************************
*    desc:  do the render                                                            
************************************************************************/
void CUIControl::Render( const CMatrix & matrix )
{
    for( size_t i = 0; i < m_pSpriteVec.size(); ++i )
        m_pSpriteVec[i]->Render( matrix );

}   // Render


/************************************************************************
*    desc:  Handle events
************************************************************************/
void CUIControl::HandleEvent( const SDL_Event & rEvent )
{
    if( rEvent.type == NMenu::EGE_MENU_CONTROL_STATE_CHANGE )
    {
        OnStateChange( rEvent );
    }
    else if( rEvent.type == NMenu::EGE_MENU_SELECT_EXECUTE )
    {
        OnSelectExecute( rEvent );
    }
    else if( rEvent.type == NMenu::EGE_MENU_SET_ACTIVE_CONTROL )
    {
        OnSetActiveControl( rEvent );
    }
    else if( rEvent.type == NMenu::EGE_MENU_REACTIVATE )
    {
        OnReactivate( rEvent );
    }
    else if( rEvent.type == NMenu::EGE_MENU_TRANS_IN )
    {
        OnTransIn( rEvent );
    }
    else if( rEvent.type == NMenu::EGE_MENU_TRANS_OUT )
    {
        OnTransOut( rEvent );
    }

}   // HandleEvent


/************************************************************************
*    desc:  Handle OnTransIn message
************************************************************************/
void CUIControl::OnTransIn( const SDL_Event & rEvent )
{
    if( rEvent.user.code == NMenu::ETC_BEGIN )
    {
        // Set the script functions for the current displayed state
        if( m_lastState != m_state )
            SetDisplayState();

        PrepareSpriteScriptFunction( NUIControl::ESSF_TRANS_IN );

        if( m_scpSmartGui )
            m_scpSmartGui->Display();
    }

}   // OnTransIn

/************************************************************************
*    desc:  Handle OnTransOut message
************************************************************************/
void CUIControl::OnTransOut( const SDL_Event & rEvent )
{
    if( rEvent.user.code == NMenu::ETC_BEGIN )
    {
        // Reset the control 
        Reset();

        // Recycle the contexts
        RecycleContext();

        // Set the script functions for the current displayed state
        if( m_lastState != m_state )
            SetDisplayState();

        PrepareSpriteScriptFunction( NUIControl::ESSF_TRANS_OUT );
    }

}   // OnTransOut

/************************************************************************
*    desc:  Handle OnStateChange message
************************************************************************/
void CUIControl::OnStateChange( const SDL_Event & rEvent )
{
    const std::string & ctrlName = *(std::string *)rEvent.user.data1;
    NUIControl::EControlState state = NUIControl::EControlState(rEvent.user.code);

    // This control is the focus of the state change
    if( ctrlName == m_name )
        ChangeState( state );
    else
        DeactivateControl();

}   // OnStateChange

/************************************************************************
*    desc:  Handle OnSelectExecute message
************************************************************************/
void CUIControl::OnSelectExecute( const SDL_Event & rEvent )
{
    if( m_state == NUIControl::ECS_SELECTED )
    {
        if( m_actionType == NUIControl::ECAT_TO_MENU )
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_TO_MENU, 0, &m_executionAction );

        else if( m_actionType == NUIControl::ECAT_BACK )
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_BACK_ACTION );

        else if( m_actionType == NUIControl::ECAT_CLOSE )
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_TOGGLE_ACTION );

        else if( m_actionType == NUIControl::ECAT_BACK_TO_MAIN_MENU )
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_BACK_TO_MAIN_MENU );

        else if( m_actionType == NUIControl::ECAT_GAME_STATE_CHANGE )
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_GAME_STATE_CHANGE );

        else if( m_actionType == NUIControl::ECAT_QUIT_GAME )
            NGenFunc::DispatchEvent( SDL_QUIT );

        // Smart gui execution
        ExecuteAction();
    }

}   // OnSelectExecute

/************************************************************************
*    desc:  Handle OnSetActiveControl message
************************************************************************/
void CUIControl::OnSetActiveControl( const SDL_Event & rEvent )
{
    // Set the last active control to be active again
    if( (rEvent.user.code == NMenu::EAC_LAST_ACTIVE_CONTROL) &&
        (m_lastState > NUIControl::ECS_INACTIVE))
    {
        m_lastState = m_state = NUIControl::ECS_ACTIVE;
        RecycleContext();
        SetDisplayState();
    }

}   // OnSetActiveControl

/************************************************************************
*    desc:  Handle OnReactivate message
************************************************************************/
void CUIControl::OnReactivate( const SDL_Event & rEvent )
{
    // Set the last active control to be active again
    if( m_state > NUIControl::ECS_INACTIVE )
    {
        m_lastState = m_state = NUIControl::ECS_ACTIVE;
        RecycleContext();
        SetDisplayState();
    }

}   // OnReactivate


/************************************************************************
*    desc:  Change the control state
************************************************************************/
void CUIControl::ChangeState( NUIControl::EControlState state )
{
    if( m_state != state )
    {
        m_state = state;

        // Call any script functions associated with the state change
        if( m_state == NUIControl::ECS_ACTIVE )
            PrepareControlScriptFunction( NUIControl::ECSF_ON_ACTIVE );

        else if( m_state == NUIControl::ECS_SELECTED )
            PrepareControlScriptFunction( NUIControl::ECSF_ON_SELECTED );

        RecycleContext();
        SetDisplayState();

        m_lastState = m_state;
    }

}   // ChangeState


/************************************************************************
*    desc:  Activate the control
************************************************************************/
bool CUIControl::ActivateControl()
{
    // The focus has switched to this control
    if( !IsDisabled() )
    {
        m_lastState = m_state = NUIControl::ECS_ACTIVE;

        RecycleContext();
        SetDisplayState();
        
        return true;
    }
    
    return false;

}   // ActivateControl


/************************************************************************
*    desc:  Deactivate the control
************************************************************************/
void CUIControl::DeactivateControl()
{
    // The focus has switched away from this control
    if( (m_lastState == NUIControl::ECS_NULL) || 
        (m_lastState > NUIControl::ECS_INACTIVE) )
    {
        // Reset the control
        Reset();

        RecycleContext();
        SetDisplayState();

        m_lastState = m_state;
    }

}   // DeactivateControl


/************************************************************************
*    desc:  Set the sprite's display based on it's current state
************************************************************************/
void CUIControl::SetDisplayState()
{
    NUIControl::ESpriteScriptFunctions scriptFunc = NUIControl::ESSF_DISABLED;

    if( m_state == NUIControl::ECS_INACTIVE )
        scriptFunc = NUIControl::ESSF_INACTIVE;

    else if( m_state == NUIControl::ECS_ACTIVE )
        scriptFunc = NUIControl::ESSF_ACTIVE;

    else if( m_state == NUIControl::ECS_SELECTED )
        scriptFunc = NUIControl::ESSF_SELECTED;

    // Set the script function
    PrepareSpriteScriptFunction( scriptFunc );

}   // SetControlState


/************************************************************************
*    desc:  Prepare the sprite script function
************************************************************************/
void CUIControl::PrepareSpriteScriptFunction( NUIControl::ESpriteScriptFunctions scriptFunc )
{
    for( size_t i = 0; i < m_pSpriteVec.size(); ++i )
        m_pSpriteVec[i]->Prepare( scriptFunc );

}   // PrepareSpriteScriptFunction


/************************************************************************
*    desc:  Prepare the script function to run
************************************************************************/
void CUIControl::PrepareControlScriptFunction( NUIControl::EControlScriptFunctions scriptFunc )
{
    auto iter = m_scriptFunction.find(scriptFunc);

    if( iter != m_scriptFunction.end() )
        m_scriptComponent.PrepareScript(iter->second, m_group);

}   // PrepareControlScriptFunction


/************************************************************************
*    desc:  Reset and recycle the contexts
************************************************************************/
void CUIControl::Reset( bool complete )
{
    if( m_state > NUIControl::ECS_INACTIVE )
        m_state = NUIControl::ECS_INACTIVE;

    if( complete )
        m_lastState = m_state;

}   // Reset


/************************************************************************
*    desc:  Recycle the contexts
************************************************************************/
void CUIControl::RecycleContext()
{
    for( size_t i = 0; i < m_pSpriteVec.size(); ++i )
        m_pSpriteVec[i]->GetScriptComponent().ResetAndRecycle();

}   // RecycleContext


/************************************************************************
*    desc:  Set the default state of this control
************************************************************************/
void CUIControl::SetDefaultState( const std::string & value )
{
    if( value == "inactive" )
        m_defaultState = NUIControl::ECS_INACTIVE;

    else if( value == "active" )
        m_defaultState = NUIControl::ECS_ACTIVE;

    else if( value == "disabled" )
        m_defaultState = NUIControl::ECS_DISABLED;

    else if( value == "selected" )
        m_defaultState = NUIControl::ECS_SELECTED;

}   // SetDefaultState

void CUIControl::SetDefaultState( NUIControl::EControlState value )
{
    m_defaultState = value;

}   // SetDefaultState


/************************************************************************
*    desc:  Set/Get the smart control pointer. This class owns the pointer
************************************************************************/
void CUIControl::SetSmartGui( CSmartGuiControl * pSmartGuiControl )
{
    m_scpSmartGui.reset( pSmartGuiControl );

}	// SetSmartGui

CSmartGuiControl * CUIControl::GetSmartGuiPtr()
{
    return m_scpSmartGui.get();

}	// GetActionType


/************************************************************************
*    desc:  Do any smart create
************************************************************************/
void CUIControl::SmartCreate()
{
    if( m_scpSmartGui )
        m_scpSmartGui->Create();

}	// SmartCreate


/************************************************************************
*    desc:  Execute the action
************************************************************************/
void CUIControl::ExecuteAction()
{
    if( m_scpSmartGui )
        m_scpSmartGui->Execute();

}	// ExecuteAction


/************************************************************************
*    desc:  Do any smart Display Init
************************************************************************/
void CUIControl::SmartDisplay()
{
    if( m_scpSmartGui )
        m_scpSmartGui->Display();

}	// SmartDisplay


/************************************************************************
*    desc:  Set the control to their default behavior
************************************************************************/
void CUIControl::RevertToDefaultState()
{
    m_state = m_defaultState;

}   // RevertToDefaultState


/************************************************************************
*    desc:  Get/Set the state of this control
************************************************************************/
NUIControl::EControlState CUIControl::GetState() const
{
    return m_state;

}   // GetState

void CUIControl::SetState( NUIControl::EControlState state, bool setLastState )
{
    m_state = state;

    if( setLastState )
        m_lastState = state;

}   // SetState


/************************************************************************
*    desc:  Set/Get the control name
************************************************************************/
void CUIControl::SetName( const std::string & name )
{
    m_name = name;

}   // SetName

std::string & CUIControl::GetName()
{
    return m_name;

}   // GetName


/************************************************************************
*    desc:  Set/Get the control type
************************************************************************/
void CUIControl::SetType( NUIControl::EControlType type )
{
    m_type = type;

}   // SetName

NUIControl::EControlType CUIControl::GetType() const
{
    return m_type;

}   // GetName


/************************************************************************
*    desc:  Get/Set the control's action type
************************************************************************/
NUIControl::EControlActionType CUIControl::GetActionType()
{
    return m_actionType;

}   // GetActionType

void CUIControl::SetActionType( NUIControl::EControlActionType value )
{
    m_actionType = value;

}   // SetActionType

void CUIControl::SetActionType( const std::string & value )
{
    if( value == "action" )
        m_actionType = NUIControl::ECAT_ACTION;

    else if( value == "to_menu" )
        m_actionType = NUIControl::ECAT_TO_MENU;

    else if( value == "back" )
        m_actionType = NUIControl::ECAT_BACK;

    else if( value == "close" )
        m_actionType = NUIControl::ECAT_CLOSE;

    else if( value == "change_focus" )
        m_actionType = NUIControl::ECAT_CHANGE_FOCUS;

    else if( value == "back_to_main_menu" )
        m_actionType = NUIControl::ECAT_BACK_TO_MAIN_MENU;

    else if( value == "state_change" )
        m_actionType = NUIControl::ECAT_GAME_STATE_CHANGE;

    else if( value == "quit_game" )
        m_actionType = NUIControl::ECAT_QUIT_GAME;

}   // SetActionType


/************************************************************************
*    desc:  Get/Set the execution action
************************************************************************/
const std::string & CUIControl::GetExecutionAction()
{
    return m_executionAction;

}   // GetExecutionAction

void CUIControl::SetExecutionAction( const std::string & action )
{
    m_executionAction = action;

}   // SetExecutionAction


/************************************************************************
*    desc:  Create the font string
************************************************************************/
void CUIControl::CreateFontString( const std::string & fontString, int spriteIndex )
{
    int fontSpriteCounter(0);

    for( size_t i = 0; i < m_pSpriteVec.size(); ++i )
    {
        if( m_pSpriteVec[i]->GetObjectData().GetVisualData().GetGenerationType() == NDefs::EGT_FONT )
        {
            if( fontSpriteCounter == spriteIndex )
            {
                m_pSpriteVec[i]->GetVisualComponent().CreateFontString( fontString );
                break;
            }

            ++fontSpriteCounter;
        }
    }

}   // CreateFontString

void CUIControl::CreateFontString( int stringIndex, int spriteIndex )
{
    CreateFontString( m_stringVec[stringIndex], spriteIndex );

}   // CreateFontString


/************************************************************************
*    desc:  Get the string vector
************************************************************************/
const std::vector<std::string> & CUIControl::GetStringVec() const
{
    return m_stringVec;
}


/************************************************************************
*    desc:  Get the sprite vector
************************************************************************/
const std::vector<CMenuSprite *> & CUIControl::GetSpriteVec() const
{
    return m_pSpriteVec;
}


/************************************************************************
*    desc:  Handle the mouse move
************************************************************************/
bool CUIControl::HandleMouseMove( int x, int y )
{
    bool result(false);

    if( !IsDisabled() && IsPointInControl( x, y ) )
    {
        result = true;

        // Only send the message if it's not already active
        if( !IsActive() )
        {
            NGenFunc::DispatchEvent( 
                NMenu::EGE_MENU_CONTROL_STATE_CHANGE,
                NUIControl::ECS_ACTIVE,
                &m_name );
        }
    }

    return result;

}   // HandleMouseMove


/************************************************************************
*    desc:  Handle the select action
************************************************************************/
bool CUIControl::HandleSelectAction( NDefs::EDeviceId deviceUsed, NMenu::EActionPress pressType )
{
    // Only process this message if it's keyboard/gamepad down or mouse up
    if( IsActive() )
    {
        if( ((deviceUsed == NDefs::MOUSE) && (pressType == m_mouseSelectType)) ||
            ((deviceUsed != NDefs::MOUSE) && (pressType == NMenu::EAP_DOWN)) )
        {
            NGenFunc::DispatchEvent( 
                NMenu::EGE_MENU_CONTROL_STATE_CHANGE,
                NUIControl::ECS_SELECTED,
                &m_name );

            return true;
        }
    }

    return false;

}   // HandleSelectAction


/************************************************************************
*    desc: Set the first inactive control to be active
*    NOTE: This is mainly here to be virtual for sub controls
************************************************************************/
bool CUIControl::ActivateFirstInactiveControl()
{
    return ActivateControl();

}   // ActivateInactiveControl


/************************************************************************
*    desc:  Is the point in the control
************************************************************************/
bool CUIControl::IsPointInControl( int x, int y )
{
    //return m_collisionRect.IsPointInRect( x, y );
    return m_collisionQuad.IsPointInQuad( x, y );

}   // IsPointInControl


/************************************************************************
*    desc:  Get the pointer to the control if found
*
*    NOTE: This function is mainly for sub controls
************************************************************************/
CUIControl * CUIControl::FindControl( const std::string & name )
{
    if( m_name == name )
        return this;

    return nullptr;

}   // FindControl


/************************************************************************
*    desc:  Set the string to vector
************************************************************************/
void CUIControl::SetStringToList( const std::string & str )
{
    m_stringVec.push_back( str );

}   // SetToFontStringList


/************************************************************************
*    desc:  Is this control active/selected
************************************************************************/
bool CUIControl::IsActive()
{
    return (m_state == NUIControl::ECS_ACTIVE);

}   // IsActive

bool CUIControl::IsSelected()
{
    return (m_state == NUIControl::ECS_SELECTED);

}   // IsActive


/************************************************************************
*    desc:  Check if th control is disabled
************************************************************************/
bool CUIControl::IsDisabled()
{
    return m_state == NUIControl::ECS_DISABLED;
    
}   // IsDisabled


/************************************************************************
*    desc:  Get the collision position
************************************************************************/
const CPoint<float> & CUIControl::GetCollisionPos() const
{
    return m_collisionCenter;

}	// GetCollisionPos


/************************************************************************
*    desc:  Get the scroll params
************************************************************************/
CScrollParam & CUIControl::GetScrollParam()
{
    return m_scrollParam;

}   // GetScrollParam


/************************************************************************
*    desc:  Get the object group name
************************************************************************/
const std::string & CUIControl::GetGroup() const
{
    return m_group;

}	// GetGroup
