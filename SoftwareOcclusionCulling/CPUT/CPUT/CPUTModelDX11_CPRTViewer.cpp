
#include "CPUTModelDX11_CPRTViewer.h"

CPUTModel_CPRT::CPUTModel_CPRT()	
{
	boxes.resize(0);
}
CPUTModel_CPRT::~CPUTModel_CPRT()
{
#ifdef _DEBUG
	printf("CPUTModel_CPRT getting destructed.\n");
#endif
	boxes.clear();
	if(vertices.size() > 0)
	{
		for(std::vector< float* >::iterator it = vertices.begin(); it != vertices.end(); ++it)
		{
			delete (*it);
		}
	}
	if(normals.size() > 0)
	{
		for(std::vector< float* >::iterator it = normals.begin(); it != normals.end(); ++it)
		{
			delete (*it);
		}
	}
	if(tangents.size() > 0)
	{
		for(std::vector< float* >::iterator it = tangents.begin(); it != tangents.end(); ++it)
		{
			delete (*it);
		}
	}
}

bool CPUTModel_CPRT::isMeshLayer(int meshIndex)
{
	for(unsigned int i = 0; i < layerIndices.size(); ++i)
		if(layerIndices[i] == meshIndex)
			return true;
	return false;
}

bool CPUTModel_CPRT::isTangentOverlay(CPUTMesh* mesh)
{
	for(std::vector<CPUTMesh*>::iterator it = tangentOverlays.begin(); it!=tangentOverlays.end(); ++it)
		if( (*it) == mesh )
			return true;
	return false;
}

bool CPUTModel_CPRT::isNormalOverlay(CPUTMesh* mesh)
{
	for(std::vector<CPUTMesh*>::iterator it = normalOverlays.begin(); it!=normalOverlays.end(); ++it)
		if( (*it) == mesh )
			return true;
	return false;
}

void CPUTModel_CPRT::SetVertices(float* incVerts, int incCount)
{
	vertices.push_back(incVerts);
	vertexCount.push_back(incCount);
}

void CPUTModel_CPRT::SetNormals(float* incNormals, int incCount)
{
	normals.push_back(incNormals);
	normalCount.push_back(incCount);
}

void CPUTModel_CPRT::SetTangents(float* incTangents, int incCount)
{
	tangents.push_back(incTangents);
	tangentCount.push_back(incCount);
}

void CPUTModel_CPRT::CreateModelLayer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext, ID3D10Blob* pVertexShaderBlob, LayerInitialization layerType)
{
	CPUTMesh* pMesh = new CPUTMesh();

	float* layer = NULL;
	switch(layerType)
	{
	case NORMAL_LAYER:
		if(normals.size() < 1)
			return;
		normalOverlays.push_back(pMesh);
		layer = normals.front();
		break;
	case TANGENT_LAYER:
		if(tangents.size() < 1)
			return;
		tangentOverlays.push_back(pMesh);
		layer = tangents.front();
		break;
	}
	
	float* vertsToSubmit = new float[vertexCount.front()*3*2];
	float* uvsToSubmit = new float[vertexCount.front()*3*2];

	unsigned int vertIndexToSubmit = 0, uvIndexToSubmit=0;
	for(unsigned int i = 0; i < vertexCount.front()*3; ++i,vertIndexToSubmit+=3,uvIndexToSubmit+=2)//Going to process the vertex/normal information to create a bunch of lines
	{
		vertsToSubmit[vertIndexToSubmit] = vertices.front()[i];
		uvsToSubmit[uvIndexToSubmit] = 0.0f;
		vertsToSubmit[vertIndexToSubmit+3] = vertices.front()[i]+0.2f*layer[i];
		uvsToSubmit[uvIndexToSubmit+2] = 1.0f;
		++i,++vertIndexToSubmit;++uvIndexToSubmit;
		vertsToSubmit[vertIndexToSubmit] = vertices.front()[i];
		uvsToSubmit[uvIndexToSubmit] = 0.0f;
		vertsToSubmit[vertIndexToSubmit+3] = vertices.front()[i]+0.2f*layer[i];
		uvsToSubmit[uvIndexToSubmit+2] = 1.0f;
		++i,++vertIndexToSubmit;++uvIndexToSubmit;
		vertsToSubmit[vertIndexToSubmit] = vertices.front()[i];
		vertsToSubmit[vertIndexToSubmit+3] = vertices.front()[i]+0.2f*layer[i];
		++vertIndexToSubmit;
	}
	delete layer;
	// set the topology
	pMesh->SetMeshTopology( CPUT_TOPOLOGY_LINE_LIST );
	CPUTMeshStreamUniform* meshStream = new CPUTMeshStreamUniform();
	// position stream
	CPUTMeshStreamInfoBlock* infoBlock = new CPUTMeshStreamInfoBlock();
	meshStream->SetName("POSITION");
	meshStream->SetData(vertsToSubmit);
	infoBlock->m_StreamBufferType = CPUT_STREAM_TYPE_VERTEX;
	infoBlock->m_StreamComponentsLayoutType =  CPUT_STREAM_ELEMENT_LAYOUT_UNIFORM;
	infoBlock->m_DataFormatElementType = CPUT_F32;
	infoBlock->m_NumberDataFormatElements = 3;  // 3xF32 
	infoBlock->m_DataElementBlockSize = sizeof(float)*3; // 12 bytes = size of CPUT_F32_F32_F32 element
	infoBlock->m_NumberVerticies = vertexCount.front()*2;
	meshStream->SetStreamInfo(infoBlock);
	pMesh->AddVertexStream(meshStream);

	// UV stream
	meshStream = new CPUTMeshStreamUniform();
	infoBlock = new CPUTMeshStreamInfoBlock();
	meshStream->SetName("TEXCOORD");
	meshStream->SetData(uvsToSubmit);
	infoBlock->m_StreamBufferType = CPUT_STREAM_TYPE_VERTEX;
	infoBlock->m_StreamComponentsLayoutType =  CPUT_STREAM_ELEMENT_LAYOUT_UNIFORM;
	infoBlock->m_DataFormatElementType = CPUT_F32;
	infoBlock->m_NumberDataFormatElements = 2;  // 3xF32 
	infoBlock->m_DataElementBlockSize = sizeof(float)*3; // 12 bytes = size of CPUT_F32_F32_F32 element
	infoBlock->m_NumberVerticies = vertexCount.front()*2;
	meshStream->SetStreamInfo(infoBlock);
	pMesh->AddVertexStream(meshStream);

	CPUTMaterialPhong* pMaterialPhong = new CPUTMaterialPhong();
	switch(layerType)
	{
	case NORMAL_LAYER:
		pMaterialPhong->SetTexture( CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_TEXTURE, pd3dDevice, "../media/normals.png" );
		break;
	case TANGENT_LAYER:
		pMaterialPhong->SetTexture( CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_TEXTURE, pd3dDevice, "../media/tangents.png" );
		break;
	}
		
	pMesh->SetMaterial((CPUTMaterialBase*)pMaterialPhong);

	int currentNumMeshes = AddMesh(pMesh);

	D3D11_INPUT_ELEMENT_DESC* layout = NULL;
	ID3D11InputLayout*  pVertexLayout = NULL;
	pMesh = (CPUTMesh*) GetMesh(currentNumMeshes-1);
	layerIndices.push_back(currentNumMeshes-1);

	// register each mesh with the current graphics device
	pMesh->Register(pd3dDevice, &layout);
                    
	// Create the input layout (TODO: should this be left to user to do?) 
	int numInputLayoutElements;
	pMesh->GetNumberOfInputLayoutElements(numInputLayoutElements); 

	int numElements = pMesh->GetVertexStreamCount();            
	pd3dDevice->CreateInputLayout( layout, numInputLayoutElements, pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), &pVertexLayout );         
	pMesh->SetDXLayout(pVertexLayout);
	delete vertsToSubmit;
	delete uvsToSubmit;
	
	switch(layerType)
	{
	case NORMAL_LAYER:
		normals.erase(normals.begin());
		break;
	case TANGENT_LAYER:
		tangents.erase(tangents.begin());
		break;
	}
}

void CPUTModel_CPRT::AddBox(int meshIndex, std::vector<float> &extents, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext, ID3D10Blob* pVertexShaderBlob)
{
	BoundingBox box(meshIndex);
	box.SetExtents(extents);
	box.CreateModel(pd3dDevice, pImmediateContext, pVertexShaderBlob);
	boxes.push_back(box);	
}

//This is the sphere test from the DX SDK "Touch" sample.  Pretty elegant imo.
bool CPUTModel_CPRT::IntersectionTest(const XMVECTOR &rayOrigin, const XMVECTOR &rayDirection, XMVECTOR &modelPosition, BoundingBox &box)
{
	modelPosition = XMVectorSet ( m_pTransform.m_pData[0][3] , m_pTransform.m_pData[1][3] , m_pTransform.m_pData[2][3] , m_pTransform.m_pData[3][3] );  //TODO: may have to change this
	for(unsigned int i = 0; i < boxes.size(); ++i)
	{
		box = boxes.at(i);
		XMVECTOR rayToSphere = modelPosition - rayOrigin;
		
		//Basic idea is to compare the projection of the casted ray on the ray from the camera position to the sphere position.
		//If the the casted ray is sufficiently close to the rayToSphere then box.GetSphereRadiusSquared() - lengthOfDisplacementSquared will be > 0
		float pointingTheRightWayMeasure = XMVectorGetW( XMVector3Dot( rayToSphere , rayDirection ) );
		float lengthOfDisplacementSquared = XMVectorGetW( XMVector3Dot( rayToSphere , rayToSphere ) );
		float magic = pointingTheRightWayMeasure*pointingTheRightWayMeasure - lengthOfDisplacementSquared + box.GetSphereRadiusSquared();
		
		selected = ( magic > 0 ) ? true : false ;
		if(selected)	return selected;
	}
	return selected;
}

bool CPUTModel_CPRT::IntersectionTestRectangle(const XMVECTOR &rayOrigin, const XMVECTOR &rayDirection, XMVECTOR &modelPosition, BoundingBox &box)
{
	modelPosition = XMVectorSet ( m_pTransform.m_pData[0][3] , m_pTransform.m_pData[1][3] , m_pTransform.m_pData[2][3] , m_pTransform.m_pData[3][3] );
	bool result = false;
	for(unsigned int i = 0; i < boxes.size(); ++i)
	{
		box = boxes.at(i);
		//ghetto transform
		float partialSum = 0.0f, partialSumTwo = 0.0f;;
		float maximum[3];
		float minimum[3];
		for(int i = 0;i < 3; ++i)
		{
			partialSum = 0.0f;
			partialSumTwo = 0.0f;
			for(int j = 0; j < 4; ++j)
			{
				partialSum += m_pTransform.m_pData[i][j] * ( j != 3 ? box.maximum[j] : 1.0f );
				partialSumTwo += m_pTransform.m_pData[i][j] * ( j != 3 ? box.minimum[j] : 1.0f );
			}
			maximum[i] = partialSum;
			minimum[i] = partialSumTwo;
		}
		//front face
		result = IntersectFace(rayOrigin,rayDirection,
								XMVectorSet(minimum[0],minimum[1],minimum[2],1.0),
								XMVectorSet(minimum[0],minimum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],minimum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],minimum[1],minimum[2],1.0) );
		if(result)	return result;

		//back face
		result = IntersectFace(rayOrigin,rayDirection,
								XMVectorSet(minimum[0],maximum[1],minimum[2],1.0),
								XMVectorSet(minimum[0],maximum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],maximum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],maximum[1],minimum[2],1.0) );
		if(result)	return result;

		//top
		result = IntersectFace(rayOrigin,rayDirection,
								XMVectorSet(minimum[0],minimum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],minimum[1],maximum[2],1.0),
								XMVectorSet(minimum[0],maximum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],maximum[1],maximum[2],1.0) );
		if(result)	return result;

		//bottom
		result = IntersectFace(rayOrigin,rayDirection,
								XMVectorSet(minimum[0],minimum[1],minimum[2],1.0),
								XMVectorSet(minimum[0],maximum[1],minimum[2],1.0),
								XMVectorSet(maximum[0],maximum[1],minimum[2],1.0),
								XMVectorSet(maximum[0],minimum[1],minimum[2],1.0) );
		if(result)	return result;

		//left
		result = IntersectFace(rayOrigin,rayDirection,
								XMVectorSet(minimum[0],minimum[1],minimum[2],1.0),
								XMVectorSet(minimum[0],minimum[1],maximum[2],1.0),
								XMVectorSet(minimum[0],maximum[1],maximum[2],1.0),
								XMVectorSet(minimum[0],maximum[1],minimum[2],1.0) );
		if(result)	return result;

		//right
		result = IntersectFace(rayOrigin,rayDirection,
								XMVectorSet(maximum[0],minimum[1],minimum[2],1.0),
								XMVectorSet(maximum[0],minimum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],maximum[1],maximum[2],1.0),
								XMVectorSet(maximum[0],maximum[1],minimum[2],1.0) );
		if(result)	return result;
	}
	return false;
}
	
bool CPUTModel_CPRT::IntersectFace(	const XMVECTOR &rayOrigin, const XMVECTOR &rayDirection,
					const XMVECTOR &vertexOne, const XMVECTOR &vertexTwo, const XMVECTOR &vertexThree,
					const XMVECTOR &vertexFour)
{
	for(int i = 0; i < 2; ++i)
	{
		XMVECTOR edge1 = (i==0) ? vertexOne-vertexTwo : vertexOne-vertexFour;
		XMVECTOR edge2 = (i==0) ? vertexThree-vertexTwo : vertexThree-vertexFour;

		XMVECTOR crossWithPickRay = XMVector3Cross(rayDirection, edge2);

		float otherEdgeProjection = XMVectorGetX( XMVector3Dot(edge1, crossWithPickRay) );

		XMVECTOR tvec = (otherEdgeProjection > 0) ? (i==0 ? rayOrigin - vertexTwo : rayOrigin - vertexFour) : (i==0 ? vertexTwo - rayOrigin : vertexFour - rayOrigin);
		otherEdgeProjection = (otherEdgeProjection > 0) ? otherEdgeProjection : - otherEdgeProjection;
		if(otherEdgeProjection < 0.0001f)
			continue;

		float u = XMVectorGetX( XMVector3Dot( tvec, crossWithPickRay ) );
		if( u < 0.0f || u > otherEdgeProjection )
			continue;

		XMVECTOR qvec = XMVector3Cross( tvec, edge1 );

		float v = XMVectorGetX( XMVector3Dot( rayDirection, qvec ) );
		if ( v < 0.0f || u + v > otherEdgeProjection )
			continue;
		return true;
	}
	return false;
}