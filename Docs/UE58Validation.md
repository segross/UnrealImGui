# UE 5.8 validation

Verified on 2026-10-04 with a UE 5.8.0 source editor build, MSVC 14.44, Windows SDK 10.0.22621.0, and an NVIDIA RTX 3080:

| Check | Result |
| --- | --- |
| Win64 Development Editor build, non-unity ImGui module | Passed |
| NullRHI automation | 13/13 passed, zero warnings |
| D3D12 automation, including GPU pixel readback | 13/13 passed, zero warnings |
| Win64 Development Game module source compilation | ImGui and validation host passed; no executable link |
| Win64 Shipping validation host source compilation | Passed with `WITH_IMGUI=0`; no executable link |
| D3D12 editor-hosted game viewport | 161 frames drawn, screenshot captured, normal exit |
| Vendored source, headers, license and documentation | All 17 files SHA256 match upstream `v1.92.9b` |

The [Slate screenshot](Evidence/UE58-ImGui-Slate.png) shows the demo, tables, dynamic 13/20/32 px fonts, and a registered Unreal checkerboard texture. It was visually inspected and its copied bytes verified by SHA256.

The [validation snapshot](Evidence/UE58-ReviewValidation.json) records the final test names/results, build scope, screenshot hash, and hashes of retained local logs. See [the independent review](UE58Review.md) for the findings and fixes.

An independent full Game target build was attempted with `-UsePrecompiled`. This local engine lacks the required Game manifests (including Launch and BuildSettings). Source compilation with UBT module filters then passed for the non-editor Development plugin/host and Shipping host with ImGui excluded. Module filters do not link a game executable; packaged-game support remains unverified. The editor build and game viewport checks above passed independently.

The fixture in `Tests/UE58` loads this plugin in a minimal C++ project. Its C# rules are stored as `.cs.in` templates to prevent duplicate UBT rule discovery inside the plugin. The PowerShell runner stages the project outside the checkout, materializes the rules, and retains build logs, automation reports, and a Slate screenshot.

Run against an already built UE 5.8 Win64 editor:

```powershell
.\Scripts\ValidateUE58.ps1 -EnginePath 'F:\UnrealEngine'
```

Use `-SkipGPU` for a NullRHI run. Without that switch, the runner also uses D3D12, checks GPU pixel readback, and starts an offscreen game viewport showing the demo, tables, dynamic font sizes, and a registered Unreal texture. Inspect the generated `UE58-ImGui-Slate.png` after the run.

Add `-VerifyGameModules` to also compile the Development Game ImGui/host modules and Shipping host source. The fixture conditions its ImGui dependency on `Target.bBuildDeveloperTools` and guards application code with `WITH_IMGUI`; it exercises Shipping exclusion rather than enabling this DeveloperTool plugin in Shipping.

The `ImGui.Integration` automation group covers:

- Named keys and texture handle conversions, including direct `ImGui::Image(handle, size)` use.
- Fast keyboard/mouse press and release events, modifier keys, and resetting held inputs.
- Gamepad buttons, stick dead zones, and release after device disconnection.
- Explicit draw index/vertex offsets above 65,535 vertices, callbacks, and initialized Slate vertex fields.
- Dynamic texture creation, subregion updates, staged resource destruction, and RGBA upload snapshots. GPU runs additionally compare readback colors and unchanged pixels.
- Two contexts sharing an atlas, different DPI scales, dynamically sized fonts, and atlas reference cleanup.
- Mixed move/click and text/navigation arrival order; channel-specific cancellation of locally queued and already trickled input; focus recovery.
- Fast gamepad and touch taps through the Unreal input state, analog disconnect, first-input connection timing, navigation disable, and immediate touch-to-mouse handoff.
- Released/reused/same-name texture identities, stable IDs on live updates, and external UObject garbage collection.
- Runtime DPI round trips, fractional custom themes, user edits at scaled DPI, invalid scales, and the current reset-state callback.
- Actual multi-frame atlas growth, delayed release, GC, a slower context's staged IDs, and old/new atlas GPU readback.

The runner derives expected test names from source and verifies every name, success count, failure count, and warning count in the exported report. Unreal process exit code alone can be zero even when an automation test fails.

These checks cover the UE 5.8 Win64 editor and an editor-hosted game viewport. They do not verify packaged Development/Shipping builds, other engines or platforms, physical gamepad input, interactive multi-PIE sessions, or hot reload.

Draw callbacks run on the game thread while draw data is staged for Slate. The reset-render-state callback is a no-op because Slate owns the graphics state.
