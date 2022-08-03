#include "SwitchPro.h"

namespace SwitchPro
{
	namespace
	{
		const unsigned char CommandGetMAC[]         = { 0x80, 0x01 };
		const unsigned char CommandHandshake[]      = { 0x80, 0x02 };
		const unsigned char CommandSwitchBaudrate[] = { 0x80, 0x03 };
		const unsigned char CommandHIDOnlyMode[]    = { 0x80, 0x04 };
		const unsigned char CommandDisconnect[]     = { 0x80, 0x05 };

		const unsigned char CommandEnable[]         = { 0x01 };
		const unsigned char CommandLedValue[]       = { 0x01 };

		constexpr unsigned char CommandRumble  { 0x48 };
		constexpr unsigned char CommandImuData { 0x40 };
		constexpr unsigned char CommandLed     { 0x30 };
		constexpr unsigned char CommandGetInput{ 0x1f };
	}

	template<size_t Len>
	HidDevice::ExchangeArray HidDevice::Exchange(const unsigned char(&data)[Len])
	{
		if (hid_write(device_.get(), data, Len) < 0)
		{
			return {};
		}

		std::array<unsigned char, 0x400> ret;
		hid_read(device_.get(), ret.data(), 0x400);
		return ret;
	}

	HidDevice::ExchangeArray HidDevice::SendCommand(unsigned char cmd)
	{
		unsigned char buf[0x9]{ 0x80, 0x92, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, cmd };
		return Exchange(buf);
	}

	template<size_t Len>
	HidDevice::ExchangeArray HidDevice::SendCommand(unsigned char cmd, const unsigned char(&data)[Len])
	{
		unsigned char buf[9 + Len];
		buf[0x0] = 0x80;
		buf[0x1] = 0x92;
		buf[0x2] = 0x00;
		buf[0x3] = 0x31;
		buf[0x4] = 0x00;
		buf[0x5] = 0x00;
		buf[0x6] = 0x00;
		buf[0x7] = 0x00;
		buf[0x8] = cmd;
		memcpy(buf + 0x9, data, Len);
		return Exchange(buf);
	}

	template<size_t Len>
	HidDevice::ExchangeArray HidDevice::SendSubCommand(unsigned char cmd, unsigned char subCmd, const unsigned char(&data)[Len])
	{
		unsigned char buf[10 + Len];
		buf[0x0] = (unsigned char)(rumbleCounter_++ & 0xF);
		buf[0x1] = 0x00;
		buf[0x2] = 0x01;
		buf[0x3] = 0x40;
		buf[0x4] = 0x40;
		buf[0x5] = 0x00;
		buf[0x6] = 0x01;
		buf[0x7] = 0x40;
		buf[0x8] = 0x40;
		buf[0x9] = subCmd;
		memcpy(buf + 10, data, Len);
		return SendCommand(cmd, buf);
	}

    bool HidDevice::Start(const hid_device_info* dev)
    {
		device_.reset(hid_open_path(dev->path));
		if (!device_)
			throw std::runtime_error("Unable to open controller device: device path could not be opened.");

		if (!Exchange(CommandHandshake))
		{
			throw std::runtime_error("Handshake failed.");
		}

		Exchange(CommandSwitchBaudrate);
		Exchange(CommandHandshake);
		Exchange(CommandHIDOnlyMode);

		SendSubCommand(0x1, CommandRumble , CommandEnable);
		SendSubCommand(0x1, CommandImuData, CommandEnable);
		SendSubCommand(0x1, CommandLed    , CommandLedValue);
    }

	bool HidDevice::Stop()
	{
		return !!Exchange(CommandDisconnect);
	}

	bool HidDevice::Read(Controller& controller)
	{
		auto dat = SendCommand(CommandGetInput);
		if (!dat)
		{
			return false;
		}

		if (dat.value()[0] != 0x30)
		{
			memcpy(&controller, dat.value().data(), sizeof(controller));
			controller.LeftStickX  = ((controller.LeftStickX  & 0x0F) << 4) | ((controller._Padding5 & 0xF0) >> 4);
			controller.RightStickX = ((controller.RightStickX & 0x0F) << 4) | ((controller._Padding6 & 0xF0) >> 4);
			return true;
		}
		else
		{
			return false;
		}
	}
}
