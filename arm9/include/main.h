/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#ifndef __main9_h__
#define __main9_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "VideoGL.h"
#include "VideoGLExt.h"

#endif


#ifdef __cplusplus
extern "C" {
#endif

extern int main(int argc, char ** argv);
//TGDS Dir API: Directory Iterator(s)
extern struct FileClassList * menuIteratorfileClassListCtx;			//Menu Directory Iterator
extern char curChosenBrowseFile[256+1];
extern char globalPath[MAX_TGDSFILENAME_LENGTH+1];
extern int internalCodecType;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or ADPCM

extern char args[8][MAX_TGDSFILENAME_LENGTH];
extern char *argvs[8];

extern GLuint	texture[1];			// Storage For 1 Texture
extern GLuint	box;				// Storage For The Box Display List
extern GLuint	top;				// Storage For The Top Display List
extern GLuint	xloop;				// Loop For X Axis
extern GLuint	yloop;				// Loop For Y Axis

extern GLfloat	xrot;				// Rotates Cube On The X Axis
extern GLfloat	yrot;				// Rotates Cube On The Y Axis

extern GLfloat boxcol[5][3];
extern GLfloat topcol[5][3];
extern GLvoid BuildLists();
extern GLvoid ReSizeGLScene(GLsizei width, GLsizei height);
extern int InitGL();
extern int DrawGLScene();

#ifdef __cplusplus
}
#endif