// Distributed under the MIT License (MIT) (see accompanying LICENSE file)
// @author	: Sammy Fatnassi
// @brief	: See header file for details

#include "ImguiUnrealCommand.h"

#if IMGUI_UNREAL_COMMAND_ENABLED

#include "HAL/IConsoleManager.h" 
#include "Features/IModularFeatures.h"

#define LOCTEXT_NAMESPACE "ImguiUnrealCommand"

namespace UECommandImgui
{

static FString kPresetHistoryName	= TEXT("Recents");
static int32 kPresetHistoryIndex	= 0;

//#################################################################################################
//#################################################################################################
//   PRIVATE INTERFACE
//#################################################################################################
//#################################################################################################

//=================================================================================================
// Command definition, can be executed and belongs to a Preset
//=================================================================================================
struct PresetEntry
{
	enum class eType : char {Command, Bool, Bitset, Int, Float, String, _Count};
							PresetEntry(const FString& command, IConsoleObject* pConsoleObj);
	FString					mCommand;
	TArray<char>			mCommandUTF8;
	eType					mType;
	bool					mIsCheat;
	IConsoleObject*			mpCommandObj	= nullptr;
};

//=================================================================================================
// Contains a list of commands, matching the requested criterias 
// (filled provided commands or Unreal Commands name starting with one of the filters)
//=================================================================================================
struct Preset
{
	struct Config { FString mName; TArray<FString> mCommands; TArray<FString> mFilters; };

							Preset(const FString& name);
	void					AddCommands(const TArray<FString>& commands){ mConfig.mCommands.Append(commands); mDirty = true; }
	void					AddFilters(const TArray<FString>& filters)	{ mConfig.mFilters.Append(filters); mDirty = true; }
	void					Update();

	Config					mConfig;
	TArray<char>			mNameUTF8;
	uint32					mPresetID;
	TArray<PresetEntry>		mEntries;
	bool					mDirty			= true;
	bool					mSortResults	= true;
};

//=================================================================================================
// Parameters to let user limit which Unreal Commands will be displayed in selected Preset
//=================================================================================================
struct FilterConfig
{
	enum : uint32 { kAllTypeMask = (1<<static_cast<uint32>(PresetEntry::eType::_Count))-1 };
	char					mInput[128]		= {};										// Command that will be executed on 'Enter' and string used to filter the commands displayed
	unsigned int			mShowTypeMask	= kAllTypeMask;								// PresetEntry::eType bitmask
	bool					mShowCheat		= !(UE_BUILD_SHIPPING || UE_BUILD_TEST);	// Toggle showing the 'cheat' commands	
};

//=================================================================================================
// Contains state/settings needed to update/display the UE Command Window
//=================================================================================================
struct CommandContext
{
								~CommandContext();
	inline PresetEntry*			GetEntrySelected();

	bool						mWindowActive		= false;	// If we should display this 'Unreal Commands' window
	int32						mFrameCountdown		= 1;		// Set to 1, so it update on first frame
	TArray<Preset*>				mPresets;						// List of Presets user can choose
	int							mPresetIndex		= 0;		// Selected Preset
	TArray<uint32_t>			mFilteredCommands;				// Indices of valid 'mPressets[active].mEntries' passing current filter value
	FilterConfig				mFilterConfig;					// Filtering values used to reduce list of command displayed
	int							mFilteredIndex		= 0;		// Item selected in the current list of commands
	bool						mPendingSelectCopy	= false;	// Request to copy the selected item into the input field
	bool						mPendingSelectFocus = false;	// Selection changed, makes sure the item is visible in the list
	IConsoleCommandExecutor*	mpCmdExecutor		= nullptr;	// UE Object used to send the command to
};

//=================================================================================================
// List of Default Presets ( name, {list of commands}, {list of filter})
//=================================================================================================
static const Preset::Config sDefaultPresets[] = { 
	{TEXT("Show Flags"),	{}, {"showflag."}},
	{TEXT("Statistics"),	{"Stat Unit", "Stat Fps", "Stat Memory", "Stat Quick", "Stat Game", "Stat Engine", "Stat StatSystem", "Stat MemoryProfiler", "Stat MemoryPlatform", "Stat SceneRendering", "Stat RHI", "Stat InitViews", "Stat ShadowRendering", "Stat LightRendering", "Stat SceneMemory", "Stat SceneUpdate", "Stat TickGroups", "Stat Chaos", "Stat Physics", "Stat Anim", "Stat Ai", "Stat Audio", "Stat UI", "Stat UObjects"}, {}},
	{TEXT("Rendering"),		{"rhi.DumpMemory", "visRT", "VisualizeTexture"}, {"r.", "d3d12."}},
	{TEXT("VFX"),			{}, {"fx."}},
	{TEXT("Animation"),		{}, {"a."}},
	{TEXT("AI"),			{}, {"ai."}},
	{TEXT("Audio"),			{}, {"au."}},
	{TEXT("Net"),			{}, {"net."}},
	{TEXT("Physics"),		{}, {"p."}},
	{TEXT("Slate"),			{}, {"Slate"}},
};

//=================================================================================================
// Utilities functions / methods
//=================================================================================================
inline void StringToUTF8(const FString& string, TArray<char>& stringUtf8Out)
{
	stringUtf8Out.SetNum(string.Len()+1);
	FTCHARToUTF8_Convert::Convert(stringUtf8Out.GetData(), stringUtf8Out.Num(), *string, string.Len()+1);
}

CommandContext::~CommandContext()
{
	for (auto preset : mPresets) {
		delete preset;
	}
}

PresetEntry* CommandContext::GetEntrySelected()
{
	Preset& preset		= *mPresets[mPresetIndex];
	int32 entryIndex	= mFilteredIndex < mFilteredCommands.Num() ? mFilteredCommands[mFilteredIndex] : preset.mEntries.Num();	
	return entryIndex < preset.mEntries.Num() ? &preset.mEntries[entryIndex] : nullptr;	
}

PresetEntry::PresetEntry(const FString& command, IConsoleObject* pConsoleObj)
{
	mCommand		= command;
	mpCommandObj	= pConsoleObj;
	mType			= eType::Command;
	mIsCheat		= pConsoleObj && pConsoleObj->TestFlags(ECVF_Cheat);
	StringToUTF8(mCommand, mCommandUTF8);

	if( mpCommandObj ){
		if( mpCommandObj->IsVariableBool() )		mType = eType::Bool;
		else if( mpCommandObj->IsVariableInt() )	mType = eType::Int;
		else if( mpCommandObj->IsVariableFloat() )	mType = eType::Float;
		else if( mpCommandObj->IsVariableString() )	mType = eType::String;
		else if( mpCommandObj->AsCommand() )		mType = eType::Command;
		else  {
			// Special case to handle Bitset (which is not a valid 'IsVariableBool')
			// There might be a more reliable way of detecting it...
			int valueInt = mpCommandObj->AsVariable()->GetInt();
			if (valueInt >= 0 && valueInt <= 2) {
				mType = eType::Bitset;
			}
		}
	}
}

//=================================================================================================
// Construct a new Preset
//=================================================================================================
Preset::Preset(const FString& name)
{
	mConfig.mName	= name;
	mPresetID		= FCrc::MemCrc32(*name, sizeof(TCHAR) * name.Len());
	StringToUTF8(mConfig.mName, mNameUTF8);
}

//=================================================================================================
// Fetch the first available 'Command Executor' and save it for running inputed commands later
//=================================================================================================
void InitializeExecutor(CommandContext& cmdContext)
{
	if( cmdContext.mpCmdExecutor == nullptr ){
		TArray<IConsoleCommandExecutor*> CommandExecutors = IModularFeatures::Get().GetModularFeatureImplementations<IConsoleCommandExecutor>(IConsoleCommandExecutor::ModularFeatureName());
		if (CommandExecutors.IsValidIndex(0)){
			cmdContext.mpCmdExecutor = CommandExecutors[0];
		}
	}
}

//=================================================================================================
// Find all command entries associated with this Preset
//=================================================================================================
void Preset::Update()
{
	const auto& consoleMgr	= IConsoleManager::Get();
	mDirty					= false;
	int32 reserveSize		= FMath::Max(512, mEntries.Num());
	mEntries.Reset(0);
	mEntries.Reserve(reserveSize);

	//---------------------------------------------------------------------------------------------
	// Add every requested Preset "Command". Try finding associated Unreal Command entry
	for(const auto& command : mConfig.mCommands ){
		int32 index(0);
		FString commandName = command;
		if( commandName.FindChar(TEXT(' '), index) ){
			commandName.LeftChopInline(commandName.Len() - index);
		}
		mEntries.Emplace(command, consoleMgr.FindConsoleObject(*commandName, false));
	}

	//---------------------------------------------------------------------------------------------
	// Process every Preset "Filter", finding every Unreal Commands starting with the requested strings
	TMap< const IConsoleObject*, const IConsoleObject*> commandMap;	// Used to avoid adding same element multiple time
	auto OnConsoleVariable = [this, &commandMap](const TCHAR *Name, IConsoleObject* CVar)
	{
		if (!CVar->TestFlags(ECVF_Unregistered) && !CVar->TestFlags(ECVF_ReadOnly) && !commandMap.Find(CVar)){
			mEntries.Emplace(Name, CVar);
			commandMap.Add(CVar, CVar);
		}
	};

	for(const auto& filter : mConfig.mFilters){
		consoleMgr.ForEachConsoleObjectThatStartsWith(FConsoleObjectVisitor::CreateLambda(OnConsoleVariable), *filter);
	}

	//------------------------------------------------------------------------------------------------
	// Sort the results
	if( mSortResults ){
		mEntries.Sort( [](const PresetEntry& A, const PresetEntry& B) {return A.mCommand < B.mCommand;} );
	}
}

//=================================================================================================
// Populate our list of 'Auto-Complete' with all Unreal Command matching the search filter
//=================================================================================================
void UpdateContent_FilteredCommands(CommandContext& cmdContext)
{	
	const auto& entries			= cmdContext.mPresets[cmdContext.mPresetIndex]->mEntries;
	const uint32 presetCount	= entries.Num();
	FString filterString		= UTF8_TO_TCHAR(cmdContext.mFilterConfig.mInput);
	int32 spaceIndex			= 0;
	if (filterString.FindChar(TEXT(' '), spaceIndex)) {
		filterString.LeftInline(spaceIndex);
	}
	uint32 previousSelection	= cmdContext.mFilteredIndex < cmdContext.mFilteredCommands.Num() ? cmdContext.mFilteredCommands[cmdContext.mFilteredIndex] : 0;
		
	//---------------------------------------------------------------------------------------------
	// Test each Preset entries, to see if they pass the filter
	//---------------------------------------------------------------------------------------------
	TArray<uint32> entriesStartsWith;
	TArray<uint32> entriesContains;
	entriesStartsWith.Reserve(presetCount);
	entriesContains.Reserve(presetCount);

	for(uint32 i(0); i<presetCount; ++i){
		const PresetEntry& entry	= entries[i];
		bool isValid				= (cmdContext.mFilterConfig.mShowTypeMask & (1 << static_cast<uint32>(entry.mType))) != 0;
		isValid						&= !entry.mIsCheat || cmdContext.mFilterConfig.mShowCheat;
		if( isValid ){
			if( filterString.IsEmpty() || entry.mCommand.StartsWith(filterString) )
				entriesStartsWith.Add(i);
			else if( entry.mCommand.Contains(filterString) )
				entriesContains.Add(i);
		}
	}

	//---------------------------------------------------------------------------------------------
	// Give display priority to elements starting with the input string
	//---------------------------------------------------------------------------------------------
	cmdContext.mFilteredCommands.Reset(entriesStartsWith.Num() + entriesContains.Num());
	cmdContext.mFilteredCommands.Append(entriesStartsWith);	
	cmdContext.mFilteredCommands.Append(entriesContains);

	//---------------------------------------------------------------------------------------------
	// Find the previously selected item (if it still exist)
	//---------------------------------------------------------------------------------------------
	cmdContext.mFilteredIndex	= 0;
	for(int32 i=0; i<cmdContext.mFilteredCommands.Num(); ++i){
		if( cmdContext.mFilteredCommands[i] == previousSelection ){
			cmdContext.mFilteredIndex = i;
		}
	}
}

//=================================================================================================
// Try to find a created Preset with same name, or create when not found
//=================================================================================================
Preset& FindOrCreatePreset(CommandContext& cmdContext, const FString& presetName)
{
	uint32 presetID = FCrc::MemCrc32(*presetName, sizeof(TCHAR) * presetName.Len());
	Preset* pFoundPreset = nullptr;
	for (auto preset : cmdContext.mPresets) {
		if (preset->mPresetID == presetID) {
			pFoundPreset = preset;
			break;
		}
	}

	if (!pFoundPreset) {
		cmdContext.mPresets.Add(new Preset(presetName));
		pFoundPreset = cmdContext.mPresets.Last();
	}

	return *pFoundPreset;
}

//=================================================================================================
// Handle keyboard inputs for the Window (filter selection change, command exec, ...)
//=================================================================================================
void UpdateInput( CommandContext& cmdContext )
{
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {

		const bool isValidSelection = cmdContext.mFilteredIndex >= 0 && cmdContext.mFilteredIndex < cmdContext.mFilteredCommands.Num()-1;
		//-----------------------------------------------------------------------------------------
		// Handle Selected item change request
		if( ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) && cmdContext.mFilteredIndex > 0 ){
			cmdContext.mFilteredIndex--;
			cmdContext.mPendingSelectFocus = true;
		}
		if( ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) && cmdContext.mFilteredIndex < cmdContext.mFilteredCommands.Num()-1){
			cmdContext.mFilteredIndex++;
			cmdContext.mPendingSelectFocus = true;
		}

		//-----------------------------------------------------------------------------------------
		// Handle 'Auto-Complete' copy to 'Command Input' field
		if( ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)) && isValidSelection ){
			const Preset& preset				= *cmdContext.mPresets[cmdContext.mPresetIndex];
			const PresetEntry& entry			= preset.mEntries[cmdContext.mFilteredCommands[cmdContext.mFilteredIndex]];
			cmdContext.mPendingSelectCopy		= true;
		}

		//-----------------------------------------------------------------------------------------
		// Execute the command entered in the 'Command Input' field
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) && cmdContext.mpCmdExecutor ) {
			cmdContext.mpCmdExecutor->Exec(UTF8_TO_TCHAR(cmdContext.mFilterConfig.mInput));
		}
	}
}

//=================================================================================================
// Populate the special 'History' Preset, with recently used 'Unreal Commands'
//=================================================================================================
void UpdatePresetCommandHistory(CommandContext& cmdContext)
{
	Preset& presetHistory		= FindOrCreatePreset(cmdContext, kPresetHistoryName);
	presetHistory.mDirty		= true;
	presetHistory.mSortResults	= false; // Want to preserve history order, not name
	presetHistory.mConfig.mCommands.Reset();
	presetHistory.mConfig.mFilters.Reset();
	IConsoleManager::Get().GetConsoleHistory(TEXT(""), presetHistory.mConfig.mCommands);
	
	// Invert the results, listing recent items first
	int32 lastIndex = presetHistory.mConfig.mCommands.Num()-1;
	for(int i=0; i<(lastIndex+1)/2; ++i){
		presetHistory.mConfig.mCommands.SwapMemory(i, lastIndex-i);
	}
}

//=================================================================================================
// Draw Console Command Window
//=================================================================================================
void DrawWindowUI(CommandContext& cmdContext)
{	
	bool bNeedUpdate(false); // Track value filtering changes

	//---------------------------------------------------------------------------------------------
	// Display a list of Presets filter
	// Note: These presets can either be full commands or partial(command category)
	//---------------------------------------------------------------------------------------------
	Preset*& presetSelected	= cmdContext.mPresets[cmdContext.mPresetIndex];
	if (ImGui::BeginCombo("Presets", presetSelected->mNameUTF8.GetData(),ImGuiComboFlags_HeightLarge)){
		for (const auto& preset : cmdContext.mPresets ) {
			if( ImGui::Selectable(preset->mNameUTF8.GetData(), &preset == &presetSelected)) {
				cmdContext.mPresetIndex				= &preset - &cmdContext.mPresets[0];
				cmdContext.mFilteredIndex			= 0;
				cmdContext.mFilteredCommands.Reset();
				bNeedUpdate							= true;
				// Special Case for 'History' Preset, content needs refreshing
				if( cmdContext.mPresetIndex == kPresetHistoryIndex ){
					UpdatePresetCommandHistory(cmdContext);
				}
			}
		}
		ImGui::EndCombo();
	}
	
	//---------------------------------------------------------------------------------------------
	// Filter to restrict 'Auto-Complete' result based on Unreal command type
	//---------------------------------------------------------------------------------------------
	FilterConfig& filterCfg = cmdContext.mFilterConfig;
	bool bAllActive			= (filterCfg.mShowTypeMask & FilterConfig::kAllTypeMask) == FilterConfig::kAllTypeMask;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	bAllActive				&= filterCfg.mShowCheat;
#endif	
	if( ImGui::BeginCombo("Filters", bAllActive ? "No Filter" : "Custom")) {
		bNeedUpdate |= ImGui::CheckboxFlags("Command",	&filterCfg.mShowTypeMask, 1<<static_cast<uint32>(PresetEntry::eType::Command));
		bNeedUpdate |= ImGui::CheckboxFlags("Bool",		&filterCfg.mShowTypeMask, 1<<static_cast<uint32>(PresetEntry::eType::Bool) | 1<<static_cast<uint32>(PresetEntry::eType::Bitset));
		bNeedUpdate |= ImGui::CheckboxFlags("Int",		&filterCfg.mShowTypeMask, 1<<static_cast<uint32>(PresetEntry::eType::Int));
		bNeedUpdate |= ImGui::CheckboxFlags("Float",	&filterCfg.mShowTypeMask, 1<<static_cast<uint32>(PresetEntry::eType::Float));
		bNeedUpdate |= ImGui::CheckboxFlags("String",	&filterCfg.mShowTypeMask, 1<<static_cast<uint32>(PresetEntry::eType::String));
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		bNeedUpdate |= ImGui::Checkbox("Cheat",			&filterCfg.mShowCheat);
	#endif
		ImGui::EndCombo();
	}

	//---------------------------------------------------------------------------------------------
	// Display the command text input
	//---------------------------------------------------------------------------------------------
	struct TextCallback{
		static int CopySelection(ImGuiInputTextCallbackData* data)
		{
			CommandContext& context		= *reinterpret_cast<CommandContext*>(data->UserData);
			const Preset& preset		= *context.mPresets[context.mPresetIndex];
			const PresetEntry& entry	= preset.mEntries[context.mFilteredCommands[context.mFilteredIndex]];
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, &entry.mCommandUTF8[0]);
			data->SelectionStart		= data->SelectionEnd = data->CursorPos = data->BufTextLen;
			context.mPendingSelectCopy	= false;
			return 0;
		}
	};

	if( ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) || cmdContext.mPendingSelectCopy ){
		ImGui::SetItemDefaultFocus();
		ImGui::SetKeyboardFocusHere();
	}
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.4f, 0.2f, 0.5f));
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
	unsigned int textInputFlag	= cmdContext.mPendingSelectCopy ? ImGuiInputTextFlags_CallbackAlways : 0;
	bool bChanged				= ImGui::InputTextWithHint("##Command", "Unreal Command", cmdContext.mFilterConfig.mInput, sizeof(cmdContext.mFilterConfig.mInput), textInputFlag, TextCallback::CopySelection, &cmdContext);
	bNeedUpdate					|= textInputFlag == 0 && bChanged;
	ImGui::PopStyleColor();

	//---------------------------------------------------------------------------------------------
	// Start a count down to update 'AutoComplete' results	
	//---------------------------------------------------------------------------------------------
	cmdContext.mFrameCountdown = bNeedUpdate ? 8 : cmdContext.mFrameCountdown;
}

//=================================================================================================
// Update the current Preset and the displayed filtered entries list
//=================================================================================================
void UpdateContent(CommandContext& cmdContext)
{
	//---------------------------------------------------------------------------------------------
	// Preset update  (when needed)
	//---------------------------------------------------------------------------------------------
	Preset*& presetSelected	= cmdContext.mPresets[cmdContext.mPresetIndex];
	if( presetSelected->mDirty ){
		presetSelected->Update();
		cmdContext.mFrameCountdown = 1;	// Will refresh filtered list immediately
	}

	//---------------------------------------------------------------------------------------------
	// Wait a certain number of frame before updating
	// Note: Wait a few frames of no changes before refreshing the Filtered results.
	//		 This avoid wasted CPU cycle, not refreshing content until user is done typing.
	//---------------------------------------------------------------------------------------------
	if( cmdContext.mFrameCountdown > 0 )
	{
		--cmdContext.mFrameCountdown;
		if( cmdContext.mFrameCountdown != 0 ) 
			return;

		UpdateContent_FilteredCommands(cmdContext);
	}
}

//=================================================================================================
// Display Command list meeting the 'Preset' and 'Filter' requirements
//=================================================================================================
void DrawFilteredList(CommandContext& cmdContext)
{
	const Preset& preset = *cmdContext.mPresets[cmdContext.mPresetIndex];
	//---------------------------------------------------------------------------------------------
	// Display the list of Preset entries fitting the current filter
	//---------------------------------------------------------------------------------------------	
	if( ImGui::BeginChildFrame(ImGui::GetID("Filtered Command(s)"), ImVec2(0.f, ImGui::GetContentRegionAvail().y - 5.f*ImGui::GetTextLineHeightWithSpacing())) )
	{
		for(int32 i(0); i<cmdContext.mFilteredCommands.Num(); ++i){
			bool bClicked(false), bClicked2x(false);
			const PresetEntry& entry = preset.mEntries[cmdContext.mFilteredCommands[i]];

			// Display the Auto-Complete entry
			ImGui::Selectable(entry.mCommandUTF8.GetData(), i == cmdContext.mFilteredIndex);
			bClicked	|= ImGui::IsItemHovered() && ImGui::IsMouseClicked(0);
			bClicked2x	|= ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);

			//Note: Could handle transition between mouseclick->up/down key more smoothly
			if( cmdContext.mPendingSelectFocus && i == cmdContext.mFilteredIndex ){
				ImGui::SetScrollHereY();
				cmdContext.mPendingSelectFocus = false;
			}

			// Show command Tooltip (when one is available)
			if (ImGui::IsItemHovered() && entry.mpCommandObj) {
				const TCHAR* pHelp = entry.mpCommandObj->GetHelp();
				if( pHelp && pHelp[0] != 0 ){
					ImGui::BeginTooltip(); // Avoiding 'SetTooltip' since help string can contain some '%' character, causing issue with printf used internally
					ImGui::TextUnformatted(TCHAR_TO_UTF8(pHelp)); 
					ImGui::EndTooltip();
				}
			}

			// Handle mouse interactions
			if (bClicked2x && cmdContext.mpCmdExecutor) {
				cmdContext.mpCmdExecutor->Exec(*entry.mCommand);
			}
			if (bClicked || bClicked2x) {
				cmdContext.mFilteredIndex		= i;
				cmdContext.mPendingSelectCopy	= true;
			}
		}
		ImGui::EndChildFrame();
	}

	//---------------------------------------------------------------------------------------------
	// Extra informations at the bottom, about currently selected entry
	//---------------------------------------------------------------------------------------------
	const PresetEntry* pEntry = cmdContext.GetEntrySelected();
	if (pEntry && pEntry->mpCommandObj ) {		
		if( ImGui::BeginChildFrame(ImGui::GetID("##Help"),  ImVec2(0.f,0.f), ImGuiWindowFlags_NoBackground))
		{
			static float sAlignPos = ImGui::CalcTextSize("Value:..").x;
			if( pEntry->mpCommandObj->AsVariable() ){
				ImGui::TextColored(ImVec4(0.1f, 1.f, 0.1f, 1.f), "Value:");
				ImGui::SameLine(sAlignPos);
				ImGui::TextUnformatted(TCHAR_TO_UTF8(*(pEntry->mpCommandObj->AsVariable()->GetString())));
			}

			const TCHAR* pHelp = pEntry->mpCommandObj->GetHelp();
			if( pHelp ){
				ImGui::TextColored(ImVec4(0.1f, 1.f, 0.1f, 1.f), "Help:");
				ImGui::SameLine(sAlignPos);
				ImGui::TextWrapped("%s", TCHAR_TO_UTF8(pHelp));
			}
			ImGui::EndChildFrame();
		}
	}
}

//#################################################################################################
//#################################################################################################
//   PUBLIC INTERFACE
//#################################################################################################
//#################################################################################################
 
//=================================================================================================
// Allocate a new state tracking object for this 'Unreal Command' window
//=================================================================================================
CommandContext* Create(bool addDefaultPreset)
{
	CommandContext* pCmdContext = new CommandContext;
	if (pCmdContext)
	{
		// Special Presets that are always present
		AddPresetFilters(pCmdContext, kPresetHistoryName, {""});
		AddPresetFilters(pCmdContext, "All", {""});

		// Add this plugin built-in Presets
		for(int i(0); addDefaultPreset && i<UE_ARRAY_COUNT(sDefaultPresets); i++){
			if (addDefaultPreset) {
				const auto& defaultPreset = sDefaultPresets[i];
				AddPresetCommands(pCmdContext, defaultPreset.mName, defaultPreset.mCommands);
				AddPresetFilters(pCmdContext, defaultPreset.mName, defaultPreset.mFilters);
			}
		}

		// Default selected Preset is 'Commands History', initialize this special Preset
		kPresetHistoryIndex			= pCmdContext->mPresets.Find(&FindOrCreatePreset(*pCmdContext, kPresetHistoryName));		
		pCmdContext->mPresetIndex	= kPresetHistoryIndex;
		
		UpdatePresetCommandHistory(*pCmdContext);
	}
	return pCmdContext;
}

//=================================================================================================
// 
//=================================================================================================
void AddPresetCommands(CommandContext* pCmdContext, const FString& presetName, const TArray<FString>& commands)
{
	if( pCmdContext ){
		Preset& preset = FindOrCreatePreset(*pCmdContext, presetName);
		preset.AddCommands(commands);
	}
}

//=================================================================================================
//
//=================================================================================================
void AddPresetFilters(CommandContext* pCmdContext, const FString& presetName, const TArray<FString>& filters)
{
	if( pCmdContext ){
		Preset& preset = FindOrCreatePreset(*pCmdContext, presetName);
		preset.AddFilters(filters);
	}
}

//=================================================================================================
// Free the allocated resources
//=================================================================================================
void Destroy(CommandContext*& pCmdContext)
{
	if (pCmdContext) {		
		delete pCmdContext;
		pCmdContext = nullptr;
	}
}

//=================================================================================================
// Draw a 'Unreal Console Window' using Dear ImGui
//=================================================================================================
void Show(CommandContext* pCmdContext)
{
	//---------------------------------------------------------------------------------------------
	// Make sure we are initialized properly
	//---------------------------------------------------------------------------------------------
	if (pCmdContext == nullptr) {
		return;
	}
	InitializeExecutor(*pCmdContext);

	//---------------------------------------------------------------------------------------------
	// Display Unreal Command Window and refresh its content when needed
	//---------------------------------------------------------------------------------------------
	if (pCmdContext->mWindowActive)
	{
		if (ImGui::Begin("Unreal Commands [?]", &pCmdContext->mWindowActive))
		{
			if( ImGui::IsItemHovered() ){
				ImGui::SetTooltip(	"[ Unreal Commands browser ]\n"
									"  -Press 'Enter' to execute the 'Command' typed in the input field.\n"
									"  -Press 'Up/Down' to change selection from the Filtered command list.\n"
									"  -Press 'Tab' to copy the selected 'Command' into the input field.\n"
									"  -Double-Click on a 'Command' to execute it directly");
			}
			UpdateInput(*pCmdContext);
			DrawWindowUI(*pCmdContext);
			UpdateContent(*pCmdContext);
			DrawFilteredList(*pCmdContext);
		}
		ImGui::End();
	}
}

//=================================================================================================
// If the UE Command window will be displayed
//=================================================================================================
bool& IsVisible(CommandContext* pCmdContext)
{
	if (pCmdContext == nullptr || pCmdContext->mpCmdExecutor == nullptr) {
		static bool sInvalid;
		sInvalid = false;
		return sInvalid;
	}

	return pCmdContext->mWindowActive;
}

} //namespace UECommandImgui

#undef LOCTEXT_NAMESPACE

#endif // IMGUI_UNREAL_COMMAND_ENABLED
