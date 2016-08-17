
/************************************************************************
*    FILE NAME:       menutree.h
*
*    DESCRIPTION:     Class that hold a tree of menus
************************************************************************/

// Physical component dependency
#include <gui/menutree.h>

// Game lib dependencies
#include <utilities/xmlParser.h>
#include <utilities/xmlparsehelper.h>
#include <utilities/exceptionhandling.h>
#include <utilities/genfunc.h>
#include <gui/menu.h>

// Boost lib dependencies
#include <boost/format.hpp>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CMenuTree::CMenuTree( 
    std::map<const std::string, CMenu> & rMenuMap, 
    const std::string & rootMenu,
    const std::string & defaultMenu,
    bool interfaceMenu ) :
        m_rMenuMap(rMenuMap),
        m_pRootMenu( nullptr),
        m_pDefaultMenu( nullptr ),
        m_interfaceMenu(interfaceMenu),
        m_state(NMenu::EMTS_IDLE )
{
    auto iter = rMenuMap.find( rootMenu );
    if( iter != rMenuMap.end() )
        m_pRootMenu = &iter->second;
    
    iter = rMenuMap.find( defaultMenu );
    if( iter != rMenuMap.end() )
        m_pDefaultMenu = &iter->second;
    
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CMenuTree::~CMenuTree()
{
}   // destructer


/************************************************************************
*    desc:  Init the tree for use
************************************************************************/
void CMenuTree::Init()
{
    m_pMenuPathVec.clear();

    if( m_pRootMenu != nullptr )
    {
        // If we have a root menu, add it to the path
        m_pMenuPathVec.push_back( m_pRootMenu );

        m_pRootMenu->ActivateMenu();
    }

}   // Init


/************************************************************************
*    desc:  Update the menu tree
************************************************************************/
void CMenuTree::Update()
{
    if( !m_pMenuPathVec.empty() )
        m_pMenuPathVec.back()->Update();

}   // Update


/************************************************************************
*    desc:  Transform the menu tree
************************************************************************/
void CMenuTree::DoTransform()
{
    if( !m_pMenuPathVec.empty() )
        m_pMenuPathVec.back()->DoTransform();

}   // DoTransform


/************************************************************************
*    desc:  do the render                                                            
************************************************************************/
void CMenuTree::Render( const CMatrix & matrix )
{
    if( !m_pMenuPathVec.empty() )
        m_pMenuPathVec.back()->Render( matrix );

}   // Render


/************************************************************************
*    desc:  Is a menu active?
************************************************************************/
bool CMenuTree::IsActive()
{
    return !m_pMenuPathVec.empty();

}   // IsActive


/************************************************************************
*    desc:  Does this tee have a root menu
************************************************************************/
bool CMenuTree::HasRootMenu()
{
    return (m_pRootMenu != nullptr);

}   // HasRootMenu


/************************************************************************
*    desc:  Handle events
************************************************************************/
void CMenuTree::HandleEvent( const SDL_Event & rEvent )
{
    // Trap only controller events to check for actions
    if( !m_interfaceMenu )
    {
        if( !m_pMenuPathVec.empty() )
            m_pMenuPathVec.back()->HandleEvent( rEvent );
        
        if( m_state == NMenu::EMTS_IDLE )
        {
            if( rEvent.type == NMenu::EGE_MENU_ESCAPE_ACTION )
            {
                OnEscape( rEvent );
            }
            else if( rEvent.type == NMenu::EGE_MENU_TOGGLE_ACTION )
            {
                OnToggle( rEvent );
            }
            else if( rEvent.type == NMenu::EGE_MENU_BACK_ACTION )
            {
                OnBack( rEvent );
            }
            else if( rEvent.type == NMenu::EGE_MENU_TO_MENU )
            {
                OnToMenu( rEvent );
            }
        }
        else if( rEvent.type == NMenu::EGE_MENU_TRANS_IN )
        {
            OnTransIn( rEvent );
        }
        else if( rEvent.type == NMenu::EGE_MENU_TRANS_OUT )
        {
            OnTransOut( rEvent );
        }
    }
    // Don't process menu specific messages if this is an interface menu
    else if( (rEvent.type < NMenu::EGE_MENU_USER_EVENTS) || (rEvent.type > NMenu::EGE_MENU_GAME_STATE_CHANGE) )
    {
        if( !m_pMenuPathVec.empty() )
            m_pMenuPathVec.back()->HandleEvent( rEvent );
    }

}   // HandleEvent


/************************************************************************
*    desc:  Transition the menu
************************************************************************/
void CMenuTree::TransitionMenu()
{
    // If the path vector is empty, transition to the default menu
    if( m_pMenuPathVec.empty() )
    {
        // Make sure the menu exists
        if( m_pDefaultMenu == nullptr )
            throw NExcept::CCriticalException("Menu Display Error!",
                boost::str( boost::format("Menu does not exist (%s).\n\n%s\nLine: %s")
                    % m_pDefaultMenu->GetName() % __FUNCTION__ % __LINE__ ));

        // Add the default menu to the path
        m_pMenuPathVec.push_back( m_pDefaultMenu );

        // Get the name of the menu we are transitioning to
        // This is also used as a flag to indicate moving up the menu tree
        m_toMenu = m_pDefaultMenu->GetName();

        // Set the state as "active" so that input messages are ignored
        m_state = NMenu::EMTS_ACTIVE;

        // Start the transition in
        NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_IN, NMenu::ETC_BEGIN );
    }
    else
    {
        // If this isn't the root menu, start the transition out
        if( m_pMenuPathVec.back() != m_pRootMenu )
        {
            // Set the state as "active" so that input messages are ignored
            m_state = NMenu::EMTS_ACTIVE;

            // Start the transition out
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_OUT, NMenu::ETC_BEGIN );
        }
    }

}   // TransitionMenu


/************************************************************************
*    desc:  Handle OnEscape message
************************************************************************/
void CMenuTree::OnEscape( const SDL_Event & rEvent )
{
    TransitionMenu();

}   // OnEscape

/************************************************************************
*    desc:  Handle OnToggle message
************************************************************************/
void CMenuTree::OnToggle( const SDL_Event & rEvent )
{
    // Toggling only works when there is no root menu
    if( m_pRootMenu == nullptr )
    {
        TransitionMenu();

        // For toggle, clear out the path vector except for the current menu
        if( m_pMenuPathVec.size() > 1 )
        {
            CMenu * pCurMenu = m_pMenuPathVec.back();
            m_pMenuPathVec.clear();
            m_pMenuPathVec.push_back( pCurMenu );
        }
    }
    else
    {
        if( m_pMenuPathVec.size() > 1 )
            TransitionMenu();

        // For toggle, clear out the path vector except for the current and root menu
        if( m_pMenuPathVec.size() > 2 )
        {
            CMenu * pCurMenu = m_pMenuPathVec.back();
            m_pMenuPathVec.clear();
            m_pMenuPathVec.push_back( m_pRootMenu );
            m_pMenuPathVec.push_back( pCurMenu );
        }
    }

}   // OnToggle

/************************************************************************
*    desc:  Handle OnBack message
************************************************************************/
void CMenuTree::OnBack( const SDL_Event & rEvent )
{
    // Going back one require there to be a active menu that is not root
    if( !m_pMenuPathVec.empty() && (m_pMenuPathVec.back() != m_pRootMenu) )
    {
        TransitionMenu();
    }

}   // OnBack

/************************************************************************
*    desc:  Handle OnToMenu message
************************************************************************/
void CMenuTree::OnToMenu( const SDL_Event & rEvent )
{
    // Set the state as "active" so that input messages are ignored
    m_state = NMenu::EMTS_ACTIVE;

    // Get the name of the menu we are transitioning to
    // This is also used as a flag to indicate moving deaper into the menu tree
    m_toMenu = *(std::string *)rEvent.user.data1;

    // Do a sanity check to make sure the menu exists
    if( m_rMenuMap.find(m_toMenu) == m_rMenuMap.end() )
        throw NExcept::CCriticalException("Menu Display Error!",
            boost::str( boost::format("Menu does not exist (%s).\n\n%s\nLine: %s")
                % m_toMenu % __FUNCTION__ % __LINE__ ));

    // Start the transition out
    NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_OUT, NMenu::ETC_BEGIN );

}   // OnToMenu

/************************************************************************
*    desc:  Handle OnTransOut message
************************************************************************/
void CMenuTree::OnTransOut( const SDL_Event & rEvent )
{
    if( rEvent.user.code == NMenu::ETC_END )
    {
        if( !m_toMenu.empty() )
        {
            m_pMenuPathVec.push_back( &m_rMenuMap.find(m_toMenu)->second );
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_IN, NMenu::ETC_BEGIN );
        }
        else if( !m_pMenuPathVec.empty() && (m_pMenuPathVec.back() != m_pRootMenu) )
        {
            // Do a full reset on all the controls
            m_pMenuPathVec.back()->Reset();

            // Pop it off the vector because this menu is done
            m_pMenuPathVec.pop_back();

            if( !m_pMenuPathVec.empty() )
                NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_IN, NMenu::ETC_BEGIN );
        }

        // Normally, after one menu transitions out, the next menu transitions in
        // Only set the idle state if this transition out is final
        if( m_pMenuPathVec.empty() )
            m_state = NMenu::EMTS_IDLE;
    }

}   // OnTransOut

/************************************************************************
*    desc:  Handle OnTransIn message
************************************************************************/
void CMenuTree::OnTransIn( const SDL_Event & rEvent )
{
    if( rEvent.user.code == NMenu::ETC_END )
    {
        // m_toMenu is also used as a flag to indicate moving up the menu tree
        // When moving up the menu tree, activate the first control on the menu
        // When backing out of the menu tree, activate the last control used
        NGenFunc::DispatchEvent( NMenu::EGE_MENU_SET_ACTIVE_CONTROL, 
            (m_toMenu.empty()) ? NMenu::EAC_LAST_ACTIVE_CONTROL : NMenu::EAC_FIRST_ACTIVE_CONTROL );

        // Set to idle to allow for input messages to come through
        m_state = NMenu::EMTS_IDLE;
        
        // Clear in the event we start backing out of the menu tree
        m_toMenu.clear();
    }

}   // OnTransIn


/************************************************************************
*    desc:  Get the active menu
************************************************************************/
CMenu & CMenuTree::GetActiveMenu()
{
    if( m_pMenuPathVec.empty() )
        throw NExcept::CCriticalException("Menu Error!",
            boost::str( boost::format("There is no active menu.\n\n%s\nLine: %s")
                % __FUNCTION__ % __LINE__ ));

    return *m_pMenuPathVec.back();

}   // GetActiveMenu


/************************************************************************
*    desc:  Get the scroll param data
************************************************************************/
CScrollParam & CMenuTree::GetScrollParam( int msg )
{
    if( m_pMenuPathVec.empty() )
        throw NExcept::CCriticalException("Menu Error!",
            boost::str( boost::format("There is no active menu.\n\n%s\nLine: %s")
                % __FUNCTION__ % __LINE__ ));

    return m_pMenuPathVec.back()->GetScrollParam( msg );

}   // GetScrollParam


/************************************************************************
*    desc:  Is a menu item active
************************************************************************/
bool CMenuTree::IsMenuItemActive()
{
    bool result(false);

    if( IsActive() )
    {
        result = true;

        // Check if this is a root menu
        if( HasRootMenu() )
        {
            // Get the active menu
            CMenu & rMenu = GetActiveMenu();

            // If the root menu is active, see if a control is active
            if( m_pRootMenu == &rMenu )
            {
                if( rMenu.GetPtrToActiveControl() == nullptr )
                    result = false;
            }
        }
    }

    return result;

}   // IsMenuItemActive


/************************************************************************
*    desc:  Is this menu an interface
************************************************************************/
bool CMenuTree::IsMenuInterface() const
{
    return m_interfaceMenu;
    
}   // IsMenuInterface
