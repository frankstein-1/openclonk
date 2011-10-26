/*
	Crate
	Author: Ringwaul

	Used for deliveries.
*/

#include Library_ItemContainer
#include Library_CarryHeavy

local crateanim;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

protected func Construction()
{
	crateanim = PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20,80),0,1,0));
	return _inherited(...);
}

/*-- Contents --*/

private func MenuOnInteraction() { return true; }

private func MaxContentsCount()
{
	return 4;
}

func GetInteractionMetaInfo(object clonk)
{
	if(content_menu)
		return { Description = "$CloseCrate$", IconName = nil, IconID = nil, Selected = true };
	else
		return { Description = "$OpenCrate$", IconName = nil, IconID = nil, Selected = false };
}

// callback: menu was closed
func MenuClosed()
{
	Close();
}

private func OnContentMenuOpen() { Open(); }

private func Open()
{
	StopAnimation(crateanim);
	crateanim = PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Hold), Anim_Const(1000));
	Sound("ChestOpen");
}

private func Close()
{
	StopAnimation(crateanim);
	crateanim = PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Hold), Anim_Const(1000));
	Sound("ChestClose");
}

protected func Definition(def)
{
		SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30,1,0,0), Trans_Rotate(30,0,1,0), Trans_Translate(1000,1,0)),def);
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Name = "$Name$";
local Collectible = false;
local Touchable = 2;