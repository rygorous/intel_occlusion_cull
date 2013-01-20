#ifndef __CPUTMODELDX11_CPRTVIEWER_H__
#define __CPUTMODELDX11_CPRTVIEWER_H__

#include <stdio.h>
#include "CPUTModelDX11.h"
#include "..\..\..\samples\CPUT\CPUT\CPUT_DX11.h"
#include "..\..\..\samples\CPUT\CPUT\CPUTMaterialPhongDX11.h"
#include "..\..\..\Tools\CPRT Viewer\CPRTViewer\BoundingBox.h"

enum LayerInitialization	: unsigned short
{
	NORMAL_LAYER,
	TANGENT_LAYER

};

struct BoundingBox;

class CPUTModel_CPRT :public CPUTModel
{
public:
	CPUTModel_CPRT();
	~CPUTModel_CPRT();

	void SetVertices(float* incVerts, int incCount);

	void SetNormals(float* incNormals, int incCount);

	void SetTangents(float* incTangents, int incCount);

	void CreateModelLayer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext, ID3D10Blob* pVertexShaderBlob, LayerInitialization layerType);

	void AddBox(int meshIndex, std::vector<float> &extents, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext, ID3D10Blob* pVertexShaderBlob);

	const std::vector<BoundingBox>& GetBoundingBox()	{ return boxes;	}

	//This is the sphere test from the DX SDK "Touch" sample.  Pretty elegant imo.
	bool IntersectionTest(const XMVECTOR &rayOrigin, const XMVECTOR &rayDirection, XMVECTOR &modelPosition, BoundingBox &box);

	bool IntersectionTestRectangle(const XMVECTOR &rayOrigin, const XMVECTOR &rayDirection, XMVECTOR &modelPosition, BoundingBox &box);
	
	bool IntersectFace(	const XMVECTOR &rayOrigin, const XMVECTOR &rayDirection,
						const XMVECTOR &vertexOne, const XMVECTOR &vertexTwo, const XMVECTOR &vertexThree,
						const XMVECTOR &vertexFour);

	bool isMeshLayer(int meshIndex);

	bool isTangentOverlay(CPUTMesh* mesh);

	bool isNormalOverlay(CPUTMesh* mesh);

private:
	std::vector<float*> vertices;
	std::vector<unsigned int> vertexCount;
	std::vector<float*> normals;
	std::vector<unsigned int> normalCount;
	std::vector<float*> tangents;
	std::vector<unsigned int> tangentCount;
	std::vector<CPUTMesh*> tangentOverlays;
	std::vector<CPUTMesh*> normalOverlays;
	std::vector<BoundingBox> boxes;
	bool selected;
	std::vector<int> layerIndices;
};

#endif //#ifndef __CPUTMODELDX11_CPRTVIEWER_H__