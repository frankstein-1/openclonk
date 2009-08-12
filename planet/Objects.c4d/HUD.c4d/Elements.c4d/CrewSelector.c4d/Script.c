#strict 2
#include BAR0

/*
	Crew selector HUD
	
	For each crew member, one of these HUD elements exist in the top bar.
	It shows the rank, health, breath and magic bars as well as the title
	(or portrait) and is clickable. If clicked, the associated crew member
	get's selected.
	HUD elements are passive, they don't update their status by themselves
	but rely on the HUD controller to be notified of any changes.

*/

local crew, breathbar, magicbar, hotkey;

public func BarSpacing() { return -5; }
public func HealthBarHeight() { return 12; }
public func BreathBarHeight() { return 7; }
public func MagicBarHeight() { return 7; }

/*
	usage of layers:
	-----------------
	layer 0 - unused
	layer 1 - title
	layer 2,3 - health bar
	layer 4,5 - breath bar
	layer 6,7 - magic bar
	
	layer 10,11 - rank
	layer 12 - hotkey
*/

/*
	The crew selector needs to be notified when
	-------------------
	...his clonk...
	+ changes his energy	->	UpdateHealthBar()
	+ changes his breath	->	UpdateBreathBar()
	+ changes his magic energy	->	UpdateMagicBar()
	+ (temporarily) changes his physical (energy, breath, magic energy)	-> see above
	+ gains a rank	->	UpdateRank()
	+ is selected/is deselected as cursor	->	UpdateSelectionStatus()
	+ changes it's title graphic (either by SetGraphics or by ChangeDef)	->	UpdateTitleGraphic()
	
*/

protected func Construction()
{
	_inherited();
	
	breathbar = false;
	magicbar = false;
	hotkey = false;
	
	// parallaxity
	this["Parallaxity"] = [0,0];
	
	// visibility
	this["Visibility"] = VIS_None;
	
	// health bar
	SetBarLayers(2,0);
	SetBarOffset(0,BarOffset(0),0);
	SetBarDimensions(GetDefWidth(GetID()),HealthBarHeight(),0);
	SetClrModulation(RGB(200,0,0),nil,3);
}

public func MouseSelection(int plr)
{
	if(!crew) return false;
	if(plr != GetOwner()) return false;

	// select this clonk
	SetCursor(plr, crew);
	
	return true;
}

public func SetCrew(object c)
{
	crew = c;
	UpdateHealthBar();
	UpdateBreathBar();
	UpdateMagicBar();
	UpdateTitleGraphic();
	UpdateRank();
	UpdateController();
	UpdateSelectionStatus();
	
	this["Visibility"] = VIS_Owner;
	
	//ScheduleCall(0,"SetCrew",1,0,c);
}

public func SetHotkey(int num)
{
	if(num < 0 || num > 9)
	{
		SetGraphics(nil,nil,nil,12);
		hotkey = false;
		return;
	}
	
	hotkey = true;
	var name = Format("%d",num);
	SetGraphics(name,nil,NUMB,12,GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(300,0,16000,0,300,-30000, nil, 12);
	SetClrModulation(HSL(0,0,180),nil,12);
}

public func CrewGone()
{
	RemoveObject();
}

public func UpdateController()
{
	if(!crew) return;
	// visibility
	SetOwner(crew->GetController());
}

public func UpdateSelectionStatus()
{
	if(!crew) return;
	if(!hotkey) return;

	if(crew == GetCursor(GetOwner()))
		SetClrModulation(HSL(0,0,250),nil,12);
	else
		SetClrModulation(HSL(0,0,180),nil,12);
}

public func UpdateRank()
{
	if(!crew) return;
	
	var rank = crew->GetRank();
	var nrank = rank % 25;
	var brank = rank / 25;
	
	var rankx = -1000 * GetDefWidth(GetID())/2 + 10000;
	var ranky = -15000;
	
	SetGraphics(nil,nil,RANK,10,GFXOV_MODE_Action,Format("Rank%d",nrank));
	SetObjDrawTransform(1000,0,rankx,0,1000,ranky, nil, 10);
	
	// extra rank (the star if the clonk is too experienced for normal ranks)
	if(brank > 0)
	{
		SetGraphics(nil,nil,RANK,11,GFXOV_MODE_Action,Format("RankExtra%d",brank));
		SetObjDrawTransform(1000,0,rankx-6000,0,1000,ranky-4000, nil, 11);
	}
	else
	{
		SetGraphics(nil,nil,nil,11);
	}
}

public func UpdateTitleGraphic()
{
	if(!crew) return;
	
	SetGraphics(nil,nil,crew->GetID(),1,GFXOV_MODE_Object,nil,nil,crew);
	
	//SetGraphics(nil,nil,crew->GetID(),1,GFXOV_MODE_IngamePicture);
	
	// doesn't work:
	//SetColorDw(crew->GetColorDw());
}

public func UpdateHealthBar()
{
	if(!crew) return;
	var phys = crew->GetPhysical("Energy");
	var promille;
	if(phys == 0) promille = 0;
	else promille = 1000 * crew->GetEnergy() / (phys / 1000);
	
	SetBarProgress(promille,0);
}

public func UpdateBreathBar()
{
	if(!crew) return;
	var phys = crew->GetPhysical("Breath");
	var promille;
	if(phys == 0) promille = 0;
	else promille = 1000 * crew->GetBreath() / (phys / 1000);

	// remove breath bar if full breath
	if(promille == 1000)
	{
		if(breathbar)
			RemoveBreathBar();
	}
	// add breath bar if there is none
	else
	{
		if(!breathbar)
			AddBreathBar();
		
		SetBarProgress(promille,1);
	}

}

public func UpdateMagicBar()
{
	if(!crew) return;
	var phys = crew->GetPhysical("Magic");
	var promille = 0;
	if(phys != 0) promille = 1000 * crew->GetMagicEnergy() / (phys / 1000);

	// remove magic bar if no physical magic!
	if(phys == 0)
	{
		if(magicbar)
			RemoveMagicBar();
	}
	// add magic bar if there is none
	else
	{
		if(!magicbar)
			AddMagicBar();
		
		SetBarProgress(promille,2);
	}

}

private func BarOffset(int num)
{
	var offset = GetDefWidth(GetID())/2 + HealthBarHeight()/2 + num * BarSpacing() + num;
	if(num > 0) offset += HealthBarHeight();
	if(num > 1) offset += BreathBarHeight();
	return offset;
}

private func AddBreathBar()
{
	// breath bar
	SetBarLayers(4,1);
	SetBarOffset(0,BarOffset(1),1);
	SetBarDimensions(GetDefWidth(GetID()),BreathBarHeight(),1);
	SetClrModulation(RGB(0,200,200),nil,5);
	
	breathbar = true;
	
	// update position of magic bar (if any)
	if(magicbar)
		SetBarOffset(0,BarOffset(2),2);
}

private func RemoveBreathBar()
{
	RemoveBarLayers(4);

	breathbar = false;
	
	// update position of magic bar (if any)
	if(magicbar)
		SetBarOffset(0,BarOffset(1),2);
}

private func AddMagicBar()
{
	var num = 1;
	if(breathbar) num = 2;

	SetBarLayers(6,2);
	SetBarOffset(0,BarOffset(num),2);
	SetBarDimensions(GetDefWidth(GetID()),MagicBarHeight(),2);
	SetClrModulation(RGB(0,0,200),nil,7);
	
	magicbar = true;
}

private func RemoveMagicBar()
{
	RemoveBarLayers(6);

	magicbar = false;
}
