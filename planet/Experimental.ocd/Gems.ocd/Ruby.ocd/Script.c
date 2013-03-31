/*--- Ruby ---*/

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

func Initialize()
{
	AddEffect("Sparkle", this, 10, 2, this);
	var gfx = Random(4);
	if (gfx) SetGraphics(Format("%d", gfx+1));
	return true;
}

func FxSparkleTimer(target, effect, effect_time)
{
	if(this()->Contained()) return;
	CreateParticle("MagicRing", 0, 0, 0, 0, Cos(effect_time*10, 100), RGBa(255,20,20,100), this, false);
	return true;
}

func IsValuable() { return true; }

func QueryOnSell()
{
	// Inform goal of gem sale
	var goal = FindObject(Find_ID(Goal_SellGems));
	if (goal) goal->OnGemSold();
	return false; // do perform selling
}

func Hit()
{
	Sound("GlassHit*");
	return true;
}