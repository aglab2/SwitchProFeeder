#include "SwitchProController.h"

#include "ControllerInterfaceImpl.h"
#include "SerializationImpl.h"

namespace YAML
{
    const std::map<std::string, SwitchPro::Axises> convert<SwitchPro::Axises>::names
    {
#define ENUMSTR(name) { #name, SwitchPro::Axises::name } 
        ENUMSTR(LeftStickX),
        ENUMSTR(LeftStickY),
        ENUMSTR(RightStickX),
        ENUMSTR(RightStickY),
#undef ENUMSTR
    };

    const std::map<std::string, SwitchPro::Buttons> convert<SwitchPro::Buttons>::names
    {
#define ENUMSTR(name) { #name, SwitchPro::Buttons::name } 
        ENUMSTR(DpadDown),
        ENUMSTR(DpadUp),
        ENUMSTR(DpadRight),
        ENUMSTR(DpadLeft),
        ENUMSTR(L),
        ENUMSTR(ZL),

        ENUMSTR(Y),
        ENUMSTR(X),
        ENUMSTR(B),
        ENUMSTR(A),
        ENUMSTR(R),
        ENUMSTR(ZR),

        ENUMSTR(Minus),
        ENUMSTR(Plus),
        ENUMSTR(RStick),
        ENUMSTR(LStick),
        ENUMSTR(Home),
        ENUMSTR(Share),
#undef ENUMSTR
    };

    Node convert<SwitchPro::Axises>::encode(const enum SwitchPro::Axises& thumb)
    {
        return Serializer::Encode(names, thumb);
    }

    bool convert<SwitchPro::Axises>::decode(const Node& node, enum SwitchPro::Axises& thumb)
    {
        return Serializer::Decode(names, node, thumb);
    }

    Node convert<SwitchPro::Buttons>::encode(const enum SwitchPro::Buttons& thumb)
    {
        return Serializer::EncodeBitWise(names, thumb);
    }

    bool convert<SwitchPro::Buttons>::decode(const Node& node, enum SwitchPro::Buttons& thumb)
    {
        return Serializer::DecodeBitWise(names, node, thumb);
    }

    Node convert<SwitchPro::IEventPtr>::encode(const SwitchPro::IEventPtr& ptr)
    {
        return ptr->Serialize();
    }

    bool convert<SwitchPro::IEventPtr>::decode(const Node& node, SwitchPro::IEventPtr& ptr)
    {
        if (node.IsScalar() || node.IsSequence())
        {
            // Button case
            auto buttons = node.as<SwitchPro::Buttons>();
            ptr = std::make_shared<SwitchPro::Button>(buttons);
            return true;
        }
        else if (node.IsMap())
        {
            auto typeNode = node["type"];
            if (!typeNode)
                return false;

            auto type = typeNode.as<std::string>();
            if (type == "axis")
            {
                auto offsetNode = node["offset"];
                auto valueNode = node["axis"];
                auto comparNode = node["comparer"];
                if (!offsetNode || !valueNode || !comparNode)
                    return false;

                auto axises = offsetNode.as<SwitchPro::Axises>();
                auto compar = comparNode.as<ControllerInterface::AxisComparerType>();
                auto value = (unsigned char)valueNode.as<int>();

                ptr = std::make_shared<SwitchPro::Axis>(axises, compar, value);
                return true;
            }
            else
            {
                return false;
            }
        }
        return false;
    }
}

namespace SwitchPro
{
    Button::Button(Buttons b) : IButton(b) { }

    bool Button::Happened(const Controller& c) const
    {
        return Applied(c.Buttons);
    }

    Axis::Axis(Axises a, ControllerInterface::AxisComparerType compar, unsigned char value)
        : IAxis(value, compar, a) { }

    bool Axis::Happened(const Controller& c) const
    {
        return Applied(&c);
    }
}
