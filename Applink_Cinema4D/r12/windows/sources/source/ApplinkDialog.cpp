#include <windows.h>
#include <tlhelp32.h>
#include "c4d_symbols.h"
#include "c4d_file.h"
#include "c4d_string.h"
#include "ApplinkDialog.h"
#include "ApplinkExporter.h"
#include "ApplinkImporter.h"

ApplinkDialog::ApplinkDialog(void){};

ApplinkDialog::~ApplinkDialog(void){};

Bool Find3DCoat()
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	if(!hSnapshot)return false;	
	pe.dwSize = sizeof(pe);
	for(int i = Process32First(hSnapshot, &pe); i; i = Process32Next(hSnapshot, &pe))
	{
		HANDLE hModuleSnap = NULL;
		MODULEENTRY32 me;
		hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe.th32ProcessID);
		if(hModuleSnap == (HANDLE) -1)continue;
		me.dwSize = sizeof(MODULEENTRY32);
		if(Module32First(hModuleSnap, &me))
		{
			do
			{
				char temp[MAX_PATH];
				strcpy_s(temp,MAX_PATH, me.szExePath);
				_strupr_s(temp, MAX_PATH);
				int p = (int)strlen(temp);
				char c = 0;
				while(c != '\\' && c != '/' && p != 0)
				{
					c = temp[p--];
				}
				char* s = temp + p;
				strupr(s);
				if(strstr(s,"3D-COAT"))
				{
					return true;
				}
				break;
			}while(Module32Next(hModuleSnap, &me));
		}
	}
	CloseHandle(hSnapshot);
	return false;
}

Bool ApplinkDialog::CreateLayout(void)
{
	return LoadDialogResource(DLG_APPLINK_PREFERENCES, NULL, 0);
}

///
Bool ApplinkDialog::InitValues(void)
{
	if (!GeDialog::InitValues()) return FALSE;

	filenamePrefs = GeGetPluginPath()+ "preference.ini";
	dirty = FALSE;
	
	AutoAlloc<HyperFile> hyperfilePrefs;

	if(!GeFExist(filenamePrefs, FALSE))
	{
		if (!hyperfilePrefs->Open('coat', filenamePrefs.GetString(), FILEOPEN_WRITE, FILEDIALOG_ANY)) return FALSE;

		gPreferences.SetString(IDC_TMP_FOLDER, "");

		Filename exFolder = GeGetC4DPath(C4D_PATH_MYDOCUMENTS) + "3D-CoatV3" + "Exchange";
		if(GeFExist(exFolder, TRUE))
		{
			gPreferences.SetString(IDC_EXCH_FOLDER, exFolder.GetString());
		}
		else
		{
			GePrint(String("Folder ..\\MyDocuments\\3D-CoatV3\\Exchange not found!"));
			gPreferences.SetString(IDC_EXCH_FOLDER, "");
		}

		gPreferences.SetLong(IDC_COMBO_MAP_TYPE, 0);
		gPreferences.SetBool(IDC_CHK_EXP_MAT, TRUE);
		gPreferences.SetBool(IDC_CHK_EXP_UV, TRUE);
		gPreferences.SetBool(IDC_CHK_SKIP_IMP_DIALOG, FALSE);
		gPreferences.SetBool(IDC_CHK_SKIP_EXP_DIALOG, FALSE);


		gPreferences.SetBool(IDC_CHK_IMP_MAT, TRUE);
		gPreferences.SetLong(IDC_COMBO_MAP_IMPORT, 0);
		gPreferences.SetBool(IDC_CHK_IMP_UV, TRUE);
		gPreferences.SetBool(IDC_CHK_REPLACE, TRUE);
		gPreferences.SetBool(IDC_CHK_PROMPT, FALSE);
		
		gPreferences.SetString(IDC_COAT_EXE_PATH, "");
		gPreferences.SetBool(IDC_CHK_COAT_START, FALSE);

		hyperfilePrefs->WriteContainer(gPreferences);
		hyperfilePrefs->Close();
	}

	if(!hyperfilePrefs->Open('coat', filenamePrefs.GetString(), FILEOPEN_READ, FILEDIALOG_ANY)) return FALSE;

	hyperfilePrefs->ReadContainer(&gPreferences, TRUE);
	hyperfilePrefs->Close();

	SetString(IDC_TMP_FOLDER, gPreferences.GetString(IDC_TMP_FOLDER));
	SetString(IDC_EXCH_FOLDER, gPreferences.GetString(IDC_EXCH_FOLDER));

	SetLong(IDC_COMBO_MAP_TYPE, gPreferences.GetLong(IDC_COMBO_MAP_TYPE));
	SetBool(IDC_CHK_EXP_MAT, gPreferences.GetBool(IDC_CHK_EXP_MAT));
	SetBool(IDC_CHK_EXP_UV, gPreferences.GetBool(IDC_CHK_EXP_UV));
	SetBool(IDC_CHK_SKIP_IMP_DIALOG, gPreferences.GetBool(IDC_CHK_SKIP_IMP_DIALOG));
	SetBool(IDC_CHK_SKIP_EXP_DIALOG, gPreferences.GetBool(IDC_CHK_SKIP_EXP_DIALOG));


	SetBool(IDC_CHK_IMP_MAT, gPreferences.GetBool(IDC_CHK_IMP_MAT));
	SetLong(IDC_COMBO_MAP_IMPORT, gPreferences.GetLong(IDC_COMBO_MAP_IMPORT));
	SetBool(IDC_CHK_IMP_UV, gPreferences.GetBool(IDC_CHK_IMP_UV));
	SetBool(IDC_CHK_REPLACE, gPreferences.GetBool(IDC_CHK_REPLACE));
	SetBool(IDC_CHK_PROMPT, gPreferences.GetBool(IDC_CHK_PROMPT));
	
	SetString(IDC_COAT_EXE_PATH, gPreferences.GetString(IDC_COAT_EXE_PATH));
	SetBool(IDC_CHK_COAT_START, gPreferences.GetBool(IDC_CHK_COAT_START));

	SetTimer(1000);

	return TRUE;
}

void ApplinkDialog::Timer(const BaseContainer &msg)
{
	Bool b;
	GetBool(IDC_CHK_PROMPT, b);
	if(b)
	{
		Filename fn;
		fn.SetDirectory(gPreferences.GetString(IDC_EXCH_FOLDER));
		fn.SetFile("export.txt");

		if(GeFExist(fn))
		{
			GePrint("File exists!");
			SetTimer(0);
			if(GeOutString("To import a new object?", GEMB_OKCANCEL) == GEMB_R_OK)
			{
				GePrint("Start import!");
				BaseDocument* doc = GetActiveDocument();
				ApplinkImporter* importer = gNew ApplinkImporter;
				importer->Execute(doc, &gPreferences);
				SetTimer(1000);
			}
			else
			{
				SetBool(IDC_CHK_PROMPT, FALSE);
			}
		}
	}
}
Bool ApplinkDialog::Command(LONG id, const BaseContainer& msg)
{
	dirty = TRUE;
	saveSettings();

	switch(id)
	{
	case IDC_BTN_EXPORT:
		{
			BaseDocument* doc = GetActiveDocument();
			ApplinkExporter* exporter = gNew ApplinkExporter;
			exporter->Execute(doc, &gPreferences);
			//GePrint("Start export!");

			String exeCoat = gPreferences.GetString(IDC_COAT_EXE_PATH);
			if(gPreferences.GetBool(IDC_CHK_COAT_START) && exeCoat != "")
			{
				if(!Find3DCoat())
				{
					LONG length = exeCoat.GetCStringLen(STRINGENCODING_8BIT) + 1;
					char* buffer = (CHAR*)GeAlloc(length);
					exeCoat.GetCString(buffer, length, STRINGENCODING_8BIT);

					SHELLEXECUTEINFO ExecuteInfo;

					memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));

					ExecuteInfo.cbSize       = sizeof(ExecuteInfo);
					ExecuteInfo.fMask        = 0;                
					ExecuteInfo.hwnd         = 0;                
					ExecuteInfo.lpVerb       = "open";                      // Operation to perform
					ExecuteInfo.lpFile       = buffer;  // Application name
					ExecuteInfo.lpDirectory  = 0;                           // Default directory
					ExecuteInfo.nShow        = SW_SHOWNORMAL;
					ExecuteInfo.hInstApp     = 0;

					if(ShellExecuteEx(&ExecuteInfo) == FALSE)
					// Could not start application -> call 'GetLastError()'
					//if((int)::ShellExecute(NULL, TEXT("open"), buffer, NULL, NULL, SW_SHOWNORMAL) <= 32)
					{
						GePrint("3D-Coat.exe not found!");
					}
				}
				else
				{
					GePrint("3D-Coat.exe is run!");
				}
			}
		}
		break;

	case IDC_BTN_IMPORT:
		{
			GePrint("Start import!");
			BaseDocument* doc = GetActiveDocument();
			ApplinkImporter* importer = gNew ApplinkImporter;
			importer->Execute(doc, &gPreferences);
		}
		break;

	case IDC_CHK_PROMPT:
		{
			Bool b;
			GetBool(IDC_CHK_PROMPT, b);
			if(b) SetTimer(1000);
		}
		break;
	default:
		break;
	}

	return TRUE;
}

void ApplinkDialog::DestroyWindow(void)
{
	if(dirty)
	{
		AutoAlloc<HyperFile> hyperfilePrefs;

		if (hyperfilePrefs->Open('coat', filenamePrefs.GetString(), FILEOPEN_WRITE, FILEDIALOG_ANY))
		{
			saveSettings();
			hyperfilePrefs->WriteContainer(gPreferences);
			hyperfilePrefs->Close();
		}
	}
}

void ApplinkDialog::saveSettings()
{
	GetString(IDC_TMP_FOLDER, &gPreferences, IDC_TMP_FOLDER);
	GetString(IDC_EXCH_FOLDER, &gPreferences, IDC_EXCH_FOLDER);

	GetLong(IDC_COMBO_MAP_TYPE, &gPreferences, IDC_COMBO_MAP_TYPE);
	GetBool(IDC_CHK_EXP_MAT, &gPreferences, IDC_CHK_EXP_MAT);
	GetBool(IDC_CHK_EXP_UV, &gPreferences, IDC_CHK_EXP_UV);
	GetBool(IDC_CHK_SKIP_IMP_DIALOG, &gPreferences, IDC_CHK_SKIP_IMP_DIALOG);
	GetBool(IDC_CHK_SKIP_EXP_DIALOG, &gPreferences, IDC_CHK_SKIP_EXP_DIALOG);

	GetBool(IDC_CHK_IMP_MAT, &gPreferences, IDC_CHK_IMP_MAT);
	GetLong(IDC_COMBO_MAP_IMPORT, &gPreferences, IDC_COMBO_MAP_IMPORT);
	GetBool(IDC_CHK_IMP_UV, &gPreferences, IDC_CHK_IMP_UV);
	GetBool(IDC_CHK_REPLACE, &gPreferences, IDC_CHK_REPLACE);
	GetBool(IDC_CHK_PROMPT, &gPreferences, IDC_CHK_PROMPT);
	
	GetString(IDC_COAT_EXE_PATH, &gPreferences, IDC_COAT_EXE_PATH);
	GetBool(IDC_CHK_COAT_START, &gPreferences, IDC_CHK_COAT_START);
}