#include "Core.h"
#include "MapData.h"
#include "ImgHelper.h"
#include "imgui.h"
#include <d3d11.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <fstream>

Core::Core(ID3D11Device* g_pd3dDevice) {
    // dx device
    this->g_pd3dDevice = g_pd3dDevice;

    // default map display
    this->map_zoom = 1.0f;
    this->map_texture_render_size = ImVec2(1000, 1000);
    this->map_uv_offset = ImVec2(0.0f, 0.0f);

    // check file structure
    if (this->CheckDirectoryStructure())
    {
        // list of all the maps in the file system
        GetMapInFiles();

        if (this->maps_list.size() > 0)
        {
            Core::LoadNewMap(this->maps_list[0]); // the tool starts with the first map in the list
        }

        // Load top left logo texture
        // Not a dynamic image load by design
        CreateTextureFromRawBytes(logoST, &this->logo_texture, this->logo_texture_width, this->logo_texture_height, g_pd3dDevice);
    }

    // indicator is init at the center 0.5 0.5 of the map
    this->indicator_pos = ImVec2(0.5f, 0.5f);
}

Core::~Core() {
    if (this->loaded_map) {
        delete this->loaded_map;
        this->loaded_map = nullptr;
    }
}

bool Core::IsMapLoaded() {
    return this->loaded_map != NULL;
}

std::vector<std::string> Core::GetMapList()
{
    return this->maps_list;
}

std::string Core::GetLoadedMapName()
{
    return this->loaded_map->GetName();
}

void Core::LoadNewMap(std::string map_file_name)
{
    if (std::find(this->maps_list.begin(), this->maps_list.end(), map_file_name) != this->maps_list.end()) {
        if (this->loaded_map) {
            delete this->loaded_map;
            this->loaded_map = nullptr;
        }
        this->loaded_map = new MapData(map_file_name, g_pd3dDevice);
    }

    this->map_zoom = 1.0f;
    this->map_texture_render_size = ImVec2(1000, 1000);
    this->map_uv_offset = ImVec2(0.5f, 0.5f);

    ProcessUVs();

    // indicator is init at the center 0.5 0.5 of the map
    this->indicator_pos = ImVec2(0.5f, 0.5f);
}

ID3D11ShaderResourceView* Core::GetMapTexture()
{
    if (loaded_map)
    {
        return loaded_map->GetMapTexture();
    }
    else
    {
        return nullptr;
    }
}

ID3D11ShaderResourceView* Core::GetShadowTexture()
{
    if (loaded_map)
    {
        return loaded_map->GetShadowTexture();
    }
    else
    {
        return nullptr;
    }
}

ID3D11ShaderResourceView* Core::GetLogoTexture()
{
    return this->logo_texture;
}

ImVec2 Core::GetTextureImageSize()
{
    return this->map_texture_render_size;
}

std::pair<ImVec2, ImVec2> Core::GetUVs()
{
    return { this->map_uv0, this->map_uv1 };
}

void Core::ZoomBy(float increment)
{
    this->map_zoom = std::clamp(this->map_zoom + increment, 1.0f, 10.0f); // hardcoded values
    this->ProcessUVs();
}

void Core::DragBy(ImVec2 delta_pxl)
{
    ImVec2 delta_uv;
    delta_uv.x = delta_pxl.x / map_texture_render_size.x;
    delta_uv.y = delta_pxl.y / map_texture_render_size.y;

    this->map_uv_offset.x = std::clamp(this->map_uv_offset.x - delta_uv.x * 1 / this->map_zoom, 0.0f, 1.0f); // hardcoded values
    this->map_uv_offset.y = std::clamp(this->map_uv_offset.y - delta_uv.y * 1 / this->map_zoom, 0.0f, 1.0f); // hardcoded values
    this->ProcessUVs();
}

ImVec2 Core::GetIndicatorPos(ImVec2 window_pos_delta)
{
    ImVec2 rel_uv((indicator_pos.x - this->map_uv0.x) * map_zoom, (indicator_pos.y - this->map_uv0.y) * map_zoom);

    return ImVec2(rel_uv.x * map_texture_render_size.x + window_pos_delta.x, rel_uv.y * map_texture_render_size.y + window_pos_delta.y);
}

void Core::SetIndicatorPos(const ImVec2 click_pos_pxl)
{
    float half_uv = 0.5f / this->map_zoom;
    ImVec2 uv_range = ImVec2(1.0f / this->map_zoom, 1.0f / this->map_zoom);
    ImVec2 px_ratio = ImVec2(click_pos_pxl.x / this->map_texture_render_size.x, click_pos_pxl.y / this->map_texture_render_size.y);

    this->indicator_pos.x = std::clamp(map_uv0.x + px_ratio.x * uv_range.x, 0.0f, 1.0f); // hardcoded values
    this->indicator_pos.y = std::clamp(map_uv0.y + px_ratio.y * uv_range.y, 0.0f, 1.0f); // hardcoded values
}

void Core::ProcessShadowMap(const float height_start, const float height_end)
{
    if (this->loaded_map)
    {
        this->loaded_map->ProcessShadowTexture(this->g_pd3dDevice, this->indicator_pos, height_start, height_end);
    }
}

void Core::ClearShadowMap()
{
    if (this->loaded_map)
    {
        this->loaded_map->ClearShadowTexture();
    }
}

bool Core::IsShadowMapProcessed()
{
    if (this->loaded_map)
    {
        return this->loaded_map->IsShadowMapProcessed();
    }

    return false;
}

bool Core::CheckDirectoryStructure()
{
    std::filesystem::path current_dir = std::filesystem::current_path();
    std::filesystem::path data_dir = current_dir / "data";
    if (!std::filesystem::exists(data_dir)) {
        return false;
    }

    std::filesystem::path maps_dir = data_dir / "maps";
    if (!std::filesystem::exists(maps_dir)) {
        return false;
    }

    return true;
}

void Core::GetMapInFiles() {
    std::filesystem::path data_path = std::filesystem::current_path() / "data" / "maps";
    std::vector<std::string> subfolders;
    for (const auto& entry : std::filesystem::directory_iterator(data_path)) {
        if (entry.is_directory()) {
            subfolders.push_back(entry.path().filename().string());
        }
    }
    this->maps_list = subfolders;
}

void Core::ProcessUVs()
{
    float half_uv = 0.5f / this->map_zoom;

    this->map_uv0 = ImVec2(this->map_uv_offset.x - half_uv, this->map_uv_offset.y - half_uv);
    this->map_uv1 = ImVec2(this->map_uv_offset.x + half_uv, this->map_uv_offset.y + half_uv);
}