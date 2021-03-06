/*--- Coconut ---*/

private func GetSeedTime() { return 15 * 2; }

protected func Initialize()
{
	// AddEffect to grow trees, called every half a second.
	var effect = AddEffect("IntSeed", this, 100, 18, this);
	effect.SeedTime = GetSeedTime();
	return;
}

/** Destroy coconut instead of seeding if it tries to seed outside given area
*/
public func SetConfinement(conf)
{
	this.Confinement = conf;
	return true;
}

public func FxIntSeedTimer(object coconut, proplist effect, int timer)
{
	// Start germination timer if in the environment
	if (!Contained())
		effect.SeedTime--;
		
	// Germinate if the coconut is on the earth.
	var has_soil = !!GetMaterialVal("Soil", "Material", GetMaterial(0, 3));
	if (effect.SeedTime <= 0 && !Contained() && has_soil && GetCon() >= 100)
 	{
		// Do we want to seed here?
		if (Tree_Coconut->CheckSeedChance(GetX(), GetY()))
		{
			// Are there any trees too close? Is the coconut underwater?
			if (!FindObject(Find_Func("IsTree"), Find_Distance(40)) && !GBackLiquid())
			{
				if (!this.Confinement || this.Confinement->IsPointContained(GetX(), GetY()))
				{
					if (Germinate())
						return -1;
				}
			}
		}
	}
	
	// Destruct if sitting for too long 
	if (effect.SeedTime == -120)
		coconut.Collectible = 0;
	if (effect.SeedTime < -120) 
		coconut->DoCon(-5);	
	
	return 0;
}

public func Germinate() 
{
	// Try to sprout a coconut tree.
	var d = 8, tree;
	if (tree = PlaceVegetation(Tree_Coconut, -d/2, -d/2, d, d, 100))
	{
		if (this.Confinement) tree->KeepArea(this.Confinement);
		this.Collectible = 0;	
		AddEffect("IntGerminate", this, 100, 1, this); 
		return true;
	}
	return false;
}

public func FxIntGerminateTimer(object coconut, proplist effect, int timer)
{
	if (timer == 1)
	{
		this.Collectible = 0;
		// Tree sprouts
		PlaceVegetation(Tree_Coconut, 0, 0, 1,1, 100);
	}
	// Fade out
	SetObjAlpha(255 - (timer * 255 / 100));
	if (timer == 100)
		coconut->RemoveObject();
	return;
}
		
/*-- Eating --*/

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
	return true;
}

public func NutritionalValue() { return 5; }

/*-- Bounce --*/

protected func Hit(int dx, int dy)
{
	// Bounce: useful for spreading seeds further from parent tree.
	if (dy > 1)
		SetYDir(dy / -2, 100);
		
	StonyObjectHit(dx, dy);
	return;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local BlastIncinerate = 8;
local ContactIncinerate = 2;
local Confinement;