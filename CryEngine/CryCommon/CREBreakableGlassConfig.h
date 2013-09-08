#ifndef _CRE_BREAKABLE_GLASS_CONFIG_
#define _CRE_BREAKABLE_GLASS_CONFIG_
#pragma once

//==================================================================================================
// Name: CREBreakableGlassConfig
// Desc: Breakable glass defines and platform-specific configuration
// Author: Chris Bunner
//==================================================================================================

// Glass material
#define GLASSCFG_PLANE_TOUGHNESS								1.0f																		/* Direct scale for impulse needed to cause a break */
#define GLASSCFG_PLANE_SHATTER_IMPULSE					(8000.0f * GLASSCFG_PLANE_TOUGHNESS)		/* Impulse threshold that will instantly shatter plane */
#define GLASSCFG_PLANE_SPLIT_IMPULSE						(280.0f * GLASSCFG_PLANE_TOUGHNESS)			/* Min impulse to cause a physical hole in the mesh */
#define GLASSCFG_PLANE_SPLIT_IMPULSE_STRENGTH		(6.5f / GLASSCFG_PLANE_SHATTER_IMPULSE)	/* Direct scale for size of physical mesh holes */

// Hash grid size
#define GLASSCFG_HASH_GRID_SIZE						8				/* Optimizations rely on power of two, so check usage if changing */
#define GLASSCFG_HASH_GRID_BUCKET_SIZE		16			/* Should be max number of overlapping triangles in any given cell */

// Common
#define GLASSCFG_MIN_BULLET_SPEED					400.0f	/* Anything above this speed will be considered a bullet (Generally 450 to 1100)*/
#define GLASSCFG_MIN_BULLET_HOLE_SCALE		1.1f		/* Minimum size for physical hole created by bullets */
#define GLASSCFG_MIN_STABLE_FRAG_AREA			0.0095f /* Any fragment sized below this threshold will be forced loose */
#define GLASSCFG_MIN_VALID_FRAG_AREA			0.0035f /* Any fragment sized below this threshold will not be created */

// Drawing
#define GLASSCFG_GLASS_PLANE_FLAG_LOD			255			/* Fixed number used to flag if render call is plane or fragment */

// X360/PS3 detail scaling






















#define GLASSCFG_MIN_NUM_RADIAL_CRACKS		3
#define GLASSCFG_MAX_NUM_RADIAL_CRACKS		6
#define GLASSCFG_MAX_NUM_CRACK_POINTS			64

#define GLASSCFG_SIMPLIFY_CRACKS					4.5f
#define GLASSCFG_SIMPLIFY_AREA						1.0f
#define GLASSCFG_SHATTER_ON_EXPLOSION			0
#define GLASSCFG_SPLIT_ON_EXPLOSION				0

#define	GLASSCFG_MAX_NUM_IMPACTS					12
#define GLASSCFG_MAX_NUM_IMPACT_DECALS		8
#define GLASSCFG_MAX_NUM_STABLE_FRAGMENTS	41
#define	GLASSCFG_MAX_NUM_FRAGMENTS				64
#define GLASSCFG_FRAGMENT_ARRAY_SIZE			28

#define GLASSCFG_MAX_NUM_ACTIVE_GLASS			20
#define GLASSCFG_MAX_NUM_PHYS_FRAGMENTS		10
#define	GLASSCFG_MAX_NUM_PLANE_VERTS			512


// Geometry buffer sizes
#define	GLASSCFG_MAX_NUM_CRACK_VERTS			(GLASSCFG_MAX_NUM_PLANE_VERTS*2)
#define	GLASSCFG_MAX_NUM_PLANE_INDS				(GLASSCFG_MAX_NUM_PLANE_VERTS*3)
#define	GLASSCFG_MAX_NUM_CRACK_INDS				(GLASSCFG_MAX_NUM_CRACK_VERTS*6)

#endif // _CRE_BREAKABLE_GLASS_CONFIG_