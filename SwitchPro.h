#pragma once

#include "SwitchProController.h"

#include "hidapi.h"

#define MAX_RESPONCE_SIZE 64

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

        using ExchangeArray = std::optional<std::array<unsigned char, MAX_RESPONCE_SIZE>>;

        template<size_t Len>
        ExchangeArray ExchangeImmediate(const unsigned char(&data)[Len]);

        template<size_t Len>
        ExchangeArray ExchangeWithReadRetries(unsigned char cmd, const unsigned char(&data)[Len]);

        template<size_t Len>
        ExchangeArray SendSubCommand(unsigned char cmd, const unsigned char(&data)[Len]);

        void BlinkHomeLight();
    };
}
