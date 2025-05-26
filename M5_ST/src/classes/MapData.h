#pragma once
#include "imgui.h"
#include <string>
#include <d3d11.h>
#include <vector>

class MapData {
public:
    MapData(std::string map_file_name, ID3D11Device* g_pd3dDevice);
    ~MapData();
    std::string GetName();
    ID3D11ShaderResourceView* GetMapTexture();
    ID3D11ShaderResourceView* GetShadowTexture();
    void ProcessShadowTexture(ID3D11Device* g_pd3dDevice, const ImVec2 indicator_pos, const float height_start, const float height_end);
    void ClearShadowTexture();
    bool IsShadowMapProcessed();
    int GetMapSize();

private:
    std::string                         map_name;
    ID3D11ShaderResourceView*           map_texture = nullptr;
    int                                 map_texture_width = 0;
    int                                 map_texture_height = 0;
    ID3D11ShaderResourceView*           shadow_texture = nullptr;
    int                                 shadow_texture_width = 0;
    int                                 shadow_texture_height = 0;
    std::vector<std::vector<float>>     height_map;
    int                                 map_size;
    int                                 fog_distance;

    std::vector<std::vector<float>> JsonToHeightMap(const std::string& map_file_path);
    void LoadMapData(const std::string& map_file_path);

    float lerp(const float a, const float b, const float t);
    void calculate_dda(const int x1, const int y1, const int x2, const int y2, int& steps, float& x_incr, float& y_incr);
    void pixel_shadow_2d(int x_pos, int y_pos, const std::vector<std::vector<float>>& heightmap_data, std::vector<std::vector<int>>& shadowmap, const float height_start, const float height_end, const int fog_distance);
};