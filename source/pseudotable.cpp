//************************************************************************************************
//
// Localizer Application
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : pseudotable.cpp
// Description : Pseudo Localization Table
//
//************************************************************************************************
/*
	See "Pseudo-Localization" at http://blogs.msdn.com/b/e7/
*/

#include "pseudotable.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/propertyfile.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if (0 && DEBUG)
CCL_KERNEL_INIT_LEVEL (PseudoTable, kFirstRun)
{
	String test = PseudoTable::instance ().getPseudoString (CCLSTR ("Hello World!"));
	return true;
}
#endif

//************************************************************************************************
// PseudoTable
//************************************************************************************************

DEFINE_SINGLETON (PseudoTable)

//////////////////////////////////////////////////////////////////////////////////////////////////

PseudoTable::PseudoTable ()
{
	Java::PropertyFile props;
	bool loaded = props.loadFromFile (ResourceUrl (CCLSTR ("pseudo.properties")));
	ASSERT (loaded == true)

	const StringDictionary& dict = props.getProperties ();
	for(int i = 0, count = dict.countEntries (); i < count; i++)
	{
		uchar inputChar = dict.getKeyAt (i).firstChar ();
		uchar pseudoChar = dict.getValueAt (i).firstChar ();

		ASSERT (inputChar != 0 && pseudoChar != 0)
		if(inputChar == 0 || pseudoChar == 0)
			continue;

		// TODO: check for double entries!

		mappedChars.add (Item (inputChar, pseudoChar));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar PseudoTable::getMappedChar (uchar inputChar) const
{
	VectorForEach (mappedChars, Item, item)
		if(item.inputChar == inputChar)
			return item.pseudoChar;
	EndFor
	return inputChar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PseudoTable::getPseudoString (StringRef inputString) const
{
	String result;
	StringWriter<512> writer (result);

	StringChars chars (inputString);
	bool insideVariable = false;
	uchar lastChar = 0;
	for(int i = 0, count = inputString.length (); i < count; i++)
	{
		uchar inputChar = chars[i];
		uchar nextChar = i+1 < count ? chars[i+1] : 0;

		bool escaped = false;
		if(lastChar == '\\') // check if previous char indicates an escape sequence
		{
			switch(inputChar)
			{
			case 'n' : case 'r' : case 't' : case '\"' :
				escaped = true;
			}
		}

		if(insideVariable)
		{
			if(!Unicode::isAlpha (inputChar))
				insideVariable = false;
		}
		else
		{
			if(inputChar == '$' && nextChar != '$')
				insideVariable = true;
		}

		uchar outputChar = insideVariable || escaped ? inputChar : getMappedChar (inputChar);
		writer.append (outputChar);

		lastChar = inputChar;
	}

	writer.flush ();
	return result;
}
