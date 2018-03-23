#include "skse/skse.h"

#include "skse64_common/skse_version.h"
#include "skse64/GameData.h"
#include "skse64/GameForms.h"
#include "skse64/GameData.h"
#include "skse64/InternalSerialization.h"
#include "skse64/PluginAPI.h"
#include "skse64/PapyrusVM.h"

#include "gtest.h"

#include "util/stl_ext.h"
#include "forms/form_handling.h"

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
    virtual std::optional<std::uint8_t> loaded_mod_index (string_view const& name) = 0;
    virtual std::optional<std::uint8_t> loaded_light_mod_index (string_view const& name) = 0;

    virtual const char* modname_from_index (std::uint8_t idx) = 0;
    virtual std::uint8_t modindex_from_name (const char* name) = 0;

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
    std::uint8_t modindex_from_name (const char* name) override {
        return name && std::isalpha (*name) ? *name : 0xFF;
    }

    const char* modname_from_index (std::uint8_t idx) override
    {
        static const std::string_view dict =
            "\0A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0P\0Q\0R\0S\0T\0U\0V\0W\0X\0Y\0Z";
        if (auto n = dict.find (char (idx)); n != std::string_view::npos)
            return &dict[n + !idx];
        return nullptr;
    }

    std::optional<std::uint8_t> loaded_mod_index (string_view const& name) override {
        std::uint8_t n = modindex_from_name (name.data ());
        return n != 0xFF ? std::make_optional (n) : std::nullopt;
    }

    std::optional<std::uint8_t> loaded_light_mod_index (string_view const& name) override {
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

    TEST (modname_from_index, test)
    {
        auto res = modname_from_index ('Z');
        EXPECT_TRUE (strcmp (res, "Z") == 0);

        EXPECT_TRUE (modindex_from_name ("Action") == 'A');

        EXPECT_NIL (modname_from_index ('|'));
        EXPECT_NIL (modname_from_index ('a'));
    }
};

//--------------------------------------------------------------------------------------------------

/// Used to silence at run-time calls to SKSE (explain why?)
struct silent_api : public skse_api
{
    std::optional<std::uint8_t> loaded_mod_index (string_view const&) override { return 0; }
    std::optional<std::uint8_t> loaded_light_mod_index (string_view const&) override { return 0; }
    const char* modname_from_index (std::uint8_t) override { return ""; }
    std::uint8_t modindex_from_name (const char*) override { return 0; }
    FormId resolve_handle (FormId) override { return FormId::Zero; }
    TESForm* lookup_form (FormId) override { return nullptr; }
    bool try_retain_handle (FormId) override { return true; }
    void release_handle (FormId) override {}
    void console_print (const char*, const va_list&) override {}
};

//--------------------------------------------------------------------------------------------------

struct real_api : public skse_api
{
    std::optional<std::uint8_t> loaded_mod_index (string_view const& name) override
    {
        auto ndx = DataHandler::GetSingleton ()->GetLoadedModIndex (name.data ());
        return ndx != 0xFF ? std::make_optional (ndx) : std::nullopt;
    }

    std::optional<std::uint8_t> loaded_light_mod_index (string_view const& name) override
    {
        auto ndx = DataHandler::GetSingleton ()->GetLoadedLightModIndex (name.data ());
        return ndx != 0xFF ? std::make_optional (ndx) : std::nullopt;
    }

    const char* modname_from_index (std::uint8_t idx) override
    {
        DataHandler* p = DataHandler::GetSingleton ();
        if (idx < p->modList.loadedMods.count)
            return p->modList.loadedMods[idx]->name;
        return nullptr;
    }

    std::uint8_t modindex_from_name (const char* name) override {
        return DataHandler::GetSingleton ()->GetModIndex (name);
    }

    FormId resolve_handle (FormId handle) override
    {
        std::uint64_t a (handle), b = 0;
        if ((a & 0xFF000000u) == 0xFF000000u)
            return handle; // dynamic form - return untouched
        return g_serialization->ResolveHandle (a, &b) ? (FormId) b : FormId::Zero;
    }

    TESForm* lookup_form (FormId handle) override
    {
        return LookupFormByID (static_cast<std::uint32_t> (handle));
    }

    bool try_retain_handle (FormId id) override
    {
        std::uint64_t h = forms::form_id_to_handle (id);
        auto form = reinterpret_cast<TESForm*> (
                (*g_objectHandlePolicy)->Resolve (TESForm::kTypeID, h));
        if (form)
        {
            (*g_objectHandlePolicy)->AddRef (h);
            return true;
        }
        return false;
    }

    void release_handle (FormId handle) override
    {
        (*g_objectHandlePolicy)->Release ((std::uint64_t) forms::form_id_to_handle (handle));
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

std::optional<std::uint8_t> loaded_mod_index (string_view const& name)
{
    return g_current_api->loaded_mod_index (name);
}

std::optional<std::uint8_t> loaded_light_mod_index (string_view const& name)
{
    return g_current_api->loaded_light_mod_index (name);
}

const char* modname_from_index (std::uint8_t idx)
{
    return g_current_api->modname_from_index(idx);
}

std::uint8_t modindex_from_name (const char* name)
{
    return g_current_api->modindex_from_name (name);
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

