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
////////////////////////////////////////////////////////////////////////////////
//
//  File:   language.h
//  Date:   07-10-01
//  Author: JCK
//
////////////////////////////////////////////////////////////////////////////////
//  Description:
//  This class handles the support for different language packs in XML-Format.
////////////////////////////////////////////////////////////////////////////////

#ifndef LANGUAGE_H
#define LANGUAGE_H

////////////////////////////////////////////////////////////////////////////////

#define LANGUAGE_FILE_FOLDER cSettings::getInstance().getLangPath()
#define LANGUAGE_FILE_NAME   "lang_"
#define LANGUAGE_FILE_EXT    ".xml"

////////////////////////////////////////////////////////////////////////////////
// XML-Node paths

// With NULL as ending sign
#define XNP_MAX_LANG_FILE "MAX_Language_File", NULL
#define XNP_MAX_LANG_FILE_HEADER_AUTHOR "MAX_Language_File", "Header", "Author", NULL
#define XNP_MAX_LANG_FILE_HEADER_AUTHOR_EDITOR "MAX_Language_File", "Header", "Author", "Editor", NULL
#define XNP_MAX_LANG_FILE_HEADER_GAMEVERSION "MAX_Language_File", "Header", "Game_Version", NULL
#define XNP_MAX_LANG_FILE_TEXT "MAX_Language_File", "Text", NULL
#define XNP_MAX_LANG_FILE_GRAPHIC "MAX_Language_File", "Graphic", NULL
#define XNP_MAX_LANG_FILE_SPEECH "MAX_Language_File", "Speech", NULL

// Without NULL as ending sign. Do not forget it in parameter list !
#define XNP_MAX_LANG_FILE_TEXT_MAIN "MAX_Language_File", "Text", "Main"
#define XNP_MAX_LANG_FILE_TEXT_ERROR_MSG "MAX_Language_File", "Text", "Error_Messages"

////////////////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
#include "defines.h"
#include "tinyxml2.h"

class cLanguage
{
public:
	cLanguage();

	const std::string& GetCurrentLanguage() const;
	int         SetCurrentLanguage (const std::string& szLanguageCode);
	std::string i18n (const std::string& szInputText);
	// Translation with replace %s
	std::string i18n (const std::string& szMainText, const std::string& szInsertText);
	int         ReadLanguagePack();
	int         CheckCurrentLanguagePack (bool bInsertMissingEntries);

private:
	typedef std::map<std::string, std::string> StrStrMap;

	int         ReadSingleTranslation (char const* pszCurrent, ...);
	std::string ReadSingleTranslation (const std::string& strInput);
	int         ReadLanguagePackHeader();
	int         ReadLanguagePackHeader (const std::string& strLanguageCode);
	int         ReadLanguageMaster();
	int         ReadRecursiveLanguagePack (tinyxml2::XMLElement* xmlElement, std::string strNodePath);

	int         checkTimeStamp (std::string rstrData);

	tinyxml2::XMLDocument m_XmlDoc;
	// Use ISO 639-2 codes to identify languages
	// (http://www.loc.gov/standards/iso639-2/php/code_list.php)
	std::string m_szLanguage;
	std::string m_szLanguageFile;
	std::string m_szLanguageFileMaster;
	std::string m_szEncoding;
	std::string m_szLastEditor;
	StrStrMap   m_mpLanguage;
	bool        m_bLeftToRight;
	bool        m_bErrorMsgTranslationLoaded;
};

#endif
