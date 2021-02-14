#pragma once

#include "NetImgui_Shared.h"
#include "NetImgui_CmdPackets_DrawFrame.h"

namespace NetImgui { namespace Internal
{

//Note: If updating any of these commands data structure, increase 'CmdVersion::eVersion'

struct CmdHeader
{
	enum class eCommands : uint8_t { Invalid, Ping, Disconnect, Version, Texture, Input, DrawFrame };
				CmdHeader(){}
				CmdHeader(eCommands CmdType, uint16_t Size) : mSize(Size), mType(CmdType){}
	uint32_t	mSize		= 0;
	eCommands	mType		= eCommands::Invalid;
	uint8_t		mPadding[3]	= {0,0,0};
};

struct alignas(8) CmdPing
{
	CmdHeader mHeader = CmdHeader(CmdHeader::eCommands::Ping, sizeof(CmdPing));
};

struct alignas(8) CmdDisconnect
{
	CmdHeader mHeader = CmdHeader(CmdHeader::eCommands::Disconnect, sizeof(CmdDisconnect));
};

struct alignas(8) CmdVersion
{
	enum class eVersion : uint32_t
	{
		Initial				= 1,
		NewTextureFormat	= 2,
		ImguiVersionInfo	= 3,	// Added Dear Imgui/ NetImgui version info to 'CmdVersion'
		ServerRefactor		= 4,	// Change to 'CmdInput' and 'CmdVersion' store size of 'ImWchar' to make sure they are compatible
		// Insert new version here

		//--------------------------------
		_Count,
		_Current			= _Count -1
	};

	CmdHeader	mHeader					= CmdHeader(CmdHeader::eCommands::Version, sizeof(CmdVersion));
	eVersion	mVersion				= eVersion::_Current;
	char		mClientName[16]			= {0};
	char		mImguiVerName[16]		= {IMGUI_VERSION};
	char		mNetImguiVerName[16]	= {NETIMGUI_VERSION};
	uint32_t	mImguiVerID				= IMGUI_VERSION_NUM;
	uint32_t	mNetImguiVerID			= NETIMGUI_VERSION_NUM;
	uint8_t		mWCharSize				= static_cast<uint8_t>(sizeof(ImWchar));
	char		PADDING[3];
};

struct alignas(8) CmdInput
{
	// From Windows Virtual Keys Code
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	enum class eVirtualKeys{ 
		vkMouseBtnLeft		= 0x01,
		vkMouseBtnRight		= 0x02,
		vkMouseBtnMid		= 0x04,
		vkMouseBtnExtra1	= 0x05, // VK_XBUTTON1
		vkMouseBtnExtra2	= 0x06, // VK_XBUTTON2
		vkKeyboardShift		= 0x10,
		vkKeyboardCtrl		= 0x11,
		vkKeyboardAlt		= 0x12,
		vkKeyboardTab		= 0x09,
		vkKeyboardLeft		= 0x25,
		vkKeyboardRight		= 0x27,
		vkKeyboardUp		= 0x26,
		vkKeyboardDown		= 0x28,
		vkKeyboardPageUp	= 0x21,
		vkKeyboardPageDown	= 0x22,
		vkKeyboardHome		= 0x24,
		vkKeyboardEnd		= 0x23,
		vkKeyboardInsert	= 0x2D,
		vkKeyboardDelete	= 0x2E,
		vkKeyboardBackspace	= 0x08,
		vkKeyboardSpace		= 0x20,
		vkKeyboardEnter		= 0x0D,
		vkKeyboardEscape	= 0x1B,
		vkKeyboardSuper1	= 0x5B, // VK_LWIN
		vkKeyboardSuper2	= 0x5C, // VK_LWIN
		vkKeyboardSuperF1	= 0x70, // F1 ... F24
		vkKeyboardA			= 0x41, // A ... Z
		vkKeyboard0			= 0x30,	// 0 ... 9
		vkNumpad0			= 0x60, // 0 ... 9
		vkNumpadAdd			= 0x6B,
		vkNumpadSub			= 0x6D,
		vkNumpadMul			= 0x6A,
		vkNumpadDiv			= 0x6F,
		vkNumpadDecimal		= 0x6E,
	};
	inline bool IsKeyDown(eVirtualKeys vkKey)const;
	inline void SetKeyDown(eVirtualKeys vkKey, bool isDown);

	CmdHeader					mHeader			= CmdHeader(CmdHeader::eCommands::Input, sizeof(CmdInput));
	uint16_t					mScreenSize[2];
	int16_t						mMousePos[2];	
	float						mMouseWheelVert;
	float						mMouseWheelHoriz;
	ImWchar						mKeyChars[1024];		// Input characters	
	uint64_t					mKeysDownMask[512/64];	// List of keys currently pressed (follow Windows Virtual-Key codes)
	uint16_t					mKeyCharCount;			// Number of valid input characters
	uint8_t						PADDING[6]		= {0};
};

struct alignas(8) CmdTexture
{		
	CmdHeader					mHeader			= CmdHeader(CmdHeader::eCommands::Texture, sizeof(CmdTexture));
	OffsetPointer<uint8_t>		mpTextureData;
	uint64_t					mTextureId		= 0;
	uint16_t					mWidth			= 0;
	uint16_t					mHeight			= 0;
	eTexFormat					mFormat			= eTexFormat::kTexFmt_Invalid;	// eTexFormat
	uint8_t						PADDING[3]		= {0};
};

struct alignas(8) CmdDrawFrame
{
	CmdHeader					mHeader			= CmdHeader(CmdHeader::eCommands::DrawFrame, sizeof(CmdDrawFrame));	
	uint32_t					mVerticeCount	= 0;
	uint32_t					mIndiceCount	= 0;
	uint32_t					mIndiceByteSize	= 0;
	uint32_t					mDrawCount		= 0;
	uint32_t					mMouseCursor	= 0;	// ImGuiMouseCursor value
	float						mDisplayArea[4]	= {0};
	uint8_t						PADDING[4];
	OffsetPointer<ImguiVert>	mpVertices;
	OffsetPointer<uint8_t>		mpIndices;
	OffsetPointer<ImguiDraw>	mpDraws;
	inline void					ToPointers();
	inline void					ToOffsets();
};

}} // namespace NetImgui::Internal

#include "NetImgui_CmdPackets.inl"
