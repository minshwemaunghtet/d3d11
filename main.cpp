#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <assert.h>

struct ConstantBuffer
{
    float x_offset = 0.0f;
    float y_offset = 0.0f;
    float padding[2] = { 0.0f, 0.0f };
};


static ConstantBuffer vertexOffset = { 0.0f, 0.0f };
static ID3D11Buffer* constantBuffer;
static ID3D11Buffer* indexBuffer;
static ID3D11RasterizerState* rasterizerState;
static const int width = 1024;
static const int height = 768;


// Data for Index Buffer
static const unsigned short indices[] = { 
    2, 1, 0,
};


// Data for Vertex Buffer
static const float vertexData[] = { // x, y, z
        0.0f,  0.5f, 0.f,
        0.5f, -0.5f, 0.f,
        -0.5f, -0.5f, 0.f
};


// Window Procedure including movement of triangle
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    switch(msg)
    {
    case WM_KEYDOWN:
    {
        if(wparam == VK_ESCAPE)
            DestroyWindow(hwnd);
        if (wparam == VK_UP)
            vertexOffset.y_offset += 0.01f;
        if (wparam == VK_DOWN)
            vertexOffset.y_offset -= 0.01f;
        if (wparam == VK_LEFT)
            vertexOffset.x_offset -= 0.01f;
        if (wparam == VK_RIGHT)
            vertexOffset.x_offset += 0.01f;
        break;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }

    default:
        result = DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return result;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    // Create and Open a window
    HWND hwnd;
    {
        WNDCLASSEXW winClass = {};
        winClass.cbSize = sizeof(WNDCLASSEXW);
        winClass.style = CS_HREDRAW | CS_VREDRAW;
        winClass.lpfnWndProc = &WndProc;
        winClass.hInstance = hInstance;
        winClass.hIcon = LoadIconW(0, IDI_APPLICATION);
        winClass.hCursor = LoadCursorW(0, IDC_ARROW);
        winClass.lpszClassName = L"MyWindowClass";
        winClass.hIconSm = LoadIconW(0, IDI_APPLICATION);

        if(!RegisterClassExW(&winClass)) {
            MessageBoxA(0, "RegisterClassEx failed", "Fatal Error", MB_OK);
            return GetLastError();
        }

        RECT initialRect = { 0, 0, width, height };
        AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
        LONG initialWidth = initialRect.right - initialRect.left;
        LONG initialHeight = initialRect.bottom - initialRect.top;

        hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
                                winClass.lpszClassName,
                                L"Direct 3D Triangle",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                initialWidth, 
                                initialHeight,
                                0, 0, hInstance, 0);

        if(!hwnd) {
            MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
            return GetLastError();
        }
    }


    // Create D3D11 Device and Context
    ID3D11Device1* d3d11Device;
    ID3D11DeviceContext1* d3d11DeviceContext;
    {
        ID3D11Device* baseDevice;
        ID3D11DeviceContext* baseDeviceContext;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        HRESULT hResult = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 
                                            0, creationFlags, 
                                            featureLevels, ARRAYSIZE(featureLevels), 
                                            D3D11_SDK_VERSION, &baseDevice, 
                                            0, &baseDeviceContext);
        if(FAILED(hResult)){
            MessageBoxA(0, "D3D11CreateDevice() failed", "Fatal Error", MB_OK);
            return GetLastError();
        }
        
        // Get D3D11 Device and Context
        hResult = baseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&d3d11Device);
        assert(SUCCEEDED(hResult));
        baseDevice->Release();

        hResult = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&d3d11DeviceContext);
        assert(SUCCEEDED(hResult));
        baseDeviceContext->Release();
    }


    // Create Swap Chain
    IDXGISwapChain1* d3d11SwapChain;
    {
        // Get DXGI Factory (needed to create Swap Chain)
        IDXGIFactory2* dxgiFactory;
        {
            IDXGIDevice1* dxgiDevice;
            HRESULT hResult = d3d11Device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
            assert(SUCCEEDED(hResult));

            IDXGIAdapter* dxgiAdapter;
            hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
            assert(SUCCEEDED(hResult));
            dxgiDevice->Release();

            DXGI_ADAPTER_DESC adapterDesc;
            dxgiAdapter->GetDesc(&adapterDesc);

            OutputDebugStringA("Graphics Device: ");
            OutputDebugStringW(adapterDesc.Description);

            hResult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);
            assert(SUCCEEDED(hResult));
            dxgiAdapter->Release();
        }
        
        DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
        d3d11SwapChainDesc.Width = width; 
        d3d11SwapChainDesc.Height = height; 
        d3d11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        d3d11SwapChainDesc.SampleDesc.Count = 1;
        d3d11SwapChainDesc.SampleDesc.Quality = 0;
        d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        d3d11SwapChainDesc.BufferCount = 2;
        d3d11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        d3d11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        d3d11SwapChainDesc.Flags = 0;

        HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(d3d11Device, hwnd, &d3d11SwapChainDesc, 0, 0, &d3d11SwapChain);
        assert(SUCCEEDED(hResult));

        dxgiFactory->Release();
    }

    
    // Create Framebuffer Render Target
    ID3D11RenderTargetView* d3d11FrameBufferView;
    {
        ID3D11Texture2D* d3d11FrameBuffer;
        
        HRESULT hResult = d3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
        assert(SUCCEEDED(hResult));

        hResult = d3d11Device->CreateRenderTargetView(d3d11FrameBuffer, 0, &d3d11FrameBufferView);
        assert(SUCCEEDED(hResult));
        
        d3d11FrameBuffer->Release();
    }


    // Create Vertex Shader
    ID3DBlob* vsBlob;
    ID3D11VertexShader* vertexShader;
    {
        ID3DBlob* shaderCompileErrorsBlob;
        
        HRESULT hResult = D3DCompileFromFile(L"../shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, &shaderCompileErrorsBlob);
        if(FAILED(hResult))
        {
            const char* errorString = NULL;
            if(hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                errorString = "Could not compile shader; file not found";
            else if(shaderCompileErrorsBlob){
                errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
                shaderCompileErrorsBlob->Release();
            }
            MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
            return 1;
        }

        hResult = d3d11Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
        assert(SUCCEEDED(hResult));
        if(shaderCompileErrorsBlob)
            shaderCompileErrorsBlob->Release();
        //vsBlob->Release();  If release, it will give error
    }


    // Create Pixel Shader
    ID3D11PixelShader* pixelShader;
    {
        ID3DBlob* psBlob;
        ID3DBlob* shaderCompileErrorsBlob;
        
        HRESULT hResult = D3DCompileFromFile(L"../shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, &shaderCompileErrorsBlob);
        if(FAILED(hResult))
        {
            const char* errorString = NULL;
            if(hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                errorString = "Could not compile shader; file not found";
            else if(shaderCompileErrorsBlob){
                errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
                shaderCompileErrorsBlob->Release();
            }
            MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
            return 1;
        }

        hResult = d3d11Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
        assert(SUCCEEDED(hResult));
        psBlob->Release();
        if(shaderCompileErrorsBlob)
            shaderCompileErrorsBlob->Release();
    }


    // Create Input Layout
    ID3D11InputLayout* inputLayout;
    {
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            // Position = xyz data, TexCoord = texture (UV) data
            { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            //{ "Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            //{ "TexCoord", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            //{ "Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        HRESULT hResult = d3d11Device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
        assert(SUCCEEDED(hResult));
        //vsBlob->Release();
    }


    // Create Vertex Buffer
    ID3D11Buffer* vertexBuffer;
    UINT numVerts;
    UINT stride;
    UINT offset;
    {
        stride = 3 * sizeof(float);
        numVerts = sizeof(vertexData) / stride;
        offset = 0;

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = sizeof(vertexData);
        vertexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexSubresourceData = { vertexData };

        HRESULT hResult = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
        assert(SUCCEEDED(hResult));
    }


    // Create Index Buffer
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = UINT(sizeof(unsigned short) * 3);
        bufferDesc.StructureByteStride = sizeof(unsigned short);
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0u;
        bufferDesc.MiscFlags = 0u;

        // Define the resource data.
        D3D11_SUBRESOURCE_DATA InitData = {};
        InitData.pSysMem = indices;

        // Create the buffer with the device.
       d3d11Device->CreateBuffer(&bufferDesc, &InitData, &indexBuffer);
    }


    // Create Constant Buffer
    {
        D3D11_BUFFER_DESC cbd;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbd.MiscFlags = 0u;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.StructureByteStride = 0u;

        D3D11_SUBRESOURCE_DATA csd = {};
        csd.pSysMem = &vertexOffset;
        d3d11Device->CreateBuffer(&cbd, &csd, &constantBuffer);
        //constantBuffer->Release();

    }


    // Create Rasterizer State
    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.FrontCounterClockwise = true;
        
        d3d11Device->CreateRasterizerState(&desc, &rasterizerState);
    }


    // Main Loop
    bool isRunning = true;
    while(isRunning)
    {
        MSG msg = {};
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
                isRunning = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }


        // update constant buffer (every frame)
        D3D11_MAPPED_SUBRESOURCE mappedSubresource;
        d3d11DeviceContext->Map(constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mappedSubresource);
        memcpy(mappedSubresource.pData, &vertexOffset, sizeof(ConstantBuffer));
        d3d11DeviceContext->Unmap(constantBuffer, 0u);


        FLOAT backgroundColor[4] = { 0.0f, 0.0f, 0.0f, 0.8f };
        // Clear the backbuffer and set the viewport
        d3d11DeviceContext->ClearRenderTargetView(d3d11FrameBufferView, backgroundColor);

        // Set viewport
        RECT winRect;
        GetClientRect(hwnd, &winRect);
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };
        d3d11DeviceContext->RSSetViewports(1, &viewport);

        // bind frame buffer / set the output merger
        d3d11DeviceContext->OMSetRenderTargets(1, &d3d11FrameBufferView, nullptr);
        d3d11DeviceContext->RSSetState(rasterizerState);

        // bind constant buffer
        d3d11DeviceContext->VSSetConstantBuffers(0u, 1u, &constantBuffer);

        // bind primitive topology
        d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // bind vertex layout/input assembler
        d3d11DeviceContext->IASetInputLayout(inputLayout);

        // bind shaders
        d3d11DeviceContext->VSSetShader(vertexShader, nullptr, 0);
        d3d11DeviceContext->PSSetShader(pixelShader, nullptr, 0);

        // bind vertex buffers
        d3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        d3d11DeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0u);

        // draw triangle
        d3d11DeviceContext->DrawIndexed(3, 0, 0);
        
        // swap front and back buffers
        d3d11SwapChain->Present(1, 0);

    }

    return 0;
}