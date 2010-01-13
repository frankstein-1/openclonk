/*-- Neustart --*/

func Activate(iPlr)
{
  // Szenario benachrichtigen
  if(GameCall("OnRestart", iPlr)) return;
  // Den Clonk des Spielers l�schen
  var pClonk = GetCrew(iPlr);
  if (pClonk) pClonk->RemoveObject(true);
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
