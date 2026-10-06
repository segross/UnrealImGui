# Independent UE 5.8 integration review

A fresh subagent reviewed commit `82622b7` without the implementation conversation. The review inspected the UE adapter, vendored ImGui event processing, rendering, shared font atlas, and validation coverage. Small standalone C++ programs reproduced the input ordering, reset, DPI theme, and touch-to-mouse handoff issues against the vendored ImGui implementation.

| Finding | Resulting fix | Regression coverage |
| --- | --- | --- |
| Grouping inputs by type changed move/click and text/navigation order | One ordered stream for keyboard, text, pointer, wheel, gamepad and focus events; Slate button/wheel handlers supply current position | `OrderedPointerAndText` |
| Reset left locally queued and ImGui-trickled events alive | Cancel queued events and clear consumed state by input channel; forward focus loss/recovery | `PendingInputCancellation`, `FastInputAndReset` |
| Gamepad state snapshots lost a press and release between frames | Submit every digital/analog transition, and cancel input on disconnect or navigation disable | `FastGamepadAndTouch`, `GamepadMappingAndDisconnect` |
| Device detection ran after input consumption, losing the first held event at connection | Refresh device/configuration state before consumption; replay held navigation state on attachment | `GamepadConnectionTransitions` includes late detection and first-frame fast taps |
| Runtime DPI changes replaced custom themes with defaults | Preserve non-size properties; scale from an unrounded baseline, detecting user edits per component | `RuntimeDPIAndTheme`, `SharedAtlasAndDPI` |
| A filtered mouse position could poison local deduplication after touch | Let ImGui deduplicate submitted positions; restore the physical pointer after touch; always submit click position first | `FastGamepadAndTouch` includes immediate same-position click after touch release |
| Reused texture slots could make stale IDs render a different registration | Encode a registration generation in IDs and retain it through draw staging and Slate lookup | `TextureIdentityAndGC`, `DrawOffsetsAndCallbacks` |
| External textures could be collected without invalidating registration lookup | Track them weakly, reject dead resources and still allow registration cleanup | `TextureIdentityAndGC` |
| Atlas destruction coverage manually set its frame age | Trigger actual dynamic-font atlas growth, advance frames, collect garbage, retain a slower context's staged command and read back old/new GPU white texels | `AtlasGrowthAndContextLifetime` |

The DPI issue existed before the ImGui upgrade. Texture generations are an API lifecycle improvement: updating a live name retains its ID, while releasing and recreating even the same name invalidates earlier handles. IDs are not persistent storage values. The vendored Dear ImGui files remain unchanged.

The backend also advertises the current `PlatformIO.DrawCallback_ResetRenderState` callback. It is a no-op because Slate controls render state; application draw callbacks still run on the game thread during staging.

The same reviewer performed a final read-only pass over the combined changes. The touch handoff and gamepad attachment regressions above were found during that pass and included in the fixes. Final engine validation and its scope are recorded in [UE58Validation.md](UE58Validation.md).
