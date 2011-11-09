/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011 Armin Burgmeier
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <C4Include.h>
#include <C4Reloc.h>

#include <C4Config.h>
#include <C4Application.h>

C4Reloc Reloc; // singleton

void C4Reloc::Init()
{
	Paths.clear();

	// Check for system group at EXE path - only add if found
	if (FileExists(Config.AtExePath(C4CFN_System)))
		AddPath(Config.General.ExePath.getData());

	// Or a dedicated data folder directly below
	else if (FileExists(Config.AtExePath(C4CFN_DataFolder DirSep C4CFN_System)))
		AddPath(Config.AtExePath(C4CFN_DataFolder));

	AddPath(Config.General.UserDataPath);
	AddPath(Config.General.SystemDataPath);
}

bool C4Reloc::AddPath(const char* path)
{
	if(!IsGlobalPath(path))
		return false;

	if(std::find(Paths.begin(), Paths.end(), path) != Paths.end())
		return false;

	Paths.push_back(StdCopyStrBuf(path));
	return true;
}

C4Reloc::iterator C4Reloc::begin() const
{
	return Paths.begin();
}

C4Reloc::iterator C4Reloc::end() const
{
	return Paths.end();
}

bool C4Reloc::Open(C4Group& hGroup, const char* filename) const
{
	if(IsGlobalPath(filename)) return hGroup.Open(filename);

	for(iterator iter = begin(); iter != end(); ++iter)
		if(hGroup.Open((*iter + DirSep + filename).getData()))
			return true;

	return false;
}

bool C4Reloc::LocateItem(const char* filename, StdStrBuf& str) const
{
	if(IsGlobalPath(filename))
	{
		str.Copy(filename);
		return true;
	}

	for(iterator iter = begin(); iter != end(); ++iter)
	{
		str.Copy(*iter + DirSep + filename);
		if(ItemExists(str.getData()))
			return true;
	}

	return false;
}
