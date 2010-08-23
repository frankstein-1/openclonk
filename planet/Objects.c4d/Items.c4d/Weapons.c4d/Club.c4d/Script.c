/*-- Club --*/

#include Library_MeleeWeapon

private func Hit()
{
	Sound("WoodHit");
}

public func GetCarryMode() { return CARRY_HandBack; }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }

local animation_set;

func Initialize()
{
	animation_set = {
		AimMode         = AIM_Weight,
		AnimationAim    = "BatAimArms",
		AnimationAim2   = "BatAim2Arms",
		AimTime         = 35*3,
		AnimationShoot  = "BatStrikeArms",
		AnimationShoot2 = "BatStrike2Arms",
		ShootTime       = 35/2,
		ShootTime2      = (35/2)*6/19, // At 6/19 of the shooting animation
	};
}

public func GetAnimationSet() { return animation_set; }

public func HoldingEnabled() { return true; }

local fAiming;

public func ControlUseStart(object clonk, int x, int y)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
		return true;

	fAiming = true;

	clonk->StartAim(this);
	return 1;
}

public func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy);
	angle = Normalize(angle,-180);

	clonk->SetAimPosition(angle);

	return true;
}

protected func ControlUseStop(object clonk, ix, iy)
{
	clonk->StopAim();
	return true;
}

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	clonk->StartShoot(this);
	return true;
}

protected func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	return true;
}

public func Reset(clonk)
{
	fAiming = 0;
}

// Called in the half of the shoot animation (when ShootTime2 is over)
public func DuringShoot(object clonk, int angle)
{
	DoStrike(clonk, angle);
}

func DoStrike(clonk, angle)
{
	var x=Sin(angle, 7);
	var y=-Cos(angle, 7);
	
	for(var obj in FindObjects(Find_Distance(7, x, y), Find_Or(Find_OCF(OCF_Alive), Find_Category(C4D_Object)), Find_Exclude(clonk), Find_NoContainer()))
	{
		if(obj->GetOCF() & OCF_Alive)
		{
			ApplyWeaponBash(obj, 400, angle);
		}
		else
		{
			obj->SetXDir((obj->GetXDir(100) + Sin(angle, 2000)) / 2, 100);
			obj->SetYDir((obj->GetYDir(100) - Cos(angle, 2000)) / 2, 100);
		}
	}
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("Description", "$Description$", def);
	SetProperty("PictureTransformation",Trans_Rotate(-30,0,0,1),def);
}
