/**
	Sawmill
	Cuts trees or other objects into wood. Accepts only objects purely made from wood.
	
	@authors Ringwaul, Clonkonaut
*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer

public func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(-20, 0, 1, 0));
	SetAction("Default");
	return _inherited(...);
}

public func Initialize()
{
	this.SpinAnimation = PlayAnimation("work", 10, Anim_Const(0), Anim_Const(1000));
	AddTimer("CollectTrees", 4);
	return _inherited(...);
}

/*-- Interaction --*/

// Sawmill acts as a container to be able to collect wooden objects.
public func IsContainer() { return true; }

// Do not show normal inventory menu. Instead we show the remaining wood in an extra menu.
public func RejectContentsMenu() { return true; }

// Sawmill can't be interacted with.
public func IsInteractable() { return false; }

// Automatically search for trees in front of sawmill. Temporary solution?
protected func FindTrees()
{
	var tree = FindObject(Find_AtPoint(), Find_Func("IsTree"), Find_Not(Find_Func("IsStanding")), Find_Func("GetComponent", Wood));
	if (!tree)
		return;
	
	return Saw(tree);
}

// Returns whether the object is made purely out of wood.
private func CheckWoodObject(object target)
{
	if (target->GetComponent(nil, 0) != Wood) 
		return false;
	if (target->GetComponent(nil, 1)) 
		return false;
	return true;
}

/*-- Production --*/

// Overload production menu entries to show helpful hint to player.
public func GetProductionMenuEntries()
{
	var wood_count = ContentsCount(Wood);
	var info_text =
	{
		Right = "100%", Bottom = "6em",
		text = {Left = "2em", Text = "$AutoProduction$", Style = GUI_TextVCenter | GUI_TextHCenter},
		image = {Right = "2em", Bottom = "2em", Symbol = Wood},
		queue =
		{
			Top = "100% - 1.5em",
			Style = GUI_TextRight,
			Text = Format("$WoodInQueue$: %2d {{Wood}}", wood_count)
		}
	};
	return [{symbol = Wood, custom = info_text}];
}

private func IgnoreKnowledge() { return true; }

public func Saw(object target)
{
	if (target->Contained() != this)
		target->Enter(this);
	var output = target->GetComponent(Wood);
	target->Split2Components();
	AddToQueue(Wood, output);
	// Refresh interaction menus to show the wood count.
	UpdateInteractionMenus(this.GetProductionMenuEntries);
	return true;
}

private func IsProduct(id product_id)
{
	return product_id->~IsSawmillProduct();
}
private func ProductionTime(id toProduce) { return 100; }
public func PowerNeed() { return 20; }

public func OnProductionStart(id product)
{
	if (!GetEffect("Sawing", this))
	{
		SpinOn();
		AddEffect("Sawing", this, 100, 1, this);
	}
}

public func OnProductionHold(id product)
{
	SpinOff();
	RemoveEffect("Sawing", this);
}

public func OnProductionContinued(id product)
{
	if (!GetEffect("Sawing", this))
	{
		SpinOn();
		AddEffect("Sawing", this, 100, 1, this);
	}
}

public func OnProductionFinish(id product)
{
	if (!GetLength(queue))
	{
		SpinOff();
		RemoveEffect("Sawing", this);
	}
}	

// Timer, check for objects to collect in the designated collection zone
public func CollectTrees()
{
	if (GetCon() < 100) 
		return;
	// Only take one tree at a time
	if (GetLength(queue) == 0)
		FindTrees();
	return;
}

protected func Collection(object obj)
{
	Sound("Clonk");
	Saw(obj);
}

public func FxSawingTimer(object target, proplist effect, int time)
{
	var dir = GetCalcDir();
	if (time >= this.SpinStep * 3 && time % 5)
		CreateParticle("WoodChip", PV_Random(-7 * dir, -3 * dir), PV_Random(3, 6), PV_Random(-5 * dir, -11 * dir), PV_Random(-4, -2), PV_Random(36 * 3, 36 * 10), Particles_WoodChip(), 3);

	if (!(time % 20))
		Smoke(10 * GetCalcDir(),10,10);
}

public func OnProductEjection(object product)
{
	product->SetPosition(GetX() - 25 * GetCalcDir(), GetY() - 8);
	product->SetSpeed(-7 * GetCalcDir(), 5);
	product->SetR(30 - Random(59));
	Sound("Pop");
}

protected func RejectCollect(id id_def, object collect)
{
	// Don't collect wood
	if (id_def == Wood) 
		return true;
	if (collect->~IsSawmillIngredient() || CheckWoodObject(collect)) 
		return false;
	return true;
}

/*-- Animation --*/

private func SpinOn(int call)
{
	var spin;
	// Slowest spin on first call
	if (!call) { spin = 100; SetMeshMaterial("Beltspin", 1); ClearScheduleCall(this, "SpinOff"); }
	if (call == 1) spin = 75;
	if (call == 2) spin = 50;
	if (call == 3) { spin = 30; SetMeshMaterial("SawmillBlade.Spin", 2); }

	SetAnimationPosition(this.SpinAnimation, Anim_Linear(GetAnimationPosition(this.SpinAnimation), 0, GetAnimationLength("work"), spin, ANIM_Loop));

	if (call < 3) ScheduleCall(this, "SpinOn", this.SpinStep, nil, call+1);
	else Sound("SawmillRipcut", nil, nil, nil, +1);
}

private func SpinOff(int call, int animation_no)
{
	var spin;
	if (!call) { spin = 50; SetMeshMaterial("SawmillBlade", 2); Sound("SawmillRipcut", nil, nil, nil, -1); ClearScheduleCall(this, "SpinOn"); }
	if (call == 1) spin = 75;
	if (call == 2) spin = 100;
	if (call == 3) spin = 150;
	if (call == 4)
	{
		SetMeshMaterial("SawmillBelt", 1);
		SetAnimationPosition(this.SpinAnimation, Anim_Const(GetAnimationPosition(this.SpinAnimation)));
		return;
	}

	SetAnimationPosition(this.SpinAnimation, Anim_Linear(GetAnimationPosition(this.SpinAnimation), 0, GetAnimationLength("work"), spin, ANIM_Loop));

	ScheduleCall(this, "SpinOff", this.SpinStep * 2, nil, call+1);
}

/*-- Properties --*/

local ActMap = {
		Default = {
			Prototype = Action,
			Name = "Default",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 1,
			NextAction = "Default",
		},
};

func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000, 0, 7000), Trans_Rotate(-20, 1, 0, 0),Trans_Rotate(30, 0, 1, 0)), def);
}
local Name = "$Name$";
local Description = "$Description$";
local SpinStep = 30;
local ContainBlast = true;
local BlastIncinerate = 100;
local HitPoints = 70;
