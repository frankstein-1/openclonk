/*--
		Objects.c
		Authors: Maikel, boni, Ringwaul, Sven2, flgr, Clonkonaut, G�nther

		Functions generally applicable to objects; not enough to be worth distinct scripts though.
--*/

// Does not set the speed of an object. But you can set two components of the velocity vector with this function.
global func SetSpeed(int x_dir, int y_dir, int prec)
{
	SetXDir(x_dir, prec);
	SetYDir(y_dir, prec);
	return;
}

// Sets an objects's speed and its direction, doesn't it?
// Can set either speed or angle of velocity, or both
global func SetVelocity(int angle, int speed, int precAng, int precSpd)
{
	if(!speed)
		speed = Distance(0,0, GetXDir(precSpd), GetYDir(precSpd));
	if(!angle)
		angle = Angle(0,0, GetXDir(precSpd), GetYDir(precSpd), precAng);
	if(!precAng) precAng = 1;
	var x_dir = Sin(angle, speed, precAng);
	var y_dir = -Cos(angle, speed, precAng);

	SetXDir(x_dir, precSpd);
	SetYDir(y_dir, precSpd);
	return;
}

// Sets the completion of this to new_con.
global func SetCon(int new_con)
{
	return DoCon(new_con - GetCon());
}

// Sets the object's transparency.
global func SetObjAlpha(int by_alpha)
{
	var clr_mod = GetClrModulation();
	
	if (!clr_mod)
		clr_mod = by_alpha << 24;
	else
		clr_mod = clr_mod & 16777215 | by_alpha << 24;
	return SetClrModulation(clr_mod);
}

// Makes the calling object invincible.
global func MakeInvincible()
{
	if (!this)
		return;
	return AddEffect("IntInvincible", this, 300);
}

global func FxIntInvincibleDamage()
{
	// Object receives zero damage.
	return 0;
}

// Move an object by the given parameters relative to its position.
global func MovePosition(int x, int y, int prec)
{
	SetPosition(GetX(prec) + x, GetY(prec) + y, nil, prec);
}

// Speed the calling object into the given direction (angle)
global func LaunchProjectile(int angle, int dist, int speed, int x, int y, bool rel_x)
{
	// dist: Distance object travels on angle. Offset from calling object.
	// x: X offset from container's center
	// y: Y offset from container's center
	// rel_x: if true, makes the X offset relative to container direction. (x=+30 will become x=-30 when Clonk turns left. This way offset always stays in front of a Clonk.)

	var x_offset = Sin(angle, dist);
	var y_offset = -Cos(angle, dist);

	if (Contained() != nil && rel_x == true)
		if (Contained()->GetDir() == 0)
			x = -x;

	if (Contained() != nil)
	{
		Exit(x_offset + x, y_offset + y, angle);
		SetVelocity(angle, speed);
		return true;
	}

	if (Contained() == nil)
	{
		SetPosition(GetX() + x_offset + x, GetY() + y_offset + y);
		SetR(angle);
		SetVelocity(angle, speed);
		return true;
	}
	return false;
}

// Sets the MaxEnergy value of an object and does the necessary callbacks.
global func SetMaxEnergy(int value)
{
	if (!this)
		return;
	value *= 1000;
	var old_maxenergy = this.MaxEnergy;
	this.MaxEnergy = value;
	// Change current energy percentage wise and implicit callback.
	DoEnergy(GetEnergy() * (value - old_maxenergy) / old_maxenergy);
	return;
}

// Returns the MaxEnergy value of an object.
global func GetMaxEnergy()
{
	if (!this)
		return;
	return this.MaxEnergy / 1000;
}

// Sets the MaxBreath value of an object and does the necessary callbacks.
global func SetMaxBreath(int value)
{
	if (!this)
		return;
	var old_maxbreath = this.MaxBreath;
	this.MaxBreath = value;
	// Change current breath percentage wise and implicit callback.
	DoBreath(GetBreath() * (value - old_maxbreath) / old_maxbreath);
	return;
}

// Returns the MaxBreath value of an object.
global func GetMaxBreath()
{
	if (!this)
		return;
	return this.MaxBreath;
}

// Makes an object gain Con until it is FullCon
global func StartGrowth(int value)
{
	return AddEffect("IntGrowth", this, 1, 35, nil, nil, value);
}

global func StopGrowth()
{
	return RemoveEffect("IntGrowth", this);
}

global func GetGrowthValue()
{
	var e = GetEffect("IntGrowth", this);
	if(!e) return 0;
	return e.growth;
}

global func FxIntGrowthStart(object obj, effect, int temporary, int value)
{
	if (!temporary) effect.growth = value;
}

global func FxIntGrowthTimer(object obj, effect)
{
	if (obj->OnFire()) return;
	obj->DoCon(effect.growth, 1000);
	var done = obj->GetCon(1000) >= 1000;
	return -done;
}

// Plays hit sounds for an average object made of stone or stone-like material.
// x and y need to be the parameters passed to Hit() from the engine.
global func StonyObjectHit(int x, int y)
{
	// Failsafe
	if (!this) return false;
	var xdir = GetXDir(), ydir = GetYDir();
	if(x) x = x / Abs(x);
	if(y) y = y / Abs(y);
	// Check for solid in hit direction
	var i = 0;
	var average_obj_size = Distance(0,0, GetObjWidth(), GetObjHeight()) / 2 + 2;
	while(!GBackSolid(x*i, y*i) && i < average_obj_size) i++;
	// To catch some high speed cases: if no solid found, check directly beneath
	if (!GBackSolid(x*i, y*i))
		while(!GBackSolid(x*i, y*i) && i < average_obj_size) i++;
	// Check if digfree
	if (!GetMaterialVal("DigFree", "Material", GetMaterial(x*i, y*i)) && GBackSolid(x*i, y*i))
		return Sound("RockHit?");
	// Else play standard sound
	if (Distance(0,0,xdir,ydir) > 10)
			return Sound("SoftTouch?");
		else
			return Sound("SoftHit?");
}

// Removes all objects of the given type.
global func RemoveAll(p)
{
	var cnt;
	if (GetType(p) == C4V_PropList) p = Find_ID(p); // RemoveAll(ID) shortcut
	for (var obj in FindObjects(p, ...))
	{
		if (obj)
		{
			obj->RemoveObject();
			cnt++;
		}
	}
	return cnt;
}

// Splits the calling object into its components.
global func Split2Components()
{
	if (!this)
		return false;
	var ctr = Contained();
	// Transfer all contents to container.
	while (Contents())
		if (!ctr || !Contents()->Enter(ctr))
			Contents()->Exit();
	// Split components.
	for (var i = 0, compid; compid = GetComponent(nil, i); ++i)
		for (var j = 0; j < GetComponent(compid); ++j)
		{
			var comp = CreateObject(compid, nil, nil, GetOwner());
			if (OnFire()) comp->Incinerate();
			if (!ctr || !comp->Enter(ctr))
			{
				comp->SetR(Random(360));
				comp->SetXDir(Random(3) - 1);
				comp->SetYDir(Random(3) - 1);
				comp->SetRDir(Random(3) - 1);
			}
		}
	RemoveObject();
	return;
}