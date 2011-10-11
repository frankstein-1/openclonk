/*-- Tools workshop --*/

#include Library_PowerConsumer
#include Library_Producer



public func Initialize()
{
	// SetProperty("MeshTransformation", Trans_Rotate(RandomX(-30,30),0,1,0));
	return _inherited(...);
}



/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsToolProduct();
}

private func ProductionTime() { return 150; }
private func PowerNeed(id product) { return 150; }

public func NeedsRawMaterial(id rawmat_id)
{
	return true;
}

public func OnProductionStart(id product)
{
	SetSign(product);
	AddEffect("Working", this, 100, 1, this);
	return;
}

public func OnProductionHold(id product)
{
	return;
}

public func OnProductionContinued(id product)
{

	return;
}

public func OnProductionFinish(id product)
{
	RemoveEffect("Working", this);
	SetSign(nil);
	return;
}

protected func FxWorkingTimer()
{
	Smoking();
	return 1;
}

private func Smoking()
{
	if (Random(6)) Smoke(+16,-14,16);
	if (Random(8)) Smoke(10,-14,15+Random(3));
	return 1;
}

public func SetSign(id def)
{
	if (!def)
		return SetGraphics("", nil, 1, 4);
	var iSize = Max(def->GetDefCoreVal("Picture", "DefCore", 2), def->GetDefCoreVal("Picture", "DefCore", 3));
	SetGraphics("", def, 1, 4);
	SetObjDrawTransform(200, 0, 460*iSize, 0, 200, 90*iSize, 1);
}

local ActMap = {
	Build = {
		Prototype = Action,
		Name = "Build",
		Procedure = DFA_NONE,
		Length = 40,
		Delay = 1,
		FacetBase=1,
		NextAction = "Build",
		//Animation = "Turn",
		PhaseCall="Smoking",
	},
};
func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
