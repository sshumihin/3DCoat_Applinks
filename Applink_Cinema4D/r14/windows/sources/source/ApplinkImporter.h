#ifndef __APPLINKIMPORTER_H__
#define __APPLINKIMPORTER_H__
#include <string>
#include <vector>
#include <c4d.h>

struct AppMaterial
{
	String Name;
	float Ns;
	float d;
	int illum;
	Vector Kd;
	Vector Ka;
	Vector Ks;
	Vector Ke;
	String map_Kd;
	String map_Ks;
	float disp_k;
	String disp_map;
	String norm_map;
	Bool useTextures;
};
// Ties the verticies, normals and texture coordinates in the file together to form an object
struct Face
{
	long vp[4];
	long vt[4];
};
// One object 'group' (lines beginning with 'g ' in the file). 
// This includes a pointer to the faces (to be) loaded from the file, 
// The name of the material and texture associated to the group and some states 
struct Group
{
	char groupName[100];	// Name of the group (not really necessary but looks cool)
	long numGFaces;	// Number of faces
	long numGVertices;	// Number of vertices
	long numGTVertices;	// Number of texture
	Face *faces;	// Pointer to the faces that will be loaded
	int* polyMatIdx;// Array material id by poly
};
struct PreferenceSet
{
	Bool impUV;
	Bool impMat;
	Bool impReplace;
	long impComboMap;
	String tempPath;
};
class ApplinkImporter
{
public:
	ApplinkImporter();
	~ApplinkImporter();
	Bool Execute(BaseDocument* document, BaseContainer* bc);

	PreferenceSet pSet;
	
private:
	Bool createMaterials();
	Bool readMTL();
	Bool insertMaterials();
	Bool CreateChannelColor(long i, BaseMaterial* mat);
	Bool CreateChannelSpecular(long i, BaseMaterial* mat);
	Bool CreateChannelBump(long i, BaseMaterial* mat);
	Bool CreateChannelNormal(long i, BaseMaterial* mat);
	Bool CreateChannelDisplacement(long i, BaseMaterial* mat);

	Bool parseObjFile(char* path);
	Bool gatheringObjDate(char* path);
	Bool createObjects();
	Bool createMaterials(char file[256]);
	BaseTag* GetLastTag(PolygonObject* pObj);
	void InsertTextureTag(PolygonObject* pObj, String mat, String& name);

	std::vector<std::string> Split(const std::string& strValue, const char separator);

	BaseDocument* doc;
	Filename exportFilePath;// export.txt
	Filename texturesFilePath;// textures.txt
	Filename mtlFilePath;// output.mtl
	long numFaces;		// Number of faces
	long numVerticies;	// Number of verticies
	long numTexCoords;	// Number of texture coordinates
	int numGroups;		// The number of groups in the file 

	Vector* verticies;	// Verticies.
	Vector* uvw;	// Texture coordinates
	std::vector<Group> groups;		// Holds the face data, inside groups. See typedef struct Group
	std::vector<AppMaterial> matArray;// materials
};

#endif  // #ifndef __APPLINKIMPORTER_H__