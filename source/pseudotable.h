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
// Filename    : pseudotable.h
// Description : Pseudo Localization Table
//
//************************************************************************************************

#ifndef _localizer_pseudotable_h
#define _localizer_pseudotable_h

#include "ccl/base/singleton.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"

namespace Localizer {

//************************************************************************************************
// PseudoTable
//************************************************************************************************

class PseudoTable: public CCL::Object,
				   public CCL::Singleton<PseudoTable>
{
public:
	PseudoTable ();

	CCL::String getPseudoString (CCL::StringRef inputString) const;

protected:
	struct Item
	{
		CCL::uchar inputChar;
		CCL::uchar pseudoChar;

		Item (CCL::uchar inputChar = 0,
			  CCL::uchar pseudoChar = 0)
		: inputChar (inputChar),
		  pseudoChar (pseudoChar)
		{}
	};

	CCL::Vector<Item> mappedChars;

	CCL::uchar getMappedChar (CCL::uchar inputChar) const;
};

} // namespace Localizer

#endif // _localizer_pseudotable_h
