#pragma once

#include <assert.h>
#include <string.h>

namespace NetImgui { namespace Internal
{

template <typename TType, typename... Args> 
TType* netImguiNew(Args... args)
{
	return new( ImGui::MemAlloc(sizeof(TType)) ) TType(args...);
}

template <typename TType> 
TType* netImguiSizedNew(size_t placementSize)
{
	return new( ImGui::MemAlloc(placementSize > sizeof(TType) ? placementSize : sizeof(TType)) ) TType();
}

template <typename TType> 
void netImguiDelete(TType* pData)
{
	if( pData )
	{
		pData->~TType();
		ImGui::MemFree(pData);
	}
}

template <typename TType> 
void netImguiDeleteSafe(TType*& pData)
{
	netImguiDelete(pData);
	pData = nullptr;
}

//=============================================================================
//=============================================================================
template <typename TType>
TType* ExchangePtr<TType>::Release()
{
	return mpData.exchange(nullptr);
}

//-----------------------------------------------------------------------------
// Take ownership of the provided data.
// If there's a previous unclaimed pointer to some data, release it
//-----------------------------------------------------------------------------
template <typename TType>
void ExchangePtr<TType>::Assign(TType*& pNewData)
{
	netImguiDelete( mpData.exchange(pNewData) );
	pNewData = nullptr;
}

template <typename TType>
void ExchangePtr<TType>::Free()
{
	TType* pNull(nullptr);
	Assign(pNull);
}

template <typename TType>
ExchangePtr<TType>::~ExchangePtr()	
{ 
	Free();
}

//=============================================================================
//
//=============================================================================
template <typename TType>
OffsetPointer<TType>::OffsetPointer()
{
	SetOff(0);
}

template <typename TType>
OffsetPointer<TType>::OffsetPointer(TType* pPointer)
{
	SetPtr(pPointer);
}

template <typename TType>
OffsetPointer<TType>::OffsetPointer(uint64_t offset)
{
	SetOff(offset);
}

template <typename TType>
void OffsetPointer<TType>::SetPtr(TType* pPointer)
{
	mPointer = pPointer;
}

template <typename TType>
void OffsetPointer<TType>::SetOff(uint64_t offset)
{
	mOffset = offset | 0x8000000000000000;
}

template <typename TType>
uint64_t OffsetPointer<TType>::GetOff()const
{
	return mOffset & ~0x8000000000000000;
}

template <typename TType>
bool OffsetPointer<TType>::IsOffset()const
{
	return (mOffset & 0x8000000000000000) != 0;
}

template <typename TType>
bool OffsetPointer<TType>::IsPointer()const
{
	return !IsOffset();
}

template <typename TType>
TType* OffsetPointer<TType>::ToPointer()
{
	assert(IsOffset());
	SetPtr( reinterpret_cast<TType*>( reinterpret_cast<uint64_t>(&mPointer) + GetOff() ) );
	return mPointer;
}

template <typename TType>
uint64_t OffsetPointer<TType>::ToOffset()
{
	assert(IsPointer());
	SetOff( reinterpret_cast<uint64_t>(mPointer) - reinterpret_cast<uint64_t>(&mPointer) );
	return mOffset;
}

template <typename TType>
TType* OffsetPointer<TType>::operator->()
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
const TType* OffsetPointer<TType>::operator->()const
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
TType* OffsetPointer<TType>::Get()
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
const TType* OffsetPointer<TType>::Get()const
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
TType& OffsetPointer<TType>::operator[](size_t index)
{
	assert(IsPointer());
	return mPointer[index];
}

template <typename TType>
const TType& OffsetPointer<TType>::operator[](size_t index)const
{
	assert(IsPointer());
	return mPointer[index];
}

//=============================================================================
template <typename TType, size_t TCount>
void Ringbuffer<TType,TCount>::AddData(const TType* pData, size_t& count)
//=============================================================================
{
	size_t i(0);
	for(; i<count && mPosLast-mPosCur < TCount; ++i)
	{
		mBuffer[(mPosLast + i) % TCount] = pData[i];
		++mPosLast;
	}
	count = i;
}

//=============================================================================
template <typename TType, size_t TCount>
void Ringbuffer<TType,TCount>::ReadData(TType* pData, size_t& count)
//=============================================================================
{
	size_t i(0);
	for(; i<count && mPosLast != mPosCur; ++i)
	{
		pData[i] = mBuffer[mPosCur % TCount];
		++mPosCur;
	}
	count = i;
}


//=============================================================================
// The _s string functions are a mess. There's really no way to do this right
// in a cross-platform way. Best solution I've found is to set just use
// strncpy, infer the buffer length, and null terminate. Still need to suppress
// the warning on Windows.
// See https://randomascii.wordpress.com/2013/04/03/stop-using-strncpy-already/
// and many other discussions online on the topic.
//=============================================================================
template <size_t charCount>
void StringCopy(char (&output)[charCount], const char* pSrc)
{
#if defined(_MSC_VER) && defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
	#pragma warning (push)
	#pragma warning (disable: 4996)
#endif

	strncpy(output, pSrc, charCount - 1);
	output[charCount - 1] = 0;

#if defined(_MSC_VER) && defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning (pop)
#endif
}

}} //namespace NetImgui::Internal
