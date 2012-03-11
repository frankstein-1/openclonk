/*--
	Plane construction site
	Author: Sven2

	Successive buildup of the plane
--*/

local progress, next_part;

public func Initialize()
{
	SetProgress(0);
}

func SetProgress(int new_progress)
{
	var parts = [Plane_Skids, Plane_Chassis, Plane_Wings, Plane_Engine, Plane_Propeller, nil];
	progress = new_progress;
	if (!progress)
	{
		SetGraphics("Site");
		SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
	}
	else if (progress < 5)
	{
		SetGraphics(Format("%d", new_progress));
		SetGraphics("Site", GetID(), GFX_Overlay, GFXOV_MODE_Base);
	}
	else
	{
		SetGraphics();
		SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
	}
	next_part = parts[progress];
	return true;
}

func Timer()
{
	if (next_part)
		for (var part in FindObjects(Find_ID(next_part), Find_InRect(-30,-15,60,30), Find_Layer(GetObjectLayer())))
			if (part->GetCon() >= 100)
			{
				AddPart(part);
				return;
			}
}

func AddPart(object part)
{
	part->RemoveObject();
	Sound("Applause", true);
	SetProgress(progress+1);
	return true;
}


func Definition(def) {

}

local Name = "$Name$";
