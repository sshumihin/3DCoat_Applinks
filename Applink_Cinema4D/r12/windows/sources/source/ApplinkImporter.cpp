#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "c4d_symbols.h"

#include "c4d_basedocument.h"
#include "c4d_basecontainer.h"
#include "c4d_baseobject.h"
#include "c4d_string.h"
#include "ge_prepass.h"

#include "ApplinkImporter.h"
#include "ApplinkDialog.h"
#include "ApplinkPreferences.h"
#include "Triangulator.h"

using namespace std;
ApplinkImporter::ApplinkImporter()
{
	// These variables should be fairly self explanatory - they are counters
	numFaces = 0;
	numVerticies = 0;
	numTexCoords = 0;
	numGroups = -1;
	groups.clear();
	matArray.clear();
}
// Destructor - frees memory from the pointers, even if the system does it for us anyway
ApplinkImporter::~ApplinkImporter()
{
	// The memory *should* be cleared by the OS, but we do this just to be sure
	// Free the memory for the following:
	//delete[] groups;
	delete[] verticies;
	delete[] uvw;
}
BaseTag* ApplinkImporter::GetLastTag(PolygonObject* pObj)
{
	BaseTag* lastTag = NULL;
	for(BaseTag* tag = pObj->GetFirstTag(); tag != NULL; tag = tag->GetNext())
	{
		lastTag = tag;
	}
	return lastTag;
}

void ApplinkImporter::InsertTextureTag(PolygonObject* pObj, String mat, String& name)
{
	TextureTag* txttag = TextureTag::Alloc();
	txttag->SetMaterial(doc->SearchMaterial(mat));
	BaseContainer bc = txttag->GetData();
	bc.SetLong(TEXTURETAG_PROJECTION, TEXTURETAG_PROJECTION_UVW);
	bc.SetLong(TEXTURETAG_SIDE, TEXTURETAG_SIDE_FRONT);
	bc.SetString(TEXTURETAG_RESTRICTION, name);
	bc.SetLong(TEXTURETAG_TILE, 1);
	txttag->SetData(bc);

	pObj->InsertTag(txttag, this->GetLastTag(pObj));
	txttag->Message(MSG_UPDATE);
}
vector<string> ApplinkImporter::Split(const string& strValue, const char separator)
{
    vector<string> vecstrResult;
    int startpos=0;
    int endpos=0;

    endpos = (int)strValue.find_first_of(separator, startpos);
    while (endpos != -1)
    {       
        vecstrResult.push_back(strValue.substr(startpos, endpos-startpos)); // add to vector
        startpos = endpos+1; //jump past sep
        endpos = (int)strValue.find_first_of(separator, startpos); // find next
        if(endpos==-1)
        {
            //lastone, so no 2nd param required to go to end of string
            vecstrResult.push_back(strValue.substr(startpos));
        }
    }

    return vecstrResult;
}

Bool ApplinkImporter::gatheringObjDate(char* path)
{
	char line[256];
	std::ifstream fsImportObj;
	fsImportObj.open(path);

	if(!fsImportObj.is_open()){GePrint("File " + String(path) + " not found!");return false;}

	StatusSetText("Gathering of data...");

	while(!fsImportObj.eof())
	{
		fsImportObj.getline(line, 99);
	
		if(!strncmp("v ", line, 2))
		{
			numVerticies++;
			groups[numGroups].numGVertices++;
		}
		else if(!strncmp("vt", line, 2))
		{
			numTexCoords++;
			groups[numGroups].numGTVertices++;
		}
		else if(!strncmp("g ", line, 2))
		{
			numGroups++;
			groups.resize(numGroups+1);
			groups[numGroups].numGFaces = 0;
			groups[numGroups].numGVertices = 0;
			groups[numGroups].numGTVertices = 0;
		}
		else if(!strncmp("f ", line, 2))
		{
			numFaces++;		// Only for global counting, nothing else

			istringstream iss(line);
			vector<string> stringArray;
			copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(stringArray));
			int numFace = (int)(stringArray.size() - 1);
			if(numFace < 3)
			{
				GePrint("Wrong face in OBJ file!");
				return false;
			}
			else if(numFace == 3 || numFace == 4)
			{
				groups[numGroups].numGFaces++;
			}
			else if(numFace > 4)
			{
				groups[numGroups].numGFaces += (numFace-2);
			}
		}
	}

	fsImportObj.close();
	
	verticies = new Vector[numVerticies];
	uvw = new Vector[numTexCoords];
	
	//GePrint("Nb Grp: " + LongToString(numGroups));
	for(unsigned int i = 0; i < groups.size(); i++)
	{
		groups[i].faces = new Face[groups[i].numGFaces + 1];
		groups[i].polyMatIdx = new int[groups[i].numGFaces + 1];
		//GePrint("Nb faces: " + LongToString(groups[i].numGFaces));
		//GePrint("Nb tex vert: " + LongToString(groups[i].numGTVertices));
		//GePrint("Nb norm vert: " + LongToString(groups[i].numGNVertices));
	}

	return true;
}
Bool ApplinkImporter::parseObjFile(char* path)
{
	char line[256];
	vector<string> stringArray;
	vector<string> subArray;
	ifstream fsImportObj;
	fsImportObj.open(path);

	if(!fsImportObj.is_open()){GePrint("File " + String(path) + " not found!");return false;}
	GePrint("Open file: " + String(path) + ".");

	long faceIndex = 0;
	int groupIndex = -1;	// First group at 0
	long pointIndex = 0;
	long texCoordIndex = 0;
	String matName;
	long idxMat = -1;
	
	StatusSetText("Parse file...");

	while(!fsImportObj.eof())
	{
		fsImportObj.getline(line, 255);
		//GePrint("row: " + String(buffer));
		if(!strncmp("mtllib ", line, 7))
		{
			char file[256];
			sscanf(line, "mtllib %s", file);

			this->mtlFilePath = path;
			this->mtlFilePath.ClearSuffix();
			this->mtlFilePath.SetFile(file);
		}
		else if(!strncmp("usemtl ", line, 7))
		{
			String newMat;
			stringArray.clear();
			istringstream iss(line);
			copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string>>(stringArray));

			newMat = stringArray[1].c_str();
			bool inMats = false;
			for(unsigned int i = 0; i < this->matArray.size(); i++)
			{
				if(this->matArray[i].Name == newMat)
				{
					inMats = true;
					idxMat = i;
					break;
				}
			}
			if(!inMats)
			{
				this->matArray.resize(this->matArray.size() + 1);
				this->matArray[this->matArray.size() - 1].Name = newMat;
				idxMat = (long)this->matArray.size() - 1;
			}
		}
		else if(!strncmp("g ", line, 2))
		{			
			groupIndex++;
			faceIndex = 0;
			sscanf(line, "g %s", &groups[groupIndex].groupName);
		}
		else if(!strncmp("v ", line, 2))
		{
			sscanf(line, "v %lf %lf %lf", &(verticies[pointIndex].x), &(verticies[pointIndex].y), &(verticies[pointIndex].z));
			pointIndex++;
		}
		else if(!strncmp("vt", line, 2))
		{
			sscanf(line, "vt %lf %lf", &(this->uvw[texCoordIndex].x), &(this->uvw[texCoordIndex].y));
			sscanf(line, "vt %lf %lf %lf", &(this->uvw[texCoordIndex].x), &(this->uvw[texCoordIndex].y), &(this->uvw[texCoordIndex].z));

			this->uvw[texCoordIndex].y = 1 - this->uvw[texCoordIndex].y;
			texCoordIndex++;
		}
		else if(!strncmp("f ", line, 2))
		{
			if(idxMat != -1)
			{
				groups[groupIndex].polyMatIdx[faceIndex] = idxMat;
			}
			//GePrint("size: " + LongToString(sizeof(line)));
			stringArray.clear();
			istringstream iss(line);
			copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(stringArray));
			//GePrint("size: " + LongToString(stringArray.size()));
			subArray.clear();
			int numFaces = (int)(stringArray.size() -1);
			if(numFaces == 3 || numFaces == 4)
			{
				for(ULONG i = 1; i <= numFaces; i++)
				{
					subArray = this->Split(stringArray[i], '/');
					groups[groupIndex].faces[faceIndex].vp[i-1] = atol(subArray[0].c_str()) -1;
					if(subArray[1] != "")
					{					
						groups[groupIndex].faces[faceIndex].vt[i-1] = atol(subArray[1].c_str()) - 1;
					}
				}
				
				if(numFaces == 3)// this triangle, non quad.
				{
					groups[groupIndex].faces[faceIndex].vp[3] = atol(subArray[0].c_str()) - 1;
					if(subArray[1] != "")
					{
						groups[groupIndex].faces[faceIndex].vt[3] = atol(subArray[1].c_str()) - 1;
					}
				}
				faceIndex++;
			}
			else// N-Gons
			{
				Triangulator *t = createTriangulator();
				vector<ULONG> vpIdxs;
				vector<ULONG> vtIdxs;
				for(ULONG j=1; j <= numFaces; j++)
				{
					subArray = this->Split(stringArray[j], '/');
					ULONG idx = atol(subArray[0].c_str()) -1;
					vpIdxs.push_back(idx);
					if(subArray[1] != "")
					{
						vtIdxs.push_back(atol(subArray[1].c_str()) -1);
					}
					t->addPoint(verticies[idx].x, verticies[idx].y, verticies[idx].z);
				}

				unsigned int tcount;
				unsigned int *indices = t->triangulate(tcount);				

				// print out the results.
				/*String str = "";
				for (int t=0; t<tcount; t++)
				{
					str += "(";
					str += LongToString(indices[t*3+0]);
					str += ",";
					str += LongToString(indices[t*3+1]);
					str += ",";
					str += LongToString(indices[t*3+2]);
					str += ")";
				}				
				GePrint("NGones: " + str);
*/
				for(ULONG f = 0; f < tcount; f++)
				{
					for(ULONG v=0; v < 4; v++)
					{
						ULONG vv = 2-v;
						if(v == 3) vv = 0;
						groups[groupIndex].faces[faceIndex].vp[v] = vpIdxs[indices[f*3+vv]];//vp
						if(vpIdxs.size() > 0)//vt
						{					
							groups[groupIndex].faces[faceIndex].vt[v] = vtIdxs[indices[f*3+vv]];
						}
					}

					faceIndex++;
					//GePrint("Face3: " + LongToString(groups[groupIndex].faces[faceIndex].vp[0]) + ", " + LongToString(groups[groupIndex].faces[faceIndex].vp[1]) + ", " + LongToString(groups[groupIndex].faces[faceIndex].vp[2]));
				}
				
				releaseTriangulator(t);
			}
		}
	}

	fsImportObj.close();

	return true;
}


Bool ApplinkImporter::createObjects()
{
	BaseDocument* doc = GetActiveDocument();
	LONG vertCnt = 0;
	const Matrix tM(LVector(0.0f, 0.0f, 0.0f), LVector(1.0f, 0.0f, 0.0f), LVector(0.0f, 1.0f, 0.0f), LVector(0.0f, 0.0f, -1.0f));

	StatusSetText("Create objects...");
	for(unsigned int i = 0; i < groups.size(); i++)
	{
		Vector *vadr = 0;// vertex adress
		CPolygon *padr = 0;// polygon adress

		Group gr = this->groups[i];

		if(this->pSet.impReplace)
		{
			BaseObject* bo = doc->SearchObject(gr.groupName);
			if(bo)
			{				
				BaseObject::Free(bo);
			}
		}

		PolygonObject* pObj = PolygonObject::Alloc(gr.numGVertices, gr.numGFaces);
		if (!pObj){ return false;}

		pObj->SetName(this->groups[i].groupName);

		//GePrint("Name group: " + pObj->GetName());
		//GePrint("V count: " + LongToString(gr.numGVertices));
		//GePrint("Poly count: " + LongToString(gr.numGFaces));

		vadr = pObj->GetPointW();
		padr = pObj->GetPolygonW();

		for (int p = 0; p < gr.numGFaces; p++)
		{
			padr[p] = CPolygon(gr.faces[p].vp[0] - vertCnt, gr.faces[p].vp[1] - vertCnt, gr.faces[p].vp[2] - vertCnt, gr.faces[p].vp[3] - vertCnt);
			//GePrint("poly " + LongToString(p) + ": " + LongToString(padr[p].a) + ", " + LongToString(padr[p].b) + ", " + LongToString(padr[p].c) + ", " + LongToString(padr[p].d));
		}

		for (int v = 0; v < gr.numGVertices; v++)
		{
			vadr[v] = this->verticies[v + vertCnt] * tM;
			//GePrint("vert " + LongToString(v) + ": " + LongToString(vadr[v].x) + ", " + LongToString(vadr[v].y) + ", " + LongToString(vadr[v].z));
		}
		
//// import UV
		if(ApplinkImporter::pSet.impUV && gr.numGTVertices > 0)
		{
			UVWStruct us;
			UVWTag* uvwtag = NULL;

			uvwtag = UVWTag::Alloc(gr.numGFaces);
			if(!uvwtag) return false;

			for (LONG p = 0; p < gr.numGFaces; p++)
			{
				us = UVWStruct(this->uvw[gr.faces[p].vt[0]], this->uvw[gr.faces[p].vt[1]], this->uvw[gr.faces[p].vt[2]], this->uvw[gr.faces[p].vt[3]]);
				void *dataptr = uvwtag->GetDataAddressW();
				uvwtag->Set(dataptr, p, us);
			}

			pObj->InsertTag(uvwtag, NULL);
			uvwtag->Message(MSG_UPDATE);
		}

////// insert phongTag
		if (!pObj->MakeTag(Tphong)) GePrint("Error on inserting phongTag. Object: " + pObj->GetName());

/////add materials, textures and poly clusters
		if(this->pSet.impMat)
		{
			String selTagName = "";
			if(this->matArray.size() == 1)
			{
				this->InsertTextureTag(pObj, this->matArray[0].Name, selTagName);
			}
			else if(this->matArray.size() > 1)
			{
				this->InsertTextureTag(pObj, this->matArray[0].Name, selTagName);

				CPolygon ps;
				SelectionTag* polyTag = NULL;

				for (unsigned int c = 1; c < this->matArray.size(); c++)
				{
					polyTag = SelectionTag::Alloc(Tpolygonselection);
					if(!polyTag) return false;

					selTagName = "Selection " + LongToString(c);
					polyTag->SetName(selTagName);
					BaseSelect* sel = polyTag->GetBaseSelect();
					for (LONG p = 0; p < gr.numGFaces; p++)
					{
						if(gr.polyMatIdx[p] == c)
						{
							sel->Select(p);
						}
					}
					
					pObj->InsertTag(polyTag, this->GetLastTag(pObj));
					polyTag->Message(MSG_UPDATE);

					this->InsertTextureTag(pObj, this->matArray[c].Name, selTagName);
				}
			}
		}

		doc->InsertObject(pObj, NULL, NULL);
		pObj->Message(MSG_UPDATE);

		ModelingCommandData md;
		md.doc = doc;
		md.op  = pObj;
		if(!SendModelingCommand(MCOMMAND_REVERSENORMALS, md)) return false;

		pObj = 0;
		vertCnt += gr.numGVertices;
	}

	return true;
}
Bool ApplinkImporter::Execute(BaseDocument* document, BaseContainer* bc)
{
	this->doc = document;
	this->pSet.impUV = bc->GetBool(IDC_CHK_IMP_UV);
	this->pSet.impMat = bc->GetBool(IDC_CHK_IMP_MAT);
	this->pSet.impReplace = bc->GetBool(IDC_CHK_REPLACE);
	this->pSet.impComboMap = bc->GetLong(IDC_COMBO_MAP_IMPORT);
	this->pSet.tempPath = bc->GetString(IDC_TMP_FOLDER);
//////////////////////////////////
//// read file export.txt
/////////////////////////////////
	this->exportFilePath.SetDirectory(bc->GetString(IDC_EXCH_FOLDER));
	this->exportFilePath.SetFile("export.txt");

	this->texturesFilePath.SetDirectory(bc->GetString(IDC_EXCH_FOLDER));
	this->texturesFilePath.SetFile("textures.txt");

	char file[255];
	ifstream fsExportTxt;
	this->exportFilePath.GetString().GetCString(file, 255, STRINGENCODING_XBIT);
	fsExportTxt.open(file);

	if(!fsExportTxt.is_open()){GePrint("File " + this->exportFilePath.GetString() + " not found!");return false;}	
		
	GePrint("Open file: " + this->exportFilePath.GetString() + ".");
///////////////////////////////////////////
//// read the path to exported obj
///////////////////////////////////////////
	char buffer[255];
	fsExportTxt.getline(buffer, 255);
	fsExportTxt.close();

///////////////////////////////////////////
//// remove file export.txt
///////////////////////////////////////////

	if(remove(file) == -1)
	{
		GePrint("File " + String(buffer) + " can not removed!");
	}
	
	//GePrint("buffer " + String(buffer));
//////////////////////////////////
////read exported *.obj
//////////////////////////////////
	StatusSetSpin();
	this->gatheringObjDate(buffer);
	
	this->parseObjFile(buffer);

	if(this->pSet.impMat && this->matArray.size() > 0)
	{
		this->createMaterials();
	}

	this->createObjects();

	StatusClear();

	DrawViews(DRAWFLAGS_ONLY_ACTIVE_VIEW|DRAWFLAGS_NO_THREAD|DRAWFLAGS_NO_ANIMATION);

	return true;
}

Bool ApplinkImporter::createMaterials()
{
	this->readMTL();
	this->insertMaterials();

	return true;
}

Bool ApplinkImporter::readMTL()
{
	char line[256];
	std::ifstream fsImportMtl;

	String smtlFilePath = this->mtlFilePath.GetString();
	LONG length = smtlFilePath.GetCStringLen(STRINGENCODING_8BIT) + 1;
	char* buffer = (CHAR*)GeAlloc(length);
	smtlFilePath.GetCString(buffer, length, STRINGENCODING_8BIT);

	fsImportMtl.open(buffer);

	if(!fsImportMtl.is_open()){GePrint("File " + this->mtlFilePath.GetString() + " not found!");return false;}

	GePrint("Open file " + this->mtlFilePath.GetString() + ".");

	int materialIndex = -1;

	while(!fsImportMtl.eof())
	{
		fsImportMtl.getline(line, 255);
	
		if(!strncmp("newmtl ", line, 7))
		{
			materialIndex++;			
			this->matArray.resize(materialIndex + 1);
			char temp[100];		// Temp buffer, for comparison
			sscanf(line, "newmtl %s", temp);	// Get the name of the material from the line
			this->matArray[materialIndex].Name = String(temp);

			this->matArray[materialIndex].useTextures = false;
		}
		else if(!strncmp("Ns ", line, 3))
		{
			sscanf(line, "Ns %lf", &(this->matArray[materialIndex].Ns));
		}
		else if(!strncmp("d ", line, 2))
		{
			sscanf(line, "d %lf", &(this->matArray[materialIndex].d));
		}
		else if(!strncmp("illum ", line, 6))
		{
			sscanf(line, "illum %d", &(this->matArray[materialIndex].illum));
		}
		else if(!strncmp("Kd ", line, 3))
		{
			sscanf(line, "Kd %lf %lf %lf", &(this->matArray[materialIndex].Kd.x), &(this->matArray[materialIndex].Kd.y), &(this->matArray[materialIndex].Kd.z));
		}
		else if(!strncmp("Ka ", line, 3))
		{
			sscanf(line, "Ka %lf %lf %lf", &(this->matArray[materialIndex].Ka.x), &(this->matArray[materialIndex].Ka.y), &(this->matArray[materialIndex].Ka.z));
		}
		else if(!strncmp("Ks ", line, 3))
		{
			sscanf(line, "Ks %lf %lf %lf", &(this->matArray[materialIndex].Ks.x), &(this->matArray[materialIndex].Ks.y), &(this->matArray[materialIndex].Ks.z));
		}
		else if(!strncmp("Ke ", line, 3))
		{
			sscanf(line, "Ke %lf %lf %lf", &(this->matArray[materialIndex].Ke.x), &(this->matArray[materialIndex].Ke.y), &(this->matArray[materialIndex].Ke.z));
		}
		else if(!strncmp("map_Kd ", line, 7))
		{
			char str[256];
			sscanf(line, "map_Kd %s", str);
			this->matArray[materialIndex].map_Kd = str;
			this->matArray[materialIndex].useTextures = true;
		}
		else if(!strncmp("map_Ks ", line, 7))
		{
			char str[256];
			sscanf(line, "map_Ks %s", str);
			this->matArray[materialIndex].map_Ks = str;
			this->matArray[materialIndex].useTextures = true;
		}
	}

	fsImportMtl.close();

//////////////////////////////////////////////
//	read textures.txt
//////////////////////////////////////////////

	std::ifstream fsImportText;
	String textFilePath = this->texturesFilePath.GetString();
	length = textFilePath.GetCStringLen(STRINGENCODING_8BIT) + 1;
	buffer = (CHAR*)GeAlloc(length);
	textFilePath.GetCString(buffer, length, STRINGENCODING_8BIT);
	char* fileText = buffer;

	fsImportText.open(buffer);

	if(!fsImportText.is_open()){GePrint("File " + this->texturesFilePath.GetString() + " not found!");return false;}

	GePrint("Open file " + this->texturesFilePath.GetString() + ".");

	int matIdx = 0;

	while(!fsImportText.eof())
	{
		fsImportText.getline(line, 255);

		for(unsigned int i = 0; i < this->matArray.size(); i++)
		{
			if(this->matArray[i].Name == String(line))
			{
				matIdx = i;
				break;
			}
		}

		if(!strncmp("displacement ", line, 13))
		{
			sscanf(line, "displacement %f", &(this->matArray[matIdx].disp_k));

			fsImportText.getline(line, 255);
			this->matArray[matIdx].disp_map = line;
		}
		else if(!strncmp("normalmap", line, 9))
		{
			fsImportText.getline(line, 255);
			this->matArray[matIdx].norm_map = line;
		}
	}

	fsImportText.close();

///////////////////////////
//remove file texture.txt
//////////////////////////
	if(remove(fileText) == -1)
	{
		GePrint("File " + String(fileText) + " can not removed!");
	}

	return true;
}

Bool ApplinkImporter::insertMaterials()
{
	BaseMaterial *mat = NULL;

	for(unsigned int i = 0; i < this->matArray.size(); i++)
	{
		mat = this->doc->SearchMaterial(this->matArray[i].Name);

		if (!mat)
		{
			mat = BaseMaterial::Alloc(Mmaterial);
			if (!mat)
			{
				GePrint("Memory allocation error for material.");
				return false;
			}
			mat->SetName(this->matArray[i].Name);
			doc->InsertMaterial(mat, NULL, FALSE);
		}

		CreateChannelColor(i, mat);
		CreateChannelSpecular(i, mat);

		//GePrint("map: " + LongToString(this->pSet.impComboMap));
		if(this->pSet.impComboMap == 0)
		{
			CreateChannelDisplacement(i, mat);
		}
		else
		{
			CreateChannelBump(i, mat);
		}

		CreateChannelNormal(i, mat);

		mat->Update(TRUE, TRUE);
		mat->Message(MSG_UPDATE);
	}

	return true;
}

Bool ApplinkImporter::CreateChannelColor(long i, BaseMaterial* mat)
{
	BaseChannel *color = mat->GetChannel(CHANNEL_COLOR);
	if (!color) return false;	// return some error
	BaseContainer cdata = color->GetData();
	cdata.SetVector(BASECHANNEL_COLOR_EX, this->matArray[i].Kd);

	if (this->matArray[i].useTextures)
	{
		//GePrint ("Material Use texture: " + this->matArray[i].map_Kd);
		cdata.SetString(BASECHANNEL_TEXTURE, this->matArray[i].map_Kd);
		cdata.SetFilename(BASECHANNEL_SUGGESTEDFOLDER, this->pSet.tempPath);
	}
	color->SetData(cdata);
	
	return true;
}

Bool ApplinkImporter::CreateChannelSpecular(long i, BaseMaterial* mat)
{
	if(this->matArray[i].map_Ks != "")
	{
		BaseChannel *color = mat->GetChannel(CHANNEL_SPECULARCOLOR);
		if (!color) return false;

		GeData b;
		mat->GetParameter(DescID(MATERIAL_USE_SPECULARCOLOR), b, DESCFLAGS_GET_0);
		if(!b.GetBool()) mat->SetParameter(DescID(MATERIAL_USE_SPECULARCOLOR), TRUE, DESCFLAGS_SET_0);
		BaseContainer cdata = color->GetData();
		cdata.SetVector(BASECHANNEL_COLOR_EX, this->matArray[i].Ks);

		if (this->matArray[i].useTextures)
		{
			//GePrint ("Material Use texture: " + this->matArray[i].map_Kd);
			cdata.SetString(BASECHANNEL_TEXTURE, this->matArray[i].map_Ks);
			cdata.SetFilename(BASECHANNEL_SUGGESTEDFOLDER, this->pSet.tempPath);
		}
		color->SetData(cdata);
	}
	
	return true;
}

Bool ApplinkImporter::CreateChannelBump(long i, BaseMaterial* mat)
{
	if(this->matArray[i].disp_map != "")
	{
		BaseChannel *color = mat->GetChannel(CHANNEL_BUMP);
		if (!color) return false;

		GeData b;
		mat->GetParameter(DescID(MATERIAL_USE_BUMP), b, DESCFLAGS_GET_0);
		if(!b.GetBool())	mat->SetParameter(DescID(MATERIAL_USE_BUMP), TRUE, DESCFLAGS_SET_0);
		BaseContainer cdata = color->GetData();

		if (this->matArray[i].useTextures)
		{
			//GePrint ("Material Use texture: " + this->matArray[i].map_Kd);
			cdata.SetString(BASECHANNEL_TEXTURE, this->matArray[i].disp_map);
			cdata.SetFilename(BASECHANNEL_SUGGESTEDFOLDER, this->pSet.tempPath);
		}
		color->SetData(cdata);
	}
	
	return true;
}

Bool ApplinkImporter::CreateChannelDisplacement(long i, BaseMaterial* mat)
{
	if(this->matArray[i].disp_map != "")
	{
		BaseChannel *color = mat->GetChannel(CHANNEL_DISPLACEMENT);
		if (!color)	return false;
		
		GeData b;
		mat->GetParameter(DescID(MATERIAL_USE_DISPLACEMENT), b, DESCFLAGS_GET_0);
		if(!b.GetBool())	mat->SetParameter(DescID(MATERIAL_USE_DISPLACEMENT), TRUE, DESCFLAGS_SET_0);
		BaseContainer cdata = color->GetData();
		//GePrint("disp: " + RealToString(this->matArray[i].disp_k));
		mat->SetParameter(DescID(MATERIAL_DISPLACEMENT_STRENGTH), this->matArray[i].disp_k/100, DESCFLAGS_SET_0);

		if (this->matArray[i].useTextures)
		{
			//GePrint ("Material Use texture: " + this->matArray[i].map_Kd);
			cdata.SetString(BASECHANNEL_TEXTURE, this->matArray[i].disp_map);
			cdata.SetFilename(BASECHANNEL_SUGGESTEDFOLDER, this->pSet.tempPath);
		}
		color->SetData(cdata);
	}
	
	return true;
}

Bool ApplinkImporter::CreateChannelNormal(long i, BaseMaterial* mat)
{
	if(this->matArray[i].norm_map != "")
	{
		BaseChannel *color = mat->GetChannel(CHANNEL_NORMAL);
		if (!color)	return false;
		GeData b;
		mat->GetParameter(DescID(MATERIAL_USE_NORMAL), b, DESCFLAGS_GET_0);
		if(!b.GetBool()) mat->SetParameter(DescID(MATERIAL_USE_NORMAL), TRUE, DESCFLAGS_SET_0);
		BaseContainer cdata = color->GetData();

		if (this->matArray[i].useTextures)
		{
			//GePrint ("Material Use texture: " + this->matArray[i].map_Kd);
			cdata.SetString(BASECHANNEL_TEXTURE, this->matArray[i].norm_map);
			cdata.SetFilename(BASECHANNEL_SUGGESTEDFOLDER, this->pSet.tempPath);
		}
		color->SetData(cdata);
	}
	
	return true;
}
