// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "GlobalAppSettings.h"
#include "../../types/inc/Utils.hpp"
#include "../../inc/DefaultSettings.h"
#include "JsonUtils.h"
#include "TerminalSettingsSerializationHelpers.h"

#include "GlobalAppSettings.g.cpp"

using namespace Microsoft::Terminal::Settings::Model;
using namespace winrt::Microsoft::Terminal::Settings::Model::implementation;
using namespace winrt::Windows::UI::Xaml;
using namespace ::Microsoft::Console;
using namespace winrt::Microsoft::UI::Xaml::Controls;

static constexpr std::string_view LegacyKeybindingsKey{ "keybindings" };
static constexpr std::string_view ActionsKey{ "actions" };
static constexpr std::string_view DefaultProfileKey{ "defaultProfile" };
static constexpr std::string_view AlwaysShowTabsKey{ "alwaysShowTabs" };
static constexpr std::string_view InitialRowsKey{ "initialRows" };
static constexpr std::string_view InitialColsKey{ "initialCols" };
static constexpr std::string_view InitialPositionKey{ "initialPosition" };
static constexpr std::string_view ShowTitleInTitlebarKey{ "showTerminalTitleInTitlebar" };
static constexpr std::string_view ThemeKey{ "theme" };
static constexpr std::string_view TabWidthModeKey{ "tabWidthMode" };
static constexpr std::string_view ShowTabsInTitlebarKey{ "showTabsInTitlebar" };
static constexpr std::string_view WordDelimitersKey{ "wordDelimiters" };
static constexpr std::string_view CopyOnSelectKey{ "copyOnSelect" };
static constexpr std::string_view CopyFormattingKey{ "copyFormatting" };
static constexpr std::string_view WarnAboutLargePasteKey{ "largePasteWarning" };
static constexpr std::string_view WarnAboutMultiLinePasteKey{ "multiLinePasteWarning" };
static constexpr std::string_view LaunchModeKey{ "launchMode" };
static constexpr std::string_view ConfirmCloseAllKey{ "confirmCloseAllTabs" };
static constexpr std::string_view SnapToGridOnResizeKey{ "snapToGridOnResize" };
static constexpr std::string_view EnableStartupTaskKey{ "startOnUserLogin" };
static constexpr std::string_view AlwaysOnTopKey{ "alwaysOnTop" };
static constexpr std::string_view UseTabSwitcherKey{ "useTabSwitcher" };
static constexpr std::string_view DisableAnimationsKey{ "disableAnimations" };

static constexpr std::string_view DebugFeaturesKey{ "debugFeatures" };

static constexpr std::string_view ForceFullRepaintRenderingKey{ "experimental.rendering.forceFullRepaint" };
static constexpr std::string_view SoftwareRenderingKey{ "experimental.rendering.software" };
static constexpr std::string_view ForceVTInputKey{ "experimental.input.forceVT" };

#ifdef _DEBUG
static constexpr bool debugFeaturesDefault{ true };
#else
static constexpr bool debugFeaturesDefault{ false };
#endif

GlobalAppSettings::GlobalAppSettings() :
    _keymap{ winrt::make_self<KeyMapping>() },
    _keybindingsWarnings{},
    _validDefaultProfile{ false },
    _defaultProfile{},
    _DebugFeaturesEnabled{ debugFeaturesDefault }
{
    _commands = winrt::single_threaded_map<winrt::hstring, Model::Command>();
    _colorSchemes = winrt::single_threaded_map<winrt::hstring, Model::ColorScheme>();
}

// Method Description:
// - Copies any extraneous data from the parent before completing a CreateChild call
// Arguments:
// - <none>
// Return Value:
// - <none>
void GlobalAppSettings::_FinalizeInheritance()
{
    // TODO CARLOS: Crash invoking IActionAndArgs->Copy
    //_keymap = _parent->_keymap->Copy();
    _keymap = _parent->_keymap;
    std::copy(_parent->_keybindingsWarnings.begin(), _parent->_keybindingsWarnings.end(), std::back_inserter(_keybindingsWarnings));
    for (auto kv : _parent->_colorSchemes)
    {
        const auto schemeImpl{ winrt::get_self<ColorScheme>(kv.Value()) };
        _colorSchemes.Insert(kv.Key(), *schemeImpl->Copy());
    }

    for (auto kv : _parent->_commands)
    {
        const auto commandImpl{ winrt::get_self<Command>(kv.Value()) };
        _commands.Insert(kv.Key(), *commandImpl->Copy());
    }
}

winrt::com_ptr<GlobalAppSettings> GlobalAppSettings::Copy() const
{
    auto globals{ winrt::make_self<GlobalAppSettings>() };
    globals->_InitialRows = _InitialRows;
    globals->_InitialCols = _InitialCols;
    globals->_AlwaysShowTabs = _AlwaysShowTabs;
    globals->_ShowTitleInTitlebar = _ShowTitleInTitlebar;
    globals->_ConfirmCloseAllTabs = _ConfirmCloseAllTabs;
    globals->_Theme = _Theme;
    globals->_TabWidthMode = _TabWidthMode;
    globals->_ShowTabsInTitlebar = _ShowTabsInTitlebar;
    globals->_WordDelimiters = _WordDelimiters;
    globals->_CopyOnSelect = _CopyOnSelect;
    globals->_CopyFormatting = _CopyFormatting;
    globals->_WarnAboutLargePaste = _WarnAboutLargePaste;
    globals->_WarnAboutMultiLinePaste = _WarnAboutMultiLinePaste;
    globals->_InitialPosition = _InitialPosition;
    globals->_LaunchMode = _LaunchMode;
    globals->_SnapToGridOnResize = _SnapToGridOnResize;
    globals->_ForceFullRepaintRendering = _ForceFullRepaintRendering;
    globals->_SoftwareRendering = _SoftwareRendering;
    globals->_ForceVTInput = _ForceVTInput;
    globals->_DebugFeaturesEnabled = _DebugFeaturesEnabled;
    globals->_StartOnUserLogin = _StartOnUserLogin;
    globals->_AlwaysOnTop = _AlwaysOnTop;
    globals->_UseTabSwitcher = _UseTabSwitcher;
    globals->_DisableAnimations = _DisableAnimations;

    globals->_UnparsedDefaultProfile = _UnparsedDefaultProfile;
    globals->_defaultProfile = _defaultProfile;
    // TODO CARLOS: Crash invoking IActionAndArgs->Copy
    //globals->_keymap = _keymap->Copy();
    globals->_keymap = _keymap;
    std::copy(_keybindingsWarnings.begin(), _keybindingsWarnings.end(), std::back_inserter(globals->_keybindingsWarnings));

    for (auto kv : _colorSchemes)
    {
        const auto schemeImpl{ winrt::get_self<ColorScheme>(kv.Value()) };
        globals->_colorSchemes.Insert(kv.Key(), *schemeImpl->Copy());
    }

    for (auto kv : _commands)
    {
        const auto commandImpl{ winrt::get_self<Command>(kv.Value()) };
        globals->_commands.Insert(kv.Key(), *commandImpl->Copy());
    }
    return globals;
}

winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Microsoft::Terminal::Settings::Model::ColorScheme> GlobalAppSettings::ColorSchemes() noexcept
{
    return _colorSchemes.GetView();
}

#pragma region DefaultProfile
void GlobalAppSettings::DefaultProfile(const winrt::guid& defaultProfile) noexcept
{
    _validDefaultProfile = true;
    _defaultProfile = defaultProfile;
}

winrt::guid GlobalAppSettings::DefaultProfile() const
{
    // If we have an unresolved default profile, we should likely explode.
    THROW_HR_IF(E_INVALIDARG, !_validDefaultProfile);
    return _defaultProfile;
}

bool GlobalAppSettings::HasUnparsedDefaultProfile() const
{
    return _UnparsedDefaultProfile.has_value();
}

winrt::hstring GlobalAppSettings::UnparsedDefaultProfile() const
{
    const auto val{ _getUnparsedDefaultProfileImpl() };
    return val ? *val : hstring{ L"" };
}

void GlobalAppSettings::UnparsedDefaultProfile(const hstring& value)
{
    if (_UnparsedDefaultProfile != value)
    {
        _UnparsedDefaultProfile = value;
        _validDefaultProfile = false;
    }
}

void GlobalAppSettings::ClearUnparsedDefaultProfile()
{
    if (HasUnparsedDefaultProfile())
    {
        _UnparsedDefaultProfile = std::nullopt;
    }
}

std::optional<winrt::hstring> GlobalAppSettings::_getUnparsedDefaultProfileImpl() const
{
    return _UnparsedDefaultProfile ?
               _UnparsedDefaultProfile :
               (_parent ?
                    _parent->_getUnparsedDefaultProfileImpl() :
                    std::nullopt);
}
#pragma endregion

winrt::Microsoft::Terminal::Settings::Model::KeyMapping GlobalAppSettings::KeyMap() const noexcept
{
    return *_keymap;
}

// Method Description:
// - Create a new instance of this class from a serialized JsonObject.
// Arguments:
// - json: an object which should be a serialization of a GlobalAppSettings object.
// Return Value:
// - a new GlobalAppSettings instance created from the values in `json`
winrt::com_ptr<GlobalAppSettings> GlobalAppSettings::FromJson(const Json::Value& json)
{
    auto result = winrt::make_self<GlobalAppSettings>();
    result->LayerJson(json);
    return result;
}

void GlobalAppSettings::LayerJson(const Json::Value& json)
{
    // _validDefaultProfile keeps track of whether we've verified that DefaultProfile points to something
    // CascadiaSettings::_ResolveDefaultProfile performs a validation and updates DefaultProfile() with the
    // resolved value, then making it valid.
    _validDefaultProfile = !JsonUtils::GetValueForKey(json, DefaultProfileKey, _UnparsedDefaultProfile);

    JsonUtils::GetValueForKey(json, AlwaysShowTabsKey, _AlwaysShowTabs);

    JsonUtils::GetValueForKey(json, ConfirmCloseAllKey, _ConfirmCloseAllTabs);

    JsonUtils::GetValueForKey(json, InitialRowsKey, _InitialRows);

    JsonUtils::GetValueForKey(json, InitialColsKey, _InitialCols);

    JsonUtils::GetValueForKey(json, InitialPositionKey, _InitialPosition);

    JsonUtils::GetValueForKey(json, ShowTitleInTitlebarKey, _ShowTitleInTitlebar);

    JsonUtils::GetValueForKey(json, ShowTabsInTitlebarKey, _ShowTabsInTitlebar);

    JsonUtils::GetValueForKey(json, WordDelimitersKey, _WordDelimiters);

    JsonUtils::GetValueForKey(json, CopyOnSelectKey, _CopyOnSelect);

    JsonUtils::GetValueForKey(json, CopyFormattingKey, _CopyFormatting);

    JsonUtils::GetValueForKey(json, WarnAboutLargePasteKey, _WarnAboutLargePaste);

    JsonUtils::GetValueForKey(json, WarnAboutMultiLinePasteKey, _WarnAboutMultiLinePaste);

    JsonUtils::GetValueForKey(json, LaunchModeKey, _LaunchMode);

    JsonUtils::GetValueForKey(json, ThemeKey, _Theme);

    JsonUtils::GetValueForKey(json, TabWidthModeKey, _TabWidthMode);

    JsonUtils::GetValueForKey(json, SnapToGridOnResizeKey, _SnapToGridOnResize);

    // GetValueForKey will only override the current value if the key exists
    JsonUtils::GetValueForKey(json, DebugFeaturesKey, _DebugFeaturesEnabled);

    JsonUtils::GetValueForKey(json, ForceFullRepaintRenderingKey, _ForceFullRepaintRendering);

    JsonUtils::GetValueForKey(json, SoftwareRenderingKey, _SoftwareRendering);
    JsonUtils::GetValueForKey(json, ForceVTInputKey, _ForceVTInput);

    JsonUtils::GetValueForKey(json, EnableStartupTaskKey, _StartOnUserLogin);

    JsonUtils::GetValueForKey(json, AlwaysOnTopKey, _AlwaysOnTop);

    JsonUtils::GetValueForKey(json, UseTabSwitcherKey, _UseTabSwitcher);

    JsonUtils::GetValueForKey(json, DisableAnimationsKey, _DisableAnimations);

    // This is a helper lambda to get the keybindings and commands out of both
    // and array of objects. We'll use this twice, once on the legacy
    // `keybindings` key, and again on the newer `bindings` key.
    auto parseBindings = [this, &json](auto jsonKey) {
        if (auto bindings{ json[JsonKey(jsonKey)] })
        {
            auto warnings = _keymap->LayerJson(bindings);
            // It's possible that the user provided keybindings have some warnings
            // in them - problems that we should alert the user to, but we can
            // recover from. Most of these warnings cannot be detected later in the
            // Validate settings phase, so we'll collect them now. If there were any
            // warnings generated from parsing these keybindings, add them to our
            // list of warnings.
            _keybindingsWarnings.insert(_keybindingsWarnings.end(), warnings.begin(), warnings.end());

            // Now parse the array again, but this time as a list of commands.
            warnings = implementation::Command::LayerJson(_commands, bindings);
        }
    };
    parseBindings(LegacyKeybindingsKey);
    parseBindings(ActionsKey);
}

// Method Description:
// - Adds the given colorscheme to our map of schemes, using its name as the key.
// Arguments:
// - scheme: the color scheme to add
// Return Value:
// - <none>
void GlobalAppSettings::AddColorScheme(const Model::ColorScheme& scheme)
{
    _colorSchemes.Insert(scheme.Name(), scheme);
}

// Method Description:
// - Return the warnings that we've collected during parsing the JSON for the
//   keybindings. It's possible that the user provided keybindings have some
//   warnings in them - problems that we should alert the user to, but we can
//   recover from.
// Arguments:
// - <none>
// Return Value:
// - <none>
std::vector<winrt::Microsoft::Terminal::Settings::Model::SettingsLoadWarnings> GlobalAppSettings::KeybindingsWarnings() const
{
    return _keybindingsWarnings;
}

winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Microsoft::Terminal::Settings::Model::Command> GlobalAppSettings::Commands() noexcept
{
    return _commands.GetView();
}
