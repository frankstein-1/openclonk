/*--
		Item restorer
		Author: Maikel

		Restores an item and transports it back to its original position or container,
		can be used to prevent players from messing up tutorials by wasting items.
--*/


/*-- Item Restoration --*/

protected func Initialize()
{
	return;
}

public func SetRestoreObject(object to_revive, object to_container, int to_x, int to_y, string ctrl_string)
{
	to_revive->Enter(this);
	var effect = AddEffect("Restore", this, 100, 2, this);
	EffectVar(0, this, effect) = to_revive;
	EffectVar(1, this, effect) = to_container;
	EffectVar(2, this, effect) = to_x;
	EffectVar(3, this, effect) = to_y;
	EffectVar(4, this, effect) = ctrl_string;
	return;
}

// Restore effect.
// Effectvar 0: Object to revive.
// Effectvar 1: Container to which must be revived.
// Effectvar 2: x-coordinate to which must be revived.
// Effectvar 3: y-coordinate to which must be revived.
protected func FxRestoreStart(object target, int num, int temporary)
{
	return 1;
}

protected func FxRestoreTimer(object target, int num, int time)
{
	// Remove effect if there is nothing to revive.
	if (!EffectVar(0, target, num))
		return -1;
	// Get coordinates.
	var init_x = target->GetX();
	var init_y = target->GetY();
	var to_container = EffectVar(1, target, num);
	if (to_container)
	{
		var to_x = to_container->GetX();
		var to_y = to_container->GetY();
	}
	else
	{
		var to_x = EffectVar(2, target, num);
		var to_y = EffectVar(3, target, num);
	}
	// Move to the object with a weighed sin-wave centered around the halfway point.
	var length = Distance(init_x, init_y, to_x, to_y);
	// Remove effect if animation is done.
	if (2 * time > length)
		return -1;
	var angle = Angle(init_x, init_y, to_x, to_y);
	var dev, std_dev = length / 16;
	if (time < length / 4)
		dev = 4 * std_dev * time / length;
	else
		dev = 2 * std_dev - 4 * std_dev * time / length;
	var x = Sin(angle, 2 * time) + Cos(angle, Sin(20 * time, dev));
	var y = -Cos(angle, 2 * time) + Sin(angle, Sin(20 * time, dev));
	var color = RGB(128 + Cos(4 * time, 127), 128 + Cos(4 * time + 120, 127), 128 + Cos(4 * time + 240, 127));
	CreateParticle("PSpark", x, y, 0, 0, 32, color);
	return 1;
}

protected func FxRestoreStop(object target, int num, int reason, bool  temporary)
{
	var to_revive = EffectVar(0, target, num);
	var to_container = EffectVar(1, target, num);
	var to_x = EffectVar(2, target, num);
	var to_y = EffectVar(3, target, num);
	var ctrl_string = EffectVar(4, target, num);
	// Only if there is something to revive.
	if (to_revive)
	{			
		to_revive->Exit(); 
		if (to_container)
			to_revive->Enter(to_container);
		else
			to_revive->SetPosition(to_x, to_y);
		// Add new revive mode.
		if (ctrl_string)
		{
			var effect = AddEffect(ctrl_string, to_revive, 100, 35);
			EffectVar(0, to_revive, effect) = to_container;
			EffectVar(1, to_revive, effect) = to_x;
			EffectVar(2, to_revive, effect) = to_y;
		}
		else
			to_revive->AddRestoreMode(to_container, to_x, to_y);		
		// Add particle effect.
		for (var i = 0; i < 20; i++)
		{
			if (to_container)
			{
				to_x = to_container->GetX();
				to_y = to_container->GetY();			
			}
			var color = RGB(128 + Cos(18 * i, 127), 128 + Cos(18 * i + 120, 127), 128 + Cos(18 * i + 240, 127));
			CreateParticle("PSpark", to_x - GetX(), to_y - GetY(), RandomX(-10, 10), RandomX(-10, 10), 32, color);			
		}
		// Sound.
		//TODO new sound.
	}
	// Remove revival object.
	if (target)
		target->RemoveObject();
	return 1;
}

/*-- Global restoration on destruction --*/

// Adds an effect to restore an item on destruction.
global func AddRestoreMode(object to_container, int to_x, int to_y)
{
	if (!this)
		return;
	var effect = AddEffect("RestoreMode", this, 100);	
	EffectVar(0, this, effect) = to_container;
	EffectVar(1, this, effect) = to_x;
	EffectVar(2, this, effect) = to_y;
	return;
}	

// Destruction check, uses Fx*Stop to detect item removal.
// Effectvar 0: Container to which must be revived.
// Effectvar 1: x-coordinate to which must be revived.
// Effectvar 2: y-coordinate to which must be revived.
global func FxRestoreModeStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var reviver = CreateObject(ItemRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		reviver->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		var revived = CreateObject(target->GetID(), 0, 0, target->GetOwner());
		reviver->SetRestoreObject(revived, to_container, to_x, to_y);	
	}
	return 1;
}

func Definition(def)
{
	SetProperty("Name", "$Name$", def);
}
