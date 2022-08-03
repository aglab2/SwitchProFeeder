#include "debug.h"

#ifdef _DEBUG

#include "Mapping.h"
#include "MappingImpl.h"

#include "SwitchProController.h"
#include "X360Controller.h"

#include "ControllerInterfaceImpl.h"

#include "Win.h"

#include <filesystem>
#include <fstream>

namespace Mapping
{
    namespace Debug
    {
#define SPRO_BUTTON(x) std::make_shared<SwitchPro::Button>(SwitchPro::Buttons::x)
#define X360_BUTTON(x) std::make_shared<X360     ::Button>(X360     ::Buttons::x)
        const Mappers Mapper = {
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(A), X360_BUTTON(A)),
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(B), X360_BUTTON(X)),
            //std::make_shared<Digital::Mapper>(SwitchPro_BUTTON(X), X360_BUTTON(B)),
            //std::make_shared<Digital::Mapper>(SwitchPro_BUTTON(Y), X360_BUTTON(Y)),

            std::make_shared<Digital::Mapper>(SPRO_BUTTON(DpadLeft),  X360_BUTTON(DpadLeft)),
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(DpadRight), X360_BUTTON(DpadRight)),
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(DpadDown),  X360_BUTTON(DpadDown)),
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(DpadUp),    X360_BUTTON(DpadUp)),

            std::make_shared<Digital::Mapper>(SPRO_BUTTON(Plus) , X360_BUTTON(Start)),
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(Minus), X360_BUTTON(Back)),

            std::make_shared<Digital::Mapper>(SPRO_BUTTON(R), X360_BUTTON(R)),
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(L), X360_BUTTON(L)),

            // std::make_shared<Digital::Mapper>(std::make_shared<SwitchPro::Axis>(SwitchPro::Axises::RightTrigger, ControllerInterface::AxisComparerType::More, 120), X360_BUTTON(R)),
            // std::make_shared<Digital::Mapper>(std::make_shared<SwitchPro::Axis>(SwitchPro::Axises::LeftTrigger,  ControllerInterface::AxisComparerType::More, 120), X360_BUTTON(L)),

            std::make_shared<Analog::LinearStickMapper>(SwitchPro::LinearConverter(SwitchPro::Axises::LeftStickX, 0x80, 100), X360::ThumbsConverter(X360::Thumbs::LeftX, 0, 32000)),
            std::make_shared<Analog::LinearStickMapper>(SwitchPro::LinearConverter(SwitchPro::Axises::LeftStickY, 0x6d, 100), X360::ThumbsConverter(X360::Thumbs::LeftY, 0, 32000)),

            std::make_shared<Analog::LinearStickMapper>(SwitchPro::LinearConverter(SwitchPro::Axises::RightStickX, 0x80, 100), X360::ThumbsConverter(X360::Thumbs::RightX, 0, 32000)),
            std::make_shared<Analog::LinearStickMapper>(SwitchPro::LinearConverter(SwitchPro::Axises::RightStickY, 0x85, 100), X360::ThumbsConverter(X360::Thumbs::RightY, 0, 32000)),

            //std::make_shared<Analog::LinearTriggerMapper>(SwitchPro::LinearConverter(SwitchPro::Axises::LeftTrigger, 0, 200),  X360::TriggersConverter(X360::Triggers::LeftTrigger, 0, 200)),
            //std::make_shared<Analog::LinearTriggerMapper>(SwitchPro::LinearConverter(SwitchPro::Axises::RightTrigger, 0, 200), X360::TriggersConverter(X360::Triggers::RightTrigger, 0, 200)),

            std::make_shared<Digital::Mapper>(SPRO_BUTTON(X), std::make_shared<X360::Button>((X360::Buttons)(X360::Buttons::A | X360::Buttons::L))),
            std::make_shared<Digital::Mapper>(SPRO_BUTTON(Y), std::make_shared<X360::Thumb>(X360::Thumbs::LeftY, 32000)),

            //std::make_shared<Digital::Mapper>(std::make_shared<SwitchPro::Button>((SwitchPro::Buttons) (SwitchPro::Buttons::L | SwitchPro::Buttons::R)), X360_BUTTON(L)),
        };

        void Map(const SwitchPro::Controller& from, X360::Controller& to)
        {
            Mapping::Map(Mapper, from, to);
        }

#ifdef _DEBUG
        void test()
        {
            std::filesystem::path exePath(Win::ExecutablePath());
            auto exeDir = exePath.parent_path();
            auto cfgPath = exeDir / "mapping.yaml";

            {
                YAML::Node node(Mapper);
                YAML::Emitter emitter;
                emitter << node;
                printf("%s", emitter.c_str());

                std::ofstream cfgFile(cfgPath);
                if (cfgFile.is_open())
                {
                    cfgFile << emitter.c_str();
                }
            }
            {
                YAML::Node node = YAML::LoadFile(cfgPath.u8string());
                auto mappers = node.as<Mappers>();
            }
        }
#endif
    }
}

#endif
