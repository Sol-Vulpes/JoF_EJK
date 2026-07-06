#============================================================================
# rd-null : headless (no-GPU) renderer module for EternalJK (JoF)
#
# Reuses the dedicated server's renderer sources (codemp/rd-dedicated), which
# already provide GPU-free ghoul2/model/shader/skin code, and adds the null
# client-facing stubs from tr_null.cpp. Built as a swappable renderer DLL that
# the client loads via `cl_renderer rd-null`, so the full client can run
# headless (console/chat only) with no window and no GPU usage.
#============================================================================

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPNullRendererIncludeDirectories
	${MPDir}
	${SharedDir}
	"${MPDir}/rd-dedicated"
	"${GSLIncludeDirectory}"
	${OpenJKLibDir}
	"${CMAKE_CURRENT_BINARY_DIR}/../versionfile"
	)

# Renderer core, shared verbatim with the dedicated server (GPU-free).
set(MPNullRendererFiles
	"${MPDir}/rd-dedicated/tr_local.h"
	"${MPDir}/rd-dedicated/G2_API.cpp"
	"${MPDir}/rd-dedicated/G2_bolts.cpp"
	"${MPDir}/rd-dedicated/G2_bones.cpp"
	"${MPDir}/rd-dedicated/G2_misc.cpp"
	"${MPDir}/rd-dedicated/G2_surfaces.cpp"
	"${MPDir}/rd-dedicated/tr_backend.cpp"
	"${MPDir}/rd-dedicated/tr_ghoul2.cpp"
	"${MPDir}/rd-dedicated/tr_init.cpp"
	"${MPDir}/rd-dedicated/tr_main.cpp"
	"${MPDir}/rd-dedicated/tr_mesh.cpp"
	"${MPDir}/rd-dedicated/tr_model.cpp"
	"${MPDir}/rd-dedicated/tr_shader.cpp"
	"${MPDir}/rd-dedicated/tr_skin.cpp"

	# null client-facing entry point + stubs
	"${MPDir}/rd-null/tr_null.cpp"

	# modular-renderer engine shim (Com_Printf/Error, Hunk_*, Z_* -> ri.*)
	"${MPDir}/rd-vanilla/tr_subs.cpp"

	# GL-backend stubs the renderer core references (R_IssuePendingRenderCommands,
	# RB_StageIterator*); same file the dedicated server links.
	"${MPDir}/null/null_renderer.cpp"
	)
source_group("renderer" FILES ${MPNullRendererFiles})

set(MPNullRendererGhoul2Files
	"${MPDir}/ghoul2/g2_local.h"
	"${MPDir}/ghoul2/ghoul2_shared.h"
	"${MPDir}/ghoul2/G2_gore.cpp"
	"${MPDir}/ghoul2/G2_gore.h")
source_group("ghoul2" FILES ${MPNullRendererGhoul2Files})
set(MPNullRendererFiles ${MPNullRendererFiles} ${MPNullRendererGhoul2Files})

set(MPNullRendererRdCommonFiles
	"${MPDir}/rd-common/mdx_format.h"
	"${MPDir}/rd-common/tr_common.h"
	"${MPDir}/rd-common/tr_noise.cpp"
	"${MPDir}/rd-common/tr_public.h"
	"${MPDir}/rd-common/tr_types.h")
source_group("rd-common" FILES ${MPNullRendererRdCommonFiles})
set(MPNullRendererFiles ${MPNullRendererFiles} ${MPNullRendererRdCommonFiles})

set(MPNullRendererCommonFiles
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_shared.cpp"

	${SharedCommonFiles})
source_group("common" FILES ${MPNullRendererCommonFiles})
set(MPNullRendererFiles ${MPNullRendererFiles} ${MPNullRendererCommonFiles})

set(MPNullRendererCommonSafeFiles
	${SharedCommonSafeFiles})
source_group("common/safe" FILES ${MPNullRendererCommonSafeFiles})
set(MPNullRendererFiles ${MPNullRendererFiles} ${MPNullRendererCommonSafeFiles})

add_library(${MPNullRenderer} SHARED ${MPNullRendererFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPNullRenderer} PROPERTIES PREFIX "")
endif()

if(WIN32)
	install(TARGETS ${MPNullRenderer}
		RUNTIME
		DESTINATION ${JKAInstallDir}
		COMPONENT ${JKAMPClientComponent})
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPNullRenderer}
			LIBRARY
			DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/"
			COMPONENT ${JKAMPClientComponent})
	else()
		install(TARGETS ${MPNullRenderer}
			LIBRARY
			DESTINATION ${JKAInstallDir}
			COMPONENT ${JKAMPClientComponent})
	endif()
endif()

# SharedDefines + REND_NULL (selects GetRefAPI_Core in rd-dedicated/tr_init.cpp)
set_target_properties(${MPNullRenderer} PROPERTIES COMPILE_DEFINITIONS "${SharedDefines};REND_NULL")

# Hide symbols not explicitly marked public.
set_property(TARGET ${MPNullRenderer} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

set_target_properties(${MPNullRenderer} PROPERTIES INCLUDE_DIRECTORIES "${MPNullRendererIncludeDirectories}")
set_target_properties(${MPNullRenderer} PROPERTIES PROJECT_LABEL "MP Null Renderer")

if(NOT MSVC)
	# https://github.com/JACoders/OpenJK/issues/1033
	target_link_libraries(${MPNullRenderer} ${CMAKE_DL_LIBS})
endif()
