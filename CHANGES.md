This is only a summary provided to give a quick overview of changes. It does not list details which can be found in commit messages. If you think that more detailed changelog would be beneficiary, please raise it as an issue.

Versions marked as 'unofficial' are labelled only for the needs of this changelog. Officially I maintain version numbers since 2019/04, starting from version 1.14. If you have any of the earlier commits then you will see plugin signed as a version 1.0.

Change History
--------------

Version: 1.22 (2021/04)
- Fixed potential for initialization fiasco when using delegates container.
- Fixed bug in code protecting redirecting handles from self-referencing.
- Fixed cached resource handles getting invalid after reloading texture resources.

Version: 1.21 (2020/07-09)
Improving stability
- Fixed a crash in the input handler caused by invalidated by hot-reload instance trying to unregister a delegate.
- Improved hot-reload stability and support for reloading after recompiling outside of the editor. Both methods should be equally supported and work together.
- Improved behaviour of delegates when hot-reloading.
- Changed context index mapping to fix issues with multi-PIE debugging in 4.25.
- Fixed Linux crash caused by wrong mapping of key codes.

Version: 1.20 (2020/06)
Transition to IWYU and maintenance:
- Replaced includes of monolithic headers.
- Removed explicit PCH and switched to IWYU-style PCH model.
- Fixed includes to compile without explicit PCH in non-unity mode.
- Fixed a few issues causing compilation errors in older engine versions.
- Fixed debug code to compile on platforms using other than char or wchar_t characters.
- Fixed issues in recently added DPI scaling.
- Cleaned obsolete and unused code.
- Replaced custom scope guards with the template provided by the engine.

Version: 1.19 (2020/03-06)
- Integrated fix for issue with ImGui popup/modal windows not being able to be closed in transparent mouse input mode. 
- Integrated first version of Adaptive Canvas Size.
- Added different options to define canvas size, with Adaptive Canvas Size being one of the options (viewport).
- Added option for DPI scaling.

Version: 1.18 (2020/01)
- Updated to engine version 4.24.
- Updated to ImGui version 1.74.

Version: 1.17 (2019/04)
- Added experimental support for touch input.
- Integrated fixes allowing to build this as an engine plugin:
- Added support for sharing with game mouse input.
- Refactorization of input handling, with changes in SImGuiWidget and compatibility breaking changes in UImGuiInputHandler.

Version: 1.16 (2019/05)
- Fixed issue with SImGuiLayout blocking mouse input for other Slate widgets, which was introduced by refactorization of widgets (version 1.14, commit c144658f).

Version: 1.15 (2019/04)
- Added new FImGuiDelegates interface for ImGui debug delegates.
- Added code preserving delegates during hot-reloading and moving them to a new module.
- DEPRECIATED old FImGuiModule delegates interface and FImGuiDelegateHandle.
- Delegates registered with depreciated interface are redirected and get benefit of being preserved during hot-reloading. This can be controlled with IMGUI_REDIRECT_OBSOLETE_DELEGATES.
- Added IMGUI_WITH_OBSOLETE_DELEGATES allowing to strip depreciated interface from builds (that interface will be officially removed in one of later releases).
- Added new ImGui early debug delegates called during world tick start.
- Delegates are called in fixed order: multi-context early debug, world early debug (called during world tick start), world debug, multi-context debug (called during world post actor tick or if not available, during world tick start).
- Removed from build script configuration of debug delegates.

Version: 1.14 (2019/03)
- Added SImGuiLayout to resets layout for SImGuiWidget.
- Refactored rendering in SImGuiWidget to take advantage of layout reset.
- Reworked ImGui canvas dragging and scaling and moved it to SImGuiCanvasControl.
- Removed dependency on ImGui internal cursor data.

Version: 1.13 (unofficial) (2019/03)
- Fixed mapping from FKey to ImGui key index to gracefully handle unsupported keys and work on platforms that do not support all the keys defined in ImGui key map.
- Fixed non-unity compile warnings and errors. 

Version: 1.12 (unofficial) (2018/12)
- Added support for sharing with game keyboard and gamepad input.
- Added FImGuiModuleSettings to handle delayed loading of UImGuiSettings and serve as settings proxy for other classes.

Version: 1.11 (unofficial) (2018/10-11)
- Moved ImGui Draw events to be called at the end of the world update during post actor tick event. Only available in UE 4.18 or later, with old behaviour available as an option.
- Replaced console variable based configuration of ImGui Draw events with macros.
- Replaced console variable based configuration of software cursor with a setting.
- Console variables and logging that are primarily focused on module development are hidden by default and can be enabled by setting IMGUI_MODULE_DEVELOPER to 1.
- Replaced console variables with module properties and settings.
- Added console commands to control module properties.
- Added support to preserve and move module properties to hot-reloaded module.
- Moved properties to public interface.
- Added FImGuiModule interface to access properties instance.
- DEPRECIATED FImGuiModule functions to modify single properties.

Version: 1.10 (unofficial) (2018/10)
- Changed module type to 'Developer' to make it easier strip it from shipping or other builds.
- Added runtime loader to allow automatically load developer module in runtime builds.

Version: 1.09 (unofficial) (2018/08-09)
- Added interface to register user textures for use in ImGui.
- Fixed bad deallocation in Texture Manager.
- Updated to ImGui 1.61.
- Updated to UE 4.20.

Version: 1.08 (unofficial) (2018/07-08)
- Added ImGui Input Handler to allow to customization of input handling.
- Added ImGui settings with editor page in project properties.
- Added command to switch input mode with configurable key binding.
- Added backward compatibility macros.
- Fixed hot-reloading issues with using ImGui implementation.

Version: 1.07 (unofficial) (2018/05)
- Improved initialization to allow loading module in any loading phase.

Version: 1.06 (unofficial) (2018/05)
- Updated to ImGui 1.61
- Added support for gamepad and keyboard navigation.

Version: 1.05 (unofficial) (2018/04)
- Added mode to scale and drag ImGui canvas.
- Using ImGui internal cursor data to draw drag icon.

Version: 1.04 (unofficial) (2018/03)
- Minimised lag between ending ImGui frame and rendering draw data in Slate.
- Moved ImGui Draw event to be called during world tick start with configuration to use old behaviour.

Version: 1.03 (unofficial) (2018/01-03)
- Fixed warnings and errors found in non-unity, Linux, PS4 or XBox builds.
- Added configuration to choose between hardware and software cursor with hardware cursor used by default.

Version: 1.02 (unofficial) (2018/01)
- Updated to ImGui 1.53.
- Fixed problems with ImGui Demo working in multi-context environment.
- Added FImGuiModule interface to change input mode and demo visibility.
- Fixed input state issues after application loses focus.
- Added input state debugging and `ImGui.Debug.Input` console variable to switch it.

Version: 1.01 (unofficial) (2017/10)
- Added `ImGui.ShowDemo` console variable to show/hide ImGui demo.
- Added automatic switching to right ImGui context at the beginning of the world tick.
- Updated to UE 4.18

Version: <=1.00 (unofficial) (2017/04-10)
- Added ImGui source code as an external module to expose it in IDE.
- Integrated ImGui source code to build as part of the ImGui module.
- Added FImGuiModule to implement ImGui module interface.
- Added FImGuiModuleManager to control other module components.
- Added FTextureManager to manage texture resources and map them to ImTextureID.
- Added SImGuiWidget to handle Slate input and render in Slate ImGui draw data.
- Added FImGuiInputState to collect and store input before it can be copied to ImGui IO.
- Added FContextProxy to represent a single ImGui context.
- Added FImGuiContextManager to create and manage ImGui context proxies.
- Added Multi-PIE support with each PIE instance getting a dedicated ImGui context proxy, input state and widget.
- Added `ImGui.InputEnabled` console variable to control whether input mode is enabled.
- Added widget debugging and `ImGui.Debug.Widget` console variable to switch it.
- Added ImGui software cursor.
- Added support for session reloading with ini file names based on world type and PIE index.
- Added FImGuiModule interface to register ImGui delegates called to draw ImGui controls.
