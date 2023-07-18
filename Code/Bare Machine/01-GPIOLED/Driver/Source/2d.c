/**************************************************************************//**
* @file     2d.c
* @version  V1.00
* $Revision: 1 $
* $Date: 15/05/27 5:21p $
* @brief    NUC970 GE2D driver source file
*
* @note
* Copyright (C) 2015 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nuc970.h"
#include "sys.h"
#include "2d.h"

/** @addtogroup NUC970_Device_Driver NUC970 Device Driver
  @{
*/

/** @addtogroup NUC970_GE2D_Driver GE2D Driver
  @{
*/

/** @addtogroup NUC970_GE2D_EXPORTED_CONSTANTS GE2D Exported Constants
  @{
*/

/// @cond HIDDEN_SYMBOLS
static unsigned int GFX_BPP;
static unsigned int GFX_WIDTH;
static unsigned int GFX_HEIGHT;
static unsigned int GFX_PITCH;
static unsigned int GFX_SIZE;

static __align(32) void *GFX_START_ADDR;
static __align(32) void *MONO_SOURCE_ADDR;
static __align(32) void *COLOR_SOURCE_ADDR;
static __align(32) void *CMODEL_START_ADDR;
static __align(32) void *GFX_OFFSCREEN_ADDR;
static __align(32) void *GFX_PAT_ADDR;

static void *Orig_GFX_START_ADDR;
static void *Orig_MONO_SOURCE_ADDR;
static void *Orig_COLOR_SOURCE_ADDR;
static void *Orig_CMODEL_START_ADDR;
static void *Orig_GFX_OFFSCREEN_ADDR;
static void *Orig_GFX_PAT_ADDR;

#define  PN   1  // Quadrant 1
#define  NN   2  // Quadrant 2
#define  NP   3  // Quadrant 3
#define  PP   4  // Quadrant 4

#define ABS(x)      (((x)>0)?(x):-(x))
#define MAX(a,b)    (((a)>(b))?(a):(b))

/* octant code of line drawing */

#define XpYpXl      (0<<1)   // XY octant position is 1~3 in Control register
#define XpYpYl      (1<<1)
#define XpYmXl      (2<<1)
#define XpYmYl      (3<<1)
#define XmYpXl      (4<<1)
#define XmYpYl      (5<<1)
#define XmYmXl      (6<<1)
#define XmYmYl      (7<<1)

static MONOPATTERN MonoPatternData[6] = {
    {0x00000000, 0xff000000}, // HS_HORIZONTAL
    {0x08080808, 0x08080808}, // HS_VERTICAL
    {0x80402010, 0x08040201}, // HS_FDIAGONAL
    {0x01020408, 0x10204080}, // HS_BDIAGONAL
    {0x08080808, 0xff080808}, // HS_CROSS
    {0x81422418, 0x18244281}  // HS_DIAGCROSS
};

static char _DrawMode = MODE_OPAQUE;
static UINT32 _ColorKey;
static UINT32 _ColorKeyMask;

static BOOL _EnableAlpha = FALSE;
static int _AlphaKs, _AlphaKd;
static BOOL _ClipEnable = FALSE;
static BOOL _OutsideClip = FALSE;
static UINT32 _ClipTL, _ClipBR;
static int _PatternType;

/// @endcond /* HIDDEN_SYMBOLS */

/*@}*/ /* end of group NUC970_GE2D_EXPORTED_CONSTANTS */

/** @addtogroup NUC970_GE2D_EXPORTED_FUNCTIONS GE2D Exported Functions
  @{
*/

/// @cond HIDDEN_SYMBOLS

/* For align 32 */
static unsigned int shift_pointer(int ptr, int align)
{
    unsigned int pos;
    int remain;
    pos = ptr;

    if( (ptr%align)!=0) {
        remain = ptr % align;
        ptr = ptr + (align - remain);
        return ptr;
    } else
        return pos;
}

static unsigned long make_color(int color)
{
    UINT32 r, g, b;

    if (GFX_BPP==8) {
        r = (color & 0x00e00000) >> 16; // 3 bits
        g = (color & 0x0000e000) >> 11; // 3 bits
        b = (color & 0x000000c0) >> 6;  // 2 bits
        return (r | g | b);
    } else if (GFX_BPP==16) {
        r = (color & 0x00f80000) >> 8; // 5 bits
        g = (color & 0x0000fc00) >> 5; // 6 bits
        b = (color & 0x000000f8) >> 3; // 5 bits
        return (r | g | b);
    } else return (UINT32)color;
}
/// @endcond /* HIDDEN_SYMBOLS */

/**
  * @brief  Clear the on screen buffer with a specified color.
  * @param[in] color clear with this color.
  * @return none
  */
void ge2dClearScreen(int color)
{
    UINT32 cmd32;
    UINT32 color32, dest_pitch, dest_dimension;

    color32 = make_color(color);

    cmd32 = 0xcc430040;
    outpw(REG_GE2D_CTL, cmd32);
    outpw(REG_GE2D_BGCOLR, color32); // fill with background color

    dest_pitch = GFX_WIDTH << 16; // pitch in pixels
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    outpw(REG_GE2D_DSTSPA, 0); // starts from (0,0)

    dest_dimension = GFX_HEIGHT << 16 | GFX_WIDTH;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Set output data mask.
  * @param[in] mask is mask value
  * @return none
  */
void ge2dSetWriteMask(int mask)
{
    outpw(REG_GE2D_WRPLNMSK, make_color(mask));
}

/**
  * @brief Set source origin starting address.
  * @param[in] ptr pointer of start address
  * @return none
  */
void ge2dSetSourceOriginStarting(void *ptr)
{
    outpw(REG_GE2D_XYSORG, (int)ptr);
}

/**
  * @brief Set destination origin starting address.
  * @param[in] ptr pointer of start address
  * @return none
  */
void ge2dSetDestinationOriginStarting(void *ptr)
{
    outpw(REG_GE2D_XYDORG, (int)ptr);
}

/**
  * @brief Graphics engine initialization.
  * @param[in] bpp bit per pixel
  * @param[in] width is width of display memory
  * @param[in] height is height of display memory
  * @param[in] destination is pointer of destination buffer address
  * @return none
  */
void ge2dInit(int bpp, int width, int height, void *destination)
{
    UINT32 data32;

    GFX_BPP = bpp;
    GFX_WIDTH = width;
    GFX_HEIGHT = height;
    GFX_PITCH = (GFX_WIDTH*(GFX_BPP/8));
    GFX_SIZE = (GFX_HEIGHT*GFX_PITCH);

    if(destination == NULL)
        return;

    Orig_GFX_START_ADDR = GFX_START_ADDR = (void *)destination;

    Orig_GFX_PAT_ADDR = GFX_PAT_ADDR = (void *)malloc((8*8*(GFX_BPP/8))*2);
    GFX_PAT_ADDR = (void *)shift_pointer((int)GFX_PAT_ADDR, (8*8*(GFX_BPP/8))*2);  // two times of boundary size


    Orig_MONO_SOURCE_ADDR = MONO_SOURCE_ADDR = (void *)malloc(GFX_SIZE+32);
    MONO_SOURCE_ADDR = (void *)shift_pointer((int)MONO_SOURCE_ADDR, 32);
    Orig_COLOR_SOURCE_ADDR = COLOR_SOURCE_ADDR = (void *)malloc(GFX_SIZE+32);
    COLOR_SOURCE_ADDR = (void *)shift_pointer((int)COLOR_SOURCE_ADDR, 32);
    Orig_CMODEL_START_ADDR = CMODEL_START_ADDR = (void *)malloc(GFX_SIZE+32);
    CMODEL_START_ADDR = (void *)shift_pointer((int)CMODEL_START_ADDR, 32);
    Orig_GFX_OFFSCREEN_ADDR = GFX_OFFSCREEN_ADDR = (void *)malloc(GFX_SIZE+32);
    GFX_OFFSCREEN_ADDR = (void *)shift_pointer((int)GFX_OFFSCREEN_ADDR, 32);

#ifdef DEBUG
    sysprintf("init_GE()\n");
    sysprintf("screen width = %d\n", GFX_WIDTH);
    sysprintf("screen height = %d\n", GFX_HEIGHT);
    sysprintf("screen bpp = %d\n", GFX_BPP);
    sysprintf("screen size = 0x%x\n", GFX_SIZE);
#endif

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<28));

    outpw(REG_GE2D_INTSTS, 0); // clear interrupt
    outpw(REG_GE2D_PATSA, (unsigned int)GFX_PAT_ADDR);
    outpw(REG_GE2D_CTL, 0); // disable interrupt
    outpw(REG_GE2D_XYDORG, (unsigned int)GFX_START_ADDR);
    outpw(REG_GE2D_XYSORG, (unsigned int)GFX_START_ADDR);

    outpw(REG_GE2D_WRPLNMSK, 0x00ffffff); // write plane mask

    data32 = GE_BPP_8; // default is 8 bpp

    if (GFX_BPP==16) {
        data32 |= GE_BPP_16;
    } else if (GFX_BPP==32) {
        data32 |= GE_BPP_32;
    }

    outpw(REG_GE2D_MISCTL, data32);
}

/**
  * @brief Reset graphics engine.
  * @param none
  * @return none
  */
void ge2dReset(void)
{
    outpw(REG_GE2D_MISCTL, 0x40); // FIFO reset
    outpw(REG_GE2D_MISCTL, 0x00);

    outpw(REG_GE2D_MISCTL, 0x80); // Engine reset
    outpw(REG_GE2D_MISCTL, 0x00);

    free(Orig_GFX_START_ADDR);
    free(Orig_COLOR_SOURCE_ADDR);
    free(Orig_CMODEL_START_ADDR);
    free(Orig_GFX_OFFSCREEN_ADDR);
    free(Orig_GFX_PAT_ADDR);
    free(Orig_MONO_SOURCE_ADDR);
}

/**
  * @brief Reset FIFO of graphics engine.
  * @param none
  * @return none
  */
void ge2dResetFIFO(void)
{
    UINT32 temp32;

    temp32 = inpw(REG_GE2D_MISCTL);
    temp32 |= 0x00000040;
    outpw(REG_GE2D_MISCTL, temp32);
    temp32 &= 0xffffffbf;
    outpw(REG_GE2D_MISCTL, temp32);
}

/**
  * @brief Set BitBlt drawing mode.
  * @param[in] opt is drawing mode
  * @param[in] ckey is value of color key
  * @param[in] mask is value of color mask
  * @return none
  */
void ge2dBitblt_SetDrawMode(int opt, int ckey, int mask)
{
    if (opt==MODE_TRANSPARENT) {
        _DrawMode = MODE_TRANSPARENT;

        _ColorKey = make_color(ckey);
        _ColorKeyMask = make_color(mask);

        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    } else if (opt==MODE_DEST_TRANSPARENT) {
        _DrawMode = MODE_DEST_TRANSPARENT;

        _ColorKey = make_color(ckey);
        _ColorKeyMask = make_color(mask);

        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    } else {
        _DrawMode = MODE_OPAQUE; // default is OPAQUE
    }
}

/**
  * @brief Set alpha blending programming.
  * @param[in] opt is selection for enable or disable
  * @param[in] ks is value of alpha blending factor Ks
  * @param[in] kd is value of alpha blending factor Kd
  * @return none
  */
int ge2dBitblt_SetAlphaMode(int opt, int ks, int kd)
{
    if (ks + kd > 255)
        return -1;

    if (opt==1) {
        _EnableAlpha = TRUE;
        _AlphaKs = ks;
        _AlphaKd = kd;
    } else {
        _EnableAlpha = FALSE;
    }

    return 0;
}

/**
  * @brief Screen-to-Screen BitBlt with SRCCOPY ROP operation.
  * @param[in] srcx is source x position
  * @param[in] srcy is source y position
  * @param[in] destx is destination x position
  * @param[in] desty is destination y position
  * @param[in] width is display width
  * @param[in] height is display width
  * @return none
  */
void ge2dBitblt_ScreenToScreen(int srcx, int srcy, int destx, int desty, int width, int height)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;

#ifdef DEBUG
    sysprintf("screen_to_screen_blt():\n");
    sysprintf("(%d,%d)=>(%d,%d)\n", srcx, srcy, destx, desty);
    sysprintf("width=%d height=%d\n", width, height);
#endif

    cmd32 = 0xcc430000;

    outpw(REG_GE2D_CTL, cmd32);

    if (srcx > destx) { //+X
        if (srcy > desty) { //+Y
        } else { //-Y
            cmd32 |= 0x08;
            srcy = srcy + height - 1;
            desty = desty + height - 1;
        }
    } else { //-X
        if (srcy > desty) { //+Y
            cmd32 |= 0x04; // 010
            srcx = srcx + width - 1;
            destx = destx + width - 1;
        } else { //-Y
            cmd32 |= 0xc; // 110
            srcx = srcx + width - 1;
            destx = destx + width - 1;
            srcy = srcy + height - 1;
            desty = desty + height - 1;
        }
    }

#ifdef DEBUG
    sysprintf("new srcx=%d srcy=%d\n", srcx, srcy);
    sysprintf("new destx=%d desty=%d\n", destx, desty);
#endif

    outpw(REG_GE2D_CTL, cmd32);

    pitch = GFX_WIDTH << 16 | GFX_WIDTH;
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = srcy << 16 | srcx;
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = desty << 16 | destx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    //
    // force to use the same starting address
    //
    outpw(REG_GE2D_XYSORG, (int)GFX_START_ADDR);
    outpw(REG_GE2D_XYDORG, (int)GFX_START_ADDR);  //smf

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    if (_DrawMode==MODE_TRANSPARENT) {
        cmd32 |= 0x00008000; // color transparency
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    } else if (_DrawMode==MODE_DEST_TRANSPARENT) {
        cmd32 |= 0x00009000;   // destination pixels control transparency
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    }

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Screen-to-Screen BitBlt with ROP option.
  * @param[in] srcx is source x position
  * @param[in] srcy is source y position
  * @param[in] destx is destination x position
  * @param[in] desty is destination y position
  * @param[in] width is display width
  * @param[in] height is display width
  * @param[in] rop is rop option
  * @return none
  */
void ge2dBitblt_ScreenToScreenRop(int srcx, int srcy, int destx, int desty, int width, int height, int rop)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;

#ifdef DEBUG
    sysprintf("screen_to_screen_rop_blt():\n");
    sysprintf("ROP=0x%x\n", rop);
    sysprintf("(%d,%d)=>(%d,%d)\n", srcx, srcy, destx, desty);
    sysprintf("width=%d height=%d\n", width, height);
#endif

    cmd32 = 0x00430000 | (rop << 24);

    if (_PatternType==TYPE_MONO) {
        cmd32 |= 0x00000010; // default is TYPE_COLOR
    }

    outpw(REG_GE2D_CTL, cmd32);

    if (srcx > destx) { //+X
        if (srcy > desty) { //+Y
        } else { //-Y
            cmd32 |= 0x08;
            srcy = srcy + height - 1;
            desty = desty + height - 1;
        }
    } else { //-X
        if (srcy > desty) { //+Y
            cmd32 |= 0x04; // 010
            srcx = srcx + width - 1;
            destx = destx + width - 1;
        } else { //-Y
            cmd32 |= 0xc; // 110
            srcx = srcx + width - 1;
            destx = destx + width - 1;
            srcy = srcy + height - 1;
            desty = desty + height - 1;
        }
    }

#ifdef DEBUG
    sysprintf("new srcx=%d srcy=%d\n", srcx, srcy);
    sysprintf("new destx=%d desty=%d\n", destx, desty);
#endif

    outpw(REG_GE2D_CTL, cmd32);

    pitch = GFX_WIDTH << 16 | GFX_WIDTH; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = srcy << 16 | srcx;
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = desty << 16 | destx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    //
    // force to use the same starting address
    //
    outpw(REG_GE2D_XYSORG, (int)GFX_START_ADDR);
    outpw(REG_GE2D_XYDORG, (int)GFX_START_ADDR);  //smf

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    if (_DrawMode==MODE_TRANSPARENT) {
        cmd32 |= 0x00008000; // color transparency
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    } else if (_DrawMode==MODE_DEST_TRANSPARENT) {
        cmd32 |= 0x00009000;
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    }

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    if ((rop==0x00) || (rop==0xff)) {
        cmd32 = (cmd32 & 0xffff0fff) | 0x00009000;
        outpw(REG_GE2D_CTL, cmd32);
    }

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Source to destination BitBlt with SRCCOPY ROP operation.
  * @param[in] srcx is source x position
  * @param[in] srcy is source y position
  * @param[in] destx is destination x position
  * @param[in] desty is destination y position
  * @param[in] width is display width
  * @param[in] height is display width
  * @param[in] srcpitch is source pixel pitch
  * @param[in] destpitch is destination pixel pitch
  * @return none
  * @note before calling this function, it would set the source and destination origin starting place
  */
void ge2dBitblt_SourceToDestination(int srcx, int srcy, int destx, int desty, int width, int height, int srcpitch, int destpitch)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;

#ifdef DEBUG
    sysprintf("source_to_destination_blt():\n");
    sysprintf("(%d,%d)=>(%d,%d)\n", srcx, srcy, destx, desty);
    sysprintf("width=%d height=%d\n", width, height);
#endif

    cmd32 = 0xcc430000;

    outpw(REG_GE2D_CTL, cmd32);

    if (srcx > destx) { //+X
        if (srcy > desty) { //+Y
        } else { //-Y
            cmd32 |= 0x08;
            srcy = srcy + height - 1;
            desty = desty + height - 1;
        }
    } else { //-X
        if (srcy > desty) { //+Y
            cmd32 |= 0x04; // 010
            srcx = srcx + width - 1;
            destx = destx + width - 1;
        } else { //-Y
            cmd32 |= 0xc; // 110
            srcx = srcx + width - 1;
            destx = destx + width - 1;
            srcy = srcy + height - 1;
            desty = desty + height - 1;
        }
    }

#ifdef DEBUG
    sysprintf("new srcx=%d srcy=%d\n", srcx, srcy);
    sysprintf("new destx=%d desty=%d\n", destx, desty);
#endif

    outpw(REG_GE2D_CTL, cmd32);

    pitch = destpitch << 16 | srcpitch; // pitch in pixel, back | GFX_WIDTH ??
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = srcy << 16 | srcx;
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = desty << 16 | destx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dimension);


    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    if (_DrawMode==MODE_TRANSPARENT) {
        cmd32 |= 0x00008000; // color transparency
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    } else if (_DrawMode==MODE_DEST_TRANSPARENT) {
        cmd32 |= 0x00009000;   // destination pixels control transparency
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    }

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Set the clip rectangle. (top-left to down-right).
  * @param[in] x1 is top-left x position
  * @param[in] y1 is top-left y position
  * @param[in] x2 is down-right x position
  * @param[in] y2 is down-right y position
  * @return none
  */
void ge2dClip_SetClip(int x1, int y1, int x2, int y2)
{

#ifdef DEBUG
    sysprintf("set_clip(): (%d,%d)-(%d,%d)\n", x1, y1, x2, y2);
#endif

    if ((x1>=0) && (y1>=0) && (x2>=0) && (y2>=0)) {
        if ((x2 > x1) && (y2 > y1)) {
            _ClipEnable = TRUE;
            /* hardware clipper not includes last pixel */
            x2++;
            y2++;
            _ClipTL = (UINT32)((y1 << 16) | x1);
            _ClipBR = (UINT32)((y2 << 16) | x2);
        } else {
            _ClipEnable = FALSE;
        }
    } else {
        _ClipEnable = FALSE;
    }
}

/**
  * @brief Set the clip to inside clip or outside clip.
  * @param[in] opt is option for setting clip inside or outside, value could be
  *                                 - \ref MODE_INSIDE_CLIP
  *                                 - \ref MODE_OUTSIDE_CLIP
  * @return none
  */
void ge2dClip_SetClipMode(int opt)
{
    _OutsideClip = (opt==0) ? FALSE : TRUE;

    if (_OutsideClip) {
#ifdef DEBUG
        sysprintf("set_clip_mode(): OUTSIDE\n");
#endif
    } else {
#ifdef DEBUG
        sysprintf("set_clip_mode(): INSIDE\n");
#endif
    }
}

/**
  * @brief Draw an one-pixel rectangle frame.
  * @param[in] x1 is top-left x position
  * @param[in] y1 is top-left y position
  * @param[in] x2 is down-right x position
  * @param[in] y2 is down-right y position
  * @param[in] color is color of this rectangle
  * @param[in] opt is draw option, value could be
  *                                - 0: rectangle
  *                                - 1: diagonal
  * @return none
  */
void ge2dDrawFrame(int x1, int y1, int x2, int y2, int color, int opt)
{
    UINT32 dest_pitch, dest_start, dest_dimension;
    UINT32 color32;

#ifdef DEBUG
    sysprintf("draw_frame():\n");
    sysprintf("(%d,%d)-(%d,%d)\n", x1, y1, x2, y2);
    sysprintf("color=0x%x opt=%d\n", color, opt);
#endif

    /*
    ** The opt==1 case must be specially handled.
    */

    if (opt==0) {
        outpw(REG_GE2D_CTL, 0xcccb0000); // rectangle
    } else {
        outpw(REG_GE2D_CTL, 0xcccf0000); // diagonal
    }

#ifdef DEBUG
    sysprintf("(%d,%d)-(%d,%d)\n", x1, y1, x2, y2);
#endif

    color32 = make_color(color);
    outpw(REG_GE2D_FGCOLR, color32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = y1 << 16 | x1;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = (y2-y1) << 16 | (x2-x1);
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    outpw(REG_GE2D_MISCTL, inpw(REG_GE2D_MISCTL)); // address caculation

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Draw an solid rectangle line.
  * @param[in] x1 is top-left x position
  * @param[in] y1 is top-left y position
  * @param[in] x2 is down-right x position
  * @param[in] y2 is down-right y position
  * @param[in] color is color of this line
  * @return none
  */
void ge2dLine_DrawSolidLine(int x1, int y1, int x2, int y2, int color)
{
    int abs_X, abs_Y, min, max;
    UINT32 step_constant, initial_error, direction_code;
    UINT32 cmd32, dest_pitch, dest_start;

#ifdef DEBUG
    sysprintf("draw_solid_line():\n");
    sysprintf("(%d,%d)-(%d,%d)\n", x1, y1, x2, y2);
    sysprintf("color=0x%x\n", color);
#endif

    abs_X = ABS(x2-x1);   //absolute value
    abs_Y = ABS(y2-y1);   //absolute value
    if (abs_X > abs_Y) {  // X major
        max = abs_X;
        min = abs_Y;

        step_constant = (((UINT32)(2*(min-max))) << 16) | (UINT32)(2*min);
        initial_error = (((UINT32)(2*(min)-max)) << 16) | (UINT32)(max);

        if (x2 > x1) { // +X direction
            if (y2 > y1) // +Y direction
                direction_code = XpYpXl;
            else // -Y direction
                direction_code = XpYmXl;
        } else { // -X direction
            if (y2 > y1) // +Y direction
                direction_code = XmYpXl;
            else // -Y direction
                direction_code = XmYmXl;
        }
    } else { // Y major
        max = abs_Y;
        min = abs_X;

        step_constant = (((UINT32)(2*(min-max))) << 16) | (UINT32)(2*min);
        initial_error = (((UINT32)(2*(min)-max)) << 16) | (UINT32)(max);

        if (x2 > x1) { // +X direction
            if (y2 > y1) // +Y direction
                direction_code = XpYpYl;
            else // -Y direction
                direction_code = XpYmYl;
        } else { // -X direction
            if (y2 > y1) // +Y direction
                direction_code = XmYpYl;
            else // -Y direction
                direction_code = XmYmYl;
        }
    }

    outpw(REG_GE2D_BETSC, step_constant);
    outpw(REG_GE2D_BIEPC, initial_error);

    cmd32 = 0x008b0000 | direction_code;

    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_BGCOLR, make_color(color));
    outpw(REG_GE2D_FGCOLR, make_color(color));

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    outpw(REG_GE2D_XYDORG, (int)GFX_START_ADDR);

    dest_start = y1 << 16 | x1;
    outpw(REG_GE2D_DSTSPA, dest_start);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Draw a styled line.
  * @param[in] x1 is top-left x position
  * @param[in] y1 is top-left y position
  * @param[in] x2 is down-right x position
  * @param[in] y2 is down-right y position
  * @param[in] style is style of line pattern
  * @param[in] fgcolor is color of foreground
  * @param[in] bkcolor is color of background
  * @param[in] draw_mode is transparent is enable or not
  * @return none
  */
void ge2dLine_DrawStyledLine(int x1, int y1, int x2, int y2, int style, int fgcolor, int bkcolor, int draw_mode)
{
    int abs_X, abs_Y, min, max;
    UINT32 step_constant, initial_error, direction_code;
    UINT32 cmd32, dest_pitch, dest_start;
    UINT32 temp32, line_control_code;

    abs_X = ABS(x2-x1);
    abs_Y = ABS(y2-y1);
    if (abs_X > abs_Y) { // X major
        max = abs_X;
        min = abs_Y;

        step_constant = (((UINT32)(2*(min-max))) << 16) | (UINT32)(2*min);
        initial_error = (((UINT32)(2*min-max)) << 16) | (UINT32)(max);

        if (x2 > x1) { // +X direction
            if (y2 > y1) // +Y direction
                direction_code = XpYpXl;
            else // -Y direction
                direction_code = XpYmXl;
        } else { // -X direction
            if (y2 > y1) // +Y direction
                direction_code = XmYpXl;
            else // -Y direction
                direction_code = XmYmXl;
        }
    } else { // Y major
        max = abs_Y;
        min = abs_X;

        step_constant = (((UINT32)(2*(min-max))) << 16) | (UINT32)(2*min);
        initial_error = (((UINT32)(2*min-max)) << 16) | (UINT32)(max);

        if (x2 > x1) { // +X direction
            if (y2 > y1) // +Y direction
                direction_code = XpYpYl;
            else // -Y direction
                direction_code = XpYmYl;
        } else { // -X direction
            if (y2 > y1) // +Y direction
                direction_code = XmYpYl;
            else // -Y direction
                direction_code = XmYmYl;
        }
    }

    outpw(REG_GE2D_BETSC, step_constant);
    outpw(REG_GE2D_BIEPC, initial_error);

    cmd32 = 0x009b0000 | direction_code; // styled line
    if (draw_mode==MODE_TRANSPARENT) {
        cmd32 |= 0x00008000; // default is MODE_OPAQUE
    }
    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_BGCOLR, make_color(bkcolor));
    outpw(REG_GE2D_FGCOLR, make_color(fgcolor));

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    outpw(REG_GE2D_XYDORG, (int)GFX_START_ADDR);

    dest_start = y1 << 16 | x1;
    outpw(REG_GE2D_DSTSPA, dest_start);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    line_control_code = style;
    temp32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
    temp32 = (line_control_code << 16) | temp32;

    outpw(REG_GE2D_MISCTL, temp32); // address caculation

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Rectangle solid color fill with foreground color.
  * @param[in] dx x position
  * @param[in] dy y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] color is color of foreground
  * @return none
  */
void ge2dFill_Solid(int dx, int dy, int width, int height, int color)
{
    UINT32 cmd32, color32;
    UINT32 dest_start, dest_pitch, dest_dimension;

#ifdef DEBUG
    sysprintf("solid_fill()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", dx, dy, dx+width-1, dy+height-1);
    sysprintf("color=0x%x\n", color);
#endif

    color32 = make_color(color);
    cmd32 = 0xcc430060;
    outpw(REG_GE2D_CTL, cmd32);
    outpw(REG_GE2D_FGCOLR, color32); // fill with foreground color

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = dy << 16 | dx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Rectangle solid color fill with background color.
  * @param[in] dx x position
  * @param[in] dy y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] color is color of background
  * @return none
  */
void ge2dFill_SolidBackground(int dx, int dy, int width, int height, int color)
{
    UINT32 cmd32, color32;
    UINT32 dest_start, dest_pitch, dest_dimension;

#ifdef DEBUG
    sysprintf("solid_fill_back()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", dx, dy, dx+width-1,dy+height-1);
    sysprintf("color=0x%x\n", color);
#endif

    color32 = make_color(color);

    cmd32 = 0xcc430040;
    outpw(REG_GE2D_CTL, cmd32);
    outpw(REG_GE2D_BGCOLR, color32); // fill with foreground color

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = dy << 16 | dx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Rectangle fill with 8x8 color pattern.
  * @param[in] dx x position
  * @param[in] dy y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @return none
  * @note The color pattern data is stored in the off-screen buffer.
  */
void ge2dFill_ColorPattern(int dx, int dy, int width, int height)
{
    UINT32 cmd32;
    UINT32 dest_start, dest_pitch, dest_dimension;

#ifdef DEBUG
    sysprintf("color_pattern_fill()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", dx, dy, dx+width-1, dy+height-1);
    sysprintf("pattern offset (%d,%d)\n", dx%8, dy%8);
#endif

    cmd32 = 0xf0430000;
    outpw(REG_GE2D_CTL, cmd32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = dy << 16 | dx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Rectangle fill with 8x8 mono pattern.
  * @param[in] dx x position
  * @param[in] dy y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] opt is transparent is enable or not
  * @return none
  */
void ge2dFill_MonoPattern(int dx, int dy, int width, int height, int opt)
{
    UINT32 cmd32;
    UINT32 dest_start, dest_pitch, dest_dimension;

#ifdef DEBUG
    sysprintf("mono_pattern_fill()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", dx, dy, dx+width-1, dy+height-1);
#endif

    cmd32 = 0xf0430010;
    if (opt==MODE_TRANSPARENT) {
        cmd32 |= 0x00006000;
    }
    outpw(REG_GE2D_CTL, cmd32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = dy << 16 | dx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Rectangle fill with 8x8 color pattern.
  * @param[in] sx x position
  * @param[in] sy y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] rop is ROP operation code
  * @return none
  */
void ge2dFill_ColorPatternROP(int sx, int sy, int width, int height, int rop)
{
    UINT32 cmd32;
    UINT32 dest_start, dest_pitch, dest_dimension;

#ifdef DEBUG
    sysprintf("color_pattern_fill()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", sx, sy, sx+width-1, sy+height-1);
    sysprintf("pattern offset (%d,%d)\n", sx%8, sy%8);
#endif

    cmd32 = 0x00430000 | (rop<<24);
    outpw(REG_GE2D_CTL, cmd32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = sy << 16 | sx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Rectangle fill with 8x8 mono pattern.
  * @param[in] sx x position
  * @param[in] sy y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] opt is transparent is enable or not
  * @param[in] rop is ROP operation code
  * @return none
  */
void ge2dFill_MonoPatternROP(int sx, int sy, int width, int height, int rop, int opt)
{
    UINT32 cmd32;
    UINT32 dest_start, dest_pitch, dest_dimension;

#ifdef DEBUG
    sysprintf("mono_pattern_fill()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", sx, sy, sx+width-1, sy+height-1);
#endif

    cmd32 = 0x00430010 | (rop<<24);
    if (opt==MODE_TRANSPARENT) {
        cmd32 |= 0x00006000;
    }
    outpw(REG_GE2D_CTL, cmd32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = sy << 16 | sx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_TRG, 1);

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief TileBLT function.
  * @param[in] srcx source x position
  * @param[in] srcy source y position
  * @param[in] destx destination x position
  * @param[in] desty destination y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] x_count is tile count for x-axis
  * @param[in] y_count is tile count for y-axis
  * @return none
  */
void ge2dFill_TileBlt(int srcx, int srcy, int destx, int desty, int width, int height, int x_count, int y_count)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 tile_ctl;

#ifdef DEBUG
    sysprintf("tile_blt_image()\n");
    sysprintf("(%d,%d)=>(%d,%d)\n", srcx, srcy, destx, desty);
    sysprintf("width=%d height=%d\n", width, height);
    sysprintf("%dx%d grids\n", x_count, y_count);
#endif

    if (x_count > 0) x_count--;
    if (y_count > 0) y_count--;

    cmd32 = 0xcc430400; // b10 is the tile control

    outpw(REG_GE2D_CTL, cmd32);

    pitch = GFX_WIDTH << 16 | GFX_WIDTH; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = srcy << 16 | srcx;           // redundancy ??
    outpw(REG_GE2D_SRCSPA, src_start);  // redundancy ??

    dest_start = desty << 16 | destx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    tile_ctl = (y_count << 8) | (x_count);
    outpw(REG_GE2D_TCNTVHSF, tile_ctl);

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Host-to-Screen BitBlt with SRCCOPY (through data port)
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] buf is pointer of HostBLT data
  * @return none
  */
void ge2dHostBlt_Write(int x, int y, int width, int height, void *buf)
{
    UINT32 cmd32, dest_pitch, dest_start, dest_dimension;
    int transfer_count, i, j;
    UINT32 *ptr32, data32;

#ifdef DEBUG
    sysprintf("host_write_blt()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", x, y, x+width-1, y+height-1);
    sysprintf("width=%d height=%d\n", width, height);
#endif

    cmd32 = 0xcc430020;

    outpw(REG_GE2D_CTL, cmd32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    outpw(REG_GE2D_TRG, 1);

    ptr32 = (UINT32 *)buf;
    for (i=0; i<height; i++) { // 120
        transfer_count = (width * (GFX_BPP/8) + 3) / 4; // 4-byte count

        while (transfer_count >= 8) {
            while ((inpw(REG_GE2D_MISCTL) & 0x00000800)==0); // check empty
            for (j=0; j<8; j++) {
                data32 = *ptr32++;
                outpw(REG_GE2D_GEHBDW0, data32);
            }
            transfer_count -= 8;
        }

        if (transfer_count > 0) {
            while ((inpw(REG_GE2D_MISCTL) & 0x00000800)==0); // check empty
            for (j=0; j<transfer_count; j++) {
                data32 = *ptr32++;
                outpw(REG_GE2D_GEHBDW0, data32);
            }
        }
    }

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Screen-to-Host BitBlt with SRCCOPY (through data port).
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] buf is pointer of HostBLT data
  * @return none
  */
void ge2dHostBlt_Read(int x, int y, int width, int height, void *buf)
{
    UINT32 cmd32, dest_pitch, dest_start, dest_dimension;
    int transfer_count, i, j;
    UINT32 *ptr32;

#ifdef DEBUG
    sysprintf("host_read_blt()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", x, y, x+width-1, y+height-1);
    sysprintf("width=%d height=%d\n", width, height);
#endif

    cmd32 = 0xcc430001;

    outpw(REG_GE2D_CTL, cmd32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    outpw(REG_GE2D_TRG, 1);

    ptr32 = (UINT32 *)buf;
    for (i=0; i<height; i++) {
        transfer_count = (width * (GFX_BPP/8) + 3) / 4; // 4-byte count

        while (transfer_count >= 8) {
            while ((inpw(REG_GE2D_MISCTL) & 0x00000400)==0);
            for (j=0; j<8; j++) {
                *ptr32++ = inpw(REG_GE2D_GEHBDW0);
            }
            transfer_count -= 8;
        }

        if (transfer_count > 0) {
            while (((inpw(REG_GE2D_MISCTL) & 0x0000f000)>>12) != transfer_count);
            for (j=0; j<transfer_count; j++) {
                *ptr32++ = inpw(REG_GE2D_GEHBDW0);
            }
        }
    }

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Host-to-Screen SpriteBlt with SRCCOPY.
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] buf is pointer of HostBLT data
  * @return none
  */
void ge2dHostBlt_Sprite(int x, int y, int width, int height, void *buf)
{
    UINT32 cmd32, dest_pitch, dest_start, dest_dimension;
    int transfer_count, i, j;
    UINT32 *ptr32, data32, alpha;

#ifdef DEBUG
    sysprintf("host_sprite_blt()\n");
    sysprintf("(%d,%d)-(%d,%d)\n", x, y, x+width-1, y+height-1);
#endif

    cmd32 = 0xcc430020;

    outpw(REG_GE2D_CTL, cmd32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    if (_DrawMode==MODE_TRANSPARENT) {
        cmd32 |= 0x00008000; // color transparency
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    } else if (_DrawMode==MODE_DEST_TRANSPARENT) {
        cmd32 |= 0x00009000;
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_TRNSCOLR, _ColorKey);
        outpw(REG_GE2D_TCMSK, _ColorKeyMask);
    }

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    outpw(REG_GE2D_TRG, 1);

    ptr32 = (UINT32 *)buf;
    for (i=0; i<height; i++) {
        transfer_count = width * (GFX_BPP/8) / 4; // 4-byte count

        while (transfer_count > 8) {
            while ((inpw(REG_GE2D_MISCTL) & 0x00000800)==0); // check empty
            for (j=0; j<8; j++) {
                data32 = *ptr32++;
                outpw(REG_GE2D_GEHBDW0, data32);
            }
            transfer_count -= 8;
        }

        if (transfer_count > 0) {
            while ((inpw(REG_GE2D_MISCTL) & 0x00000800)==0); // check empty
            for (j=0; j<transfer_count; j++) {
                data32 = *ptr32++;
                outpw(REG_GE2D_GEHBDW0, data32);
            }
        }
    }

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Captured the specified photo data from display memory, then displayed on display memory by rotation angle
  * @param[in] srcx source x position
  * @param[in] srcy source y position
  * @param[in] destx destination x position
  * @param[in] desty destination y position
  * @param[in] width is display width
  * @param[in] height is display height
  * @param[in] ctl is drawing direction
  * @return none
  */
void ge2dRotation(int srcx, int srcy, int destx, int desty, int width, int height, int ctl)
{
    UINT32 cmd32, dest_start, src_start, dimension, pitch;
    void *tmpscreen,*orig_dest_start00;

    tmpscreen = (void *)malloc(width*height*GFX_BPP/8);

#ifdef DEBUG
    sysprintf("rotation_image()\n");
    sysprintf("(%d,%d)=>(%d,%d)\n", srcx, srcy, destx, desty);
    sysprintf("width=%d height=%d\n", width, height);
#endif

    memset(tmpscreen,0,width*height*GFX_BPP/8);

    orig_dest_start00 = (void *)inpw(REG_GE2D_XYDORG);
    outpw(REG_GE2D_XYDORG, (int)tmpscreen);   //captured photo to another position
    outpw(REG_GE2D_XYSORG, (int)GFX_START_ADDR);

    ge2dBitblt_SourceToDestination(srcx,srcy,0,0,width,height,GFX_WIDTH,width);

    src_start = dest_start = dimension = cmd32 = pitch = 0;

    outpw(REG_GE2D_XYDORG, (int)orig_dest_start00);
    outpw(REG_GE2D_XYSORG, (int)tmpscreen);

    pitch = GFX_WIDTH << 16 | width;
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = 0 << 16 | 0;  // captured photo at (0,0) position
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = desty << 16 | destx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    cmd32 = 0xcc030000 | (ctl << 1);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    /* set rotation reference point xy register, then nothing happened */
    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status

    free(tmpscreen);
}

/**
  * @brief OffScreen-to-OnScreen SpriteBlt with SRCCOPY.
  * @param[in] destx destination x position
  * @param[in] desty destination y position
  * @param[in] sprite_width is sprite width
  * @param[in] sprite_height is sprite height
  * @param[in] buf is pointer of origin data
  * @return none
  */
void ge2dSpriteBlt_Screen(int destx, int desty, int sprite_width, int sprite_height, void *buf)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;

#ifdef DEBUG
    sysprintf("screen_sprite_blt():\n");
    sysprintf("x=%d y=%d width=%d height=%d\n", destx, desty, sprite_width, sprite_height);
#endif

    cmd32 = 0xcc430000;

    outpw(REG_GE2D_CTL, cmd32);

    pitch = GFX_WIDTH << 16 | sprite_width; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = 0; // start from (0,0) of sprite
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = desty << 16 | destx;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = sprite_height << 16 | sprite_width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    outpw(REG_GE2D_XYSORG, (UINT32)buf);
    outpw(REG_GE2D_XYDORG, (int)GFX_START_ADDR);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    //
    // default source color transparent is ON
    //

    cmd32 |= 0x00008000; // color transparency
    outpw(REG_GE2D_CTL, cmd32);
    outpw(REG_GE2D_TRNSCOLR, make_color(COLOR_KEY));
    outpw(REG_GE2D_TCMSK, 0xffffff);

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief OffScreen-to-OnScreen SpriteBlt with SRCCOPY.
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] sprite_sx sprite x position
  * @param[in] sprite_sy sprite y position
  * @param[in] width is width
  * @param[in] height is height
  * @param[in] sprite_width is sprite width
  * @param[in] sprite_height is sprite height
  * @param[in] buf is pointer of origin data
  * @return none
  * @note The sprite starting address can be programmed.
  */
void ge2dSpriteBltx_Screen(int x, int y, int sprite_sx, int sprite_sy, int width, int height, int sprite_width, int sprite_height, void *buf)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;

#ifdef DEBUG
    sysprintf("screen_sprite_bltx(): (%d,%d)\n", x, y);
    sysprintf("sprite width=%d height=%d\n", sprite_width, sprite_height);
    sysprintf("x=%d y=%d width=%d height=%d\n", sprite_sx, sprite_sy, width, height);
#endif

    cmd32 = 0xcc430000;

    outpw(REG_GE2D_CTL, cmd32);

    pitch = GFX_WIDTH << 16 | sprite_width; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, pitch);

    outpw(REG_GE2D_XYSORG, (UINT32)buf);
    outpw(REG_GE2D_XYDORG, (int)GFX_START_ADDR);

    src_start = sprite_sy << 16 | sprite_sx;
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    //
    // default source color transparent is ON
    //
    cmd32 |= 0x00008000; // color transparency
    outpw(REG_GE2D_CTL, cmd32);
    outpw(REG_GE2D_TRNSCOLR, make_color(COLOR_KEY));
    outpw(REG_GE2D_TCMSK, 0xffffff);

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief OffScreen-to-OnScreen SpriteBlt with ROP.
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] sprite_width is sprite width
  * @param[in] sprite_height is sprite height
  * @param[in] buf is pointer of origin data
  * @param[in] rop is ROP operation code
  * @return none
  * @note The sprite always starts from (0,0) for the BLT.
  */
void ge2dSpriteBlt_ScreenRop(int x, int y, int sprite_width, int sprite_height, void *buf, int rop)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;

#ifdef DEBUG
    sysprintf("screen_sprite_rop_blt():\n");
    sysprintf("x=%d y=%d width=%d height=%d\n", x, y, sprite_width, sprite_height);
    sysprintf("rop=0x%x\n", rop);
#endif

    cmd32 = 0x00430000 | (rop << 24);

    if (_PatternType==TYPE_MONO) {
        cmd32 |= 0x00000010; // default is TYPE_COLOR
    }

    outpw(REG_GE2D_CTL, cmd32);

    pitch = GFX_WIDTH << 16 | sprite_width; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = 0; // start from (0,0) of sprite
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = sprite_height << 16 | sprite_width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    outpw(REG_GE2D_XYSORG, (UINT32)buf);
    outpw(REG_GE2D_XYDORG,(int) GFX_START_ADDR);  //smf

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    //
    // default source color transparent is ON
    //
    cmd32 |= 0x00008000; // color transparency
    outpw(REG_GE2D_CTL, cmd32);
    outpw(REG_GE2D_TRNSCOLR, make_color(COLOR_KEY));
    outpw(REG_GE2D_TCMSK, 0xffffff);

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    if ((rop==0x00) || (rop==0xff)) {
        cmd32 = (cmd32 & 0xffff0fff) | 0x00009000;
        outpw(REG_GE2D_CTL, cmd32);
    }

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief OffScreen-to-OnScreen SpriteBlt with ROP.
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] sprite_sx sprite x position
  * @param[in] sprite_sy sprite y position
  * @param[in] width is width
  * @param[in] height is height
  * @param[in] sprite_width is sprite width
  * @param[in] sprite_height is sprite height
  * @param[in] buf is pointer of origin data
  * @param[in] rop is ROP operation code
  * @return none
  * @note The sprite always starts from (0,0) for the BLT.
  */
void ge2dSpriteBltx_ScreenRop(int x, int y, int sprite_sx, int sprite_sy, int width, int height, int sprite_width, int sprite_height, void *buf, int rop)
{
    UINT32 cmd32, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;

#ifdef DEBUG
    sysprintf("screen_sprite_rop_bltx():\n");
    sysprintf("x=%d y=%d width=%d height=%d\n", x, y, sprite_width, sprite_height);
    sysprintf("rop=0x%x\n", rop);
#endif

    cmd32 = 0x00430000 | (rop << 24);

    if (_PatternType==TYPE_MONO) {
        cmd32 |= 0x00000010; // default is TYPE_COLOR
    }

    outpw(REG_GE2D_CTL, cmd32);

    pitch = GFX_WIDTH << 16 | sprite_width; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, pitch);

    src_start = sprite_sy << 16 | sprite_sx;
    outpw(REG_GE2D_SRCSPA, src_start);

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dimension);

    outpw(REG_GE2D_XYSORG, (UINT32)buf);
    outpw(REG_GE2D_XYDORG, (int)GFX_START_ADDR);  //smf

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    //
    // default source color transparent is ON
    //
    cmd32 |= 0x00008000; // color transparency
    outpw(REG_GE2D_CTL, cmd32);
    outpw(REG_GE2D_TRNSCOLR, make_color(COLOR_KEY));
    outpw(REG_GE2D_TCMSK, 0xffffff);

    if (_EnableAlpha) {
        cmd32 |= 0x00200000;
        outpw(REG_GE2D_CTL, cmd32);

        data32 = inpw(REG_GE2D_MISCTL) & 0x0000ffff;
        alpha = (UINT32)((_AlphaKs << 8) | _AlphaKd);
        data32 |= (alpha << 16);

        outpw(REG_GE2D_MISCTL, data32);
    }

    if ((rop==0x00) || (rop==0xff)) {
        cmd32 = (cmd32 & 0xffff0fff) | 0x00009000;
        outpw(REG_GE2D_CTL, cmd32);
    }

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief OffScreen-to-OnScreen TextBLT.
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] width is width
  * @param[in] height is height
  * @param[in] fore_color is color of foreground
  * @param[in] back_color is color of background
  * @param[in] opt is transparent is enable or not
  * @param[in] buf is pointer of origin data
  * @return none
  * @note Fetch the monochrome source data from off-screen memory to the desired destination area
  */
void ge2dColorExpansionBlt(int x, int y, int width, int height, int fore_color, int back_color, int opt, void *buf)
{
    UINT32 cmd32, dest_pitch, src_pitch, pitch, dest_start, dest_dimension;
    UINT32 fore_color32, back_color32;

    fore_color32 = make_color(fore_color);
    back_color32 = make_color(back_color);

    cmd32 = 0xcc430080;
    if (opt==MODE_TRANSPARENT) {
        cmd32 |= 0x00004000; // mono transparency
    }

    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_FGCOLR, fore_color32);
    outpw(REG_GE2D_BGCOLR, back_color32);

    dest_pitch = GFX_WIDTH; // pitch in pixels
    src_pitch = width; // pitch in pixels

    pitch = (dest_pitch << 16) | src_pitch;
    outpw(REG_GE2D_SDPITCH, pitch);

    outpw(REG_GE2D_XYSORG, (int)buf);
    outpw(REG_GE2D_SRCSPA, 0); // always start from (0,0)

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    if (_ClipEnable) {
        cmd32 |= 0x00000200;
        if (_OutsideClip) {
            cmd32 |= 0x00000100;
        }
        outpw(REG_GE2D_CTL, cmd32);
        outpw(REG_GE2D_CLPBTL, _ClipTL);
        outpw(REG_GE2D_CLPBBR, _ClipBR);
    }

    outpw(REG_GE2D_TRG, 1);
    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete
    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Host-to-Screen TextBLT through data port.
  * @param[in] x x position
  * @param[in] y y position
  * @param[in] width is width
  * @param[in] height is height
  * @param[in] fore_color is color of foreground
  * @param[in] back_color is color of background
  * @param[in] opt is transparent is enable or not
  * @param[in] buf is pointer of origin data
  * @return none
  */
void ge2dHostColorExpansionBlt(int x, int y, int width, int height, int fore_color, int back_color, int opt, void *buf)
{
    UINT32 cmd32, dest_pitch, dest_start, dest_dimension;
    UINT32 fore_color32, back_color32;
    UINT32 *ptr32, data32;
    int transfer_count, i, j;

    fore_color32 = make_color(fore_color);
    back_color32 = make_color(back_color);

    cmd32 = 0xcc4300a0;
    if (opt==MODE_TRANSPARENT) {
        cmd32 |= 0x00004000; // mono transparency
    }

    outpw(REG_GE2D_CTL, cmd32);

    outpw(REG_GE2D_FGCOLR, fore_color32);
    outpw(REG_GE2D_BGCOLR, back_color32);

    dest_pitch = GFX_WIDTH << 16; // pitch in pixel
    outpw(REG_GE2D_SDPITCH, dest_pitch);

    dest_start = y << 16 | x;
    outpw(REG_GE2D_DSTSPA, dest_start);
    outpw(REG_GE2D_SRCSPA, dest_start);

    dest_dimension = height << 16 | width;
    outpw(REG_GE2D_RTGLSZ, dest_dimension);

    outpw(REG_GE2D_TRG, 1);

    ptr32 = (UINT32 *)buf;
    for (i=0; i<height; i++) {
        transfer_count = (width+31) / 32; // 32 pixels unit

        while (transfer_count > 8) {
            while ((inpw(REG_GE2D_MISCTL) & 0x00000800)==0); // check empty
            for (j=0; j<8; j++) {
                data32 = *ptr32++;
                outpw(REG_GE2D_GEHBDW0, data32);
            }
            transfer_count -= 8;
        }

        if (transfer_count > 0) {
            while ((inpw(REG_GE2D_MISCTL) & 0x00000800)==0); // check empty
            for (j=0; j<transfer_count; j++) {
                data32 = *ptr32++;
                outpw(REG_GE2D_GEHBDW0, data32);
            }
        }
    }

    while ((inpw(REG_GE2D_INTSTS)&0x01)==0); // wait for command complete

    outpw(REG_GE2D_INTSTS, 1); // clear interrupt status
}

/**
  * @brief Set the 8x8 mono pattern for following BitBLT functions.
  * @param[in] opt is index for build-in pattern
  * @param[in] fore_color is color of foreground
  * @param[in] back_color is color of background
  * @return none
  */
void ge2dInitMonoPattern(int opt, int fore_color, int back_color)
{
    UINT32 color32;

    /*
    ** If hardware pattern definition is a little different from software.
    ** Need to do the BYTE swap before programming the pattern registers.
    */

    outpw(REG_GE2D_PTNA, MonoPatternData[opt].PatternA);
    outpw(REG_GE2D_PTNB, MonoPatternData[opt].PatternB);

    color32 = make_color(fore_color);
    outpw(REG_GE2D_FGCOLR, color32);

    color32 = make_color(back_color);
    outpw(REG_GE2D_BGCOLR, color32);

    _PatternType = TYPE_MONO;
}

/**
  * @brief Set the 8x8 mono pattern for following BitBLT functions.
  * @param[in] PatternA is pattern A
  * @param[in] PatternB is pattern B
  * @param[in] fore_color is color of foreground
  * @param[in] back_color is color of background
  * @return none
  */
void ge2dInitMonoInputPattern(UINT32 PatternA, UINT32 PatternB, int fore_color, int back_color)
{
    UINT32 color32;

    /*
    ** If hardware pattern definition is a little different from software.
    ** Need to do the BYTE swap before programming the pattern registers.
    */

    outpw(REG_GE2D_PTNA, PatternA);
    outpw(REG_GE2D_PTNB, PatternB);

    color32 = make_color(fore_color);
    outpw(REG_GE2D_FGCOLR, color32);

    color32 = make_color(back_color);
    outpw(REG_GE2D_BGCOLR, color32);

    _PatternType = TYPE_MONO;
}

/**
  * @brief Set the 8x8 color pattern for following BitBLT functions.
  * @param[in] patformat is format of pattern, value could be
  *                                     - \ref RGB888
  *                                     - \ref RGB565
  *                                     - \ref RGB332
  * @param[in] patdata is pointer of input pattern image
  * @return none
  * @note This function transfers those forms:
  *   RGB888 to RGB565 or RGB332
  *   RGB565 to RGB332 or RGB888
  *   RGB332 to RGB565 or RGB888
  */
void ge2dInitColorPattern(int patformat, void *patdata)
{
    UINT8 *ptr_pat;
    UINT8 *ptr8, r8, g8, b8;
    UINT16 *ptr16, r16, g16, b16, g16_1, g16_2;
    UINT32 *ptr32, r32, g32, b32, g32_1, g32_2;
    int idx;

    ptr_pat = (UINT8 *)patdata;
    if(patformat == RGB888) {
        if (GFX_BPP==8) {
            ptr8 = (UINT8 *)GFX_PAT_ADDR;
            for (idx=0; idx<64; idx++) {
                b8 = (UINT8)(*ptr_pat++) & 0xc0; // 2 bits
                g8 = (UINT8)(*ptr_pat++) & 0xe0; // 3 bits
                r8 = (UINT8)(*ptr_pat++) & 0xe0; // 3 bits
                ptr_pat++;
                *ptr8++ = r8 | (g8>>3) | (b8>>6);
            }
        } else if (GFX_BPP==16) {
            ptr16 = (UINT16 *)GFX_PAT_ADDR;
            for (idx=0; idx<64; idx++) {
                b16 = (UINT16)(*ptr_pat++) & 0x000f8; // 5 bits
                g16 = (UINT16)(*ptr_pat++) & 0x000fc; // 6 bits
                r16 = (UINT16)(*ptr_pat++) & 0x000f8; // 5 bits
                ptr_pat++;
                *ptr16++ = (r16<<8) | (g16<<3) | (b16>>3);
            }
        } else if (GFX_BPP==32) {
            ptr32 = (UINT32 *)GFX_PAT_ADDR;
            for (idx=0; idx<64; idx++) {
                b32 = (UINT32)(*ptr_pat++);
                g32 = (UINT32)(*ptr_pat++);
                r32 = (UINT32)(*ptr_pat++);
                ptr_pat++;
                *ptr32++ = (r32<<16) | (g32<<8) | b32;
            }
        }
    } else if(patformat == RGB565) {
        if (GFX_BPP==8) {
            ptr8 = (UINT8 *)GFX_PAT_ADDR;

            for (idx=0; idx<64; idx++) {
                b8 = (UINT8)(*ptr_pat++) & 0x00018; // 2 bits
                g8 = (UINT8)(*ptr_pat) & 0x00007;  // 3 bits
                r8 = (UINT8)(*ptr_pat++) & 0x000e0;  // 3bits
                *ptr8++ = r8 | (g8<<2) | (b8>>3);
            }
        } else if (GFX_BPP==16) {
            ptr16 = (UINT16 *)GFX_PAT_ADDR;

            for (idx=0; idx<64; idx++) {
                *ptr16++ = (*ptr_pat) | (*(ptr_pat+1)) << 8;
                ptr_pat+=2;
            }
        } else if (GFX_BPP==32) {
            ptr32 = (UINT32 *)GFX_PAT_ADDR;

            for (idx=0; idx<64; idx++) {
                b32 = (UINT8)(*ptr_pat) & 0x1f;      // 5 bits
                g32_1 = (UINT8)(*ptr_pat++) & 0xe0;  // front 3 bits
                g32_2 = (UINT8)(*ptr_pat) & 0x07;    // back 3 bits
                g32 = ((g32_1>>5) | (g32_2<<3))<<2;
                r32 = (UINT8)(*ptr_pat++) & 0xf8;    // 5 bits
                *ptr32++ = 0<<24 | (r32<<16) | (g32<<8) | (b32<<3);
            }
        }
    } else if(patformat == RGB332) {
        if (GFX_BPP==8) {
            ptr8 = (UINT8 *)GFX_PAT_ADDR;

            for (idx=0; idx<64; idx++) {
                *ptr8++ = *ptr_pat;
                ptr_pat++;
            }
        } else if (GFX_BPP==16) {
            ptr16 = (UINT16 *)GFX_PAT_ADDR;

            for (idx=0; idx<64; idx++) {
                r16 = (UINT8)(*ptr_pat) & 0xe0; // 3 bits
                g16_1 = (UINT8)(*ptr_pat) & 0x10;
                g16_2 = (UINT8)(*ptr_pat) & 0x0c;
                g16 = (g16_1>>2) | (g16_2>>2);
                b16 = (UINT8)(*ptr_pat++) & 0x3; // 2 bits
                *ptr16++ = (r16<<8) | (g16<<8) | (b16<<3);
            }
        } else if (GFX_BPP==32) {
            ptr32 = (UINT32 *)GFX_PAT_ADDR;

            for (idx=0; idx<64; idx++) {
                r32 = (UINT8)(*ptr_pat) & 0xe0;  // 3 bits
                g32 = (UINT8)(*ptr_pat) & 0x1c;  // 3 bits
                b32 = (UINT8)(*ptr_pat++) & 0x3;  // 2 bits
                *ptr32++ = 0<<24 | (r32<<15) | (g32<<11) | (b32<<6);
            }
        }
    }

    _PatternType = TYPE_COLOR;
}
/*@}*/ /* end of group NUC970_GE2D_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group NUC970_GE2D_Driver */

/*@}*/ /* end of group NUC970_Device_Driver */

/*** (C) COPYRIGHT 2015 Nuvoton Technology Corp. ***/
