/*
 *  An implementation of the ARC4 algorithm
 *
 *  Copyright (C) 2001-2003  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "rc4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char rc4Seed[]={
	184,8,103,25,92,114,32,132,55,121,209,180,
	218,8,22,174,104,164,115,88,18,21,86,73,48,
	49,69,219,177,25,250,165,9,246,148,203,135,
	214,234,98,172,111,139,194,41,190,162,104,
	177,219,109,32,87,158,240,196,76,174,197,2,
	87,31,222,30,194,103,106,135,97,254,57,145,
	78,155,136,141,167,41,232,15,194,15,191,253,
	195,198,250,112,149,134,191,67,26,60,75,207,
	80,252,87,182,111,79,228,128,176,32,5,67,146,
	197,255,161,126,161,128,85,15,70,34,4,181,76,
	32,0,157,224,83,157,9,34,50,110,64,75,146,108,
	147,47,64,157,94,74,187,29,95,53,123,33,81,160,
	100,18,57,227,105,250,183,152,94,44,80,120,26,
	0,159,199,167,88,196,129,136,242,43,64,169,241,
	39,216,136,236,186,167,190,222,243,127,127,115,
	206,248,191,253,255,251,115,232,198,44,35,178,8,
	240,183,237,84,119,205,250,33,80,175,176,45,128,
	185,220,246,200,250,46,208,165,28,203,37,46,52,
	146,136,175,16,94,249,143,7,189,137,114,202,114,
	186,131,145,113,89,250,254,131,202,134,105,66,
	104,76,70,119
};


void rc4_setup( struct rc4_state *s, unsigned char *key,  int length )
{
    int i, j, k, *m, a;

    s->x = 0;
    s->y = 0;
    m = s->m;

    for( i = 0; i < 256; i++ )
    {
        m[i] = rc4Seed[i];
     }

    j = k = 0;

    for( i = 0; i < 256; i++ )
    {
        a = m[i];
        j = (unsigned char) ( j + a + key[k] );
        m[i] = m[j]; m[j] = a;
        if( ++k >= length ) k = 0;
    }
} 

void rc4_crypt( struct rc4_state *s, unsigned char *data, int length )
{ 
    int i, x, y, *m, a, b;

    x = s->x;
    y = s->y;
    m = s->m;

    for( i = 0; i < length; i++ )
     {
        x = (unsigned char) ( x + 1 ); a = m[x];
        y = (unsigned char) ( y + a );
        m[x] = b = m[y];
        m[y] = a;
        data[i] ^= m[(unsigned char) ( a + b )];
    }

    s->x = x;
    s->y = y;
}
