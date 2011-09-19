/*--
	Catapult
	Author: Ringwaul
	
	Tosses objects farther than a clonk can. Requires no fuel.
--*/

local aim_anim;
local turn_anim;
local olddir;
local dir;
local clonkmesh;

public func IsToolProduct() { return 1; }

protected func Initialize()
{
	dir = 1;
	SetAction("Roll");
	olddir = 1;
	aim_anim =  PlayAnimation("ArmPosition", 1,  Anim_Const(0),Anim_Const(1000));
	turn_anim = PlayAnimation("TurnRight", 5, Anim_Const(GetAnimationLength("TurnRight")), Anim_Const(1000));
}

//some left-overs from Lorry script. Not sure of it's purpose...
protected func ContactLeft()
{
	if(Stuck() && !Random(5)) SetRDir(RandomX(-7, +7));
}
protected func ContactRight()
{
	if(Stuck() && !Random(5)) SetRDir(RandomX(-7, +7));
}

/* ---- CONTROLS ---- */

//Activate turn animations
func ControlLeft(object clonk)
{
	dir = 0;
	if(dir != olddir)
	{
		olddir = dir;

		//if the animation is playing for the other direction, turn back from where it already is in the animation
		var animstart = 0;
		if(GetAnimationPosition(turn_anim) != GetAnimationLength("TurnRight"))
		{
			animstart = GetAnimationLength("TurnRight") - GetAnimationPosition(turn_anim);
		}

		StopAnimation(turn_anim);
		turn_anim = PlayAnimation("TurnLeft", 5, Anim_Linear(animstart, 0, GetAnimationLength("TurnLeft"), 36 - (animstart * 204617 / 10000000), ANIM_Hold), Anim_Const(1000));
	}
}

func ControlRight(object clonk)
{
	dir = 1;
	if(dir != olddir)
	{
		olddir = dir;

		//if the animation is playing for the other direction, turn back from where it already is in the animation
		var animstart = 0;
		if(GetAnimationPosition(turn_anim) != GetAnimationLength("TurnLeft"))
		{
			animstart = GetAnimationLength("TurnLeft") - GetAnimationPosition(turn_anim);
		}

		StopAnimation(turn_anim);
		turn_anim = PlayAnimation("TurnRight", 5, Anim_Linear(animstart, 0, GetAnimationLength("TurnRight"), 36 - (animstart * 204617 / 10000000), ANIM_Hold), Anim_Const(1000));
	}
}

public func ControlUseStart(object clonk)
{
	return true;
}

public func ControlUseAltStart(object clonk)
{
	return true;
}

public func HoldingEnabled()	{	return true;	}

public func ControlUseAnyHolding(object clonk, int x, int y)
{
	ArmAnimation(x,y);
}

public func ControlUseHolding(object clonk, int x, int y)
{
	ControlUseAnyHolding(clonk,x,y);
}

public func ControlUseAltHolding(object clonk, int x, int y)
{
	ControlUseAnyHolding(clonk,x,y);
}

public func DefinePower(int x, int y)
{
	var angle = Angle(0,0,x,y);
	
	//balancing stats
	var padding = 20;

	var x2 = Sin(padding,angle);
	var y2 = -Cos(padding,angle);
	var power = Distance(x2,y2,x2+x,y2+y) - padding;

	if(power > 100) power = 100;
	if(power < 0) power = 0;
	
	return power;
}

public func ArmAnimation(int x, int y)
{
	var power = DefinePower(x,y);
	SetAnimationPosition(aim_anim, Anim_Const(759 - (power * 759 / 100)));
}

public func ControlUseStop(object clonk, int x, int y)
{
	PlayAnimation("Launch", 5, Anim_Linear(0,0, GetAnimationLength("Launch"), 10, ANIM_Remove), Anim_Const(1000));
	aim_anim = PlayAnimation("ArmPosition", 1, Anim_Linear(GetAnimationPosition(aim_anim),0, GetAnimationLength("ArmPosition"), 3, ANIM_Hold), Anim_Const(1000));

	var projectile = nil;
	if(Contents(0))	projectile = Contents(0);
	else
		if(clonk->GetItem(0)) projectile = clonk->GetItem(0);
	if(projectile) DoFire(projectile, clonk, DefinePower(x,y));
}

public func ControlUseAltStop(object clonk, int x, int y)
{
	PlayAnimation("Launch", 5, Anim_Linear(0,0, GetAnimationLength("Launch"), 10, ANIM_Remove), Anim_Const(1000));
	aim_anim = PlayAnimation("ArmPosition", 1, Anim_Linear(GetAnimationPosition(aim_anim),0, GetAnimationLength("ArmPosition"), 3, ANIM_Hold), Anim_Const(1000));

	var projectile = nil;
	if(Contents(0))	projectile = Contents(0);
	else
		if(clonk->GetItem(1)) projectile = clonk->GetItem(1);
	if(projectile) DoFire(projectile, clonk, DefinePower(x,y));
}

protected func DoFire(object iammo, object clonk, int power)
{
	//finding the spot of the catapult's arm depending on rotation
	var i = 1;
	if(dir == 0) i = -1; 
	var x = 8*i;
	var y = 28;

	iammo->Exit();
	//Put the ammo at the catapult's arm
	iammo->SetPosition(GetX() + x, GetY() - y);

	if(iammo->~IsClonk())
	{
		CatapultDismount(iammo);
		iammo->SetAction("Tumble");
	}

	//Sound
	Sound("Catapult_Launch.ogg");

	var angle = -45;
	if(dir == 1) angle = 45;
	iammo->SetVelocity(angle + GetR(), power);
}

public func ActivateEntrance(object clonk)
{
	var cnt = ObjectCount(Find_Container(this), Find_OCF(OCF_CrewMember));
	if(cnt > 0)
		if(clonk->Contained() == this)
		{
			CatapultDismount(clonk);
			clonk->Exit();
			return;
		}
		else
			return;

	if(cnt == 0)
	{
		SetAnimationPosition(aim_anim, Anim_Const(0));
		clonk->Enter(this);
		SetOwner(clonk->GetController());
		clonkmesh = AttachMesh(clonk,"shot","skeleton_body",Trans_Mul(Trans_Rotate(180,0,1,0), Trans_Translate(0,-3000,-1000)),AM_DrawBefore);
		clonk->PlayAnimation("CatapultSit", 5, Anim_Const(0), Anim_Const(1000));
	}
}

public func CatapultDismount(object clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(15));
	DetachMesh(clonkmesh);
	clonkmesh = nil;
	return true;
}

local ActMap = {
Roll = {
	Prototype = Action,
	Name = "Roll",
	Procedure = DFA_NONE,
	Directions = 1,
	FlipDir = 1,
	Length = 50,
	Delay = 2,
	X = 0,
	Y = 0,
	Wdt = 22,
	Hgt = 16,
	NextAction = "Roll",
},
};
func Definitio(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(6000,0,0),Trans_Rotate(-20,1,0,0),Trans_Rotate(35,0,1,0),Trans_Scale(1350)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Rebuy = true;
