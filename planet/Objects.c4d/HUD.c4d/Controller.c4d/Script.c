#strict 2

/*
	Per-Player Controller (HUD)
	Author: Newton

	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller. Responsible for taking
	care of the action (inventory) bar.
*/

// TODO - 
// following callbacks missing:
// OnClonkUnRecruitment - clonk gets de-recruited from a crew
// ...? - entire player is eliminated

local actionbar;

protected func Construction()
{
	actionbar = CreateArray();

	// find all clonks of this crew which do not have a selector yet (and can have one)
	for(var i=GetCrewCount(GetOwner())-1; i >= 0; --i)
	{
		var crew = GetCrew(GetOwner(),i);
		if(!(crew->HUDAdapter())) continue;
		
		var sel = crew->GetSelector();
		if(!sel)
			CreateSelectorFor(crew);
	}
	
	// reorder the crew selectors
	ReorderCrewSelectors();
}

protected func OnClonkRecruitment(object clonk, int plr)
{
	// not my business
	if(plr != GetOwner()) return;
	
	if(!(clonk->HUDAdapter())) return;
	
	// not enabled
	if(!clonk->GetCrewEnabled()) return;
	
	// if the clonk already has a hud, it means that he belonged to another
	// crew. So we need another handling here in this case.
	var sel;
	if(sel = clonk->GetSelector())
	{
		var owner = sel->GetOwner();
		sel->UpdateController();
		
		// reorder stuff in the other one
		var othercontroller = FindObject(Find_ID(GetID()), Find_Owner(owner));
		othercontroller->ReorderCrewSelectors();
	}
	// create new crew selector
	else
	{
		CreateSelectorFor(clonk);
	}
	
	// reorder the crew selectors
	ReorderCrewSelectors();
}

protected func OnClonkDeath(object clonk, int killer)
{
	if(clonk->GetController() != GetOwner()) return;
	
	if(!(clonk->HUDAdapter())) return;
	
	OnCrewDisabled(clonk);
}

public func OnCrewDisabled(object clonk)
{
	// notify the hud and reorder
	clonk->GetSelector()->CrewGone();
	ReorderCrewSelectors();
}

public func OnCrewEnabled(object clonk)
{
	CreateSelectorFor(clonk);
	ReorderCrewSelectors();
}

// call from HUDAdapter (Clonk)
public func OnCrewSelection(object clonk, bool deselect)
{

	// selected
	if(!deselect)
	{
		// TODO: what if two clonks are selected? Which clonk gets the actionbar?
		
		// fill actionbar
		var max = 10;
		
		// inventory
		var i;
		for(i = 0; i < clonk->MaxContentsCount(); ++i)
		{
			ActionButton(clonk,i,clonk->GetItem(i),ACTIONTYPE_INVENTORY);
		}		
		ClearButtons(i);
		
		// and start effect to monitor vehicles and structures...
		AddEffect("IntSearchInteractionObjects",clonk,1,10,this,nil,i,max);
	}
	else
	{
		// remove effect
		RemoveEffect("IntSearchInteractionObjects",clonk,0);
		ClearButtons();
	}
}


public func FxIntSearchInteractionObjectsStart(object target, int num, int temp, startAt, max)
{
	if(temp != 0) return;
	EffectVar(0,target,num) = startAt;
	EffectVar(1,target,num) = max;
	EffectCall(target,num,"Timer",target,num,0);
}

public func FxIntSearchInteractionObjectsTimer(object target, int num, int time)
{

	// find vehicles & structures
	var startAt = EffectVar(0,target,num);
	var i = startAt;
	var max = EffectVar(1,target,num);
	
	// target->FindObjects(Find_AtPoint(0,0),Find_OCF(OCF_Grab),Find_NoContainer());
	// doesnt work!! -> BUG! (TODO)
	
	var vehicles = CreateArray();
	var pushed = nil;
	
	// if contained, don't search for vehicles...
	// TODO: change to: list vehicles inside buildings to push out?
	if((!target->Contained()))
	{
		vehicles = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_OCF(OCF_Grab),Find_NoContainer());
		
		// don't forget the vehicle that the clonk is pushing (might not be found
		// by the findobjects because it is not at that point)
		if(target->GetProcedure() == "PUSH" && (pushed = target->GetActionTarget()))
		{
			ActionButton(target,i++,pushed,ACTIONTYPE_VEHICLE);
			if(max) if(i >= max) return ClearButtons(i);
		}
	}
	
	for(var vehicle in vehicles)
	{
		// ...and exclude the pushed one here...
		if(vehicle == pushed) continue;
		ActionButton(target,i++,vehicle,ACTIONTYPE_VEHICLE);
		if(max) if(i >= max) return ClearButtons(i);
	}

	var structures = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_OCF(OCF_Entrance),Find_NoContainer());
	for(var structure in structures)
	{
		ActionButton(target,i++,structure,ACTIONTYPE_STRUCTURE);
		if(max) if(i >= max) return ClearButtons(i);
	}

	//Message("found %d vehicles and %d structures",target,GetLength(vehicles),GetLength(structures));
	
	ClearButtons(i);
	
	return;
}

// call from HUDAdapter (Clonk)
public func OnSelectionChanged(int old, int new)
{
	//Log("selection changed from %d to %d", old, new);
	// update both old and new
	actionbar[old]->UpdateSelectionStatus();
	actionbar[new]->UpdateSelectionStatus();
}

// call from HUDAdapter (Clonk)
public func OnSlotObjectChanged(int slot)
{
	//Log("slot %d changed", slot);
	var obj = GetCursor(GetOwner())->GetItem(slot);
	actionbar[slot]->SetObject(obj, ACTIONTYPE_INVENTORY, slot);
}

private func ActionButton(object forClonk, int pos, object interaction, int actiontype)
{
	var size = ACBT->GetDefWidth();
	var spacing = 12 + size;

	// don't forget the spacings between inventory - vehicle,structure
	var extra = 0;
	if(forClonk->MaxContentsCount() <= pos) extra = 80;
	
	var bt = actionbar[pos];
	// no object yet... create it
	if(!bt)
	{
		bt = CreateObject(ACBT,0,0,GetOwner());
	}
	
	bt->SetPosition(64 + pos * spacing + extra, -16 - size/2);
	
	bt->SetCrew(forClonk);
	bt->SetObject(interaction,actiontype,pos);
	
	actionbar[pos] = bt;
	return bt;
}

private func ClearButtons(int start)
{

	// make rest invisible
	for(var j = start; j < GetLength(actionbar); ++j)
	{
		// we don't have to remove them all the time, no?
		if(actionbar[j])
			actionbar[j]->Clear();
	}
}

// hotkey control
public func ControlHotkey(int hotindex)
{
	if(GetLength(actionbar) <= hotindex) return false;
	
	actionbar[hotindex]->~MouseSelection(GetOwner());
	
	return true;
}

private func CreateSelectorFor(object clonk)
{
		var selector = CreateObject(CSLR,10,10,-1);
		selector->SetCrew(clonk);
		clonk->SetSelector(selector);
		return selector;
}

public func ReorderCrewSelectors()
{
	// somehow new crew gets sorted at the beginning
	// because we dont want that, the for loop starts from the end
	var j = 0;
	for(var i=GetCrewCount(GetOwner())-1; i >= 0; --i)
	{
		var spacing = 12;
		var crew = GetCrew(GetOwner(),i);
		var sel = crew->GetSelector();
		if(sel)
		{
			sel->SetPosition(32 + j * (CSLR->GetDefWidth() + spacing) + CSLR->GetDefWidth()/2, 16+CSLR->GetDefHeight()/2);
			if(j+1 == 10) sel->SetHotkey(0);
			else if(j+1 < 10) sel->SetHotkey(j+1);
		}
		
		j++;
	}
}
