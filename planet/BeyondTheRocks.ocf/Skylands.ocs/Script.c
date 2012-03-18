/**
	Skylands
	Assemble a plane on some floating islands
	
	@authors Sven2
*/

static const LOAM_Bridge_Amount = 65; // longer bridges!

static g_is_initialized;
static g_intro_initialized;

func DoInit(int first_player)
{
	// Set time of day to evening and create some clouds and celestials.
	EnsureObject(Environment_Clouds,0,0,-1);
	EnsureObject(Environment_Celestial,0,0,-1);
	EnsureObject(Rule_BuyAtFlagpole,0,0,-1);
	var time = EnsureObject(Environment_Time,0,0,-1);
	time->SetTime(600);
	time->SetCycleSpeed(12);
	// Goal
	CreateObject(Goal_Plane);
	// Plane part restore
	for (var part in FindObjects(Find_Func("IsPlanePart"))) part->AddRestoreMode();
	return true;
}

func EnsureObject(id def, int x, int y, int owner)
{
	var obj = FindObject(Find_ID(def));
	if (!obj) obj = CreateObject(def,x,y,owner);
	return obj;
}

func InitializePlayer(int plr)
{
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		var x = 150 + Random(50);
		crew->SetPosition(x , 400);
		crew->CreateContents(Shovel);
		// one clonk can construct, another can mine.
		if (index == 1)
		{
			crew->CreateContents(Hammer);
			crew->CreateContents(Wood,4);
			crew->CreateContents(Metal);
		}
		else
		{
			crew->CreateContents(Axe);
			crew->CreateContents(GrappleBow);
		}
		index++;
	}
	return;
}

func OnPlaneFinished(object plane)
{
  // todo: outro
  plane->CreateObject(Plane, 0,12, NO_OWNER);
  plane->RemoveObject();
}
