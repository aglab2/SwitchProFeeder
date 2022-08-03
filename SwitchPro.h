#pragma once

#include "SwitchProController.h"

#include "hidapi.h"

namespace SwitchPro
{
    struct HIDCloser 
    {
        void operator()(hid_device* ptr)
        {
            hid_close(ptr);
        }
    };

    class HidDevice
    {
    public:
        explicit HidDevice(uint8_t port) : port_(port)
        {}

        bool Start(const hid_device_info*);
        bool Stop();
        bool Read(Controller&);

    private:
        uint8_t port_;
        std::unique_ptr<hid_device, HIDCloser> device_;
        int rumbleCounter_ = 0;

        using ExchangeArray = std::optional<std::array<unsigned char, 0x400>>;

        template<size_t Len>
        ExchangeArray Exchange(const unsigned char(&data)[Len]);

        ExchangeArray SendCommand(unsigned char cmd);

        template<size_t Len>
        ExchangeArray SendCommand(unsigned char cmd, const unsigned char(&data)[Len]);

        template<size_t Len>
        ExchangeArray SendSubCommand(unsigned char cmd, unsigned char subCmd, const unsigned char(&data)[Len]);
    };
}
