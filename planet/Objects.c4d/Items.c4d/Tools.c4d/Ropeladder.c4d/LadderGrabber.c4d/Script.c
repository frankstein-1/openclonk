/*-- Ropeladder_Grabber --*/


public func Interact(object clonk)
{
	if(GetActionTarget())
		GetActionTarget()->StartRollUp();
	else
		RemoveObject();
}

public func IsInteractable(object clonk)
{
	return clonk->GetProcedure() == "WALK" || clonk->GetProcedure() == "SCALE" || clonk->GetProcedure() == "HANGLE";
}

func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$GrabLadder$", IconName = nil, IconID = nil, Selected = false };
}

local ActMap = {
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
		},
};
local Name = "$Name$";
local Description = "$Description$";
	