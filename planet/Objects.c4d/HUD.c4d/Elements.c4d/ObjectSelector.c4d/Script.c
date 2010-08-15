/*
	Object selector HUD
	Author: Newton
	
	For each inventory item (and each vehicle, house etc on the same pos)
	one of this objects exists in the bottom bar. If clicked, the associated
	object is selected (inventory item is selected, vehicle is grabbed
	or ungrabbed, house is entered or exited...).
	
	HUD elements are passive, they don't update their status by themselves
	but rely on the clonk to update their status.

	This object works only for crew members that included the standard clonk
	controls (see Libraries.c4d/ClonkControl.c4d)
	
*/

/*
	usage of layers:
	-----------------
	layer 0 - unused
	layer 1 - title
	layer 2 - actionicon
	layer 3,4 - hands
	
	layer 12 - hotkey

*/

local selected, crew, hotkey, myobject, actiontype, subselector, position;

static const ACTIONTYPE_INVENTORY = 0;
static const ACTIONTYPE_VEHICLE = 1;
static const ACTIONTYPE_STRUCTURE = 2;
static const ACTIONTYPE_SCRIPT = 3;

private func HandSize() { return 400; }
private func IconSize() { return 500; }

protected func Construction()
{
	_inherited();
	
	selected = 0;
	hotkey = 0;
	position = 0;
	myobject = nil;
	subselector = nil;
	
	// parallaxity
	this["Parallaxity"] = [0,0];
	
	// visibility
	this["Visibility"] = VIS_None;
}

protected func Destruction()
{
	if(subselector)
		subselector->RemoveObject();
}

public func MouseSelectionAlt(int plr)
{
	if(!myobject) return;
	
	// close other messages...
	crew->OnDisplayInfoMessage();
	
	var msg = Format("<c ff0000>%s|%s</c>",myobject->GetName(),myobject->GetDesc());
	CustomMessage(msg,this,plr);
	return true;
}

public func MouseSelection(int plr)
{
	if(!crew) return false;
	if(plr != GetOwner()) return false;

	// no object
	if(!myobject) return false;
	
	// object is a pushable vehicle
	if(actiontype == ACTIONTYPE_VEHICLE)
	{
		var proc = crew->GetProcedure();
		
		// object is inside building -> activate
		if(crew->Contained() && myobject->Contained() == crew->Contained())
		{
			crew->SetCommand("Activate",myobject);
			return true;
		}
		// crew is currently pushing vehicle
		else if(proc == "PUSH")
		{
			// which is mine -> let go
			if(crew->GetActionTarget() == myobject)
				crew->ObjectCommand("UnGrab");
			else
				crew->ObjectCommand("Grab", myobject);
				
			return true;
		}
		// grab
		else if(proc == "WALK")
		{
			crew->ObjectCommand("Grab", myobject);
			return true;
		}
	}
	
	// object is a building
	if(actiontype == ACTIONTYPE_STRUCTURE)
	{
		// inside? -> exit
		if(crew->Contained() == myobject)
		{
			crew->ObjectCommand("Exit");
			return true;
		}
		// outside? -> enter
		else if(crew->CanEnter())
		{
			crew->ObjectCommand("Enter", myobject);
			return true;
		}
	}
	if(actiontype == ACTIONTYPE_SCRIPT)
	{
		if(myobject->~IsInteractable(crew))
		{
			myobject->Interact(crew);
			return true;
		}	
	}
}

public func MouseDragDone(obj, object target)
{
	// not on landscape
	if(target) return;
	if(GetType(obj) != C4V_C4Object) return;
	if(!crew) return false;
	
	var container;
	if(container = obj->Contained())
	{
		if(obj->GetOCF() & OCF_Collectible)
		{
			container->SetCommand("Drop",obj);
		}
		else if(obj->GetOCF() & OCF_Grab)
		{
			if(crew->Contained() == container)
				crew->SetCommand("Activate",obj);
		}
	}
}

public func MouseDrag(int plr)
{
	if(plr != GetOwner()) return false;
	
	if(actiontype == ACTIONTYPE_INVENTORY || actiontype == ACTIONTYPE_VEHICLE)
		return myobject;
		
	return false;
}

public func MouseDrop(int plr, obj)
{
	if(plr != GetOwner()) return false;
	if(GetType(obj) != C4V_C4Object) return false;
	if(!crew) return false;

	// anything theoretically can be dropped on script objects
	if(actiontype == ACTIONTYPE_SCRIPT)
	{
		return myobject->~DropObject(obj);
	}
	// a collectible object
	else if(obj->GetOCF() & OCF_Collectible)
	{
		if(actiontype == ACTIONTYPE_INVENTORY)
		{
			var objcontainer = obj->Contained();
			
			// object container is the clonk too? Just switch
			if(objcontainer == crew)
			{
				crew->Switch2Items(position, crew->GetItemPos(obj));
				return true;
			}
		
			// slot is already full: switch places with other object
			if(myobject != nil)
			{
				var myoldobject = myobject;
				
				// 1. exit my object
				myoldobject->Exit();
				// 2. try to enter the other object
				if(crew->Collect(obj,false,position))
				{
					// 3. try to enter my object into other container
					if(!(objcontainer->Collect(myoldobject,true)))
						// -> otherwise, recollect my object
						crew->Collect(myoldobject,false,position);
				}
				// -> otherwise, recollect my object
				else
					crew->Collect(myoldobject,false,position);
			}
			// otherwise, just collect
			else
			{
				crew->Collect(obj,false,position);
			}
			return true;
		}
		else if(actiontype == ACTIONTYPE_VEHICLE)
		{
			if(!myobject) return false;
			// collect if possible
			if(myobject->Collect(obj)) return true;
			// otherwise (lorry is full?): fail
			return false;
		}
	}
	else if(obj->GetOCF() & OCF_Grab)
	{
		if(actiontype == ACTIONTYPE_STRUCTURE)
		{
			// respect no push enter
			if (obj->GetDefCoreVal("NoPushEnter","DefCore")) return false;
			// enter vehicle into structure
			crew->ObjectCommand("PushTo", obj, 0, 0, myobject);
			return true;
		}
	}
}



public func Clear()
{
	myobject = nil;
	actiontype = -1;
	hotkey = 0;
	position = 0;
	this["Visibility"] = VIS_None;
	if(subselector)
		subselector->RemoveObject();
}

public func ClearMessage()
{
	CustomMessage("",this,GetOwner());
	if(subselector)
		CustomMessage("",subselector,GetOwner());
}

public func SetObject(object obj, int type, int pos, int hot)
{
	this["Visibility"] = VIS_Owner;

	// remove effect that checks whether the object to which this selector
	// refers to is still existant because now this selector gets a new
	// object to which to refer.
	RemoveEffect("IntRemoveGuard",myobject);
	
	position = pos;
	actiontype = type;
	myobject = obj;
	hotkey = hot;
	
	if(!myobject) 
	{	
		SetGraphics(nil,nil,1);
		SetName(Format("$TxtSlot$",pos+1));
		this["MouseDragImage"] = nil;
		if(subselector)
			subselector->RemoveObject();
	}
	else
	{
		SetGraphics(nil,nil,1,GFXOV_MODE_ObjectPicture, 0, 0, myobject);
		SetName(Format("$TxtSelect$",myobject->GetName()));
		this["MouseDragImage"] = myobject;

		// if object has extra slot, show it
		if(myobject->~HasExtraSlot())
		{
			if(!subselector)
			{
				subselector = CreateObject(GUI_ExtraSlot,0,0,GetOwner());
				subselector->SetPosition(GetX()+16,GetY()+16);
			}
			subselector->SetContainer(myobject);
			subselector->SetCrew(crew);
		}
		// or otherwise, remove it
		else if(subselector)
		{
			subselector->RemoveObject();
		}

		// create an effect which monitors whether the object is removed:
		// this is only necessary for inventory as all the other slots
		// are checked via the effect
		if(actiontype == ACTIONTYPE_INVENTORY)
		{
			AddEffect("IntRemoveGuard",myobject,1,0,this);
		}
	}

	ShowHotkey();
	UpdateSelectionStatus();
}

public func FxIntRemoveGuardStop(object target, int num, int reason, bool temp)
{
	if(reason == 3)
		if(target == myobject)
			SetObject(nil,0,position);
}

public func SetCrew(object c)
{
	if(crew == c) return;
	
	crew = c;
	SetOwner(c->GetOwner());
	
	this["Visibility"] = VIS_Owner;
}

public func GetCrew() { return crew; }

public func ShowHotkey()
{
	if(hotkey > 10 || hotkey <= 0)
	{
		SetGraphics(nil,nil,12);
	}
	else
	{
		var num = hotkey;
		if(hotkey == 10) num = 0;
		var name = Format("%d",num);
		SetGraphics(name,Icon_Number,12,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(300,0,16000,0,300,-34000, 12);
		SetClrModulation(RGB(160,0,0),12);
	}
}

public func Selected()
{
	return selected;
}

public func UpdateSelectionStatus()
{
	if(!crew) return;
	
	// determine...
	var sel = 0;

	// script...
	if(actiontype == ACTIONTYPE_SCRIPT)
	{
		var metainfo = myobject->~GetInteractionMetaInfo(crew);
		if(metainfo)
		{
			SetGraphics(metainfo["IconName"],metainfo["IconID"],2,GFXOV_MODE_IngamePicture);
			SetObjDrawTransform(IconSize(),0,-16000,0,IconSize(),20000, 2);
			
			var desc = metainfo["Description"];
			if(desc) SetName(desc);
			
			if(metainfo["Selected"])
				SetObjDrawTransform(1200,0,0,0,1200,0,1);
		}

		return;
	}
	else if(actiontype == ACTIONTYPE_VEHICLE)
		if(crew->GetProcedure() == "PUSH" && crew->GetActionTarget() == myobject)
			sel = 1;	
	else if(actiontype == ACTIONTYPE_STRUCTURE)
		if(crew->Contained() == myobject)
			sel = 1;
	else if(actiontype == ACTIONTYPE_INVENTORY)
	{
		if(0 == position)
			sel += 1;
		if(1 == position)
			sel += 2;
	}
			
	selected = sel;

	// and set the icon...
	if(selected)
	{
		//SetClrModulation(RGB(220,0,0),12);
		//SetObjDrawTransform(500,0,16000,0,500,-34000, 12);
		if(myobject) {
			SetObjDrawTransform(1200,0,0,0,1200,0,1);
		}
		
		if(actiontype == ACTIONTYPE_VEHICLE)
		{
			SetGraphics("LetGo",GetID(),2,GFXOV_MODE_Base);
			SetName(Format("$TxtUnGrab$",myobject->GetName()));
		}
		else if(actiontype == ACTIONTYPE_STRUCTURE)
		{
			SetGraphics("Exit",GetID(),2,GFXOV_MODE_Base);
			SetName(Format("$TxtExit$",myobject->GetName()));
		}
	}
	else
	{
		//SetClrModulation(RGB(160,0,0),12);
		//SetObjDrawTransform(300,0,16000,0,300,-34000, 12);
		if(myobject) {
			SetObjDrawTransform(900,0,0,0,900,0,1);
		}
		
		if(actiontype == ACTIONTYPE_VEHICLE)
		{
			if(!(myobject->Contained()))
			{
				SetGraphics("Grab",GetID(),2,GFXOV_MODE_Base);
				SetName(Format("$TxtGrab$",myobject->GetName()));
			}
			else
			{
				SetGraphics("Exit",GetID(),2,GFXOV_MODE_Base);
				SetName(Format("$TxtPushOut$",myobject->GetName()));
			}
		}
		if(actiontype == ACTIONTYPE_STRUCTURE)
		{
			SetGraphics("Enter",GetID(),2,GFXOV_MODE_Base);
			SetName(Format("$TxtEnter$",myobject->GetName()));
		}
	}
	SetObjDrawTransform(IconSize(),0,-16000,0,IconSize(),20000, 2);
	
	UpdateHands();
}

public func UpdateHands()
{
	if(!crew) return;

	// the hands...
	var hands = selected;
	// .. are not displayed for inventory if the clonk is inside
	// a building or is pushing something because the controls
	// are redirected to those objects
	if(actiontype == ACTIONTYPE_INVENTORY)
		if(crew->Contained() || crew->GetProcedure() == "PUSH")
			hands = 0;
			
	if(hands)
	{
		if(hands & 1 || actiontype != ACTIONTYPE_INVENTORY)
		{
			SetGraphics("One",GetID(),3,GFXOV_MODE_Base);
			SetObjDrawTransform(HandSize(),0,-16000,0,HandSize(),-12000, 3);
		}
		else SetGraphics(nil,nil,3);
		if(hands & 2 || actiontype != ACTIONTYPE_INVENTORY)
		{
			SetGraphics("Two",GetID(),4,GFXOV_MODE_Base);
			SetObjDrawTransform(HandSize(),0,8000,0,HandSize(),-12000, 4);
		}
		else SetGraphics(nil,nil,4);
	}
	else
	{
		SetGraphics(nil,nil,3);
		SetGraphics(nil,nil,4);
	}
}
