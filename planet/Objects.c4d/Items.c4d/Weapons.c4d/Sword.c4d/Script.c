/*-- Sword --*/

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryTransform() { return Trans_Scale(130); }
public func GetCarryBone() { return "Main"; }

local fDrawn;
local fAttack;
local iAnimStrike;

static const SWOR_StrikeTime = 35;

public func ControlUse(object clonk, int x, int y)
{
	if(fDrawn && !fAttack)
	{
		Message("!Attack!", this);
		fAttack = 1;
//		if(iAnimStrike) clonk->StopAnimation(iAnimStrike);
		iAnimStrike = clonk->PlayAnimation("StrikeArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("StrikeArms"), SWOR_StrikeTime, ANIM_Remove), Anim_Const(1000));
		ScheduleCall(this, "DoStrike", 15*SWOR_StrikeTime/20, 1, clonk);  // Do Damage at animation frame 15 of 20
		ScheduleCall(this, "StrikeEnd", SWOR_StrikeTime, 1, clonk);
	}
	else if(clonk->GetAction() == "Walk" && !fAttack)
	{
		DrawSword(0);
		Message("Draw!", this);
	}

	return true;
}

public func DoStrike(clonk)
{
	if(!fAttack) return;
	clonk->CreateParticle("Blast", 20*(-1+2*clonk->GetDir()), 0, 0, 0, 20);
	// TODO Make Damage!
}

public func StrikeEnd(clonk)
{
	clonk->StopAnimation(iAnimStrike);
	fAttack = 0;
}

public func DrawSword(fUndraw)
{
	if(fDrawn != fUndraw) return 0;
	if(fDrawn) RemoveEffect("IntSword", Contained());
	else AddEffect("IntSword", Contained(), 1, 1, this);
	return 1;	
}

func FxIntSwordStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
//	pTarget->SetPhysical("Walk", 20000, 2);
	fDrawn = 1;
	fAttack = 0;
}

func FxIntSwordTimer(pTarget, iNumber, iTime)
{
	if(pTarget->GetAction() != "Walk" && pTarget->GetAction() != "Jump") return -1;
}

func FxIntSwordStop(pTarget, iNumber, iReason, fTmp)
{
	if(fTmp) return;
	if(fAttack) StrikeEnd(pTarget);
  fDrawn = 0;
}

local itemmesh;

public func Entrance(pTarget)
{
	fDrawn = 0;
}

public func Departure()
{
	// if the item had a holdmode detach mesh
}

public func Selection(pTarget, fSecond)
{
//	if(second) return;
}

public func Deselection(pTarget, fSecond)
{
	DrawSword(1);
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}