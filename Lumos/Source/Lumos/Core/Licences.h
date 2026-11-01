#pragma once
#include "Core/Types.h"

namespace Lumos
{
    struct ThirdPartyLicence
    {
        const char* Name;
        const char* Licence; // id for GetLicenceText: "MIT", "Apache-2.0", ...
        const char* Copyright;
        const char* URL;
    };

    // Third-party code and fonts built into this binary, for an acknowledgements screen.
    const ThirdPartyLicence* GetThirdPartyLicences(u32& count);

    // Full text for a licence id, "" when unknown. The copyright line is per library.
    const char* GetLicenceText(const char* licence);
}
