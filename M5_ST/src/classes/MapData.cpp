#define NOMINMAX // issue with std::max otherwise

#include "MapData.h"
#include "imgui.h"
#include "simdjson.h"
#include "ImgHelper.h"
#include <d3d11.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <filesystem>

MapData::MapData(std::string map_file_name, ID3D11Device* g_pd3dDevice) { // load map data (name, texture, heightmap)

    std::filesystem::path map_texture_path = std::filesystem::current_path() / "data" / "maps" / map_file_name / "basemap.png";
    std::filesystem::path heightmap_data_path = std::filesystem::current_path() / "data" / "maps" / map_file_name / "heightmap.json";
    std::filesystem::path map_data_path = std::filesystem::current_path() / "data" / "maps" / map_file_name / "data.json";

    this->map_name = map_file_name;
    LoadTextureFromFile(map_texture_path.string().c_str(),
                        &this->map_texture,
                        &this->map_texture_width,
                        &this->map_texture_height,
                        g_pd3dDevice);
    this->height_map = std::move(JsonToHeightMap(heightmap_data_path.string()));

    this->LoadMapData(map_data_path.string());
}

MapData::~MapData() {
    if (this->map_texture) {
        this->map_texture->Release();
        this->map_texture = nullptr;
    }
    if (this->shadow_texture) {
        this->shadow_texture->Release();
        this->shadow_texture = nullptr;
    }
}

std::string MapData::GetName()
{
    return this->map_name;
}

ID3D11ShaderResourceView* MapData::GetMapTexture()
{
    return this->map_texture;
}

ID3D11ShaderResourceView* MapData::GetShadowTexture()
{
    return this->shadow_texture;
}

void MapData::ProcessShadowTexture(ID3D11Device* g_pd3dDevice, const ImVec2 indicator_pos, const float height_start, const float height_end)
{
    // size
    this->shadow_texture_height = (int)this->height_map.size();
    this->shadow_texture_width = (int)this->height_map.empty() ? 0 : (int)this->height_map[0].size();

    // indicator uv to xy
    int x_pos = static_cast<int>(std::floor(indicator_pos.x * this->shadow_texture_width));;
    int y_pos = static_cast<int>(std::floor(indicator_pos.y * this->shadow_texture_height));;

    // declare shadowresult
    std::vector<std::vector<int>> shadow_result;
    shadow_result.resize(this->shadow_texture_height);
    for (int y = 0; y < this->shadow_texture_height; ++y) {
        shadow_result[y].resize(this->shadow_texture_width, 0);
    }

    // process
    pixel_shadow_2d(x_pos, y_pos, this->height_map, shadow_result, height_start, height_end, this->fog_distance);

    // turn into texture
    // release previous shadowmap
    if (this->shadow_texture) {
        this->shadow_texture->Release();
        this->shadow_texture = nullptr;
    }

    unsigned char* rgba_bytes = new unsigned char[this->shadow_texture_width * this->shadow_texture_height * 4];
    // populate rgba bytes
    for (int i = 0; i < this->shadow_texture_width * this->shadow_texture_height; i++) {
        int x = i % this->shadow_texture_width;
        int y = i / this->shadow_texture_width;
        SetPixel(rgba_bytes, this->shadow_texture_width, this->shadow_texture_height, x, y, 0, 0, 0, shadow_result[y][x]);
    }
    CreateTextureFromRawBytes(rgba_bytes, &this->shadow_texture, this->shadow_texture_width, this->shadow_texture_height, g_pd3dDevice);
}

void MapData::ClearShadowTexture()
{
    if (this->shadow_texture) {
        this->shadow_texture->Release();
        this->shadow_texture = nullptr;
    }
}

bool MapData::IsShadowMapProcessed()
{
    return shadow_texture != nullptr;
}

int MapData::GetMapSize()
{
    return this->map_size;
}

std::vector<std::vector<float>> MapData::JsonToHeightMap(const std::string& map_file_path) {

    simdjson::ondemand::parser parser;

    // Charge le fichier JSON avec padding requis
    simdjson::padded_string json_data = simdjson::padded_string::load(map_file_path);

    simdjson::ondemand::document doc = parser.iterate(json_data);

    std::vector<std::vector<float>> result;

    // Le document est un tableau de lignes (2D)
    for (simdjson::ondemand::array row : doc.get_array()) {
        std::vector<float> current_row;
        for (simdjson::ondemand::value val : row) {
            current_row.push_back(float(val.get_double()));
        }
        result.push_back(std::move(current_row));
    }

    return result;
}

void MapData::LoadMapData(const std::string& map_data_path)
{
    simdjson::ondemand::parser parser;

    // Charge le fichier JSON avec padding requis
    simdjson::padded_string json_data = simdjson::padded_string::load(map_data_path);

    simdjson::ondemand::document doc = parser.iterate(json_data);

    //this->fog_distance = int64_t(doc["fog"]); will uncomment when all the datas are validated
    this->fog_distance = 400;
    this->map_size = int64_t(doc["size"]);
}

float MapData::lerp(const float a, const float b, const float t)
{
    return a + t * (b - a);
}

void MapData::calculate_dda(const int x1, const int y1, const int x2, const int y2, int& steps, float& x_incr, float& y_incr)
{
    if (x1 == x2 && y1 == y2) {
        steps = 1;
        x_incr = 0.0f;
        y_incr = 0.0f;
        return;
    }

    int dx = x2 - x1;
    int dy = y2 - y1;
    steps = std::max(std::abs(dx), std::abs(dy));

    x_incr = static_cast<float>(dx) / steps;
    y_incr = static_cast<float>(dy) / steps;
}

void MapData::pixel_shadow_2d(int x_pos, int y_pos, const std::vector<std::vector<float>>& heightmap_data, std::vector<std::vector<int>>& shadowmap, const float height_start, const float height_end, const int fog_distance)
{
    int height = (int)shadowmap.size();
    if (height == 0) return;
    int width = (int)shadowmap[0].size();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++)
        {
            int xy_steps;
            float x_incr, y_incr;
            calculate_dda(x_pos, y_pos, x, y, xy_steps, x_incr, y_incr);

            bool is_shadow = false;

            for (int step = 0; step < xy_steps; step++)
            {
                int current_x = static_cast<int>(std::round(x_pos + (x_incr * step)));
                int current_y = static_cast<int>(std::round(y_pos + (y_incr * step)));

                if (!is_shadow)
                {
                    float step_ratio = static_cast<float>(step) / xy_steps;
                    float ray_height = lerp(heightmap_data[y_pos][x_pos] + height_start, heightmap_data[y][x] + height_end, step_ratio);
                    float ground_height = heightmap_data[current_y][current_x];

                    if (ray_height < ground_height)
                    {
                        is_shadow = true;
                    }
                }

                if (is_shadow)
                {
                    break;
                }
            }

            // fog processing
            float distance_to_center = std::sqrt((x_pos - x) * (x_pos - x) + (y_pos - y) * (y_pos - y)); // Euclidean distance
            float fog_ratio = distance_to_center / fog_distance;
            float test = fog_ratio * 255;
            shadowmap[y][x] = std::clamp(static_cast<int>(is_shadow) * 255 + static_cast<int>(0 * 255), 0, 255);

            // will replace the line above whenever all the fog values per maps are correctly set
            // shadowmap[y][x] = std::clamp(static_cast<int>(is_shadow) * 255 + static_cast<int>(fog_ratio * 255), 0, 255);
        }
    }
}
