/*
* Viry3D
* Copyright 2014-2018 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "json/json.h"
#include "io/File.h"
#include "memory/Memory.h"
#include "graphics/Texture.h"
#include "Compressonator.h"

using namespace Viry3D;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage:\n");
        printf("\tCubeMapCompress.exe input.json\n");
        return 0;
    }

    std::string input = argv[1];
    std::string input_buffer = File::ReadAllText(input.c_str()).CString();

    auto reader = Ref<Json::CharReader>(Json::CharReaderBuilder().newCharReader());
    Json::Value root;
    const char* begin = input_buffer.c_str();
    const char* end = begin + input_buffer.size();
    if (reader->parse(begin, end, &root, nullptr))
    {
        auto cubemap_levels = root["cubemap_levels"];
        auto output = root["output"];

        if (cubemap_levels.isArray() && cubemap_levels.size() > 0 && output.isString())
        {
            int base_width = 0;
            int base_height = 0;
            Vector<Vector<ByteBuffer>> compressed_levels(cubemap_levels.size());

            for (Json::ArrayIndex i = 0; i < cubemap_levels.size(); ++i)
            {
                Json::Value faces = cubemap_levels[i];
                if (faces.size() != 6)
                {
                    return 0;
                }

                compressed_levels[i].Resize(faces.size());
                for (Json::ArrayIndex j = 0; j < faces.size(); ++j)
                {
                    int width;
                    int height;
                    int bpp;
                    ByteBuffer pixels = Texture::LoadImageFromFile(faces[j].asCString(), width, height, bpp);
                    if (pixels.Size() == 0)
                    {
                        return 0;
                    }

                    if (i == 0)
                    {
                        base_width = width;
                        base_height = height;
                    }

                    CMP_Texture src;
                    Memory::Zero(&src, sizeof(src));
                    src.dwSize = sizeof(src);
                    src.dwWidth = width;
                    src.dwHeight = height;
                    src.dwPitch = 0;
                    src.format = CMP_FORMAT_RGBA_8888;
                    src.dwDataSize = pixels.Size();
                    src.pData = pixels.Bytes();

                    CMP_Texture dst;
                    Memory::Zero(&dst, sizeof(dst));
                    dst.dwSize = sizeof(dst);
                    dst.dwWidth = src.dwWidth;
                    dst.dwHeight = src.dwHeight;
                    dst.dwPitch = 0;
                    dst.format = CMP_FORMAT_BC1;
                    ByteBuffer compressed(CMP_CalculateBufferSize(&dst));
                    dst.dwDataSize = compressed.Size();
                    dst.pData = compressed.Bytes();

                    CMP_CompressOptions options;
                    Memory::Zero(&options, sizeof(options));
                    options.dwSize = sizeof(options);
                    options.fquality = 0.05f;
                    options.dwnumThreads = 8;

                    CMP_ERROR cmp_status = CMP_ConvertTexture(&src, &dst, &options, nullptr, 0, 0);
                    if (cmp_status != CMP_OK)
                    {
                        return 0;
                    }

                    compressed_levels[i][j] = compressed;
                }
            }

            // write to ktx

        }
    }

    return 0;
}
