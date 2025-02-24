// R_draw.c

#include "DoomDef.h"
#include "R_local.h"

/*

All drawing to the view buffer is accomplished in this file.  The other refresh
files only know about ccordinates, not the architecture of the frame buffer.

*/

byte *viewimage;
int viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;
byte *ylookup[MAXHEIGHT];
int columnofs[MAXWIDTH];
byte translations[3][256]; // color tables for different players
byte *tinttable; // used for translucent sprites

/*
==================
=
= R_DrawColumn
=
= Source is the top of the column to scale
=
==================
*/

lighttable_t	*dc_colormap;
int				dc_x;
int				dc_yl;
int				dc_yh;
fixed_t			dc_iscale;
fixed_t			dc_texturemid;
byte			*dc_source;		// first pixel in a column (possibly virtual)

int				dccount;		// just for profiling

void R_DrawColumn (void)
{
	int			count;
	byte		*dest;
	fixed_t		frac, fracstep;	

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
				
#ifdef RANGECHECK
	if ((unsigned)dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
		I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

	dest = ylookup[dc_yl] + columnofs[dc_x]; 
	
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl-centery)*fracstep;

	do
	{
		*dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
		dest += SCREENWIDTH;
		frac += fracstep;
	} while (count--);
}

#define FUZZTABLE	50

#define FUZZOFF	(SCREENWIDTH)
int		fuzzoffset[FUZZTABLE] = {
FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};
int fuzzpos = 0;

void R_DrawFuzzColumn (void)
{
	int			count;
	byte		*dest;
	fixed_t		frac, fracstep;	

	if (!dc_yl)
		dc_yl = 1;
	if (dc_yh == viewheight-1)
		dc_yh = viewheight - 2;
		
	count = dc_yh - dc_yl;
	if (count < 0)
		return;
				
#ifdef RANGECHECK
	if ((unsigned)dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
		I_Error ("R_DrawFuzzColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

	dest = ylookup[dc_yl] + columnofs[dc_x];

	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl-centery)*fracstep;

// OLD FUZZY INVISO SPRITE STUFF
/*	do
	{
		*dest = colormaps[6*256+dest[fuzzoffset[fuzzpos]]];
		if (++fuzzpos == FUZZTABLE)
			fuzzpos = 0;
		dest += SCREENWIDTH;
		frac += fracstep;
	} while (count--);
*/

	do
	{
		*dest = tinttable[((*dest)<<8)+dc_colormap[dc_source[(frac>>FRACBITS)&127]]];

		//*dest = dest[SCREENWIDTH*10+5];

//		*dest = //tinttable[((*dest)<<8)+colormaps[dc_source[(frac>>FRACBITS)&127]]];

//		*dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
		dest += SCREENWIDTH;
		frac += fracstep;
	} while(count--);
}

/*
========================
=
= R_DrawTranslatedColumn
=
========================
*/

byte *dc_translation;
byte *translationtables;

void R_DrawTranslatedColumn (void)
{
	int			count;
	byte		*dest;
	fixed_t		frac, fracstep;	

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
				
#ifdef RANGECHECK
	if ((unsigned)dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
		I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

	dest = ylookup[dc_yl] + columnofs[dc_x];
	
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl-centery)*fracstep;

	do
	{
		*dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
		dest += SCREENWIDTH;
		frac += fracstep;
	} while (count--);
}

void R_DrawTranslatedFuzzColumn (void)
{
	int			count;
	byte		*dest;
	fixed_t		frac, fracstep;	

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
				
#ifdef RANGECHECK
	if ((unsigned)dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
		I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

	dest = ylookup[dc_yl] + columnofs[dc_x];
	
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl-centery)*fracstep;

	do
	{
		*dest = tinttable[((*dest)<<8)
			+dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]];
		dest += SCREENWIDTH;
		frac += fracstep;
	} while (count--);
}

//--------------------------------------------------------------------------
//
// PROC R_InitTranslationTables
//
//--------------------------------------------------------------------------

void R_InitTranslationTables (void)
{
	int i;

	// Load tint table
	tinttable = W_CacheLumpName("TINTTAB", PU_STATIC);

	// Allocate translation tables
	translationtables = Z_Malloc(256*3+255, PU_STATIC, 0);
	translationtables = (byte *)(( (int)translationtables + 255 )& ~255);

	// Fill out the translation tables
	for(i = 0; i < 256; i++)
	{
		if(i >= 225 && i <= 240)
		{
			translationtables[i] = 114+(i-225); // yellow
			translationtables[i+256] = 145+(i-225); // red
			translationtables[i+512] = 190+(i-225); // blue
		}
		else
		{
			translationtables[i] = translationtables[i+256] 
			= translationtables[i+512] = i;
		}
	}
}

/*
================
=
= R_DrawSpan
=
================
*/

int				ds_y;
int				ds_x1;
int				ds_x2;
lighttable_t	*ds_colormap;
fixed_t			ds_xfrac;
fixed_t			ds_yfrac;
fixed_t			ds_xstep;
fixed_t			ds_ystep;
byte			*ds_source;		// start of a 64*64 tile image

int				dscount;		// just for profiling

void R_DrawSpan (void)
{
	fixed_t		xfrac, yfrac;
	byte		*dest;
	int			count, spot;
	
#ifdef RANGECHECK
	if (ds_x2 < ds_x1 || ds_x1<0 || ds_x2>=SCREENWIDTH 
	|| (unsigned)ds_y>SCREENHEIGHT)
		I_Error ("R_DrawSpan: %i to %i at %i",ds_x1,ds_x2,ds_y);
//	dscount++;
#endif
	
	xfrac = ds_xfrac;
	yfrac = ds_yfrac;
	
	dest = ylookup[ds_y] + columnofs[ds_x1];	
	count = ds_x2 - ds_x1;
	do
	{
		spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
		*dest++ = ds_colormap[ds_source[spot]];
		xfrac += ds_xstep;
		yfrac += ds_ystep;
	} while (count--);
}


/*
================
=
= R_InitBuffer
=
=================
*/

void R_InitBuffer (int width, int height)
{
	int		i;
	
	viewwindowx = (SCREENWIDTH-width) >> 1;
	for (i=0 ; i<width ; i++)
		columnofs[i] = viewwindowx + i;
	if (width == SCREENWIDTH)
		viewwindowy = 0;
	else
		viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1;
	for (i=0 ; i<height ; i++)
		ylookup[i] = screen + (i+viewwindowy)*SCREENWIDTH;
}


/*
==================
=
= R_DrawViewBorder
=
= Draws the border around the view for different size windows
==================
*/

boolean BorderNeedRefresh;

void R_DrawViewBorder (void)
{
	byte	*src, *dest;
	int		x,y;
	
	if (scaledviewwidth == SCREENWIDTH)
		return;

	if(shareware)
	{
		src = W_CacheLumpName ("FLOOR04", PU_CACHE);
	}
	else
	{
		src = W_CacheLumpName ("FLAT513", PU_CACHE);
	}
	dest = screen;
	
	for (y=0 ; y<SCREENHEIGHT-SBARHEIGHT ; y++)
	{
		for (x=0 ; x<SCREENWIDTH/64 ; x++)
		{
			memcpy (dest, src+((y&63)<<6), 64);
			dest += 64;
		}
		if (SCREENWIDTH&63)
		{
			memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
			dest += (SCREENWIDTH&63);
		}
	}
	for(x=viewwindowx; x < viewwindowx+viewwidth; x += 16)
	{
		V_DrawPatch(x, viewwindowy-4, W_CacheLumpName("bordt", PU_CACHE));
		V_DrawPatch(x, viewwindowy+viewheight, W_CacheLumpName("bordb", 
			PU_CACHE));
	}
	for(y=viewwindowy; y < viewwindowy+viewheight; y += 16)
	{
		V_DrawPatch(viewwindowx-4, y, W_CacheLumpName("bordl", PU_CACHE));
		V_DrawPatch(viewwindowx+viewwidth, y, W_CacheLumpName("bordr", 
			PU_CACHE));
	}
	V_DrawPatch(viewwindowx-4, viewwindowy-4, W_CacheLumpName("bordtl", 
		PU_CACHE));
	V_DrawPatch(viewwindowx+viewwidth, viewwindowy-4, 
		W_CacheLumpName("bordtr", PU_CACHE));
	V_DrawPatch(viewwindowx+viewwidth, viewwindowy+viewheight, 
		W_CacheLumpName("bordbr", PU_CACHE));
	V_DrawPatch(viewwindowx-4, viewwindowy+viewheight, 
		W_CacheLumpName("bordbl", PU_CACHE));
}

/*
==================
=
= R_DrawTopBorder
=
= Draws the top border around the view for different size windows
==================
*/

boolean BorderTopRefresh;

void R_DrawTopBorder (void)
{
	byte	*src, *dest;
	int		x,y;
	
	if (scaledviewwidth == SCREENWIDTH)
		return;

	if(shareware)
	{
		src = W_CacheLumpName ("FLOOR04", PU_CACHE);
	}
	else
	{
		src = W_CacheLumpName ("FLAT513", PU_CACHE);
	}
	dest = screen;
	
	for (y=0 ; y<30 ; y++)
	{
		for (x=0 ; x<SCREENWIDTH/64 ; x++)
		{
			memcpy (dest, src+((y&63)<<6), 64);
			dest += 64;
		}
		if (SCREENWIDTH&63)
		{
			memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
			dest += (SCREENWIDTH&63);
		}
	}
	if(viewwindowy < 25)
	{
		for(x=viewwindowx; x < viewwindowx+viewwidth; x += 16)
		{
			V_DrawPatch(x, viewwindowy-4, W_CacheLumpName("bordt", PU_CACHE));
		}
		V_DrawPatch(viewwindowx-4, viewwindowy, W_CacheLumpName("bordl", 
			PU_CACHE));
		V_DrawPatch(viewwindowx+viewwidth, viewwindowy, 
			W_CacheLumpName("bordr", PU_CACHE));
		V_DrawPatch(viewwindowx-4, viewwindowy+16, W_CacheLumpName("bordl", 
			PU_CACHE));
		V_DrawPatch(viewwindowx+viewwidth, viewwindowy+16, 
			W_CacheLumpName("bordr", PU_CACHE));

		V_DrawPatch(viewwindowx-4, viewwindowy-4, W_CacheLumpName("bordtl", 
			PU_CACHE));
		V_DrawPatch(viewwindowx+viewwidth, viewwindowy-4, 
			W_CacheLumpName("bordtr", PU_CACHE));
	}
}


