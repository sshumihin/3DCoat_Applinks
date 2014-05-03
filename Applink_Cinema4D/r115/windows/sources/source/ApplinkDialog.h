#ifndef __APPLINKDIALOG_H__
#define __APPLINKDIALOG_H__

#include <c4d.h>

class ApplinkDialog : public GeDialog
{
public:
	ApplinkDialog(void);
	virtual ~ApplinkDialog(void);
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id, const BaseContainer& msg);
	virtual void DestroyWindow(void);
	virtual void Timer(const BaseContainer &msg);

	void saveSettings();

	BaseContainer gPreferences;
private:
	Filename filenamePrefs;
	Bool dirty;
};

#endif  // #ifndef __APPLINKDIALOG_H__