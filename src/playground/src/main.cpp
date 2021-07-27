#include <iostream>

#include <appfw/console/console_system.h>
#include <appfw/console/std_console.h>
#include <appfw/console/con_item.h>
#include <appfw/dbg.h>
#include <appfw/init.h>
#include <appfw/appfw.h>
#include <appfw/command_line.h>
#include <appfw/timer.h>
#include <app_base/texture_block.h>
#include <stb_image.h>
#include "bmplib.hpp"

static bool s_IsRunning = false;

ConVar<std::string> test_cvar("test_cvar", "test", "A test console variable.");
ConVar<int> test_cvar_int("test_cvar_int", 1, "A test integer console variable.");
ConVar<float> test_cvar_float("test_cvar_float", 3.1415926535f, "A test integer console variable.");
ConVar<bool> test_cvar_bool("test_cvar_bool", true, "sdfsdfsdf");

//ConVar<bool> run("run", true, "Enables/disables the app");

ConCommand quit_cmd("quit", "Exits the app", [](auto &) { s_IsRunning = false; });

float filter_cubic(float x) {
    constexpr float B = 0;
    constexpr float C = 0.5f;

    float absx = std::abs(x);
    float val = 0;

    if (absx < 1.0f) {
        val = (12 - 9 * B - 6 * C) * absx * absx * absx;
        val += (-18 + 12 * B + 6 * C) * absx * absx;
        val += 6 - 2 * B;
    } else if (std::abs(x) < 2.0f) {
        val = (-B - 6 * C) * absx * absx * absx;
        val += (6 * B + 30 * C) * absx * absx;
        val += (-12 * B - 48 * C) * absx;
        val += 8 * B + 24 * C;
    } else {
        return 0;
    }

    return val / 6;
}

float filter_linear(float x) {
    if (x < -1) {
        return 0;
    }

    if (x < 0) {
        return 1.0f + x;
    }

    if (x < 1) {
        return 1.0f - x;
    }

    return 0;
}

float filter(float x) {
    return filter_cubic(x);
}

int realMain(int argc, char **argv) {
    appfw::InitComponent appfwInit (appfw::InitOptions().setArgs(argc, argv));

    // Load the image
    constexpr float GAMMA = 2.2f;
    int origWide = 0, origTall = 0;
    std::vector<glm::vec3> origData;
    {
        int chan = 0;
        glm::u8vec3 *bdata = reinterpret_cast<glm::u8vec3 *>(
            stbi_load("smile_20.png", &origWide, &origTall, &chan, 3));

        if (!bdata) {
            printe("Failed to open");
            return -1;
        }

        origData.resize(origWide * origTall);

        for (int i = 0; i < origWide * origTall; i++) {
            origData[i] = glm::vec3(bdata[i]) / 255.0f;
            origData[i] = glm::pow(origData[i], glm::vec3(GAMMA));
        }
    }

    // Resize image
    int newWide = 100;
    int newTall = 100;
    std::vector<glm::vec3> newData(newWide * newTall);

#if 0
    float ky = 1.0f / (origTall - 1);
    float kx = 1.0f / (origWide - 1);

    for (int y = 0; y < newTall; y++) {
        float fy = y / (float)(newTall - 1);

        int origSamplesY[4];
        origSamplesY[1] = (int)(fy * (origTall - 1));
        origSamplesY[0] = origSamplesY[1] - 1;
        origSamplesY[2] = origSamplesY[1] + 1;
        origSamplesY[3] = origSamplesY[1] + 2;

        for (int x = 0; x < newWide; x++) {
            float fx = x / (float)(newWide - 1);
            glm::vec3 &out = newData[y * newWide + x];

            int origSamplesX[4];
            origSamplesX[1] = (int)(fx * (origWide - 1));
            origSamplesX[0] = origSamplesX[1] - 1;
            origSamplesX[2] = origSamplesX[1] + 1;
            origSamplesX[3] = origSamplesX[1] + 2;

            for (int y2 = 0; y2 < 4; y2++) {
                for (int x2 = 0; x2 < 4; x2++) {
                    float samplex = origSamplesX[x2] / (float)(origWide - 1);
                    float sampley = origSamplesY[y2] / (float)(origTall - 1);
                    float filterValX = filter(kx * (samplex - fx));
                    float filterValY = filter(ky * (sampley - fy));

                    int origXPos = std::clamp(origSamplesX[x2], 0, origWide - 1);
                    int origYPos = std::clamp(origSamplesY[y2], 0, origTall - 1);
                    const glm::vec3 &in = origData[origYPos * origWide + origXPos];
                    out += (filterValX * filterValY) * in;
                }
            }
        }
    }
#else
    auto origXtoFloat = [&](int pos) { return (0.5f + pos) / origWide; };
    auto origYtoFloat = [&](int pos) { return (0.5f + pos) / origTall; };
    auto newXtoFloat = [&](int pos) { return (0.5f + pos) / newWide; };
    auto newYtoFloat = [&](int pos) { return (0.5f + pos) / newTall; };

    float origPixelXLen = 1.0f / origWide;
    float origPixelYLen = 1.0f / origTall;
    float newPixelXLen = 1.0f / newWide;
    float newPixelYLen = 1.0f / newTall;

    float kx = (float)std::min(origWide, newWide);
    float ky = (float)std::min(origTall, newTall);
    float samplerDeltaX = std::max(origPixelXLen, newPixelXLen);
    float samplerDeltaY = std::max(origPixelYLen, newPixelYLen);

    for (int newy = 0; newy < newTall; newy++) {
        float newsampley = newYtoFloat(newy);
        float lefty = newsampley - 2 * origPixelYLen;
        float righty = newsampley + 2 * origPixelYLen;

        for (int newx = 0; newx < newWide; newx++) {
            glm::vec3 &out = newData[newy * newWide + newx];

            float newsamplex = newXtoFloat(newx);
            float leftx = newsamplex - 2 * samplerDeltaX;
            float rightx = newsamplex + 2 * samplerDeltaY;

            int sampleCount = 0;
            float weightSum = 0;

            for (int origy = 0; origy < origTall; origy++) {
                float origsampley = origYtoFloat(origy);

                if (origsampley > lefty && origsampley < righty) {
                    for (int origx = 0; origx < origWide; origx++) {
                        float origsamplex = origXtoFloat(origx);

                        if (origsamplex > leftx && origsamplex < rightx) {
                            float filterValX = filter(kx * (origsamplex - newsamplex));
                            float filterValY = filter(ky * (origsampley - newsampley));
                            float weight = filterValX * filterValY;
                            const glm::vec3 &in = origData[origy * origWide + origx];
                            out += weight * in;
                            weightSum += weight;
                            sampleCount++;
                        }
                    }
                }
            }

            out /= weightSum;

            //printi("{} {}: {}", newx, newy, sampleCount);
        }
    }
#endif

    // Save
#if 1
    bitMapImage<32> outImage(newWide, newTall);

    for (int y = 0; y < newTall; y++) {
        for (int x = 0; x < newWide; x++) {
            glm::vec3 out = newData[y * newWide + x];
            //glm::vec3 out = glm::clamp(in, glm::vec3(0), glm::vec3(1));

            if (out.r > 1) {
                out.r = 1;
            }

            if (out.g > 1) {
                out.g = 1;
            }

            if (out.b > 1) {
                out.b = 1;
            }

            out = glm::pow(out, glm::vec3(1.0f / GAMMA)) * 255.0f;

            outImage.setPixel(x, newTall - y - 1, Color((int)out.r, (int)out.g, (int)out.b));
        }
    }
#else
    bitMapImage<32> outImage(origWide, origTall);
    for (int y = 0; y < origTall; y++) {
        for (int x = 0; x < origWide; x++) {
            glm::vec3 &in = origData[y * origWide + x];
            glm::vec3 out = glm::pow(in, glm::vec3(1.0f / GAMMA)) * 255.0f;
            outImage.setPixel(x, origWide - y - 1, Color((int)out.r, (int)out.g, (int)out.b));
        }
    }
#endif

    outImage.saveToFile("resized.bmp");

    printd("Quitting...");
    return 0;
}

int main(int argc, char **argv) { return realMain(argc, argv); }
