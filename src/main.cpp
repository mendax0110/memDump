#include "../include/memDump.h"

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#ifdef USE_GUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <imgui_impl_win32.cpp>

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

using namespace MemoryUtils;

int main(int argc, char* argv[])
{
    bool useGui = false;
    Memory mem;
    DWORD pid = 0;
    std::string outputFileName;
    std::vector<std::string> searchWords;
    std::string dumpOutput;
    std::stringstream dumpStream;
    std::vector<std::string> searchResults;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--gui")
        {
            useGui = true;
        }
        else if (arg == "-p" || arg == "--pid")
        {
            if (i + 1 < argc)
            {
                pid = std::stoul(argv[++i]);
            }
            else
            {
                std::cerr << "Error: Missing PID after " << arg << "\n";
                return 1;
            }
        }
        else if (arg == "-o" || arg == "--output")
        {
            if (i + 1 < argc)
            {
                outputFileName = argv[++i];
            }
            else
            {
                std::cerr << "Error: Missing output file name after " << arg << "\n";
                return 1;
            }
        }
        else if (arg == "-w" || arg == "--word")
        {
            if (i + 1 < argc)
            {
                searchWords.push_back(argv[++i]);
            }
            else
            {
                std::cerr << "Error: Missing word after " << arg << "\n";
                return 1;
            }
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

#ifdef USE_GUI
    if (useGui)
    {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Memory Dump GUI"), NULL };
        RegisterClassEx(&wc);
        HWND hwnd = CreateWindow(wc.lpszClassName, _T("Memory Dump GUI"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            ImGui::DockSpaceOverViewport();


            ImGui::Begin("Memory Dump Tool", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar);

            static char pid_input[32] = "";
            static char output_file_input[256] = "";
            static char search_word_input[256] = "";

            ImGui::InputText("PID", pid_input, IM_ARRAYSIZE(pid_input));
            ImGui::InputText("Output File", output_file_input, IM_ARRAYSIZE(output_file_input));
            ImGui::InputText("Search Word", search_word_input, IM_ARRAYSIZE(search_word_input));

            if (ImGui::Button("Start Dump"))
            {
                pid = std::stoul(pid_input);
                outputFileName = output_file_input;
                searchWords.clear();
                if (search_word_input[0] != '\0')
                {
                    searchWords.push_back(search_word_input);
                }

                if (outputFileName.empty())
                {
                    dumpStream.str("");
                    dumpStream.clear();
                    mem.scanAndDumpMemory(pid, dumpStream);
                    dumpOutput = dumpStream.str();

                    if (!searchWords.empty())
                    {
                        searchResults.clear();
                        dumpStream.clear();
                        dumpStream.str(dumpOutput);
                        mem.searchInDumpStream(dumpStream, searchWords, searchResults);
                    }
                }
                else
                {
                    std::ofstream ofs(outputFileName);
                    if (ofs.is_open())
                    {
                        mem.scanAndDumpMemory(pid, ofs);
                        ofs.close();
                    }
                    else
                    {
                        std::cerr << "Failed to open output file: " << outputFileName << "\n";
                    }
                }
            }

            if (outputFileName.empty() && !dumpOutput.empty())
            {
                ImGui::Separator();
                ImGui::TextWrapped("Dump Output:");
                ImGui::InputTextMultiline("##dump", &dumpOutput[0], dumpOutput.size(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 30), ImGuiInputTextFlags_ReadOnly);

                if (!searchWords.empty())
                {
                    ImGui::Separator();
                    ImGui::Text("Search Results:");
                    for (const auto& result : searchResults)
                    {
                        ImGui::TextWrapped(result.c_str());
                    }
                }
            }

            ImGui::End();

            ImGui::Render();
            const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            g_pSwapChain->Present(1, 0);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);

        return 0;
    }
#endif

    if (pid == 0)
    {
        std::cerr << "Error: PID is required.\n";
        std::cerr << "Usage: memDump.exe -p <PID> [-o <output_file>] [-w <word_to_search_1> -w <word_to_search_2> ...]\n";
        return 1;
    }

    std::ostream* os = &std::cout;
    std::ofstream ofs;

    if (!outputFileName.empty())
    {
        ofs.open(outputFileName);
        if (ofs.is_open())
        {
            os = &ofs;
        }
        else
        {
            std::cerr << "Failed to open output file: " << outputFileName << "\n";
            return 1;
        }
    }

    mem.scanAndDumpMemory(pid, *os);

    if (ofs.is_open())
    {
        ofs.close();
    }

    if (!searchWords.empty())
    {
        std::string searchFile = outputFileName.empty() ? "dump.txt" : outputFileName;
        mem.searchInDump(searchFile, searchWords);
    }

    return 0;
}

#ifdef USE_GUI
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pd3dDeviceContext);
    if (FAILED(hr))
    {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = NULL;
    if (SUCCEEDED(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)))
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif
