#include "Precompiled.h"
#include "Store.h"
#include <cstdlib>
#include <cstring>

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
#define LUMOS_STOREKIT 1
// Platform/Apple/Store.swift
extern "C"
{
    bool LumosStoreSupported(void);
    void LumosStoreRequest(const char* ids);
    char* LumosStoreProducts(void);
    void LumosStoreBuy(const char* id);
    void LumosStoreRestore(void);
    bool LumosStoreEntitled(const char* id);
    char* LumosStorePoll(void);
    void LumosStoreRequestReview(void);
    char* LumosAppVersion(void);
}
#endif

namespace Lumos
{
    namespace Store
    {
        namespace
        {
            // fields split by 0x1f, records by 0x1e
            void Split(const char* s, char sep, TDArray<std::string>& out)
            {
                out.Clear();
                if(!s || !*s)
                    return;
                const char* start = s;
                for(const char* p = s;; p++)
                {
                    if(*p == sep || *p == 0)
                    {
                        out.PushBack(std::string(start, p - start));
                        if(*p == 0)
                            break;
                        start = p + 1;
                    }
                }
            }

            struct Fake
            {
                bool on = false;
                bool loaded = false;
                TDArray<std::string> requested;
                TDArray<std::string> owned;
                TDArray<Event> events;
            };

            Fake& GetFake()
            {
                static Fake f = [] {
                    Fake x;
                    const char* e = getenv("LUMOS_FAKE_STORE");
                    x.on          = e && *e && strcmp(e, "0") != 0;
                    return x;
                }();
                return f;
            }

            bool Owns(const Fake& f, const std::string& id)
            {
                for(auto& o : f.owned)
                    if(o == id)
                        return true;
                return false;
            }

            // tips / anything with "tip" in the id count as consumables in the fake
            bool FakeConsumable(const std::string& id) { return id.find("tip") != std::string::npos; }
        }

        bool IsSupported()
        {
            if(GetFake().on)
                return true;
#ifdef LUMOS_STOREKIT
            return LumosStoreSupported();
#else
            return false;
#endif
        }

        void Request(const TDArray<std::string>& ids)
        {
            Fake& f = GetFake();
            if(f.on)
            {
                f.requested = ids;
                f.loaded    = true;
                f.events.PushBack({ "products", "", "" });
                return;
            }
#ifdef LUMOS_STOREKIT
            std::string joined;
            for(u32 i = 0; i < ids.Size(); i++)
            {
                if(i)
                    joined += '\x1f';
                joined += ids[i];
            }
            LumosStoreRequest(joined.c_str());
#endif
        }

        bool GetProducts(TDArray<Product>& out)
        {
            out.Clear();
            Fake& f = GetFake();
            if(f.on)
            {
                if(!f.loaded)
                    return false;
                for(auto& id : f.requested)
                {
                    Product p;
                    p.id         = id;
                    p.consumable = FakeConsumable(id);
                    p.name       = p.consumable ? "Tip" : "Pro";
                    p.price      = !p.consumable ? "£9.99" : id.find("large") != std::string::npos ? "£9.99" : id.find("medium") != std::string::npos ? "£4.99" : "£1.99";
                    p.plainPrice = "GBP " + p.price.substr(2); // "£" is two bytes
                    out.PushBack(p);
                }
                return true;
            }
#ifdef LUMOS_STOREKIT
            char* s = LumosStoreProducts();
            if(!s)
                return false;
            TDArray<std::string> records, fields;
            Split(s, '\x1e', records);
            free(s);
            for(auto& r : records)
            {
                Split(r.c_str(), '\x1f', fields);
                if(fields.Size() < 5)
                    continue;
                Product p;
                p.id          = fields[0];
                p.name        = fields[1];
                p.description = fields[2];
                p.price       = fields[3];
                p.consumable  = fields[4] == "consumable";
                p.plainPrice  = fields.Size() > 5 ? fields[5] : p.price;
                out.PushBack(p);
            }
            return true;
#else
            return false;
#endif
        }

        void Buy(const std::string& id)
        {
            Fake& f = GetFake();
            if(f.on)
            {
                if(!FakeConsumable(id) && !Owns(f, id))
                {
                    f.owned.PushBack(id);
                    f.events.PushBack({ "entitlements", "", "" });
                }
                f.events.PushBack({ "purchased", id, "" });
                return;
            }
#ifdef LUMOS_STOREKIT
            LumosStoreBuy(id.c_str());
#endif
        }

        void Restore()
        {
            Fake& f = GetFake();
            if(f.on)
            {
                f.events.PushBack({ "restored", "", "" });
                return;
            }
#ifdef LUMOS_STOREKIT
            LumosStoreRestore();
#endif
        }

        bool Entitled(const std::string& id)
        {
            Fake& f = GetFake();
            if(f.on)
                return Owns(f, id);
#ifdef LUMOS_STOREKIT
            return LumosStoreEntitled(id.c_str());
#else
            return false;
#endif
        }

        void Poll(TDArray<Event>& out)
        {
            out.Clear();
            Fake& f = GetFake();
            if(f.on)
            {
                out = f.events;
                f.events.Clear();
                return;
            }
#ifdef LUMOS_STOREKIT
            char* s = LumosStorePoll();
            if(!s)
                return;
            TDArray<std::string> records, fields;
            Split(s, '\x1e', records);
            free(s);
            for(auto& r : records)
            {
                Split(r.c_str(), '\x1f', fields);
                if(fields.Size() < 1)
                    continue;
                out.PushBack({ fields[0], fields.Size() > 1 ? fields[1] : "", fields.Size() > 2 ? fields[2] : "" });
            }
#endif
        }

        void RequestReview()
        {
            if(GetFake().on)
                return;
#ifdef LUMOS_STOREKIT
            LumosStoreRequestReview();
#endif
        }

        std::string AppVersion()
        {
#ifdef LUMOS_STOREKIT
            char* s = LumosAppVersion();
            if(!s)
                return "";
            std::string v = s;
            free(s);
            return v;
#else
            return "";
#endif
        }
    }
}
