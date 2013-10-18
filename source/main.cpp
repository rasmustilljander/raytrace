#include "stdafx.h"
#include "ComputeHelp.h"
#include "D3D11Timer.h"
#include "Primitives.h"
#include "Light.h"
#include "Camera.h"

#define MOUSE_SENSE 0.0087266f
#define MOVE_SPEED  150.0f

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE					g_hInst					= NULL;  
HWND						g_hWnd					= NULL;

IDXGISwapChain*				g_SwapChain				= NULL;
ID3D11Device*				g_Device				= NULL;
ID3D11DeviceContext*		g_DeviceContext			= NULL;

ID3D11UnorderedAccessView*	g_BackBufferUAV			= NULL;		
ID3D11Buffer*				g_EveryFrameBuffer		= NULL; 	
ID3D11Buffer*				g_PrimitivesBuffer		= NULL;
ID3D11Buffer*				g_LightBuffer			= NULL;


ComputeWrap*				g_ComputeSys			= NULL;
ComputeShader*				g_ComputeShader			= NULL;

D3D11Timer*					g_Timer					= NULL;

int							g_Width, g_Height;

POINT						m_oldMousePos;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT             InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT				Init();
HRESULT				InitializeDXDeviceAndSwapChain();
HRESULT				CreatePrimitiveBuffer();
void				FillPrimitiveBuffer();
HRESULT				CreateLightBuffer();
void				FillLightBuffer();
HRESULT				CreateCameraBuffer();
void				FillCameraBuffer();
HRESULT				Render(float deltaTime);
HRESULT				Update(float deltaTime);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
char*				FeatureLevelToString(D3D_FEATURE_LEVEL featureLevel);


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
		return 0;

	if( FAILED( Init() ) )
		return 0;

	__int64 cntsPerSec = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&cntsPerSec);
	float secsPerCnt = 1.0f / (float)cntsPerSec;

	__int64 prevTimeStamp = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&prevTimeStamp);

	// Main message loop
	MSG msg = {0};
	while(WM_QUIT != msg.message)
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			__int64 currTimeStamp = 0;
			QueryPerformanceCounter((LARGE_INTEGER*)&currTimeStamp);
			float dt = (currTimeStamp - prevTimeStamp) * secsPerCnt;

			//render
			Update(dt);
			Render(dt);

			prevTimeStamp = currTimeStamp;
		}
	}

	return (int) msg.wParam;
}
//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = 0;
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = "BTH_D3D_Template";
	wcex.hIconSm        = 0;
	if( !RegisterClassEx(&wcex) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance; 
	RECT rc = { 0, 0, 800, 800 };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	
	if(!(g_hWnd = CreateWindow("BTH_D3D_Template", "BTH - Direct3D 11.0 Template",
							WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT, CW_USEDEFAULT,
							rc.right - rc.left,
							rc.bottom - rc.top,
							NULL, NULL, hInstance, NULL)))
	{
		return E_FAIL;
	}

	ShowWindow( g_hWnd, nCmdShow );

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT Init()
{
	HRESULT hr;
	hr = InitializeDXDeviceAndSwapChain();
	if(FAILED(hr))	return hr;
	
	
	// My things
	//GetCamera().setLens(0.5f * PI, 1.0f, 1.0f, 1000.0f);
	//GetCamera().setLens(0.25f*PI, ScreenAspect, NearPlane, FarPlane);
	
	hr = CreateCameraBuffer();
	if(FAILED(hr))	
		return hr;

	hr = CreatePrimitiveBuffer();
	if(FAILED(hr))	
		return hr;

	hr = CreateLightBuffer();
	if(FAILED(hr))	
		return hr;


	FillPrimitiveBuffer();
	FillLightBuffer();

	return S_OK;
}
HRESULT InitializeDXDeviceAndSwapChain()
{
	HRESULT hr = S_OK;;

	RECT rc;
	GetClientRect( g_hWnd, &rc );
	g_Width = rc.right - rc.left;;
	g_Height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
	#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

	D3D_DRIVER_TYPE driverType;

	D3D_DRIVER_TYPE driverTypes[] = 
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_Width;
	sd.BufferDesc.Height = g_Height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL featureLevelsToTry[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	D3D_FEATURE_LEVEL initiatedFeatureLevel;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			driverType,
			NULL,
			createDeviceFlags,
			featureLevelsToTry,
			ARRAYSIZE(featureLevelsToTry),
			D3D11_SDK_VERSION,
			&sd,
			&g_SwapChain,
			&g_Device,
			&initiatedFeatureLevel,
			&g_DeviceContext);

		if( SUCCEEDED( hr ) )
		{
			char title[256];
			sprintf_s(
				title,
				sizeof(title),
				"BTH - Direct3D 11.0 Template | Direct3D 11.0 device initiated with Direct3D %s feature level",
				FeatureLevelToString(initiatedFeatureLevel)
			);
			SetWindowText(g_hWnd, title);

			break;
		}
	}
	if( FAILED(hr) )
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer;
	hr = g_SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
	if( FAILED(hr) )
		return hr;

	// create shader unordered access view on back buffer for compute shader to write into texture
	hr = g_Device->CreateUnorderedAccessView( pBackBuffer, NULL, &g_BackBufferUAV );

	//create helper sys and compute shader instance
	g_ComputeSys = new ComputeWrap(g_Device, g_DeviceContext);
	g_ComputeShader = g_ComputeSys->CreateComputeShader(_T("effect\\BasicCompute.fx"), NULL, "main", NULL);
	g_Timer = new D3D11Timer(g_Device, g_DeviceContext);
	return S_OK;
}

HRESULT CreateCameraBuffer()
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC CameraData;
	CameraData.BindFlags			=	D3D11_BIND_CONSTANT_BUFFER ;
	CameraData.Usage				=	D3D11_USAGE_DYNAMIC; 
	CameraData.CPUAccessFlags		=	D3D11_CPU_ACCESS_WRITE;
	CameraData.MiscFlags			=	0;
	CameraData.ByteWidth			=	sizeof(CustomPrimitiveStruct::EachFrameDataStructure);
	hr = g_Device->CreateBuffer( &CameraData, NULL, &g_EveryFrameBuffer);

	return hr;
}

void FillCameraBuffer()
{
	D3DXMATRIX l_projection, l_view, l_inverseProjection, l_inverseView;
	float l_determinant;
	l_view		 = GetCamera().GetProj();
	l_projection = GetCamera().GetView();

	l_determinant = D3DXMatrixDeterminant(&l_projection);
	D3DXMatrixInverse(&l_inverseProjection, &l_determinant, &l_projection);

    l_determinant  = D3DXMatrixDeterminant(&l_view);
    D3DXMatrixInverse(&l_inverseView, &l_determinant, &l_view);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	g_DeviceContext->Map(g_EveryFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	CustomPrimitiveStruct::EachFrameDataStructure l_eachFrameData;
	l_eachFrameData.cameraPosition		= GetCamera().GetPosition();
	l_eachFrameData.inverseProjection	= l_inverseProjection;
	l_eachFrameData.inverseView			= l_inverseView;

	l_eachFrameData.screenWidth			= 800.0f;
	l_eachFrameData.screenHeight		= 800.0f;
	l_eachFrameData.padding1			= 0;
	l_eachFrameData.padding2			= 0;
	*(CustomPrimitiveStruct::EachFrameDataStructure*)mappedResource.pData = l_eachFrameData;
	g_DeviceContext->Unmap(g_EveryFrameBuffer, 0);
}

HRESULT CreatePrimitiveBuffer()
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC PrimitiveData;
	PrimitiveData.BindFlags			=	D3D11_BIND_CONSTANT_BUFFER ;
	PrimitiveData.Usage				=	D3D11_USAGE_DYNAMIC; 
	PrimitiveData.CPUAccessFlags	=	D3D11_CPU_ACCESS_WRITE;
	PrimitiveData.MiscFlags			=	0;
	PrimitiveData.ByteWidth			=	sizeof(CustomPrimitiveStruct::Primitive);
	hr = g_Device->CreateBuffer( &PrimitiveData, NULL, &g_PrimitivesBuffer);

	return hr;
}

void FillPrimitiveBuffer()
{
	D3D11_MAPPED_SUBRESOURCE PrimitivesResources;
	g_DeviceContext->Map(g_PrimitivesBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &PrimitivesResources);
	CustomPrimitiveStruct::Primitive l_primitive;
	
	l_primitive.Sphere[0].MidPosition			= D3DXVECTOR4 (0.0f, 0.0f, 700.0f, 1.0f);
	l_primitive.Sphere[0].Radius				= 200.0f;
	l_primitive.Sphere[0].Color					= D3DXVECTOR3(1.0f, 0.0f, 0.0f);

	l_primitive.Triangle[0].Color				= D3DXVECTOR4(0.0f, 1.0f, 0.0f, 1.0f);
	l_primitive.Triangle[0].Position0			= D3DXVECTOR4(400.0f,	-200.0f,	 500.0f, 1.0f);
	l_primitive.Triangle[0].Position1			= D3DXVECTOR4(400.0f,	 400.0f,	 800.0f, 1.0f);
	l_primitive.Triangle[0].Position2			= D3DXVECTOR4(400.0f,	-200.0f,	1000.0f, 1.0f);

	l_primitive.Sphere[1].MidPosition			= D3DXVECTOR4 (-900.0f, 0.0f, 700.0f, 1.0f);
	l_primitive.Sphere[1].Radius				= 200.0f;
	l_primitive.Sphere[1].Color					= D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	/*
	l_primitive.Triangle[1].Color				= D3DXVECTOR4(1.0f, 1.0f, 0.0f, 1.0f);
	l_primitive.Triangle[1].Position0			= D3DXVECTOR4(-400.0f,	-200.0f,	 500.0f, 1.0f);
	l_primitive.Triangle[1].Position1			= D3DXVECTOR4(-400.0f,	 400.0f,	 800.0f, 1.0f);
	l_primitive.Triangle[1].Position2			= D3DXVECTOR4(-400.0f,	-200.0f,	1000.0f, 1.0f);
	*/
	
	l_primitive.Triangle[1].Color				= D3DXVECTOR4(1.0f, 1.0f, 0.0f, 1.0f);
	l_primitive.Triangle[1].Position0			= D3DXVECTOR4(400.0f,	-200.0f,	1100.0f, 1.0f);
	l_primitive.Triangle[1].Position1			= D3DXVECTOR4(400.0f,	 400.0f,	1400.0f, 1.0f);
	l_primitive.Triangle[1].Position2			= D3DXVECTOR4(400.0f,	-200.0f,	1600.0f, 1.0f);
	

	l_primitive.SphereCount = 2;
	l_primitive.TriangleCount = 2;
	l_primitive.padding1 = -1;
	l_primitive.padding2 = -1;

	*(CustomPrimitiveStruct::Primitive*)PrimitivesResources.pData = l_primitive;
	g_DeviceContext->Unmap(g_PrimitivesBuffer, 0);
}

HRESULT CreateLightBuffer()
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC LightData;
	LightData.BindFlags			=	D3D11_BIND_CONSTANT_BUFFER ;
	LightData.Usage				=	D3D11_USAGE_DYNAMIC; 
	LightData.CPUAccessFlags		=	D3D11_CPU_ACCESS_WRITE;
	LightData.MiscFlags			=	0;
	LightData.ByteWidth			=	sizeof(CustomLightStruct::AllLight);
	hr = g_Device->CreateBuffer( &LightData, NULL, &g_LightBuffer);



	return hr;
}

void FillLightBuffer()
{
	D3D11_MAPPED_SUBRESOURCE LightResources;
	g_DeviceContext->Map(g_LightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &LightResources);
	CustomLightStruct::AllLight l_light;

	l_light.Light[0].ambient		= D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
	l_light.Light[0].diffuse		= D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
	l_light.Light[0].specular		= D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
	l_light.Light[0].attenuation	= D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
	l_light.Light[0].position		= D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	l_light.Light[0].range			= 500.0f;
		
	*(CustomLightStruct::AllLight*)LightResources.pData = l_light;
	g_DeviceContext->Unmap(g_LightBuffer, 0);
}



HRESULT Update(float deltaTime)
{
	if(GetAsyncKeyState('W') & 0x8000)
		GetCamera().walk(MOVE_SPEED * deltaTime);
	if(GetAsyncKeyState('S') & 0x8000)
		GetCamera().walk(MOVE_SPEED* -deltaTime);
	if(GetAsyncKeyState('A') & 0x8000)
		GetCamera().strafe(MOVE_SPEED * -deltaTime);
	if(GetAsyncKeyState('D') & 0x8000)
		GetCamera().strafe(MOVE_SPEED *deltaTime);
	GetCamera().rebuildView();
	
	
	
	FillCameraBuffer();
	
	return S_OK;
}

HRESULT Render(float deltaTime)
{
	ID3D11UnorderedAccessView* uav[] = { g_BackBufferUAV };
	ID3D11Buffer* ppCB[] = {g_EveryFrameBuffer, g_PrimitivesBuffer, g_LightBuffer};

	g_DeviceContext->CSSetUnorderedAccessViews(0, 1, uav, NULL);
	g_DeviceContext->CSSetConstantBuffers(0, 3, ppCB);

	g_ComputeShader->Set();
	g_Timer->Start();
	g_DeviceContext->Dispatch( 25, 25, 1 );
	g_Timer->Stop();
	g_ComputeShader->Unset();

	if(FAILED(g_SwapChain->Present(0, 0)))
		return E_FAIL;

	float x = GetCamera().GetPosition().x;
	float y = GetCamera().GetPosition().y;
	float z = GetCamera().GetPosition().z;

	char title[256];
	sprintf_s(
		title,
		sizeof(title),
		"BTH - DirectCompute raytracing - Dispatch time: %f - camera_position = %f %f %f",
		g_Timer->GetTime(), x, y, z
	);
	SetWindowText(g_hWnd, title);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;
	POINT l_mousePos;
	int dx,dy;

	switch (message) 
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch(wParam)
		{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
		}
		break;
	case WM_MOUSEMOVE:
		//if(wParam & MK_LBUTTON)
		//{
			l_mousePos.x = (int)LOWORD(lParam);
			l_mousePos.y = (int)HIWORD(lParam);

			dx = l_mousePos.x - m_oldMousePos.x;
			dy = l_mousePos.y - m_oldMousePos.y;
			GetCamera().pitch(		dy * MOUSE_SENSE);
			GetCamera().rotateY(	-dx * MOUSE_SENSE);
			m_oldMousePos = l_mousePos;
		//}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

char* FeatureLevelToString(D3D_FEATURE_LEVEL featureLevel)
{
	if(featureLevel == D3D_FEATURE_LEVEL_11_0)
		return "11.0";
	if(featureLevel == D3D_FEATURE_LEVEL_10_1)
		return "10.1";
	if(featureLevel == D3D_FEATURE_LEVEL_10_0)
		return "10.0";

	return "Unknown";
}