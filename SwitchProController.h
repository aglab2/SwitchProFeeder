#pragma once

#include <memory>
#include <yaml-cpp/yaml.h>

#include "ControllerInterface.h"

namespace SwitchPro
{
#pragma pack(push, 1)
    struct Controller
    {
        uint8_t _Padding0[2];
        union
        {
            uint32_t Buttons;

            struct
            {
                uint32_t _Padding1 : 8;

                uint32_t Y : 1;
                uint32_t X : 1;
                uint32_t B : 1;
                uint32_t A : 1;
                uint32_t _Padding3 : 2;
                uint32_t R : 1;
                uint32_t ZR : 1;

                uint32_t Minus : 1;
                uint32_t Plus : 1;
                uint32_t RStick : 1;
                uint32_t LStick : 1;
                uint32_t Home : 1;
                uint32_t Share : 1;
                uint32_t _Padding4 : 2;

                uint32_t DpadDown : 1;
                uint32_t DpadUp : 1;
                uint32_t DpadRight : 1;
                uint32_t DpadLeft : 1;
                uint32_t _Padding2 : 2;
                uint32_t L : 1;
                uint32_t ZL : 1;
            };
        };

        uint8_t _Padding5;
        uint8_t LeftStickX;
        uint8_t LeftStickY;

        uint8_t _Padding6;
        uint8_t RightStickX;
        uint8_t RightStickY;
    };
#pragma pack(pop)

    using IEvent = ControllerInterface::IEvent<Controller>;
    using IEventPtr = ControllerInterface::IEventPtr<Controller>;

    enum Buttons : uint32_t
    {
        Y = 1 << 8,
        X = 1 << 9,
        B = 1 << 10,
        A = 1 << 11,
        R = 1 << 14,
        ZR = 1 << 15,

        Minus  = 1 << 16,
        Plus   = 1 << 17,
        RStick = 1 << 18,
        LStick = 1 << 19,
        Home   = 1 << 20,
        Share  = 1 << 21,

        DpadDown = 1 << 24,
        DpadUp = 1 << 25,
        DpadRight = 1 << 26,
        DpadLeft = 1 << 27,
        L = 1 << 30,
        ZL = 1 << 31,
    };

    YAML::Emitter& operator<<(YAML::Emitter&, enum Buttons);

    enum Axises : size_t
    {
        LeftStickX  = offsetof(Controller, LeftStickX),
        LeftStickY  = offsetof(Controller, LeftStickY),
        RightStickX = offsetof(Controller, RightStickX),
        RightStickY = offsetof(Controller, RightStickY),
    };

    YAML::Emitter& operator<<(YAML::Emitter&, enum Axises);

    using IButton = ControllerInterface::Button<Buttons, uint32_t>;
    class Button final : public IEvent
        , public IButton
    {
    public:
        Button(Buttons);

        virtual bool Happened(const Controller&) const;
    };

    using IAxis = ControllerInterface::AxisEvent<unsigned char, Axises>;
    struct Axis final : public IEvent
        , public IAxis
    {
    public:
        Axis(Axises, ControllerInterface::AxisComparerType type, unsigned char value);

        virtual bool Happened(const Controller&) const;
    };

    using LinearConverter = ControllerInterface::LinearConverter<Axises, unsigned char>;
}

namespace YAML
{
    template<>
    struct convert<SwitchPro::Axises>
    {
        using Serializer = Serialization::EnumSerializer<SwitchPro::Axises>;

        static const std::map<std::string, SwitchPro::Axises> names;

        static Node encode(const enum SwitchPro::Axises& thumb);
        static bool decode(const Node& node, enum SwitchPro::Axises& thumb);
    };

    template<>
    struct convert<SwitchPro::Buttons>
    {
        using Serializer = Serialization::EnumSerializer<SwitchPro::Buttons>;

        static const std::map<std::string, SwitchPro::Buttons> names;

        static Node encode(const enum SwitchPro::Buttons& thumb);
        static bool decode(const Node& node, enum SwitchPro::Buttons& thumb);
    };

    template<>
    struct convert<SwitchPro::IEventPtr>
    {
        static Node encode(const SwitchPro::IEventPtr&);
        static bool decode(const Node& node, SwitchPro::IEventPtr&);
    };
}
