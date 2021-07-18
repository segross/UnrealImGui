#pragma once

#include "NetImgui_Shared.h"

namespace NetImgui { namespace Internal
{

struct ImguiVert
{
	//Note: Keep these 4 constants in sync with 'ImguiVS.hlsl'
	//Note: If updating this, increase 'CmdVersion::eVersion'
	enum Constants{ kUvRange_Min=-2, kUvRange_Max=2, kPosRange_Min=-2048, kPosRange_Max=4096};
	uint16_t	mPos[2];	
	uint16_t	mUV[2];
	uint32_t	mColor;
};

struct ImguiDraw
{
	uint64_t	mTextureId;
	uint32_t	mIdxCount;
	uint32_t	mIdxOffset;	// In Bytes
	uint32_t	mVtxOffset;	// In ImguiVert Index		
	float		mClipRect[4];
	uint16_t	mIndexSize;	// 2, 4 bytes
	uint8_t		PADDING[2];
};

struct CmdDrawFrame* CreateCmdDrawFrame(const ImDrawData* pDearImguiData, ImGuiMouseCursor cursor);

}} // namespace NetImgui::Internal
