/*-- Shovel --*/

private func Hit()
{
  Sound("WoodHit");
}

public func GetCarryMode() { return CARRY_Back; }
public func GetCarryTransform() { return Trans_Scale(2000); }

public func GetCarrySpecial(clonk) { if(clonk->~GetAction() == "Dig") return "pos_hand1"; }

public func ControlUseStart(object clonk, int x, int y)
{
	if(clonk->GetAction() == "Walk")
	{

		clonk->SetAction("Dig");
		clonk->SetComDir(COMD_None);
		clonk->SetXDir(0);
		clonk->SetYDir(1);
		AddEffect("ShovelDust",clonk,1,1,this);
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
	var speed = clonk->GetPhysical("Dig")/500;
	
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

	RemoveEffect("ShovelDust",clonk,0);
	if(clonk->GetAction() != "Dig") return true;
	
	clonk->SetAction("Walk");
	clonk->SetComDir(COMD_Stop);

	return true;
}

public func FxShovelDustTimer(object target, int num, int time)
{
	var xdir = target->GetXDir();
	var ydir = target->GetYDir();
	
	// particle effect
	var angle = Angle(0,0,xdir,ydir)+RandomX(-25,25);
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
  SetProperty("PerspectiveTheta", 20, def);
  SetProperty("PerspectivePhi", 70, def);
}
