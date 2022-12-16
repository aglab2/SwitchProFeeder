#include "SwitchPro.h"

#include <thread>

namespace SwitchPro
{
	namespace
	{
		const unsigned char CommandGetMAC[]          = { 0x80, 0x01 };
		const unsigned char CommandHandshake[]       = { 0x80, 0x02 };
		const unsigned char CommandSwitchBaudrate[]  = { 0x80, 0x03 };
		const unsigned char CommandHIDOnlyMode[]     = { 0x80, 0x04 };
		const unsigned char CommandDisconnect1[]     = { 0x80, 0x05 };
		const unsigned char CommandDisconnect2[]     = { 0x80, 0x05 };

		const unsigned char CommandEnable[]          = { 0x01 };
		const unsigned char CommandLedValue[]        = { 0x01 };
		const unsigned char CommandPollInputsValue[] = { 0x30 };

		constexpr unsigned char CommandReset     { 0x1  };
		constexpr unsigned char CommandPollInputs{ 0x3 };
		constexpr unsigned char CommandRemediate { 0x6 };
		constexpr unsigned char CommandRumble    { 0x48 };
		constexpr unsigned char CommandImuData   { 0x40 };
		constexpr unsigned char CommandLed       { 0x30 };
		constexpr unsigned char CommandBlink     { 0x38 };
		constexpr unsigned char CommandGetInput  { 0x1f };
	}

	template<size_t Len>
	HidDevice::ExchangeArray HidDevice::ExchangeImmediate(const unsigned char(&data)[Len])
	{
		if (hid_write(device_.get(), data, Len) < 0)
		{
			return {};
		}

		std::array<unsigned char, MAX_RESPONCE_SIZE> ret{};
		hid_read_timeout(device_.get(), ret.data(), MAX_RESPONCE_SIZE, 100);
		return ret;
	}

#define EXCHANGE_IMMEDIATE_LOG(params) do{ printf("ExchangeImmediate(%s)\n", #params); auto res = ExchangeImmediate(params); printf("ExchangeImmediate(%s) -> %s\n", #params, res ? "OK" : "FAIL"); if (!res) throw 0; }while(0)
#define SEND_SUBCOMMAND_LOG(cmd, buf)  do{ printf("SendSubCommand(%s, %s)\n", #cmd, #buf); auto res = SendSubCommand(cmd, buf); printf("SendSubCommand(%s, %s) -> %s\n", #cmd, #buf, res ? "OK" : "FAIL"); if (!res) throw 0; }while(0)

	template<size_t Len>
	HidDevice::ExchangeArray HidDevice::ExchangeWithReadRetries(unsigned char cmd, const unsigned char(&data)[Len])
	{
		if (hid_write(device_.get(), data, Len) < 0)
		{
			return {};
		}

		std::array<unsigned char, MAX_RESPONCE_SIZE> ret{ };
		int tries = 0;
		do
		{
			int res = hid_read_timeout(device_.get(), ret.data(), MAX_RESPONCE_SIZE, 100);
			if (res < 1)
			{
				ret[0] = 0;
			}
			tries++;
		} while (tries < 10 && ret[0] != 0x21 && ret[14] != cmd);

		return ret;
	}

	template<size_t Len>
	HidDevice::ExchangeArray HidDevice::SendSubCommand(unsigned char cmd, const unsigned char(&data)[Len])
	{
		unsigned char buf[11 + Len];
		buf[0x0] = 0x01;
		buf[0x1] = (unsigned char)(rumbleCounter_++ & 0xF);
		buf[0x2] = 0x00;
		buf[0x3] = 0x01;
		buf[0x4] = 0x40;
		buf[0x5] = 0x40;
		buf[0x6] = 0x00;
		buf[0x7] = 0x01;
		buf[0x8] = 0x40;
		buf[0x9] = 0x40;
		buf[0xa] = cmd;
		memcpy(buf + 11, data, Len);
		return ExchangeWithReadRetries(cmd, buf);
	}

	void HidDevice::BlinkHomeLight()
	{
		unsigned char buf[25];
		memset(buf, 0xff, sizeof(buf));
		buf[0] = 0x18;
		buf[1] = 0x01;
		SEND_SUBCOMMAND_LOG(CommandBlink, buf);
	}

    bool HidDevice::Start(const hid_device_info* dev)
    {
		constexpr int ConnectionRetriesCount = 100;
		for (int retry = 0; retry < ConnectionRetriesCount; retry++)
		try
		{
			device_.reset(hid_open_path(dev->path));
			if (!device_)
				throw std::runtime_error("Unable to open controller device: device path could not be opened.");

			hid_set_nonblocking(device_.get(), 0);

			printf("[%d] Trying to initialize controller\n", retry);
			{
				auto mac = ExchangeImmediate(CommandGetMAC);
				if (!mac || (*mac)[0] != 0x81)
				{
					printf("[~] Glitched controller is detected, trying to remediate...\n");
					SEND_SUBCOMMAND_LOG(0x6, CommandEnable);
				}
			}

			EXCHANGE_IMMEDIATE_LOG(CommandHandshake);
			EXCHANGE_IMMEDIATE_LOG(CommandSwitchBaudrate);
			EXCHANGE_IMMEDIATE_LOG(CommandHandshake);
			EXCHANGE_IMMEDIATE_LOG(CommandHIDOnlyMode);

			BlinkHomeLight();
			SEND_SUBCOMMAND_LOG(CommandLed, CommandLedValue);
			SEND_SUBCOMMAND_LOG(CommandImuData, CommandEnable);
			SEND_SUBCOMMAND_LOG(CommandRumble, CommandEnable);
			SEND_SUBCOMMAND_LOG(CommandPollInputs, CommandPollInputsValue);

			printf("Controller seems to be initialized, trying to poll inputs...\n");
			bool ok = false;
			constexpr int ControllerResponseCount = 10;
			for (int i = 0; i < ControllerResponseCount; i++)
			{
				ExchangeArray dat = std::array<unsigned char, MAX_RESPONCE_SIZE>{};
				int ret = hid_read_timeout(device_.get(), dat.value().data(), MAX_RESPONCE_SIZE, 5);
				ok = ret > 0;
				printf("{%d} PollController -> %d%s\n", i, ret, ok ? " OK" : "");
				if (ok)
				{
					break;
				}
			}
			if (!ok)
			{
				printf("[!] Controller is not reporting the state, FAIL\n");
				throw 0;
			}

			break;
		}
		catch (...)
		{
			constexpr int msToSleep = 1000;
			printf("[!] Fail detected, retrying after %d milliseconds\n", msToSleep);
			std::this_thread::sleep_for(std::chrono::milliseconds(msToSleep));
		}

		hid_set_nonblocking(device_.get(), 1);
    }

	bool HidDevice::Stop()
	try
	{
		hid_set_nonblocking(device_.get(), 0);
		EXCHANGE_IMMEDIATE_LOG(CommandDisconnect1);
		EXCHANGE_IMMEDIATE_LOG(CommandDisconnect2);
		return true;
	}
	catch (...)
	{
		return false;
	}

	bool HidDevice::Read(Controller& controller)
	{
		size_t sz = MAX_RESPONCE_SIZE;
		ExchangeArray dat = std::array<unsigned char, MAX_RESPONCE_SIZE>{};
		int ret = hid_read_timeout(device_.get(), dat.value().data(), sz, 5);
		if (ret <= 0)
			return false;

		memcpy(&controller, dat.value().data(), sizeof(controller));
		controller.LeftStickX  = ((controller.LeftStickX  & 0x0F) << 4) | ((controller._Padding5 & 0xF0) >> 4);
		controller.RightStickX = ((controller.RightStickX & 0x0F) << 4) | ((controller._Padding6 & 0xF0) >> 4);
		return true;
	}
}
