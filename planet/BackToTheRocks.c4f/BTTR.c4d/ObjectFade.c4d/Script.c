/*
	ObjectFade Rule
	Author: Caesar

	Removes unused objects. Like Hazard-Arena.
	
	TODO:
	Use ChangeEffect properly to save calls
*/

local fade_time;

protected func Activate(int plr)
{
	MessageWindow(GetProperty("Description"), plr);
	return true;
}

public func DoFadeTime(int to_add)
{
	fade_time += to_add;
	return;
}

protected func Initialize() 
{
	if(ObjectCount(Find_ID(Rule_ObjectFade), Find_Exclude(this))) {
		FindObject(Find_ID(Rule_ObjectFade), Find_Exclude(this))->DoFadeTime(36);
		return RemoveObject();
	}
	fade_time = 18; // 18, because the timer will check once per second, so it's aproximately a second.
}

protected func Timer() 
{
	for (var fade in FindObjects(Find_Category(C4D_Object), Find_NoContainer(), Find_Not(Find_OCF(OCF_HitSpeed1)))) 
	{
		if (fade->GetXDir() || fade->GetYDir())
			continue;
		if (fade->GetEffect("IntFadeOut", fade)) 
			continue;
		if (GBackSolid(AbsX(fade->GetX()), AbsY(fade->GetY()))) 
			continue;
		if (fade->~HasNoFadeOut())
			continue;		
		fade->AddEffect("IntFadeOut", fade, 100, 1, this, Rule_ObjectFade);
	}
}

public func FxIntFadeOutStart(object target, effect) 
{
	effect.var0 = target->GetX();
	effect.var1 = target->GetY();
	effect.var3 = target->GetClrModulation() & 0x00ffffff; //Safe pure rgb
	effect.var4 = target->GetClrModulation() >> 24 & 255; //Safe alpha
}

public func FxIntFadeOutTimer(object target, effect, int time) 
{
	if (time < fade_time * 2/3) 
		return;

	if (!(target->Contained()) && effect.var0 == target->GetX() && effect.var1 == target->GetY())
	{
		if(time >= fade_time) 
		{
			target->RemoveObject();
			return -1;
		}
	}
	else 
	{
		target->SetClrModulation(effect.var4 << 24 | effect.var3);
		return -1;
	}

	target->SetClrModulation(((fade_time - time) * effect.var4 / (fade_time/3)) << 24 | effect.var3);
	return 1;
}

public func FxIntFadeOutTimerEffect(string new_effect_name) 
{
	if (new_effect_name == "IntFadeOut")
		return -1;
}

local Name = "Object Fade";
local Description = "$Description$";
