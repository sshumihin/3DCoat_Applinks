#ifndef __APPLINKPREFERENCES_H__
#define __APPLINKPREFERENCES_H__

#include <c4d.h>
#include "ApplinkDialog.h"

#define PID_APPLINK_PREFERENCES  1026650

class ApplinkPreferences : public CommandData
{
public:
	override Bool Execute(BaseDocument* doc);
	//override Bool ExecuteOptionID(BaseDocument* doc, Int32 plugid, Int32 subid);

private:
	ApplinkDialog dlg;
};

#endif  // #ifndef __APPLINKPREFERENCES_H__