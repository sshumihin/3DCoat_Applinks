#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>

#include <xsi_application.h>
#include <xsi_argument.h>
#include <xsi_comapihandler.h>
#include <xsi_command.h>
#include <xsi_context.h>
#include <xsi_clusterpropertybuilder.h>
#include <xsi_imageclip2.h>
#include <xsi_geometry.h>
#include <xsi_geometryaccessor.h>
#include <xsi_longarray.h>
#include <xsi_material.h>
#include <xsi_materiallibrary.h>
#include <xsi_meshbuilder.h>
#include <xsi_model.h>
#include <xsi_parameter.h>
#include <xsi_polygonmesh.h>
#include <xsi_primitive.h>
#include <xsi_project.h>
#include <xsi_progressbar.h>
#include <xsi_ref.h>
#include <xsi_scene.h>
#include <xsi_selection.h>
#include <xsi_shader.h>
#include <xsi_status.h>
#include <xsi_string.h>
#include <xsi_utils.h>
#include <xsi_x3dobject.h>
#include <xsi_vector3.h>

using namespace XSI;
using namespace std;

extern Application app;
extern ProgressBar bar;

extern CustomProperty Coat3DToolProp();
extern Parameter Get3DCoatParam( const CString& in_strName );

CStringArray MtllibNames;
CStringArray GroupNames;
CStringArray MaterialNames;
CDoubleArray VertPositions;
CDoubleArray VertNormals;
CDoubleArray VertTextures;
CLongArray FaceMats;
CLongArray FacesVp;
CLongArray FacesVn;
CLongArray FacesVt;
CLongArray FaceVertexCnt;
CLongArray ArraysSize;

struct Materials
{
	CString Name;// newmtl
	float Ns;//Ns 100.000
	float d;//d 1.00000
	LONG illum;//illum 2
	CDoubleArray Kd;//Kd 1.00000 1.00000 1.00000
	CDoubleArray Ka;//Ka 0.00000 0.00000 0.00000
	CDoubleArray Ks;//Ks 1.00000 1.00000 1.00000
	CDoubleArray Ke;//Ke 0.00000e+0 0.00000e+0 0.00000e+0
	CString map_Kd;//map_Kd F:\Work\3DCoat_Tool\Obj\objCoat_cube1_auv_color.tga
	CString map_Ks;//map_Ks F:\Work\3DCoat_Tool\Obj\objCoat_cube1_auv_specular.bmp
	float displacement;
	CString map_disp;
	CString map_normal;
};

vector <Materials> vmatCollection;

CStatus ReadMTL();
CStatus BuildObjects();
CStatus BuildMaterials();
LONG minValue(CFloatArray&);
void CreateShaders(Material&, ULONG);
bool IsShaderConnected(Shader&, const CString, CRefArray&);
bool IsShaderConnected(Material&, const CString, CRefArray&);
ImageClip2 inAllClips(CRefArray&, CString& );
CValue ConvertComma(CString&);


CValue ConvertComma(CString& str)
{
	double value;
	value = atof( str.GetAsciiString() );	

	return CValue(value);
}
LONG minValue(CLongArray& in_array)
{
	int length = in_array.GetCount();
	LONG min = in_array[0];

     for(int i = 1; i<length; i++)
     {
		if(in_array[i] < min)
		{
			min = in_array[i];
		}
	}
	return min;
}
CStatus BuildMaterials()
{
	//app.LogMessage(L"bImpMat: " + CString(Get3DCoatParam(L"bImpMat").GetValue()));
	if(Get3DCoatParam(L"bImpMat").GetValue())
	{
		//bool bNewMat = Get3DCoatParam(L"bImpNewMat").GetValue();

		Scene oScene = app.GetActiveProject().GetActiveScene();
		CRefArray oMatlibs = oScene.GetMaterialLibraries();

		bar.PutStatusText(L"Build materials...");
		//ULONG size = vmatCollection.size();
		//app.LogMessage(L"vmatCollection size: " + CString(size));
		for ( int n=0; n < vmatCollection.size(); n++ )
		{
			Material myMat;
			for ( LONG i=0; i < oMatlibs.GetCount(); i++ )
			{
				MaterialLibrary oMatlib = oMatlibs[i];
				CRefArray nbMaterials = oMatlib.GetItems();

				for ( LONG j=0; j < nbMaterials.GetCount(); j++ )
				{
					Material oMat = nbMaterials[j];
					if(vmatCollection[n].Name == oMat.GetName() )
					{
						CRefArray oObjects = oMat.GetAllImageClips();
						oObjects += oMat.GetAllShaders();

						CValueArray args(1);
						args[0] = oObjects;
						CValue retval;
						app.ExecuteCommand( L"DeleteObj", args, retval ) ;
						myMat = oMat;
						break;
					}
				}


				if(myMat.IsValid())
				{
					CreateShaders(myMat, n);
				}
				else
				{
					MaterialLibrary matlib( oScene.GetActiveMaterialLibrary( ) );
					myMat = matlib.CreateMaterial( L"Phong", vmatCollection[n].Name);
					CreateShaders(myMat, n);
				}
			}
			if(bar.IsCancelPressed()) return CStatus::False;
		}
	}
	return CStatus::OK;
}

ImageClip2 inAllClips(CRefArray& allClips, CString& imgPath)
{
	ImageClip2 clip;
	for(int i = 0; i < allClips.GetCount(); i++)
	{
		ImageClip2 gclip(allClips[i]);
		//app.LogMessage(L"gclip filename: " + gclip.GetFileName());
		//app.LogMessage(L"imgPath : " + imgPath);
		if(imgPath == gclip.GetFileName())
		{
			clip = gclip;
			break;
		}
	}
	return clip;
}
void CreateShaders(Material& myMat, ULONG n)
{
	Project proj = app.GetActiveProject();
	Scene scn = proj.GetActiveScene();
	CRefArray allClips = scn.GetImageClips();
	CRefArray oOwners = myMat.GetUsedBy();

	Parameter surf = myMat.GetParameters().GetItem(L"surface");
	CRef newSource;
	CRef prevSource;
	surf.ConnectFromProgID(L"Softimage.material-phong.1.0", prevSource, newSource);
	Shader shader = newSource;

	Shader sh = myMat.GetShaders( ).GetItem( 0 );
	Parameter Kd = sh.GetParameters( ).GetItem( L"diffuse" );
	Kd.PutParameterValue( L"red", vmatCollection[n].Kd[0]);
	Kd.PutParameterValue( L"green", vmatCollection[n].Kd[1] );
	Kd.PutParameterValue( L"blue", vmatCollection[n].Kd[2] );
	//app.LogMessage(L"GetParam diffuse" +  CString(Kd.GetParameterValue(L"red")));

	Parameter Ka = sh.GetParameters( ).GetItem( L"ambient" );
	Ka.PutParameterValue( L"red", vmatCollection[n].Ka[0]);
	Ka.PutParameterValue( L"green", vmatCollection[n].Ka[1] );
	Ka.PutParameterValue( L"blue", vmatCollection[n].Ka[2] );
	//app.LogMessage(L"GetParam ambient" + CString( Ka.GetParameterValue(L"red")));

	Parameter Ks = sh.GetParameters( ).GetItem( L"specular" );
	Ks.PutParameterValue( L"red", vmatCollection[n].Ks[0]);
	Ks.PutParameterValue( L"green", vmatCollection[n].Ks[1] );
	Ks.PutParameterValue( L"blue", vmatCollection[n].Ks[2] );

	Parameter Ns = sh.GetParameters( ).GetItem( L"shiny" );
	Ns.PutValue( vmatCollection[n].Ns);

	CString strImagePath = vmatCollection[n].map_Kd;
	if(strImagePath != L"")
	{
		CValue valImageClip2;
		ImageClip2 preClip = inAllClips(allClips, strImagePath);
		if(preClip.IsValid())
		{
			valImageClip2 = preClip;
		}
		else
		{
			CValueArray args(1);
			args[0] = CValue(strImagePath);
			app.ExecuteCommand( L"CreateImageClip", args, valImageClip2 );
		}

		//CStringArray imageName = strImagePath.Split(CUtils::Slash());
		//app.LogMessage(L"valImageClip2: " + valImageClip2.GetAsText());
		ImageClip2 clip( valImageClip2 );

		Kd.ConnectFromPreset(L"Image", siTextureShaderFamily, prevSource, newSource);
		
		Shader imageshader = newSource;

		// Connect the imageclip to phong.diffuse
		CStatus st = Kd.Connect( imageshader, prevSource ) ;

		// Connect image.tex to imageclip
		Parameter tex = imageshader.GetParameter(L"tex");
		st = tex.Connect( clip, prevSource );

		Parameter oUV = imageshader.GetParameter(L"tspace_id");

		for(int i=0; i < oOwners.GetCount(); i++)
		{
			X3DObject oOwner;
			
			CString ClassID = oOwners[i].GetClassIDName();

			if (ClassID == L"X3DObject")
			{
				oOwner = oOwners[i];
			}
			else
			{
				Cluster oCluster = oOwners[i];
				oOwner = oCluster.GetParent3DObject();
			}
			
			Geometry oGeometry(oOwner.GetActivePrimitive().GetGeometry()) ;
			CGeometryAccessor ga = PolygonMesh(oGeometry).GetGeometryAccessor();
			CRefArray sUVs = ga.GetUVs();

			//app.LogMessage(L"Owner name " + oOwner.GetName());
			oUV.PutInstanceValue(oOwner, sUVs[0]);
		}
	}

	strImagePath = vmatCollection[n].map_Ks;
	if(strImagePath != L"")
	{
		CValue valImageClip2;
		ImageClip2 preClip = inAllClips(allClips, strImagePath);
		if(preClip.IsValid())
		{
			valImageClip2 = preClip;
		}
		else
		{
			CValueArray args(1);
			args[0] = CValue(strImagePath);
			app.ExecuteCommand( L"CreateImageClip", args, valImageClip2 );
		}
		//CStringArray imageName = strImagePath.Split(CUtils::Slash());
		ImageClip2 clip( valImageClip2 );
		CRef newSource;
		CRef prevSource;
		Ks.ConnectFromPreset(L"Image", siTextureShaderFamily, prevSource, newSource);
		
		Shader imageshader = newSource;

		// Connect the imageclip to phong.diffuse
		CStatus st = Ks.Connect( imageshader, prevSource ) ;

		Parameter Kr = sh.GetParameters( ).GetItem( L"reflectivity" );
		st = Kr.Connect( imageshader, prevSource );

		// Connect image.tex to imageclip
		Parameter tex = imageshader.GetParameter(L"tex");
		st = tex.Connect( clip, prevSource );

		Parameter oUV = imageshader.GetParameter(L"tspace_id");

		for(int i=0; i < oOwners.GetCount(); i++)
		{
			X3DObject oOwner;
			
			CString ClassID = oOwners[i].GetClassIDName();

			if (ClassID == L"X3DObject")
			{
				oOwner = oOwners[i];
			}
			else
			{
				Cluster oCluster = oOwners[i];
				oOwner = oCluster.GetParent3DObject();
			}
			
			Geometry oGeometry(oOwner.GetActivePrimitive().GetGeometry()) ;
			CGeometryAccessor ga = PolygonMesh(oGeometry).GetGeometryAccessor();
			CRefArray sUVs = ga.GetUVs();

			//app.LogMessage(L"Owner name " + oOwner.GetName());
			oUV.PutInstanceValue(oOwner, sUVs[0]);
		}
	}
	
	CValue swmap = Get3DCoatParam(L"swMap").GetValue();
	if( swmap.GetAsText() == L"1")
	{
		strImagePath = vmatCollection[n].map_disp;
		if(strImagePath != L"")
		{
			//Parameter disp = myMat.GetParameters().GetItem(L"displacement");
			Parameter disp = myMat.GetParameters().GetItem(L"normal");

			CValue valImageClip2;
			ImageClip2 preClip = inAllClips(allClips, strImagePath);
			if(preClip.IsValid())
			{
				valImageClip2 = preClip;
			}
			else
			{
				CValueArray args(1);
				args[0] = CValue(strImagePath);
				app.ExecuteCommand( L"CreateImageClip", args, valImageClip2 );
			}
			ImageClip2 clip( valImageClip2 );

			CRef newSource;
			CRef prevSource;
			CStatus st = disp.ConnectFromProgID(L"Softimage.sib_zbump.1.0", prevSource, newSource);
			Shader bump = newSource;
			
			Parameter input = bump.GetParameter(L"input");
			st = input.ConnectFromPreset(L"Image", siTextureShaderFamily, prevSource, newSource);
			
			Shader imageshader = newSource;

			// Connect image.tex to imageclip
			Parameter tex = imageshader.GetParameter(L"tex");
			st = tex.Connect( clip, prevSource );

			Parameter oUV = imageshader.GetParameter(L"tspace_id");
			
			for(int i=0; i < oOwners.GetCount(); i++)
			{
				X3DObject oOwner;
				
				CString ClassID = oOwners[i].GetClassIDName();

				if (ClassID == L"X3DObject")
				{
					oOwner = oOwners[i];
				}
				else
				{
					Cluster oCluster = oOwners[i];
					oOwner = oCluster.GetParent3DObject();
				}
				
				Geometry oGeometry(oOwner.GetActivePrimitive().GetGeometry()) ;
				CGeometryAccessor ga = PolygonMesh(oGeometry).GetGeometryAccessor();
				CRefArray sUVs = ga.GetUVs();

				//app.LogMessage(L"Owner name " + oOwner.GetName());
				oUV.PutInstanceValue(oOwner, sUVs[0]);
			}
		}
	}

	if(swmap.GetAsText() == L"0")
	{
		strImagePath = vmatCollection[n].map_normal;
		if(strImagePath != L"")
		{
			Parameter normal = myMat.GetParameters().GetItem(L"normal");

			CValue valImageClip2;
			ImageClip2 preClip = inAllClips(allClips, strImagePath);
			if(preClip.IsValid())
			{
				valImageClip2 = preClip;
			}
			else
			{
				CValueArray args(1);
				args[0] = CValue(strImagePath);
				app.ExecuteCommand( L"CreateImageClip", args, valImageClip2 );
			}

			ImageClip2 clip( valImageClip2 );

			CRef newSource;
			CRef prevSource;
			CStatus st = normal.ConnectFromProgID(L"Softimage.XSINormalMap3.1.0", prevSource, newSource);
			Shader nnormal = newSource;
			
			nnormal.PutParameterValue(L"UnbiasNormalMap", false);
			Parameter map = nnormal.GetParameters().GetItem(L"Map");

			st = map.ConnectFromProgID(L"Softimage.sib_color_combine.1.0", prevSource, newSource);
			Shader combine = newSource;

			Parameter red = combine.GetParameters().GetItem(L"red");
			st = red.ConnectFromProgID(L"Softimage.sib_channel_picker.1.0", prevSource, newSource);
			Shader redp = newSource;
			redp.PutParameterValue(L"channel_rgba", LONG(1));
			Parameter rinput = redp.GetParameters().GetItem(L"input");
			st = rinput.ConnectFromPreset(L"Image", siTextureShaderFamily, prevSource, newSource);
			Shader imageshader = newSource;

			Parameter green = combine.GetParameters().GetItem(L"green");
			st = green.ConnectFromProgID(L"Softimage.sib_channel_picker.1.0", prevSource, newSource);
			Shader greenp = newSource;
			greenp.PutParameterValue(L"invert", true);
			greenp.PutParameterValue(L"channel_rgba", LONG(2));
			Parameter ginput = greenp.GetParameters().GetItem(L"input");
			st = ginput.Connect( imageshader, prevSource );

			Parameter blue = combine.GetParameters().GetItem(L"blue");
			st = blue.ConnectFromProgID(L"Softimage.sib_channel_picker.1.0", prevSource, newSource);
			Shader bluep = newSource;
			bluep.PutParameterValue(L"channel_rgba", LONG(3));
			Parameter binput = bluep.GetParameters().GetItem(L"input");
			st = binput.Connect( imageshader, prevSource );

			// Connect image.tex to imageclip
			Parameter tex = imageshader.GetParameter(L"tex");
			st = tex.Connect( clip, imageshader );


			Parameter oUV = nnormal.GetParameter(L"tspaceid");
			Parameter oTang = nnormal.GetParameter(L"tangents");
			
			for(int i=0; i < oOwners.GetCount(); i++)
			{
				X3DObject oOwner;
				
				CString ClassID = oOwners[i].GetClassIDName();

				if (ClassID == L"X3DObject")
				{
					oOwner = oOwners[i];
				}
				else
				{
					Cluster oCluster = oOwners[i];
					oOwner = oCluster.GetParent3DObject();
				}
				
				Geometry oGeometry(oOwner.GetActivePrimitive().GetGeometry()) ;
				CGeometryAccessor ga = PolygonMesh(oGeometry).GetGeometryAccessor();
				CRefArray sUVs = ga.GetUVs();

				//app.LogMessage(L"Owner name " + oOwner.GetName());
				oUV.PutInstanceValue(oOwner, sUVs[0]);

				//CClusterPropertyBuilder cpBuilder = PolygonMesh(oGeometry).GetClusterPropertyBuilder();
				// add a vertex color property on a complete cluster
				//ClusterProperty cp = cpBuilder.AddVertexColor();

				//CRefArray cav = ga.GetVertexColors();
				//oTang.PutInstanceValue(oOwner, cav[0]);

				//CreateVertexColorSupport("ColorAtVertices", "Tangents", "sphere", null);
				CValueArray args(3);
				args[0] = CString(L"ColorAtVertices");
				args[1] = CString(L"Tangents");
				args[2] = oOwner;
				CValue retval ;
				app.ExecuteCommand( L"CreateVertexColorSupport", args, retval);
				
				//ChangeVertexColorDatatype("sphere.polymsh.cls.Texture_Coordinates_AUTO.Tangents", 1, null);
				CValueArray args1(2);
				args1[0] = retval;
				args1[1] = LONG(1);
				CValue retval1 ;
				app.ExecuteCommand( L"ChangeVertexColorDatatype", args1, retval1);

				ClusterProperty cp(retval);
				//ApplyOp("TangentOp2_cpp", "sphere.polymsh.cls.Texture_Coordinates_AUTO.Tangents;sphere.polymsh.cls.Texture_Coordinates_AUTO.Texture_Projection", siUnspecified, siPersistentOperation, null, 2);
				
				CRefArray cav = ga.GetVertexColors();

				//app.LogMessage(L"cav[0]: " + cav[0].GetAsText());
				//app.LogMessage(L"sUVs[0]: " + sUVs[0].GetAsText());

				CValueArray args2(6);
				args2[0] = CString(L"TangentOp2_cpp");
				args2[1] = cav[0].GetAsText() + L";" + sUVs[0].GetAsText();
				args2[2] = (LONG)siUnspecified;
				args2[3] = (LONG)siPersistentOperation;
				//args2[4] = CString(L"");
				args2[5] = LONG(2);
				CValue retval2 ;
				app.ExecuteCommand( L"ApplyOp", args2, retval2);

				oTang.PutInstanceValue(oOwner, cav[0]);

			}
		}
	}
}

CStatus BuildObjects()
{
	Model root = app.GetActiveSceneRoot();
	Scene oScene = app.GetActiveProject().GetActiveScene();
	CRefArray oMatlibs = oScene.GetMaterialLibraries();
	Selection sel = app.GetSelection();
	sel.Clear();
	CRefArray selArray;

	bar.PutStatusText(L"Create objects...");
	for(int i=0; i<GroupNames.GetCount(); i++ )
	{
		if(Get3DCoatParam(L"bReplace").GetValue())	
		{
			X3DObject oRoot = app.GetActiveSceneRoot();
			X3DObject prevObj = oRoot.FindChild(GroupNames[i], siPolyMeshType, CStringArray());
			if(prevObj.IsValid())
			{
				CValueArray args( 2 ) ;
				args[0] = prevObj ;
				args[1] = L"BRANCH";
				CValue retval ;
				app.ExecuteCommand( L"SelectObj", args, retval ) ;

				CValueArray args2( 1 );
				args2[0] = prevObj ;
				app.ExecuteCommand( L"DeleteObj", args2, retval ) ;
			}
		}
		// use empty mesh to create the imported mesh
		X3DObject xobj;
		root.AddPrimitive( L"EmptyPolygonMesh", GroupNames[i], xobj );
		
		// get a mesh builder from the newly created geometry
		Primitive prim = xobj.GetActivePrimitive();
		PolygonMesh mesh = prim.GetGeometry();
		if (!mesh.IsValid()) return CStatus::False;

		CMeshBuilder meshBuilder = mesh.GetMeshBuilder();

		//app.LogMessage(L"array: " + ArraysSize.GetAsText());
		//app.LogMessage(L"Vertex count: " + CString(ArraysSize[(i+1)*8]));

		int start = ArraysSize[i*8];
		int end = ArraysSize[(i+1)*8];
		CDoubleArray newVertPos;

		for(int v = start; v < end; v++)
		{
			newVertPos.Add(VertPositions[v]);
		}

		//app.LogMessage(L"Vertices: " + newVertPos.GetAsText());
		// add vertices to mesh
		CStatus st = meshBuilder.AddVertices( newVertPos.GetCount()/3, newVertPos.GetArray() ); 
		st.AssertSucceeded( L"AddVertices" );
		
		//app.LogMessage(L"Face count: " + CString(ArraysSize[(i+1)*8+7]));
		int FaceCnt = ArraysSize[(i+1)*8+7];
		CLongArray PolyVertexCounts;
		CLongArray PolyMaterials;
		start = ArraysSize[i*8+7];

		for(int p = start; p < FaceCnt; p++)
		{
			PolyVertexCounts.Add(FaceVertexCnt[p]);
			PolyMaterials.Add(FaceMats[p]);
		}
		
		//app.LogMessage(L"PolyVertexCounts: " + PolyVertexCounts.GetAsText());
		//app.LogMessage(L"Vindex count: " + CString(ArraysSize[(i+1)*8+4]));
		CLongArray VtxIndices;
		start = ArraysSize[i*8+4];
		end = ArraysSize[(i+1)*8+4];

		for(int idx = start; idx < end; idx++)
		{
			VtxIndices.Add(FacesVp[idx] - 1 - ArraysSize[i*8]/3);
		}
		
		//app.LogMessage(L"VtxIndices: " + VtxIndices.GetAsText());
		//app.LogMessage(L"FaceCnt: " + CString(FaceCnt));

		// add polygons to mesh
		st = meshBuilder.AddPolygons( PolyVertexCounts.GetCount(), PolyVertexCounts.GetArray(), VtxIndices.GetArray());
		st.AssertSucceeded( L"AddPolygons" );
		
		// Generate the new cube with undo disabled
		CMeshBuilder::CErrorDescriptor err = meshBuilder.Build( false );
        if (err==CStatus::Fail)
        {
            app.LogMessage( L"Error generating the mesh: " + err.GetDescription() );
        }

		CClusterPropertyBuilder cpb = mesh.GetClusterPropertyBuilder( );
		ClusterProperty cp;

		if(Get3DCoatParam(L"bImpUV").GetValue())
		{
			// add UV properties on the grid
			cp = cpb.AddUV( );

			CFloatArray PolyUVWs;
			start = ArraysSize[i*8+2];
			end = ArraysSize[(i+1)*8+2];

			//app.LogMessage(L"PolyUVs start" + CString(start));
			//app.LogMessage(L"PolyUVs end" + CString(end));

			for(int p = start; p < end; p++)
			{
				PolyUVWs.Add(float(VertTextures[p]));
			}
			
			//app.LogMessage(L"PolyUVWs: " + PolyUVWs.GetAsText());
			//app.LogMessage(L"PolyUVWs count: " + CString(PolyUVWs.GetCount()));

			CLongArray PolyUVCounts;
			start = ArraysSize[i*8+6];
			end = ArraysSize[(i+1)*8+6];

			//app.LogMessage(L"PolyUVCounts start" + CString(start));
			//app.LogMessage(L"PolyUVCounts end" + CString(end));
						
			for(int idx = start; idx < end; idx++)
			{
				PolyUVCounts.Add(FacesVt[idx] - 1);
			}
			//app.LogMessage(L"PolyUVCounts: " + PolyUVCounts.GetAsText());
			//app.LogMessage(L"PolyUVCounts cnt: " + CString(PolyUVCounts.GetCount()));

			LONG minIdx = minValue(PolyUVCounts);

			for(int idx = 0; idx < PolyUVCounts.GetCount(); idx++)
			{
				PolyUVCounts[idx] = PolyUVCounts[idx] - minIdx;
			}
			//fix node UV
			CFloatArray fixPolyUVWs;
			for(int p = 0; p < PolyUVCounts.GetCount(); p++)
			{
				fixPolyUVWs.Add(PolyUVWs[PolyUVCounts[p]*3]);
				fixPolyUVWs.Add(PolyUVWs[PolyUVCounts[p]*3+1]);
				fixPolyUVWs.Add(PolyUVWs[PolyUVCounts[p]*3+2]);

				//app.LogMessage(L"PolyNormCounts[p]: " + CString(PolyNormCounts[p]));
				//app.LogMessage(L"PolyNormals[]: " + CString(PolyNormals[PolyNormCounts[p]*3]));
				//app.LogMessage(L"fixPolyNormals[p]: " + CString(fixPolyNormals[p*3]));
			}

			for(int idx = 0; idx < PolyUVCounts.GetCount(); idx++)
			{
				PolyUVCounts[idx] = idx;
			}

			//app.LogMessage(L"fixPolyUVWs: " + fixPolyUVWs.GetAsText());
			//app.LogMessage(L"PolyUVWs: " + PolyUVWs.GetAsText());
			//app.LogMessage(L"PolyUVWs count: " + CString(PolyUVWs.GetCount()));
			cp.SetValues( PolyUVCounts.GetArray(), fixPolyUVWs.GetArray(), PolyUVCounts.GetCount() );

		}

		if(Get3DCoatParam(L"bImpNorm").GetValue())
		{

		   // add UserNormals properties on the grid
			cp = cpb.AddUserNormal();

			CFloatArray PolyNormals;
			start = ArraysSize[i*8+1];
			end = ArraysSize[(i+1)*8+1];

			for(int p = start; p < end; p++)
			{
				PolyNormals.Add(float(VertNormals[p]));
			}
			
			//app.LogMessage(L"PolyNormals: " + PolyNormals.GetAsText());
			//app.LogMessage(L"PolyNormals count: " + CString(PolyNormals.GetCount()));
			CLongArray PolyNormCounts;
			start = ArraysSize[i*8+5];
			end = ArraysSize[(i+1)*8+5];

			for(int idx = start; idx < end; idx++)
			{
				//PolyNormCounts.Add(FacesVn[idx] - 1 - ArraysSize[i*8+5]);
				PolyNormCounts.Add(FacesVn[idx] - 1);
			}
			
			LONG minIdx = minValue(PolyNormCounts);
			//app.LogMessage(L"PolyNormCounts: " + PolyNormCounts.GetAsText());
			//app.LogMessage(L"minIdx: " + CString(minIdx));

			for(int idx = 0; idx < PolyNormCounts.GetCount(); idx++)
			{
				PolyNormCounts[idx] = PolyNormCounts[idx] - minIdx;
				//PolyNormCounts[idx] = idx;
			}

			//fix node normals
			CFloatArray fixPolyNormals;
			for(int p = 0; p < PolyNormCounts.GetCount(); p++)
			{
				fixPolyNormals.Add(PolyNormals[PolyNormCounts[p]*3]);
				fixPolyNormals.Add(PolyNormals[PolyNormCounts[p]*3+1]);
				fixPolyNormals.Add(PolyNormals[PolyNormCounts[p]*3+2]);

				//app.LogMessage(L"PolyNormCounts[p]: " + CString(PolyNormCounts[p]));
				//app.LogMessage(L"PolyNormals[]: " + CString(PolyNormals[PolyNormCounts[p]*3]));
				//app.LogMessage(L"fixPolyNormals[p]: " + CString(fixPolyNormals[p*3]));
			}

			for(int idx = 0; idx < PolyNormCounts.GetCount(); idx++)
			{
				//PolyNormCounts[idx] = PolyNormCounts[idx] - minIdx;
				PolyNormCounts[idx] = idx;
			}

			//app.LogMessage(L"fixPolyNormals: " + fixPolyNormals.GetAsText());
			//app.LogMessage(L"PolyNormals count: " + CString(PolyNormals.GetCount()));
			//app.LogMessage(L"PolyNormCounts: " + PolyNormCounts.GetAsText());
			//app.LogMessage(L"PolyNormCounts cnt: " + CString(PolyNormCounts.GetCount()));

			cp.SetValues( PolyNormCounts.GetArray(), fixPolyNormals.GetArray(), PolyNormCounts.GetCount() );
		
		}

		if(Get3DCoatParam(L"bImpMat").GetValue())
		{

			vector <CLongArray> vpolyClusters;
			vpolyClusters.clear();
			vpolyClusters.resize(MaterialNames.GetCount());
			
			//app.LogMessage(L"PolyMaterials: " + PolyMaterials.GetAsText());

			for(LONG pm=0; pm < PolyMaterials.GetCount(); pm++)
			{
				LONG temp = PolyMaterials[pm];
				vpolyClusters[temp].Add(pm);
			}

			for(ULONG lm=0; lm < vpolyClusters.size(); lm++)
			{
				Material myMat;
				if(vpolyClusters[lm].GetCount() > 0)
				{
					//app.LogMessage(L"PolyClusters: " + vpolyClusters[lm].GetAsText());

					for ( LONG m=0; m < oMatlibs.GetCount(); m++ )
					{
						MaterialLibrary oMatlib = oMatlibs[m];
						CRefArray nbMaterials = oMatlib.GetItems();

						for ( LONG n=0; n < nbMaterials.GetCount(); n++ )
						{
							Material oMat = nbMaterials[n];
							if(oMat.GetName() == MaterialNames[lm])
							{
								myMat = oMat;
								break;
							}

						}
					}
					if(!myMat.IsValid())
					{
						MaterialLibrary matlib( oScene.GetActiveMaterialLibrary( ) );
						myMat = matlib.CreateMaterial( L"Phong", MaterialNames[lm]);
					}
					
					meshBuilder.SetPolygonsMaterial( myMat, vpolyClusters[lm].GetCount(), vpolyClusters[lm].GetArray() );				
				}
			}
		}

		selArray.Add(xobj);
		if(bar.IsCancelPressed()) return CStatus::False;
	}

	sel.SetAsText(selArray.GetAsText());

	return CStatus::OK;	
}
CStatus ReadMTL()
{
	if(Get3DCoatParam(L"bImpMat").GetValue() && MtllibNames.GetCount() > 0)
	{
		//app.LogMessage(L"MtllibNames: " + MtllibNames.GetCount());
		for(int i=0; i<MtllibNames.GetCount();i++)
		{
			ifstream mfrImportMTL;
			mfrImportMTL.open(MtllibNames[i].GetAsciiString());
			
			//app.LogMessage(L"MtllibNames: " + MtllibNames[i]);
			if(mfrImportMTL.good())
			{
				bar.PutStatusText(L"Read materials...");
				while(!mfrImportMTL.eof()) // To get you all the lines.
				{
					string row;
					getline(mfrImportMTL, row);
					CString crow = row.c_str();
					//app.LogMessage(L"MtllibNames crow: " + crow);

					if(crow != L"")
					{
						CStringArray subrow = crow.Split(" ");
						
						if(subrow[0] == L"newmtl")
						{
							Materials mat;
							mat.Name = subrow[1];
							vmatCollection.push_back(mat);
						}
						else if(subrow[0] == L"Ns")
						{
							vmatCollection[vmatCollection.size()-1].Ns = ConvertComma(subrow[1]);
						}
						else if(subrow[0] == L"d")
						{
							vmatCollection[vmatCollection.size()-1].d = ConvertComma(subrow[1]);
						}
						else if(subrow[0] == L"illum")
						{
							vmatCollection[vmatCollection.size()-1].illum = ConvertComma(subrow[1]);
						}
						else if(subrow[0] == L"Kd")
						{
							vmatCollection[vmatCollection.size()-1].Kd.Add(ConvertComma(subrow[1]));
							vmatCollection[vmatCollection.size()-1].Kd.Add(ConvertComma(subrow[2]));
							vmatCollection[vmatCollection.size()-1].Kd.Add(ConvertComma(subrow[3]));
						}
						else if(subrow[0] == L"Ka")
						{
							vmatCollection[vmatCollection.size()-1].Ka.Add(ConvertComma(subrow[1]));
							vmatCollection[vmatCollection.size()-1].Ka.Add(ConvertComma(subrow[2]));
							vmatCollection[vmatCollection.size()-1].Ka.Add(ConvertComma(subrow[3]));
						}
						else if(subrow[0] == L"Ks")
						{
							vmatCollection[vmatCollection.size()-1].Ks.Add(ConvertComma(subrow[1]));
							vmatCollection[vmatCollection.size()-1].Ks.Add(ConvertComma(subrow[2]));
							vmatCollection[vmatCollection.size()-1].Ks.Add(ConvertComma(subrow[3]));
						}
						else if(subrow[0] == L"Ke")
						{
							vmatCollection[vmatCollection.size()-1].Ke.Add(ConvertComma(subrow[1]));
							vmatCollection[vmatCollection.size()-1].Ke.Add(ConvertComma(subrow[2]));
							vmatCollection[vmatCollection.size()-1].Ke.Add(ConvertComma(subrow[3]));
						}
						else if(subrow[0] == L"map_Kd")
						{
							ULONG space = crow.FindString(L" ");
							CString subrow2 = crow.GetSubString(space+1, crow.Length()-1);
							vmatCollection[vmatCollection.size()-1].map_Kd = subrow2;
						}
						else if(subrow[0] == L"map_Ks")
						{
							ULONG space = crow.FindString(L" ");
							CString subrow2 = crow.GetSubString(space+1, crow.Length()-1);
							vmatCollection[vmatCollection.size()-1].map_Ks = subrow2;
						}
					}
					if(bar.IsCancelPressed()) return CStatus::False;
				}
			}

			//app.LogMessage(L"Obj Name: "+ GroupNames[0]);
			//app.LogMessage(L"V: "+ VertPositions.GetAsText());
			//app.LogMessage(L"Vt: "+ VertTextures.GetAsText());
			//app.LogMessage(L"Vn: "+ VertNormals.GetAsText());
			//app.LogMessage(L"FaceMats: "+ FaceMats.GetAsText());
			//app.LogMessage(L"FaceV: "+ FacesVp.GetAsText());
			//app.LogMessage(L"FaceVt: "+ FacesVt.GetAsText());
			//app.LogMessage(L"FaceVn: "+ FacesVn.GetAsText());

			mfrImportMTL.close();
		}

		CString texturePath = CUtils::BuildPath(Get3DCoatParam(L"coatLocation").GetValue(), L"textures.txt");
		ifstream mfrImportMaps;
		mfrImportMaps.open(texturePath.GetAsciiString());
		
		//app.LogMessage(L"MtllibNames: " + MtllibNames[i]);
		if(mfrImportMaps.good())
		{
			bar.PutStatusText(L"Read textures.txt");
			int IdxMat;
			while(!mfrImportMaps.eof()) // To get you all the lines.
			{
				string row;
				getline(mfrImportMaps, row);
				CString crow = row.c_str();
				//app.LogMessage(L"MtllibNames crow: " + crow);

				if(crow != L"")
				{					
					CStringArray subrow = crow.Split(" ");
					
					for(ULONG i=0; i < vmatCollection.size(); i++)
					{
						if(subrow[0] == vmatCollection[i].Name)
						{
							IdxMat = i;
							break;
						}
					}

					if(subrow[0] == L"displacement")
					{
						vmatCollection[IdxMat].displacement = CValue(subrow[1]);
						getline(mfrImportMaps, row);
						crow = row.c_str();
						vmatCollection[IdxMat].map_disp = crow;
					}

					if(subrow[0] == L"normalmap")
					{
						getline(mfrImportMaps, row);
						crow = row.c_str();
						vmatCollection[IdxMat].map_normal = crow;
					}

					if (subrow[0] == L"color")
					{
						getline(mfrImportMaps, row);
						crow = row.c_str();
						vmatCollection[IdxMat].map_Kd = crow;
					}
				}
				if(bar.IsCancelPressed()) return CStatus::False;
			}
		}
	}
	return CStatus::OK;
}

SICALLBACK Coat3DImport_Init( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	Command oCmd;
	oCmd = ctxt.GetSource();
	
	// Specify that the command returns a value
	oCmd.EnableReturnValue(true);
	//Application app;
	app.LogMessage(L"Coat3DImport_Init!");

	// Add arguments to the command
	ArgumentArray oArgs;
	oArgs = oCmd.GetArguments();

	oArgs.Add(L"objPath", (CString)siString);
	oArgs.Add(L"bReplace",(CValue)siBool);

	return CStatus::OK;
}

SICALLBACK Coat3DImport_Execute( CRef& in_ctxt )
{
	// Unpack the command argument values
	Context ctxt( in_ctxt );
	CValueArray args = ctxt.GetAttribute(L"Arguments");

	CString strOut = args[0];

//////////////////////////////////////////////////////Parser obj
	//bar.PutMaximum( 50000 );
	//bar.PutStep( 100 );
	//bar.PutVisible( true );
	//bar.PutCaption( in_strCaption );

	bar.PutVisible(true);

	ArraysSize.Clear();
	MtllibNames.Clear();
	GroupNames.Clear();
	MaterialNames.Clear();
	VertPositions.Clear();
	VertNormals.Clear();
	VertTextures.Clear();
	FaceMats.Clear();
	FacesVp.Clear();
	FacesVn.Clear();
	FacesVt.Clear();
	FaceVertexCnt.Clear();
	vmatCollection.clear();

	CString LastMats;

	ifstream mfrImportObj;
	mfrImportObj.open(strOut.GetAsciiString());

	if(mfrImportObj.good())
	{
		bar.PutStatusText(L"Read output.obj...");
		while(!mfrImportObj.eof()) // To get you all the lines.
		{
			//int cntV, cntVn, cntVt, cntF;
			string row;
			getline(mfrImportObj, row);
			CString crow = row.c_str();
			//app.LogMessage(L"row: " + crow);

			if(crow != L"")
			{
				CStringArray subrow = crow.Split(" ");
				
				if(subrow[0] == L"mtllib")
				{
					ULONG npos = strOut.ReverseFindString(CUtils::Slash());
					CString substr = strOut.GetSubString(0, npos);
					MtllibNames.Add(CUtils::BuildPath(substr, subrow[1]));
				}
				else if(subrow[0] == L"g")
				{					
					GroupNames.Add(subrow[1]);

					ArraysSize.Add(VertPositions.GetCount());
					ArraysSize.Add(VertNormals.GetCount());
					ArraysSize.Add(VertTextures.GetCount());
					ArraysSize.Add(MaterialNames.GetCount());
					ArraysSize.Add(FacesVp.GetCount());
					ArraysSize.Add(FacesVn.GetCount());
					ArraysSize.Add(FacesVt.GetCount());
					ArraysSize.Add(FaceVertexCnt.GetCount());

					//cntV = cntVn = cntVt = cntF = 0;
				}
				else if(subrow[0] == L"v")
				{
					VertPositions.Add(ConvertComma(subrow[1]));
					VertPositions.Add(ConvertComma(subrow[2]));
					VertPositions.Add(ConvertComma(subrow[3]));
					//app.LogMessage(L"v: " + subrow[1] +L", "+ subrow[2] +L", "+ subrow[3]);
				}
				else if(subrow[0] == L"vn")
				{
					VertNormals.Add(ConvertComma(subrow[1]));
					VertNormals.Add(ConvertComma(subrow[2]));
					VertNormals.Add(ConvertComma(subrow[3]));
					//app.LogMessage(L"vn: " + subrow[1]);
				}
				else if(subrow[0] == L"vt")
				{
					VertTextures.Add(ConvertComma(subrow[1]));
					VertTextures.Add(ConvertComma(subrow[2]));
					VertTextures.Add(0.0f);
					//app.LogMessage(L"vt: " + subrow[1]);
				}
				else if(subrow[0] == L"usemtl")
				{
					bool inMats = false;
					for(int i=0; i<MaterialNames.GetCount(); i++)
					{
						if(MaterialNames[i] == subrow[1])
						{
							inMats = true;
							break;
						}
					}

					if(!inMats)
					{
						MaterialNames.Add(CString(subrow[1]));
					}
					LastMats = subrow[1];
				}
				else if(subrow[0] == L"f")
				{
					for(int i=0; i<MaterialNames.GetCount(); i++)
					{
						if(MaterialNames[i] == LastMats)
						{
							FaceMats.Add(i);
							break;
						}
					}

					FaceVertexCnt.Add(subrow.GetCount()-1);

					for(int i=1; i<subrow.GetCount(); i++)
					{
						CStringArray fsubrow = subrow[i].Split(L"/");

						FacesVp.Add(CValue(fsubrow[0]));

						if(fsubrow[1] != L"")
						{
							FacesVt.Add(CValue(fsubrow[1]));
						}

						if(fsubrow[2] != L"")
						{
							FacesVn.Add(CValue(fsubrow[2]));
						}
					}
				}
			}
		}
		ArraysSize.Add(VertPositions.GetCount());
		ArraysSize.Add(VertNormals.GetCount());
		ArraysSize.Add(VertTextures.GetCount());
		ArraysSize.Add(MaterialNames.GetCount());
		ArraysSize.Add(FacesVp.GetCount());
		ArraysSize.Add(FacesVn.GetCount());
		ArraysSize.Add(FacesVt.GetCount());
		ArraysSize.Add(FaceVertexCnt.GetCount());

		if(bar.IsCancelPressed()) return CStatus::False;
	}

	//app.LogMessage(L"Obj Name: "+ GroupNames[0]);
	//app.LogMessage(L"V: "+ VertPositions.GetAsText());
	//app.LogMessage(L"Vt: "+ VertTextures.GetAsText());
	//app.LogMessage(L"Vt count: "+ CString (VertTextures.GetCount()));
	//app.LogMessage(L"Vn: "+ VertNormals.GetAsText());
	//app.LogMessage(L"FaceMats: "+ FaceMats.GetAsText());
	//app.LogMessage(L"FaceV: "+ FacesVp.GetAsText());
	//app.LogMessage(L"FaceVt: "+ FacesVt.GetAsText());
	//app.LogMessage(L"FaceVt count: "+ CString(FacesVt.GetCount()));
	//app.LogMessage(L"FaceVn: "+ FacesVn.GetAsText());
	//app.LogMessage(L"FaceVertexCnt: "+ FaceVertexCnt.GetAsText());
	//app.LogMessage(L"ArraysSize: "+ ArraysSize.GetAsText());

	mfrImportObj.close();

	ReadMTL();

	BuildObjects();

	BuildMaterials();

	CString texturePath = CUtils::BuildPath(Get3DCoatParam(L"coatLocation").GetValue(), L"textures.txt");
	CString exportPath = CUtils::BuildPath(Get3DCoatParam(L"coatLocation").GetValue(), L"export.txt");
	if(remove(texturePath.GetAsciiString()) == -1)
	{
		app.LogMessage(L"File " + texturePath + L" can not removed!", siWarningMsg);
	}

	if(remove(exportPath.GetAsciiString()) == -1)
	{
		app.LogMessage(L"File " + exportPath + L" can not removed!", siWarningMsg);
	}

	bar.PutVisible(false);

	app.LogMessage(L"Import done!");
	return CStatus::OK;
}