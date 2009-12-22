/*
	Standard clonk controls
	Author: Newton
	
	This object provides handling of the clonk controls including item
	management and standard throwing behaviour. It should be included
	into any clonk definition.
	The controls in System.c4g/PlayerControl.c only provide basic movement
	handling, namely the movement left, right, up and down. The rest is
	handled here:
	Grabbing, ungrabbing, shifting and pushing vehicles into buildings;
	entering and exiting buildings; throwing, dropping; inventory shifting,
	hotkey controls	and it's callbacks and forwards to script.
	
	Objects that inherit this object need to return _inherited() in the
	following callbacks (if defined):
		Construction, Collection2, Ejection, RejectCollect, Departure,
		Entrance, AttachTargetLost, GrabLost, CrewSelection
 
	The following callbacks are made to other objects:
		*Stop
		*Left, *Right, *Up, *Down
		*Use, *UseStop, *UseHolding
	wheras * is 'Contained' if the clonk is contained and otherwise (riding,
	pushing, to self) it is 'Control'. 	The item in the inventory only gets
	the Use*-calls. If the callback is handled, you should return true.
	
	The inventory management:
	The objects in the inventory are saved (parallel to Contents()) in the
	array 'inventory' while the variable 'selected' is the index of the
	currently selected item. The currently selected item is thus retrieved by
	'inventory[selected]' or better by using the public function
	GetSelectedItem();
	Other functions are MaxContentsCount() (defines the maximum number of
	contents) and SelectItem(object or index) (selects inventory by index
	or directly)
*/

local selected, selected2;
local using, using2;
local inventory;
local mlastx, mlasty;

/* ++++++++ Item controls ++++++++++ */

/* Item limit */

public func MaxContentsCount() { return 3; }

/* Item select access*/

public func SelectItem(selection, bool second)
{
	var oldnum = GetSelected(second);
	var item = inventory[oldnum];
	
	// selection didnt change
	if (oldnum == selection) return;
	
	// set new selected
	if (!second) selected = selection;
	else selected2 = selection;
	
	// cancel the use of another item
	if (item)
	{
		if (!second) if (using == item) { item->~ControlUseStop(this,mlastx,mlasty); using = nil; }
		if (second) if (using2 == item) { item->~ControlUseStop(this,mlastx,mlasty); using2 = nil; }
	}
	
	// de-select previous (if any)
	if (item) item->~Deselection(this);
	
	// select new (if any)
	if (!second) item = inventory[selected];
	else item = inventory[selected2];
	
	// DEBUG
	//var bla = "nothing";
	//if (item) bla = item->~GetName();
	//Message("selected %s (position %d)", this, bla, selected);
	
	if (item)
		if (!item->~Selection(this))
			Sound("Grab");
			
	// callback to clonk (self):
	// OnSelectionChanged(oldslotnumber, newslotnumber)
	this->~OnSelectionChanged(oldnum, GetSelected(second));
	
	// the first and secondary selection may not be on the same spot
	if (selected2 == selected)
	{
		if(second) SelectItem(oldnum,false);
		else SelectItem(oldnum,true);
	}
}

public func ShiftItem(int dir, bool second)
{
	var sel = selected;
	if (second) sel = selected2;

	if (dir < 0)
	{
		sel--;
		if (sel <= 0) sel = MaxContentsCount()-1;
		
		SelectItem(sel,second);
	}
	else if (dir > 0)
	{
		SelectItem((sel+1) % MaxContentsCount(),second);
	}
}

public func GetSelected(bool second)
{
	if (!second) return selected;
	return selected2;
}

public func GetSelectedItem(bool second)
{
	if (!second) return inventory[selected];
	return inventory[selected2];
}

public func GetItem(int i)
{
	return inventory[i];
}

// disable ShiftContents for objects with ClonkControl.c4d

global func ShiftContents()
{
	if (this)
		if (this->~GetSelected() != nil)
			return false;
	return _inherited(...);
}

/* ################################################# */

protected func Construction()
{
	selected = 0;
	selected2 = 2;
	inventory = CreateArray();
	return _inherited(...);
}

protected func Collection2(object obj)
{
	var sel;

	// into selected area if empty
	if (!inventory[selected])
	{
		inventory[selected] = obj;
		sel = selected;
	}
	// otherwise, next if empty
	else
	{
		for(var i = 1; i < MaxContentsCount(); ++i)
		{
			sel = (selected+i) % MaxContentsCount();
			if (!inventory[sel])
			{
				inventory[sel] = obj;
				break;
			}
		}
	}
	this->~OnSlotFull(sel);

	return _inherited(obj,...);
}

protected func Ejection(object obj)
{
	var i;
	// find obj in array and delete
	for(i = 0; i < MaxContentsCount(); ++i)
	{
		if (inventory[i] == obj)
			{ inventory[i] = nil; break; }
	}
	
	this->~OnSlotEmpty(i);
	
	_inherited(obj,...);
}

protected func RejectCollect(id objid, object obj)
{
	// check max contents
	if (ContentsCount() >= MaxContentsCount()) return true;
	return _inherited(objid,obj,...);
}

/* ################################################# */

private func CancelUse()
{
	if (!using) return;
	var control = "Control";
	if (Contained() == using) control = "Contained";
	using->~Call(Format("~%sUseStop",control),this,mlastx,mlasty);
	using = nil;
}

// The using-command hast to be canceled if the clonk is entered into
// or exited from a building.

protected func Entrance()         { CancelUse(); return _inherited(...); }
protected func Departure()        { CancelUse(); return _inherited(...); }

// The same for vehicles
protected func AttachTargetLost() { CancelUse(); return _inherited(...); }
protected func GrabLost()         { CancelUse(); return _inherited(...); }
// TODO: what is missing here is a callback for when the clonk STARTs a attach or push
// action.
// So if a clonk e.g. uses a tool and still while using it (holding down the mouse button)
// hits SPACE (grab vehicle), ControlUseStop is not called to the tool. 
// the workaround for now is that the controls do not allow to grab a vehicle while still
// holding down the mouse button. But this does not cover the (seldom?) case that the clonk
// is put into a grabbing/attached action via Script.


// ...aaand the same for when the clonk is deselected
protected func CrewSelection(bool unselect)
{
	if (unselect) CancelUse();
	return _inherited(unselect,...);
}

/* Main control function */
public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) return false;
	
	
	//Log(Format("%d, %d, %d, %v, %v",  x,y,ctrl, repeat, release));
	
	// Any control resets a previously given command
	SetCommand("None");

	// hotkeys (inventory, vehicle and structure control)
	var hot = 0;
	if (ctrl == CON_Hotkey0) hot = 10;
	if (ctrl == CON_Hotkey1) hot = 1;
	if (ctrl == CON_Hotkey2) hot = 2;
	if (ctrl == CON_Hotkey3) hot = 3;
	if (ctrl == CON_Hotkey4) hot = 4;
	if (ctrl == CON_Hotkey5) hot = 5;
	if (ctrl == CON_Hotkey6) hot = 6;
	if (ctrl == CON_Hotkey7) hot = 7;
	if (ctrl == CON_Hotkey8) hot = 8;
	if (ctrl == CON_Hotkey9) hot = 9;
	
	if (hot > 0) return this->~ControlHotkey(hot-1);
	
	var proc = GetProcedure();

	// building, vehicle, contents control
	var house = Contained();
	var vehicle = GetActionTarget();
	var contents = GetSelectedItem();
	var contents2 = GetSelectedItem(true);

	if (house)
	{
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Contained", house))
			return true;
	}
	else if (vehicle && (proc == "PUSH" || proc == "ATTACH"))
	{
		// control to grabbed vehicle or riding etc.
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", vehicle))
			return true;
	}
	// out of convencience we call Control2Script, even though it handles
	// left, right, up and down, too. We don't want that, so this is why we
	// check that ctrl is Use.
	else if (contents && ctrl == CON_Use)
	{
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", contents))
			return true;
	}
	else if (contents2 && ctrl == CON_UseAlt)
	{
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", contents2))
			return true;
	}
	/*
	if (!vehicle && !house)
	{
		if (ctrl == CON_Jump) if (this->~ControlJump(this)) return true;
	}*/
	
	// everything down from here:
	// standard controls that are called if not overloaded via script
	
	// Movement controls
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
		return ObjectControlMovement(plr, ctrl, strength, release);

	// Push controls
	if (ctrl == CON_Grab || ctrl == CON_Ungrab || ctrl == CON_PushEnter || ctrl == CON_GrabPrevious || ctrl == CON_GrabNext)
		return ObjectControlPush(plr, ctrl);

	// Entrance controls
	if (ctrl == CON_Enter || ctrl == CON_Exit)
		return ObjectControlEntrance(plr,ctrl);

	// Inventory control
	var inv_control = true;
	if (ctrl == CON_NextItem)              { ShiftItem(+1); }
	else if (ctrl == CON_PreviousItem)     { ShiftItem(-1); }
	else if (ctrl == CON_NextAltItem)      { ShiftItem(+1,true); }
	else if (ctrl == CON_PreviousAltItem)  { ShiftItem(-1,true); }
	else { inv_control = false; }
	
	if (inv_control) return true;

	// only if not in house, not grabbing a vehicle and an item selected
	if (!house && (!vehicle || (proc != "PUSH" && proc != "ATTACH")))
	{
		if (contents)
		{
			// throw
			if (ctrl == CON_Throw)
			{
			    if (proc == "SCALE" || proc == "HANGLE")
			      return PlayerObjectCommand(plr, false, "Drop", contents);
			    else
			      return PlayerObjectCommand(plr, false, "Throw", contents, x, y);
			}
			// drop
			if (ctrl == CON_Drop)
			{
				return PlayerObjectCommand(plr, false, "Drop", contents);
			}
		}
		// same for contents2 (copypasta)
		if (contents2)
		{
			// throw
			if (ctrl == CON_ThrowAlt)
			{
			    if (proc == "SCALE" || proc == "HANGLE")
			      return PlayerObjectCommand(plr, false, "Drop", contents2);
			    else
			      return PlayerObjectCommand(plr, false, "Throw", contents2, x, y);
			}
			// drop
			if (ctrl == CON_DropAlt)
			{
				return PlayerObjectCommand(plr, false, "Drop", contents2);
			}
		}
	}
	
	// Unhandled control
	return false;
}

public func ObjectCommand(string command, object target, int tx, int ty, object target2)
{
	// special control for throw and jump
	// but only with controls, not with general commands
	if (command == "Throw") ControlThrow(target,tx,ty);
	else if (command == "Jump") ControlJump();
	// else standard command
	else SetCommand(command,target,tx,ty,target2);
}

// Control redirected to script
private func Control2Script(int ctrl, int x, int y, int strength, bool repeat, bool release, string control, object obj)
{
	// for the use command
	if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		var handled = false;
		
		var estr = "";
		// not an inventory object
		if(ctrl == CON_UseAlt)
			if(!(obj->Contained()))
				estr = "Alt";
		
		if (!release && !repeat)
		{
			handled = obj->Call(Format("~%sUse%s",control,estr),this,x,y);
			if (ctrl == CON_Use) using = obj;
			else using2 = obj;
		}
		else if (release && (using == obj || using2 == obj))
		{
			handled = obj->Call(Format("~%sUse%sStop",control,estr),this,x,y);
			if (ctrl == CON_Use) using = nil;
			else using2 = nil;
		}
		else if (using == obj || using2 == obj)
		{
			handled = obj->Call(Format("~%sUse%sHolding",control,estr),this,x,y);
			// if that function returns -1, the control is stopped (*UseStop)
			// and no more *UseHolding-calls are made.
			if (handled == -1)
			{
				obj->Call(Format("~%sUse%sStop",control,estr),this,x,y);
				if (ctrl == CON_Use) using = nil;
				else using2 = nil;
				handled = true;
			}
		}
		
		mlastx = x;
		mlasty = y;
		
		return handled;
	}
	// overloads of movement commandos
	else if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Down || ctrl == CON_Up)
	{
		if (release)
		{
			// if any movement key has been released, ControlStop is called
			if (obj->Call(Format("~%sStop",control),this))  return true;
		}
		else
		{
			// Control*
			if (ctrl == CON_Left)  if (obj->Call(Format("~%sLeft",control),this))  return true;
			if (ctrl == CON_Right) if (obj->Call(Format("~%sRight",control),this)) return true;
			if (ctrl == CON_Up)    if (obj->Call(Format("~%sUp",control),this))    return true;
			if (ctrl == CON_Down)  if (obj->Call(Format("~%sDown",control),this))  return true;
		}
	}
	
	return false;
}

// returns true if the clonk is able to enter a building (actionwise)
public func CanEnter()
{
	var proc = GetProcedure();
	if (proc != "WALK" && proc != "SWIM" && proc != "SCALE" &&
		proc != "HANGLE" && proc != "FLOAT" && proc != "FLIGHT" &&
		proc != "PUSH") return false;
	return true;
}

// Handles enter and exit
private func ObjectControlEntrance(int plr, int ctrl)
{
	var proc = GetProcedure();

	// enter
	if (ctrl == CON_Enter)
	{
		// contained
		if (Contained()) return false;
		// enter only if... one can
		if (!CanEnter()) return false;

		// a building with an entrance at right position is there?
		var obj = GetEntranceObject();
		if (!obj) return false;
		
		PlayerObjectCommand(plr, false, "Enter", obj);
		return true;
	}
	
	// exit
	if (ctrl == CON_Exit)
	{
		if (!Contained()) return false;
		
		PlayerObjectCommand(plr, false, "Exit");
		return true;
	}
	
	return false;
}

// Handles push controls
private func ObjectControlPush(int plr, int ctrl)
{
	if (!this) return false;
	
	var proc = GetProcedure();
	
	// grabbing
	if (ctrl == CON_Grab)
	{
		// grab only if he walks
		if (proc != "WALK") return false;
		
		// disallow if the clonk is still using something
		if (using) return false;

		// only if there is someting to grab
		var obj = FindObject(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this));
		if (!obj) return false;

		// grab
		PlayerObjectCommand(plr, false, "Grab", obj);
		return true;
	}
	
	// grab next/previous
	if (ctrl == CON_GrabNext)
		return ShiftVehicle(plr, false);
	if (ctrl == CON_GrabPrevious)
		return ShiftVehicle(plr, true);
	
	// ungrabbing
	if (ctrl == CON_Ungrab)
	{
		// ungrab only if he pushes
		if (proc != "PUSH") return false;

		PlayerObjectCommand(plr, false, "UnGrab");
		return true;
	}
	
	// push into building
	if (ctrl == CON_PushEnter)
	{
		if (proc != "PUSH") return false;
		
		// respect no push enter
		if (GetActionTarget()->GetDefCoreVal("NoPushEnter","DefCore")) return false;
		
		// a building with an entrance at right position is there?
		var obj = GetActionTarget()->GetEntranceObject();
		if (!obj) return false;

		PlayerObjectCommand(plr, false, "PushTo", GetActionTarget(), 0, 0, obj);
		return true;
	}
	
}

// grabs the next/previous vehicle (if there is any)
private func ShiftVehicle(int plr, bool back)
{
	if (!this) return false;
	
	if (GetProcedure() != "PUSH") return false;

	var lorry = GetActionTarget();
	// get all grabbable objects
	var objs = FindObjects(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this));
		
	// nothing to switch to (there is no other grabbable object)
	if (GetLength(objs) <= 1) return false;
		
	// find out at what index of the array objs the vehicle is located
	var index = 0;
	for(var obj in objs)
	{
		if (obj == lorry) break;
		index++;
	}
		
	// get the next/previous vehicle
	if (back)
	{
		--index;
		if (index < 0) index = GetLength(objs)-1;
	}
	else
	{
		++index;
		if (index >= GetLength(objs)) index = 0;
	}
	
	PlayerObjectCommand(plr, false, "Grab", objs[index]);
	
	return true;
} 

// Throwing
private func DoThrow(object obj, int angle)
{
  // parameters...
  var iX, iY, iR, iXDir, iYDir, iRDir;
  iX = 8; if (!GetDir()) iX = -iX;
  iY = Cos(angle,-8);
  iR = Random(360);
  iRDir = RandomX(-10,10);

  var speed = GetPhysical("Throw");

  iXDir = speed * Sin(angle,1000) / 17000;
  iYDir = speed * Cos(angle,-1000) / 17000;
  // throw boost (throws stronger upwards than downwards)
  if (iYDir < 0) iYDir = iYDir * 13/10;
  if (iYDir > 0) iYDir = iYDir * 8/10;
  
  // is riding? add it's velocity
  var proc = GetProcedure();
  if (GetActionTarget() && (proc == "PUSH" || proc == "ATTACH"))
  {
    iXDir += GetActionTarget()->GetXDir() / 10;
    iYDir += GetActionTarget()->GetYDir() / 10;
  }
  // throw
  obj->Exit(iX, iY, iR, 0, 0, iRDir);  
  obj->SetXDir(iXDir,1000);
  obj->SetYDir(iYDir,1000);
  
  return true;
}

// custom throw
public func ControlThrow(object target, int x, int y)
{
	// standard throw after all
	if (!x && !y) return false;
	if (!target) return false;
	
	var throwAngle = Angle(0,0,x,y);
	
	// walking (later with animation: flight, scale, hangle?)
	if (GetProcedure() == "WALK")
	{
		if (throwAngle < 180) SetDir(DIR_Right);
		else SetDir(DIR_Left);
		SetAction("Throw");
		return DoThrow(target,throwAngle);
	}
	// riding
	if (GetAction() == "Ride" || GetAction() == "RideStill")
	{
		SetAction("RideThrow");
		return DoThrow(target,throwAngle);
	}
	return false;
}

public func ControlJump()
{
	var ydir = 0;
	
	if(GetProcedure() == "WALK")
	{
		ydir = GetPhysical("Jump")*GetCon()/1000/100;
	}
	else if(InLiquid())
	{
		if(!GBackSemiSolid(0,-1))
			ydir = BoundBy(GetPhysical("Swim")/2500,24,38)*GetCon()/100;
	}		
	
	if(ydir)
	{
		SetPosition(GetX(),GetY()-1);
		SetAction("Jump");
		SetSpeed(GetXDir(),-ydir);
		
		var iX=GetX(),iY=GetY(),iXDir=GetXDir(),iYDir=GetYDir();
		if(SimFlight(iX,iY,iXDir,iYDir,25))
			if(GBackLiquid(iX-GetX(),iY-GetY()) && GBackLiquid(iX-GetX(),iY+GetDefHeight()/2-GetY()))
				SetAction("Dive");
				
		return true;
	}
	return false;
}
