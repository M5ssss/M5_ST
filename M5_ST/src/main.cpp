// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "Core.h"
#include "ImgHelper.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "stb_image.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <filesystem>
#include <algorithm>
#include <comdef.h>
#include <fstream>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static ID3D11SamplerState*      g_ClipSampler = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
    // Masquer la fenêtre console
    #ifdef NDEBUG
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    #endif

    // Create application window
    // ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"M5_ST", WS_EX_LAYERED | WS_POPUP | WS_VISIBLE, 100, 5, 1284, 1066, nullptr, nullptr, wc.hInstance, nullptr);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    ImGui_ImplWin32_EnableAlphaCompositing(hwnd);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.35f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.10f, 0.60f);
    colors[ImGuiCol_Button] = ImVec4(0.85f, 0.75f, 0.10f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.88f, 0.20f, 0.85f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.78f, 0.00f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 0.85f, 0.20f, 0.30f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.85f, 0.20f, 0.60f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.90f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.85f, 0.20f, 0.60f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 0.85f, 0.20f, 0.30f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.85f, 0.20f, 0.65f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.70f, 0.70f, 0.30f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(1.00f, 0.90f, 0.40f, 0.80f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 0.80f, 0.20f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.85f, 0.75f, 0.10f, 0.30f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 0.88f, 0.20f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(1.00f, 0.78f, 0.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 0.85f, 0.20f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 0.90f, 0.30f, 0.50f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.80f, 0.00f, 0.90f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.90f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.85f, 0.20f, 1.00f);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Link Custom Sampler
    D3D11_SAMPLER_DESC sampler_desc = {};
    ZeroMemory(&sampler_desc, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.MipLODBias = 0.f;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampler_desc.MinLOD = 0.f;
    sampler_desc.MaxLOD = 0.f;

    g_pd3dDevice->CreateSamplerState(&sampler_desc, &g_ClipSampler);

    // Window transparency
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

    // Core Init
    Core core(g_pd3dDevice);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // My UI
        // Global ui variables
        static float los_transparency = 0.5f;
        static float marker_height = 1.4f;
        static float target_height = 0.0f;
        const std::vector<std::pair<const char*, float>> height_presets = {
                    { "Ground (0m)", 0.0f },
                    { "Emplaced ATGM (1m)", 1.0f },
                    { "Soldier (1.7m)",      1.7f },
                    { "Vehicle (2.5m)",    2.5f }
        };
        // Custom Title Bar
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.20f, 0.20f, 0.22f, 1.00f));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1284, 38));
        if (ImGui::Begin("TitleBar", nullptr,   ImGuiWindowFlags_NoResize |
                                                ImGuiWindowFlags_NoCollapse |
                                                ImGuiWindowFlags_NoTitleBar |
                                                ImGuiWindowFlags_NoDecoration |
                                                ImGuiWindowFlags_NoMove |
                                                ImGuiWindowFlags_NoScrollbar |
                                                ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::Text("M5_ST");
            ImGui::SameLine(ImGui::GetWindowWidth() - 60);
            ImGui::SetCursorPosY(8);
            // Minimize button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("-", ImVec2(22, 22)))
            {
                PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            }

            ImGui::SameLine();
            ImGui::SetCursorPosY(8);
            // X button
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("X", ImVec2(22, 22)))
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            if (ImGui::IsWindowFocused() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = ImGui::GetMouseDragDelta();
                RECT rect;
                if (GetWindowRect(hwnd, &rect)) {
                    int new_x = (int)rect.left + (int)delta.x;
                    int new_y = (int)rect.top + (int)delta.y;
                    SetWindowPos(hwnd, nullptr, new_x, new_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
                }
            }

        }ImGui::End();

        // Left Side Menu
        ImGui::SetNextWindowPos(ImVec2(0, 42));
        ImGui::SetNextWindowSize(ImVec2(256, 1024));

        if (ImGui::Begin("Setting Menu", nullptr,   ImGuiWindowFlags_NoResize |
                                                    ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar |
                                                    ImGuiWindowFlags_NoMove))
        {
            static int selected_map_idx = 0;
            std::vector<std::string> map_list = core.GetMapList();
            std::string loaded_map_name = core.GetLoadedMapName();

            // logo
            ImGui::Image((ImTextureID)(intptr_t)core.GetLogoTexture(), ImVec2(232, 232)); // hard coded

            ImGui::SeparatorText("Select Map");
            if (ImGui::BeginCombo("##0", loaded_map_name.c_str()))
            {
                for (int n = 0; n < map_list.size(); n++)
                {
                    const bool is_selected = (selected_map_idx == n);
                    if (ImGui::Selectable(map_list[n].c_str(), is_selected))
                    {
                        selected_map_idx = n;
                        core.LoadNewMap(map_list[n]);
                    }

                    // Set the initial focus when opening the combo
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::NewLine();
            ImGui::SeparatorText("Line of sight");
            if (ImGui::Button("Process", ImVec2(-FLT_MIN, 0.0f)))
            {
                core.ProcessShadowMap(marker_height, target_height);
            }

            // neutral or negative theme
            if (core.IsShadowMapProcessed())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 0.90f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.30f, 0.25f, 0.40f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.45f, 0.35f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.35f, 0.30f, 1.00f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.16f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.16f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.16f, 1.00f));
            }
            if (ImGui::Button("Clear", ImVec2(-FLT_MIN, 0.0f)))
            {
                core.ClearShadowMap();
            }
            ImGui::PopStyleColor(4);

            ImGui::NewLine();
            ImGui::Text("Transparency :");
            ImGui::SliderFloat("##1", &los_transparency, 0.0f, 1.0f);

            ImGui::NewLine();
            ImGui::Text("Marker Height :");
            ImGui::InputFloat("##2", &marker_height, 0.00f, 0.5f, "%.1f m");

            ImGui::SameLine();
            if (ImGui::Button("v##1"))
            {
                ImGui::OpenPopup("HeightPresets##1");
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Presets");
            }
            if (ImGui::BeginPopup("HeightPresets##1"))
            {
                for (auto& [label, value] : height_presets)
                {
                    if (ImGui::Selectable(label))
                        marker_height = value;
                }
                ImGui::EndPopup();
            }
            marker_height = std::clamp(marker_height, 0.0f, 100.0f);

            ImGui::NewLine();
            ImGui::Text("Target Height :");
            ImGui::InputFloat("##3", &target_height, 0.00f, 0.5f, "%.1f m");

            ImGui::SameLine();
            if (ImGui::Button("v##2"))
            {
                ImGui::OpenPopup("HeightPresets##2");
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Presets");
            }
            if (ImGui::BeginPopup("HeightPresets##2"))
            {
                for (auto& [label, value] : height_presets)
                {
                    if (ImGui::Selectable(label))
                        target_height = value;
                }
                ImGui::EndPopup();
            }

            target_height = std::clamp(target_height, 0.0f, 100.0f);

        }ImGui::End();

        // Map & UI Drawing
        ImGui::SetNextWindowPos(ImVec2(260, 42));
        ImGui::SetNextWindowSize(ImVec2(1024, 1024));
        if (ImGui::Begin("Map", nullptr,    ImGuiWindowFlags_NoResize | 
                                            ImGuiWindowFlags_NoCollapse | 
                                            ImGuiWindowFlags_NoTitleBar | 
                                            ImGuiWindowFlags_NoMove | 
                                            ImGuiWindowFlags_NoScrollbar |
                                            ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImVec2 window_pos = ImGui::GetWindowPos();
            ImVec2 cursor_pos = ImGui::GetCursorPos();
            window_pos.x += cursor_pos.x;
            window_pos.y += cursor_pos.y;
            // Use custom sampler to clamp texture
            auto [uv0, uv1] = core.GetUVs();
            ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ImplDX11_SetSampler, g_ClipSampler); // Set custom sampler
            ImGui::Image((ImTextureID)(intptr_t)core.GetMapTexture(), core.GetTextureImageSize(), uv0, uv1);
            if (core.GetShadowTexture()) // if a shadow texture has been generated
            {
                ImGui::SetCursorPos(cursor_pos);
                ImGui::ImageWithBg((ImTextureID)(intptr_t)core.GetShadowTexture(), core.GetTextureImageSize(), uv0, uv1, ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, los_transparency));
            }
            ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ImplDX11_SetSampler, NULL); // Restore
            
            // Draw indicator
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 indicator_pos = core.GetIndicatorPos(window_pos);
            indicator_pos.x += 1;
            indicator_pos.y += 1;
            draw_list->AddCircle(indicator_pos, 10, IM_COL32(0, 0, 0, 100), 32, 1.0f); // shadow circle
            indicator_pos.x -= 1;
            indicator_pos.y -= 1;
            draw_list->AddCircleFilled(indicator_pos, 10, IM_COL32(255, 0, 0, 50), 32);  // red circle
            draw_list->AddCircle(indicator_pos, 10, IM_COL32(255, 0, 0, 200), 32, 1.0f);  //

            // Click to move the indicator
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImVec2 click_px = ImGui::GetIO().MousePos;
                ImVec2 click_local = ImVec2(click_px.x - window_pos.x, click_px.y - window_pos.y);
                core.SetIndicatorPos(click_local);
            }

            // Drag map around
            if (ImGui::IsWindowFocused() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                core.DragBy(delta);
            }

            // Zoom map with mouse wheel
            if (ImGui::IsWindowHovered()) {
                float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) {
                    core.ZoomBy(wheel > 0.0f ? 1.0f : -1.0f);
                }
            }

            // Tips at the bottom right

            #ifdef _DEBUG
            ImGui::SetCursorPos(ImVec2(25, 937));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
            ImGui::TextWrapped("<Left Click> : Drag\n\n<Right Click> : Move Marker\n\n<Scroll Wheel> : Zoom");
            ImGui::PopStyleColor();
            ImGui::SetCursorPos(ImVec2(24, 936));
            ImGui::TextWrapped("<Left Click> : Drag\n\n<Right Click> : Move Marker\n\n<Scroll Wheel> : Zoom");
            #else
            ImGui::SetCursorPos(ImVec2(25, 937));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
            ImGui::TextWrapped("<Left Click> : Drag\n\n<Right Click> : Move Marker\n\n<Scroll Wheel> : Zoom");
            ImGui::PopStyleColor();
            ImGui::SetCursorPos(ImVec2(24, 936));
            ImGui::TextWrapped("<Left Click> : Drag\n\n<Right Click> : Move Marker\n\n<Scroll Wheel> : Zoom");
            #endif

        }ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_ClipSampler) { g_ClipSampler->Release(); g_ClipSampler = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
