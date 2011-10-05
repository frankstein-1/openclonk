/*-- Mushroom --*/

#include Library_Plant

private func SeedChance() { return 250; }
private func SeedAreaSize() { return 150; }
private func SeedAmount() { return 5; }

/* Initialisation */

func Construction()
{
	StartGrowth(3);
	_inherited(...);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

/* Harvesting */

private func CanBeHarvested() { return true; }

public func Harvest(object clonk)
{
	this.Collectible = 1;
	clonk->Collect(this);
	return true;
}

public func IsInteractable(object clonk)
{
	return GetProperty("Collectible") != 1 && inherited(clonk);
}

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$PickMushroom$" };
}

/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->DoEnergy(10);
	RemoveObject();
}

local Name = "$Name$";
local Collectible = 0;