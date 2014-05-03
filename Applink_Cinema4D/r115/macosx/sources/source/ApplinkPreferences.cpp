#include "c4d_symbols.h"
#include "c4d_file.h"
#include "c4d_string.h"
#include "ApplinkPreferences.h"

Bool ApplinkPreferences::Execute(BaseDocument* doc)
{
	return dlg.Open(TRUE, PID_APPLINK_PREFERENCES, 100, 100);
}

//
Bool RegisterApplink(void)
{
	return RegisterCommandPlugin(PID_APPLINK_PREFERENCES, "Applink 3D-Coat", 0, "icon_coat.tif", GeLoadString(IDS_APPLINK_PREFERENCES_DESCR), gNew ApplinkPreferences);
}