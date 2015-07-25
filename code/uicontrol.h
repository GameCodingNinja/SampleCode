
/************************************************************************
*    FILE NAME:       uicontrol.h
*
*    DESCRIPTION:     base class for user interface controls
************************************************************************/

#ifndef __ui_control_h__
#define __ui_control_h__

// SDL/OpenGL lib dependencies
#include <SDL.h>

// Physical component dependency
#include <common/object.h>

// Standard lib dependencies
#include <string>
#include <vector>
#include <map>

// Boost lib dependencies
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

// Game lib dependencies
#include <gui/uicontroldefs.h>
#include <gui/menudefs.h>
#include <gui/scrollparam.h>
#include <common/size.h>
#include <common/quad.h>
#include <common/rect.h>
#include <script/scriptcomponent.h>

// Forward declaration(s)
class CMenuSprite;
class CSmartGuiControl;
struct XMLNode;

class CUIControl : public CObject, boost::noncopyable
{
public:

    // Constructor
    CUIControl();

    // Destructor
    virtual ~CUIControl();

    // Load the initial info from XML node
    virtual void LoadFromNode( const XMLNode & node );

    // Update the control
    virtual void Update();

    // Transform the control
    virtual void DoTransform( const CObject & object );

    // do the render
    virtual void Render( const CMatrix & matrix );

    // Handle events
    virtual void HandleEvent( const SDL_Event & rEvent );

    // Get the control name
    std::string & GetName();

    // Get the control type
    NUIControl::EControlType GetType() const;

    // Set the control to their default behavior
    void RevertToDefaultState();

    // Get/Set the state of this control
    NUIControl::EControlState GetState() const;
    void SetState( NUIControl::EControlState state, bool setLastState = false );

    // Get the type of action
    NUIControl::EControlActionType GetActionType();

    // Get the execution action
    const std::string & GetExecutionAction();

    // Reset the control to inactive if its not disabled
    virtual void Reset( bool complete = false );

    // Check if th control is disabled
    bool IsDisabled();

    // Set/Get the Smart GUI pointer
    void SetSmartGui( CSmartGuiControl * pSmartGuiControl );
    CSmartGuiControl * GetSmartGuiPtr();

    // Do any smart create
    void SmartCreate();

    // Do any smart Display
    void SmartDisplay();

    // Set the type of action
    void SetActionType( NUIControl::EControlActionType value );

    // Set the execution action
    void SetExecutionAction( const std::string & action );

    // Create the font string
    void CreateFontString( const std::string & fontString, int spriteIndex = 0 );
    void CreateFontString( int stringIndex = 0, int spriteIndex = 0 );

    // Is the point in the control
    bool IsPointInControl( int x, int y );

    // Handle the mouse move
    virtual bool HandleMouseMove( int x, int y );

    // Handle the select action
    virtual bool HandleSelectAction( NDefs::EDeviceId deviceUsed, NMenu::EActionPress pressType );

    // Set the first inactive control to be active
    virtual bool ActivateFirstInactiveControl();

    // Get the pointer to the control if found
    virtual CUIControl * FindControl( const std::string & name );

    // Set the string to vector
    void SetStringToList( const std::string & str );

    // Change the control state
    void ChangeState( NUIControl::EControlState state );

    // Is this control active/selected
    bool IsActive();
    bool IsSelected();

    // Get the collision position
    const CPoint<float> & GetCollisionPos() const;

    // Activate the control
    bool ActivateControl();

    // Deactivate the control
    virtual void DeactivateControl();

    // Get the scroll params
    CScrollParam & GetScrollParam();

    // Get the object group name
    const std::string & GetGroup() const;

protected:

    // Load the control specific info from XML node
    virtual void LoadControlFromNode( const XMLNode & node );

    // Set the control name
    void SetName( const std::string & name );

    // Set the control type
    void SetType( NUIControl::EControlType type );

    // Set the type of action
    void SetActionType( const std::string & value );

    // Set the sprite's display based on it's current state
    void SetDisplayState();

    // Prepare the sprite script function
    void PrepareSpriteScriptFunction( NUIControl::ESpriteScriptFunctions scriptFunc );

    // Prepare the control script function to run
    void PrepareControlScriptFunction( NUIControl::EControlScriptFunctions scriptFunc );

    // Recycle the contexts
    void RecycleContext();

    // Get the number of strings
    const std::vector<std::string> & GetStringVec() const;

    // Get the sprite vector
    const std::vector<CMenuSprite *> & GetSpriteVec() const;

    // Execute the action
    void ExecuteAction();

    // Handle message
    virtual void OnStateChange( const SDL_Event & rEvent );
    virtual void OnSelectExecute( const SDL_Event & rEvent );
    void OnSetActiveControl( const SDL_Event & rEvent );
    void OnReactivate( const SDL_Event & rEvent );
    virtual void OnTransIn( const SDL_Event & rEvent );
    void OnTransOut( const SDL_Event & rEvent );

private:

    // Load a sprite from an XML node
    void LoadSpriteFromNode( const XMLNode & node );

    // Set the default state of this control
    void SetDefaultState( NUIControl::EControlState value );
    void SetDefaultState( const std::string & value );

private:

    // The type of control
    NUIControl::EControlType m_type;

    // Object group name
    std::string m_group;

    // String id
    std::string m_name;

    // control's default state
    NUIControl::EControlState m_defaultState;

    // control's current state
    NUIControl::EControlState m_state;
    NUIControl::EControlState m_lastState;

    // Name of the action to perform under the correct circumstances
    std::string m_executionAction;

    // How the control should respond when selected
    NUIControl::EControlActionType m_actionType;

    // Control string list
    std::vector<std::string> m_stringVec;

    // sprite group vector
    std::vector<CMenuSprite *> m_pSpriteVec;

    // This control's size
    CSize<float> m_size;

    // This is the size modifier
    // when calculating the collision rect
    CSize<float> m_sizeModifier;

    // Collision rect
    CQuad m_collisionQuad;

    // Collision center
    CPoint<float> m_collisionCenter;

    // Base control action class scoped pointer
    boost::scoped_ptr<CSmartGuiControl> m_scpSmartGui;

    // Mouse selection type
    NMenu::EActionPress m_mouseSelectType;

    // On state script function names
    std::map< int, std::string > m_scriptFunction;

    // Script component object
    CScriptComponent m_scriptComponent;

    // Scrolling parameters
    CScrollParam m_scrollParam;

    // No of font sprites in this control
    size_t m_fontSpriteCount;
};

#endif  // __classtemplate_h__


