#pragma once

#include "SwitchProController.h"
#include "X360Controller.h"

namespace Mapping
{
    namespace Debug
    {
        void Map(const SwitchPro::Controller& from, X360::Controller& to);

#ifdef _DEBUG
        void test();
#endif
    }
}
