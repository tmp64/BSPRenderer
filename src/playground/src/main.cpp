#include <iostream>

#include <appfw/console/console_system.h>
#include <appfw/console/std_console.h>
#include <appfw/console/con_item.h>
#include <appfw/dbg.h>
#include <appfw/init.h>
#include <appfw/appfw.h>
#include <appfw/command_line.h>
#include <appfw/timer.h>
#include <renderer/texture_block.h>
#include <renderer/stb_image.h>
#include "bmplib.hpp"

static bool s_IsRunning = false;

ConVar<std::string> test_cvar("test_cvar", "test", "A test console variable.");
ConVar<int> test_cvar_int("test_cvar_int", 1, "A test integer console variable.");
ConVar<float> test_cvar_float("test_cvar_float", 3.1415926535f, "A test integer console variable.");
ConVar<bool> test_cvar_bool("test_cvar_bool", true, "sdfsdfsdf");

//ConVar<bool> run("run", true, "Enables/disables the app");

ConCommand quit_cmd("quit", "Exits the app", [](auto &) { s_IsRunning = false; });

void testBlockByte() {
    struct Img {
        glm::u8vec3 *data;
        int wide;
        int tall;
    };

    auto fnLoadImg = [](const char *name) {
        Img img;
        int chan;
        img.data = reinterpret_cast<glm::u8vec3 *>(stbi_load(name, &img.wide, &img.tall, &chan, 3));
        AFW_ASSERT(img.data);
        return img;
    };

    Img test1 = fnLoadImg("test1.png");
    Img test2 = fnLoadImg("test2.png");

    int x, y;

    // constexpr int SIZE = 128;
    constexpr int SIZE = 1024;

    TextureBlock<glm::u8vec3> block(SIZE, SIZE);
    /*AFW_ASSERT(block.insert(test1.data, test1.wide, test1.tall, x, y, 0));
    printi("test1: {} {}", x, y);
    AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    printi("test2: {} {}", x, y);
    AFW_ASSERT(block.insert(test1.data, test1.wide, test1.tall, x, y, 0));
    printi("test1: {} {}", x, y);
    AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    printi("test2: {} {}", x, y);
    //AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    //printi("test2: {} {}", x, y);
    //AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    //printi("test2: {} {}", x, y);
    AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    printi("test2: {} {}", x, y);
    AFW_ASSERT(block.insert(test1.data, test1.wide, test1.tall, x, y, 0));
    printi("test1: {} {}", x, y);
    //AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    //printi("test2: {} {}", x, y);*/

    appfw::Timer timer;
    timer.start();

    while (block.insert(test2.data, test2.wide, test2.tall, x, y, 5))
        ;

    while (block.insert(test1.data, test1.wide, test1.tall, x, y, 5))
        ;

    timer.stop();
    printd("Time: {:.3f} ms", timer.dseconds() * 1000);

    bitMapImage<32> out(SIZE, SIZE);

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            auto c = block.getData()[i * SIZE + j];
            out.setPixel(j, SIZE - i - 1, Color(c.r, c.g, c.b));
        }
    }

    out.saveToFile("out_byte.bmp");
}

void testBlockFloat() {
    struct Img {
        std::vector<glm::vec3> data;
        int wide;
        int tall;
    };

    auto fnLoadImg = [](const char *name) {
        Img img;
        int chan;
        glm::u8vec3 *bdata = reinterpret_cast<glm::u8vec3 *>(stbi_load(name, &img.wide, &img.tall, &chan, 3));
        img.data.resize(img.wide * img.tall);

        for (int i = 0; i < img.wide * img.tall; i++) {
            img.data[i] = glm::vec3(bdata[i]);
        }

        return img;
    };

    Img test1 = fnLoadImg("test1.png");
    Img test2 = fnLoadImg("test2.png");

    int x, y;

    // constexpr int SIZE = 128;
    constexpr int SIZE = 1024;

    TextureBlock<glm::vec3> block(SIZE, SIZE);
    /*AFW_ASSERT(block.insert(test1.data, test1.wide, test1.tall, x, y, 0));
    printi("test1: {} {}", x, y);
    AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    printi("test2: {} {}", x, y);
    AFW_ASSERT(block.insert(test1.data, test1.wide, test1.tall, x, y, 0));
    printi("test1: {} {}", x, y);
    AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    printi("test2: {} {}", x, y);
    //AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    //printi("test2: {} {}", x, y);
    //AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    //printi("test2: {} {}", x, y);
    AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    printi("test2: {} {}", x, y);
    AFW_ASSERT(block.insert(test1.data, test1.wide, test1.tall, x, y, 0));
    printi("test1: {} {}", x, y);
    //AFW_ASSERT(block.insert(test2.data, test2.wide, test2.tall, x, y, 0));
    //printi("test2: {} {}", x, y);*/

    appfw::Timer timer;
    timer.start();

    while (block.insert(test2.data.data(), test2.wide, test2.tall, x, y, 5))
        ;

    while (block.insert(test1.data.data(), test1.wide, test1.tall, x, y, 5))
        ;

    timer.stop();
    printd("Time: {:.3f} ms", timer.dseconds() * 1000);

    bitMapImage<32> out(SIZE, SIZE);

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            auto c = block.getData()[i * SIZE + j];
            out.setPixel(j, SIZE - i - 1, Color((int)c.r, (int)c.g, (int)c.b));
        }
    }

    out.saveToFile("out_float.bmp");
}

int realMain(int argc, char **argv) {
    appfw::InitComponent appfwInit (appfw::InitOptions().setArgs(argc, argv));

    printw("Test");

    /*try {
        // --arg1 "val1 with spaces" --fl1 -fA +cmd cmdarg
        int argc = 7;
        char *argv[] = {"exec_name", "--arg1", "val1 with spaces", "--fl1", "-fA", "+cmd", "cmdarg"};
        appfw::CommandLine cmd;
        cmd.parseCommandLine(argc, argv);
        printi("parsed");
    }
    catch (const std::exception &e) {
        printe("{}", e.what());
    }*/
    

    /*s_IsRunning = true;

    while (s_IsRunning) {
        appfw::init::mainLoopTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
    }*/

    testBlockByte();
    testBlockFloat();

    printd("Quitting...");
    return 0;
}

int main(int argc, char **argv) { return realMain(argc, argv); }
