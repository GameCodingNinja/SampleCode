
/************************************************************************
*    FILE NAME:       menutree.h
*
*    DESCRIPTION:     Class that hold a tree of menus
************************************************************************/

// Physical component dependency
#include <gui/menutree.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <utilities/xmlParser.h>
#include <utilities/xmlparsehelper.h>
#include <utilities/exceptionhandling.h>
#include <utilities/genfunc.h>
#include <gui/menu.h>
#include <managers/actionmanager.h>

/************************************************************************
*    desc:  Constructor
************************************************************************/
CMenuTree::CMenuTree( std::map<const std::string, CMenu *> & pMenuMap, 
                      const std::string & rootMenu,
                      const std::string & defaultMenu )
    : m_pMenuMap(pMenuMap),
      m_rootMenu(rootMenu),
      m_defaultMenu(defaultMenu),
      m_state(NMenu::EMTS_IDLE)
{
}   // constructor


/************************************************************************
*    desc:  destructor                                                             
************************************************************************/
CMenuTree::~CMenuTree()
{
}   // destructor


/************************************************************************
*    desc:  Init the tree for use
************************************************************************/
void CMenuTree::Init()
{
    m_pathVector.clear();

    if( !m_rootMenu.empty() )
    {
        // If we have a root menu, add it to the path
        m_pathVector.push_back( m_rootMenu );

        auto iter = m_pMenuMap.find(m_rootMenu);
        if( iter != m_pMenuMap.end() )
            iter->second->ActivateMenu();
    }

}   // Init


/************************************************************************
*    desc:  Update the menu tree
************************************************************************/
void CMenuTree::Update()
{
    if( !m_pathVector.empty() )
    {
        m_pMenuMap.find(m_pathVector.back())->second->Update();
    }

}   // Update


/************************************************************************
*    desc:  Transform the menu tree
************************************************************************/
void CMenuTree::DoTransform()
{
    if( !m_pathVector.empty() )
    {
        m_pMenuMap.find(m_pathVector.back())->second->DoTransform();
    }

}   // DoTransform


/************************************************************************
*    desc:  do the render                                                            
************************************************************************/
void CMenuTree::Render( const CMatrix & matrix )
{
    if( !m_pathVector.empty() )
    {
        m_pMenuMap.find(m_pathVector.back())->second->Render( matrix );
    }

}   // Render


/************************************************************************
*    desc:  Is a menu active?
************************************************************************/
bool CMenuTree::IsActive()
{
    return !m_pathVector.empty();

}   // IsActive


/************************************************************************
*    desc:  Does this tee have a root menu
************************************************************************/
bool CMenuTree::HasRootMenu()
{
    return !m_rootMenu.empty();

}   // HasRootMenu


/************************************************************************
*    desc:  Handle events
************************************************************************/
void CMenuTree::HandleEvent( const SDL_Event & rEvent )
{
    if( !m_pathVector.empty() )
        m_pMenuMap.find(m_pathVector.back())->second->HandleEvent( rEvent );

    // Trap only controller events to check for actions
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

}   // HandleEvent


/************************************************************************
*    desc:  Transition the menu
************************************************************************/
void CMenuTree::TransitionMenu()
{
    // If the path vector is empty, transition to the default menu
    if( m_pathVector.empty() )
    {
        // Make sure the menu exists
        if( m_pMenuMap.find(m_defaultMenu) == m_pMenuMap.end() )
            throw NExcept::CCriticalException("Menu Display Error!",
                boost::str( boost::format("Menu does not exist (%s).\n\n%s\nLine: %s") % m_defaultMenu % __FUNCTION__ % __LINE__ ));

        // Add the default menu to the path
        m_pathVector.push_back( m_defaultMenu );

        // Get the name of the menu we are transitioning to
        m_toMenu = m_defaultMenu;

        // Set the state as "active" so that it can't be interrupted
        m_state = NMenu::EMTS_ACTIVE;

        // Start the transition in
        NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_IN, NMenu::ETC_BEGIN );
    }
    else
    {
        // If this isn't the root menu, start the transition out
        if( m_pathVector.back() != m_rootMenu )
        {
            // Set the state as "active" so that it can't be interrupted
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
    if( m_rootMenu.empty() )
    {
        TransitionMenu();

        // For toggle, clear out the path vector except for the current menu
        if( m_pathVector.size() > 1 )
        {
            std::string curMenu = m_pathVector.back();
            m_pathVector.clear();
            m_pathVector.push_back( curMenu );
        }
    }
    else
    {
        if( m_pathVector.size() > 1 )
            TransitionMenu();

        // For toggle, clear out the path vector except for the current and root menu
        if( m_pathVector.size() > 2 )
        {
            std::string curMenu = m_pathVector.back();
            m_pathVector.clear();
            m_pathVector.push_back( m_rootMenu );
            m_pathVector.push_back( curMenu );
        }
    }

}   // OnToggle

/************************************************************************
*    desc:  Handle OnBack message
************************************************************************/
void CMenuTree::OnBack( const SDL_Event & rEvent )
{
    // Going back one require there to be a active menu that is not root
    if( !m_pathVector.empty() && (m_pathVector.back() != m_rootMenu) )
    {
        TransitionMenu();
    }

}   // OnBack

/************************************************************************
*    desc:  Handle OnToMenu message
************************************************************************/
void CMenuTree::OnToMenu( const SDL_Event & rEvent )
{
    // Set the state as "active" so that it can't be interrupted
    m_state = NMenu::EMTS_ACTIVE;

    // Get the name of the menu we are transitioning to
    m_toMenu = *(std::string *)rEvent.user.data1;

    if( m_pMenuMap.find(m_toMenu) == m_pMenuMap.end() )
        throw NExcept::CCriticalException("Menu Display Error!",
            boost::str( boost::format("Menu does not exist (%s).\n\n%s\nLine: %s") % m_toMenu % __FUNCTION__ % __LINE__ ));

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
            m_pathVector.push_back( m_toMenu );
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_IN, NMenu::ETC_BEGIN );
        }
        else if( m_pathVector.back() != m_rootMenu )
        {
            // Do a full reset on all the controls
            m_pMenuMap.find(m_pathVector.back())->second->Reset();

            // Pop it off the vector because this menu is done
            m_pathVector.pop_back();

            if( !m_pathVector.empty() )
                NGenFunc::DispatchEvent( NMenu::EGE_MENU_TRANS_IN, NMenu::ETC_BEGIN );
        }

        // Normally, after one menu transitions out, the next menu transitions in
        // Only set the idle state if this transition out is final
        if( m_pathVector.empty() )
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
        if( !CActionManager::Instance().WasLastDeviceMouse() )
        {
            NGenFunc::DispatchEvent( NMenu::EGE_MENU_SET_ACTIVE_CONTROL, 
                (m_toMenu.empty()) ? NMenu::EAC_LAST_ACTIVE_CONTROL : NMenu::EAC_FIRST_ACTIVE_CONTROL );
        }

        m_state = NMenu::EMTS_IDLE;
        m_toMenu.clear();
    }

}   // OnTransIn


/************************************************************************
*    desc:  Get the active menu
************************************************************************/
CMenu * CMenuTree::GetActiveMenu()
{
    if( m_pathVector.empty() )
        throw NExcept::CCriticalException("Menu Error!",
            boost::str( boost::format("There is no active menu.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    return m_pMenuMap.find(m_pathVector.back())->second;

}   // GetActiveMenu


/************************************************************************
*    desc:  Get the scroll param data
************************************************************************/
CScrollParam & CMenuTree::GetScrollParam( int msg )
{
    if( m_pathVector.empty() )
        throw NExcept::CCriticalException("Menu Error!",
            boost::str( boost::format("There is no active menu.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    return m_pMenuMap.find(m_pathVector.back())->second->GetScrollParam( msg );

}   // GetScrollParam

