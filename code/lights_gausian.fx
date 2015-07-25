
//-----------------------------------------------------------------------------
// #defines
//-----------------------------------------------------------------------------

// Make sure this matches in lightlst.h
#define MAX_LIGHTS          3  // 12 max for shader 2, more for shader 3
#define MAX_JOINTS          20

#define ELT_NONE            0
#define ELT_DIRECTIONAL     1
#define ELT_POINT_INFINITE  2
#define ELT_POINT_RADIUS    3
#define ELT_AMBIENT         4


//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

// joint matrix
float4x4 jointMatrix[MAX_JOINTS];

// Lighting only matrix - rigid body transform
// This matrix has no scale in it
float4x4 worldMatrix;

// Scaled world view matrix
float4x4 scaledWorldMatrix;

// camera position
float4 cameraPosition;

// Pos only camera view matrix
float4x4 cameraViewProjMatrix;

// Projection matrix from the lights view point
float4x4 lightViewProjMatrix;
float4x4 lightViewProjMatrixMinMaxBias;

// Light related globals
bool enableLights;
int lightType[MAX_LIGHTS];
float4 lightPos[MAX_LIGHTS];
float4 lightDif[MAX_LIGHTS];
float lightRadius[MAX_LIGHTS];
float4 ambient;
int lightCount;

// Object's Specular properties
float specularShine;
float specularIntensity;

// Object's color
float4 materialColor;

// texture filter
int textureFilter;

// Anisotropy rate
int anisotropyRate;

// Textures - diffuse, normal, specular and displacement
// To Do: add normal, specular and displacement support
texture diffuseTexture;
texture normalTexture;

// Texture - shadow map
texture shadowMapBufferTexture;


//-----------------------------------------------------------------------------
// Structor Definitions
//-----------------------------------------------------------------------------

struct VS_INPUT_RIGID_BODY
{
	float4 pos	  : POSITION;
	float3 normal : NORMAL;
	float2 uv0    : TEXCOORD0;
};

struct VS_INPUT_JOINT
{
	float4 pos	  : POSITION;
	float weight  : BLENDWEIGHT;
	uint jIndex   : BLENDINDICES;
	float3 normal : NORMAL;
	float2 uv0    : TEXCOORD0;
};

struct VS_INPUT_JOINT_SHADOW
{
	float4 pos	  : POSITION;
	float weight  : BLENDWEIGHT;
	uint jIndex   : BLENDINDICES;
};

struct VS_OUTPUT
{
	float4 pos    : POSITION;
	float2 uv0    : TEXCOORD0;
    float4 color  : COLOR0;
	float4 lPos   : TEXCOORD1;
};

struct VS_OUTPUT_TXTLIT
{
	float4 pos    : POSITION;
	float2 uv0    : TEXCOORD0;
	float4 lPos   : TEXCOORD1;
	float4 wPos   : TEXCOORD2;
	float4 view   : TEXCOORD3;
};

struct VS_OUTPUT_SHADOW
{
	float4 pos    : POSITION;
	float  depth  : TEXCOORD0;
};

struct PS_INPUT
{
	float2 uv0    : TEXCOORD0;
    float4 color  : COLOR0;
};

struct PS_INPUT_SHADOW
{
	float2 uv0    : TEXCOORD0;
    float4 color  : COLOR0;
	float4 lPos   : TEXCOORD1;
};

struct PS_INPUT_TXTLIT_SHADOW
{
	float2 uv0    : TEXCOORD0;
	float4 lPos   : TEXCOORD1;
	float4 wPos   : TEXCOORD2;
	float4 view   : TEXCOORD3;
};

struct COLOR_PAIR
{
    float4 diff   : COLOR0;
	float4 spec   : COLOR1;
};

struct SLight
{
	int type;    // light type
	float3 pos;  // light position
	float3 dif;  // light defuse
	float rad;   // light radius
};

struct SVert
{
	float3 pos;  // position (can also be texture pos)
	float3 nor;  // normal (can also be normal of texture position)
	float3 view; // vector of camera to vert position
};


//-----------------------------------------------------------------------------
// Texture samplers
//-----------------------------------------------------------------------------
sampler diffuseTextSampler = 
sampler_state
{
    Texture = <diffuseTexture>;
	MinFilter = (textureFilter);
    MagFilter = (textureFilter);
    MipFilter = (textureFilter);
	MaxAnisotropy = (anisotropyRate);
};

sampler normalTextSampler = 
sampler_state
{
    Texture = <normalTexture>;
	MinFilter = (textureFilter);
    MagFilter = (textureFilter);
    MipFilter = (textureFilter);
	MaxAnisotropy = (anisotropyRate);
};

sampler shadowMapBufTextSampler = 
sampler_state
{
    Texture = <shadowMapBufferTexture>;
	MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
};


//-----------------------------------------------------------------------------
// Point Radius light shader function
//
// param: wVert - vert in world view
//        light - light info
//-----------------------------------------------------------------------------
COLOR_PAIR CalcPointRadiusLight( SVert vert, SLight light )
{
	COLOR_PAIR OUT = (COLOR_PAIR)0;
	
	// Light vector
	float3 lVec = light.pos - vert.pos;
	
	// Get light distance
	float dist = sqrt(length(lVec));
	
	float3 dir = (float3)normalize( lVec );
	float diffuse = saturate(dot(dir, vert.nor)) * ((light.rad*light.rad) / (dist*dist));
	
	if( diffuse > 0.0f )
	{
		OUT.diff = float4( (light.dif * diffuse), 1.0 );

		if( specularShine > 0.001f )
		{
			float3 halfAngle = normalize( dir + vert.view );
			OUT.spec = pow( saturate( dot( vert.nor, halfAngle ) ), specularShine ) * float4( light.dif, 1.0 ) * specularIntensity * diffuse;
		}
	}

	return OUT;
}

//-----------------------------------------------------------------------------
// Directional light shader function
//
// param: wVert - vert in world view
//        light - light info
//-----------------------------------------------------------------------------
COLOR_PAIR CalcDirLight( SVert vert, SLight light )
{
	COLOR_PAIR OUT = (COLOR_PAIR)0;
	float diffuse = max(0.0, dot(light.pos, vert.nor));
	
	if( diffuse > 0.0f )
		OUT.diff = float4( light.dif * diffuse, 1.0 );

	if( specularShine > 0.001f )
	{
		float3 halfAngle = normalize( light.pos + vert.view );
		OUT.spec = pow( saturate( dot( vert.nor, halfAngle ) ), specularShine ) * float4( light.dif, 1.0 ) * specularIntensity * diffuse;
	}
	
	return OUT;
}

//-----------------------------------------------------------------------------
// Point Infinite light shader function
//
// param: vert - vert in world view
//        light - light info
//-----------------------------------------------------------------------------
COLOR_PAIR CalcPointInfiniteLight( SVert vert, SLight light )
{
	COLOR_PAIR OUT = (COLOR_PAIR)0;
	
	float3 dir = normalize( light.pos - vert.pos );
	float diffuse = saturate(dot(dir, vert.nor));
	
	if( diffuse > 0.0f )
		OUT.diff = float4( light.dif * diffuse, 1.0 );

	if( specularShine > 0.001f )
	{
		float3 halfAngle = normalize( dir + vert.view );
		OUT.spec = pow( saturate( dot( vert.nor, halfAngle ) ), specularShine ) * float4( light.dif, 1.0 ) * specularIntensity;
	}

	return OUT;
}

//-----------------------------------------------------------------------------
// function to pick the type of light to use to set the color
//
// param: wVert - vert in world view
//        light - light info
//-----------------------------------------------------------------------------
COLOR_PAIR CalcLight( SVert vert, SLight light )
{
	COLOR_PAIR OUT = (COLOR_PAIR)0;
	
	if( light.type == ELT_POINT_INFINITE )
		OUT = CalcPointInfiniteLight( vert, light );

	else if( light.type == ELT_POINT_RADIUS )
		OUT = CalcPointRadiusLight( vert, light );

	else if( light.type == ELT_DIRECTIONAL )
		OUT = CalcDirLight( vert, light );
	
	return OUT;
}

//-----------------------------------------------------------------------------
// pixel shader to compair Z depth based on shadow map buffer to 
// color or shade the pixel
//
// param: color - color of pixel based on light calculations
//        lPos - position of pixel based on shadow map camera (light) projection
//-----------------------------------------------------------------------------
float4 ShadowPixel( float4 color, float4 lPos )
{
	float4 finialColor = ambient;

	float2 projTexCoords;

	// Convert the 3d position into 2d texture coordinates
	projTexCoords[0] = lPos.x/lPos.w/2.0f + 0.5f;
	projTexCoords[1] = -lPos.y/lPos.w/2.0f + 0.5f;

	// Use saturate to see if the values are within [0, 1] range
	float2 projTexCoordsSat = saturate(projTexCoords);

	// Check if the projected x and y coordinates are within the [0, 1] range
	// If not, then they are not in view of the light
	if( (projTexCoordsSat.x == projTexCoords.x) &&
		(projTexCoordsSat.y == projTexCoords.y) &&
		(lPos.z > 0) )
	{
		// Check our value against our depth minus the "shadow bias"
		float ourDepth = lPos.z / lPos.w;

		// Get the shadow map depth value for this pixel
		float shadowMapDepth = tex2D(shadowMapBufTextSampler, projTexCoords).r;

		// Check our depth with the depth from the shadow map
		if( ourDepth < shadowMapDepth )
			finialColor = color;
	}
	else
		finialColor = color;

	return finialColor;
}

//-----------------------------------------------------------------------------
// Vertex shader for ridig bodies
//-----------------------------------------------------------------------------
VS_OUTPUT v_shaderRigidBody( VS_INPUT_RIGID_BODY IN )
{
    VS_OUTPUT OUT;
	OUT.color = ambient;

	if( enableLights )
	{
		// World view locals
		SVert vert;
		vert.pos = (float3)mul( IN.pos, scaledWorldMatrix );
		vert.nor = (float3)mul( IN.normal, (float3x3)worldMatrix );
		vert.view = normalize( (float3)cameraPosition - vert.pos );

		COLOR_PAIR color = (COLOR_PAIR)0;

		for( int i = 0; i < lightCount; i++ )
		{
			SLight light;
			light.type = lightType[i];
			light.pos = (float3)lightPos[i];
			light.dif = (float3)lightDif[i];
			light.rad = lightRadius[i];

			COLOR_PAIR colorOut = CalcLight( vert, light );
			color.diff += colorOut.diff;
			color.spec += colorOut.spec;
		}

		// build the final color
		OUT.color = saturate((color.diff + ambient + color.spec) * materialColor);
	}

	OUT.pos = mul( IN.pos, cameraViewProjMatrix );
	OUT.uv0 = IN.uv0;

	// Transform the position of the light
	OUT.lPos = mul( IN.pos, lightViewProjMatrix );

	return OUT;
}

//-----------------------------------------------------------------------------
// Vertex shader for ridig bodies
//-----------------------------------------------------------------------------
VS_OUTPUT_TXTLIT v_shaderRigidBodyTextLit( float4 pos : POSITION, float2 uv0 : TEXCOORD0 )
{
    VS_OUTPUT_TXTLIT OUT;
	
	OUT.wPos = mul( pos, scaledWorldMatrix );
	OUT.pos = mul( pos, cameraViewProjMatrix );
	OUT.view = normalize( cameraPosition - OUT.wPos );

	OUT.uv0 = uv0;

	// Transform the position of the light
	OUT.lPos = mul( pos, lightViewProjMatrix );

	return OUT;
}

//-----------------------------------------------------------------------------
// Vertex shader for joint deformation
//-----------------------------------------------------------------------------
VS_OUTPUT v_shaderJoint( VS_INPUT_JOINT IN )
{
    VS_OUTPUT OUT;
	OUT.color = ambient;

	// calculate the new position of the point and normal via deformation
	float4 pos = mul( IN.pos, jointMatrix[IN.jIndex] );
	float3 nor = mul( IN.normal, (float3x3)jointMatrix[IN.jIndex] );

	if( enableLights )
	{
		// World view locals
		SVert vert;
		vert.pos = mul( pos, scaledWorldMatrix );
		vert.nor = mul( nor, (float3x3)worldMatrix );
		vert.view = normalize( cameraPosition - vert.pos );

		COLOR_PAIR color = (COLOR_PAIR)0;

		for( int i = 0; i < lightCount; i++ )
		{
			SLight light;
			light.type = lightType[i];
			light.pos = (float3)lightPos[i];
			light.dif = (float3)lightDif[i];
			light.rad = lightRadius[i];

			COLOR_PAIR colorOut = CalcLight( vert, light );
			color.diff += colorOut.diff;
			color.spec += colorOut.spec;
		}

		// build the final color
		OUT.color = saturate((color.diff + ambient + color.spec) * materialColor);
	}

	OUT.pos = mul( pos, cameraViewProjMatrix );
	OUT.uv0 = IN.uv0;

	// Transform the position of the light
	OUT.lPos = mul( pos, lightViewProjMatrix );

	return OUT;
}

//-----------------------------------------------------------------------------
// Vertex shader for rendering ridig bodies to the shadow map buffer
//-----------------------------------------------------------------------------
VS_OUTPUT_SHADOW v_shaderRigidBodyShadow( float4 pos : POSITION )
{
	VS_OUTPUT_SHADOW OUT;

	// Transform to the light's position
	OUT.pos = mul( pos, lightViewProjMatrixMinMaxBias );

	// Depth is Z/W.  This is returned by the pixel shader.
	OUT.depth = OUT.pos.z / OUT.pos.w;

	return OUT;
}

//-----------------------------------------------------------------------------
// Vertex shader for rendering joint deformation to the shadow map buffer
//-----------------------------------------------------------------------------
VS_OUTPUT_SHADOW v_shaderJointShadow( VS_INPUT_JOINT_SHADOW IN )
{
    VS_OUTPUT_SHADOW OUT;

	// calculate the new position of the point via deformation
	float4 pos = mul( IN.pos, jointMatrix[IN.jIndex] );
	
	OUT.pos = mul( pos, lightViewProjMatrixMinMaxBias );

	// Depth is Z/W.  This is returned by the pixel shader.
	OUT.depth = OUT.pos.z / OUT.pos.w;

	return OUT;
}

//-----------------------------------------------------------------------------
// pixel shader to compair Z depth based on shadow map buffer to 
// color or shade the pixel
//-----------------------------------------------------------------------------
float4 p_shaderShadowed( PS_INPUT_SHADOW IN ) : COLOR
{
	// Make sure the lights are being used
	if( enableLights )
		IN.color = ShadowPixel( IN.color, IN.lPos );

	return tex2D( diffuseTextSampler, IN.uv0 ) * IN.color;
}

//-----------------------------------------------------------------------------
// pixel shader to compair Z depth based on shadow map buffer to 
// color or shade the pixel
//-----------------------------------------------------------------------------
float4 p_shaderShadowedTextLit( PS_INPUT_TXTLIT_SHADOW IN ) : COLOR
{
	float4 finalColor = ambient;

	// Make sure the lights are being used
	if( enableLights )
	{
		// Get the surface normal from the texture
		float3 txtNormal = normalize( tex2D( normalTextSampler, IN.uv0 ).rgb * 2.0f - 1.0f );

		SVert vert;
		vert.pos = IN.wPos;
		vert.nor = mul( txtNormal, (float3x3)worldMatrix );
		vert.view = IN.view;

		COLOR_PAIR color = (COLOR_PAIR)0;
		
		for( int i = 0; i < lightCount; i++ )
		{
			SLight light;
			light.type = lightType[i];
			light.pos = (float3)lightPos[i];
			light.dif = (float3)lightDif[i];
			light.rad = lightRadius[i];

			COLOR_PAIR colorOut = CalcLight( vert, light );
			color.diff += colorOut.diff;
			color.spec += colorOut.spec;
		}

		// build the final color
		finalColor = saturate((color.diff + ambient + color.spec) * materialColor);

		// Shadow the pixel if need be
		finalColor = ShadowPixel( finalColor, IN.lPos );
	}

	return tex2D( diffuseTextSampler, IN.uv0 ) * finalColor;
}

//-----------------------------------------------------------------------------
// pixel shader to compair Z depth based on shadow map buffer to 
// color or shade the pixel
//-----------------------------------------------------------------------------
float4 p_shaderTextLit( PS_INPUT_TXTLIT_SHADOW IN ) : COLOR
{
	float4 finalColor = ambient;

	// Make sure the lights are being used
	if( enableLights )
	{
		// Get the surface normal from the texture
		float3 txtNormal = normalize( tex2D( normalTextSampler, IN.uv0 ).rgb * 2.0f - 1.0f );

		SVert vert;
		vert.pos = IN.wPos;
		vert.nor = mul( txtNormal, (float3x3)worldMatrix );
		vert.view = IN.view;

		COLOR_PAIR color = (COLOR_PAIR)0;
		
		for( int i = 0; i < lightCount; i++ )
		{
			SLight light;
			light.type = lightType[i];
			light.pos = (float3)lightPos[i];
			light.dif = (float3)lightDif[i];
			light.rad = lightRadius[i];

			COLOR_PAIR colorOut = CalcLight( vert, light );
			color.diff += colorOut.diff;
			color.spec += colorOut.spec;
		}

		// build the final color
		finalColor = saturate((color.diff + ambient + color.spec) * materialColor);
	}

	return tex2D( diffuseTextSampler, IN.uv0 ) * finalColor;
}

//-----------------------------------------------------------------------------
// pixel shader function for rendering the lit pixel
//-----------------------------------------------------------------------------
float4 p_shader( PS_INPUT IN ) : COLOR
{
	return tex2D( diffuseTextSampler, IN.uv0 ) * IN.color;
}

//-----------------------------------------------------------------------------
// pixel shader function for rendering the pixel straight from texture
//-----------------------------------------------------------------------------
float4 p_shaderNoLight( PS_INPUT IN ) : COLOR
{
	return tex2D( diffuseTextSampler, IN.uv0 );
}

//-----------------------------------------------------------------------------
// Pixel shader for rendering to the shadow map buffer
//-----------------------------------------------------------------------------
float4 p_shader_Shadow( VS_OUTPUT_SHADOW IN ) : COLOR0
{
   // Output the scene depth
   return float4( IN.depth, IN.depth, IN.depth, 1.0f );
}


//-----------------------------------------------------------------------------
// Effects
//-----------------------------------------------------------------------------

technique rigidBodyTransform
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderRigidBody();
		PixelShader  = compile ps_3_0 p_shaderNoLight();
    }
}

technique rigidBodyLightTransform
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderRigidBody();
		PixelShader  = compile ps_3_0 p_shader();
    }
}

technique rigidBodyLightTransformWithShadow
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderRigidBody();
		PixelShader  = compile ps_3_0 p_shaderShadowed();
    }
}

technique jointBodyLightTransform
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderJoint();
		PixelShader  = compile ps_3_0 p_shader();
    }
}

technique jointBodyLightTransformWithShadow
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderJoint();
		PixelShader  = compile ps_3_0 p_shaderShadowed();
    }
}

technique rigidBodyShadowTransform
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderRigidBodyShadow();
		PixelShader  = compile ps_3_0 p_shader_Shadow();
    }
}

technique jointBodyShadowTransform
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderJointShadow();
		PixelShader  = compile ps_3_0 p_shader_Shadow();
    }
}

technique rigidBodyLightTextLightTransformWithShadow
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderRigidBodyTextLit();
		PixelShader  = compile ps_3_0 p_shaderShadowedTextLit();
    }
}

technique rigidBodyLightTextLightTransform
{
    pass Pass0
    {
		VertexShader = compile vs_3_0 v_shaderRigidBodyTextLit();
		PixelShader  = compile ps_3_0 p_shaderTextLit();
    }
}


