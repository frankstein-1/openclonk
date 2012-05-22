/**--
	Time Controller
	Author:Ringwall
	
	Creates time based on the 24-hour time scheme.
	Time is computed in minutes, which are by default
	1/2 a second in real life (18 frames). This will
	make each complete day/night cycle last 12 minutes
	in real life.
--*/


local time; 

// Sets the current time using a 24*60 minute clock scheme.
public func SetTime(int to_time) 
{
	// Set time.
	time = to_time % (24 * 60);
	// Adjust to time.
	AdjustToTime();
	return;
}

// Returns the time in minutes.
public func GetTime()
{
	return time;
}

// Sets the number of frames per clonk-minute.
// Standard is 18 frames per minute, making a day-night cycle of 12 minutes.
// Setting minute lenght to 0 will stop day-night cycle.
public func SetCycleSpeed(int speed)
{
	//GetEffect("IntTimeCycle", this).Interval = Max(0, speed);
	RemoveEffect("IntTimeCycle", this);
	AddEffect("IntTimeCycle", this, 100, Max(0, speed), this);
	return;
}

public func GetCycleSpeed()
{
	return GetEffect("IntTimeCycle", this).Interval;
}

local time_set;


protected func Initialize()
{
	// Only one time control object.
	if (ObjectCount(Find_ID(Environment_Time)) > 1) 
		return RemoveObject();
		
	time_set = {
		SunriseStart = 180,
		SunriseEnd = 540,
		SunsetStart = 900,
		SunsetEnd = 1260,
	};
	
	// Add effect that controls time cycle.
	AddEffect("IntTimeCycle", this, 100, 18, this);
	
	// Set the time to midday (12:00).
	time = 720; 

	// Create moon and stars.
	if (FindObject(Find_ID(Environment_Celestial)))
	{
		CreateObject(Moon, LandscapeWidth() / 2, LandscapeHeight() / 6);
		PlaceStars();
	}
	return;
}

public func IsDay()
{
	var day_start = (time_set["SunriseStart"] + time_set["SunriseEnd"]) / 2;
	var day_end = (time_set["SunsetStart"] + time_set["SunsetEnd"]) / 2;
	if (Inside(time, day_start, day_end))
		return true;
	return false;
}

public func IsNight()
{
	var night_start = (time_set["SunsetStart"] + time_set["SunsetEnd"]) / 2;
	var night_end = (time_set["SunriseStart"] + time_set["SunriseEnd"]) / 2;
	if (Inside(time, night_start, night_end))
		return true;
	return false;
}

private func PlaceStars()
{
	//Star Creation
	var maxamount = LandscapeWidth() * LandscapeHeight() / 40000;
	var amount = 0;

	while (amount != maxamount)
	{
		var pos;
		if (pos = FindPosInMat("Sky", 0, 0, LandscapeWidth(), LandscapeHeight()))
			CreateObject(Star, pos[0], pos[1]); 
		amount++;
	}
	return;
}

// Cycles through day and night.
protected func FxIntTimeCycleTimer(object target)
{
	// Adjust to time.
	AdjustToTime();

	// Advance time.
	time++;
	time %= (24 * 60);
	
	return 1;
}

// Adjusts the sky, celestial and others to the current time. Use SetTime() at runtime, not this.
private func AdjustToTime()
{
	var skyshade = [0,0,0,0]; //R,G,B,A
	var nightcolour = [10,25,40]; //default darkest-night colour
	
	//Darkness of night dependant on moon-phase
	var satellite = FindObject(Find_ID(Moon)); //pointer to the moon
	if(satellite){
		var phase = satellite->GetPhase();
		
		if(phase == 1 || phase == 5) nightcolour = [4,7,9]; //super dark when moon is crescent
		else if(phase == 2 || phase == 4) nightcolour = [5,15,25]; //somewhat dark when moon is half
		else nightcolour = [10,25,40]; //deep-blue when moon is full
	}
		
	
	// Sunrise 
	if (Inside(time, time_set["SunriseStart"], time_set["SunriseEnd"]))
	{
		skyshade[0] = Sin((GetTime() - time_set["SunriseStart"]) / 4, 255 - nightcolour[0]) + nightcolour[0];
		skyshade[1] = Sin((GetTime() - time_set["SunriseStart"]) / 4, 255 - nightcolour[1]) + nightcolour[1];
		skyshade[2] = Sin((GetTime() - time_set["SunriseStart"]) / 4, 255 - nightcolour[2]) + nightcolour[2];
		
		skyshade[3] = Sin((GetTime() - time_set["SunriseStart"]) / 4, 255);
		if (time == 540)
			if (satellite)
				satellite->Phase();
	}
	// Day
	else if (Inside(time, time_set["SunriseEnd"], time_set["SunsetStart"]))
	{
		skyshade[0] = 255;
		skyshade[1] = 255;
		skyshade[2] = 255;
		
		skyshade[3] = 255;
	}
	//Sunset
	else if (Inside(time, time_set["SunsetStart"], time_set["SunsetEnd"]))
	{
		skyshade[0] = 255 - Sin((GetTime() - time_set["SunsetStart"]) / 4, 255 - nightcolour[0]);
		skyshade[1] = 255 - Sin((GetTime() - time_set["SunsetStart"]) / 4, 255 - nightcolour[1]);
		skyshade[2] = 255 - Sin((GetTime() - time_set["SunsetStart"]) / 4, 255 - nightcolour[2]);
		
		skyshade[3] = 255 - Sin((GetTime() - time_set["SunsetStart"]) / 4, 255);
	}
	// Night
	else if (time > time_set["SunsetEnd"] || time < time_set["SunriseStart"])
	{
		skyshade[0] = nightcolour[0];
		skyshade[1] = nightcolour[1];
		skyshade[2] = nightcolour[2];
		
		skyshade[3] = 0;
	}
	
	// Shade sky.
	SetSkyAdjust(RGB(skyshade[0], skyshade[1], skyshade[2]));
	
	// Adjust celestial objects.
	for (var celestial in FindObjects(Find_Func("IsCelestial")))
			celestial->SetObjAlpha(255 - skyshade[3]);
			
	// Adjust clouds
	for(var cloud in FindObjects(Find_ID(Cloud))){
		cloud->RequestAlpha(skyshade[3]);
	}
	
	return;
}

local Name = "Time";
