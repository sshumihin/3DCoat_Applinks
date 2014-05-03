#include <math.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <xsi_application.h>
#include <xsi_argument.h>
#include <xsi_comapihandler.h>
#include <xsi_command.h>
#include <xsi_context.h>
#include <xsi_imageclip2.h>
#include <xsi_geometryaccessor.h>
#include <xsi_kinematics.h>
#include <xsi_material.h>
#include <xsi_menu.h>
#include <xsi_model.h>
#include <xsi_parameter.h>
#include <xsi_pluginregistrar.h>
#include <xsi_polygonmesh.h>
#include <xsi_ppglayout.h>
#include <xsi_primitive.h>
#include <xsi_project.h>
#include <xsi_progressbar.h>
#include <xsi_ref.h>
#include <xsi_scene.h>
#include <xsi_selection.h>
#include <xsi_shader.h>
#include <xsi_source.h>
#include <xsi_status.h>
#include <xsi_string.h>
#include <xsi_transformation.h>
#include <xsi_uitoolkit.h>
#include <xsi_utils.h>
#include <xsi_x3dobject.h>
#include <xsi_value.h>
#include <xsi_vector3.h>

using namespace XSI;

extern Application app;
extern ProgressBar bar;

int gV; //vertices position count
int gVprev; //vertices position count
int gVn; //vertices normal count
int gVnPrev; //vertices normal count
int gVt; //vertices texture count
int gVtPrev; //vertices texture count
int gObjCnt;// object count
CLongArray g_aNodeIsland;// new array PolyVt

extern CustomProperty Coat3DToolProp();
extern Parameter Get3DCoatParam( const CString& in_strName );

void OutputVertices( std::ofstream&, CGeometryAccessor&, X3DObject& );
void OutputArrayPositions( std::ofstream& , CDoubleArray&, MATH::CTransformation& );
void OutputArray( std::ofstream&, CFloatArray&, int, const CString& );
SICALLBACK OutputMaterials( Selection&);
void OutputPolygonComponents( std::ofstream&, CGeometryAccessor&);
void OutputClusterPropertyValues( std::ofstream&, CGeometryAccessor&, CRefArray& );
void OutputHeader( std::ofstream&);
SICALLBACK OutputImportTxt();
CString FormatNumber(double&);
CString FormatNumber(float&);

void OutputArray( std::ofstream& in_mfw, CFloatArray& in_array, int in_nDims, const CString& in_str )
{	
	if (!in_nDims) return;	

	LONG nVals = in_array.GetCount()/in_nDims;
	double s;
	
	bar.PutMinimum(0);
	bar.PutMaximum(nVals);
	for ( LONG i=0, nCurrent=0; i<nVals; i++ )
	{
		bar.PutValue(i);
		CString str;
		s = in_array[nCurrent];
		str += FormatNumber(s);

		for (LONG j = 1; j < in_nDims; j++)
		{
			str += L" ";
			s = in_array[nCurrent+j];
			str += FormatNumber(s);
		}

		in_mfw << in_str.GetAsciiString();
		in_mfw << str.GetAsciiString();
		in_mfw << "\n";
		
		nCurrent += in_nDims;
		gVn++;
		//bar.Increment();
	}
}

void OutputArrayPositions( std::ofstream& in_mfw, CDoubleArray& in_array, MATH::CTransformation& localTransform)
{	
	MATH::CVector3 position;
	MATH::CVector3 worldPosition;
	double x, y, z;
	
	bar.PutMinimum(0);
	bar.PutMaximum(in_array.GetCount()/3);
	for ( LONG i=0; i<in_array.GetCount(); i += 3 )
	{
		bar.PutValue(i);
		CString str;
		position.PutX(in_array[i]);
		position.PutY(in_array[i+1]);
		position.PutZ(in_array[i+2]);

		worldPosition = MATH::MapObjectPositionToWorldSpace(localTransform, position);

		x = worldPosition.GetX();
		y = worldPosition.GetY();
		z = worldPosition.GetZ();

		str = L"v " + FormatNumber(x) + L" " + FormatNumber(y) + L" " + FormatNumber(z);

		in_mfw << str.GetAsciiString();
		in_mfw << "\n";
		
		gV++;
		//bar.Increment();
	}
}

CString FormatNumber(double& s)
{
	char buffer[32];
	::sprintf_s( buffer, 32, "%.6f", s );
	return CString(buffer);
}

CString FormatNumber(float& s)
{
	char buffer[32];
	::sprintf_s( buffer, 32, "%.6f", s );
	return CString(buffer);
}


void OutputVertices( std::ofstream& in_mfw, CGeometryAccessor& in_ga, X3DObject& xobj)
{
	bar.PutStatusText( L"Vertices..." );
	
	// polygon vertex positions
	CDoubleArray vtxPosArray;
	CStatus st = in_ga.GetVertexPositions(vtxPosArray);
	st.AssertSucceeded( L"GetVertexPositions" );

	MATH::CTransformation localTransformation = xobj.GetKinematics().GetLocal().GetTransform();
	CString string = L"#begin " +  CString(in_ga.GetVertexCount()) + L" vertices\n";
	in_mfw << string.GetAsciiString();
	OutputArrayPositions( in_mfw, vtxPosArray, localTransformation );
	string = L"#end " +  CString(in_ga.GetVertexCount()) + L" vertices\n";
	in_mfw << string.GetAsciiString();
	in_mfw << "\n";

	if(Get3DCoatParam(L"bExpUV").GetValue())
	{
		// uv props: siClusterPropertyUVType
		CRefArray uvProps = in_ga.GetUVs();
		if(uvProps.GetCount() > 0)
		{
			OutputClusterPropertyValues( in_mfw, in_ga, uvProps );
		}
	}
	if(Get3DCoatParam(L"bExpNorm").GetValue())
	{

		// polygon node normals
		CFloatArray nodeArray;
		st = in_ga.GetNodeNormals(nodeArray);
		st.AssertSucceeded( L"GetNodeNormals" );

		//app.LogMessage(L"NormalArray: " + nodeArray.GetAsText());
		string = L"#begin " +  CString(in_ga.GetNodeCount()) + L" normals\n";
		in_mfw << string.GetAsciiString();
		OutputArray( in_mfw, nodeArray, 3,  L"vn " );
		string = L"#end " +  CString(in_ga.GetNodeCount()) + L" normals\n";
		in_mfw << string.GetAsciiString();
		in_mfw << "\n";
	}
}



SICALLBACK OutputMaterials( Selection& in_sel )
{
	// prepare the mtl file
	Project prj = app.GetActiveProject();
	Scene scn = prj.GetActiveScene();
	CString tmpLocation = Get3DCoatParam( L"tempLocation" ).GetValue();

	ULONG npos = tmpLocation.ReverseFindString(L".");
	CString substr = tmpLocation.GetSubString(0, npos+1);
	CString strOut = substr + L"mtl";	

	//app.LogMessage(L"strOut:" + strOut);

	std::ofstream matfw;
	matfw.open(strOut.GetAsciiString(), std::ios_base::out | std::ios_base::trunc);
	if (matfw.is_open())
	{
		CRefArray tempMats;

		for(int i=0; i< in_sel.GetCount(); i++)
		{
			X3DObject xobj(in_sel.GetItem(i));

			// Get a geometry accessor from the selected object	
			Primitive prim = xobj.GetActivePrimitive();
			PolygonMesh mesh = prim.GetGeometry();
			if (!mesh.IsValid()) return CStatus::False;

			CGeometryAccessor ga = mesh.GetGeometryAccessor();

			// get the material objects used by the mesh
			CRefArray materials = ga.GetMaterials();

			for (LONG n=0; n < materials.GetCount(); n++)
			{
				bar.PutStatusText( L"materials" );

				Material mat(materials[n]);
				bool inMats = false;
				//app.LogMessage(CString(n) +L" : "+ CString(i)+ L" :" + mat.GetName());

				for(int m = 0; m < tempMats.GetCount(); m++)
				{
					Material tmat(tempMats[m]);
					if(mat.GetName() == tmat.GetName())
					{
						inMats = true;
						break;
					}
				}

				//app.LogMessage(CString(inMats));

				if(!inMats)
				{
					CString string = L"newmtl " + mat.GetName() + L"\n";
					matfw << string.GetAsciiString();

					CRefArray lShaders = mat.GetShaders();

					for( long j=0; j < lShaders.GetCount(); j++ )
					{
						Shader lShader(lShaders[ j ]);

						//app.LogMessage(L"ClassID: " + lShader.GetClassID());
						//app.LogMessage(L"GetFullName: " + lShader.GetFullName());
						//app.LogMessage(L"GetName: " + lShader.GetName());

						Parameter lTarget = lShader.GetShaderParameterTargets( L"out" )[ 0 ];
						if ( lTarget.GetName().IsEqualNoCase( L"surface" ) )
						{
							//app.LogMessage( L"Found surface shader: " + lShader.GetFullName() + L", Class: " + lShader.GetClassIDName() + L", Type: " + lShader.GetType() );

							if ( lShader.GetProgID() == L"Softimage.material-phong.1.0" )
							{
								//app.LogMessage(L"GetProgID: " + lShader.GetProgID());

								float r, g, b, a;

								lShader.GetColorParameterValue(L"ambient", r, g, b, a );
								CString ka = L"Ka " + FormatNumber(r) + L" " + FormatNumber(g) + L" " + FormatNumber(b);
								lShader.GetColorParameterValue(L"diffuse", r, g, b, a );
								CString kd = L"Kd " + FormatNumber(r) + L" " + FormatNumber(g) + L" " + FormatNumber(b);
								lShader.GetColorParameterValue(L"specular", r, g, b, a );
								CString ks = L"Ks " + FormatNumber(r) + L" " + FormatNumber(g) + L" " + FormatNumber(b);
								float ns = lShader.GetParameterValue(L"shiny");
								float d = 1.0f;
								CValue illum = 2;

								matfw << ka.GetAsciiString();
								matfw << "\n";
								matfw << kd.GetAsciiString();
								matfw << "\n";
								matfw << ks.GetAsciiString();
								matfw << "\n";
								matfw << "Ns ";
								matfw << FormatNumber(ns).GetAsciiString();
								matfw << "\n";
								matfw << "d ";
								matfw << FormatNumber(d).GetAsciiString();
								matfw << "\n";
								matfw << "illum ";
								matfw << illum.GetAsText().GetAsciiString();
								matfw << "\n";


								CRefArray lImages = lShader.GetShaders();
								for( long k=0; k < lImages.GetCount(); k++ )
								{
									Shader lImage(lImages[ k ]);
									Parameter lTargetIm = lImage.GetShaderParameterTargets( L"out" )[ 0 ];
									if ( lTargetIm.GetName().IsEqualNoCase( L"diffuse" ) )
									{
										//app.LogMessage( L"Found image shader: " + lImage.GetFullName() + L", Class: " + lImage.GetClassIDName() + L", Type: " + lImage.GetType() );
										
										CRefArray lTextures = lImage.GetImageClips();

										for( long l=0; l < lTextures.GetCount(); l++ )
										{
											ImageClip2 lTexture(lTextures[ l ]);
											//app.LogMessage(L"texture GetName: " + lTexture.GetName());
											
											//app.LogMessage(L"texture GetFullName: " + lTexture.GetFullName());

											Parameter lTargetTex = lTexture.GetShaderParameterTargets()[ 0 ];
											if ( lTargetTex.GetName().IsEqualNoCase( L"tex" ) )
											{
												//app.LogMessage( L"Found texture shader: " + lTexture.GetFullName() + L", Class: " + lTexture.GetClassIDName() + L", Type: " + lTexture.GetType() );
												//app.LogMessage(L"texture GetFileName: " + lTexture.GetFileName());
												matfw << "map_Kd ";
												matfw << lTexture.GetFileName().GetAsciiString();
												matfw << "\n";
											}
										}
									}
									else if( lTargetIm.GetName().IsEqualNoCase( L"specular" ))
									{
										CRefArray lTextures = lImage.GetImageClips();

										for( long l=0; l < lTextures.GetCount(); l++ )
										{
											ImageClip2 lTexture(lTextures[ l ]);
											//app.LogMessage(L"texture GetName: " + lTexture.GetName());
											
											//app.LogMessage(L"texture GetFullName: " + lTexture.GetFullName());

											Parameter lTargetTex = lTexture.GetShaderParameterTargets()[ 0 ];
											if ( lTargetTex.GetName().IsEqualNoCase( L"tex" ) )
											{
												//app.LogMessage( L"Found texture shader: " + lTexture.GetFullName() + L", Class: " + lTexture.GetClassIDName() + L", Type: " + lTexture.GetType() );
												//app.LogMessage(L"texture GetFileName: " + lTexture.GetFileName());
												matfw << "map_Ks ";
												matfw << lTexture.GetFileName().GetAsciiString();
												matfw << "\n";
											}
										}
									}
								}
							}
						}
					}

					////Parameter pSurf = mat.GetParameter(L"surface");
					//Parameter pSurf = mat.GetParameter(L"surface");

					//CComAPIHandler pCOM(pSurf.GetRef());
					//CRef surfSrc = pCOM.GetProperty(L"Source");
					//
					////CRef surfSrc = pSurf.GetSource();

					//app.LogMessage(L"surface source: " + surfSrc.GetAsText());
					//
					//Shader op(surfSrc);

					//CComAPIHandler sCOM(op.GetRef());
					//CValue sname = sCOM.GetProperty(L"Name");

					//app.LogMessage(L"surface shader name: " + op.GetName());
					//app.LogMessage(L"surface shader name: " + sname.GetAsText());
					////app.LogMessage(L"surface shader id: " + op.GetClassID());

					//CRefArray shaders = mat.GetShaders();
		/*			for(int j=0; j < shaders.GetCount(); j++)
					{
						Shader shader(shaders[i]);
						if ( shader.ProgID == "Softimage.material-phong.1.0" )
						{
							float r;
							float g;
							float b;
							float a;

							CValue ka = shader.GetColorParameterValue(L"ambient", r, g, b, a );
							CValue kd = shader.GetColorParameterValue(L"diffuse", r, g, b, a );
							CValue ks = shader.GetColorParameterValue(L"specular", r, g, b, a );
							CValue ns = shader.GetParameterValue(L"shiny");
							CValue d = L"1.00000";
							CValue illum = 2;
							CValue map_kd = shader;
						}
					}*/
					tempMats.Add(mat);
					matfw << "\n";
					matfw << "\n";
				}
			}		
		}
		matfw.close();
	}
	else
	{
		matfw.close();
		LONG button;
		UIToolkit uitoolkit = app.GetUIToolkit();
		CString str = L"File "+strOut+" is not be writed!\nMake sure that the user permissions allow write file.";
		uitoolkit.MsgBox(str, siMsgCritical, L"Applink Message", button);
		app.LogMessage(str, siErrorMsg);
		return CStatus::False;
	}

	return CStatus::OK;
}

void OutputClusterPropertyValues( std::ofstream& in_mfw,	CGeometryAccessor& in_ga, CRefArray& in_array)
{	
	LONG nVals = in_array.GetCount();
	double s;

	ClusterProperty cp(in_array[0]);

	CFloatArray valueArray;
	CBitArray flags;		
	cp.GetValues( valueArray, flags );

	LONG nValueSize = cp.GetValueSize();
	
	bar.PutValue(0);
	bar.PutStatusText(L"Optimize UV...");
	// polygon node indices
	CLongArray polyNodeIdxArray;
	CStatus st = in_ga.GetNodeIndices(polyNodeIdxArray);
	st.AssertSucceeded( L"GetNodeIndices");

	//app.LogMessage(L"polyNodeIdxArray: " + polyNodeIdxArray.GetAsText());
	g_aNodeIsland = polyNodeIdxArray;
	std::vector<float> newValueArray;
	newValueArray.clear();

	for ( LONG j=0; j < polyNodeIdxArray.GetCount(); j++)
	{	
		float u = valueArray[polyNodeIdxArray[j]*3];
		float v = valueArray[polyNodeIdxArray[j]*3+1];
		//app.LogMessage(L"u = " + CString(u)+ "; v = "+ CString(v));
		LONG nmb = 0;
		bool bIs = false;
		for ( LONG k = 0; k < newValueArray.size(); k += 3 )
		{
			if(fabs(newValueArray.at(k) - u) < 0.000002 && fabs(newValueArray.at(k+1) - v) < 0.000002)
			{
				nmb = k/3;
				bIs = true;
				break;
				//app.LogMessage(L"Yarr!: g_aNodeIsland["+ CString(j)+"] = "+ CString(k/3));
			}
		}
		
		if(bIs)
		{
			g_aNodeIsland[j] = nmb;
			//app.LogMessage(L"Yarr!: g_aNodeIsland["+ CString(j)+"] = "+ CString(nmb));
		}
		else
		{
			newValueArray.push_back(u);
			newValueArray.push_back(v);
			newValueArray.push_back(0.0f);
			g_aNodeIsland[j] = (LONG)(newValueArray.size()/3-1);
			//app.LogMessage(L"g_aNodeIsland["+ CString(j)+"] = "+ CString(newValueArray.size()/3-1));
		}
	}

	in_mfw << "#begin ";
	in_mfw << CString(newValueArray.size()/nValueSize).GetAsciiString();
	in_mfw << "\n";
	
	bar.PutStatusText(L"Clusters...");
	bar.PutMinimum(0);
	bar.PutMaximum((LONG)newValueArray.size()/nValueSize);
	for ( LONG j=0; j < newValueArray.size(); j += nValueSize)
	{
		//bar.PutValue(j);
		s = newValueArray.at(j);
		CString strValues = FormatNumber(s);
		
		for ( LONG k=1; k<nValueSize; k++ )
		{
			s = newValueArray.at(j+k);
			strValues += L" " + FormatNumber(s);
		}
		
		in_mfw << "vt ";
		in_mfw << strValues.GetAsciiString();
		in_mfw << "\n";

		gVt++;
		bar.Increment();
	}
	CString string = L"#end " +  CString(newValueArray.size()/nValueSize) + L"\n";
	in_mfw << string.GetAsciiString();
	in_mfw << "\n";
}

void OutputPolygonComponents( std::ofstream& in_mfw, CGeometryAccessor& in_ga)
{
	// polygon node indices
	CLongArray polyNodeIdxArray;
	CStatus st = in_ga.GetNodeIndices(polyNodeIdxArray);
	st.AssertSucceeded( L"GetNodeIndices");
	
	//app.LogMessage(L"NodeIdx: " + polyNodeIdxArray.GetAsText());

	CLongArray polySizeArray;
	st = in_ga.GetPolygonVerticesCount(polySizeArray);
	st.AssertSucceeded( L"GetPolygonVerticesCount" );

	//app.LogMessage(L"GetPolygonVerticesCount: " + polySizeArray.GetAsText());

	// polygon vertex indices
	CLongArray polyVtxIdxArray;
	st = in_ga.GetVertexIndices(polyVtxIdxArray);
	st.AssertSucceeded( L"GetVertexIndices" );

	CString string = L"#begin " +  CString(in_ga.GetPolygonCount()) + L" faces\n";
	in_mfw << string.GetAsciiString();

	// get the material objects used by the mesh
	CRefArray materials = in_ga.GetMaterials();
	CRefArray uvProps = in_ga.GetUVs();

	// get the material indices used by each polygon
	CLongArray pmIndices;
	in_ga.GetPolygonMaterialIndices(pmIndices);

	CString prevMat = L"";
	CString curMat = L"";

	bar.PutMinimum(0);
	bar.PutMaximum(polySizeArray.GetCount());
	bar.PutStatusText(L"Faces...");

	bool bUV = (bool)Get3DCoatParam(L"bExpUV").GetValue();
	bool bNrm = (bool)Get3DCoatParam(L"bExpNorm").GetValue();
	bool bMtl = (bool)Get3DCoatParam(L"bExpMat").GetValue();
	bool bUVCnt	= (uvProps.GetCount() > 0)?true:false;
	CString strVertices;

	for (LONG i=0, offset=0; i < polySizeArray.GetCount(); i++)
	{	
		bar.PutValue(i);
		strVertices = "";
		strVertices += CString(polyVtxIdxArray[offset] + 1 + gVprev);// vertices idx[0]
		if(bUV && bUVCnt)
		{
			strVertices += L"/";
			//strVertices += CString(polyNodeIdxArray[offset] + 1 + gVtPrev);// texture nodes idx[0]
			strVertices += CString(g_aNodeIsland[offset] + 1 + gVtPrev);// texture nodes idx[0]
			
			if(bNrm)
			{
				strVertices += L"/";
				strVertices += CString(polyNodeIdxArray[offset] + 1 + gVnPrev);// normal vertices idx[0]
			}
		}
		else if(bNrm)
		{
			strVertices += L"//";
			strVertices += CString(polyNodeIdxArray[offset] + 1 + gVnPrev);// normal vertices idx[0]
		}

		for (LONG j=1; j<polySizeArray[i]; j++)
		{
			strVertices += L" ";
			strVertices += CString(polyVtxIdxArray[offset+j] + 1 + gVprev);// vertices idx[12]
			if(bUV && bUVCnt)
			{
				strVertices += L"/";
				//strVertices += CString(polyNodeIdxArray[offset+j] + 1 + gVtPrev);// texture nodes idx[12]
				strVertices += CString(g_aNodeIsland[offset+j] + 1 + gVtPrev);// texture nodes idx[0]
				
				if(bNrm)
				{
					strVertices += L"/";
					strVertices += CString(polyNodeIdxArray[offset+j] + 1 + gVnPrev);// normal vertices idx[12]
				}
			}
			else if(bNrm)
			{
				strVertices += L"//";
				strVertices += CString(polyNodeIdxArray[offset+j] + 1 + gVnPrev);// normal vertices idx[12]
			}
		}

		if(bMtl)
		{
			Material mat(materials[ pmIndices[i] ]);
			curMat = mat.GetName();
			if(curMat != prevMat)
			{
				in_mfw << "usemtl ";
				in_mfw << curMat.GetAsciiString();
				in_mfw << "\n";
				prevMat = curMat;
			}
		}

		in_mfw << "f ";
		in_mfw << strVertices.GetAsciiString();
		in_mfw << "\n";
		
		offset += polySizeArray[i];

		//bar.Increment();
	}
	string = L"#end " +  CString(in_ga.GetPolygonCount()) + L" faces\n";
	in_mfw << string.GetAsciiString();
	in_mfw << "\n";
}

void OutputHeader( std::ofstream& in_mfw)
{
//# XSI Wavefront OBJ Export v3.0
//# File Created: Mon Oct 11 15:58:17 2010
//# XSI Version: 9.1.91.0

	in_mfw << "#Wavefront OBJ Export for 3D-Coat";
	in_mfw << "\n";
	in_mfw << "# ";
	in_mfw << "\n";
	in_mfw << "#XSI Version: ";
	in_mfw << app.GetVersion().GetAsciiString();
	in_mfw << "\n";

	if(Get3DCoatParam( L"bExpMat" ).GetValue())
	{
		//mtllib object.mtl	
		Project prj = app.GetActiveProject();
		Scene scn = prj.GetActiveScene();
		CString string = L"mtllib " + scn.GetName() + L".mtl";
		in_mfw << string.GetAsciiString();
		in_mfw << "\n";
	}

	in_mfw << "\n";
}


SICALLBACK OutputImportTxt()
{
	// prepare the import.txt file
	CString strOut = CUtils::BuildPath(Get3DCoatParam( L"coatLocation" ).GetValue(), L"import.txt");

	std::ofstream tfw;
	tfw.open(strOut.GetAsciiString(), std::ios_base::out | std::ios_base::trunc );
	if (tfw.is_open())
	{
		CString objTmp = Get3DCoatParam( L"tempLocation" ).GetValue();
		tfw << objTmp.GetAsciiString();
		tfw << "\n";

		CString objOut = objTmp.GetSubString(0, objTmp.ReverseFindString(CUtils::Slash())+1);
		objOut += L"output.obj";

		tfw << objOut.GetAsciiString();
		tfw << "\n";

		int typePaint = Get3DCoatParam( L"typePaint" ).GetValue();

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

		CString strPaint = L"[";
		switch (typePaint)
		{
			case 0:
				strPaint += L"ppp";
				break;
			case 1:
				strPaint += L"mv";
				break;
			case 2:
				strPaint += L"ptex";
				break;
			case 3:
				strPaint += L"uv";
				break;
			case 4:
				strPaint += L"ref";
				break;
			case 5:
				strPaint += L"retopo";
				break;
			case 6:
				strPaint += L"vox";
				break;
			case 7:
				strPaint += L"alpha";
				break;
			case 8:
				strPaint += L"prim";
				break;
			case 9:
				strPaint += L"curv";
				break;
			case 10:
				strPaint += L"autopo";
				break;
		}

		strPaint += L"]";

		tfw << strPaint.GetAsciiString();

		bool bExpSkipIm = Get3DCoatParam( L"bExpSkipImp" ).GetValue();
		if(bExpSkipIm)
		{
			tfw << "\n";
			tfw << "[SkipImport]";
		}

		bool bExpSkipExp = Get3DCoatParam( L"bExpSkipExp" ).GetValue();
		if(bExpSkipExp)
		{
			tfw << "\n";
			tfw << "[SkipExport]";
		}

		tfw.close();
	}
	else
	{
		tfw.close();
		LONG button;
		UIToolkit uitoolkit = app.GetUIToolkit();
		CString str = L"File "+strOut+" is not be writed!\nMake sure that the user permissions allow write file.";
		uitoolkit.MsgBox(str, siMsgCritical, L"Applink Message", button);
		app.LogMessage(str, siErrorMsg);
		return CStatus::False;
	}

	return CStatus::OK;
}

SICALLBACK Coat3DExport_Init( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	Command oCmd;
	oCmd = ctxt.GetSource();
	
	// Specify that the command returns a value
	oCmd.EnableReturnValue(true);
	//Application app;
	app.LogMessage(L"Coat3DExport_Init!");

	// Add arguments to the command
	ArgumentArray oArgs;
	oArgs = oCmd.GetArguments();

	oArgs.Add(L"tempLocation", (CString)siString);
	oArgs.Add(L"coatLocation",(CString)siString);
	oArgs.Add(L"typePaint",(CValue)siUInt);
	//oArgs.Add(L"bCopyTexE",(CValue)siBool);
	oArgs.Add(L"bExpMat",(CValue)siBool);
	oArgs.Add(L"bExpUV",(CValue)siBool);
	oArgs.Add(L"bExpNorm",(CValue)siBool);

	return CStatus::OK;
}

SICALLBACK Coat3DExport_Execute( CRef& in_ctxt )
{
	// Unpack the command argument values
	Context ctxt( in_ctxt );
	CValueArray args = ctxt.GetAttribute(L"Arguments");
	CString string;

	// A 3d object with a mesh geometry must be selected
	Selection selection(app.GetSelection());

	bool isPolymesh = true;
	for(int i =0; i< selection.GetCount(); i++)
	{
		X3DObject obj(selection[i]);
		//app.LogMessage(L"obj.IsA(siPolygonMeshID): " + CString(obj.GetType()));
		if(obj.GetType() != L"polymsh" )
		{
			isPolymesh = false;
			break;
		}
	}

	if (selection.GetCount() > 0 && isPolymesh)

	{
		gV = 0; gVn = 0; gVt = 0;
		gVprev = 0; gVnPrev = 0; gVtPrev = 0;
		// prepare the output text file
		CString strOut = Get3DCoatParam( L"tempLocation" ).GetValue();
		
		std::ofstream mfw;
		mfw.open(strOut.GetAsciiString(), std::ios_base::out | std::ios_base::trunc);
		if (mfw.is_open())
		{
			bar.PutMaximum( selection.GetCount() );
			bar.PutStep( 1 );
			bar.PutVisible( true );		

			OutputHeader( mfw);				
			// output the data
			for (int i=0; i < selection.GetCount(); i++)
			{
				gObjCnt = i;
				gVprev = gV;
				gVtPrev = gVt;
				gVnPrev = gVn;
				X3DObject xobj(selection.GetItem(i));

				bar.PutValue(i);
				bar.PutCaption( L"Exporting " + xobj.GetName());

				mfw << "\n";
				mfw << "# Hierarchy (from self to top father)\n";
				string = L"g " + xobj.GetName() + L"\n";
				mfw << string.GetAsciiString();
				mfw << "\n";

				// Get a geometry accessor from the selected object	
				Primitive prim = xobj.GetActivePrimitive();
				PolygonMesh mesh = prim.GetGeometry();
				if (!mesh.IsValid()) return CStatus::False;

				CGeometryAccessor ga = mesh.GetGeometryAccessor();

				OutputVertices( mfw, ga, xobj );
				if (bar.IsCancelPressed()) return CStatus::False;
				OutputPolygonComponents( mfw, ga );
				if (bar.IsCancelPressed()) return CStatus::False;
				//bar.Increment();		
			}
		}
		else
		{
			mfw.close();
			LONG button;
			UIToolkit uitoolkit = app.GetUIToolkit();
			CString str = L"File "+strOut+" is not be writed!\nMake sure that the user permissions allow write file.";
			uitoolkit.MsgBox(str, siMsgCritical, L"Applink Message", button);
			app.LogMessage(str, siErrorMsg);
			return CStatus::False;
		}
		mfw.close();

		if(Get3DCoatParam(L"bExpMat").GetValue())
		{
			OutputMaterials(selection );
		}
		bar.PutStatusText( L"import.txt" );
		OutputImportTxt();
		bar.PutVisible(false);

		app.LogMessage(L"Export done!");
	}
	else
	{
		app.LogMessage(L"Please, select objects!", siWarningMsg);
		return CStatus::False;
	}

	return CStatus::OK;
}