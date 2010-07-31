/*
Marker for King of the Hill
Author: Zapper
*/

func Definition(def) {
	def["Name"] = "$Name$";
}

func Initialize()
{
	AddEffect("Timer", this, 10, 1, this);
}

func FxTimerTimer()
{
	var origin=this["origin"];
	if(!origin) return RemoveObject();
	
	var x, y;
	if(origin->GetKing())
	{
		x=origin->GetKing()->GetX();
		y=origin->GetKing()->GetY() - 20;
	}
	else
	{
		x=origin->GetX();
		y=origin->GetY();
	}
	
	if(Distance(this->GetX(), this->GetY(), x, y) <= 10)
	{
		this->SetPosition(x, y);
	}
	else
	{
		var x_dir=BoundBy((x - this->GetX()) / 10, -4, 4);
		var y_dir=BoundBy((y - this->GetY()) / 10, -4, 4);
		this->SetPosition(this->GetX() + x_dir, this->GetY() + y_dir);
	}
	
	this->SetClrModulation(origin->GetStarColor(Sin(this->GetActTime(), 50)));
}
