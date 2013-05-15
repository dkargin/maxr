/***************************************************************************
 *      Mechanized Assault and Exploration Reloaded Projectfile            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef unifontsH
#define unifontsH

#include "autosurface.h"
#include <SDL.h>
#include <string>
#include "defines.h"
#include "video.h"


/** diffrent fonttypes*/
enum eUnicodeFontType
{
	FONT_LATIN_NORMAL,
	FONT_LATIN_BIG,
	FONT_LATIN_BIG_GOLD,
	FONT_LATIN_SMALL_WHITE,
	FONT_LATIN_SMALL_RED,
	FONT_LATIN_SMALL_GREEN,
	FONT_LATIN_SMALL_YELLOW,
};

/** diffrent sizes that fonttypes can have*/
enum eUnicodeFontSize
{
	FONT_SIZE_NORMAL,
	FONT_SIZE_BIG,
	FONT_SIZE_SMALL,
};

/** diffrent ISO-8559-X charsets*/
enum eUnicodeFontCharset
{
	CHARSET_ISO8559_ALL,	// main part of the charsets which is the same in all charsets
	CHARSET_ISO8559_1,
	CHARSET_ISO8559_2,
	CHARSET_ISO8559_3,
	CHARSET_ISO8559_4,
	CHARSET_ISO8559_5,
	CHARSET_ISO8559_6,
	CHARSET_ISO8559_7,
	CHARSET_ISO8559_8,
	CHARSET_ISO8559_9,
	CHARSET_ISO8559_10,
	CHARSET_ISO8559_11,
	CHARSET_ISO8559_12,		// doesn't exists but is just a placeholder that the enum-numbers are the same as the iso-numbers
	CHARSET_ISO8559_13,
	CHARSET_ISO8559_14,
	CHARSET_ISO8559_15,
	CHARSET_ISO8559_16,
};

/**
 * @author alzi alias DoctorDeath
 * Loads the fontbitmaps from a ISO-8859 structure to a unicode structure and handles theire output to the screen
*/
class cUnicodeFont
{
public:
	cUnicodeFont();

	/**
	 * Wrapper for showText for easy use of SDL_Rects
	 * @author beko
	 * @param rdest destination to start drawing
	 * @param sText text to draw
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @param surface SDL_Surface to draw on. Default is buffer
	 */
	void showText (SDL_Rect rDest, const std::string& sText, eUnicodeFontType fonttype = FONT_LATIN_NORMAL, SDL_Surface* surface = buffer, bool encode = true);
	/**
	 * Displays a text
	 * @author beko
	 * @param x position x to start drawing
	 * @param y position y to start drawing
	 * @param sText text to draw
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @param surface SDL_Surface to draw on. Default is buffer
	 */
	void showText (int x, int y, const std::string& sText, eUnicodeFontType fonttype = FONT_LATIN_NORMAL, SDL_Surface* surface = buffer, bool encode = true);
	/**
	 * Displays a text as block.<br><br> This does <b>not</b> allow blanks in line. Linebreaks are interpreted. Unneeded blanks will be snipped.<br><br>
	 * Example: "Headline\n\n This is my text for a textblock that get's linebreaked automagically"!
	 * @author beko
	 * @param rDest SDL_Rect for position and wide of textbox. Height is not taken care of!
	 * @param sText text to draw
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @param surface SDL_Surface to draw on. Default is buffer
	 */
	int showTextAsBlock (SDL_Rect rDest, const std::string& sText, eUnicodeFontType fonttype = FONT_LATIN_NORMAL, SDL_Surface* surface = buffer, bool encode = true);
	/**
	 * Displays a text centered on given X
	 * @author beko
	 * @param rDest DL_Rect for position.<br>Use X for position to center on.<br>Y is not taken care of!
	 * @param sText text to draw
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @param surface SDL_Surface to draw on. Default is buffer
	 */
	void showTextCentered (SDL_Rect rDest, const std::string& sText, eUnicodeFontType fonttype = FONT_LATIN_NORMAL, SDL_Surface* surface = buffer, bool encode = true);
	/**
	 * Displays a text centered on given X
	 * @author beko
	 * @param x Use X for position to center on.<br>Y is not taken care of!
	 * @param y position y to start drawing
	 * @param sText text to draw
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @param surface SDL_Surface to draw on. Default is buffer
	 */
	void showTextCentered (int x, int y, const std::string& sText, eUnicodeFontType fonttype = FONT_LATIN_NORMAL, SDL_Surface* surface = buffer, bool encode = true);
	/**
	 * Calculates the needed width for a text in pixels
	 * @author beko
	 * @param sText text to check
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @return needed width for text
	 */
	int getTextWide (const std::string& sText, eUnicodeFontType fonttype = FONT_LATIN_NORMAL, bool encode = true);
	/**
	 * Calculates the needed space for a text in pixels
	 * @author beko
	 * @param sText text to check
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @return SDL_Rect with needed width and height for text
	 */
	SDL_Rect getTextSize (const std::string& sText, eUnicodeFontType fonttype = FONT_LATIN_NORMAL, bool encode = true);
	/**
	 * Holds information of font height
	 * @author beko
	 * @param eBitmapFontType enum of fonttype. LATIN_NORMAL is default
	 * @return Height of fonttype in pixels
	 */
	int getFontHeight (eUnicodeFontType fonttype = FONT_LATIN_NORMAL);
	/**
	 * Holds information of font size
	 * @author alzi
	 * @param eBitmapFontType enum of fonttype.
	 * @return eUnicodeFontSize enum size of fonttype
	 */
	eUnicodeFontSize getFontSize (eUnicodeFontType fonttype) const;

	std::string shortenStringToSize (const std::string& str, int size, eUnicodeFontType fonttype);
private:
	// character surfaces. Since SDL maximal gives us the unicodes from BMP we maximaly need 0xFFFF surfaces
	AutoSurface charsNormal[0xFFFF];
	AutoSurface charsSmallWhite[0xFFFF];
	AutoSurface charsSmallGreen[0xFFFF];
	AutoSurface charsSmallRed[0xFFFF];
	AutoSurface charsSmallYellow[0xFFFF];
	AutoSurface charsBig[0xFFFF];
	AutoSurface charsBigGold[0xFFFF];

	/**
	 * loads all characters of a ISO table and fonttype.
	 * @author beko
	 * @param charset the charset which should be loaded.
	 * @param fonttype the fonttype which should be loaded.
	 */
	void loadChars (eUnicodeFontCharset charset, eUnicodeFontType fonttype);
	Uint32 getPixel32 (int x, int y, SDL_Surface* surface);
	/**
	 * returns the character array of a fonttype.
	 * @author alzi alias DoctorDeath
	 * @param fonttype the fonttype of which the chracter array should be returned.
	 * @return the character array for the fonttype.
	 */
	AutoSurface* getFontTypeSurfaces (eUnicodeFontType fonttype);
	/**
	 * loads the ISO-8859 bitmap font surface
	 * @author alzi alias DoctorDeath
	 * @param charset the charset which bitmap should be loaded.
	 * @param fonttype the fonttype which bitmap should be loaded.
	 * @return the bitmap surface
	 */
	SDL_Surface* loadCharsetSurface (eUnicodeFontCharset charset, eUnicodeFontType fonttype);
	/**
	 * returns the iso page with the unicode positions of the characters in a ISO-8859 font
	 * @author alzi alias DoctorDeath
	 * @param charset the charset for that the iso page should be returned.
	 * @return the iso page
	 */
	const unsigned short* getIsoPage (eUnicodeFontCharset charset);
	int drawWithBreakLines (SDL_Rect rDest, const std::string& sText, eUnicodeFontType fonttype, SDL_Surface* surface, bool encode);
	/**
	 * encodes a UTF-8 character to his unicode position
	 * @author alzi alias DoctorDeath
	 * @param pch pointer to the character string
	 * @param increase number which will be changed to the value how much bytes the character has taken in UTF-8
	 * @return unicode position
	 */
	Uint16 encodeUTF8Char (const unsigned char* pch, int* increase) const;
};

EX cUnicodeFont* font;


/*
 * The following stuff is part of the GNU LIBICONV Library.
 * Copyright (C) 1999-2001 Free Software Foundation, Inc.
 *
 * The GNU LIBICONV Library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * The pages to assign the iso glyphes to their unicode matches have
 * been taken from the "libiconv" project.
 * The author is "Bruno Haible" <bruno@clisp.org>
 * To get more information about libiconv vist
 * http://www.gnu.org/software/libiconv/
 */

static const unsigned short iso8859_2_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x0104, 0x02d8, 0x0141, 0x00a4, 0x013d, 0x015a, 0x00a7,
	0x00a8, 0x0160, 0x015e, 0x0164, 0x0179, 0x00ad, 0x017d, 0x017b,
	/* 0xb0 */
	0x00b0, 0x0105, 0x02db, 0x0142, 0x00b4, 0x013e, 0x015b, 0x02c7,
	0x00b8, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c,
	/* 0xc0 */
	0x0154, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139, 0x0106, 0x00c7,
	0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce, 0x010e,
	/* 0xd0 */
	0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7,
	0x0158, 0x016e, 0x00da, 0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df,
	/* 0xe0 */
	0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7,
	0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f,
	/* 0xf0 */
	0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00f7,
	0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9,
};

static const unsigned short iso8859_3_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x0126, 0x02d8, 0x00a3, 0x00a4, 0xfffd, 0x0124, 0x00a7,
	0x00a8, 0x0130, 0x015e, 0x011e, 0x0134, 0x00ad, 0xfffd, 0x017b,
	/* 0xb0 */
	0x00b0, 0x0127, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x0125, 0x00b7,
	0x00b8, 0x0131, 0x015f, 0x011f, 0x0135, 0x00bd, 0xfffd, 0x017c,
	/* 0xc0 */
	0x00c0, 0x00c1, 0x00c2, 0xfffd, 0x00c4, 0x010a, 0x0108, 0x00c7,
	0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	/* 0xd0 */
	0xfffd, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x0120, 0x00d6, 0x00d7,
	0x011c, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x016c, 0x015c, 0x00df,
	/* 0xe0 */
	0x00e0, 0x00e1, 0x00e2, 0xfffd, 0x00e4, 0x010b, 0x0109, 0x00e7,
	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	/* 0xf0 */
	0xfffd, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x0121, 0x00f6, 0x00f7,
	0x011d, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x016d, 0x015d, 0x02d9,
};

static const unsigned short iso8859_4_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x0104, 0x0138, 0x0156, 0x00a4, 0x0128, 0x013b, 0x00a7,
	0x00a8, 0x0160, 0x0112, 0x0122, 0x0166, 0x00ad, 0x017d, 0x00af,
	/* 0xb0 */
	0x00b0, 0x0105, 0x02db, 0x0157, 0x00b4, 0x0129, 0x013c, 0x02c7,
	0x00b8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014a, 0x017e, 0x014b,
	/* 0xc0 */
	0x0100, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x012e,
	0x010c, 0x00c9, 0x0118, 0x00cb, 0x0116, 0x00cd, 0x00ce, 0x012a,
	/* 0xd0 */
	0x0110, 0x0145, 0x014c, 0x0136, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
	0x00d8, 0x0172, 0x00da, 0x00db, 0x00dc, 0x0168, 0x016a, 0x00df,
	/* 0xe0 */
	0x0101, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x012f,
	0x010d, 0x00e9, 0x0119, 0x00eb, 0x0117, 0x00ed, 0x00ee, 0x012b,
	/* 0xf0 */
	0x0111, 0x0146, 0x014d, 0x0137, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
	0x00f8, 0x0173, 0x00fa, 0x00fb, 0x00fc, 0x0169, 0x016b, 0x02d9,
};

static const unsigned short iso8859_5_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407,
	0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x00ad, 0x040e, 0x040f,
	/* 0xb0 */
	0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
	0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f,
	/* 0xc0 */
	0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
	0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f,
	/* 0xd0 */
	0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
	0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
	/* 0xe0 */
	0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
	0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f,
	/* 0xf0 */
	0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457,
	0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x00a7, 0x045e, 0x045f,
};

static const unsigned short iso8859_6_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0xfffd, 0xfffd, 0xfffd, 0x00a4, 0xfffd, 0xfffd, 0xfffd,
	0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x060c, 0x00ad, 0xfffd, 0xfffd,
	/* 0xb0 */
	0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
	0xfffd, 0xfffd, 0xfffd, 0x061b, 0xfffd, 0xfffd, 0xfffd, 0x061f,
	/* 0xc0 */
	0xfffd, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627,
	0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f,
	/* 0xd0 */
	0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637,
	0x0638, 0x0639, 0x063a, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
	/* 0xe0 */
	0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647,
	0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f,
	/* 0xf0 */
	0x0650, 0x0651, 0x0652, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
	0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
};

static const unsigned short iso8859_7_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x2018, 0x2019, 0x00a3, 0x20ac, 0x20af, 0x00a6, 0x00a7,
	0x00a8, 0x00a9, 0x037a, 0x00ab, 0x00ac, 0x00ad, 0xfffd, 0x2015,
	/* 0xb0 */
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0384, 0x0385, 0x0386, 0x00b7,
	0x0388, 0x0389, 0x038a, 0x00bb, 0x038c, 0x00bd, 0x038e, 0x038f,
	/* 0xc0 */
	0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
	0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
	/* 0xd0 */
	0x03a0, 0x03a1, 0xfffd, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
	0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af,
	/* 0xe0 */
	0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
	0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
	/* 0xf0 */
	0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
	0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0xfffd,
};

static const unsigned short iso8859_8_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0xfffd, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
	0x00a8, 0x00a9, 0x00d7, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
	/* 0xb0 */
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
	0x00b8, 0x00b9, 0x00f7, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0xfffd,
	/* 0xc0 */
	0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
	0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
	/* 0xd0 */
	0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
	0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x2017,
	/* 0xe0 */
	0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7,
	0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df,
	/* 0xf0 */
	0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7,
	0x05e8, 0x05e9, 0x05ea, 0xfffd, 0xfffd, 0x200e, 0x200f, 0xfffd,
};

static const unsigned short iso8859_9_2uni[48] =
{
	/* 0xd0 */
	0x011e, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
	0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0130, 0x015e, 0x00df,
	/* 0xe0 */
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	/* 0xf0 */
	0x011f, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
	0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x0131, 0x015f, 0x00ff,
};

static const unsigned short iso8859_10_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x0104, 0x0112, 0x0122, 0x012a, 0x0128, 0x0136, 0x00a7,
	0x013b, 0x0110, 0x0160, 0x0166, 0x017d, 0x00ad, 0x016a, 0x014a,
	/* 0xb0 */
	0x00b0, 0x0105, 0x0113, 0x0123, 0x012b, 0x0129, 0x0137, 0x00b7,
	0x013c, 0x0111, 0x0161, 0x0167, 0x017e, 0x2015, 0x016b, 0x014b,
	/* 0xc0 */
	0x0100, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x012e,
	0x010c, 0x00c9, 0x0118, 0x00cb, 0x0116, 0x00cd, 0x00ce, 0x00cf,
	/* 0xd0 */
	0x00d0, 0x0145, 0x014c, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x0168,
	0x00d8, 0x0172, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
	/* 0xe0 */
	0x0101, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x012f,
	0x010d, 0x00e9, 0x0119, 0x00eb, 0x0117, 0x00ed, 0x00ee, 0x00ef,
	/* 0xf0 */
	0x00f0, 0x0146, 0x014d, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x0169,
	0x00f8, 0x0173, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x0138,
};

// ISO-8859-11 isn't supportet yet

// ISO-8859-12 doesn't exists

static const unsigned short iso8859_13_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x201d, 0x00a2, 0x00a3, 0x00a4, 0x201e, 0x00a6, 0x00a7,
	0x00d8, 0x00a9, 0x0156, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00c6,
	/* 0xb0 */
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x201c, 0x00b5, 0x00b6, 0x00b7,
	0x00f8, 0x00b9, 0x0157, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00e6,
	/* 0xc0 */
	0x0104, 0x012e, 0x0100, 0x0106, 0x00c4, 0x00c5, 0x0118, 0x0112,
	0x010c, 0x00c9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012a, 0x013b,
	/* 0xd0 */
	0x0160, 0x0143, 0x0145, 0x00d3, 0x014c, 0x00d5, 0x00d6, 0x00d7,
	0x0172, 0x0141, 0x015a, 0x016a, 0x00dc, 0x017b, 0x017d, 0x00df,
	/* 0xe0 */
	0x0105, 0x012f, 0x0101, 0x0107, 0x00e4, 0x00e5, 0x0119, 0x0113,
	0x010d, 0x00e9, 0x017a, 0x0117, 0x0123, 0x0137, 0x012b, 0x013c,
	/* 0xf0 */
	0x0161, 0x0144, 0x0146, 0x00f3, 0x014d, 0x00f5, 0x00f6, 0x00f7,
	0x0173, 0x0142, 0x015b, 0x016b, 0x00fc, 0x017c, 0x017e, 0x2019,
};

static const unsigned short iso8859_14_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x1e02, 0x1e03, 0x00a3, 0x010a, 0x010b, 0x1e0a, 0x00a7,
	0x1e80, 0x00a9, 0x1e82, 0x1e0b, 0x1ef2, 0x00ad, 0x00ae, 0x0178,
	/* 0xb0 */
	0x1e1e, 0x1e1f, 0x0120, 0x0121, 0x1e40, 0x1e41, 0x00b6, 0x1e56,
	0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61,
	/* 0xc0 */
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
	0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	/* 0xd0 */
	0x0174, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x1e6a,
	0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x0176, 0x00df,
	/* 0xe0 */
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	/* 0xf0 */
	0x0175, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x1e6b,
	0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x0177, 0x00ff,
};

static const unsigned short iso8859_15_2uni[32] =
{
	/* 0xa0 */
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x20ac, 0x00a5, 0x0160, 0x00a7,
	0x0161, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
	/* 0xb0 */
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x017d, 0x00b5, 0x00b6, 0x00b7,
	0x017e, 0x00b9, 0x00ba, 0x00bb, 0x0152, 0x0153, 0x0178, 0x00bf,
};

static const unsigned short iso8859_16_2uni[96] =
{
	/* 0xa0 */
	0x00a0, 0x0104, 0x0105, 0x0141, 0x20ac, 0x201e, 0x0160, 0x00a7,
	0x0161, 0x00a9, 0x0218, 0x00ab, 0x0179, 0x00ad, 0x017a, 0x017b,
	/* 0xb0 */
	0x00b0, 0x00b1, 0x010c, 0x0142, 0x017d, 0x201d, 0x00b6, 0x00b7,
	0x017e, 0x010d, 0x0219, 0x00bb, 0x0152, 0x0153, 0x0178, 0x017c,
	/* 0xc0 */
	0x00c0, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0106, 0x00c6, 0x00c7,
	0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	/* 0xd0 */
	0x0110, 0x0143, 0x00d2, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x015a,
	0x0170, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0118, 0x021a, 0x00df,
	/* 0xe0 */
	0x00e0, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x0107, 0x00e6, 0x00e7,
	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	/* 0xf0 */
	0x0111, 0x0144, 0x00f2, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x015b,
	0x0171, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x0119, 0x021b, 0x00ff,
};

#endif
