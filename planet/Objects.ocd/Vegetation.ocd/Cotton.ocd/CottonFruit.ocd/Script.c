/*
	Cotton Fruit
	Author: Clonkonaut, Win
*/

// Animation length is 875
// At about 500 the fruit starts filling up with gas
local first_animation_stage = 500;
// Growing time for first stage in frames
local grow_time = 3100;
local grow_anim;
local attach_branch;
// Filling time & also animation duration for second stage
local fill_time = 3100;
// When the balloon is full of gas, it's ripe!
local ripened = false;
// Time the filled balloon will stay on the branch (time the player has to prepare for harvesting)
local ripe_time = 3500;
// Time the player has for harvesting before the balloon starts rising
local float_time = 350;
// Half of the time the balloon will fly up and half will just drift with the wind
local fly_time = 700;

private func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(180, 0,0,1));
}

/* Growing & filling with gas */

public func Grow(int branch, bool fullgrown)
{
	attach_branch = branch;
	if (!fullgrown)
	{
		grow_anim = PlayAnimation("grow", 1, Anim_Linear(0,0, first_animation_stage, grow_time, ANIM_Hold), Anim_Const(1000));
		AddTimer("Growing", 35);
		ScheduleCall(this, "Fill", grow_time);
	}
	else
	{
		if (!grow_anim) grow_anim = PlayAnimation("grow", 1, Anim_Linear(GetAnimationLength("grow"),0, GetAnimationLength("grow"), 1, ANIM_Hold), Anim_Const(1000));
		else SetAnimationPosition(grow_anim, Anim_Const(GetAnimationLength("grow")));
		if (Contained())
		{
			Contained()->UpdateFruitAttachTransform(attach_branch, 2000);
			Contained()->FruitFills(attach_branch, nil, true);
		}
		Ripen();
	}
}

public func IsGrowing()
{
	return !ripened;
}

// Every ~1 second update scaling on cotton plant
private func Growing()
{
	// Shouldn't happen
	if (!Contained()) return RemoveTimer("Growing");
	Contained()->UpdateFruitAttachTransform(attach_branch, GetAnimationPosition(grow_anim) * 1000 / first_animation_stage);
	if (GetAnimationPosition(grow_anim) >= first_animation_stage) RemoveTimer("Growing");
}

// The fruit is now grown and starts filling with gas, this will also move the branch
private func Fill()
{
	// No plant, no fill
	if (!Contained()) return;
	SetAnimationPosition(grow_anim, Anim_Linear(GetAnimationPosition(grow_anim), 0, GetAnimationLength("grow"), fill_time, ANIM_Hold));
	Contained()->FruitFills(attach_branch, fill_time, false); // Will start the branch's animation
	ScheduleCall(this, "Ripen", fill_time);
}

/* Ripening & Flying */

private func Ripen()
{
	SetMeshMaterial("Cotton_Fruit_Ripe");
	ripened = true;
	AddEffect("IntPrepareFlight", this, 1, 1, this);
}

private func FxIntPrepareFlightTimer(object target, proplist effect, int time)
{
	if (!Contained()) return FX_Execute_Kill;
	if (time > 10 && !effect.shakes)
	{
		effect.shakes = 1;
		Contained()->ShakeFruit(attach_branch);
	}
	if (time > ripe_time / 2 && effect.shakes == 1)
	{
		effect.shakes = 2;
		Contained()->ShakeFruit(attach_branch);
	}
	if (time >= ripe_time)
	{
		Fly();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

// May be called from elsewhere e.g. the cotton plant when harvesting with a sickle
public func Fly() // you fools
{
	var plant = Contained();
	if (plant)
	{
		var pos = plant->GetFruitExitPosition(attach_branch);
		Exit(pos.x, pos.y);
		plant->DetachFruit(attach_branch); // bye-bye
	}
	SetAction("Fly");
	SetYDir(-1);
	var effect = AddEffect("IntFlight", this, 1, 5, this);
	effect.random = Random(360);
}

private func FxIntFlightTimer(object target, proplist effect, int time)
{
	var xdir = GetXDir();
	var ydir = GetYDir();
	if (time < float_time)
	{
		ydir += Sin(time + effect.random, 2);
	}
	// Fly upwards
	if (time > float_time)
	{
		if (time < float_time + fly_time / 2)
			ydir -= Max(Tan((900 - time + float_time)%900, 10, 10), 0);
		// Fly with the wind
		if (GetWind())
			xdir += Tan((time - float_time)%900, GetWind(), 10);
		else if (xdir != 0)
			xdir -= xdir / Abs(xdir);
	}
	if (time > float_time + fly_time / 2)
	{
		if (ydir < 0) ydir += 1;
		// ~1 second until pop!
	}
	if (time > float_time + fly_time + 35)
	{
		Pop();
		// Pop deletes this object
		return FX_Execute_Kill;
	}

	SetSpeed(xdir, ydir);
	SetProperty("MeshTransformation", Trans_Rotate(180 + BoundBy(xdir, -50, 50), 0,0,1));
	return FX_OK;
}

/* Shooting, harvesting, popping */

public func IsProjectileTarget() { return true; }
public func IsHarvestable() { return true; }
public func SickleHarvesting() { return true; }

public func OnProjectileHit() { Pop(); }
public func Damage() { Pop(); }
public func Harvest() { Pop(); }

public func Pop()
{
	CreateParticle("CottonBalloon", 0,0, PV_Random(-15,15), PV_Random(-10,-5), 100, Particles_CottonBalloon(), RandomX(4,8));
	var seed = CreateObject(CottonSeed);
	if (OnFire()) seed->Incinerate();
	RemoveObject();
}

/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	// Do not save fruit inside a cotton plant
	if (Contained())
		if (Contained()->GetID() == Cotton)
			return false;
	var effect;
	if (effect = GetEffect("IntFlight", this))
	{
		props->AddCall("Flight", this, "ResumeFlight", effect.time, effect.random);
	}
	return true;
}

// Loading
private func ResumeFlight(int effect_time, int effect_random)
{
	var effect = AddEffect("IntFlight", this, 1, 5, this);
	effect.time = effect_time;
	effect.random = effect_random;
}

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Speed = 50,
		Accel = 5,
		NextAction = "Fly",
	},
};

local Plane = 460;
local BlastIncinerate = 1;
local ContactIncinerate = 3;