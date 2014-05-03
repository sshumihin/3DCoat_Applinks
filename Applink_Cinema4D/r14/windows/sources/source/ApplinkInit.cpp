#include "c4d.h"
#include "ApplinkPreferences.h"

//19/V/2013
//version 1.0
//for Cinema 14
//Writed by Svyatoslav Shumikhin, sshumihin@gmail.com

Bool RegisterApplink(void);

C4D_CrashHandler old_handler;

void ApplinkCrashHandler(CHAR *crashinfo)
{
	// don't forget to call the original handler!!!
	if (old_handler) (*old_handler)(crashinfo);
}

Bool PluginStart(void)
{
	// example of installing a crashhandler
	old_handler = C4DOS.CrashHandler; // backup the original handler (must be called!)
	C4DOS.CrashHandler = ApplinkCrashHandler; // insert the own handler

	if (!RegisterApplink()) return FALSE;
	return TRUE;
}

void PluginEnd(void)
{
}

Bool PluginMessage(LONG id, void *data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return FALSE; // don't start plugin without resource
			return TRUE;

		case C4DMSG_PRIORITY: 
			return TRUE;
	}

	return FALSE;
}
