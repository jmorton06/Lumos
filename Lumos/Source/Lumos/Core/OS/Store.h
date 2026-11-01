#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // In-app purchases: StoreKit 2 on macOS 12+ / iOS 15+, unsupported elsewhere.
    // LUMOS_FAKE_STORE=1 swaps in a local fake (every product buys instantly) for tests and dev runs.
    namespace Store
    {
        struct Product
        {
            std::string id, name, description, price; // price is localised ("£9.99", "10,99 €")
            std::string plainPrice;                  // currency code + number ("EUR 10.99")
            bool consumable = false;
        };

        struct Event
        {
            // products: Request finished; purchased / cancelled / pending / failed: a Buy (or a
            // purchase made elsewhere, e.g. on another device); restored; entitlements: owned set changed
            std::string kind, id, message;
        };

        LUMOS_EXPORT bool IsSupported();
        // Fetches product info from the store; a "products" event lands when done
        LUMOS_EXPORT void Request(const TDArray<std::string>& ids);
        // False until a Request has finished
        LUMOS_EXPORT bool GetProducts(TDArray<Product>& out);
        LUMOS_EXPORT void Buy(const std::string& id);
        LUMOS_EXPORT void Restore();
        // Non-consumables the user owns (checked on device, works offline once known)
        LUMOS_EXPORT bool Entitled(const std::string& id);
        // Events since the last call, oldest first
        LUMOS_EXPORT void Poll(TDArray<Event>& out);
        // Asks the OS to show its rate-this-app prompt (it decides whether it appears, ~3 times a year)
        LUMOS_EXPORT void RequestReview();
        // "1.2 (34)" from the bundle; empty where there's no bundle
        LUMOS_EXPORT std::string AppVersion();
    }
}
