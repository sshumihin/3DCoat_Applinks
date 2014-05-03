#include "c4d_symbols.h"

#include "c4d_gui.h"
#include "c4d_basedocument.h"
#include "c4d_basecontainer.h"
#include "c4d_string.h"
#include "c4d_baselist.h"
#include "customgui_datetime.h"
#include "ge_dynamicarray.h"
#include "ge_math.h"
#include "ge_lvector.h"

#include "ApplinkExporter.h"
#include "ApplinkDialog.h"
#include "ApplinkPreferences.h"

ApplinkExporter::ExportObject::ExportObject(void)
{
	Vp.Free(); Vt.Free(); //Vn.Free();
	Fv.Free(); Fvt.Free(); Fvn.Free(); Fpvnb.Free();
	pmatidxArray.Free(); tempMats.Free();
	pVertexCount = 0;
}
ApplinkExporter::ExportObject::~ExportObject(void)
{
	StatusClear();
}
LONG ApplinkExporter::PVertexLength(ExportObject& mObject)
{
	LONG length=0;
	for(LONG p=0; p < mObject.Fpvnb.GetCount(); p++)
	{
		length += mObject.Fpvnb[p];
	}
	return length;
}
BaseList2D* ApplinkExporter::getParameterLink(GeListNode& node, LONG paramID, LONG instanceOf)
{
	GeData parameter;
	if (node.GetParameter(DescLevel(paramID), parameter,0))
	{
		BaseLink* link = parameter.GetBaseLink();
		if (link) 
		{
			return link->GetLink(node.GetDocument(), instanceOf);
		}
	}
	return 0;
}
String ApplinkExporter::getParameterString(BaseTag& tag, LONG paramID)
{
	GeData parameter;
	if (tag.GetParameter(DescLevel(paramID), parameter,0)) 
	{
		return parameter.GetString();
	}
	return "";
}
LONG ApplinkExporter::getParameterLong(BaseMaterial& mat, LONG paramID)
{
	GeData parameter;
	if (mat.GetParameter(DescLevel(paramID), parameter,0))
	{
		return parameter.GetLong();
	}
	return 0;
}
Real ApplinkExporter::getParameterReal(C4DAtom& atom, LONG paramID, Real preset)
{
	GeData parameter;
	if (atom.GetParameter(DescLevel(paramID), parameter,0) && (parameter.GetType() != DA_NIL))
	{
		return parameter.GetReal();
	}
	return preset;
}

Vector ApplinkExporter::getParameterVector(C4DAtom& atom, LONG paramID)
{
	GeData parameter;
	if (atom.GetParameter(DescLevel(paramID), parameter,0)) 
	{
		return parameter.GetVector();
	}
	return NULL;
}
Filename ApplinkExporter::getParameterFilename(C4DAtom& atom, LONG paramID)
{
	GeData parameter;
	if (atom.GetParameter(DescLevel(paramID), parameter,0)) {
		return parameter.GetFilename();
	}
	return NULL;
}

Bool ApplinkExporter::Execute(BaseDocument* document, BaseContainer* bc)
{
	matDefault = BaseMaterial::Alloc(Mmaterial);
	if(!matDefault) return false;

	Filename fileObjPath;
	fileObjPath.SetDirectory(bc->GetString(IDC_TMP_FOLDER));
	fileObjPath.SetFile(document->GetDocumentName());
	fileObjPath.SetSuffix("obj");
	Filename fileObjOutPath;
	fileObjOutPath.SetDirectory(bc->GetString(IDC_TMP_FOLDER));
	fileObjOutPath.SetFile("output.obj");
	Filename fileImport;
	fileImport.SetDirectory(bc->GetString(IDC_EXCH_FOLDER));
	fileImport.SetFile("import.txt");

	GePrint(fileObjPath.GetString());
	GePrint(fileObjOutPath.GetString());
	GePrint(fileImport.GetString());

	const Matrix tM(Vector(0.0f, 0.0f, 0.0f), Vector(1.0f, 0.0f, 0.0f), Vector(0.0f, 1.0f, 0.0f), Vector(0.0f, 0.0f, -1.0f));

	//BaseDocument* doc = document->Polygonize();
	AutoAlloc<AtomArray> oSel;
	document->GetActiveObjects(oSel, FALSE);

	if(oSel->GetCount() > 0)
	{
//Write import.txt//
		AutoAlloc<BaseFile> basefileImport;
		
		if (!basefileImport->Open(fileImport, GE_WRITE, FILE_NODIALOG, GeGetByteOrder())) return FALSE;
		
		this->WriteString(fileObjPath.GetString() + "\n", basefileImport);
		this->WriteString(fileObjOutPath.GetString() + "\n", basefileImport);
		this->WriteString(mapType(bc->GetLong(IDC_COMBO_MAP_TYPE)) + "\n", basefileImport);

		GePrint(mapType(bc->GetLong(IDC_COMBO_MAP_TYPE)));
		basefileImport->Close();

		GePrint("File " + fileImport.GetString() + " write success!");

//Write file.obj//
		AutoAlloc<BaseFile> objfile;

		//if (!objfile) return FALSE;
		if (!objfile->Open(fileObjPath, GE_WRITE, FILE_NODIALOG, GeGetByteOrder())) return FALSE;

		String str;
		str = "#Wavefront OBJ Export for 3D-Coat\n";
		this->WriteString(str, objfile);
		tagDateTime t;
		DateTimeNow(t);
		str = "#File created: " + FormatTime("%d.%m.%Y  %H:%M:%S", t) + "\n";
		this->WriteString(str, objfile);
		str = "#Cinema4D Version: " + LongToString(GetC4DVersion()) + "\n";
		this->WriteString(str, objfile);
		this->WriteEndLine(objfile);

		Bool expMat = bc->GetBool(IDC_CHK_EXP_MAT);
		vpcnt = vtcnt = 0;

		for(int i = 0; i < oSel->GetCount(); i++)
		{
			StatusSetSpin();
			PolygonObject* ob = (PolygonObject*) oSel->GetIndex(i);
			if (ob->GetType() == Opolygon)
			{
				StatusSetText("Export object " + ob->GetName());
				ExportObject mObject;

				GePrint("Name " + ob->GetName());
				//GePrint("Type " + LongToString(ob->GetType()));

				if(expMat)
				{
					mObject.pmatidxArray.ReSize(ob->GetPolygonCount());
					mObject.tempMats.ReSize(1);
					mObject.pmatidxArray.Fill(0);
					Bool haveMats = false;
	//////////////////////////////////////////
					for(BaseTag* tag = ob->GetFirstTag(); tag != NULL; tag = tag->GetNext())
					{
						LONG typ = tag->GetType();
						if(typ == Ttexture)
						{
							if (!getParameterLink(*tag, TEXTURETAG_MATERIAL, Mbase)) continue;
						
							haveMats = true;
							TextureTag* txttag = (TextureTag*)tag;
							BaseMaterial* material = txttag->GetMaterial();

							if(material == NULL)
							{
								GePrint("Material not found on " + ob->GetName() + "object.");
								return false;
							}
							//GePrint("Mat Name: " + material->GetName());						

							String restrict = getParameterString(*tag, TEXTURETAG_RESTRICTION);
							if (restrict.Content())
							{
								mObject.tempMats.Push(material);
								//GePrint("Selection: " + restrict);
								for(BaseTag* seltag = ob->GetFirstTag(); seltag != NULL; seltag = seltag->GetNext())
								{
									LONG seltyp = seltag->GetType();
									if(seltyp == Tpolygonselection && seltag->GetName() == restrict)
									{
										SelectionTag* selecttag = (SelectionTag*)seltag;
										BaseSelect* sel = selecttag->GetBaseSelect();
										//GePrint("sel data count: " + LongToString(sel->GetCount()));

										LONG seg = 0, a, b, p;
										while (sel->GetRange(seg++, &a, &b))
										{
											for (p = a; p <= b; ++p)
											{
												//GePrint("seltpolygon: " + LongToString(p));
												mObject.pmatidxArray[p] = mObject.tempMats.GetCount()-1;
											}
										}
									}
								}
							}
							else
							{
								mObject.tempMats[0] = material;
								mObject.pmatidxArray.Fill(0);
							}
						}
					}

					if(!mObject.tempMats[0])
					{
						matDefault->SetName("Default");

						BaseChannel* color = matDefault->GetChannel(CHANNEL_COLOR);
						if (!color) return false;	// return some error
						BaseContainer cdata = color->GetData();
						cdata.SetVector(BASECHANNEL_COLOR_EX, Vector(1.0f, 1.0f, 1.0f));
						
						//document->InsertMaterial(matDefault, NULL, FALSE);
						//matDefault->Update(TRUE, TRUE);
						//matDefault->Message(MSG_UPDATE);

						mObject.tempMats[0] = matDefault;
						//GePrint("Global material not found on object " + ob->GetName() + ".");
						//return false;
					}

					if(haveMats)
					{
						//GePrint("mObject.tempMats.GetCount(): " + LongToString(mObject.tempMats.GetCount()));
						for(LONG m = 0; m < mObject.tempMats.GetCount(); m++)
						{
							Bool inMats = false;
							//GePrint("materialArray.GetCount(): " + LongToString(materialArray.GetCount()));
							for(LONG n = 0; n < materialArray.GetCount(); n++)
							{
								if(mObject.tempMats[m]->GetName() == materialArray[n]->GetName())
								{
									inMats = true;
									break;
								}
							}
							if(!inMats)
							{
								materialArray.Push(mObject.tempMats[m]);
							}
						}
					}

					//String str1;
					//for (LONG p = 0; p < ob->GetPolygonCount(); p++)
					//{
					//	str1 += LongToString(mObject.pmatidxArray[p]) + ",";
					//}
					//GePrint(str1);
				}
/////////////////////////////////////////////////
				const Vector* vadr = ob->GetPointR();
				const CPolygon* padr = ob->GetPolygonR();
				LONG vcnt = ob->GetPointCount();
				LONG pcnt = ob->GetPolygonCount();

				mObject.Fpvnb.ReSize(pcnt);// poly counts
				for(LONG p = 0; p < pcnt; p++)
				{
					if(padr[p].c != padr[p].d)
					{
						mObject.Fpvnb[p] = 4;
					}
					else
					{
						mObject.Fpvnb[p] = 3;
					}
				}
				mObject.pVertexCount = PVertexLength(mObject);

				//Vertex positions
				mObject.Vp.ReSize(vcnt);
				Matrix mg = tM * ob->GetMgn();
				for (LONG v = 0; v < vcnt; v++)
				{
					mObject.Vp[v] = vadr[v] * mg;
					//GePrint("Point[" + LongToString(i) + "] " + LongToString(padr[i].x) + ", " + LongToString(padr[i].y) + ", " + LongToString(padr[i].z));
					//str = "v " + LongToString(vadr[p].x) + " " + LongToString(vadr[p].y) + " " + LongToString(vadr[p].z) + "\n";
					//this->WriteString(str, objfile);
				}
				
				mObject.Fv.ReSize(mObject.pVertexCount);
				LONG y=0;
				for (LONG p = 0; p < pcnt; p++)
				{
					if(mObject.Fpvnb[p] == 4)
					{
						mObject.Fv[y] = padr[p].d;
						mObject.Fv[y+1] = padr[p].c;
						mObject.Fv[y+2] = padr[p].b;
						mObject.Fv[y+3] = padr[p].a;
					}
					else
					{
						mObject.Fv[y] = padr[p].c;
						mObject.Fv[y+1] = padr[p].b;
						mObject.Fv[y+2] = padr[p].a;
					}

					y += mObject.Fpvnb[p];
				}
				
				//String str1;
				//for (LONG p = 0; p < mObject.Fv.GetCount(); p++)
				//{
				//	str1 += LongToString(mObject.Fv[p]) + " ";
				//}
				//GePrint(str1);
///////////////////////////////
///////////vertex UV
//////////////////////////////
				if(bc->GetBool(IDC_CHK_EXP_UV))
				{
					// Get first UV tag (if at least one)
					UVWTag* uvw_tag = (UVWTag*)ob->GetTag(Tuvw, 0);
					if(!uvw_tag)
					{
						GePrint("Object \"" + ob->GetName() + "\" has no UVW tag.\nUV coordinates can't be exported.");
						return FALSE;
					}
					else
					{
						mObject.Vt.ReSize(mObject.pVertexCount);
						mObject.Fvt.ReSize(mObject.pVertexCount);						
						UVWStruct res;
						
						for(LONG t=0, y=0; t < pcnt; t++)
						{
							//GePrint("y: " + LongToString(y));
							res = uvw_tag->Get(t);
							if(mObject.Fpvnb[t] == 4)
							{
								mObject.Vt[y] = res.d;
								mObject.Vt[y + 1] = res.c;
								mObject.Vt[y + 2] = res.b;
								mObject.Vt[y + 3] = res.a;
							
								mObject.Fvt[y] = y;
								mObject.Fvt[y + 1] = y + 1;
								mObject.Fvt[y + 2] = y + 2;
								mObject.Fvt[y + 3] = y + 3;

							}
							else
							{
								mObject.Vt[y] = res.c;
								mObject.Vt[y + 1] = res.b;
								mObject.Vt[y + 2] = res.a;

								mObject.Fvt[y] = y;
								mObject.Fvt[y + 1] = y + 1;
								mObject.Fvt[y + 2] = y + 2;

							}
							y += mObject.Fpvnb[t];
						}
					}
					//String str1;
					//for (LONG p = 0; p < mObject.Fvt.GetCount(); p++)
					//{
					//	str1 += LongToString(mObject.Fvt[p]) + " ";
					//}
					//GePrint(str1);

				}

				WriteExportFile(bc, ob, objfile, mObject, vcnt, pcnt);
				//GePrint("Fvt: " + LongToString(Fvt.GetCount()));
				vpcnt += mObject.Vp.GetCount();
				if(bc->GetBool(IDC_CHK_EXP_UV))
					vtcnt += mObject.Vt.GetCount();
			}
		}
		objfile->Close();

		if(expMat && materialArray.GetCount() > 0)
			WriteMatsFile(document, bc);
	}
	else
	{
		GePrint("No selected objects!");
	}

	BaseMaterial::Free(matDefault);
	return TRUE;
}

String ApplinkExporter::mapType(LONG id)
{
	String strPaint = "[";
	switch (id)
	{
		case 0:
			strPaint += "ppp";
			break;
		case 1:
			strPaint += "mv";
			break;
		case 2:
			strPaint += "ptex";
			break;
		case 3:
			strPaint += "uv";
			break;
		case 4:
			strPaint += "ref";
			break;
		case 5:
			strPaint += "retopo";
			break;
		case 6:
			strPaint += "vox";
			break;
		case 7:
			strPaint += "alpha";
			break;
		case 8:
			strPaint += "prim";
			break;
		case 9:
			strPaint += "curv";
			break;
		case 10:
			strPaint += "autopo";
			break;

	}
	strPaint += "]";

	return strPaint;
}

void ApplinkExporter::WriteString(String string, BaseFile* file)
{
	LONG length = string.GetCStringLen(St8bit) + 1;
	CHAR* to_write = (CHAR*)GeAlloc(length);
	string.GetCString(to_write, length, St8bit);
	INT t;
	for(t = 0; t < length - 1; t++)
	{
		file->WriteChar(to_write[t]);
	}
	GeFree(to_write);
}

void ApplinkExporter::WriteEndLine(BaseFile* file)
{
	String string = "\n";
	LONG length = string.GetCStringLen(St8bit) + 1;
	CHAR* to_write = (CHAR*)GeAlloc(length);
	string.GetCString(to_write, length, St8bit);
	INT t;
	for(t = 0; t < length - 1; t++)
	{
		file->WriteChar(to_write[t]);
	}
	GeFree(to_write);
}

void ApplinkExporter::WriteVertexPositions(BaseFile* objfile, ExportObject& mObject, LONG vcnt)
{
	String str = "# begin " + LongToString(vcnt) + " vertices\n";
	this->WriteString(str, objfile);	

	//GePrint("Fv count: " + LongToString(Fv.GetCount()));
	//for (p = 0; p < Fv.GetCount(); p++)
	//{
	//	GePrint(LongToString(Fv[p]));
	//}

	for (LONG p = 0; p < mObject.Vp.GetCount(); p++)
	{
		str = "v " + RealToString(mObject.Vp[p].x, NULL, 6) + " " + RealToString(mObject.Vp[p].y, NULL, 6) + " " + RealToString(mObject.Vp[p].z, NULL, 6) + "\n";
		this->WriteString(str, objfile);
	}

	str = "# end " + LongToString(vcnt) + " vertices\n";
	this->WriteString(str, objfile);	
	this->WriteEndLine(objfile);
}
void ApplinkExporter::WriteUVWTag(BaseFile* objfile, ExportObject& mObject, LONG pcnt, const CPolygon* padr)
{	
	//GePrint("Length UV: " + RealToString(pUV->GetLength()));
/*	for(t = 0; t < pcnt*4; t++)
	{
		GePrint(LongToString(t) + ": " + RealToString(pUV[t].x) + ", " + RealToString(pUV[t].y) );
	}*/

	String str = "# begin " + LongToString(mObject.Vt.GetCount()) + " texture vertices\n";
	this->WriteString(str, objfile);

	for(LONG t = 0, y=0; t < pcnt; t++)
	{							
		str = "vt " + RealToString(mObject.Vt[y].x, NULL, 6) + " " + RealToString( 1 - mObject.Vt[y].y, NULL, 6) + " " + RealToString(mObject.Vt[y].z, NULL, 6) + "\n";
		this->WriteString(str, objfile);
		str = "vt " + RealToString(mObject.Vt[y+1].x, NULL, 6) + " " + RealToString(1 - mObject.Vt[y+1].y, NULL, 6) + " " + RealToString(mObject.Vt[y+1].z, NULL, 6) + "\n";
		this->WriteString(str, objfile);
		str = "vt " + RealToString(mObject.Vt[y+2].x, NULL, 6) + " " + RealToString(1 - mObject.Vt[y+2].y, NULL, 6) + " " + RealToString(mObject.Vt[y+2].z, NULL, 6) + "\n";
		this->WriteString(str, objfile);
		if(mObject.Fpvnb[t] == 4)
		{
			str = "vt " + RealToString(mObject.Vt[y+3].x, NULL, 6) + " " + RealToString(1 - mObject.Vt[y+3].y, NULL, 6) + " " + RealToString(mObject.Vt[y+3].z, NULL, 6) + "\n";
			this->WriteString(str, objfile);
		}
		y += mObject.Fpvnb[t];
	}

	str = "# end " + LongToString(mObject.Vt.GetCount()) + " texture vertices\n";
	this->WriteString(str, objfile);
	this->WriteEndLine(objfile);
}


void ApplinkExporter::WriteExportFile(BaseContainer* bc, PolygonObject* ob, BaseFile* objfile, ExportObject& mObject, LONG vcnt, LONG pcnt)
{
	const CPolygon* padr = ob->GetPolygonR();
	Bool expUV = bc->GetBool(IDC_CHK_EXP_UV);
	Bool expMat = bc->GetBool(IDC_CHK_EXP_MAT);
	String str;

	if(expMat && materialArray.GetCount() > 0)
	{
		Filename fileMatObj;
		fileMatObj.SetFile(GetActiveDocument()->GetDocumentName());
		fileMatObj.SetSuffix("mtl");

		str = "mtllib " + fileMatObj.GetFile().GetString() + "\n";
		this->WriteString(str, objfile);
		this->WriteEndLine(objfile);
	}

	str = "g " + ob->GetName() + "\n";
	this->WriteString(str, objfile);
	this->WriteEndLine(objfile);

	// vertex positions
	ApplinkExporter::WriteVertexPositions(objfile, mObject, vcnt);
	//UV
	if(expUV)
		ApplinkExporter::WriteUVWTag(objfile, mObject, pcnt, padr);

	//Polygon faces v/vt/vn (v//vn)
	str = "# begin " + LongToString(pcnt) + " faces\n";
	this->WriteString(str, objfile);

	LONG y=0;
	String prevMat = "", currMat = "";

	for (LONG i = 0; i < pcnt; i++)
	{
		if(expMat && materialArray.GetCount() > 0)
		{
			currMat = mObject.tempMats[mObject.pmatidxArray[i]]->GetName();
			if(currMat != prevMat)
			{
				str = "usemtl " + currMat + "\n";
				this->WriteString(str, objfile);
				prevMat = currMat;
			}
		}

		//GePrint("Polygon[" + LongToString(i) + "] " + LongToString(vadr[i].a) + ", " + LongToString(vadr[i].b) + ", " + LongToString(vadr[i].c) + ", " + LongToString(vadr[i].d));
		str = "f";
		//GePrint("poly vertices: " + LongToString(mObject.Fpvnb[i]));
		for(LONG j = 0; j < mObject.Fpvnb[i]; j++)
		{			
			str += " ";
			str += LongToString(mObject.Fv[y+j] + 1 + vpcnt);

			if(expUV && mObject.Fvt.GetCount() > 0)
			{
				str += "/";
				str += LongToString(mObject.Fvt[y+j] + 1 + vtcnt);
			}
		}

		str += "\n";
		//GePrint("str = " + str);
		this->WriteString(str, objfile);
		y += mObject.Fpvnb[i];
	}

	str = "# end " + LongToString(pcnt) + " faces\n";
	this->WriteString(str, objfile);
	this->WriteEndLine(objfile);
}

Bool ApplinkExporter::WriteMatsFile(BaseDocument* document, BaseContainer* bc)
{
	Filename filenameMTL;
	filenameMTL.SetDirectory(bc->GetString(IDC_TMP_FOLDER));
	filenameMTL.SetFile(document->GetDocumentName());
	filenameMTL.SetSuffix("mtl");
	
	GePrint(filenameMTL.GetString());

	AutoAlloc<BaseFile> fileMTL;
		
	if (!fileMTL->Open(filenameMTL, GE_WRITE, FILE_NODIALOG, GeGetByteOrder())) return FALSE;
	
	for(LONG i=0; i < materialArray.GetCount(); i++)
	{
		BaseMaterial* mat = materialArray[i];
		
		String str;
		str = "newmtl " + mat->GetName() + "\n";
		this->WriteString(str, fileMTL);

		//Ka
		str = "Ka 0.300000 0.300000 0.300000\n";
		this->WriteString(str, fileMTL);

/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
		//Kd
		if(getParameterLong(*mat, MATERIAL_USE_COLOR))
		{
			ExportChannel(document, fileMTL, *mat, MATERIAL_COLOR_SHADER, MATERIAL_COLOR_COLOR, "Kd");
		}

		//Ks
		if(getParameterLong(*mat, MATERIAL_USE_REFLECTION))
		{
			ExportChannel(document, fileMTL, *mat, MATERIAL_REFLECTION_SHADER, MATERIAL_REFLECTION_COLOR, "Ks");
		}

		//Ns
		str = "Ns 50.000000\n";
		this->WriteString(str, fileMTL);

		//Tr
		str = "Tr 0.000000\n";
		this->WriteString(str, fileMTL);

		//illum
		str = "illum 2\n";
		this->WriteString(str, fileMTL);
		this->WriteEndLine(fileMTL);
	}

	fileMTL->Close();

	return TRUE;
}

void ApplinkExporter::ExportChannel(BaseDocument* document, BaseFile* file, BaseMaterial& material, LONG shaderId, LONG colorId, const String s)
{
	String str = s + " ";

	// get texture strength and base colour
	Vector color = getParameterVector(material, colorId);

	str += RealToString(color.x, NULL, 6) + " " + RealToString(color.y, NULL, 6) + " " + RealToString(color.z, NULL, 6) + "\n";
	this->WriteString(str, file);

	// fetch bitmap shader, if available
	BaseList2D* bitmapLink = ApplinkExporter::getParameterLink(material, shaderId, Xbitmap);

	if(bitmapLink)
	{
		// if we are here, we've got a bitmap shader -> let's create an imagemap texture
		Filename bitmapPath = ApplinkExporter::getParameterFilename(*bitmapLink, BITMAPSHADER_FILENAME);
		Filename fullBitmapPath;
		GenerateTexturePath(document->GetDocumentPath(), bitmapPath, Filename(), &fullBitmapPath);

		str = "map_" + s + " ";
		str += fullBitmapPath.GetString() + "\n";
		this->WriteString(str, file);
	}
}
