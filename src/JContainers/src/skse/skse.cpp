#include "skse/skse.h"

#include "skse64/GameData.h"
#include "skse64/GameForms.h"
#include "skse64/GameData.h"
#include "skse64/InternalSerialization.h"
#include "skse64/PluginAPI.h"
#include "skse64/PapyrusVM.h"

#include "util/stl_ext.h"
#include "forms/form_handling.h"

#include "gtest.h"

#include <algorithm>

extern SKSESerializationInterface* g_serialization;

namespace skse
{

namespace
{

using forms::FormId;
using forms::FormIdUnredlying;

//--------------------------------------------------------------------------------------------------

/// Internal interface to follow on, same meaning as in the skse.h
struct skse_api
{
    virtual std::optional<std::uint8_t> loaded_mod_index (std::string_view const& name) = 0;
    virtual std::optional<std::uint16_t> loaded_light_mod_index (std::string_view const& name) = 0;

    virtual std::optional<std::string_view> loaded_mod_name (std::uint8_t ndx) = 0;
    virtual std::optional<std::string_view> loaded_light_mod_name (std::uint16_t ndx) = 0;

    virtual FormId resolve_handle (FormId handle) = 0;
    virtual TESForm* lookup_form (FormId handle) = 0;

    virtual bool try_retain_handle (FormId handle) = 0;
    virtual void release_handle (FormId handle) = 0;

    virtual void console_print (const char * fmt, const va_list& args) = 0;
};

//--------------------------------------------------------------------------------------------------

/// Fake (for testing) API implementation
struct fake_api : public skse_api
{
    const std::string_view dict {
        "\0A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0P\0Q\0R\0S\0T\0U\0V\0W\0X\0Y\0Z", 53 };

    std::optional<std::string_view> loaded_mod_name (std::uint8_t ndx) override 
    { 
        if (auto n = dict.find (char (ndx)); n != std::string_view::npos)
            return &dict[n + !ndx];
        return std::nullopt;
    }

    std::optional<std::string_view> loaded_light_mod_name (std::uint16_t ndx) override 
    { 
        return loaded_mod_name (std::uint8_t (ndx));
    }

    std::optional<std::uint8_t> loaded_mod_index (std::string_view const& name) override 
    {
        if (name.empty () || dict.find (name.front ()) == std::string_view::npos)
            return std::nullopt;
        return std::make_optional (name.front ());
    }

    std::optional<std::uint16_t> loaded_light_mod_index (std::string_view const& name) override 
    {
        return loaded_mod_index (name);
    }

    FormId resolve_handle (FormId handle) override { return handle; }

    TESForm* lookup_form (FormId) override
    {
        static char blob[sizeof TESForm] = { '\0' };
        return reinterpret_cast<TESForm*> (&blob);
    }

    bool try_retain_handle (FormId) override { return true; }

    void release_handle (FormId) override {}

    void console_print (const char*, const va_list&) override {}

};

TEST (skseAPI, testModnameFromIndex)
{
    fake_api t;

    EXPECT_EQ (t.loaded_mod_name ('Z'), "Z");
    EXPECT_EQ (t.loaded_light_mod_name ('A'), "A");

    EXPECT_EQ (t.loaded_mod_index ("Action"), 'A');
    EXPECT_EQ (t.loaded_light_mod_index ("Action"), 'A');

    EXPECT_FALSE (t.loaded_mod_name ('|'));
    EXPECT_FALSE (t.loaded_mod_name ('a'));
    EXPECT_FALSE (t.loaded_light_mod_name ('|'));
    EXPECT_FALSE (t.loaded_light_mod_name ('a'));
}

//--------------------------------------------------------------------------------------------------

/// Used to silence at run-time calls to SKSE (explain why?)
struct silent_api : public skse_api
{
    std::optional<std::uint8_t> loaded_mod_index (std::string_view const&) override { return 0; }
    std::optional<std::uint16_t> loaded_light_mod_index (std::string_view const&) override { return 0; }
    std::optional<std::string_view> loaded_mod_name (std::uint8_t) override { return ""; }
    std::optional<std::string_view> loaded_light_mod_name (std::uint16_t) override { return ""; }
    FormId resolve_handle (FormId) override { return FormId::Zero; }
    TESForm* lookup_form (FormId) override { return nullptr; }
    bool try_retain_handle (FormId) override { return true; }
    void release_handle (FormId) override {}
    void console_print (const char*, const va_list&) override {}
};

//--------------------------------------------------------------------------------------------------

/// Actual wrapper around thin calls to SKSE
struct real_api : public skse_api
{
    std::optional<std::uint8_t> loaded_mod_index (std::string_view const& name) override
    {
        using namespace std;
        auto ndx = DataHandler::GetSingleton ()->GetLoadedModIndex (string (name).c_str ());
        return ndx != 0xFF ? make_optional (ndx) : nullopt;
    }

    /// Asuming modIndex is 16-bit, next 16-bit ones in the memory layout looks like reporting 
    /// the light weight mod index.
    static inline std::uint16_t light_index (ModInfo const& mi)
    {
        auto p = reinterpret_cast<uint16_t const*> (&mi.modIndex);
        return *(p + 1);
    }

    std::optional<std::uint16_t> loaded_light_mod_index (std::string_view const& name) override
    {
        using namespace std;
        auto modinfo = DataHandler::GetSingleton ()->LookupLoadedLightModByName (string (name).c_str ());
        return modinfo ? make_optional (light_index (*modinfo)) : nullopt;
    }

    /// Question: order in *Mods list is considered as modIndex or modLighIndex?
    std::optional<std::string_view> loaded_mod_name (std::uint8_t i) override
    {
        DataHandler* p = DataHandler::GetSingleton ();
        if (i < p->modList.loadedMods.count)
            return p->modList.loadedMods[i]->name;
        return std::nullopt;
    }

    std::optional<std::string_view> loaded_light_mod_name (std::uint16_t i) override
    {
        DataHandler* p = DataHandler::GetSingleton ();
        if (i < p->modList.loadedCCMods.count)
            return p->modList.loadedCCMods[i]->name;
        return std::nullopt;
    }

    FormId resolve_handle (FormId handle) override
    {
        std::uint64_t out = 0,
                      in  = static_cast<std::uint64_t> (handle);
        if ((in & 0xFF000000u) == 0xFF000000u)
            return handle; // dynamic form - return untouched
        return g_serialization->ResolveHandle (in, &out) ? (FormId) out : FormId::Zero;
    }

    TESForm* lookup_form (FormId handle) override
    {
        return LookupFormByID (static_cast<std::uint32_t> (handle));
    }

    bool try_retain_handle (FormId id) override
    {
        auto h = static_cast<std::uint64_t> (forms::form_id_to_handle (id));
        auto f = static_cast<TESForm*> ((*g_objectHandlePolicy)->Resolve (TESForm::kTypeID, h));
        if (f)
        {
            (*g_objectHandlePolicy)->AddRef (h);
            return true;
        }
        return false;
    }

    void release_handle (FormId handle) override
    {
        auto h = static_cast<std::uint64_t> (forms::form_id_to_handle (handle));
        (*g_objectHandlePolicy)->Release (h);
    }

    void console_print (const char * fmt, const va_list& args) override
    {
        if (ConsoleManager* mgr = *g_console)
            CALL_MEMBER_FN (mgr, VPrint) (fmt, args);
    }
};

//--------------------------------------------------------------------------------------------------

fake_api g_fake_api;
real_api g_real_api;
silent_api g_silent_api;
skse_api* g_current_api = &g_fake_api;

} // anonymous namespace

//--------------------------------------------------------------------------------------------------

void set_real_api ()
{
    g_current_api = &g_real_api;
}

void set_fake_api ()
{
   g_current_api = &g_fake_api;
}

void set_silent_api ()
{
    g_current_api = &g_silent_api;
}

FormId resolve_handle (FormId handle)
{
    return g_current_api->resolve_handle (handle);
}

TESForm* lookup_form (FormId handle)
{
    return handle != FormId::Zero ? g_current_api->lookup_form (handle) : nullptr;
}

std::optional<std::uint8_t> loaded_mod_index (std::string_view const& name)
{
    return g_current_api->loaded_mod_index (name);
}

std::optional<std::uint16_t> loaded_light_mod_index (std::string_view const& name)
{
    return g_current_api->loaded_light_mod_index (name);
}

std::optional<std::string_view> loaded_mod_name (std::uint8_t idx)
{
    return g_current_api->loaded_mod_name (idx);
}

std::optional<std::string_view> loaded_light_mod_name (std::uint16_t idx)
{
    return g_current_api->loaded_light_mod_name (idx);
}

void console_print (const char* fmt, const va_list& args)
{
    g_current_api->console_print (fmt, args);
}

void console_print (const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    console_print (fmt, args);
    va_end (args);
}

bool try_retain_handle (FormId handle)
{
    return g_current_api->try_retain_handle (handle);
}

void release_handle (FormId handle)
{
    g_current_api->release_handle (handle);
}

//--------------------------------------------------------------------------------------------------

} // namespace skse

