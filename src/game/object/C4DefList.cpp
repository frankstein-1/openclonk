/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2003-2007  Sven Eberhardt
 * Copyright (c) 2004-2005, 2007  Peter Wortmann
 * Copyright (c) 2009, 2011  Günther Brammer
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Nicolas Hake
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

/* Object definition */

#include <C4Include.h>
#include <C4DefList.h>

#include <C4Config.h>
#include <C4Def.h>
#include <C4FileMonitor.h>
#include <C4GameVersion.h>
#include <C4Language.h>

C4DefList::C4DefList()
{
	Default();
}

C4DefList::~C4DefList()
{
	Clear();
}

int32_t C4DefList::Load(C4Group &hGroup, DWORD dwLoadWhat,
                        const char *szLanguage,
                        C4SoundSystem *pSoundSystem,
                        bool fOverload,
                        bool fSearchMessage, int32_t iMinProgress, int32_t iMaxProgress, bool fLoadSysGroups)
{
	int32_t iResult=0;
	C4Def *nDef;
	char szEntryname[_MAX_FNAME+1];
	C4Group hChild;
	bool fPrimaryDef=false;
	bool fThisSearchMessage=false;

	// This search message
	if (fSearchMessage)
		if (SEqualNoCase(GetExtension(hGroup.GetName()),"c4d")
		    || SEqualNoCase(GetExtension(hGroup.GetName()),"c4s")
		    || SEqualNoCase(GetExtension(hGroup.GetName()),"c4f"))
		{
			fThisSearchMessage=true;
			fSearchMessage=false;
		}

	if (fThisSearchMessage) { LogF("%s...",GetFilename(hGroup.GetName())); }

	// Load primary definition
	if ((nDef=new C4Def))
	{
		if ( nDef->Load(hGroup,dwLoadWhat,szLanguage,pSoundSystem) && Add(nDef,fOverload) )
			{ iResult++; fPrimaryDef=true; }
		else
			{ delete nDef; }
	}

	// Load sub definitions
	int i = 0;
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_DefFiles,szEntryname))
		if (hChild.OpenAsChild(&hGroup,szEntryname))
		{
			// Hack: Assume that there are sixteen sub definitions to avoid unnecessary I/O
			int iSubMinProgress = Min<int32_t>(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * i) / 16);
			int iSubMaxProgress = Min<int32_t>(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * (i + 1)) / 16);
			++i;
			iResult += Load(hChild,dwLoadWhat,szLanguage,pSoundSystem,fOverload,fSearchMessage,iSubMinProgress,iSubMaxProgress);
			hChild.Close();
		}

	// load additional system scripts for def groups only
	if (!fPrimaryDef && fLoadSysGroups) Game.LoadAdditionalSystemGroup(hGroup);

	if (fThisSearchMessage) { LogF(LoadResStr("IDS_PRC_DEFSLOADED"),iResult); }

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return iResult;
}

int32_t C4DefList::LoadFolderLocal( const char *szPath,
                                    DWORD dwLoadWhat, const char *szLanguage,
                                    C4SoundSystem *pSoundSystem,
                                    bool fOverload, char *sStoreName, int32_t iMinProgress, int32_t iMaxProgress)
{
	int32_t iResult = 0;

	// Scan path for folder names
	int32_t cnt,iBackslash,iDefs;
	char szFoldername[_MAX_PATH+1];
	for (cnt=0; (iBackslash=SCharPos('\\',szPath,cnt)) > -1; cnt++)
	{
		SCopy(szPath,szFoldername,iBackslash);
		// Load from parent folder
		if (SEqualNoCase(GetExtension(szFoldername),"c4f"))
			if ((iDefs=Load(szFoldername,dwLoadWhat,szLanguage,pSoundSystem,fOverload)))
			{
				iResult+=iDefs;
				// Add any folder containing defs to store list
				if (sStoreName) { SNewSegment(sStoreName); SAppend(szFoldername,sStoreName); }
			}
	}

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return iResult;
}

int32_t C4DefList::Load(const char *szSearch,
                        DWORD dwLoadWhat, const char *szLanguage,
                        C4SoundSystem *pSoundSystem,
                        bool fOverload, int32_t iMinProgress, int32_t iMaxProgress)
{
	int32_t iResult=0;

	// Empty
	if (!szSearch[0]) return iResult;

	// Segments
	char szSegment[_MAX_PATH+1]; int32_t iGroupCount;
	if ((iGroupCount=SCharCount(';',szSearch)))
	{
		++iGroupCount; int32_t iPrg=iMaxProgress-iMinProgress;
		for (int32_t cseg=0; SCopySegment(szSearch,cseg,szSegment,';',_MAX_PATH); cseg++)
			iResult += Load(szSegment,dwLoadWhat,szLanguage,pSoundSystem,fOverload,
			                iMinProgress+iPrg*cseg/iGroupCount, iMinProgress+iPrg*(cseg+1)/iGroupCount);
		return iResult;
	}

	// Wildcard items
	if (SCharCount('*',szSearch))
	{
#ifdef _WIN32
		struct _finddata_t fdt; int32_t fdthnd;
		if ((fdthnd=_findfirst(szSearch,&fdt))<0) return false;
		do
		{
			iResult += Load(fdt.name,dwLoadWhat,szLanguage,pSoundSystem,fOverload);
		}
		while (_findnext(fdthnd,&fdt)==0);
		_findclose(fdthnd);
		// progress
		if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));
#else
		fputs("FIXME: C4DefList::Load\n", stderr);
#endif
		return iResult;
	}

	// File specified with creation (currently not used)
	char szCreation[25+1];
	int32_t iCreation=0;
	if (SCopyEnclosed(szSearch,'(',')',szCreation,25))
	{
		// Scan creation
		SClearFrontBack(szCreation);
		sscanf(szCreation,"%i",&iCreation);
		// Extract filename
		SCopyUntil(szSearch,szSegment,'(',_MAX_PATH);
		SClearFrontBack(szSegment);
		szSearch = szSegment;
	}

	// Load from specified file
	C4Group hGroup;
	if (!Reloc.Open(hGroup, szSearch))
	{
		// Specified file not found (failure)
		LogFatal(FormatString(LoadResStr("IDS_PRC_DEFNOTFOUND"),szSearch).getData());
		LoadFailure=true;
		return iResult;
	}
	iResult += Load(hGroup,dwLoadWhat,szLanguage,pSoundSystem,fOverload,true,iMinProgress,iMaxProgress);
	hGroup.Close();

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return iResult;
}

bool C4DefList::Add(C4Def *pDef, bool fOverload)
{
	if (!pDef) return false;

	// Check old def to overload
	C4Def *pLastDef = ID2Def(pDef->id);
	if (pLastDef && !fOverload) return false;

	// Log overloaded def
	if (Config.Graphics.VerboseObjectLoading>=1)
		if (pLastDef)
		{
			LogF(LoadResStr("IDS_PRC_DEFOVERLOAD"),pDef->GetName(),pLastDef->id.ToString());
			if (Config.Graphics.VerboseObjectLoading >= 2)
			{
				LogF("      Old def at %s",pLastDef->Filename);
				LogF("     Overload by %s",pDef->Filename);
			}
		}

	// Remove old def
	Remove(pDef->id);

	// Add new def
	pDef->Next=FirstDef;
	FirstDef=pDef;

	return true;
}

bool C4DefList::Remove(C4ID id)
{
	C4Def *cdef,*prev;
	for (cdef=FirstDef,prev=NULL; cdef; prev=cdef,cdef=cdef->Next)
		if (cdef->id==id)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			return true;
		}
	return false;
}

void C4DefList::Remove(C4Def *def)
{
	C4Def *cdef,*prev;
	for (cdef=FirstDef,prev=NULL; cdef; prev=cdef,cdef=cdef->Next)
		if (cdef==def)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			return;
		}
}

void C4DefList::Clear()
{
	C4Def *cdef,*next;
	for (cdef=FirstDef; cdef; cdef=next)
	{
		next=cdef->Next;
		delete cdef;
	}
	FirstDef=NULL;
	// clear quick access table
	table.clear();
}

C4Def* C4DefList::ID2Def(C4ID id)
{
	if (id==C4ID::None) return NULL;
	if (table.empty())
	{
		// table not yet built: search list
		C4Def *cdef;
		for (cdef=FirstDef; cdef; cdef=cdef->Next)
			if (cdef->id==id) return cdef;
	}
	else
	{
		Table::const_iterator it = table.find(id);
		if (it != table.end())
			return it->second;
	}
	// none found
	return NULL;
}

int32_t C4DefList::GetIndex(C4ID id)
{
	C4Def *cdef;
	int32_t cindex;
	for (cdef=FirstDef,cindex=0; cdef; cdef=cdef->Next,cindex++)
		if (cdef->id==id) return cindex;
	return -1;
}

int32_t C4DefList::GetDefCount()
{
	C4Def *cdef; int32_t ccount=0;
	for (cdef=FirstDef; cdef; cdef=cdef->Next)
		ccount++;
	return ccount;
}

C4Def* C4DefList::GetDef(int32_t iIndex)
{
	C4Def *pDef; int32_t iCurrentIndex;
	if (iIndex<0) return NULL;
	for (pDef=FirstDef,iCurrentIndex=-1; pDef; pDef=pDef->Next)
	{
		iCurrentIndex++;
		if (iCurrentIndex==iIndex) return pDef;
	}
	return NULL;
}

C4Def *C4DefList::GetByPath(const char *szPath)
{
	// search defs
	const char *szDefPath;
	for (C4Def *pDef = FirstDef; pDef; pDef = pDef->Next)
		if ((szDefPath = Config.AtRelativePath(pDef->Filename)))
			if (SEqual2NoCase(szPath, szDefPath))
			{
				// the definition itself?
				if (!szPath[SLen(szDefPath)])
					return pDef;
				// or a component?
				else if (szPath[SLen(szDefPath)] == '\\')
					if (!strchr(szPath + SLen(szDefPath) + 1, '\\'))
						return pDef;
			}
	// not found
	return NULL;
}

int32_t C4DefList::RemoveTemporary()
{
	C4Def *cdef,*prev,*next;
	int32_t removed=0;
	for (cdef=FirstDef,prev=NULL; cdef; cdef=next)
	{
		next=cdef->Next;
		if (cdef->Temporary)
		{
			if (prev) prev->Next=next;
			else FirstDef=next;
			delete cdef;
			removed++;
		}
		else
			prev=cdef;
	}
	// rebuild quick access table
	BuildTable();
	return removed;
}

int32_t C4DefList::CheckEngineVersion(int32_t ver1, int32_t ver2, int32_t ver3, int32_t ver4)
{
	int32_t rcount=0;
	C4Def *cdef,*prev,*next;
	for (cdef=FirstDef,prev=NULL; cdef; cdef=next)
	{
		next=cdef->Next;
		if (CompareVersion(cdef->rC4XVer[0],cdef->rC4XVer[1],cdef->rC4XVer[2],cdef->rC4XVer[3],ver1,ver2,ver3,ver4) > 0)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			rcount++;
		}
		else prev=cdef;
	}
	return rcount;
}

int32_t C4DefList::CheckRequireDef()
{
	int32_t rcount=0, rcount2;
	C4Def *cdef,*prev,*next;
	do
	{
		rcount2 = rcount;
		for (cdef=FirstDef,prev=NULL; cdef; cdef=next)
		{
			next=cdef->Next;
			for (int32_t i = 0; i < cdef->RequireDef.GetNumberOfIDs(); i++)
				if (GetIndex(cdef->RequireDef.GetID(i)) < 0)
				{
					(prev ? prev->Next : FirstDef) = cdef->Next;
					delete cdef;
					rcount++;
				}
		}
	}
	while (rcount != rcount2);
	return rcount;
}

void C4DefList::Draw(C4ID id, C4Facet &cgo, bool fSelected, int32_t iColor)
{
	C4Def *cdef = ID2Def(id);
	if (cdef) cdef->Draw(cgo,fSelected,iColor);
}

void C4DefList::Default()
{
	FirstDef=NULL;
	LoadFailure=false;
	table.clear();
}

// Load scenario specified or all selected plus scenario & folder local

int32_t C4DefList::LoadForScenario(const char *szScenario,
                                   const char *szSelection,
                                   DWORD dwLoadWhat, const char *szLanguage,
                                   C4SoundSystem *pSoundSystem, bool fOverload,
                                   int32_t iMinProgress, int32_t iMaxProgress)
{
	int32_t iDefs=0;
	StdStrBuf sSpecified;

	// User selected modules
	sSpecified.Copy(szSelection);

	// Open scenario file & load core
	C4Group hScenario;
	C4Scenario C4S;
	if ( !hScenario.Open(szScenario)
	     || !C4S.Load(hScenario) )
		return 0;

	// Scenario definition specifications (override user selection)
	if (!C4S.Definitions.AllowUserChange)
		C4S.Definitions.GetModules(&sSpecified);

	// Load specified
	iDefs += Load(sSpecified.getData(),dwLoadWhat,szLanguage,pSoundSystem,fOverload);
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress+iMinProgress)/2);

	// Load folder local
	iDefs += LoadFolderLocal(szScenario,dwLoadWhat,szLanguage,pSoundSystem,fOverload);
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress*3+iMinProgress)/4);

	// Load local
	iDefs += Load(hScenario,dwLoadWhat,szLanguage,pSoundSystem,fOverload);

	// build quick access table
	BuildTable();

	// progress
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	// Done
	return iDefs;
}

bool C4DefList::Reload(C4Def *pDef, DWORD dwLoadWhat, const char *szLanguage, C4SoundSystem *pSoundSystem)
{
	// Safety
	if (!pDef) return false;
	// backup graphics names and pointers
	// GfxBackup-dtor will ensure that upon loading-failure all graphics are reset to default
	C4DefGraphicsPtrBackup GfxBackup(&pDef->Graphics);
	// Clear def
	pDef->Clear(); // Assume filename is being kept
	// Reload def
	C4Group hGroup;
	if (!hGroup.Open(pDef->Filename)) return false;
	if (!pDef->Load( hGroup, dwLoadWhat, szLanguage, pSoundSystem )) return false;
	hGroup.Close();
	// rebuild quick access table
	BuildTable();
	// update script engine - this will also do include callbacks and Freeze() this
	::ScriptEngine.ReLink(this);
	// restore graphics
	GfxBackup.AssignUpdate(&pDef->Graphics);
	// Success
	return true;
}

bool C4DefList::GetFontImage(const char *szImageTag, CFacet &rOutImgFacet)
{
	// extended: images by game
	C4FacetSurface fctOut;
	if (!Game.DrawTextSpecImage(fctOut, szImageTag)) return false;
	if (fctOut.Surface == &fctOut.GetFace()) return false; // cannot use facets that are drawn on the fly right now...
	rOutImgFacet.Set(fctOut.Surface, fctOut.X, fctOut.Y, fctOut.Wdt, fctOut.Hgt);
	// done, found
	return true;
}

void C4DefList::Synchronize()
{
	for (Table::iterator it = table.begin(); it != table.end(); ++it)
		it->second->Synchronize();
}

void C4DefList::ResetIncludeDependencies()
{
	for (Table::iterator it = table.begin(); it != table.end(); ++it)
		it->second->ResetIncludeDependencies();
}

void C4DefList::CallEveryDefinition()
{
	for (Table::iterator it = table.begin(); it != table.end(); ++it)
	{
		C4AulParSet Pars(C4VPropList(it->second));
		it->second->Script.Call(PSF_Definition, 0, &Pars, true);
		it->second->Freeze();
	}
}

void C4DefList::BuildTable()
{
	table.clear();
	for (C4Def *def = FirstDef; def; def = def->Next)
		table.insert(std::make_pair(def->id, def));
}
