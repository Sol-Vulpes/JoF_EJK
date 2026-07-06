/*
===========================================================================
rd-null : headless renderer module for EternalJK (JoF)

This renderer creates no window, no OpenGL/Vulkan context and issues no draw
calls, so it never touches the GPU. It exists so the full client (engine +
cgame + ui) can run "headless" -- connect to a server and drive chat/console
from a terminal -- with minimal impact on the machine.

It reuses the dedicated server's renderer (codemp/rd-dedicated), which already
provides real, GPU-free implementations of the ghoul2 / model / shader / skin
subsystems. rd-dedicated only fills the server half of the refexport table
(GetRefAPI_Core, compiled here with REND_NULL). This file provides the DLL
entry point GetRefAPI, wraps GetRefAPI_Core, and installs safe no-op / default
stubs for the client-facing rendering half so cgame/ui never call through a
NULL pointer.
===========================================================================
*/

#include "tr_local.h"
#include "../rd-common/tr_common.h"
#include "ghoul2/g2_local.h"

// From rd-dedicated/tr_init.cpp (compiled with REND_NULL).
extern refexport_t *GetRefAPI_Core( int apiVersion, refimport_t *rimp );

// Real, headless-safe subsystem init living in the rd-dedicated sources.
extern void R_Init( void );
extern void R_InitShaders( qboolean server );
extern void R_InitSkins( void );

// Real, headless-safe client-facing functions that already exist in rd-dedicated
// but are not wired into the server export table.
extern qhandle_t	RE_RegisterModel( const char *name );
extern qhandle_t	RE_RegisterServerModel( const char *name );
extern qhandle_t	RE_RegisterSkin( const char *name );
extern qhandle_t	RE_RegisterShader( const char *name );
extern qhandle_t	RE_RegisterShaderNoMip( const char *name );
extern const char	*RE_ShaderNameFromIndex( int index );
extern int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char *tagName );
extern void			R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

/*
================
RE_BeginRegistration_Null

Called once by the client (CL_InitRenderer). Brings up the model/shader/skin
subsystems so registration works, then hands back a plausible virtual screen
config so client/cgame scaling math never divides by zero.
================
*/
static qboolean nullRegistered = qfalse;

static void RE_BeginRegistration_Null( glconfig_t *config ) {
	if ( !nullRegistered ) {
		R_Init();				// function tables, cvars, backEndData, R_ModelInit
		R_InitShaders( qfalse );	// client-mode default shaders
		R_InitSkins();			// skin subsystem
		nullRegistered = qtrue;
	}
	tr.registered = qtrue;

	memset( config, 0, sizeof( *config ) );
	config->renderer_string		= "null";
	config->vendor_string		= "JoF headless";
	config->version_string		= "1.0";
	config->extensions_string	= "";
	config->vidWidth			= 640;
	config->vidHeight			= 480;
	config->colorBits			= 32;
	config->depthBits			= 24;
	config->stencilBits			= 8;
	config->displayFrequency	= 60;
	config->deviceSupportsGamma	= qfalse;
	config->isFullscreen		= qfalse;
	config->stereoEnabled		= qfalse;
}

// ---------------------------------------------------------------------------
// Registration / world
// ---------------------------------------------------------------------------
static void			Null_LoadWorld( const char *name ) {}
static void			Null_SetWorldVisData( const byte *vis ) {}
static void			Null_EndRegistration( void ) {}
static qboolean		Null_RegisterImages_LevelLoadEnd( void ) { return qtrue; }

// ---------------------------------------------------------------------------
// Scene building (nothing is ever rendered)
// ---------------------------------------------------------------------------
static void	Null_ClearScene( void ) {}
static void	Null_ClearDecals( void ) {}
static void	Null_AddRefEntityToScene( const refEntity_t *re ) {}
static void	Null_AddMiniRefEntityToScene( const miniRefEntity_t *re ) {}
static void	Null_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int num ) {}
static void	Null_AddDecalToScene( qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary ) {}
static int	Null_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) {
	VectorClear( ambientLight );
	VectorClear( directedLight );
	VectorSet( lightDir, 0.0f, 0.0f, 1.0f );
	return qfalse;
}
static void	Null_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {}
static void	Null_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {}
static void	Null_RenderScene( const refdef_t *fd ) {}

// ---------------------------------------------------------------------------
// 2D drawing / frame (no window, no swap)
// ---------------------------------------------------------------------------
static void	Null_SetColor( const float *rgba ) {}
static void	Null_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {}
static void	Null_DrawRotatePic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a1, qhandle_t hShader ) {}
static void	Null_DrawRotatePic2( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a1, qhandle_t hShader ) {}
static void	Null_DrawStretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty ) {}
static void	Null_UploadCinematic( int cols, int rows, const byte *data, int client, qboolean dirty ) {}
static void	Null_BeginFrame( stereoFrame_t stereoFrame ) {}
static void	Null_EndFrame( int *frontEndMsec, int *backEndMsec ) {
	if ( frontEndMsec ) *frontEndMsec = 0;
	if ( backEndMsec )  *backEndMsec = 0;
}

// ---------------------------------------------------------------------------
// Geometry queries
// ---------------------------------------------------------------------------
static int	Null_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer ) { return 0; }
static void	Null_ModelBoundsRef( refEntity_t *model, vec3_t mins, vec3_t maxs ) { VectorClear( mins ); VectorClear( maxs ); }

// ---------------------------------------------------------------------------
// Fonts / text (metrics only affect invisible layout)
// ---------------------------------------------------------------------------
static qhandle_t	Null_RegisterFont( const char *fontName ) { return 1; }
static int			Null_Font_StrLenPixels( const char *text, const int iFontIndex, const float scale ) { return 0; }
static int			Null_Font_StrLenChars( const char *text ) {
	int count = 0;
	if ( !text ) return 0;
	while ( *text ) {
		if ( Q_IsColorString( text ) ) { text += 2; continue; }
		count++;
		text++;
	}
	return count;
}
static int			Null_Font_HeightPixels( const int iFontIndex, const float scale ) { return (int)( 48.0f * scale ); }
static void			Null_Font_DrawString( int ox, int oy, const char *text, const float *rgba, const int setIndex, int iCharLimit, const float scale ) {}
static qboolean		Null_Language_IsAsian( void ) { return qfalse; }
static qboolean		Null_Language_UsesSpaces( void ) { return qtrue; }
static unsigned int	Null_AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation ) {
	if ( piAdvanceCount )			*piAdvanceCount = 1;
	if ( pbIsTrailingPunctuation )	*pbIsTrailingPunctuation = qfalse;
	return psText ? (unsigned char)psText[0] : 0;
}
static float		Null_ext_Font_StrLenPixels( const char *text, const int iFontIndex, const float scale ) { return 0.0f; }

// ---------------------------------------------------------------------------
// Misc renderer queries used by cgame/ui
// ---------------------------------------------------------------------------
static void		Null_RemapShader( const char *oldShader, const char *newShader, const char *offsetTime ) {}
static qboolean	Null_GetEntityToken( char *buffer, int size ) { if ( buffer && size > 0 ) buffer[0] = '\0'; return qfalse; }
static qboolean	Null_inPVS( const vec3_t p1, const vec3_t p2, byte *mask ) { return qtrue; }
static void		Null_GetLightStyle( int style, color4ub_t color ) { color[0] = color[1] = color[2] = color[3] = 255; }
static void		Null_SetLightStyle( int style, int color ) {}
static void		Null_GetBModelVerts( int bmodelIndex, vec3_t *vec, vec3_t normal ) { if ( normal ) VectorSet( normal, 0.0f, 0.0f, 1.0f ); }
static void		Null_SetRangedFog( float range ) {}
static void		Null_SetRefractionProperties( float distortionAlpha, float distortionStretch, qboolean distortionPrePost, qboolean distortionNegate ) {}
static float	Null_GetDistanceCull( void ) { return 10000.0f; }
static void		Null_GetRealRes( int *w, int *h ) { if ( w ) *w = 640; if ( h ) *h = 480; }
static void		Null_AutomapElevationAdjustment( float newHeight ) {}
static qboolean	Null_InitializeWireframeAutomap( void ) { return qtrue; }
static void		Null_AddWeatherZone( vec3_t mins, vec3_t maxs ) {}
static void		Null_WorldEffectCommand( const char *command ) {}
static void		Null_TakeVideoFrame( int h, int w, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg ) {}

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

The DLL entry point. Populate the ghoul2/server half via GetRefAPI_Core, then
install the client-facing half (real functions where rd-dedicated has them,
safe stubs otherwise).
@@@@@@@@@@@@@@@@@@@@@
*/
extern "C" Q_EXPORT refexport_t* QDECL GetRefAPI( int apiVersion, refimport_t *rimp ) {
	refexport_t *re = GetRefAPI_Core( apiVersion, rimp );
	if ( !re ) {
		return NULL;
	}

	// Registration -- real, GPU-free implementations from rd-dedicated.
	re->BeginRegistration		= RE_BeginRegistration_Null;
	re->RegisterModel			= RE_RegisterModel;
	re->RegisterServerModel		= RE_RegisterServerModel;
	re->RegisterSkin			= RE_RegisterSkin;
	re->RegisterShader			= RE_RegisterShader;
	re->RegisterShaderNoMip		= RE_RegisterShaderNoMip;
	re->ShaderNameFromIndex		= RE_ShaderNameFromIndex;
	re->LoadWorld				= Null_LoadWorld;
	re->SetWorldVisData			= Null_SetWorldVisData;
	re->EndRegistration			= Null_EndRegistration;
	re->RegisterImages_LevelLoadEnd	= Null_RegisterImages_LevelLoadEnd;

	// Scene.
	re->ClearScene				= Null_ClearScene;
	re->ClearDecals				= Null_ClearDecals;
	re->AddRefEntityToScene		= Null_AddRefEntityToScene;
	re->AddMiniRefEntityToScene	= Null_AddMiniRefEntityToScene;
	re->AddPolyToScene			= Null_AddPolyToScene;
	re->AddDecalToScene			= Null_AddDecalToScene;
	re->LightForPoint			= Null_LightForPoint;
	re->AddLightToScene			= Null_AddLightToScene;
	re->AddAdditiveLightToScene	= Null_AddAdditiveLightToScene;
	re->RenderScene				= Null_RenderScene;

	// 2D / frame.
	re->SetColor				= Null_SetColor;
	re->DrawStretchPic			= Null_DrawStretchPic;
	re->DrawRotatePic			= Null_DrawRotatePic;
	re->DrawRotatePic2			= Null_DrawRotatePic2;
	re->DrawStretchRaw			= Null_DrawStretchRaw;
	re->UploadCinematic			= Null_UploadCinematic;
	re->BeginFrame				= Null_BeginFrame;
	re->EndFrame				= Null_EndFrame;

	// Geometry queries.
	re->MarkFragments			= Null_MarkFragments;
	re->LerpTag					= R_LerpTag;
	re->ModelBounds				= R_ModelBounds;
	re->ModelBoundsRef			= Null_ModelBoundsRef;

	// Fonts / text.
	re->RegisterFont			= Null_RegisterFont;
	re->Font_StrLenPixels		= Null_Font_StrLenPixels;
	re->Font_StrLenChars		= Null_Font_StrLenChars;
	re->Font_HeightPixels		= Null_Font_HeightPixels;
	re->Font_DrawString			= Null_Font_DrawString;
	re->Language_IsAsian		= Null_Language_IsAsian;
	re->Language_UsesSpaces		= Null_Language_UsesSpaces;
	re->AnyLanguage_ReadCharFromString	= Null_AnyLanguage_ReadCharFromString;
	re->ext.Font_StrLenPixels	= Null_ext_Font_StrLenPixels;

	// Misc.
	re->RemapShader				= Null_RemapShader;
	re->GetEntityToken			= Null_GetEntityToken;
	re->inPVS					= Null_inPVS;
	re->GetLightStyle			= Null_GetLightStyle;
	re->SetLightStyle			= Null_SetLightStyle;
	re->GetBModelVerts			= Null_GetBModelVerts;
	re->SetRangedFog			= Null_SetRangedFog;
	re->SetRefractionProperties	= Null_SetRefractionProperties;
	re->GetDistanceCull			= Null_GetDistanceCull;
	re->GetRealRes				= Null_GetRealRes;
	re->AutomapElevationAdjustment	= Null_AutomapElevationAdjustment;
	re->InitializeWireframeAutomap	= Null_InitializeWireframeAutomap;
	re->AddWeatherZone			= Null_AddWeatherZone;
	re->WorldEffectCommand		= Null_WorldEffectCommand;
	re->TakeVideoFrame			= Null_TakeVideoFrame;

	return re;
}
