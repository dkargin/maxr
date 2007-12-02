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

#include <string.h>
#include <sstream>
#include "fonts.h"
#include "files.h"
#include "main.h"

#define DEBUGFONTS false

cBitmapFont::cBitmapFont()
{
	iLoadedCharset = -1;
	//workingsurface - points always to loaded charset
	sfTmp= NULL;
	//surfaces for default font charsets
	sfLatinNormal= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,192,8,0,0,0,0 );
	sfLatinBig= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,256,8,0,0,0,0 );
	sfLatinBigGold= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,256,8,0,0,0,0 );
	sfLatinSmallWhite= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,128,128,8,0,0,0,0 );

	//surfaces for additional languagedepending charset
	SDL_Surface *sfBigISO = SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,96,8,0,0,0,0 );
	SDL_Surface *sfNormalISO = SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,72,8,0,0,0,0 );
	SDL_Surface *sfSmallISO = SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,48,8,0,0,0,0 );

	SDL_Rect rIsoDest = { 0, sfLatinNormal->h - sfNormalISO->h, sfNormalISO->w, sfNormalISO->h}; //blit additional char to lower side of ascii chart
	
	string sFontPath = SettingsData.sFontPath + PATH_DELIMITER;
	string sISO8850;
	
	setLang(); //init lang
	
	cLog::write ( "Languagecode "+SettingsData.sLanguage+" set to " + iToStr(iLangCode) + ". Loading charset iso-8559-" + iToStr(getIsoTable(getLang())), cLog::eLOG_TYPE_DEBUG );
	
	//BEGIN NORMAL FONTS
	//load fonts
	sfLatinNormal = LoadPCX((char*)(sFontPath + "latin_normal.pcx").c_str());
	
	//load languagedepending charset
	sISO8850 = sFontPath + "latin_normal_iso-8559-" + iToStr(getIsoTable(getLang())) + ".pcx";
	if(FileExists(sISO8850.c_str()))
	{
		sfNormalISO = LoadPCX((char*)sISO8850.c_str());  //get file for languagedepending charset
		SDL_BlitSurface(sfNormalISO, NULL, sfLatinNormal, &rIsoDest); //blit special characters of lang to fonts
	}

	buildFont(sfLatinNormal); //calculate offsets
	copyArray(chars, LatinNormal); 	//store offsets
	//END NORMAL FONTS
	
	//BEGIN SMALL FONTS
	//adjust dest rect for small fonts
	rIsoDest.y = sfLatinSmallWhite->h - sfSmallISO->h;
	rIsoDest.w = sfSmallISO->w;
	rIsoDest.h = sfSmallISO->h;
	
	//load fonts
	sfLatinSmallWhite = LoadPCX((char*)(sFontPath + "latin_small.pcx").c_str());
		
	//load languagedepending charset
	sISO8850 = sFontPath + "latin_small_iso-8559-" + iToStr(getIsoTable(getLang())) + ".pcx";
	if(FileExists(sISO8850.c_str()))
	{
		sfSmallISO = LoadPCX((char*)sISO8850.c_str());  //get file for languagedepending charset
		SDL_BlitSurface(sfSmallISO, NULL, sfLatinSmallWhite, &rIsoDest); //blit special characters of lang to fonts
	}
	
	buildFont(sfLatinSmallWhite); //calculate offsets	
	copyArray(chars, LatinSmall); //store offsets
	
	//init other font colours
	sfLatinSmallRed = SDL_CreateRGBSurface ( SDL_HWSURFACE, sfLatinSmallWhite->w, sfLatinSmallWhite->h, SettingsData.iColourDepth,0,0,0,0 );
	sfLatinSmallYellow = SDL_CreateRGBSurface ( SDL_HWSURFACE, sfLatinSmallWhite->w, sfLatinSmallWhite->h, SettingsData.iColourDepth,0,0,0,0 );
	sfLatinSmallGreen = SDL_CreateRGBSurface ( SDL_HWSURFACE, sfLatinSmallWhite->w, sfLatinSmallWhite->h, SettingsData.iColourDepth,0,0,0,0 );

	//note: I don't like it to hardcode the font's colour but I don't want to read it out manually just in case somebody doesn't like my master fonts so if somebody changes the fonts colourtable he's lost in the woods -- beko
	SDL_SetColorKey ( sfLatinSmallWhite,SDL_SRCCOLORKEY, 0xf0d8b8 ); //set transparency to pseudo-white
	
	SDL_FillRect ( sfLatinSmallRed, NULL, 0xe60000); //fill with red (ish)
	SDL_FillRect ( sfLatinSmallGreen, NULL, 0x04ae04); //fill with green (ish)
	SDL_FillRect ( sfLatinSmallYellow, NULL, 0xdbde00); //fill with yellow(ish)
	
	SDL_BlitSurface ( sfLatinSmallWhite, NULL, sfLatinSmallRed, NULL); //blit small fonts "background" into color red
	SDL_BlitSurface ( sfLatinSmallWhite, NULL, sfLatinSmallGreen, NULL); //blit small fonts "background" into color green
	SDL_BlitSurface ( sfLatinSmallWhite, NULL, sfLatinSmallYellow, NULL); //blit small fonts "background" into color yellow
	
	SDL_SetColorKey ( sfLatinSmallRed, SDL_SRCCOLORKEY,0xFF00FF ); //set transparency to mangenta
	SDL_SetColorKey ( sfLatinSmallGreen, SDL_SRCCOLORKEY,0xFF00FF ); //set transparency to mangenta
	SDL_SetColorKey ( sfLatinSmallYellow, SDL_SRCCOLORKEY,0xFF00FF ); //set transparency to mangenta
	SDL_SetColorKey ( sfLatinSmallWhite,SDL_SRCCOLORKEY, 0xFF00FF ); //set transparency for white font back to magenta
	//END SMALL FONTS
	
	//BEGIN BIG FONTS
	//adjust dest rect for big fonts
	rIsoDest.y = sfLatinBig->h - sfBigISO->h;
	rIsoDest.w = sfBigISO->w;
	rIsoDest.h = sfBigISO->h;
	
	//load fonts
	sfLatinBig = LoadPCX((char*)(sFontPath + "latin_big.pcx").c_str());
	
	//load languagedepending charset
	sISO8850 = sFontPath + "latin_big_iso-8559-" + iToStr(getIsoTable(getLang())) + ".pcx";
	if(FileExists(sISO8850.c_str()))
	{
		sfBigISO = LoadPCX((char*)sISO8850.c_str());  //get file for languagedepending charset
		SDL_BlitSurface(sfBigISO, NULL, sfLatinBig, &rIsoDest); //blit special characters of lang to fonts
	}
	
	buildFont(sfLatinBig); //calculate offsets	
	copyArray(chars, LatinBig); //store offsets

	//load fonts
	sfLatinBigGold = LoadPCX((char*)(sFontPath + "latin_big_gold.pcx").c_str());
	
	//load languagedepending charset
	sISO8850 = sFontPath + "latin_big_gold_iso-8559-" + iToStr(getIsoTable(getLang())) + ".pcx";
	if(FileExists(sISO8850.c_str()))
	{
		sfBigISO = LoadPCX((char*)sISO8850.c_str());  //get file for languagedepending charset
		SDL_BlitSurface(sfBigISO, NULL, sfLatinBigGold, &rIsoDest); //blit special characters of lang to fonts
	}
	
	buildFont(sfLatinBigGold); //calculate offsets	
	copyArray(chars, LatinBigGold); //store offsets
	
	SDL_FreeSurface(sfSmallISO);
	SDL_FreeSurface(sfNormalISO);
	SDL_FreeSurface(sfBigISO);
	//END BIG FONTS
}

cBitmapFont::~cBitmapFont()
{
	/*FIXME: dunno why but app crashes when I want to free my surfaces
	SDL_FreeSurface(sfLatinNormal);
	SDL_FreeSurface(sfLatinBig);
	SDL_FreeSurface(sfLatinBigGold);
	SDL_FreeSurface(sfLatinSmallWhite);
	SDL_FreeSurface(sfLatinSmallRed);
	SDL_FreeSurface(sfLatinSmallGreen);
	SDL_FreeSurface(sfLatinSmallYellow);
	SDL_FreeSurface(sfTmp); */
}

int cBitmapFont::setLang(void)
{
	string sLang = SettingsData.sLanguage;
	
	for(int i=0; i < sLang.size(); i++)
	{
		sLang[i] = toupper(sLang[i]);
	}
	
	iLangCode = -1;
	do
	{
		if(sLang == "GER") { iLangCode = GER; continue; }	//German
		else if (sLang == "ENG") { iLangCode = ENG; continue; }	//English
		else if (sLang == "RUS") { iLangCode = RUS; continue; }	//Russian
		else if (sLang == "ALB") { iLangCode = 	ALB; continue; }	//Alban
		else if (sLang == "DAN") { iLangCode = 	DAN; continue; }	//Danish
		else if (sLang == "FAO") { iLangCode = 	FAO; continue; }	//Faroese
		else if (sLang == "FIN") { iLangCode = 	FIN; continue; }	//Finnish
		else if (sLang == "FRA") { iLangCode = 	FRA; continue; }	//French
		else if (sLang == "FRE") { iLangCode = 	FRE; continue; }	//French
		else if (sLang == "GLE") { iLangCode = 	GLE; continue; }	//Irish
		else if (sLang == "ICE") { iLangCode = 	ICE; continue; }	//Icelandic
		else if (sLang == "ISL") { iLangCode = 	ISL; continue; }	//Icelandic
		else if (sLang == "ITA") { iLangCode = 	ITA; continue; }	//Italian
		else if (sLang == "CAT") { iLangCode = 	CAT; continue; }	//Catalan Valencian
		else if (sLang == "DUT") { iLangCode = 	DUT; continue; }	//Dutch Flemish
		else if (sLang == "NLD") { iLangCode = 	NLD; continue; }	//Dutch Flemish
		else if (sLang == "NNO") { iLangCode = 	NNO; continue; }	//Norwegian Nynorsk
		else if (sLang == "NOB") { iLangCode = 	NOB; continue; }	//Norwegian Bokm�l
		else if (sLang == "POR") { iLangCode = 	POR; continue; }	//Portuguese
		else if (sLang == "SWE") { iLangCode = 	SWE; continue; }	//Swedish
		else if (sLang == "SPA") { iLangCode = 	SPA; continue; }	//Spanish
		else if (sLang == "HRV") { iLangCode = HRV; continue; }	//Croatian
		else if (sLang == "SCR") { iLangCode = 	SCR; continue; }	//Croatian
		else if (sLang == "POL") { iLangCode = 	POL; continue; }	//Polish
		else if (sLang == "RON") { iLangCode = 	RON; continue; }	//Romanian
		else if (sLang == "RUM") { iLangCode = 	RUM; continue; }	//Romanian
		else if (sLang == "SLK") { iLangCode = 	SLK; continue; }	//Slovak
		else if (sLang == "SLO") { iLangCode = 	SLO; continue; }	//Slovak
		else if (sLang == "SLV") { iLangCode = 	SLV; continue; }	//Slovenian
		else if (sLang == "CES") { iLangCode = 	CES; continue; }	//Czech
		else if (sLang == "CZE") { iLangCode = 	CZE; continue; }	//Czech
		else if (sLang == "HUN") { iLangCode = 	HUN; continue; }	//Hungarian
		else if (sLang == "EPO") { iLangCode = 	EPO; continue; }	//Esperanto
		else if (sLang == "GLG") { iLangCode = 	GLG; continue; }	//Galician
		else if (sLang == "MLT") { iLangCode = 	MLT; continue; }	//Maltese
		else if (sLang == "TUR") { iLangCode = 	TUR; continue; }	//Turkish
		else if (sLang == "EST") { iLangCode = 	EST; continue; }	//Estonian
		else if (sLang == "LAV") { iLangCode = 	LAV; continue; }	//Latvian
		else if (sLang == "LIT") { iLangCode = 	LIT; continue; }	//Lithuanian
		else if (sLang == "BUL") { iLangCode = 	BUL; continue; }	//Bulgarian
		else if (sLang == "MAK") { iLangCode = 	MAK; continue; }	//Macedonian
		else if (sLang == "MKD") { iLangCode = 	MKD; continue; }	//Macedonian
		else if (sLang == "RUS") { iLangCode = 	RUS; continue; }	//Russian
		else if (sLang == "SCC") { iLangCode = 	SCC; continue; }	//Serbian
		else if (sLang == "SRP") { iLangCode = 	SRP; continue; }	//Serbian
		else if (sLang == "UKR") { iLangCode = 	UKR; continue; }	//Ukrainian		
		else { iLangCode = ENG; continue; } //English iso-8559-1 for all unknown language tags
	}
	while(iLangCode == -1);
	return iLangCode;
}

int cBitmapFont::getIsoTable(int eFontLangCode)
{	
	//returns iso-8559-$code
	switch(eFontLangCode)
	{
		//BEGIN LATIN-5 iso-8559-5
		case BUL:	//Bulgarian
		case MAK:	//Macedonian
		case MKD:	//Macedonian
		case RUS:	//Russian
		case SCC:	//Serbian
		case SRP:	//Serbian
		case UKR:	//Ukrainian
			return 5;
		//END LATIN-5 iso-8559-5
		//BEGIN LATIN-4 iso-8559-4
		case EST:	//Estonian
		case LAV:	//Latvian
		case LIT:	//Lithuanian
			return 4;
		//END LATIN-4 iso-8559-4
		//BEGIN LATIN-3 iso-8559-3
		case EPO:	//Esperanto
		case GLG:	//Galician
		case MLT:	//Maltese
		case TUR:	//Turkish
			return 3;
		//END LATIN-3 iso-8559-3
		//BEGIN LATIN-2 iso-8559-2
		case HRV:	//Croatian
		case SCR:	//Croatian
		case POL:	//Polish
		case RON:	//Romanian
		case RUM:	//Romanian
		case SLK:	//Slovak
		case SLO:	//Slovak
		case SLV:	//Slovenian
		case CES:	//Czech
		case CZE:	//Czech
		case HUN:	//Hungarian
			return 2;
		//END LATIN-2 iso-8559-2
		//BEGIN LATIN-1 iso-8559-1
		/* no need to check. there are all left and fallback for unknown is 1 anyway.
		case ENG:	//English iso-8559-1
		case GER:	//German
		case ALB:	//Alban
		case DAN:	//Danish
		case FAO:	//Faroese
		case FIN:	//Finnish
		case FRA:	//French
		case FRE:	//French
		case GLE:	//Irish
		case ICE:	//Icelandic
		case ISL:	//Icelandic
		case ITA:	//Italian
		case CAT:	//Catalan; Valencian
		case DUT:	//Dutch; Flemish
		case NLD:	//Dutch; Flemish
		case NNO:	//Norwegian Nynorsk
		case NOB:	//Norwegian Bokm�l
		case POR:	//Portuguese
		case SWE:	//Swedish
		case SPA:	//Spanish
		*/
		default:
			return 1;
		//END LATIN-1 iso-8559-1
	}
}

int cBitmapFont::getLang(void)
{
	return iLangCode;
}

void cBitmapFont::copyArray(SDL_Rect source[256],SDL_Rect dest[256])
{	
	for(int i=0; i < 256; i++)
	{
		dest[i].x = source[i].x;
		dest[i].y = source[i].y;
		dest[i].w = source[i].w;
		dest[i].h = source[i].h;
	}
}

void cBitmapFont::showTextAsBlock ( SDL_Rect rDest, string sText, int eBitmapFontType, SDL_Surface *surface )
{
	if ( DEBUGFONTS )
		cLog::write ( "Seeking through " + sText, cLog::eLOG_TYPE_DEBUG );

	string sTmp = sText;

	string sTextShortened;

	int k;

	int lastK = 0;

	do
	{
		//search and replace \n since we want a blocktext - no manual breaklines allowed
		k = sText.find ( "\n" );

		if ( k != string::npos )
		{
			if(DEBUGFONTS) cLog::write("Fount breakline at " + iToStr(k), cLog::eLOG_TYPE_DEBUG);
			sText.erase ( k, 1 );
			sText.insert ( k, " " );
			sTmp=sText;
		}
	}
	while ( k != string::npos );

	do
	{
		//erase all blanks > 2
		k = sText.find ( "  " ); //IMPORTANT: _two_ blanks! don't change this or this will become an endless loop

		if ( k != string::npos )
		{
			if(DEBUGFONTS) cLog::write("Fount doubleblank at " + iToStr(k), cLog::eLOG_TYPE_DEBUG);
			sText.erase ( k, 1 );
			sTmp = sText;
		}
	}
	while ( k != string::npos );

	SDL_Rect rLenght = getTextSize ( sText, eBitmapFontType );

	if ( rLenght.w > rDest.w ) //text is longer than dest-width - let's snip it
	{
		do
		{
			k = sTmp.find ( " " ); //search spaces/blanks

			if ( k == string::npos ) //reached the end but leftovers might be to long
			{
				rLenght = getTextSize ( sText, eBitmapFontType ); //test new string lenght
				if ( rLenght.w > rDest.w ) //if leftover is to long snip it too
				{
					sTextShortened = sText; //get total leftover again

					if ( lastK > 0 )
					{
						sTextShortened.erase ( lastK, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, lastK + 1 ); //erase txt from original that we just copied to tmp including leading blank
					}
					else
					{
						sTextShortened.erase ( sText.size() / 2, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, sText.size() / 2 + 1 ); //erase txt from original that we just copied to tmp
						cLog::write ( "Textbox defined to small for text! Can not snip text correctly!", cLog::eLOG_TYPE_ERROR );
					}

					showText ( rDest, sTextShortened, eBitmapFontType, surface ); //blit part of text
					rDest.y += getFontHeight ( eBitmapFontType ); //and add a linebreak
				}

				showText ( rDest, sText, eBitmapFontType, surface ); //draw last part of text
			}

			if ( k != string::npos )
			{
				sTmp.erase ( k, 1 ); //replace spaces with # so we don't find them again next search
				sTmp.insert ( k, "#" );
				sTextShortened = sText; //copy clean text to tmp string
				sTextShortened.erase ( k, sTextShortened.size() ); //erase everything longer than line
				
				rLenght = getTextSize ( sTextShortened, eBitmapFontType ); //test new string lenght
				if ( rLenght.w > rDest.w )
				{
					//found important lastK to snip text since text is now to long
					sTextShortened = sText; //copy text to tmp string

					if ( lastK > 0 )
					{
						sTextShortened.erase ( lastK, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, lastK + 1 ); //erase txt from original that we just copied to tmp including leading blank
					}
					else
					{
						sTextShortened.erase ( sText.size() / 2, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, sText.size() / 2 + 1 ); //erase txt from original that we just copied to tmp
						cLog::write ( "Textbox defined to small for text! Can not snip text correctly!", cLog::eLOG_TYPE_ERROR );

					}

					sTmp = sText; //copy snipped original sText to sTmp to start searching again
					showText ( rDest, sTextShortened, eBitmapFontType, surface ); //blit part of text
					rDest.y += getFontHeight ( eBitmapFontType ); //and add a linebreak
				}
				else
				{
					lastK = k; //seek more, couldn't find korrekt lastK
				}
			}
		}
		while ( k != string::npos );
	}
	else
	{
		showText ( rDest, sText, eBitmapFontType, surface ); //nothing to shorten, just blit text
	}
}

int cBitmapFont::getFontHeight(int eBitmapFontType)
{
	getCharset(eBitmapFontType);
	return sfTmp->h / 16;
}

void cBitmapFont::buildFont(SDL_Surface *surface)
{
	if(surface == NULL)
	{
		cLog::write("Got NULL surface for font", cLog::eLOG_TYPE_ERROR);
		return;
	}
	sfTmp = surface;
	
	int iFormat = sfTmp->format->BitsPerPixel;
	
	Uint32 bgColor32;
	Uint16 bgColor16;
	Uint8 bgColor8;
	switch(iFormat)
	{
		case 32:
		case 24:
			bgColor32 = SDL_MapRGB(sfTmp->format, 0xFF, 0, 0xFF);
			break;
		case 16:
			bgColor16 = SDL_MapRGB(sfTmp->format, 0xFF, 0, 0xFF);
			break;	
		case 8:
		default:
			bgColor8 = SDL_MapRGB(sfTmp->format, 0xFF, 0, 0xFF);
			break;
	}
	

	int cellW = sfTmp->w / 16;
	int cellH = sfTmp->h / 16;
	int currentChar = 0;
	register int pX = 0;
	register int pY = 0;
			
	//go through the rows
	for( int rows = 0; rows < 16; rows ++)
	{
		//go through the cols
		for( int cols = 0; cols < 16; cols ++)
		{
			chars[currentChar].x = cellW * cols; //write each cell position and size into array
			chars[currentChar].y = cellH * rows;
			chars[currentChar].h = cellH;
			chars[currentChar].w = cellW;
			
			//go through pixels to find offset x
			for( int pCol = 0; pCol < cellH; pCol++)
			{
				for( int pRow = 0; pRow < cellH; pRow++)
				{
					pX = (cellW * cols) + pCol;
					pY = (cellH * rows) + pRow;
					bool bFound = false;

					//search for pixel != background colour
					switch(iFormat)
					{
						case 32:
							if(getPixel32(pX, pY, sfTmp) != bgColor32)
							{
								bFound = true;
							}
							break;
						case 16:
							if(getPixel16(pX, pY, sfTmp) != bgColor16)
							{
								bFound = true;
							}
							break;
						case 8:
						default:
							if(getPixel8(pX, pY, sfTmp) != bgColor8)
							{
								bFound = true;
							}
							break;	
					}
					if(bFound)
					{
						//offset
						chars[currentChar].x = pX;
						pCol = cellW; //break loop
						pRow = cellH;	
						bFound = false;		
					}
				}
			}
			
			//go through pixel to find offset w
			for( int pCol_w = cellW - 1; pCol_w >= 0; pCol_w--)
			{
				for ( int pRow_w = 0; pRow_w < cellH; pRow_w++)
				{
					pX = (cellW * cols ) + pCol_w;
					pY = (cellH * rows ) + pRow_w;
					bool bFound = false;
					//search for pixel != background colour
					switch(iFormat)
					{
						case 32:
							if(getPixel32(pX, pY, sfTmp) != bgColor32)
							{
								bFound = true;
							}
							break;
						case 16:
							if(getPixel16(pX, pY, sfTmp) != bgColor16)
							{
								bFound = true;
							}
							break;
						case 8:
						default:
							if(getPixel8(pX, pY, sfTmp) != bgColor8)
							{
								bFound = true;
							}
							break;	
					}
					if(bFound)
					{
						chars[currentChar].w = (pX - chars[currentChar].x) +1;
						pCol_w = -1; //break loop
						pRow_w = cellH;	
						bFound = false;				
					}
				
				}
			}
			//goto next character		
			currentChar++;	
		}
	}
}

void cBitmapFont::showText(SDL_Rect rdest, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	showText(rdest.x, rdest.y, sText, eBitmapFontType, surface);
}

void cBitmapFont::getCharset(int eBitmapFontType)
{
	if(iLoadedCharset != eBitmapFontType) //requested new font - load font surface and array of charlocations
	{
		iLoadedCharset = eBitmapFontType;
		if(DEBUGFONTS) cLog::write("Setting fonttype to " + iToStr(eBitmapFontType), cLog::eLOG_TYPE_DEBUG);
		switch(eBitmapFontType)
		{
			case LATIN_BIG_GOLD:
				sfTmp = sfLatinBigGold;
				copyArray(LatinBigGold, chars); //FIXME: to this with pointers to prevent high cpu load
				break;
			case LATIN_BIG:
				sfTmp = sfLatinBig;
				copyArray(LatinBig, chars);
				break;
			case LATIN_SMALL_GREEN:
				sfTmp = sfLatinSmallGreen;
				copyArray(LatinSmall, chars);
				break;
			case LATIN_SMALL_RED:
				sfTmp = sfLatinSmallRed;
				copyArray(LatinSmall, chars);
				break;
			case LATIN_SMALL_WHITE:
				sfTmp = sfLatinSmallWhite;
				copyArray(LatinSmall, chars);
				break;
			case LATIN_SMALL_YELLOW:
				sfTmp = sfLatinSmallYellow;
				copyArray(LatinSmall, chars);
				break;
			case LATIN_NORMAL :
			default:
				sfTmp = sfLatinNormal;
				copyArray(LatinNormal, chars);
		}
	}
}

SDL_Rect cBitmapFont::getTextSize(string sText, int eBitmapFontType)
{
	//tmp offsets
	SDL_Rect rTmp = {0, 0, 0, sfTmp->h / 16};
	
	getCharset(eBitmapFontType);
	
	int ascii;
	int iSpace = 0;
	
	//make sure only upper characters are read for the small fonts
	// since we don't support lower chars on the small fonts
	switch(eBitmapFontType)
	{
		case LATIN_SMALL_GREEN:
		case LATIN_SMALL_RED:
		case LATIN_SMALL_WHITE:
		case LATIN_SMALL_YELLOW:
			for(int i=0; i < sText.size(); i++)
			{
				sText[i] = toupper(sText[i]);
			}
			iSpace = 1;
			break;
		default:
			break;
	}
	

	for(int i = 0; sText[i] != '\0'; i++)
	{
		//is space?
		if(sText[i] == ' ')
		{
			rTmp.w += sfTmp->w / 32;
		} //is new line?
		else if(sText[i] == '\n')
		{
			rTmp.h += sfTmp->h / 16; //high always the same per charset
		}
		else
		{
			//get ascii value
			ascii = (unsigned char) sText[i];
			rTmp.w += chars[ascii].w + iSpace;
		}
	}
	
	if(DEBUGFONTS)
	{	
		stringstream strStream;
		strStream << "Text: " << sText << " is " << rTmp.w << " width and " << rTmp.h << " height\n";
		cLog::write(strStream.str(), cLog::eLOG_TYPE_DEBUG);
	}
	return rTmp;
}

void cBitmapFont::showTextCentered(SDL_Rect rDest, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	showTextCentered(rDest.x, rDest.y, sText, eBitmapFontType, surface);
}

void cBitmapFont::showTextCentered(int x, int y, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	SDL_Rect rTmp = getTextSize(sText, eBitmapFontType);
	showText( x - rTmp.w / 2, y, sText, eBitmapFontType, surface);	
}

void cBitmapFont::showText(int x, int y, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	//tmp offsets
	int offX = x;
	int offY = y;
	int iSpace = 0;

	getCharset(eBitmapFontType);
	
	//make sure only upper characters are read for the small fonts
	// since we don't support lower chars on the small fonts
	switch(eBitmapFontType)
	{
		case LATIN_SMALL_GREEN:
		case LATIN_SMALL_RED:
		case LATIN_SMALL_WHITE:
		case LATIN_SMALL_YELLOW:
			for(int i=0; i < sText.size(); i++)
			{
				sText[i] = toupper(sText[i]);
			}
			iSpace = 1;
			break;
		default:
			break;
	}	
		
	if(sfTmp != NULL)
	{
		for(int i = 0; sText[i] != '\0'; i++)
		{
			//is space?
			if(sText[i] == ' ')
			{
				offX += sfTmp->w / 32;
			} //is new line?
			else if(sText[i] == '\n')
			{
				offY += sfTmp->h / 16;
				offX = x;
			}
			else
			{
				//get ascii value
				int ascii = (unsigned char) sText[i];
				
				//blit signs to surface
				SDL_Rect rTmp = {offX, offY, 16, 16};
				SDL_BlitSurface(sfTmp, &chars[ascii], surface, &rTmp);
				if(DEBUGFONTS)
				{	
				 	stringstream strStream;
 					strStream << "Sign: " << sText[i] << "(" << ascii << ")" << "  x: " << chars[ascii].x << "  y: " << chars[ascii].y << "  w: " << chars[ascii].w << "  h: " << chars[ascii].h;
					cLog::write(strStream.str(), cLog::eLOG_TYPE_DEBUG);
				}
				//move one px forward for space between signs
				offX += chars[ascii].w + iSpace;
			}
		}
	}

}

Uint32 cBitmapFont::getPixel32(int x, int y, SDL_Surface *surface)
{
	//converts the Pixel to 32 bit
	Uint32 *pixels = (Uint32 *) surface->pixels;
	
	//get the requested pixels
	return pixels[(y*surface->w)+x];
}

Uint16 cBitmapFont::getPixel16(int x, int y, SDL_Surface *surface)
{
	//converts the Pixel to 16 bit
	Uint16 *pixels = (Uint16 *) surface->pixels;
	
	//get the requested pixels
	return pixels[(y*surface->w)+x];
}

Uint8 cBitmapFont::getPixel8(int x, int y, SDL_Surface *surface)
{
	//converts the Pixel to 8 bit
	Uint8 *pixels = (Uint8 *) surface->pixels;
	
	//get the requested pixels
	return pixels[(y*surface->w)+x];
}

int cBitmapFont::getTextWide(string sText, int eBitmapFontType)
{
	SDL_Rect rTmp = getTextSize(sText, eBitmapFontType); //FIXME: doesn't work proper on small fonts!
	return rTmp.w;
}

