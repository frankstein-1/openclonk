/*-- Shovel --*/

private func Hit()
{
  Sound("WoodHit");
}

public func GetCarryMode(clonk) { return CARRY_Back; }
public func GetCarryTransform(clonk)
{
	if(clonk->~GetAction() == "Dig") return Trans_Mul(Trans_Scale(2000), Trans_Translate(0,1000,0));
	else return Trans_Scale(2000);
}

public func GetCarrySpecial(clonk) { if(clonk->~GetAction() == "Dig") return "pos_hand1"; }

local fDigging;
public func IsDigging() { return fDigging; }

public func ControlUseStart(object clonk, int x, int y)
{
	if(clonk->GetAction() == "Walk")
	{
		clonk->SetAction("Dig");
		clonk->SetComDir(COMD_None);
		clonk->SetXDir(0);
		clonk->SetYDir(1);
		AddEffect("ShovelDust",clonk,1,1,this);
		fDigging = 1;
	}
	else
		clonk->CancelUse();

	return true;
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int x, int y)
{
	// something happened - don't try to dig anymore
	if(clonk->GetAction() != "Dig")
	{
		clonk->CancelUse();
		return true;
	}
	
	var angle = Angle(0,0,x,y);
	var speed = clonk->GetPhysical("Dig")/400;

	var iAnimation = EffectVar(1, clonk, GetEffect("IntDig", clonk));
	var iPosition = clonk->GetAnimationPosition(iAnimation)*180/clonk->GetAnimationLength("Dig");
	Message("%d", clonk, iPosition);
	speed = speed*(Cos(iPosition-45, 50)**2)/2500;
	Message("%d", clonk, speed);
	// limit angle
	angle = BoundBy(angle,65,300);
	clonk->SetXDir(Sin(angle,+speed),100);
	clonk->SetYDir(Cos(angle,-speed),100);
	
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	ControlUseStop(clonk, x, y);
}

public func ControlUseStop(object clonk, int x, int y)
{
	fDigging = 0;
	RemoveEffect("ShovelDust",clonk,0);
	if(clonk->GetAction() != "Dig") return true;

//	EffectCall(clonk, GetEffect("IntDig", clonk), "StopDig");
	clonk->SetXDir(0,100);
	clonk->SetYDir(0,100);
//	clonk->SetAction("Walk");
//	clonk->SetComDir(COMD_Stop);

	return true;
}

public func FxShovelDustTimer(object target, int num, int time)
{
	// Only when the clonk moves the shovel
	var iAnimation = EffectVar(1, target, GetEffect("IntDig", target));
	var iPosition = target->GetAnimationPosition(iAnimation)*100/target->GetAnimationLength("Dig");
	if(iPosition > 50)
		return;
	var xdir = target->GetXDir();
	var ydir = target->GetYDir();
	
	// particle effect
	var angle = Angle(0,0,xdir,ydir)+iPosition-25;//RandomX(-25,25);
	var groundx = Sin(angle,15);
	var groundy = -Cos(angle,15);
	var mat = GetMaterial(groundx, groundy);
	if(GetMaterialVal("DigFree","Material",mat))
		CreateParticle("Dust",groundx,groundy,RandomX(-3,3),RandomX(-3,3),RandomX(10,250),RGBa(181,137,90,80));
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 5000, def);
//  SetProperty("PerspectiveTheta", 20, def);
//  SetProperty("PerspectivePhi", 70, def);
}
