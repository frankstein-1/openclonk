/*-- 
	Javelin
	Author: Ringwaul
	
	A simple but dangerous throwing weapon.
--*/

#include Library_Stackable

public func MaxStackCount() { return 3; }

local fAiming;
local power;

local iAim;
local fWait;

local target_angle;

public func GetCarryMode(clonk) { if(fAiming >= 0) return CARRY_HandBack; }
public func GetCarryBone() { return "Javelin"; }
public func GetCarrySpecial(clonk) { if(fAiming > 0) return "pos_hand2"; }
public func GetCarryTransform() { if(fAiming == 1) return Trans_Rotate(180, 1, 0, 0); }

public func ControlUseStart(object clonk, int x, int y)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
	{
		fWait = 1;
		return true;
	}
	fWait = false;
	fAiming=true;
	clonk->SetHandAction(1);
	clonk->UpdateAttach();
	iAim = clonk->PlayAnimation("SpearAimArms", 10, Anim_Const(0), Anim_Const(1000));

	// Aim timer
	if(!GetEffect("IntAiming", clonk))
		AddEffect("IntAiming", clonk, 1, 1, this);
	
	Sound("DrawJavelin.ogg");
	
	var angle = Angle(0,0,x,y);
	angle = Normalize(angle,-180);
	clonk->SetAnimationPosition(iAim, Anim_Const(Abs(angle)*clonk->GetAnimationLength("SpearAimArms")/180));
	return 1;
}

public func HoldingEnabled() { return true; }

func FxIntAimingTimer(clonk, number)
{
	if(fWait)
	{
		if(clonk->HasHandAction())
			ControlUseStart(clonk);
		return 0;
	}
	// check procedure
	if(!clonk->ReadyToAction())
	{
		ResetClonk(clonk);
		fWait = 1;
		return true;
	}

	var iTargetPosition = Abs(target_angle)*clonk->GetAnimationLength("SpearAimArms")/180;
	var iPos = clonk->GetAnimationPosition(iAim);
	iPos += BoundBy(iTargetPosition-iPos, -50, 50);
	clonk->SetAnimationPosition(iAim, Anim_Const(iPos));
}

protected func ControlUseHolding(object clonk, int ix, int iy)
{
	// angle
	var angle = Angle(0,0,ix,iy);
	target_angle = Normalize(angle,-180);

	return 1;
}

static const Javelin_ThrowTime = 16;

protected func ControlUseStop(object clonk, int ix, int iy)
{
	RemoveEffect("IntAiming", clonk);
	
	var angle = clonk->GetAnimationPosition(iAim)*180/(clonk->GetAnimationLength("SpearAimArms"));
	if(!clonk->GetDir()) angle = -angle;

	var iThrowtime = Javelin_ThrowTime;
	if(Abs(angle) < 90)
	{
		iAim = clonk->PlayAnimation("SpearThrow2Arms",  10, Anim_Linear(0, 0, clonk->GetAnimationLength("SpearThrow2Arms" ), iThrowtime), Anim_Const(1000));
		iAim = clonk->PlayAnimation("SpearThrowArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("SpearThrowArms"), iThrowtime), Anim_Const(1000), iAim);
		clonk->SetAnimationWeight(iAim+1, Anim_Const(1000*Abs(angle)/90));
	}
	else
	{
		iAim = clonk->PlayAnimation("SpearThrowArms",  10, Anim_Linear(0, 0, clonk->GetAnimationLength("SpearThrowArms" ), iThrowtime), Anim_Const(1000));
		iAim = clonk->PlayAnimation("SpearThrow3Arms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("SpearThrow3Arms"), iThrowtime), Anim_Const(1000), iAim);
		clonk->SetAnimationWeight(iAim+1, Anim_Const(1000*(Abs(angle)-90)/90));
	}

	ScheduleCall(this, "DoThrow", iThrowtime/2, 1, clonk, angle);
}

protected func ControlUseCancel(object clonk, int x, int y)
{
	RemoveEffect("IntAiming", clonk);
  if(fWait) return;
  fAiming = 0;
  ResetClonk(clonk);
}

public func DoThrow(object clonk, int angle)
{
	var javelin=TakeObject();
	
	// how fast the javelin is thrown depends very much on
	// the speed of the clonk
	var speed = 1000 * clonk->GetPhysical("Throw") / 12000 + 100 * Abs(clonk->GetXDir())*2;
	var xdir = Sin(angle,+speed);
	var ydir = Cos(angle,-speed);
	javelin->SetXDir(xdir,1000);
	javelin->SetYDir(ydir,1000);
	
	javelin->AddEffect("Flight",javelin,1,1,javelin,nil);
	javelin->AddEffect("HitCheck",javelin,1,1,nil,nil,clonk);
	
	Sound("ThrowJavelin*.ogg");
	
	power=0;
	
	fAiming = -1;
	clonk->UpdateAttach();
	ScheduleCall(this, "ResetClonk", Javelin_ThrowTime/2, 1, clonk);
}

public func ResetClonk(clonk)
{
	fAiming = 0;

	clonk->SetHandAction(0);

	clonk->StopAnimation(clonk->GetRootAnimation(10));

	clonk->UpdateAttach();
}

protected func JavelinStrength() { return 14; }

//slightly modified HitObject() from arrow
public func HitObject(object obj)
{
	var speed = Sqrt(GetXDir()*GetXDir()+GetYDir()*GetYDir());

	if(obj->~QueryCatchBlow(this)) return;
  
	// arrow hit
	obj->~OnArrowHit(this);
	if(!this) return;
	// ouch!
	Sound("ProjectileHitLiving*.ogg");
	
	var dmg = JavelinStrength()*speed/100;
	if(obj->GetAlive())
	{
		obj->DoEnergy(-dmg);
	    obj->~CatchBlow(-dmg,this);
	}
	
	RemoveEffect("HitCheck",this);

	// target could have done something with this arrow
	if(!this) return;
	
	// tumble target
    if(obj->GetAlive())
    {
		obj->SetAction("Tumble");
		obj->SetSpeed(obj->GetXDir()+GetXDir()/3,obj->GetYDir()+GetYDir()/3-1);
    }
}

protected func Hit()
{	
	if(GetEffect("Flight",this))
	{
		Sound("JavelinHitGround.ogg");
		
		SetSpeed();

		RemoveEffect("Flight",this);
		RemoveEffect("HitCheck",this);
		
		var x=Sin(GetR(),+16);
		var y=Cos(GetR(),-16);
		var mat = GetMaterial(x,y);
		if(mat != -1)
		{
			if(GetMaterialVal("DigFree","Material",mat))
			{
			// stick in landscape
			SetVertex(2,VTX_Y,-18,1);
			}
		}
		return;
	}
	
	Sound("WoodHit");
}

func Entrance()
{
	// reset sticky-vertex
	SetVertex(2,VTX_Y,0,1);
}

protected func FxFlightStart(object pTarget, int iEffectNumber)
{
	pTarget->SetProperty("Collectible",0);
	pTarget->SetR(Angle(0,0,pTarget->GetXDir(),pTarget->GetYDir()));
}

protected func FxFlightTimer(object pTarget,int iEffectNumber, int iEffectTime)
{
	//Using Newton's arrow rotation. This would be much easier if we had tan^-1 :(
	var oldx = EffectVar(0,pTarget,iEffectNumber);
	var oldy = EffectVar(1,pTarget,iEffectNumber);
	var newx = GetX();
	var newy = GetY();

	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	pTarget->SetRDir(anglediff/2);
	pTarget->EffectVar(0,pTarget,iEffectNumber) = newx;
	pTarget->EffectVar(1,pTarget,iEffectNumber) = newy;
	pTarget->SetR(Angle(0,0,pTarget->GetXDir(),pTarget->GetYDir()));
}

protected func FxFlightStop(object pTarget,int iEffectNumber)
{
	pTarget->SetProperty("Collectible", 1);
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 15000, def);
  SetProperty("PerspectiveTheta", 10, def);
  SetProperty("PerspectivePhi", -10, def);
}