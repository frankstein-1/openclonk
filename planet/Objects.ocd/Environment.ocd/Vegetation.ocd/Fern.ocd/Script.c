/*-- Fern --*/

#include Library_Plant

func Construction()
{
	StartGrowth(1);
	inherited(...);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

public func Incineration()
{
	CastParticles("Grass",10,35,0,0,30,50,RGB(255,255,255),RGB(255,255,255));
	RemoveObject();
}

local Name = "$Name$";