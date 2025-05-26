#pragma once
#include "MapData.h"
#include "imgui.h"
#include <d3d11.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <utility>

class Core {
public:
    Core(ID3D11Device* g_pd3dDevice);
    ~Core();

    bool IsMapLoaded();
    std::vector<std::string> GetMapList();
    std::string GetLoadedMapName();
    void LoadNewMap(std::string map_file_name);
    ID3D11ShaderResourceView* GetMapTexture();
    ID3D11ShaderResourceView* GetShadowTexture();
    ID3D11ShaderResourceView* GetLogoTexture();
    ImVec2 GetTextureImageSize();
    std::pair<ImVec2, ImVec2> GetUVs();
    void ZoomBy(float increment);
    void DragBy(ImVec2 delta);
    ImVec2 GetIndicatorPos(ImVec2 window_pos_delta);
    void SetIndicatorPos(ImVec2 click_pos_pxl);
    void ProcessShadowMap(const float height_start, const float height_end);
    void ClearShadowMap();
    bool IsShadowMapProcessed();

private:
    ID3D11Device*               g_pd3dDevice;
    float                       map_zoom;
    ImVec2                      map_texture_render_size;
    ImVec2                      map_uv_offset;
    ImVec2                      map_uv0;
    ImVec2                      map_uv1;
    MapData*                    loaded_map = nullptr;
    std::vector<std::string>    maps_list;
    ImVec2                      indicator_pos;
    ID3D11ShaderResourceView*   logo_texture = nullptr;
    int                         logo_texture_width = 232;   // hardcoded
    int                         logo_texture_height = 232;  //


    bool CheckDirectoryStructure();
    void GetMapInFiles();
    void ProcessUVs();
};