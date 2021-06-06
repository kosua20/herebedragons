/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
**
** Revision 1.1.2.2  2004/10/04 09:36:00  dborca
** second cut at Glide3x for Voodoo1/Rush (massive update):
** delayed validation, vertex snapping, clip coordinates, strip/fan_continue, bugfixes.
** and probably a bunch of other crap that I forgot
**
** Revision 1.1.2.1  2004/03/02 07:55:30  dborca
** Bastardised Glide3x for SST1
**
** Revision 1.1.1.1  1999/12/07 21:48:52  joseph
** Initial checkin into SourceForge.
**
*/
#ifndef __GLIDESYS_H__
#define __GLIDESYS_H__

/*
n** -----------------------------------------------------------------------
** COMPILER/ENVIRONMENT CONFIGURATION
** -----------------------------------------------------------------------
*/

/* Endianness is stored in bits [30:31] */
#define GLIDE_ENDIAN_SHIFT      30
#define GLIDE_ENDIAN_LITTLE     (0x1 << GLIDE_ENDIAN_SHIFT)
#define GLIDE_ENDIAN_BIG        (0x2 << GLIDE_ENDIAN_SHIFT)

/* OS is stored in bits [0:6] */
#define GLIDE_OS_SHIFT          0
#define GLIDE_OS_UNIX           0x1
#define GLIDE_OS_DOS32          0x2
#define GLIDE_OS_WIN32          0x4
#define GLIDE_OS_SYSTEM7        0x8
#define GLIDE_OS_OS2            0x10
#define GLIDE_OS_OTHER          0x20 /* For Proprietary Arcade HW */

/* Sim vs. Hardware is stored in bits [7:8] */
#define GLIDE_SST_SHIFT         7
#define GLIDE_SST_SIM           (0x1 << GLIDE_SST_SHIFT)
#define GLIDE_SST_HW            (0x2 << GLIDE_SST_SHIFT )

/* Hardware Type is stored in bits [9:12] */
#define GLIDE_HW_SHIFT          9
#define GLIDE_HW_SST1           (0x1 << GLIDE_HW_SHIFT)
#define GLIDE_HW_SST96          (0x2 << GLIDE_HW_SHIFT)
#define GLIDE_HW_SSTH3          (0x4 << GLIDE_HW_SHIFT)
#define GLIDE_HW_SST2           (0x8 << GLIDE_HW_SHIFT)

/*
** Make sure we handle all instances of WIN32
*/
#ifndef __WIN32__
#  if defined ( _WIN32 ) || defined (WIN32) || defined(__NT__)
#    define __WIN32__
#  endif
#endif

/* We need two checks on the OS: one for endian, the other for OS */
/* Check for endianness */
#if defined(__IRIX__) || defined(__sparc__) || defined(MACOS)
#  define GLIDE_ENDIAN    GLIDE_ENDIAN_BIG
#else
#  define GLIDE_ENDIAN GLIDE_ENDIAN_LITTLE
#endif

/* Check for OS */
#if defined(__IRIX__) || defined(__sparc__) || defined(__linux__)
#  define GLIDE_OS        GLIDE_OS_UNIX
#elif defined(__DOS__) || defined(__MSDOS__)
#  define GLIDE_OS        GLIDE_OS_DOS32
#elif defined(__WIN32__)
#  define GLIDE_OS        GLIDE_OS_WIN32
#endif

/* Check for Simulator vs. Hardware */
#ifdef GLIDE_SIMULATOR
#  define GLIDE_SST       GLIDE_SST_SIM
#else
#  define GLIDE_SST     GLIDE_SST_HW
#endif

/* Check for type of hardware */
#ifdef SST96
#  define GLIDE_HW        GLIDE_HW_SST96
#elif defined(SSTH3)
#  define GLIDE_HW        GLIDE_HW_SSTH3
#elif defined(SST2)
#  define GLIDE_HW        GLIDE_HW_SST2
#else /* Default to SST1 */
#  define GLIDE_HW        GLIDE_HW_SST1
#endif


#define GLIDE_PLATFORM (GLIDE_ENDIAN | GLIDE_OS | GLIDE_SST | GLIDE_HW)

/*
** Control the number of TMUs
*/
#ifndef GLIDE_NUM_TMU
#  define GLIDE_NUM_TMU 2
#endif


#if ( ( GLIDE_NUM_TMU < 0 ) && ( GLIDE_NUM_TMU > 3 ) )
#  error "GLIDE_NUM_TMU set to an invalid value"
#endif

#endif /* __GLIDESYS_H__ */
