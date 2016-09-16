/**********************************************************************
*   REXXVIO.C                                                         *
*                                                                     *
*   This program extends the REXX language by providing many          *
*   REXX external functions.                                          *
*   These functions are:                                              *
*       VioReadCellStr      --  Read Screen Cells                     *
*       VioScrollLeft       --  Scroll Screen Left                    *
*       VioScrollRight      --  Scroll Screen Right                   *
*       VioScrollUp         --  Scroll Screen Up                      *
*       VioScrollDown       --  Scroll Screen Down                    *
*       VioWrtCellStr       --  Write Screen Cells                    *
*       VioWrtCharStr       --                                        *
*       VioWrtCharStrAttr   --                                        *
*       VioGetCurType       --                                        *
*       VioSetCurType       --                                        *
*       VioWrtNAttr         --                                        *
*       VioWrtNCell         --                                        *
*       VioWrtNChar         --                                        *
*                                                                     *
*   To compile:    MAKE REXXVIO                                       *
*                                                                     *
**********************************************************************/
/* Include files */

#define  INCL_WINSHELLDATA
#define  INCL_VIO
#define  INCL_DOS
#define  INCL_DOSMEMMGR
#define  INCL_DOSMISC
#define  INCL_ERRORS
#define  INCL_REXXSAA
#define  _DLL
#define  _MT
#include <os2.h>
#include <rexxsaa.h>
#include <memory.h>
#include <malloc.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>


/*********************************************************************/
/*  Declare all exported functions as REXX functions.                */
/*********************************************************************/
RexxFunctionHandler VioLoadFuncs;
RexxFunctionHandler VioDropFuncs;
RexxFunctionHandler RxVioScrollLeft;
RexxFunctionHandler RxVioScrollRight;
RexxFunctionHandler RxVioScrollUp;
RexxFunctionHandler RxVioScrollDown;
RexxFunctionHandler RxVioReadCellStr;
RexxFunctionHandler RxVioWrtCellStr;
RexxFunctionHandler RxVioWrtCharStr;
RexxFunctionHandler RxVioWrtCharStrAttr;
RexxFunctionHandler RxVioGetCurType;
RexxFunctionHandler RxVioSetCurType;
RexxFunctionHandler RxVioWrtNAttr;
RexxFunctionHandler RxVioWrtNCell;
RexxFunctionHandler RxVioWrtNChar;

/*********************************************************************/
/*  Various definitions used by various functions.                   */
/*********************************************************************/

#define  MAX_DIGITS     9          /* maximum digits in numeric arg  */
#define  MAX            256        /* temporary buffer length        */
#define  IBUF_LEN       4096       /* Input buffer length            */
#define  AllocFlag      PAG_COMMIT | PAG_WRITE  /* for DosAllocMem   */


/*********************************************************************/
/* Structures used throughout REXXVIO.C                              */
/*********************************************************************/

/*********************************************************************/
/* RxStemData                                                        */
/*   Structure which describes as generic                            */
/*   stem variable.                                                  */
/*********************************************************************/

typedef struct RxStemData {
    SHVBLOCK shvb;                     /* Request block for RxVar    */
    CHAR ibuf[IBUF_LEN];               /* Input buffer               */
    CHAR varname[MAX];                 /* Buffer for the variable    */
                                       /* name                       */
    CHAR stemname[MAX];                /* Buffer for the variable    */
                                       /* name                       */
    ULONG stemlen;                     /* Length of stem.            */
    ULONG vlen;                        /* Length of variable value   */
    ULONG j;                           /* Temp counter               */
    ULONG tlong;                       /* Temp counter               */
    ULONG count;                       /* Number of elements         */
                                       /* processed                  */
} RXSTEMDATA;

/*********************************************************************/
/* RxFncTable                                                        */
/*   Array of names of the REXXVIO functions.                        */
/*   This list is used for registration and deregistration.          */
/*********************************************************************/

static PSZ  RxFncTable[] =
   {
      "VioLoadFuncs",
      "VioDropFuncs",
      "VioScrollLeft",
      "VioScrollRight",
      "VioScrollDown",
      "VioScrollUp",
      "VioReadCellStr",
      "VioWrtCellStr",
      "VioWrtCharStr",
      "VioWrtCharStrAttr",
      "VioGetCurType",
      "VioSetCurType",
      "VioWrtNAttr",
      "VioWrtNCell",
      "VioWrtNChar",
   };

/*********************************************************************/
/* Numeric Error Return Strings                                      */
/*********************************************************************/

#define  NO_UTIL_ERROR    "0"          /* No error whatsoever        */
#define  ERROR_NOMEM      "2"          /* Insufficient memory        */
#define  ERROR_FILEOPEN   "3"          /* Error opening text file    */

/*********************************************************************/
/* Alpha Numeric Return Strings                                      */
/*********************************************************************/

#define  ERROR_RETSTR   "ERROR:"

/*********************************************************************/
/* Numeric Return calls                                              */
/*********************************************************************/

#define  INVALID_ROUTINE 40            /* Raise Rexx error           */
#define  VALID_ROUTINE    0            /* Successful completion      */

/*********************************************************************/
/* Some useful macros                                                */
/*********************************************************************/

#define BUILDRXSTRING(t, s) { \
  strcpy((t)->strptr,(s));\
  (t)->strlength = strlen((s)); \
}


/*********************************************************************/
/*****************  REXXVIO Supporting Functions  ********************/
/*****************  REXXVIO Supporting Functions  ********************/
/*****************  REXXVIO Supporting Functions  ********************/
/*********************************************************************/

/********************************************************************
* Function:  string2long(string, number)                            *
*                                                                   *
* Purpose:   Validates and converts an ASCII-Z string from string   *
*            form to an signed long.  Returns FALSE if the number   *
*            is not valid, TRUE if the number was successfully      *
*            converted.                                             *
*                                                                   *
* RC:        TRUE - Good number converted                           *
*            FALSE - Invalid number supplied.                       *
*********************************************************************/

BOOL string2long(PSZ string, LONG *number)
{
  ULONG    accumulator;                /* converted number           */
  INT      length;                     /* length of number           */
  INT      sign;                       /* sign of number             */

  sign = 1;                            /* set default sign           */
  if (*string == '-') {                /* negative?                  */
    sign = -1;                         /* change sign                */
    string++;                          /* step past sign             */
  }

  length = strlen(string);             /* get length of string       */
  if (length == 0 ||                   /* if null string             */
      length > MAX_DIGITS)             /* or too long                */
    return FALSE;                      /* not valid                  */

  accumulator = 0;                     /* start with zero            */

  while (length) {                     /* while more digits          */
    if (!isdigit(*string))             /* not a digit?               */
      return FALSE;                    /* tell caller                */
                                       /* add to accumulator         */
    accumulator = accumulator *10 + (*string - '0');
    length--;                          /* reduce length              */
    string++;                          /* step pointer               */
  }

  *number = accumulator * sign;        /* return the value           */
  return TRUE;                         /* good number                */
}


/*************************************************************************
***              <<<<<< REXXVIO Functions Follow >>>>>>>               ***
***              <<<<<< REXXVIO Functions Follow >>>>>>>               ***
***              <<<<<< REXXVIO Functions Follow >>>>>>>               ***
***              <<<<<< REXXVIO Functions Follow >>>>>>>               ***
*************************************************************************/

/*************************************************************************
* Function:  VioDropFuncs                                                *
*                                                                        *
* Syntax:    call VioDropFuncs                                           *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG VioDropFuncs(CHAR *name, ULONG numargs, RXSTRING args[],
                          CHAR *queuename, RXSTRING *retstr)
{
  INT     entries;                     /* Num of entries             */
  INT     j;                           /* Counter                    */

  if (numargs != 0)                    /* no arguments for this      */
    return INVALID_ROUTINE;            /* raise an error             */

  retstr->strlength = 0;               /* return a null string result*/

  entries = sizeof(RxFncTable)/sizeof(PSZ);

  for (j = 0; j < entries; j++)
    RexxDeregisterFunction(RxFncTable[j]);

  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  VioLoadFuncs                                                *
*                                                                        *
* Syntax:    call VioLoadFuncs [option]                                  *
*                                                                        *
* Params:    none                                                        *
*                                                                        *
* Return:    null string                                                 *
*************************************************************************/

ULONG VioLoadFuncs(CHAR *name, ULONG numargs, RXSTRING args[],
                           CHAR *queuename, RXSTRING *retstr)
{
  INT    entries;                      /* Num of entries             */
  INT    j;                            /* Counter                    */

  retstr->strlength = 0;               /* set return value           */
                                       /* check arguments            */
  if (numargs > 0)
    return INVALID_ROUTINE;

  entries = sizeof(RxFncTable)/sizeof(PSZ);

  for (j = 0; j < entries; j++) {
    RexxRegisterFunctionDll(RxFncTable[j],
          "REXXVIO", RxFncTable[j]);
  }
  return VALID_ROUTINE;
}


/*************************************************************************
* Function:  RxVioScrollLeft                                               *
*                                                                        *
* Syntax:    call VioScrollLeft top, left, bottom, right, count [,x,y]   *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*            len - The number of characters to read.  The default is the *
*                   rest of the screen.                                  *
*            hvio- reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioScrollLeft(CHAR *name, ULONG numargs, RXSTRING args[],
                                  CHAR *queuename, RXSTRING *retstr)
{
  LONG  top;
  LONG  left;
  LONG  right;
  LONG  bottom;
  LONG  count;
  LONG  attr;
  BYTE bCell[2];                       /* Char/Attribute array       */

  bCell[0] = 0x20;                     /* Space Character            */
  bCell[1] = 0x07;                     /* Default Attrib             */

  if (numargs < 5 ||                   /* validate arguments         */
      numargs > 8 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !RXVALIDSTRING(args[2]) ||
      !RXVALIDSTRING(args[3]) ||
      !RXVALIDSTRING(args[4]) ||
      !string2long(args[0].strptr, &top) || top < 0 ||
      !string2long(args[1].strptr, &left) || left < 0 ||
      !string2long(args[2].strptr, &bottom) || bottom < 0 ||
      !string2long(args[3].strptr, &right) || right < 0 ||
      !string2long(args[4].strptr, &count) || count < 0)
    return INVALID_ROUTINE;

  if (numargs >= 6 && 
      RXVALIDSTRING(args[5]))
    bCell[0] = args[5].strptr[0] ;

  if (numargs >= 7 && 
      RXVALIDSTRING(args[6]) && 
      string2long(args[6].strptr, &attr) &&
      attr < 256)
    bCell[1] = (BYTE)attr;

  VioScrollLf(top, left, bottom, right, count, bCell, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioScrollRight                                            *
*                                                                        *
* Syntax:    call VioScrollRight top, left, bottom, right, count [,...]  *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*            len - The number of characters to read.  The default is the *
*                   rest of the screen.                                  *
*            hvio- reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioScrollRight(CHAR *name, ULONG numargs, RXSTRING args[],
                                   CHAR *queuename, RXSTRING *retstr)
{
  LONG  top;
  LONG  left;
  LONG  right;
  LONG  bottom;
  LONG  count;
  LONG  attr;
  BYTE bCell[2];                       /* Char/Attribute array       */

  bCell[0] = 0x20;                     /* Space Character            */
  bCell[1] = 0x07;                     /* Default Attrib             */

  if (numargs < 5 ||                   /* validate arguments         */
      numargs > 8 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !RXVALIDSTRING(args[2]) ||
      !RXVALIDSTRING(args[3]) ||
      !RXVALIDSTRING(args[4]) ||
      !string2long(args[0].strptr, &top) || top < 0 ||
      !string2long(args[1].strptr, &left) || left < 0 ||
      !string2long(args[2].strptr, &bottom) || bottom < 0 ||
      !string2long(args[3].strptr, &right) || right < 0 ||
      !string2long(args[4].strptr, &count) || count < 0)
    return INVALID_ROUTINE;

  if (numargs >= 6 && 
      RXVALIDSTRING(args[5]))
    bCell[0] = args[5].strptr[0] ;

  if (numargs >= 7 && 
      RXVALIDSTRING(args[6]) && 
      string2long(args[6].strptr, &attr) &&
      attr < 256)
    bCell[1] = (BYTE)attr;

  VioScrollRt(top, left, bottom, right, count, bCell, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioScrollDown                                             *
*                                                                        *
* Syntax:    call VioScrollDown top, left, bottom, right, count [,...]   *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*            len - The number of characters to read.  The default is the *
*                   rest of the screen.                                  *
*            hvio- reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioScrollDown(CHAR *name, ULONG numargs, RXSTRING args[],
                                  CHAR *queuename, RXSTRING *retstr)
{
  LONG  top;
  LONG  left;
  LONG  right;
  LONG  bottom;
  LONG  count;
  LONG  attr;
  BYTE bCell[2];                       /* Char/Attribute array       */

  bCell[0] = 0x20;                     /* Space Character            */
  bCell[1] = 0x07;                     /* Default Attrib             */

  if (numargs < 5 ||                   /* validate arguments         */
      numargs > 8 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !RXVALIDSTRING(args[2]) ||
      !RXVALIDSTRING(args[3]) ||
      !RXVALIDSTRING(args[4]) ||
      !string2long(args[0].strptr, &top) || top < 0 ||
      !string2long(args[1].strptr, &left) || left < 0 ||
      !string2long(args[2].strptr, &bottom) || bottom < 0 ||
      !string2long(args[3].strptr, &right) || right < 0 ||
      !string2long(args[4].strptr, &count) || count < 0)
    return INVALID_ROUTINE;

  if (numargs >= 6 && 
      RXVALIDSTRING(args[5]))
    bCell[0] = args[5].strptr[0] ;

  if (numargs >= 7 && 
      RXVALIDSTRING(args[6]) && 
      string2long(args[6].strptr, &attr) &&
      attr < 256)
    bCell[1] = (BYTE)attr;

  VioScrollDn(top, left, bottom, right, count, bCell, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioScrollUp                                               *
*                                                                        *
* Syntax:    call VioScrollUp top, left, bottom, right, count [,...]     *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*            len - The number of characters to read.  The default is the *
*                   rest of the screen.                                  *
*            hvio- reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioScrollUp(CHAR *name, ULONG numargs, RXSTRING args[],
                                CHAR *queuename, RXSTRING *retstr)
{
  LONG  top;
  LONG  left;
  LONG  right;
  LONG  bottom;
  LONG  count;
  LONG  attr;
  BYTE bCell[2];                       /* Char/Attribute array       */

  bCell[0] = 0x20;                     /* Space Character            */
  bCell[1] = 0x07;                     /* Default Attrib             */

  if (numargs < 5 ||                   /* validate arguments         */
      numargs > 8 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !RXVALIDSTRING(args[2]) ||
      !RXVALIDSTRING(args[3]) ||
      !RXVALIDSTRING(args[4]) ||
      !string2long(args[0].strptr, &top) || top < 0 ||
      !string2long(args[1].strptr, &left) || left < 0 ||
      !string2long(args[2].strptr, &bottom) || bottom < 0 ||
      !string2long(args[3].strptr, &right) || right < 0 ||
      !string2long(args[4].strptr, &count) || count < 0)
    return INVALID_ROUTINE;

  if (numargs >= 6 && 
      RXVALIDSTRING(args[5]))
    bCell[0] = args[5].strptr[0] ;

  if (numargs >= 7 && 
      RXVALIDSTRING(args[6]) && 
      string2long(args[6].strptr, &attr) &&
      attr < 256)
    bCell[1] = (BYTE)attr;

  VioScrollUp(top, left, bottom, right, count, bCell, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioReadCellStr                                            *
*                                                                        *
* Syntax:    call VioReadCellStr row, col [,[len] [,hvio]]               *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*            len - The number of cells to read.  The default is the rest *
*                   of the screen.                                       *
*            hvio- reserved                                              *
*                                                                        *
* Return:    Cells read from text screen.                                *
*************************************************************************/

ULONG RxVioReadCellStr(CHAR *name, ULONG numargs, RXSTRING args[],
                                CHAR *queuename, RXSTRING *retstr)
{
  LONG  row;                           /* Row from which to start    */
                                       /* read                       */
  LONG  col;                           /* Column from which to start */
                                       /* read                       */
  LONG len = 16384;                    /* Max length of string,      */
                                       /* default is 16384 (8192x2)  */
  CHAR  temp[16384];                   /* Array of CHARs, aka PCH    */

  if (numargs < 2 ||                   /* validate arguments         */
      numargs > 4 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !string2long(args[0].strptr, &row) || row < 0 ||
      !string2long(args[1].strptr, &col) || col < 0)
    return INVALID_ROUTINE;

  if (numargs >= 3) {                  /* check the length           */
    if (!RXVALIDSTRING(args[2]) ||     /* bad string?                */
        !string2long(args[2].strptr, &len) || len < 0)
      return INVALID_ROUTINE;          /* error                      */

    len *= 2;
    if (len > 16384)
      len = 16384;
  }

                                       /* read the screen            */
  VioReadCellStr( temp, (PUSHORT)&len, row, col, (HVIO) 0);

  if (len > retstr->strlength)         /* default too short?         */
                                       /* allocate a new one         */
    if (DosAllocMem((PPVOID)&retstr->strptr, len, AllocFlag)) {
      BUILDRXSTRING(retstr, ERROR_NOMEM);
      return VALID_ROUTINE;
    }
  memcpy(retstr->strptr, temp, len);
  retstr->strlength = len;

  return VALID_ROUTINE;
}


/*************************************************************************
* Function:  RxVioWrtCellStr                                             *
*                                                                        *
* Syntax:    call VioWrtCellStr row, col, str [,[len] [,hvio]]           *
*                                                                        *
* Params:    row - Horizontal row on the screen to start writing to.     *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start writing to.    *
*                   The column at the left of the screen is 0.           *
*            str - The cell-string to write.                             *
*            len - The number of cells to write.  The default is the     *
*                   whole string.                                        *
*            hvio- reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioWrtCellStr(CHAR *name, ULONG numargs, RXSTRING args[],
                                  CHAR *queuename, RXSTRING *retstr)
{
  LONG  row;                           /* Row from which to start    */
                                       /* writting                   */
  LONG  col;                           /* Column from which to start */
                                       /* writting                   */
  LONG len;                            /* Max length of string,      */

  if (numargs < 3 ||                   /* validate arguments         */
      numargs > 5 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !string2long(args[0].strptr, &row) || row < 0 ||
      !string2long(args[1].strptr, &col) || col < 0)
    return INVALID_ROUTINE;

  if (numargs >= 4 &&
      RXVALIDSTRING(args[3]) &&
      string2long(args[3].strptr, &len))
    {
    if (len < 0)
      return INVALID_ROUTINE;
    else
      len = len * 2;
    }
  else
    len = args[2].strlength;

  VioWrtCellStr(args[2].strptr, len, row, col, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioWrtCharStr                                             *
*                                                                        *
* Syntax:    call VioWrtCellStr row, col, str [,[len] [,hvio]]           *
*                                                                        *
* Params:    row - Horizontal row on the screen to start writing to.     *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start writing to.    *
*                   The column at the left of the screen is 0.           *
*            str - The string to write.                                  *
*            len - The number of characters to write.  The default is    *
*                   the whole string.                                    *
*            hvio- reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioWrtCharStr(CHAR *name, ULONG numargs, RXSTRING args[],
                                  CHAR *queuename, RXSTRING *retstr)
{
  LONG  row;                           /* Row from which to start    */
                                       /* writting                   */
  LONG  col;                           /* Column from which to start */
                                       /* writting                   */
  LONG len;                            /* Max length of string,      */

  if (numargs < 3 ||                   /* validate arguments         */
      numargs > 5 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !string2long(args[0].strptr, &row) || row < 0 ||
      !string2long(args[1].strptr, &col) || col < 0)
    return INVALID_ROUTINE;

  if (numargs >= 4 &&
      RXVALIDSTRING(args[3]) &&
      string2long(args[3].strptr, &len))
    len = len;
  else
    len = args[2].strlength;

  VioWrtCharStr(args[2].strptr, len, row, col, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioWrtCharStrAttr                                         *
*                                                                        *
* Syntax:    call VioWrtCellStr row, col, str [,[len] [,[attr] [,hvio]]] *
*                                                                        *
* Params:    row - Horizontal row on the screen to start writing to.     *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start writing to.    *
*                   The column at the left of the screen is 0.           *
*            str - The string to write.                                  *
*            len - The number of characters to write.  The default is    *
*                   the whole string.                                    *
*            attr- The string attribute                                  *
*            hvio- reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioWrtCharStrAttr(CHAR *name, ULONG numargs, RXSTRING args[],
                                  CHAR *queuename, RXSTRING *retstr)
{
  LONG  row;                           /* Row from which to start    */
                                       /* writting                   */
  LONG  col;                           /* Column from which to start */
                                       /* writting                   */
  LONG len;                            /* Max length of string,      */
  LONG attr;
  BYTE battr;

  if (numargs < 3 ||                   /* validate arguments         */
      numargs > 6 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !string2long(args[0].strptr, &row) || row < 0 ||
      !string2long(args[1].strptr, &col) || col < 0)
    return INVALID_ROUTINE;

  if (numargs >= 4 &&
      RXVALIDSTRING(args[3]) &&
      string2long(args[3].strptr, &len))
    len = len;
  else
    len = args[2].strlength;

  if (numargs >= 5 &&
      RXVALIDSTRING(args[4]) &&
      string2long(args[4].strptr, &attr) &&
      attr < 256)
    battr = (BYTE)attr;
  else
    battr = 7;

  VioWrtCharStrAtt(args[2].strptr, len, row, col, &battr, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioGetCurType                                             *
*                                                                        *
* Syntax:    curType = VioGetCurType([hvio])                             *
*                                                                        *
* Params:    hvio- reserved                                              *
*                                                                        *
* Return:    startline endline cursorwidth attr                          *
*************************************************************************/

ULONG RxVioGetCurType(CHAR *name, ULONG numargs, RXSTRING args[],
                                  CHAR *queuename, RXSTRING *retstr)
{
  VIOCURSORINFO vci;

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* set default result         */
                                       /* check arguments            */
  if (numargs > 1)                     /* wrong number?              */
    return INVALID_ROUTINE;            /* raise an error             */

  VioGetCurType(&vci, (HVIO) 0);

  sprintf(retstr->strptr, "%d %d %d %d", 
                          vci.yStart, vci.cEnd, vci.cx, vci.attr);
  retstr->strlength = strlen(retstr->strptr);

  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioSetCurType                                             *
*                                                                        *
* Syntax:    call VioSetCurType yStart, cEnd [,[cx] [,[attr] [,hvio]]]   *
*                                                                        *
* Params:    yStart - startline                                          *
*            cEnd   - endline                                            *
*            cx     - cursorwidth                                        *
*            attr   -                                                    *
*            hvio   - reserved                                           *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioSetCurType(CHAR *name, ULONG numargs, RXSTRING args[],
                                  CHAR *queuename, RXSTRING *retstr)
{
  VIOCURSORINFO vci;
  LONG startline;
  LONG endline;
  LONG cursorwidth;
  LONG attr;

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* set default result         */
                                       /* check arguments            */

  if (numargs < 2 ||                   /* validate arguments         */
      numargs > 5 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !string2long(args[0].strptr, &startline) ||
      !string2long(args[1].strptr, &endline) || endline > 31)
    return INVALID_ROUTINE;

  if (startline < 0)
    startline +=65535;
  if (endline < 0)
    endline +=65535;

  if (numargs >= 3 &&
      RXVALIDSTRING(args[2]) &&
      string2long(args[2].strptr, &cursorwidth))
    cursorwidth = cursorwidth;
  else
    cursorwidth = 0;

  if (cursorwidth > 1 || cursorwidth < 0)
    return INVALID_ROUTINE;

  if (numargs >= 4 &&
      RXVALIDSTRING(args[3]) &&
      string2long(args[3].strptr, &attr))
    attr = attr;
  else
    attr = 0;

  vci.yStart = startline;
  vci.cEnd = endline;
  vci.cx = cursorwidth;
  vci.attr = attr;

  VioSetCurType(&vci, (HVIO) 0);

  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioWrtNAttr                                               *
*                                                                        *
* Syntax:    call VioWrtNAttr row, col, count [,[attr] [,hvio]]          *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*          count - The number of characters to read.  The default is the *
*                   rest of the screen.                                  *
*           attr -                                                       *  
*           hvio - reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioWrtNAttr(CHAR *name, ULONG numargs, RXSTRING args[],
                                CHAR *queuename, RXSTRING *retstr)
{
  LONG  top;
  LONG  left;
  LONG  count;
  LONG  attr;
  BYTE bCell[1];                       /* Char/Attribute array       */

  bCell[0] = 0x07;                     /* Default Attrib             */

  if (numargs < 3 ||                   /* validate arguments         */
      numargs > 5 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !RXVALIDSTRING(args[2]) ||
      !string2long(args[0].strptr, &top) || top < 0 ||
      !string2long(args[1].strptr, &left) || left < 0 ||
      !string2long(args[2].strptr, &count) || count < 0)
    return INVALID_ROUTINE;

  if (numargs >= 4 && 
      RXVALIDSTRING(args[3]) && 
      string2long(args[3].strptr, &attr) &&
      attr < 256)
    bCell[0] = (BYTE)attr;

  VioWrtNAttr(bCell, count, top, left, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioWrtNCell                                               *
*                                                                        *
* Syntax:    call VioWrtNCell row, col, count [,[char][,[attr] [,hvio]]] *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*          count - The number of characters to read.  The default is the *
*                   rest of the screen.                                  *
*           char -                                                       *  
*           attr -                                                       *  
*           hvio - reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioWrtNCell(CHAR *name, ULONG numargs, RXSTRING args[],
                                CHAR *queuename, RXSTRING *retstr)
{
  LONG  top;
  LONG  left;
  LONG  count;
  LONG  attr;
  BYTE bCell[2];                       /* Char/Attribute array       */

  bCell[0] = 0x20;                     /* Default Char               */
  bCell[1] = 0x07;                     /* Default Attrib             */

  if (numargs < 3 ||                   /* validate arguments         */
      numargs > 6 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !RXVALIDSTRING(args[2]) ||
      !string2long(args[0].strptr, &top) || top < 0 ||
      !string2long(args[1].strptr, &left) || left < 0 ||
      !string2long(args[2].strptr, &count) || count < 0)
    return INVALID_ROUTINE;

  if (numargs >= 4 &&
      RXVALIDSTRING(args[3]))
    bCell[0] = args[3].strptr[0];

  if (numargs >= 5 && 
      RXVALIDSTRING(args[4]) && 
      string2long(args[4].strptr, &attr) &&
      attr < 256)
    bCell[1] = (BYTE)attr;

  VioWrtNCell(bCell, count, top, left, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  RxVioWrtNChar                                               *
*                                                                        *
* Syntax:    call VioWrtNChar row, col, count [,[char] [,hvio]]          *
*                                                                        *
* Params:    row - Horizontal row on the screen to start reading from.   *
*                   The row at the top of the screen is 0.               *
*            col - Vertical column on the screen to start reading from.  *
*                   The column at the left of the screen is 0.           *
*          count - The number of characters to read.  The default is the *
*                   rest of the screen.                                  *
*           char -                                                       *  
*           hvio - reserved                                              *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG RxVioWrtNChar(CHAR *name, ULONG numargs, RXSTRING args[],
                                CHAR *queuename, RXSTRING *retstr)
{
  LONG  top;
  LONG  left;
  LONG  count;
  LONG  attr;
  BYTE bCell[1];                       /* Char/Attribute array       */

  bCell[0] = 0x20;                     /* Default Char               */

  if (numargs < 3 ||                   /* validate arguments         */
      numargs > 5 ||
      !RXVALIDSTRING(args[0]) ||
      !RXVALIDSTRING(args[1]) ||
      !RXVALIDSTRING(args[2]) ||
      !string2long(args[0].strptr, &top) || top < 0 ||
      !string2long(args[1].strptr, &left) || left < 0 ||
      !string2long(args[2].strptr, &count) || count < 0)
    return INVALID_ROUTINE;

  if (numargs >= 4 && 
      RXVALIDSTRING(args[3]))
    bCell[0] = args[3].strptr[0];

  VioWrtNChar(bCell, count, top, left, (HVIO) 0);

  BUILDRXSTRING(retstr, NO_UTIL_ERROR);/* pass back result           */
  return VALID_ROUTINE;                /* no error on call           */
}


