/*
 * Copyright (c) 2009  Günther Brammer <gbrammer@gmx.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 */

/* Property lists */

#ifndef C4PROPLIST_H
#define C4PROPLIST_H

#include "C4Value.h"
#include "C4StringTable.h"

class C4Def;

class C4Property {
	public:
	C4Property() : Key(0) {}
	C4Property(C4String *Key, const C4Value &Value) : Key(Key), Value(Value)
		{ assert(Key); Key->IncRef(); assert(Strings.Set.Has(Key)); }
	C4Property(const C4Property &o) : Key(o.Key), Value(o.Value) { if(Key) Key->IncRef(); }
	C4Property & operator = (const C4Property &o)
		{ assert(o.Key); o.Key->IncRef(); if(Key) Key->DecRef(); Key = o.Key; Value = o.Value; return *this; }
	~C4Property() { if(Key) Key->DecRef();}
	C4String * Key;
	C4Value Value;
	operator void * () { return Key; }
	C4Property & operator = (void * p) { assert(!p); if(Key) Key->DecRef(); Key = 0; Value.Set0(); return *this; }
};

class C4PropList {
	public:
	int32_t Number;
	int32_t Status; // NoSave //
	void AddRef(C4Value *pRef);
	void DelRef(const C4Value *pRef, C4Value * pNextRef);
	void AssignRemoval();
	const char *GetName();
	virtual void SetName (const char *NewName = 0);

	virtual C4Def * GetDef();
	virtual C4Object * GetObject();
	C4PropList * GetPrototype() { return prototype; }

	bool GetProperty(C4String * k, C4Value & to);
	C4String * GetPropertyStr(C4PropertyName k);
	int32_t GetPropertyInt(C4PropertyName k);
	void SetProperty(C4String * k, const C4Value & to);
	void ResetProperty(C4String * k);

	C4PropList(C4PropList * prototype = 0);
	virtual ~C4PropList();

	protected:
	C4Value *FirstRef; // No-Save

	C4Set<C4Property> Properties;
	private:
	C4PropList * prototype;
};

#endif // C4PROPLIST_H
