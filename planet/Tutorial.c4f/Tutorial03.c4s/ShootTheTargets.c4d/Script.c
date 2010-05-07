/*-- Target Helper Object --*/

local count;

func Initialize()
{
	SetPosition();
	count=0;
}

global func HasHitTarget()
{
	FindObject(Find_ID(ShootTheTargets))->TargetCounter();
	OnTargetDeath(FindObject(Find_ID(ShootTheTargets))->LocalN("count"));
}

public func TargetCounter()
{
	++count;
	var i = count;
	if(i == 1) MakeTarget(349,349,true);
	if(i == 2) MakeTarget(538,362,true);
	if(i == 3) MakeTarget(1253,310,true);
	if(i == 4) MakeTarget(982,247,true);
	if(i == 5) MakeTarget(720,578,false);
	if(i == 6) MakeTarget(1380,239,true);
	if(i == 7) MakeTarget(1413,530,true);
}

protected func MakeTarget(int ix, int iy, bool flying)
{
	if(flying == nil) balloon = false;

	var target = CreateObject(PracticeTarget,ix,iy);
	if(flying == true)
	{
		var balloon = CreateObject(TargetBalloon,ix,iy-30);
		target->SetAction("Attach",balloon);
		CreateParticle("Flash",ix,iy-50,0,0,500,RGB(255,255,255));
	}

	if(flying == false)
	{
		CreateParticle("Flash",ix,iy,0,0,500,RGB(255,255,255));
		target->SetAction("Float");
	}
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}