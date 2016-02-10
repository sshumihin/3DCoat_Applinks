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
		/*! Fv - (0,1,2,1,4,3,,), Fvt - (0,1,2,3,4,,,), Fvn - (0,1,2,3,4,,,), Fpidx - (3,4,4,3,4,,,)number vertices in polygon*/
		GeDynamicArray<Int32> Fv, Fvt, Fvn, Fpvnb;
		/*! number polygon vertices per object*/
		Int32 pVertexCount;
		GeDynamicArray<Int32> pmatidxArray;
		GeDynamicArray<BaseMaterial*> tempMats;
	};

public:
	Bool Execute(BaseDocument* document, BaseContainer* bc);
	void WriteString(String string, BaseFile* file);
	void WriteEndLine(BaseFile* file);
	void WriteVertexPositions(BaseFile* objfile, ExportObject& mObject, Int32 vcnt);
	void WriteUVWTag(BaseFile* objfile, ExportObject& mObject, Int32 pcnt, const CPolygon* padr);
	void WriteExportFile(BaseContainer* bc, PolygonObject* ob, BaseFile* objfile, ExportObject& mObject, Int32 vcnt, Int32 pcnt);
	Bool WriteMatsFile(BaseDocument* document, BaseContainer* bc);
	BaseList2D* getParameterLink(GeListNode& node, Int32 paramID, Int32 instanceOf);
	String getParameterString(BaseTag& tag, Int32 paramID);
	Int32 getParameterLong(BaseMaterial& mat, Int32 paramID);
	Float getParameterReal(C4DAtom& atom, Int32 paramID, Float preset);
	Vector getParameterVector(C4DAtom& atom, Int32 paramID);
	Filename getParameterFilename(C4DAtom& atom, Int32 paramID);
	void ExportChannel(BaseDocument* document, BaseFile* file, BaseMaterial& material, Int32 shaderId, Int32 colorId, const String s);

	GeDynamicArray<BaseMaterial*> materialArray;

private:
	Int32 PVertexLength(ExportObject& mObject);
	String mapType(Int32 id);
	/*! number vertex position lines*/
	Int32 vpcnt;
	/*! number texture vertex lines*/
	Int32 vtcnt;
	BaseMaterial* matDefault;
};

#endif  // #ifndef __APPLINKEXPORTER_H__