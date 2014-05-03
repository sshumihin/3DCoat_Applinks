#include "c4d_symbols.h"
#include "c4d_file.h"
#include "c4d_string.h"
#include "ApplinkPreferences.h"

Bool ApplinkPreferences::Execute(BaseDocument* doc)
{
	return dlg.Open(DLG_TYPE_ASYNC, PID_APPLINK_PREFERENCES, 100, 100);
}

//
Bool RegisterApplink(void)
{
	return RegisterCommandPlugin(PID_APPLINK_PREFERENCES, GeLoadString(IDS_APPLINK_PREFERENCES), 0, AutoBitmap("icon_coat.tif"), GeLoadString(IDS_APPLINK_PREFERENCES_DESCR), NewObjClear(ApplinkPreferences));
}