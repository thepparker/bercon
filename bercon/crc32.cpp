/*
    Copyright (C) 2012  Prithu "bladez" Parker

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <inttypes.h>
#include "crc32.h"

#include <iostream>
#include <stdio.h>

using namespace std;

crc32::crc32(void)
{
    this->init();
}

crc32::~crc32(void)
{
    //nothing to deconstruct as of yet
}

void crc32::init(void)
{
    /*need to create the look up table*/
    uint32_t rem;

    memset(this->lookupTable, 0, sizeof(this->lookupTable));

    //256 values for all ASCII character codes
    for (int i = 0; i < 256; i++)
    {
        rem = i;

        //loop through 8 bits
        for (int j = 0; j < 8; j++)
        {
            if (rem & 1)
            {
                rem >>= 1;
                rem ^= 0xedb88320;
            }
            else
                rem >>= 1;
        }
        this->lookupTable[i] = rem;
    }
}

uint32_t crc32::calc_crc32(const char *buf, size_t len)
{
    //cout << "\nStr Length: " << len << endl;

    /*for (int i = 0; i < len; i++)
        printf("%d:", buf[i]);
    printf("\n");*/

    uint32_t iCRC = 0; //initial value of the crc
    unsigned int octet;

    const char *q, *p;

    iCRC = ~iCRC;

    q = buf + len;
    //loop through all chars in the string
    for (p = buf; p < q; p++)
    {
        if (*p < 0) //had problems where negatives where massive, so i wrapped them
            octet = 256 + *p;
        else
            octet = *p; /*cast to unsigned octet*/
        //printf("octet: %u\n", octet);
        iCRC = (iCRC >> 8) ^ this->lookupTable[(iCRC & 255) ^ octet];
    }
    return ~iCRC;
}
