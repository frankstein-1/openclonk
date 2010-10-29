// Stone door destructible, and auto control for the base.

#appendto StoneDoor

protected func Damage()
{
	if (GetDamage() > 180)
		Split2Components();
	return;
}

private func IsOpen()
{
	if (GBackSolid(0, -20))
	 	return true;
	return false;
}

private func IsClosed()
{
	if (GBackSolid(0, 19))
	 	return true;
	return false;
}

// Automatically open for team stored in effect var 0.
protected func FxAutoControlStart(object target, int num, int temporary, int team)
{
	if (temporary == 0)
	EffectVar(0, target, num) =  team;
	return 1;
}

protected func FxAutoControlTimer(object target, int num, int time)
{
	var d = 0;
	if (IsOpen())
		d = 30;
	var team = EffectVar(0, target, num);
	var open_door = false;
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember), Find_InRect(-50, d - 30, 100, 60)))
	{
		var plr = clonk->GetOwner();
		var plr_team = GetPlayerTeam(plr);
		if (plr_team == team)
			open_door = true;
		else
		{
			open_door = false;
			break;
		}
	}
	
	if (open_door && IsClosed())
		OpenGateDoor();
	if (!open_door && IsOpen())
		CloseGateDoor();
	
	return 1;
}