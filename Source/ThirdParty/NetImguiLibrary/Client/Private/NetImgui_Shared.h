#pragma once

//=================================================================================================
// Include NetImgui_Api.h with almost no warning suppression.
// this is to make sure library user does not need to suppress any
#if defined(_MSC_VER)
#pragma warning (disable: 4464)		// warning C4464: relative include path contains '..'
#endif

#define NETIMGUI_INTERNAL_INCLUDE 1
#include "../NetImgui_Api.h"
#undef NETIMGUI_INTERNAL_INCLUDE

#if NETIMGUI_ENABLED

//=================================================================================================
// Include a few standard c++ header, with additional warning suppression
#include "NetImgui_WarningDisableStd.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include "NetImgui_WarningReenable.h"
//=================================================================================================

//=================================================================================================
// Enable using Dear ImGui Callbacks support to automatically intercept BeginFrame/Render.
// Needs Dear ImGui 1.81+
//=================================================================================================
#ifndef NETIMGUI_IMGUI_CALLBACK_ENABLED
	#define NETIMGUI_IMGUI_CALLBACK_ENABLED (IMGUI_VERSION_NUM >= 18100)
#endif

//=================================================================================================
#include "NetImgui_WarningDisable.h"
namespace NetImgui { namespace Internal
{

constexpr uint64_t	u64Invalid	= static_cast<uint64_t>(-1);
constexpr size_t	sizeInvalid	= static_cast<size_t>(-1);

//=============================================================================
// All allocations made by netImgui goes through here. 
// Relies in ImGui allocator
//=============================================================================
template <typename TType, typename... Args> TType*	netImguiNew(Args... args);
template <typename TType> TType*					netImguiSizedNew(size_t placementSize);
template <typename TType> void						netImguiDelete(TType* pData);
template <typename TType> void						netImguiDeleteSafe(TType*& pData);

class ScopedImguiContext
{
public:
	ScopedImguiContext(ImGuiContext* pNewContext) : mpSavedContext(ImGui::GetCurrentContext()){ ImGui::SetCurrentContext(pNewContext); }
	~ScopedImguiContext() { ImGui::SetCurrentContext(mpSavedContext); }

protected:
	ImGuiContext* mpSavedContext;
};

template<typename TType>
class ScopedValue
{
public:
	ScopedValue(TType& ValueRef, TType Value) 
	: mValueRef(ValueRef)
	, mValueRestore(ValueRef) 
	{
		mValueRef = Value; 
	}
	~ScopedValue() 
	{
		mValueRef = mValueRestore; 
	}
protected:
	TType&	mValueRef;
	TType	mValueRestore;
	uint8_t mPadding[sizeof(void*)-(sizeof(TType)%8)];
	
	// Prevents warning about implicitly delete functions
	ScopedValue(const ScopedValue&) = delete;
	ScopedValue(const ScopedValue&&) = delete;
	void operator=(const ScopedValue&) = delete;
};

using ScopedBool = ScopedValue<bool>;

//=============================================================================
// Class to safely exchange a pointer between two threads
//=============================================================================
template <typename TType>
class ExchangePtr
{
public:
						ExchangePtr():mpData(nullptr){}
						~ExchangePtr();	
	inline TType*		Release();
	inline void			Assign(TType*& pNewData);
	inline void			Free();
private:						
	std::atomic<TType*> mpData;

// Prevents warning about implicitly delete functions
private:
	ExchangePtr(const ExchangePtr&) = delete;
	ExchangePtr(const ExchangePtr&&) = delete;
	void operator=(const ExchangePtr&) = delete;
};

//=============================================================================
// Make data serialization easier
//=============================================================================
template <typename TType>
struct OffsetPointer
{
	inline				OffsetPointer();
	inline explicit		OffsetPointer(TType* pPointer);
	inline explicit		OffsetPointer(uint64_t offset);

	inline bool			IsPointer()const;
	inline bool			IsOffset()const;

	inline TType*		ToPointer();
	inline uint64_t		ToOffset();
	inline TType*		operator->();
	inline const TType*	operator->()const;
	inline TType&		operator[](size_t index);
	inline const TType&	operator[](size_t index)const;

	inline TType*		Get();
	inline const TType*	Get()const;
	inline uint64_t		GetOff()const;
	inline void			SetPtr(TType* pPointer);
	inline void			SetOff(uint64_t offset);
	
private:
	union
	{
		uint64_t	mOffset;
		TType*		mPointer;
	};
};

//=============================================================================
//=============================================================================
// @sammyfreg TODO: re purpose this to a threadsafe consume/append buffer?
template <typename TType, size_t TCount>
class Ringbuffer
{
public:
							Ringbuffer():mPosCur(0),mPosLast(0){}
	void					AddData(const TType* pData, size_t& count);
	void					ReadData(TType* pData, size_t& count);
private:
	TType					mBuffer[TCount] = {0};
	std::atomic_uint64_t	mPosCur;
	std::atomic_uint64_t	mPosLast;

// Prevents warning about implicitly delete functions
private:
	Ringbuffer(const Ringbuffer&) = delete;
	Ringbuffer(const Ringbuffer&&) = delete;
	void operator=(const Ringbuffer&) = delete;
};

template <typename T, std::size_t N>
constexpr std::size_t ArrayCount(T const (&)[N]) noexcept
{
	return N;
}

template <size_t charCount>
inline void StringCopy(char (&output)[charCount], const char* pSrc);

}} //namespace NetImgui::Internal

#include "NetImgui_Shared.inl"
#include "NetImgui_WarningReenable.h"

#endif //NETIMGUI_ENABLED
