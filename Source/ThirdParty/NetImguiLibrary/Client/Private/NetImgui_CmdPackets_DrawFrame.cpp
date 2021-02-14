#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED
#include "NetImgui_WarningDisable.h"
#include "NetImgui_CmdPackets.h"

namespace NetImgui { namespace Internal
{

template <typename IntType>
IntType RoundUp(IntType Value, IntType Round)
{
	return ((Value + Round -1) / Round) * Round;
}

inline void ImGui_ExtractVertices(ImguiVert* pOutVertices, const ImDrawList* pCmdList)
{
	for(int i(0), count(pCmdList->VtxBuffer.size()); i<count; ++i)
	{
		const auto& Vtx			= pCmdList->VtxBuffer[i];
		pOutVertices[i].mColor	= Vtx.col;
		pOutVertices[i].mUV[0]	= static_cast<uint16_t>((Vtx.uv.x	- static_cast<float>(ImguiVert::kUvRange_Min)) * 0xFFFF / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pOutVertices[i].mUV[1]	= static_cast<uint16_t>((Vtx.uv.y	- static_cast<float>(ImguiVert::kUvRange_Min)) * 0xFFFF / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pOutVertices[i].mPos[0]	= static_cast<uint16_t>((Vtx.pos.x	- static_cast<float>(ImguiVert::kPosRange_Min)) * 0xFFFF / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
		pOutVertices[i].mPos[1]	= static_cast<uint16_t>((Vtx.pos.y	- static_cast<float>(ImguiVert::kPosRange_Min)) * 0xFFFF / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
	}	
}

inline void ImGui_ExtractIndices(uint8_t* pOutIndices, const ImDrawList* pCmdList)
{
	bool is16Bit		= pCmdList->VtxBuffer.size() <= 0xFFFF;
	size_t IndexSize	= is16Bit ? 2 : 4;
	int IndexCount		= pCmdList->IdxBuffer.size();
	// No conversion needed
	if( IndexSize == sizeof(ImDrawIdx) )
	{
		memcpy(pOutIndices, &pCmdList->IdxBuffer.front(), static_cast<size_t>(IndexCount)*IndexSize); 
	}
	// From 32bits to 16bits
	else if(is16Bit)
	{
	 	for(int i(0); i < IndexCount; ++i)
	 		reinterpret_cast<uint16_t*>(pOutIndices)[i] = static_cast<uint16_t>(pCmdList->IdxBuffer[i]);
	}
	// From 16bits to 32bits
	else
	{
		for(int	i(0); i < IndexCount; ++i)
	 		reinterpret_cast<uint32_t*>(pOutIndices)[i] = static_cast<uint32_t>(pCmdList->IdxBuffer[i]);
	}	
}

inline void ImGui_ExtractDraws(uint32_t& indiceByteOffset, uint32_t& vertexIndex, uint32_t& drawIndex, ImguiDraw* pOutDraws, const ImDrawList* pCmdList)
{
	const bool is16Bit = pCmdList->VtxBuffer.size() <= 0xFFFF;
	for(int cmd_i = 0; cmd_i < pCmdList->CmdBuffer.size(); cmd_i++)
	{
		const ImDrawCmd* pCmd	= &pCmdList->CmdBuffer[cmd_i];						
		if( pCmd->UserCallback == nullptr )
		{					
			pOutDraws[drawIndex].mIdxOffset		= indiceByteOffset;
			pOutDraws[drawIndex].mVtxOffset		= vertexIndex;
			pOutDraws[drawIndex].mTextureId		= reinterpret_cast<uint64_t>(pCmd->TextureId);
			pOutDraws[drawIndex].mIdxCount		= pCmd->ElemCount;
			pOutDraws[drawIndex].mIndexSize		= is16Bit ? 2 : 4;
			pOutDraws[drawIndex].mClipRect[0]	= pCmd->ClipRect.x;
			pOutDraws[drawIndex].mClipRect[1]	= pCmd->ClipRect.y;
			pOutDraws[drawIndex].mClipRect[2]	= pCmd->ClipRect.z;
			pOutDraws[drawIndex].mClipRect[3]	= pCmd->ClipRect.w;
			indiceByteOffset					+= pCmd->ElemCount * pOutDraws[drawIndex].mIndexSize;
			drawIndex							+= 1;
		}
	}
	indiceByteOffset = RoundUp(indiceByteOffset, 4u);
}

CmdDrawFrame* CreateCmdDrawDrame(const ImDrawData* pDearImguiData, ImGuiMouseCursor mouseCursor)
{
	//-----------------------------------------------------------------------------------------
	// Find memory needed for all the data
	//-----------------------------------------------------------------------------------------
	uint32_t indiceCount(0), indiceSize(0), drawCount(0), dataSize(sizeof(CmdDrawFrame));
	for(int n = 0; n < pDearImguiData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList	= pDearImguiData->CmdLists[n];
		bool is16Bit				= pCmdList->VtxBuffer.size() <= 0xFFFF;
		indiceCount					+= static_cast<uint32_t>(pCmdList->IdxBuffer.size());
		indiceSize					+= RoundUp(static_cast<uint32_t>(pCmdList->IdxBuffer.size() * (is16Bit ? 2 : 4)), 4u);
		drawCount					+= static_cast<uint32_t>(pCmdList->CmdBuffer.size());	// Allocate maximum possible. Final count can be lower since some are for callbacks
	}		
			
	uint32_t indiceOffset			= dataSize;	
	dataSize						+= RoundUp(indiceSize, 8u);
	uint32_t verticeOffset			= dataSize;	 
	dataSize						+= RoundUp(static_cast<uint32_t>(sizeof(ImguiVert)*static_cast<uint32_t>(pDearImguiData->TotalVtxCount)), 8u);
	uint32_t drawOffset				= dataSize;
	dataSize						+= RoundUp(static_cast<uint32_t>(sizeof(ImguiDraw)*drawCount), 8u);

	//-----------------------------------------------------------------------------------------
	// Allocate Data and init general frame informations
	//-----------------------------------------------------------------------------------------	
	CmdDrawFrame* pDrawFrame		= netImguiSizedNew<CmdDrawFrame>(dataSize);
	uint8_t* pRawData				= reinterpret_cast<uint8_t*>(pDrawFrame);
	pDrawFrame->mVerticeCount		= static_cast<uint32_t>(pDearImguiData->TotalVtxCount);
	pDrawFrame->mIndiceCount		= indiceCount;
	pDrawFrame->mIndiceByteSize		= indiceSize;
	pDrawFrame->mDrawCount			= 0;
	pDrawFrame->mMouseCursor		= static_cast<uint32_t>(mouseCursor);
	pDrawFrame->mDisplayArea[0]		= pDearImguiData->DisplayPos.x;
	pDrawFrame->mDisplayArea[1]		= pDearImguiData->DisplayPos.y;
	pDrawFrame->mDisplayArea[2]		= pDearImguiData->DisplayPos.x + pDearImguiData->DisplaySize.x;
	pDrawFrame->mDisplayArea[3]		= pDearImguiData->DisplayPos.y + pDearImguiData->DisplaySize.y;
	pDrawFrame->mpIndices.SetPtr(reinterpret_cast<uint8_t*>(pRawData + indiceOffset));
	pDrawFrame->mpVertices.SetPtr(reinterpret_cast<ImguiVert*>(pRawData + verticeOffset));
	pDrawFrame->mpDraws.SetPtr(reinterpret_cast<ImguiDraw*>(pRawData + drawOffset));
			
	//-----------------------------------------------------------------------------------------
	// Copy draw data (vertices, indices, drawcall info, ...)
	//-----------------------------------------------------------------------------------------
	uint32_t vertexIndex(0), indiceByteOffset(0), drawIndex(0);	
	for(int n = 0; n < pDearImguiData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList = pDearImguiData->CmdLists[n];
		ImGui_ExtractVertices(&pDrawFrame->mpVertices[vertexIndex], pCmdList);
		ImGui_ExtractIndices(&pDrawFrame->mpIndices[indiceByteOffset], pCmdList);
		ImGui_ExtractDraws(indiceByteOffset, vertexIndex, drawIndex, pDrawFrame->mpDraws.Get(), pCmdList);
		vertexIndex += static_cast<uint32_t>(pCmdList->VtxBuffer.size());
	}	
	pDrawFrame->mDrawCount		= drawIndex;	// Not all DrawCmd generate a draw, update value to actual value
	pDrawFrame->mHeader.mSize	= dataSize - (drawCount-drawIndex)*sizeof(ImguiDraw);
	pDrawFrame->ToOffsets();
	return pDrawFrame;
}

}} // namespace NetImgui::Internal

#include "NetImgui_WarningReenable.h"
#endif //#if NETIMGUI_ENABLED
