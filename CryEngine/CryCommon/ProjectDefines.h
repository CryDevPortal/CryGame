////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ProjectDefines.h
//  Version:     v1.00
//  Created:     3/30/2004 by MartinM.
//  Compilers:   Visual Studio.NET
//  Description: to get some defines available in every CryEngine project 
// -------------------------------------------------------------------------
//  History:
//    July 20th 2004 - Mathieu Pinard
//    Updated the structure to handle more easily different configurations
//
////////////////////////////////////////////////////////////////////////////

#ifndef PROJECTDEFINES_H
#define PROJECTDEFINES_H

/*
Limited SDK cuts out:
- Live create (console option in menu)
- Uses the freeSDK menu and Asset browser dialog Ribbon
- Export to engine functionality
- Trackview functionality
*/
// #define IS_LIMITED_SANDBOX 1

#ifdef _RELEASE
	#define RELEASE
#endif

#if defined(LINUX)
#	define EXCLUDE_SCALEFORM_SDK





























#elif defined(WIN32) || defined(WIN64)
# if defined(DEDICATED_SERVER)
#		define EXCLUDE_SCALEFORM_SDK
# endif // defined(DEDICATED_SERVER)
#	if !defined(_RELEASE) || defined(PERFORMANCE_BUILD)
#		define ENABLE_STATS_AGENT
#	endif
#else
#	define EXCLUDE_SCALEFORM_SDK
#endif

#define DYNTEX_USE_SHAREDRT
#ifndef EXCLUDE_CRI_SDK
#define DYNTEX_ALLOW_SFDVIDEO
#endif

// see http://confluence/display/CRYENGINE/TerrainTexCompression for more details on this
// 0=off, 1=on
#define TERRAIN_USE_CIE_COLORSPACE 0

// for consoles every bit of memory is important so files for documentation purpose are excluded
// they are part of regular compiling to verify the interface




#if defined(WIN32) || defined(WIN64) || defined(CAFE)
#define RELEASE_LOGGING
#endif

#if defined(_RELEASE) && !defined(RELEASE_LOGGING) && !defined(DEDICATED_SERVER)
#define EXCLUDE_NORMAL_LOG
#endif

#if defined(WIN32) || defined(WIN64)
#if defined(DEDICATED_SERVER)
// enable/disable map load slicing functionality from the build
#define MAP_LOADING_SLICING
#define SLICE_AND_SLEEP() do { GetISystemScheduler()->SliceAndSleep(__FUNC__, __LINE__); } while (0)
#define SLICE_SCOPE_DEFINE() CSliceLoadingMonitor sliceScope
#endif
#endif

#if !defined(MAP_LOADING_SLICING)
#define SLICE_AND_SLEEP()
#define SLICE_SCOPE_DEFINE()
#endif





#define EXCLUDE_UNIT_TESTS	0	
#ifdef _RELEASE
#undef EXCLUDE_UNIT_TESTS
#define EXCLUDE_UNIT_TESTS	1
#endif

#if ((defined(XENON) && !defined(_LIB)) || defined(CAFE) || defined(WIN32)) && !defined(RESOURCE_COMPILER)
  #define CAPTURE_REPLAY_LOG 1
#endif











#if CAPTURE_REPLAY_LOG && defined(PS3_CRYSIZER_HEAP_TRAVERSAL)
	#undef CAPTURE_REPLAY_LOG
#endif

#if defined(RESOURCE_COMPILER) || defined(_RELEASE)
  #undef CAPTURE_REPLAY_LOG
#endif

#ifndef CAPTURE_REPLAY_LOG
  #define CAPTURE_REPLAY_LOG 0
#endif

#if (defined(PS3) || defined(XENON) || defined(WIN32) || defined(GRINGO)) && !defined(PS3_CRYSIZER_HEAP_TRAVERSAL)
		#define USE_GLOBAL_BUCKET_ALLOCATOR
#endif

#if (defined(PS3) || defined(XENON)) && !defined(PS3_CRYSIZER_HEAP_TRAVERSAL)
#define USE_LEVEL_HEAP 1
#endif

#define OLD_VOICE_SYSTEM_DEPRECATED
//#define INCLUDE_PS3PAD
//#define EXCLUDE_SCALEFORM_SDK

#ifdef CRYTEK_VISUALIZATION
#define CRYTEK_SDK_EVALUATION
#endif










#if defined(RESOURCE_COMPILER) || defined(_RELEASE)
  #undef CAPTURE_REPLAY_LOG
#endif

#ifndef CAPTURE_REPLAY_LOG
  #define CAPTURE_REPLAY_LOG 0
#endif




#	define TAGES_EXPORT


// test -------------------------------------
//#define EXCLUDE_CVARHELP

#define _DATAPROBE
#if defined(_RELEASE) && !defined(XENON) && !defined(PS3) && !defined(GRINGO)
#define ENABLE_COPY_PROTECTION
#endif // #if defined(_RELEASE) && !defined(XENON) && !defined(PS3)

// GPU pass timers are enabled here for Release builds as well
// Disable them before shipping, otherwise game is linked against instrumented libraries on 360
//#ifndef PURE_XENON_RELEASE
//#define ENABLE_SIMPLE_GPU_TIMERS
//#endif








#if !defined(PHYSICS_STACK_SIZE)
# define PHYSICS_STACK_SIZE (128U<<10)
#endif 
#if !defined(EMBED_PHYSICS_AS_FIBER)
# define EMBED_PHYSICS_AS_FIBER 0
#endif 
#if EMBED_PHYSICS_AS_FIBER && !defined(EXCLUDE_PHYSICS_THREAD)
# error cannot embed physics as fiber if the physics timestep is threaded!
#endif 

#if !defined(USE_LEVEL_HEAP)
#define USE_LEVEL_HEAP 0
#endif

#if USE_LEVEL_HEAP && !defined(_RELEASE)
#define TRACK_LEVEL_HEAP_USAGE 1
#endif

#ifndef TRACK_LEVEL_HEAP_USAGE
#define TRACK_LEVEL_HEAP_USAGE 0
#endif

#if (!defined(_RELEASE) || defined(PERFORMANCE_BUILD)) && !defined(RESOURCE_COMPILER) && !defined(linux)
#ifndef ENABLE_PROFILING_CODE
	#define ENABLE_PROFILING_CODE
#endif
//lightweight profilers, disable for submissions, disables displayinfo inside 3dengine as well
#ifndef ENABLE_LW_PROFILERS
	#define ENABLE_LW_PROFILERS
#endif
#endif

#if !defined(_RELEASE)
#define USE_FRAME_PROFILER      // comment this define to remove most profiler related code in the engine
#define CRY_TRACE_HEAP
#endif


#undef ENABLE_STATOSCOPE

#ifndef IS_CRYDEV
  #if defined(ENABLE_PROFILING_CODE)
    #define ENABLE_STATOSCOPE 1
  #endif
#endif

#if defined(ENABLE_PROFILING_CODE)
  #define ENABLE_SIMPLE_GPU_TIMERS
#endif

#if defined(ENABLE_PROFILING_CODE)
  #define USE_PERFHUD
#endif

#if defined(ENABLE_STATOSCOPE) && !defined(_RELEASE)
	#define FMOD_STREAMING_DEBUGGING 1
#endif

#if (defined(PS3) || defined(XENON) || defined(CAFE)) && !defined(ENABLE_LW_PROFILERS)
#ifndef USE_NULLFONT
#define USE_NULLFONT 1
#endif
#define USE_NULLFONT_ALWAYS 1
#endif
#if defined(WIN32) && !defined(WIN64) && !defined(DEDICATED_SERVER)
	//#define OPEN_AUTOMATE
#endif





#if (defined(WIN32) || defined(WIN64) || defined(CAFE)) && !defined(_LIB)
#define CRY_ENABLE_RC_HELPER 1
#endif










#if !defined(XENON)
#define PIXBeginNamedEvent(x,y,...)
#define PIXEndNamedEvent()
#define PIXSetMarker(x,y,...)
#elif !defined(_RELEASE) && !defined(_DEBUG)
#define USE_PIX
#endif

#if !defined(_RELEASE) && !defined(PS3) && !defined(LINUX) && !defined(GRINGO) && !defined(CAFE)
	#define SOFTCODE_SYSTEM_ENABLED
#endif

// Is SoftCoding enabled for this module? Usually set by the SoftCode AddIn in conjunction with a SoftCode.props file.
#ifdef SOFTCODE_ENABLED

	// Is this current compilation unit part of a SOFTCODE build?
	#ifdef SOFTCODE
		// Import any SC functions from the host module
		#define SC_API __declspec(dllimport)
	#else
		// Export any SC functions from the host module
		#define SC_API __declspec(dllexport)
	#endif

#else	// SoftCode disabled
	
	#define SC_API

#endif

// Enable additional structures and code for sprite motion blur: 0 or 1
#define PARTICLE_MOTION_BLUR	0

// a special ticker thread to run during load and unload of levels
#define USE_NETWORK_STALL_TICKER_THREAD

#if !defined(XENON) && !defined(PS3) && !defined(CAFE)

	//---------------------------------------------------------------------
	// Enable Tessellation Stages
	// (required for displacement mapping, subdivision, water tessellation)
	//---------------------------------------------------------------------
	// Modules   : 3DEngine, Renderer
	// Depends on: DX11
	#define TESSELLATION
	#define WATER_TESSELLATION
	#define MESH_TESSELLATION
	#define MOTIONBLUR_TESSELLATION

	#ifdef TESSELLATION
		#ifdef MESH_TESSELLATION
			#define TESSELLATION_ENGINE
		#endif
		#ifdef DIRECT3D10
			#if defined(WATER_TESSELLATION) || defined(MESH_TESSELLATION)
				#define TESSELLATION_RENDERER
			#endif
		#endif
	#endif

#endif //#if !defined(XENON) && !defined(PS3)

#if defined(WIN32) || defined(WIN64)
	//#define SEG_WORLD
#endif

#include "ProjectDefinesInclude.h"

#define FULL_ON_SCHEDULING 1

#endif // PROJECTDEFINES_H
