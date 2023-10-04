/**********************************************************************************
// Multi (Código Fonte)
//
// Criação:     27 Abr 2016
// Atualização: 15 Set 2023
// Compilador:  Visual C++ 2022
//
// Descrição:   Constrói cena usando vários buffers, um por objeto
//
**********************************************************************************/

#include "DXUT.h"

// ------------------------------------------------------------------------------

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };
};

enum Geo {BOX, CYLINDER, SPHERE, GLOBE, GRID};

// ------------------------------------------------------------------------------

class Multi : public App
{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    ID3D12PipelineState* pipelineStateSolid = nullptr;
    vector<Object> scene;

    Timer timer;
    bool spinning = true;
    bool pulse = true;
    bool solid = false;

    XMFLOAT4X4 Identity = {};
    XMFLOAT4X4 View = {};
    XMFLOAT4X4 Proj = {};

    float theta = 0;
    float phi = 0;
    float radius = 0;
    float lastMousePosX = 0;
    float lastMousePosY = 0;

    int objCount = 0;
    int selected = 1;

public:
    void Init();
    void Update();
    void Draw();
    void Finalize();

    void BuildRootSignature();
    void BuildPipelineState();

    void createGeometry(Geo geometry);
};

// ------------------------------------------------------------------------------

void Multi::Init()
{
    graphics->ResetCommands();

    // -----------------------------
    // Parâmetros Iniciais da Câmera
    // -----------------------------
 
    // controla rotação da câmera
    theta = XM_PIDIV4;
    phi = 1.3f;
    radius = 5.0f;

    // pega última posição do mouse
    lastMousePosX = (float) input->MouseX();
    lastMousePosY = (float) input->MouseY();

    // inicializa as matrizes Identity e View para a identidade
    Identity = View = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };

    // inicializa a matriz de projeção
    XMStoreFloat4x4(&Proj, XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f), 
        window->AspectRatio(), 
        1.0f, 100.0f));

    // ----------------------------------------
    // Inicialização da Geometria
    // ----------------------------------------

    createGeometry(GRID);
    createGeometry(BOX);
    createGeometry(SPHERE);

    // ---------------------------------------

    BuildRootSignature();
    BuildPipelineState();    

    // ---------------------------------------
    graphics->SubmitCommands();

    timer.Start();
}

// ------------------------------------------------------------------------------

void Multi::Update()
{
    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    float mousePosX = (float)input->MouseX();
    float mousePosY = (float)input->MouseY();

    if (input->KeyDown(VK_LBUTTON))
    {
        // cada pixel corresponde a 1/4 de grau
        float dx = XMConvertToRadians(0.25f * (mousePosX - lastMousePosX));
        float dy = XMConvertToRadians(0.25f * (mousePosY - lastMousePosY));

        // atualiza ângulos com base no deslocamento do mouse 
        // para orbitar a câmera ao redor da caixa
        theta += dx;
        phi += dy;

        // restringe o ângulo de phi ]0-180[ graus
        phi = phi < 0.1f ? 0.1f : (phi > (XM_PI - 0.1f) ? XM_PI - 0.1f : phi);
    }
    else if (input->KeyDown(VK_RBUTTON))
    {
        // cada pixel corresponde a 0.05 unidades
        float dx = 0.05f * (mousePosX - lastMousePosX);
        float dy = 0.05f * (mousePosY - lastMousePosY);

        // atualiza o raio da câmera com base no deslocamento do mouse 
        radius += dx - dy;

        // restringe o raio (3 a 15 unidades)
        radius = radius < 3.0f ? 3.0f : (radius > 15.0f ? 15.0f : radius);
    }
    
    if (input->KeyPress(VK_DELETE)) {
        scene.erase(scene.begin() + selected);
        objCount--;
        selected = (selected == 0) ? 0 : selected-1;
    }

    if (input->KeyDown('S')) {
        solid = true;
    }

    if (input->KeyDown('W')) {
        solid = false;
    }

    if (input->KeyPress(VK_TAB))
    {
        selected = (selected + 1) % objCount;
        spinning = !spinning;

        if (spinning)
            timer.Start();
        else
            timer.Stop();
    }

    if (input->KeyPress('E')) {
        createGeometry(SPHERE);
    }
    if (input->KeyPress('B')) {
        createGeometry(BOX);
    }
    if (input->KeyPress('C')) {
        createGeometry(CYLINDER);
    }
    if (input->KeyPress('G')) {
        createGeometry(GLOBE);
    }
    if (input->KeyPress('P')) {
        createGeometry(GRID);
    }

    XMMATRIX transform = XMLoadFloat4x4(&Identity);

    // TRANSLATE
    if (input->KeyDown(VK_LEFT)) {
        transform *= XMMatrixTranslation(0.003f, 0.0f, 0.0f);
    }
    if (input->KeyDown(VK_RIGHT)) {
        transform *= XMMatrixTranslation(-0.003f, 0.0f, 0.0f);
    }
    if (!input->KeyDown(VK_CONTROL)) {

        if (!input->KeyDown(VK_SHIFT)) {
            if (input->KeyDown(VK_DOWN)) {
                transform *= XMMatrixTranslation(0.0f, 0.0f, 0.003f);
            }
            if (input->KeyDown(VK_UP)) {
                transform *= XMMatrixTranslation(0.0f, 0.0f, -0.003f);
            }
        }

        if (input->KeyDown(VK_UP) && input->KeyDown(VK_SHIFT)) {
            transform *= XMMatrixTranslation(0.0f, 0.003f, 0.0f);
        }
        if (input->KeyDown(VK_DOWN) && input->KeyDown(VK_SHIFT)) {
            transform *= XMMatrixTranslation(0.0f, -0.003f, 0.0f);
        }
    }

    // SCALE
    if (input->KeyDown(VK_UP) && input->KeyDown(VK_CONTROL)) {
        transform *= XMMatrixScaling(1.001f, 1.001f, 1.001f);
    }
    if (input->KeyDown(VK_DOWN) && input->KeyDown(VK_CONTROL)) {
        transform *= XMMatrixScaling(0.999f, 0.999f, 0.999f);
    }

    // ROTATE
    if (input->KeyPress('X') && !input->KeyDown(VK_SHIFT)) {
        transform *= XMMatrixRotationX(XMConvertToRadians(15));
    }
    if (input->KeyPress('Y')) {
        transform *= XMMatrixRotationY(XMConvertToRadians(15));
    }
    if (input->KeyPress('Z')) {
        transform *= XMMatrixRotationZ(XMConvertToRadians(15));
    }

    lastMousePosX = mousePosX;
    lastMousePosY = mousePosY;

    // converte coordenadas esféricas para cartesianas
    float x = radius * sinf(phi) * cosf(theta);
    float z = radius * sinf(phi) * sinf(theta);
    float y = radius * cosf(phi);

    // constrói a matriz da câmera (view matrix)
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&View, view);

    // carrega matriz de projeção em uma XMMATRIX
    XMMATRIX proj = XMLoadFloat4x4(&Proj);

    // OPERAÇÕES COM O OBJETO SELECIONADO
    if (objCount > 0) {
        XMMATRIX objWorldMatrix = XMLoadFloat4x4(&scene[selected].worldStopped);
        XMStoreFloat4x4(&scene[selected].worldStopped, objWorldMatrix * transform);

        if (scene[selected].round) {
            XMStoreFloat4x4(&scene[selected].world,
                XMMatrixRotationY(float(timer.Elapsed())) *
                objWorldMatrix);
        }
        else {
            pulse = !pulse;
            if (pulse) {
                XMStoreFloat4x4(&scene[selected].world,
                    XMMatrixScaling(1.015f, 1.015f, 1.015f) *
                    objWorldMatrix);
            }
            else {
                XMStoreFloat4x4(&scene[selected].world,
                    XMMatrixScaling(1.0f, 1.0f, 1.0f) *
                    objWorldMatrix);
            }
        }
    }
    // ajusta o buffer constante de cada objeto
    for (auto & obj : scene)
    {
        // carrega matriz de mundo em uma XMMATRIX
        XMMATRIX world = XMLoadFloat4x4(&obj.world);      

        // constrói matriz combinada (world x view x proj)
        XMMATRIX WorldViewProj = world * view * proj;        

        // atualiza o buffer constante com a matriz combinada
        ObjectConstants constants;
        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        obj.mesh->CopyConstants(&constants);
    }
}

// ------------------------------------------------------------------------------

void Multi::Draw()
{
    // limpa o backbuffer
    if(solid)
        graphics->Clear(pipelineStateSolid);
    else    
        graphics->Clear(pipelineState);

    // desenha objetos da cena
    for (auto& obj : scene)
    {
        // comandos de configuração do pipeline
        ID3D12DescriptorHeap* descriptorHeaps[] = { obj.mesh->ConstantBufferHeap() };
        graphics->CommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
        graphics->CommandList()->IASetVertexBuffers(0, 1, obj.mesh->VertexBufferView());
        graphics->CommandList()->IASetIndexBuffer(obj.mesh->IndexBufferView());
        graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ajusta o buffer constante associado ao vertex shader
        graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.mesh->ConstantBufferHandle(0));

        // desenha objeto
        graphics->CommandList()->DrawIndexedInstanced(
            obj.submesh.indexCount, 1,
            obj.submesh.startIndex,
            obj.submesh.baseVertex,
            0);
    }
 
    // apresenta o backbuffer na tela
    graphics->Present();    
}

// ------------------------------------------------------------------------------

void Multi::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();

    for (auto& obj : scene)
        delete obj.mesh;
}

////////// TODO
void Multi::createGeometry(Geo geometry) {

    Object obj;
    XMStoreFloat4x4(&obj.world,
        XMMatrixScaling(0.4f, 0.4f, 0.4f) *
        XMMatrixTranslation(0.0f, 0.2f, 0.0f));

    XMStoreFloat4x4(&obj.worldStopped,
        XMMatrixScaling(0.4f, 0.4f, 0.4f) *
        XMMatrixTranslation(0.0f, 0.2f, 0.0f));
    
    obj.mesh = new Mesh();

    switch (geometry) {

        case BOX: {
            Box box(1.0f, 1.0f, 1.0f);
            for (auto& v : box.vertices) v.color = XMFLOAT4(DirectX::Colors::Orange);

            obj.mesh->VertexBuffer(box.VertexData(), box.VertexCount() * sizeof(Vertex), sizeof(Vertex));
            obj.mesh->IndexBuffer(box.IndexData(), box.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
            obj.submesh.indexCount = box.IndexCount();

            break;
        }
        case CYLINDER: {
            Cylinder cylinder(1.0f, 0.5f, 3.0f, 20, 20);
            for (auto& v : cylinder.vertices) v.color = XMFLOAT4(DirectX::Colors::Yellow);


            obj.mesh->VertexBuffer(cylinder.VertexData(), cylinder.VertexCount() * sizeof(Vertex), sizeof(Vertex));
            obj.mesh->IndexBuffer(cylinder.IndexData(), cylinder.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
            obj.submesh.indexCount = cylinder.IndexCount();
            obj.round = true;

            break;
        }
        case SPHERE: {
            Sphere sphere(1.0f, 20, 20);
            for (auto& v : sphere.vertices) v.color = XMFLOAT4(DirectX::Colors::Crimson);
            
            obj.mesh->VertexBuffer(sphere.VertexData(), sphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
            obj.mesh->IndexBuffer(sphere.IndexData(), sphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
            obj.submesh.indexCount = sphere.IndexCount();
            obj.round = true;

            break;
        }
        case GRID: {
            Grid grid(3.0f, 3.0f, 20, 20);
            for (auto& v : grid.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);

            obj.world = Identity;
            obj.worldStopped = Identity;
            obj.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
            obj.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
            obj.submesh.indexCount = grid.IndexCount();

            break;
        }
    }

    obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    scene.push_back(obj);
    selected = objCount++;
}



// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Multi::BuildRootSignature()
{
    // cria uma única tabela de descritores de CBVs
    D3D12_DESCRIPTOR_RANGE cbvTable = {};
    cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvTable.NumDescriptors = 1;
    cbvTable.BaseShaderRegister = 0;
    cbvTable.RegisterSpace = 0;
    cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // define parâmetro raiz com uma tabela
    D3D12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvTable;

    // uma assinatura raiz é um vetor de parâmetros raiz
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // serializa assinatura raiz
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    if (error != nullptr)
    {
        OutputDebugString((char*)error->GetBufferPointer());
    }

    // cria uma assinatura raiz com um único slot que aponta para  
    // uma faixa de descritores consistindo de um único buffer constante
    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

// ------------------------------------------------------------------------------

void Multi::BuildPipelineState()
{
    // --------------------
    // --- Input Layout ---
    // --------------------
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --------------------
    // ----- Shaders ------
    // --------------------

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    // --------------------
    // ---- Rasterizer ----
    // --------------------

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_BACK;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // ---------------------
    // --- Color Blender ---
    // ---------------------

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // ---------------------
    // --- Depth Stencil ---
    // ---------------------

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;
    
    // -----------------------------------
    // --- Pipeline State Object (PSO) ---
    // -----------------------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = rootSignature;
    pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    pso.BlendState = blender;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState = rasterizer;
    pso.DepthStencilState = depthStencil;
    pso.InputLayout = { inputLayout, 2 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = graphics->Antialiasing();
    pso.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));

    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState = rasterizer;
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineStateSolid));

    vertexShader->Release();
    pixelShader->Release();
}


// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    try
    {
        // cria motor e configura a janela
        Engine* engine = new Engine();
        engine->window->Mode(WINDOWED);
        engine->window->Size(1024, 720);
        engine->window->Color(25, 25, 25);
        engine->window->Title("Multi");
        engine->window->Icon(IDI_ICON);
        engine->window->Cursor(IDC_CURSOR);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Multi());

        // finaliza execução
        delete engine;
    }
    catch (Error & e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Multi", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------

