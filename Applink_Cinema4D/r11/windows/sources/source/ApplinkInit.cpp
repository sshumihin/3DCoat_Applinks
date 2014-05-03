#include "c4d.h"
#include "ApplinkPreferences.h"

//04/II/2011
//version 1.0

Bool RegisterApplink(void);

C4D_CrashHandler old_handler;

void ApplinkCrashHandler(CHAR *crashinfo)
{
	//printf("SDK CrashInfo:\n");
	//printf(crashinfo);
	
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
