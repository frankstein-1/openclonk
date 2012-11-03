/*--
	Waterfall
	Author: Maikel
	
	Waterfall object, use to place waterfalls in the landscape.	
--*/


protected func Initialize()
{
	
	return;
}

/*-- Waterfall --*/

global func CreateWaterfall(int x, int y, int strength, string mat)
{
	var fall = CreateObject(Waterfall, x, y, NO_OWNER);
	AddEffect("IntWaterfall", fall, 100, 1, fall, nil, x, y, strength, mat);
	return fall;
}

protected func FxIntWaterfallStart(object target, proplist effect, int temporary, int x, int y, int strength, string mat)
{
	if (temporary)
		return 1;
	effect.X = x;
	effect.Y = y;	
	effect.Strength = strength;
	effect.Material = mat;
	// Start sound.
	target->Sound("Waterfall", false, 10 * effect.Strength, nil, 1);
	return 1;
}

protected func FxIntWaterfallTimer(object target, proplist effect)
{
	// Insert liquid at location every frame.
	for (var i = 0; i < effect.Strength / 2; i++)
		InsertMaterial(Material("Water"), AbsX(effect.X), AbsY(effect.Y), effect.XDir + Random(effect.XVar), effect.YDir + Random(effect.YVar));
	return 1;
}

protected func FxIntWaterfallStop(object target, proplist effect, bool temporary)
{
	if (temporary)
		return 1;
	// Stop sound.	
	target->Sound("Waterfall", false, 10 * effect.Strength, nil, -1);
	return 1;
}

public func SetStrength(int strength)
{
	var effect = GetEffect("IntWaterfall", this);
	if (effect)
		effect.Strength = BoundBy(strength, 0, 100);
	return;
}

public func SetMaterial(int material)
{

	return;
}

public func SetDirection(int xdir, int ydir, int xvar, int yvar)
{
	var effect = GetEffect("IntWaterfall", this);
	if (effect)
	{
		effect.XDir = xdir; 
		effect.YDir = ydir; 
		effect.XVar = xvar; 
		effect.YVar = yvar; 
	}	
	return;
}

public func SetSoundLocation(int x, int y)
{
	SetPosition(x, y);
	return;
}



/*-- Liquid Drain --*/

global func CreateLiquidDrain(int x, int y, int strength)
{
	var drain = CreateObject(Waterfall, x, y, NO_OWNER);
	AddEffect("IntLiquidDrain", drain, 100, 1, drain, nil, x, y, strength);
	return drain;
}

protected func FxIntLiquidDrainStart(object target, proplist effect, int temporary, int x, int y, int strength)
{
	if (temporary)
		return 1;
	effect.X = x;
	effect.Y = y;	
	effect.Strength = strength;
	return 1;
}

protected func FxIntLiquidDrainTimer(object target, proplist effect)
{
	for (var i = 0; i < effect.Strength / 2; i++)
		ExtractLiquid(AbsX(effect.X), AbsY(effect.Y));
	return 1;
}



local Name = "$Name$";