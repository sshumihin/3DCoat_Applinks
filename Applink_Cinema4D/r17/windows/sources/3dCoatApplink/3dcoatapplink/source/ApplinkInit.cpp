#include "c4d.h"
#include "ApplinkPreferences.h"

//5/X/2013
//version 1.0
//for Cinema 15
//Writed by Svyatoslav Shumikhin, sshumihin@gmail.com

Bool RegisterApplink(void);

C4D_CrashHandler old_handler;

Bool PluginStart(void)
{
	if (!RegisterApplink()) return false;
	return true;
}

void PluginEnd(void)
{
}

Bool PluginMessage(Int32 id, void *data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return false; // don't start plugin without resource
			return true;

		case C4DMSG_PRIORITY: 
			return true;
	}

	return false;
}
