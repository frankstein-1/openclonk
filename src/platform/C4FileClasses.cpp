/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2009  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* Some class entries in the Windows registry */

#include <C4Include.h>
#include <C4FileClasses.h>

#include <StdRegistry.h>

#define C4FileClassContentType "application/vnd.clonk.c4group"

bool SetProtocol(const char *szProtocol, const char *szCommand, const char *szModule)
{

	if (!SetRegClassesRoot(szProtocol,NULL,"URL: Protocol")) return false;
	if (!SetRegClassesRoot(szProtocol,"URL Protocol","")) return false;

	char szCmd[_MAX_PATH+1],szKey[_MAX_PATH+1];
	sprintf(szCmd,szCommand,szModule);
	sprintf(szKey,"%s\\shell\\open\\command",szProtocol);
	if (!SetRegClassesRoot(szKey,"",szCmd)) return false;

	char szIconpath[_MAX_PATH+1];
	sprintf(szIconpath,"%s,1",szModule);
	sprintf(szKey,"%s\\DefaultIcon",szProtocol);
	if (!SetRegClassesRoot(szKey,"",szIconpath)) return false;

	return true;
}

bool SetC4FileClasses(const char *szEnginePath)
{

	if (!SetRegFileClass("OpenClonk.Scenario",   ,"ocs", "OpenClonk Scenario",          szEnginePath, 1, C4FileClassContentType)) return false;
	if (!SetRegFileClass("OpenClonk.Group",      ,"ocg", "OpenClonk Group",             szEnginePath, 2, C4FileClassContentType)) return false;
	if (!SetRegFileClass("OpenClonk.Folder",     ,"ocf", "OpenClonk Folder",            szEnginePath, 3, C4FileClassContentType)) return false;
	if (!SetRegFileClass("OpenClonk.Player",     ,"ocp", "OpenClonk Player",            szEnginePath, 4, C4FileClassContentType)) return false;
	if (!SetRegFileClass("OpenClonk.Definition", ,"ocd", "OpenClonk Object Definition", szEnginePath, 5, C4FileClassContentType)) return false;
	if (!SetRegFileClass("OpenClonk.Object",     ,"oci", "OpenClonk Object Info",       szEnginePath, 6, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Material",   "c4m", "Clonk 4 Material",          szEnginePath, 7, "text/plain")) return false;
	if (!SetRegFileClass("Clonk4.Binary",     "c4b", "Clonk 4 Binary",            szEnginePath, 8, "application/octet-stream")) return false;
	if (!SetRegFileClass("Clonk4.Video",      "c4v", "Clonk 4 Video",             szEnginePath, 9, "video/avi")) return false;
	if (!SetRegFileClass("Clonk4.Weblink",    "c4l", "Clonk 4 Weblink",           szEnginePath, 10, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Update",     "c4u", "Clonk 4 Update",            szEnginePath, 11, C4FileClassContentType)) return false;

	if (!SetProtocol("clonk", "%s %%1 /Fullscreen", szEnginePath)) return false;

	char strCommand[2048];
	// c4u application: send to engine
	sprintf(strCommand, "\"%s\" \"%%1\"", szEnginePath);
	if (!SetRegShell("Clonk4.Update", "Update", "Update", strCommand, true)) return false;

	// kill old App Paths registration
	DeleteRegistryKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Clonk.exe");

	return true;
}
