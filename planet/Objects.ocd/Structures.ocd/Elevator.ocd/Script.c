/*-- Elevator --*/

#include Library_Structure
#include Library_Ownable

// used in the elevator case
static const Elevator_needed_power = 20;

local case, rope;
local partner, slave;

/* Editing helpers */

// Frees a rectangle for the case
public func CreateShaft(int length)
{
	// Move the case out of the way
	case->SetPosition(case->GetX(), GetY()-10);
	ClearFreeRect(GetX() + 7 - 38*GetDir(), GetY() + 20, 24, length + 13);
	// Move the case back
	case->SetPosition(case->GetX(), GetY()+20);
}

// Move case to specified absolute y position
public func SetCasePosition(int y)
{
	if (case) return case->SetPosition(case->GetX(), y);
	return false;
}

// Overloaded to reposition the case
public func SetDir(new_dir, ...)
{
	var r = inherited(new_dir, ...);
	// Update position of child objects on direction change
	if (case) case->SetPosition(GetX() -19 * GetCalcDir(), case->GetY());
	if (rope) rope->SetPosition(GetX() -19 * GetCalcDir(), rope->GetY());
	return r;
}

// Forward config to case
public func SetNoPowerNeed(bool to_val)
{
	if (case) return case->SetNoPowerNeed(to_val);
	return false;
}

private func EditCursorMoved()
{
	// Move case and rope along with elevator in editor mode
	if (case) case->SetPosition(GetX() + GetCaseXOff(), case->GetY());
	if (rope) rope->SetPosition(GetX() + GetCaseXOff(), GetY() - 13);
	return true;
}

// return default horizontal offset of case/rope to elevator
public func GetCaseXOff() { return -19 * GetCalcDir(); }

/* Initialization */

private func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(-44,0,1,0));
	SetAction("Default");
	wheel_anim = PlayAnimation("winchSpin", 1, Anim_Const(0), Anim_Const(1000));

	return _inherited(...);
}

private func Initialize()
{
	SetCategory(C4D_StaticBack);
	CreateCase();
	CreateRope();

	if (partner)
	{
		if (Inside(partner->GetY(), GetY()-3, GetY()+3))
		{
			partner->LetsBecomeFriends(this);
			SetPosition(GetX(), partner->GetY());
		}
		else
			partner = nil;
	}
	return _inherited();
}

private func CreateCase()
{
	case = CreateObjectAbove(ElevatorCase, GetCaseXOff(), 33, GetOwner());
	if (case) case->Connect(this);
}

private func CreateRope()
{
	rope = CreateObjectAbove(ElevatorRope, GetCaseXOff(), -11, GetOwner());
	if (rope) rope->SetAction("Be", case.back);
}

/* Destruction */

private func Destruction()
{
	if(rope) rope->RemoveObject();
	if(case) case->LostElevator();
	if (partner) partner->LoseCombination();
}

public func LostCase()
{
	if(partner) partner->LoseCombination();
	if(rope) rope->RemoveObject();

	StopEngine();

	// for now: the elevator dies, too
	Incinerate();
}

/* Effects */

local wheel_anim, case_speed;

public func StartEngine(int direction, bool silent)
{
	if (!case) return;

	if (!silent)
	{
		Sound("ElevatorStart", nil, nil, nil, nil, 400);
		ScheduleCall(this, "EngineLoop", 34);
	}
	if (wheel_anim == nil) // If for some reason the animation has stopped
		wheel_anim = PlayAnimation("winchSpin", 1, Anim_Const(0), Anim_Const(1000));

	var begin, end;
	if (direction == COMD_Up) // Either that or COMD_Down
	{
		begin = GetAnimationLength("winchSpin");
		end = 0;
	}
	else
	{
		begin = 0;
		end = GetAnimationLength("winchSpin");
	}

	case_speed = Abs(case->GetYDir());
	var speed = 80 - case_speed * 2;
	SetAnimationPosition(wheel_anim, Anim_Linear(GetAnimationPosition(wheel_anim), begin, end, speed, ANIM_Loop));
}

public func EngineLoop()
{
	Sound("ElevatorMoving", nil, nil, nil, 1, 400);
}

public func StopEngine(bool silent)
{
	if (!silent)
	{
		Sound("ElevatorMoving", nil, nil, nil, -1);
		ClearScheduleCall(this, "EngineLoop");
		Sound("ElevatorStop", nil, nil, nil, nil, 400);
	}

	if (wheel_anim == nil) return;

	case_speed = nil;
	SetAnimationPosition(wheel_anim, Anim_Const(GetAnimationPosition(wheel_anim)));
}

// Adjusting the turning speed of the wheel to the case's speed
private func UpdateTurnSpeed()
{
	if (!case) return;
	if (case_speed == nil || wheel_anim == nil) return;

	if (Abs(case->GetYDir()) != case_speed)
	{
		var begin, end;
		if (case->GetYDir() < 0) // Either that or COMD_Down
		{
			begin = GetAnimationLength("winchSpin");
			end = 0;
		}
		else
		{
			begin = 0;
			end = GetAnimationLength("winchSpin");
		}
		case_speed = Abs(case->GetYDir());
		var speed = 80 - case_speed * 2;
		SetAnimationPosition(wheel_anim, Anim_Linear(GetAnimationPosition(wheel_anim), begin, end, speed, ANIM_Loop));
	}
}

/* Construction preview */

// Sticking to other elevators
public func ConstructionCombineWith() { return "IsElevator"; }
public func ConstructionCombineDirection() { return CONSTRUCTION_STICK_Left | CONSTRUCTION_STICK_Right; }

// Called to determine if sticking is possible
public func IsElevator(object previewer)
{
	if (!previewer) return true;

	if (GetDir() == DIR_Left)
	{
		if (previewer.direction == DIR_Right && previewer->GetX() > GetX()) return true;
	}
	else
	{
		if (previewer.direction == DIR_Left && previewer->GetX() < GetX()) return true;
	}
	return false;
}

// Called when the elevator construction site is created
public func CombineWith(object other)
{
	// Save for use in Initialize
	partner = other;
}

/* Combination */

// Called by a new elevator next to this one
// The other elevator will be the slave
public func LetsBecomeFriends(object other)
{
	partner = other;
	other.slave = true; // Note: This is liberal slavery
	if (case) case->StartConnection(other.case);
}

// Partner was destroyed or moved
public func LoseCombination()
{
	partner = nil;
	slave = false;
	if (case) case->LoseConnection();
}

// Called by our case because the case has a timer anyway
public func CheckSlavery()
{
	// Check if somehow we moved away from our fellow
	if (ObjectDistance(partner) > 62 || !Inside(partner->GetY(), GetY()-1, GetY()+1))
	{
		LoseCombination();
		partner->LoseCombination();
	}
}

/* Scenario saving */

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Category");
	if (partner && slave)
	{
		props->AddCall("Friends", partner, "LetsBecomeFriends", this);
	}
	if (case && case->GetY() > GetY() + 20)
	{
		props->AddCall("Shaft", this, "CreateShaft", case->GetY() - GetY() - 20);
		props->AddCall("Shaft", this, "SetCasePosition", case->GetY());
	}
	return true;
}

local ActMap = {
		Default = {
			Prototype = Action,
			Name = "Default",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 3,
			FacetBase = 1,
			NextAction = "Default",
			EndCall = "UpdateTurnSpeed",
		},
};

private func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-20,1,0), Trans_Rotate(-20, 0, 1, 0)));
}
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 100;
local HitPoints = 70;
local Plane = 249;