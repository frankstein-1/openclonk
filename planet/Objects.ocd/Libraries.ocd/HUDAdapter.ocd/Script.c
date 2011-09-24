/*
	HUD Adapter
	Author: Newton
	
	Clonk-side scripts for the HUD. This object basically redirects the
	engine callbacks for the clonk to the HUD. All crew members that
	are to be shown in the HUD have to include this object and return
	_inherited(); if they overload one of the callbacks used here.

	Requires the ClonkControl.ocd to be included in the clonk too.
*/

local HUDselector, HUDcontroller;

public func SetSelector(object sel) { HUDselector = sel; }
public func GetSelector() { return HUDselector; }

public func HUDAdapter()
{
	return true;
}

func Initialize()
{
	AddEffect("HUDBarUpdater", this, 1, 0, this);
	return _inherited(...);
}

func FxHUDBarUpdaterDamage(target, effect, int damage, int cause)
{
	if(effect.last == effect.Time) return damage;
	effect.last = effect.Time;
	
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateHealthBar();
	return damage;
}

// hotkey control
public func ControlHotkey(int hotindex)
{
	if (HUDcontroller)
		return HUDcontroller->ControlHotkey(hotindex);
}

/* Engine callbacks */

// bootstrap the hud
protected func Recruitment(int plr)
{
	HUDcontroller = FindObject(Find_ID(GUI_Controller), Find_Owner(plr));
	if (!HUDcontroller)
		HUDcontroller = CreateObject(GUI_Controller, 10, 10, plr);
	
	HUDcontroller->ScheduleUpdateBackpack();
	HUDcontroller->ScheduleUpdateHealthBar();
	
	return _inherited(plr, ...);
}

public func OnDisplayInfoMessage()
{
	if (HUDcontroller)
		HUDcontroller->ClearButtonMessages();
}
	
// calls to the crew selector hud
protected func OnPromotion() { if (HUDselector) HUDselector->UpdateRank(); return _inherited(...); }
protected func OnEnergyChange() { if (HUDselector) HUDselector->UpdateHealthBar(); return _inherited(...); }
protected func OnBreathChange() { HUDcontroller->UpdateHUDBreathBar();  if (HUDselector) HUDselector->UpdateBreathBar(); return _inherited(...); }
protected func OnMagicEnergyChange() { if (HUDselector) HUDselector->UpdateMagicBar(); return _inherited(...); }
protected func OnNameChanged() { if (HUDselector) HUDselector->UpdateName(); return _inherited(...); }

protected func OnPhysicalChange(string physical, int change)
{
	if (HUDselector)
	{		
		// all physicals are resetted
		if (!physical)
		{
			HUDselector->UpdateHealthBar();
			HUDselector->UpdateBreathBar();
			HUDselector->UpdateMagicBar();
		}
		else if (physical == "Energy") HUDselector->UpdateHealthBar();
		else if (physical == "Breath") HUDselector->UpdateBreathBar();
		else if (physical == "Magic") HUDselector->UpdateMagicBar();
	}
	return _inherited(physical, change, ...);
}

// calls to both crew selector and controller
protected func CrewSelection(bool unselect)
{
	if (HUDselector)
		HUDselector->UpdateSelectionStatus();
	if (HUDcontroller)
		HUDcontroller->OnCrewSelection(this,unselect);
	return _inherited(unselect, ...);
}

// calls to controller
protected func OnCrewEnabled()
{
	if (HUDcontroller)
		HUDcontroller->OnCrewEnabled(this);
	return _inherited(...);
}

protected func OnCrewDisabled()
{
	if (HUDcontroller)
		HUDcontroller->OnCrewDisabled(this);
	return _inherited(...);
}

// from ClonkControl.ocd
protected func OnSlotFull(int slot)
{
	if (HUDcontroller)
		HUDcontroller->OnSlotObjectChanged(slot);
	return _inherited(slot, ...);
}

protected func OnSlotEmpty(int slot)
{
	if (HUDcontroller)
		HUDcontroller->OnSlotObjectChanged(slot);
	return _inherited(slot, ...);
}

func Collection2()
{
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateBackpack();
	return _inherited(...);
}

func Ejection()
{
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateBackpack();
	return _inherited(...);
}

func ControlContents()
{
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateBackpack();
	return _inherited(...);
}
