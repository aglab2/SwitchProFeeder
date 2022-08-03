#include "EmuController.h"
#include "SwitchPro.h"
#include "Win.h"

#include <iostream>
#include <filesystem>

#include "Mapping.h"

#ifdef _DEBUG
#include "debug.h"
static void test()
{
    Mapping::Debug::test();
}
#endif

static bool Running = true;
BOOL WINAPI CtrlHandler(DWORD event)
{
    if (event == CTRL_CLOSE_EVENT)
    {
        Running = false;
        Sleep(20000);
        return TRUE;
    }
    return FALSE;
}


enum FeederErrors
{
    FE_OK = 0,
    FE_CONFIG_INVALID,
    FE_CONFIG_READ,
    FE_CONFIG_PARSE,
    FE_EMU_LIB,
    FE_CONTROLLER_EXCEPTION,
    FE_NO_CONTROLLER,
};

static void die(FeederErrors err, const char* why)
{
    printf("%s\n", why);
    getchar();
    while (0xA != getchar());
    exit(err);
}

constexpr int Procon_ID  = 0x2009;
constexpr int NintendoID = 0x057E;

int main()
{
#ifdef _DEBUG
    // test();
#endif

    std::vector<std::filesystem::path> cfgPaths;
    std::filesystem::path exePath(Win::ExecutablePath());
    auto exeDir = exePath.parent_path();
    for (const auto& entry : std::filesystem::directory_iterator(exeDir))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".yaml")
            cfgPaths.push_back(entry.path());
    }

    if (cfgPaths.empty())
        die(FE_CONFIG_INVALID, "No config files detected!");

    printf("Pick config file\n");
    for (int i = 0; i < cfgPaths.size(); i++)
    {
        auto& cfgPath = cfgPaths[i];
        wprintf(L"%d) %s\n", i + 1, cfgPath.filename().stem().c_str());
    }

    long cfgId = -1;
    do
    {
        printf("Enter config file NUMBER (from 1 to %lld): \n", cfgPaths.size());
        const char cfgIdStr[10] = {};
        scanf_s("%9s", cfgIdStr, (unsigned)_countof(cfgIdStr));

        cfgId = strtol(cfgIdStr, nullptr, 10);
    } 
    while (cfgId <= 0 || cfgId > cfgPaths.size());

    auto cfgPath = cfgPaths[cfgId - 1];

    YAML::Node node;
    try
    {
        node = YAML::LoadFile(cfgPath.u8string());
    }
    catch (...)
    {
        die(FE_CONFIG_READ, "Failed to read config");
    }

    Mapping::Mappers mappers;
    try
    {
        mappers = node.as<Mapping::Mappers>();
    }
    catch (const std::exception& ex)
    {
        printf("%s\n", ex.what());
        die(FE_CONFIG_PARSE, "Failed to parse config");
    }
    catch (...)
    {
        die(FE_CONFIG_PARSE, "Failed to parse config");
    }

    std::vector<SwitchPro::HidDevice> proControllers;
    unsigned char port = 0;
    {
        hid_device_info* devs = hid_enumerate(NintendoID, Procon_ID);
        const hid_device_info* iter = devs;
        do 
        {
            if (iter != nullptr)
            {
                if (iter->product_id == Procon_ID)
                {
                    try 
                    {
                        proControllers.emplace_back(SwitchPro::HidDevice(port++));
                        proControllers.back().Start(iter);
                    }
                    catch (const std::exception& ex) 
                    {
                        printf("Caught exception connecting to controller: %s\n", ex.what());
                        die(FE_CONTROLLER_EXCEPTION, "Exception");
                    }
                }
                iter = iter->next;
            }
        } 
        while (iter != nullptr && port < 4);

        hid_free_enumeration(devs);
    }

    if (proControllers.empty())
    {
        die(FE_NO_CONTROLLER, "No controllers detected!");
    }

    auto libEmu = Emu::Lib();
    if (!libEmu.Usable())
        die(FE_EMU_LIB, "Failed to allocate ViGEm");

    if (!libEmu.Connect())
        die(FE_EMU_LIB, "Failed to detect ViGEm");

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    std::vector<Emu::Device> emuControllers;
    int i = 0;
    for (auto& c : proControllers)
    {
        emuControllers.emplace_back(libEmu);
        printf("Controller %d is plugged!\n", i + 1);
        emuControllers.back().Connect();
        i++;
    }

    printf("Feeder is running!\n");
    while (Running)
    {
        i = 0;
        for (auto& procon : proControllers)
        {
            SwitchPro::Controller input;
            bool ok = procon.Read(input);
            if (ok)
            {
                // 
                X360::Controller emuControllersInputs = {};
                Mapping::Map(mappers, input, emuControllersInputs);
                emuControllers[i].Update(emuControllersInputs);
            }
        }
    }

    for (auto& c : proControllers)
    {
        c.Stop();
    }

    return 0;
}