#appendto Clonk

protected func RejectCollect(id objid, object obj)
{
	if(objid != Boompack) return true;
	return _inherited(objid, obj);
}

public func MaxContentsCount()
{
	return 1;
}
