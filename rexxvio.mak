# NMAKE-compatible MAKE file for the REXX sample program REXXVIO.DLL.

rexxvio.dll:     rexxvio.obj  rexxvio.def
         ILINK /NOFREE rexxvio.obj,rexxvio.dll,,REXX,rexxvio.def;

rexxvio.obj:     rexxvio.c
         icc -c -Ge- rexxvio.c


