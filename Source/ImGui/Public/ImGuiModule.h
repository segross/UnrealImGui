// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiDelegates.h"

#include <ModuleManager.h>


class FImGuiModule : public IModuleInterface
{
public:

	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though. Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FImGuiModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FImGuiModule>("ImGui");
	}

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ImGui");
	}

#if WITH_EDITOR
	/**
	 * Add a delegate called at the end of editor debug frame to draw debug controls in its ImGui context, creating
	 * that context on demand.
	 *
	 * @param Delegate - Delegate that we want to add (@see FImGuiDelegate::Create...)
	 * @returns Returns handle that can be used to remove delegate (@see RemoveImGuiDelegate)
	 */
	virtual FImGuiDelegateHandle AddEditorImGuiDelegate(const FImGuiDelegate& Delegate);
#endif

	/**
	 * Add a delegate called at the end of current world debug frame to draw debug controls in its ImGui context,
	 * creating that context on demand.
	 * This function will throw if called outside of a world context (i.e. current world cannot be found).
	 *
	 * @param Delegate - Delegate that we want to add (@see FImGuiDelegate::Create...)
	 * @returns Returns handle that can be used to remove delegate (@see RemoveImGuiDelegate)
	 */
	virtual FImGuiDelegateHandle AddWorldImGuiDelegate(const FImGuiDelegate& Delegate);

	/**
	 * Add shared delegate called for each ImGui context at the end of debug frame, after calling context specific
	 * delegate. This delegate will be used for any ImGui context, created before or after it is registered.
	 *
	 * @param Delegate - Delegate that we want to add (@see FImGuiDelegate::Create...)
	 * @returns Returns handle that can be used to remove delegate (@see RemoveImGuiDelegate)
	 */
	virtual FImGuiDelegateHandle AddMultiContextImGuiDelegate(const FImGuiDelegate& Delegate);

	/**
	 * Remove delegate added with any version of Add...ImGuiDelegate
	 *
	 * @param Handle - Delegate handle that was returned by adding function
	 */
	virtual void RemoveImGuiDelegate(const FImGuiDelegateHandle& Handle);

	/**
	 * Check whether Input Mode is enabled (tests ImGui.InputEnabled console variable).
	 *
	 * @returns True, if Input Mode is enabled (ImGui.InputEnabled != 0) and false otherwise.
	 */
	virtual bool IsInputMode() const;

	/**
	 * Set Input Mode state (sets ImGui.InputEnabled console variable, so it can be used together with a console).
	 *
	 * @param bEnabled - Whether Input Mode should be enabled (ImGui.InputEnabled = 1) or not (ImGui.InputEnabled = 0).
	 */
	virtual void SetInputMode(bool bEnabled);

	/**
	 * Toggle Input Mode state (changes ImGui.InputEnabled console variable).
	 */
	virtual void ToggleInputMode();

	/**
	 * Check whether ImGui Demo is shown (tests ImGui.ShowDemo console variable).
	 *
	 * @returns True, if demo is shown (ImGui.ShowDemo != 0) and false otherwise.
	 */
	virtual bool IsShowingDemo() const;

	/**
	 * Set whether to show ImGui Demo (sets ImGui.ShowDemo console variable, so it can be used together with a console).
	 *
	 * @param bShow - Whether to show ImGui Demo (ImGui.ShowDemo = 1) or not (ImGui.ShowDemo = 0).
	 */
	virtual void SetShowDemo(bool bShow);

	/**
	 * Toggle ImGui Demo (changes ImGui.ShowDemo console variable).
	 */
	virtual void ToggleShowDemo();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	private:

#if WITH_EDITOR
	virtual struct ImGuiContext** GetImGuiContextHandle();
#endif
};
