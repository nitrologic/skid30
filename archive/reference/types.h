#ifndef	EXEC_TYPES_H
#define	EXEC_TYPES_H
/*
**	$VER: types.h 40.1 (10.8.1993)
**	Includes Release 44.1
**
**	Data typing.  Must be included before any other Amiga include.
**
**	(C) Copyright 1985-1993 Amiga, Inc.
**	    All Rights Reserved
*/


#define INCLUDE_VERSION	44 /* Version of the include files in use. (Do not
			      use this label for OpenLibrary() calls!) */


#define GLOBAL	extern	    /* the declaratory use of an external */
#define IMPORT	extern	    /* reference to an external */
#define STATIC	static	    /* a local static variable */
#define REGISTER register   /* a (hopefully) register variable */


#ifndef VOID
#define VOID void
#endif

#ifndef CONST
#define CONST const
#endif


  /*  WARNING: APTR was redefined for the V36 Includes!  APTR is a   */
 /*  32-Bit Absolute Memory Pointer.  C pointer math will not	    */
/*  operate on APTR --	use "ULONG *" instead.			   */
#ifndef APTR_TYPEDEF
#define APTR_TYPEDEF
typedef void *		APTR;	    /* 32-bit untyped pointer */
#endif
typedef long		LONG;	    /* signed 32-bit quantity */
typedef unsigned long	ULONG;	    /* unsigned 32-bit quantity */
typedef unsigned long	LONGBITS;   /* 32 bits manipulated individually */
typedef short		WORD;	    /* signed 16-bit quantity */
typedef unsigned short	UWORD;	    /* unsigned 16-bit quantity */
typedef unsigned short	WORDBITS;   /* 16 bits manipulated individually */
#if __STDC__
typedef signed char	BYTE;	    /* signed 8-bit quantity */
#else
typedef char		BYTE;	    /* signed 8-bit quantity */
#endif
typedef unsigned char	UBYTE;	    /* unsigned 8-bit quantity */
typedef unsigned char	BYTEBITS;   /* 8 bits manipulated individually */
typedef unsigned short	RPTR;	    /* signed relative pointer */

#ifdef __cplusplus
typedef char *		STRPTR;     /* string pointer (NULL terminated) */
#else
typedef unsigned char *	STRPTR;     /* string pointer (NULL terminated) */
#endif

typedef CONST char *	CONST_STRPTR; /* constant string pointer (NULL terminated) */

/* For compatibility only: (don't use in new code) */
typedef short		SHORT;	    /* signed 16-bit quantity (use WORD) */
typedef unsigned short	USHORT;     /* unsigned 16-bit quantity (use UWORD) */
typedef short		COUNT;
typedef unsigned short	UCOUNT;
typedef ULONG		CPTR;


/* Types with specific semantics */
typedef float		FLOAT;
typedef double		DOUBLE;
typedef short		BOOL;
typedef unsigned char	TEXT;

#ifndef TRUE
#define TRUE		1
#endif
#ifndef FALSE
#define FALSE		0
#endif
#ifndef NULL
#define NULL		0L
#endif


#define BYTEMASK	0xFF


 /* #define LIBRARY_VERSION is now obsolete.  Please use LIBRARY_MINIMUM */
/* or code the specific minimum library version you require.		*/
#define LIBRARY_MINIMUM	33 /* Lowest version supported */

/* Some structure definitions include prototypes for function pointers.
 * This may not work with `C' compilers that do not comply to the ANSI
 * standard, which we will have to work around.
 */
#if __STDC__
#define __CLIB_PROTOTYPE(a) a
#else
#define __CLIB_PROTOTYPE(a)
#endif /* __STDC__ */

#endif	/* EXEC_TYPES_H */
