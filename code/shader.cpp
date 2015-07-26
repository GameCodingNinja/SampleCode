
/************************************************************************
*    FILE NAME:       shader.cpp
*
*    DESCRIPTION:     shader class singleton
************************************************************************/

// Physical component dependency
#include <managers/shader.h>

// Standard lib dependencies
#include <assert.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <xmlParser/xmlParser.h>
#include <utilities/exceptionhandling.h>
#include <system/xdevice.h>

// Required namespace(s)
using namespace std;

/************************************************************************
*    desc:  Constructer                                                             
************************************************************************/
CShader::CShader()
       : pActiveEffect(NULL)
{
    // Set the shader data type for string to type conversion
    effectTypeMap.insert(make_pair("bool", EET_BOOL));
    effectTypeMap.insert(make_pair("int", EET_INT));
    effectTypeMap.insert(make_pair("float", EET_FLOAT));
    effectTypeMap.insert(make_pair("vecter", EET_VECTOR));
    effectTypeMap.insert(make_pair("matrix", EET_MATRIX));
    effectTypeMap.insert(make_pair("texture", EET_TEXTURE));
    effectTypeMap.insert(make_pair("bool_array", EET_BOOL_ARRAY));
    effectTypeMap.insert(make_pair("int_array", EET_INT_ARRAY));
    effectTypeMap.insert(make_pair("float_array", EET_FLOAT_ARRAY));
    effectTypeMap.insert(make_pair("vecter_array", EET_VECTOR_ARRAY));
    effectTypeMap.insert(make_pair("matrix_array", EET_MATRIX_ARRAY));

}   // Constructer


/************************************************************************
*    desc:  Destructer                                                             
************************************************************************/
CShader::~CShader()
{
}   // Destructer


/************************************************************************
*    desc:  Free the shaders
************************************************************************/
void CShader::Free()
{
    pActiveEffect = NULL;
    activeEffectStr = "";

    spEffectDataMap.clear();

}	// Free


/************************************************************************
*    desc:  Connect to the shader signal
*  
*    param: slot - boost slot
************************************************************************/
void CShader::Connect( const ShaderSignal::slot_type & slot )
{
    signal.connect(slot);

}	// Connect


/************************************************************************
*    desc:  Load the shader from xml file path
*  
*    param: string & filePath - path to shader xml file
************************************************************************/
void CShader::LoadFromXML( const string & filePath )
{
    HRESULT hr;

    // this open and parse the XML file:
    XMLNode mainNode = XMLNode::openFileHelper( filePath.c_str(), "shaderLst" );
    if( mainNode.isEmpty() )
        throw NExcept::CCriticalException("Shader Load Error!",
            boost::str( boost::format("Shader XML empty (%s).\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));

    for( int i = 0; i < mainNode.nChildNode(); ++i )
    {
        XMLNode shaderNode = mainNode.getChildNode(i);

        // Get the id for this effect file
        string idStr = string(shaderNode.getChildNode("effect").getAttribute("strId"));

        // See if this shader has already been loaded
        effectDataMapIter = spEffectDataMap.find( idStr );

        // If it's not found, load the shader and add it to the list
        if( effectDataMapIter == spEffectDataMap.end() )
        {
            LPD3DXEFFECT pEffect = NULL;
            LPD3DXBUFFER pBufferError = NULL;

            string effectFilePath(shaderNode.getChildNode("effect").getAttribute("file"));

            // Try to load the shader into memory
            if( FAILED( hr = D3DXCreateEffectFromFile( CXDevice::Instance().GetXDevice(),
                                  effectFilePath.c_str(),
                                  NULL,
                                  NULL,
                                  D3DXFX_NOT_CLONEABLE,
                                  NULL,
                                  &pEffect,
                                  &pBufferError ) ) )
            {
                throw NExcept::CCriticalException("Shader Load Error!",
                    boost::str( boost::format("Error creating shader (%s).\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));
            }

            CEffectData * pEffectData = new CEffectData;
            spEffectDataMap.insert( idStr, pEffectData );

            pEffectData->spEffect.Attach( pEffect );
            pEffectData->effectNameStr = idStr;

            // Get the shader type list
            XMLNode dataTypeLstNode = shaderNode.getChildNode("dataTypeLst");

            if( !dataTypeLstNode.isEmpty() )
            {
                for( int j = 0; j < dataTypeLstNode.nChildNode(); ++j )
                {
                    string dataTypeName = string(dataTypeLstNode.getChildNode(j).getAttribute("name"));

                    D3DXHANDLE handle = pEffect->GetParameterByName( NULL, dataTypeName.c_str() );

                    if( handle == NULL )
                        throw NExcept::CCriticalException("Shader Load Error!",
                            boost::str( boost::format("Error getting handle to effect data type. (%s), (%s), (%s)\n\n%s\nLine: %s") % filePath.c_str() % effectFilePath.c_str() % dataTypeName.c_str() % __FUNCTION__ % __LINE__ ));

                    CEffectType * pEffectType = new CEffectType;
                    pEffectData->spEffectTypeMap.insert( dataTypeName, pEffectType );

                    pEffectType->handle = handle;
                    pEffectType->type = effectTypeMap[string(dataTypeLstNode.getChildNode(j).getAttribute("type"))];

                    if( dataTypeLstNode.getChildNode(j).isAttributeSet("maxElements") )
                        pEffectType->arrayCount = atoi(dataTypeLstNode.getChildNode(j).getAttribute("maxElements"));
                }
            }
        }
    }

}	// LoadFromFile


/************************************************************************
*    desc:  Ennumerate through all the loaded shaders to init them
************************************************************************/
void CShader::EnumerateShaderInit()
{
    for( effectDataMapIter = spEffectDataMap.begin();
         effectDataMapIter != spEffectDataMap.end();
         ++effectDataMapIter )
    {
        signal(effectDataMapIter->second);
    }

}	// EnumerateShaderInit


/************************************************************************
*    desc:  Get the effect
*  
*    param: string & effectStr - effect string
* 
*    return: CEffectData *  - returns effect data pointer
************************************************************************/
CEffectData * CShader::GetEffectData( const string & effectStr )
{
    CEffectData * result = NULL;

    // See if this shader has already been loaded
    effectDataMapIter = spEffectDataMap.find( effectStr );

    if( effectDataMapIter != spEffectDataMap.end() )
    {
        // Get the pointer to the effect
        result = effectDataMapIter->second;
    }
    else
        throw NExcept::CCriticalException("Shader Effect Error!",
                            boost::str( boost::format("Shader Effect not loaded (%s).\n\n%s\nLine: %s") % effectStr.c_str() % __FUNCTION__ % __LINE__ ));

    return result;

}	// GetEffectData


/************************************************************************
*    desc:  Set the active shader effect
*  
*    param: string & effectStr - effect string
* 
*    return: bool - true on success or false on fail
************************************************************************/
void CShader::SetEffect( const string & effectStr )
{
    // If it's not found, set the pointer
    if( activeEffectStr != effectStr )
    {
        // See if this effect has already been loaded
        effectDataMapIter = spEffectDataMap.find( effectStr );

        if( effectDataMapIter != spEffectDataMap.end() )
        {
            // Get the pointer to the effect
            pActiveEffect = effectDataMapIter->second;
            activeEffectStr = effectStr;
        }
        else
            throw NExcept::CCriticalException("Shader Effect Error!",
                            boost::str( boost::format("Error setting Shader Effect (%s).\n\n%s\nLine: %s") % effectStr.c_str() % __FUNCTION__ % __LINE__ ));
    }

}	// SetEffect


/************************************************************************
*    desc:  Set the active shader technique
*  
*    param: string & techniqueStr - technique name
************************************************************************/
void CShader::SetTechnique( CEffectData * pEffectData, const string & techniqueStr )
{
    if( pEffectData->activeTechniqueStr != techniqueStr )
    {
        pEffectData->activeTechniqueStr = techniqueStr;

        if( FAILED(pEffectData->spEffect->SetTechnique( techniqueStr.c_str() ) ) )
            throw NExcept::CCriticalException("Shader Technique Error!",
                            boost::str( boost::format("Error setting Shader Technique (%s).\n\n%s\nLine: %s") % techniqueStr.c_str() % __FUNCTION__ % __LINE__ ));
    }

}	// SetTechnique


/************************************************************************
*    desc:  Set the active shader effect and technique
*  
*    param: string & effectStr - effect string
*    param: string & techniqueStr - technique name
* 
*    return: CEffectData * - Return the active effect
************************************************************************/
CEffectData * CShader::SetEffectAndTechnique( const string & effectStr, const string & techniqueStr )
{
    SetEffect( effectStr );
    SetTechnique( pActiveEffect, techniqueStr );

    return pActiveEffect;

}	// SetEffectAndTechnique

void CShader::SetEffectAndTechnique( CEffectData * pEffectData, const string & techniqueStr )
{
    pActiveEffect = pEffectData;
    activeEffectStr = pActiveEffect->effectNameStr;

    SetTechnique( pEffectData, techniqueStr );

}	// SetEffectAndTechnique


/************************************************************************
*    desc:  Get the active shader
* 
*    return: LPD3DXEFFECT - pointer to shader effect
************************************************************************/
LPD3DXEFFECT CShader::GetActiveShader()
{
    if( pActiveEffect != NULL )
        return pActiveEffect->spEffect;

    else
        throw NExcept::CCriticalException("Shader Technique Error!",
            boost::str( boost::format("No active shader set.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

}	// GetActiveShader


/************************************************************************
*    desc:  Get the active effect hsader data
* 
*    return: LPD3DXEFFECT - pointer to shader effect
************************************************************************/
CEffectData * CShader::GetActiveEffectData()
{
    if( pActiveEffect != NULL )
        return pActiveEffect;

    else
        throw NExcept::CCriticalException("Shader Technique Error!",
            boost::str( boost::format("No active effect shader data.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

}	// GetActiveEffectData


/************************************************************************
*    desc:  Is a shader active
* 
*    return: bool - true if shader is active
************************************************************************/
bool CShader::IsShaderActive()
{
    return (pActiveEffect != NULL);

}	// IsShaderActive


/************************************************************************
*    desc:  Get element count
*  
*    param: string & variableStr - variable name
*
*    return: int - number of elements in this effect array
************************************************************************/
int CShader::GetElementCount( CEffectData * pEffectData, const string & variableStr )
{
    int result = 0;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, get the effect variable array count
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
        result = effectTypeMapIter->second->arrayCount;

    if( result == 0 )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

    return result;

}	// GetElementCount


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           int value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const int value )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetInt( effectTypeMapIter->second->handle, value ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           bool value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const bool value )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetBool( effectTypeMapIter->second->handle, value ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           float value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const float value )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetFloat( effectTypeMapIter->second->handle, value ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           D3DXVECTOR4 value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const D3DXVECTOR4 & value )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetVector( effectTypeMapIter->second->handle, &value ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           D3DXMATRIX value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const D3DXMATRIX & value )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetMatrix( effectTypeMapIter->second->handle, &value ) ) )
            result = true;

        if( !result )
            throw NExcept::CCriticalException("Unable to set Effect variable!",
                boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));
    }

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           LPDIRECT3DTEXTURE9 value - value
************************************************************************/
void CShader::SetEffectValue( const string & variableStr, const LPDIRECT3DTEXTURE9 pValue )
{
    if( pActiveEffect != NULL )
    {
        bool result = false;

        // See if we can find variable string in the map
        effectTypeMapIter = pActiveEffect->spEffectTypeMap.find( variableStr );

        // If it's found, see if we can set the effect variable
        if( effectTypeMapIter != pActiveEffect->spEffectTypeMap.end() )
        {
            // DirectX data type specific variable function
            if( SUCCEEDED( pActiveEffect->spEffect->SetTexture( effectTypeMapIter->second->handle, pValue ) ) )
                result = true;
        }

        if( !result )
            throw NExcept::CCriticalException("Unable to set Effect variable!",
                boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pActiveEffect->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));
    }

}	// SetEffectValue

void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, LPDIRECT3DTEXTURE9 pValue )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetTexture( effectTypeMapIter->second->handle, pValue ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           unsigned int arrayCount - number of array elements
*           bool * value - NOTE: even thought his is a bool *, use BOOL *
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const uint arrayCount, bool * pValue )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // make sure the array amount is less or equil to the max amount
        if( arrayCount > effectTypeMapIter->second->arrayCount )
            throw NExcept::CCriticalException("Shader Effect Value Error!",
                boost::str( boost::format("Array count exceeds defined shader count (%d/%d) for effect (%s) variable (%s).\n\n%s\nLine: %s") 
                    % arrayCount % effectTypeMapIter->second->arrayCount % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetBoolArray( effectTypeMapIter->second->handle,
                                                            reinterpret_cast<BOOL *>(pValue),
                                                            arrayCount ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	//SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           unsigned int arrayCount - number of array elements
*           D3DXMATRIX * value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, uint arrayCount, int * pValue )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // make sure the array amount is less or equil to the max amount
        if( arrayCount > effectTypeMapIter->second->arrayCount )
            throw NExcept::CCriticalException("Shader Effect Value Error!",
                boost::str( boost::format("Array count exceeds defined shader count (%d/%d) for effect (%s) variable (%s).\n\n%s\nLine: %s") 
                    % arrayCount % effectTypeMapIter->second->arrayCount % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetIntArray( effectTypeMapIter->second->handle,
                                                           pValue,
                                                           arrayCount ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           unsigned int arrayCount - number of array elements
*           D3DXMATRIX * value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const uint arrayCount, float * pValue )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // make sure the array amount is less or equil to the max amount
        if( arrayCount > effectTypeMapIter->second->arrayCount )
            throw NExcept::CCriticalException("Shader Effect Value Error!",
                boost::str( boost::format("Array count exceeds defined shader count (%d/%d) for effect (%s) variable (%s).\n\n%s\nLine: %s") 
                    % arrayCount % effectTypeMapIter->second->arrayCount % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetFloatArray( effectTypeMapIter->second->handle,
                                                             pValue,
                                                             arrayCount ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           unsigned int arrayCount - number of array elements
*           D3DXMATRIX * value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const uint arrayCount, D3DXVECTOR4 * pValue )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // make sure the array amount is less or equil to the max amount
        if( arrayCount > effectTypeMapIter->second->arrayCount )
            throw NExcept::CCriticalException("Shader Effect Value Error!",
                boost::str( boost::format("Array count exceeds defined shader count (%d/%d) for effect (%s) variable (%s).\n\n%s\nLine: %s") 
                    % arrayCount % effectTypeMapIter->second->arrayCount % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetVectorArray( effectTypeMapIter->second->handle,
                                                              pValue,
                                                              arrayCount ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue


/************************************************************************
*    desc:  Set the effect variable value
*  
*    param: string & variableStr - variable name
*           unsigned int arrayCount - number of array elements
*           D3DXMATRIX * value - value
************************************************************************/
void CShader::SetEffectValue( CEffectData * pEffectData, const string & variableStr, const uint arrayCount, D3DXMATRIX * pValue )
{
    bool result = false;

    // See if we can find variable string in the map
    effectTypeMapIter = pEffectData->spEffectTypeMap.find( variableStr );

    // If it's found, see if we can set the effect variable
    if( effectTypeMapIter != pEffectData->spEffectTypeMap.end() )
    {
        // make sure the array amount is less or equil to the max amount
        if( arrayCount > effectTypeMapIter->second->arrayCount )
            throw NExcept::CCriticalException("Shader Effect Value Error!",
                boost::str( boost::format("Array count exceeds defined shader count (%d/%d) for effect (%s) variable (%s).\n\n%s\nLine: %s") 
                    % arrayCount % effectTypeMapIter->second->arrayCount % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

        // DirectX data type specific variable function
        if( SUCCEEDED( pEffectData->spEffect->SetMatrixArray( effectTypeMapIter->second->handle,
                                                              pValue,
                                                              arrayCount ) ) )
            result = true;
    }

    if( !result )
        throw NExcept::CCriticalException("Unable to set Effect variable!",
            boost::str( boost::format("Error setting Effect (%s) variable (%s).\n\n%s\nLine: %s") % pEffectData->effectNameStr.c_str() % variableStr.c_str() % __FUNCTION__ % __LINE__ ));

}	// SetEffectValue

