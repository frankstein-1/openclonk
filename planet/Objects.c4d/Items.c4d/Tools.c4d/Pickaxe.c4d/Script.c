/*
	Pickaxe
	Author: Ringwaul

	A useful but tedious tool for breaking through rock without
	explosives.
*/

local maxreach;
local swingtime;
local held;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarryTransform()
{ 
	if(held==false);
	return Trans_Rotate(-90, 0, 1, 0);
}

//TODO: The pick should probably have an internal array that
//keeps the data of how much of which material has been dug.
//I wouldn't have a clue how to do this, so if anyone else 
//wants to make that, good luck! :)

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(40, 0, 0, 1),Trans_Rotate(150, 0, 1, 0), Trans_Scale(900), Trans_Translate(600, 400, 1000)),def);
}

protected func Initialize()
{
	//maxreach is the length of the pick from the clonk's hand
	maxreach=16;
	swingtime=0;
}

private func Hit()
{
	Sound("RockHit");
	return 1;
}

func ControlUseStart(object clonk, int ix, int iy)
{
	held=true;
	return true;
}

protected func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int ix, int iy)
{
	//Can clonk use pickaxe?
	if(clonk->GetProcedure() != "WALK")
	{
		clonk->CancelUse();
		return 1;
	}

	++swingtime;
	Message("%d",this,swingtime);
	if(swingtime>=108) //Waits three seconds for animation to run (we could have a clonk swing his pick 3 times)
	{
		DoSwing(clonk,ix,iy);
		clonk->CancelUse();
		return 1;
	}
	return true;
}

func ControlUseStop(object clonk, int ix, int iy)
{
	held=false;
	swingtime=0;
	return true;
}

protected func DoSwing(object clonk, int ix, int iy)
{
	var angle = Angle(0,0,ix,iy);

	//Creates an imaginary line which runs for 'maxreach' distance (units in pixels)
	//or until it hits a solid wall.
	var iDist=0;
	while(!GBackSolid(Sin(180-angle,iDist),Cos(180-angle,iDist)) && iDist < maxreach)
	{
		++iDist;
	}

	var x = Sin(180-angle,iDist);
	var y = Cos(180-angle,iDist);

	if(GBackSolid(x,y))
	{
		Message("Hit %s",this, MaterialName(GetMaterial(x,y))); //for debug
		
		//special effects
		if(GetMaterialVal("DigFree","Material",GetMaterial(x,y))==0)
		{
			CastParticles("Spark",RandomX(3,9),35,x*9/10,y*9/10,10,30,RGB(255,255,150),RGB(255,255,200));
		}

		//dig out resources too! Don't just remove landscape pixels
		BlastFree(x,y,9);

		//stops resources from launching into clonk while mining
		for(var resources in FindObjects(Find_Distance(7,x,y), Find_Category(C4D_Object), Find_Not(Find_OCF(OCF_Alive))))
			resources->SetSpeed();
	}
	else
		Message("Hit nothing",this); //for debug
}

protected func ControlUseCancel(object clonk, int ix, int iy)
{
	held=false;
	swingtime=0;
	return true;
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }
