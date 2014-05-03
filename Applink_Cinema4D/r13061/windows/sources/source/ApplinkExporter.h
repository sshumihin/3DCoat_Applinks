#ifndef __APPLINKEXPORTER_H__
#define __APPLINKEXPORTER_H__

#include <c4d.h>
#include "ge_dynamicarray.h"

#define PID_APPLINK_EXPORTER  1022832


class ApplinkExporter
{
private:
	struct ExportObject
	{
		ExportObject(void);
		~ExportObject(void);

		GeDynamicArray<Vector>  Vp, Vt;
		//GeDynamicArray<SVector> Vn;
		/*! Fv - (0,1,2,1,4,3,,), Fvt - (0,1,2,3,4,,,), Fvn - (0,1,2,3,4,,,), Fpidx - (3,4,4,3,4,,,)number vertices in polygon*/
		GeDynamicArray<LONG> Fv, Fvt, Fvn, Fpvnb;
		/*! number polygon vertices per object*/
		LONG pVertexCount;
		GeDynamicArray<LONG> pmatidxArray;
		GeDynamicArray<BaseMaterial*> tempMats;
	};

public:
	Bool Execute(BaseDocument* document, BaseContainer* bc);
	void WriteString(String string, BaseFile* file);
	void WriteEndLine(BaseFile* file);
	void WriteVertexPositions(BaseFile* objfile, ExportObject& mObject, LONG vcnt);
	void WriteUVWTag(BaseFile* objfile, ExportObject& mObject, LONG pcnt, const CPolygon* padr);
	//void WriteNormals(BaseFile* objfile, ExportObject& mObject, LONG pcnt, const CPolygon* padr);
	void WriteExportFile(BaseContainer* bc, PolygonObject* ob, BaseFile* objfile, ExportObject& mObject, LONG vcnt, LONG pcnt);
	Bool WriteMatsFile(BaseDocument* document, BaseContainer* bc);
	BaseList2D* getParameterLink(GeListNode& node, LONG paramID, LONG instanceOf);
	String getParameterString(BaseTag& tag, LONG paramID);
	LONG getParameterLong(BaseMaterial& mat, LONG paramID);
	Real getParameterReal(C4DAtom& atom, LONG paramID, Real preset);
	Vector getParameterVector(C4DAtom& atom, LONG paramID);
	Filename getParameterFilename(C4DAtom& atom, LONG paramID);
	void ExportChannel(BaseDocument* document, BaseFile* file, BaseMaterial& material, LONG shaderId, LONG colorId, const String s);

	GeDynamicArray<BaseMaterial*> materialArray;

private:
	LONG PVertexLength(ExportObject& mObject);
	String mapType(LONG id);
	/*! number vertex position lines*/
	LONG vpcnt;
	/*! number texture vertex lines*/
	LONG vtcnt;
	/*! number normal vertex lines*/
	//LONG vncnt;
	BaseMaterial* matDefault;
};

#endif  // #ifndef __APPLINKEXPORTER_H__