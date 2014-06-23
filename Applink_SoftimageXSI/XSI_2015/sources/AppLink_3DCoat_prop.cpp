#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <tlhelp32.h>

#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_customproperty.h>
#include <xsi_filereference.h>
#include <xsi_model.h>
#include <xsi_parameter.h>
#include <xsi_pluginregistrar.h>
#include <xsi_pluginitem.h>
#include <xsi_plugin.h>
#include <xsi_ppgeventcontext.h>
#include <xsi_ppglayout.h>
#include <xsi_project.h>
#include <xsi_projectitem.h>
#include <xsi_ref.h>
#include <xsi_scene.h>
#include <xsi_siobject.h>
#include <xsi_status.h>
#include <xsi_utils.h>
#include <xsi_uitoolkit.h>
#include <shlobj.h>

using namespace XSI;
extern Application app;

bool Find3DCoat();
BOOL FileOrDirectoryExists(LPCTSTR, bool);
void FindFolder3DCoatV(CustomProperty&, TCHAR*);

CString pluginPath = CUtils::BuildPath(app.GetInstallationPath(siUserAddonPath), L"AppLink_3DCoat", L"Application", L"Plugins");
//*****************************************************************************
/*!	Helper function for accessing the Import Export custom property.
 */
//*****************************************************************************
CustomProperty Get3DCoatProp()
{	
	//Application app;
	Model root = app.GetActiveSceneRoot();
	CustomProperty prop = root.GetProperties().GetItem(L"AppLink_3DCoat");
	if (!prop.IsValid())
	{
		prop = root.AddProperty( L"AppLink_3DCoat" );
	}
	return prop;
}


Parameter Get3DCoatParam( const CString& in_strName )
{
	CustomProperty pset = Get3DCoatProp();
	if (!pset.IsValid()) return CRef();
	
	Parameter param = pset.GetParameters().GetItem(in_strName);
	
	return param;
}


SICALLBACK AppLink_3DCoat_Define( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CustomProperty pset;
	Parameter param;
	pset = ctxt.GetSource();

	CValue dft ;

	pset.AddParameter(L"Logo", CValue::siString, NULL, L"", L"", dft, param);
	pset.AddParameter(L"tempLocation", CValue::siString, siPersistable, L"Temp Path", L"", dft, param);
	pset.AddParameter(L"coatLocation", CValue::siString, siPersistable, L"Exchange Path", L"", dft, param);
	pset.AddParameter(L"exeLocation", CValue::siString, siPersistable, L"3D-Coat.exe", L"", dft, param);
	pset.AddParameter(L"typePaint", CValue::siUInt1, NULL, L"Painting type", L"", (LONG)0, (LONG)0, (LONG)10,  dft, dft, param);
	pset.AddParameter(L"bExpNorm", CValue::siBool, NULL, L"Normals", L"", true, param);
	pset.AddParameter(L"bExpMat", CValue::siBool, NULL, L"Material", L"", true, param);
	pset.AddParameter(L"bExpUV", CValue::siBool, NULL, L"Current UV Set", L"", true, param);
	pset.AddParameter(L"bExpSkipImp", CValue::siBool, NULL, L"Skip Import Dialog", L"", false, param);
	pset.AddParameter(L"bExpSkipExp", CValue::siBool, NULL, L"Skip Export Dialog", L"", false, param);
	//pset.AddParameter(L"bCopyTexE", CValue::siBool, NULL, L"Copy textures", L"", false, param);
	pset.AddParameter(L"bStart", CValue::siBool, NULL, L"Start 3D-Coat", L"", true, param);

	//pset.AddParameter(L"bCopyTexI", CValue::siBool, NULL, L"Copy textures", L"", false, param);
	pset.AddParameter(L"bReplace", CValue::siBool, NULL, L"Replace Objects", L"", true, param);
	pset.AddParameter(L"bEvent", CValue::siBool, NULL, L"Prompt", L"", false, param);
	pset.AddParameter(L"bImpNorm", CValue::siBool, NULL, L"Normals", L"", true, param);
	pset.AddParameter(L"bImpMat", CValue::siBool, NULL, L"Material", L"", true, param);
	//pset.AddParameter(L"bImpNewMat", CValue::siBool, NULL, L"New material", L"", false, param);
	pset.AddParameter(L"swMap", CValue::siUInt1, NULL, L"Map type", L"", (LONG)0, (LONG)0, (LONG)1,  dft, dft, param);
	pset.AddParameter(L"bImpUV", CValue::siBool, NULL, L"UV Set", L"", true, param);

	return CStatus::OK;
}


SICALLBACK AppLink_3DCoat_DefineLayout( CRef& in_ctxt )
{	
	Context ctxt(in_ctxt);
	PPGLayout ppg = ctxt.GetSource();
	PPGItem item;

	Plugin pl(app.GetPlugins().GetItem(L"AppLink_3DCoat_Plugin"));
	CString logoPath = CUtils::BuildPath(app.GetInstallationPath(pl.GetOrigin()), L"AppLink_3DCoat", L"Application", L"Plugins", L"3dcoatLogo.bmp");

	CString txt = L"*Select folder ..\\My Documents\\3D-CoatV3\\Exchange";
	ppg.Clear();

	ppg.AddTab(L"General");
		item = ppg.AddItem(L"Logo",L"", siControlBitmap);
		item.PutAttribute(siUIFilePath, logoPath);
		item.PutAttribute(siUINoLabel, true);

		ppg.AddGroup(L"Locations");
			item = ppg.AddItem( L"tempLocation", L"", siControlFilePath );
			item.PutAttribute( siUIInitialDir, L"project" ) ;
			//item.PutAttribute( siUIFileFilter, L"3D files (*.obj,*.lwo)|*.obj,*.lwo|All Files (*.*)|*.*||" ) ;
			item = ppg.AddItem( L"coatLocation", L"", siControlFolder ) ;
			item.PutAttribute( siUIInitialDir, L"user" ) ;
			ppg.AddStaticText(txt.GetAsciiString());
			//item = ppg.AddItem( L"coatExe", L"", siControlFilePath);
			//item.PutAttribute( siUIInitialDir, L"user" );
			//item.PutAttribute( siUIFileFilter, L"Exe files (*.exe)|*.exe|All Files (*.*)|*.*||" ) ;

		ppg.EndGroup();

//Paint mesh in 3D-Coat using per-pixel painting [ppp]
//Paint mesh in 3D-Coat using microvertex painting [mv]
//Paint mesh in 3D-Coat using Ptex [ptex]
//Perform UV-mapping in 3D-Coat [uv]
//Drop reference mesh to 3D-Coat [ref]
//Drop retopo mesh as new layer in 3D-Coat [retopo]
//Drop mesh in 3D-Coat as voxel object [vox] 
//Drop mesh in 3D-Coat as new pen alpha [alpha]
//Drop mesh in 3D-Coat as new merging primitive for voxels [prim]
//Drop mesh in 3D-Coat as a curve profile [curv]
//Drop mesh in 3D-Coat for Auto-retopology [autopo]

		ppg.AddGroup(L"Export");
			CValueArray sizeItems( 22 ) ;
			sizeItems[0] = L"Per-pixel painting"; sizeItems[1] = (LONG)0 ;
			sizeItems[2] = L"Microvertex painting"; sizeItems[3] = (LONG)1 ;
			sizeItems[4] = L"Ptex"; sizeItems[5] = (LONG)2 ;
			sizeItems[6] = L"Perform UV-mapping"; sizeItems[7] = (LONG)3 ;
			sizeItems[8] = L"Drop reference mesh"; sizeItems[9] = (LONG)4 ;
			sizeItems[10] = L"Drop retopo mesh as new layer"; sizeItems[11] = (LONG)5 ;
			sizeItems[12] = L"Drop mesh as voxel object"; sizeItems[13] = (LONG)6 ;
			sizeItems[14] = L"Drop mesh as new pen alpha"; sizeItems[15] = (LONG)7 ;
			sizeItems[16] = L"Drop mesh as new merging primitive for voxels"; sizeItems[17] = (LONG)8 ;
			sizeItems[18] = L"Drop mesh as a curve profile"; sizeItems[19] = (LONG)9 ;
			sizeItems[20] = L"Drop mesh for Auto-retopology"; sizeItems[21] = (LONG)10 ;
			ppg.AddEnumControl( L"typePaint", sizeItems, L"", siControlCombo ) ;
			ppg.AddItem(L"bExpNorm");
			ppg.AddItem(L"bExpMat");
			ppg.AddItem(L"bExpUV");
			ppg.AddItem(L"bExpSkipImp");
			ppg.AddItem(L"bExpSkipExp");
			//ppg.AddItem(L"bCopyTexE");

			item = ppg.AddButton( L"Coat3DExport", L"Export to 3DCoat" );
			item.PutAttribute(siUICX, (LONG)ppg.GetAttribute(siUICX));
		ppg.EndGroup();

		ppg.AddGroup(L"Import");
			ppg.AddItem(L"bImpNorm");
			ppg.AddItem(L"bImpMat");

			CValueArray sizeItems2( 4 );
			sizeItems2[0] = L"Normal map"; sizeItems2[1] = (LONG)0 ;
			sizeItems2[2] = L"Bump map"; sizeItems2[3] = (LONG)1 ;
			ppg.AddEnumControl( L"swMap", sizeItems2, L"", siControlCombo ) ;

			ppg.AddItem(L"bImpUV");
			ppg.AddRow();
				ppg.AddItem(L"bReplace");
				ppg.AddItem(L"bEvent");
			ppg.EndRow();
			//ppg.AddItem(L"bCopyTexI");
			item = ppg.AddButton( L"Coat3DImport", L"Import from 3DCoat" );
			item.PutAttribute(siUICX, (LONG)ppg.GetAttribute(siUICX));
		ppg.EndGroup();

		ppg.AddTab(L"3D-Coat Location");
			item = ppg.AddItem( L"exeLocation", L"", siControlFilePath );
			item.PutAttribute( siUIFileFilter, L"Exe files (*.exe)|*.exe|All Files (*.*)|*.*||" );
			item.PutAttribute( siUIOpenFile, true );
			ppg.AddItem(L"bStart");

	
	return CStatus::OK;
}

SICALLBACK AppLink_3DCoat_PPGEvent( const CRef& in_ctxt )
{	
	//Application app;
	PPGEventContext ctxt( in_ctxt ) ;
	CustomProperty prop = ctxt.GetSource();
	PPGEventContext::PPGEvent eventID = ctxt.GetEventID() ;

	//CString s_ExeLocation = prop.GetParameter(L"coatExe").GetValue();

	if ( eventID == PPGEventContext::siOnInit )
	{
		CString s_ExportLocation = prop.GetParameter(L"tempLocation").GetValue();
		CString s_CoatLocation = prop.GetParameter(L"coatLocation").GetValue();

		Plugin pl(app.GetPlugins().GetItem(L"AppLink_3DCoat_Plugin"));
		pluginPath = CUtils::BuildPath(app.GetInstallationPath(pl.GetOrigin()), L"AppLink_3DCoat", L"Application", L"Plugins");

		if(prop.GetParameter(L"tempLocation").GetValue() == L"")
		{
			Project prj = app.GetActiveProject();
			Scene scn = prj.GetActiveScene();
			CString tempPath = CUtils::BuildPath(app.GetInstallationPath(siProjectPath), L"3DCoat", scn.GetName());
			prop.GetParameter(L"tempLocation").PutValue(tempPath);
			prop.GetParameter(L"tempLocation").PutCapabilityFlag(siReadOnly, true);
		}

		if(prop.GetParameter(L"coatLocation").GetValue() == L"")
		{
			TCHAR Path[MAX_PATH];
			if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, Path))) 
			{
				CString exchPath = CUtils::BuildPath(Path, L"3D-CoatV3", L"Exchange");
				if(FileOrDirectoryExists(exchPath.GetAsciiString(), true))
				{				
					prop.GetParameter(L"coatLocation").PutValue(exchPath + CUtils::Slash());
				}
				else
				{
					exchPath = CUtils::BuildPath(Path, L"3D-CoatV4", L"Exchange");
					if(FileOrDirectoryExists(exchPath.GetAsciiString(), true))
					{				
						prop.GetParameter(L"coatLocation").PutValue(exchPath + CUtils::Slash());
					}				
					else
					{
						prop.GetParameter(L"coatLocation").PutValue(L"");
					}
				}
			}
		}

		if(prop.GetParameter(L"exeLocation").GetValue() == L"")
		{
			TCHAR Path[MAX_PATH];
			if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, Path))) 
			{
				FindFolder3DCoatV(prop, Path);
			}
			else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, Path))) 
			{
				FindFolder3DCoatV(prop, Path);
			}
		}
		//prop.GetParameter(L"bImpNorm").PutCapabilityFlag(siNotInspectable, true);
		//prop.GetParameter(L"bImpNewMat").PutCapabilityFlag(siReadOnly, true);

		ctxt.PutAttribute(L"Refresh",true);

	}
	else if ( eventID == PPGEventContext::siButtonClicked )
	{
		CValue buttonPressed = ctxt.GetAttribute( L"Button" );
		CString tempLocation = prop.GetParameter(L"tempLocation").GetValue();
		CString coatLocation = prop.GetParameter(L"coatLocation").GetValue();
	
		if( buttonPressed.GetAsText() == L"Coat3DExport" )
		{
			CustomProperty in_pset = ctxt.GetSource();

			if(CUtils::EnsureFolderExists(tempLocation, true))
			{
				CValueArray args(6);
				args[0] = tempLocation;
				args[1] = coatLocation;
				args[2] = in_pset.GetParameterValue( L"typePaint" );
				//args[3] = in_pset.GetParameterValue( L"bCopyTexE" );
				args[3] = in_pset.GetParameterValue( L"bExpMat" );
				args[4] = in_pset.GetParameterValue( L"bExpUV" );
				args[5] = in_pset.GetParameterValue( L"bExpNorm" );
				
				CValue retVal;

				app.ExecuteCommand( L"Coat3DExport", args, retVal );
				
				CString exeLocation = prop.GetParameter(L"exeLocation").GetValue();
				bool bStart = prop.GetParameter(L"bStart").GetValue();
				
				if(bStart)
				{
					std::ifstream exefile(exeLocation.GetAsciiString());
					if(exefile.good())
					{
						exefile.close();

						if(!Find3DCoat())
						{
							if((int)::ShellExecute(NULL, TEXT("open"), exeLocation.GetAsciiString(), NULL, NULL, SW_SHOWNORMAL) <= 32)
							{
								app.LogMessage(L"3D-Coat.exe not found!", siWarningMsg);
							}
						}
						else
						{
							app.LogMessage(L"3D-Coat.exe is run!", siWarningMsg);
						}
					}
					else
					{
						app.LogMessage(L"3D-Coat.exe not found!", siWarningMsg);
						exefile.close();
					}
				}
			}
			else
			{
				LONG button;
				UIToolkit uitoolkit = app.GetUIToolkit();
				CString str = L"File "+tempLocation+" is not be writed!\nMake sure that the user permissions allow write file.";
				uitoolkit.MsgBox(str, siMsgCritical, L"Applink Message", button);
				app.LogMessage(str, siErrorMsg);
				return CStatus::False;
			}
		}
		else if( buttonPressed.GetAsText() == L"Coat3DImport" )
		{
			CustomProperty in_pset = ctxt.GetSource();

			CString exportPath = CUtils::BuildPath(coatLocation, L"export.txt");
			CString objPath;

			std::ifstream mfrExportTxt;
			mfrExportTxt.open (exportPath.GetAsciiString());//c:\Documents and Settings\user\My Documents\3D-CoatV3\Exchange\export.txt
			//app.LogMessage(L"Export.txt is it! :" + strOut);
			if(mfrExportTxt.good())
			{
				std::string row;
				std::getline(mfrExportTxt, row);
				objPath = row.c_str();
				//app.LogMessage(L"first row: " + CString(row.c_str()));
			}
			else
			{
				UIToolkit uitool = app.GetUIToolkit();
				LONG out;
				uitool.MsgBox(L"import file not found!", siMsgOkOnly|siMsgExclamation, L"", (LONG) out);
				if(out == siMsgOk)
				{
					return CStatus::False;
				}
			}
			mfrExportTxt.close();
			app.LogMessage(coatLocation);
			app.LogMessage(exportPath);

			if(objPath != L"")
			{
				CValueArray args(2);
				args[0] = objPath;
				args[1] = in_pset.GetParameterValue( L"bReplace" );

				CValue noret;
				app.ExecuteCommand( L"Coat3DImport", args, noret );
			}
		}
	}
	else if ( eventID == PPGEventContext::siParameterChange )
	{
		Parameter changed = ctxt.GetSource();
		CustomProperty prop = changed.GetParent();		
		CString   paramName = changed.GetScriptName();

		if ( paramName == L"bImpMat" )
		{
			CValue bFlag = prop.GetParameter(L"bImpMat").GetValue();
			//prop.GetParameter(L"swMap").PutCapabilityFlag(siNotInspectable, !bFlag);
			prop.GetParameter(L"swMap").PutCapabilityFlag(siReadOnly, !bFlag);
		}
		ctxt.PutAttribute(L"Refresh",true);
	}

	return CStatus::OK ;
}

void FindFolder3DCoatV(CustomProperty& prop, TCHAR* path)
{
	CString exePath = CUtils::BuildPath(path, L"3D-Coat-V3");
	if(FileOrDirectoryExists(exePath.GetAsciiString(), true))
	{
		prop.GetParameter(L"exeLocation").PutValue(exePath + CUtils::Slash());
	}
	else
	{
		exePath = CUtils::BuildPath(path, L"3D-Coat-V4");
		if(FileOrDirectoryExists(exePath.GetAsciiString(), true))
		{
			prop.GetParameter(L"exeLocation").PutValue(exePath + CUtils::Slash());
		}
		else
		{
			prop.GetParameter(L"exeLocation").PutValue(L"");
		}
	}
}

bool Find3DCoat()
{
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	if(!hSnapshot)return false;	
	pe.dwSize = sizeof(pe);
	for(int i = Process32First(hSnapshot, &pe); i; i = Process32Next(hSnapshot, &pe))
	{
		HANDLE hModuleSnap = NULL;
		MODULEENTRY32 me;
		hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe.th32ProcessID);
		if(hModuleSnap == (HANDLE) -1) continue;
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
				while(c != '\\' && c != '/' && (p != 0))
				{
					c = temp[p--];
				}
				char* s = temp + p;
				strupr(s);
				if(strstr(s, TEXT("3D-COAT"))) return true;
				break;
			}
			while(Module32Next(hModuleSnap, &me));
		}		
	}
	CloseHandle(hSnapshot);
	return false;
}

BOOL FileOrDirectoryExists(LPCTSTR szPath, bool isDir)
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  if(isDir) return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}