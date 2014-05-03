#ifndef __APPLINKPREFERENCES_H__
#define __APPLINKPREFERENCES_H__

#include <c4d.h>
#include "ApplinkDialog.h"

#define PID_APPLINK_PREFERENCES  1026650

class ApplinkPreferences : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc);

private:
	ApplinkDialog dlg;
};

#endif  // #ifndef __APPLINKPREFERENCES_H__