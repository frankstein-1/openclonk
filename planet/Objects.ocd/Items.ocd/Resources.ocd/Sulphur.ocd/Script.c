/*-- Sulphur --*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

protected func Hit()
{
	Sound("CrystalHit?");
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
