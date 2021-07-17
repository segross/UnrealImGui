namespace NetImgui { namespace Internal
{
// @sammyfreg TODO: Make Offset/Pointer test safer
void CmdDrawFrame::ToPointers()
{
	if( !mpIndices.IsPointer() ) //Safer to test the first element after CmdHeader
	{
		mpVertices.ToPointer();
		mpIndices.ToPointer();
		mpDraws.ToPointer();
	}
}

void CmdDrawFrame::ToOffsets()
{
	if( !mpIndices.IsOffset() ) //Safer to test the first element after CmdHeader
	{
		mpVertices.ToOffset();
		mpIndices.ToOffset();
		mpDraws.ToOffset();
	}
}

bool CmdInput::IsKeyDown(eVirtualKeys vkKey)const
{
	const uint64_t key = static_cast<uint64_t>(vkKey);
	return (mKeysDownMask[key/64] & (static_cast<uint64_t>(1)<<(key%64))) != 0;
}

void CmdInput::SetKeyDown(eVirtualKeys vkKey, bool isDown)
{
	const size_t keyEntryIndex	= static_cast<uint64_t>(vkKey) / 64;
	const uint64_t keyBitMask	= static_cast<uint64_t>(1) << static_cast<uint64_t>(vkKey) % 64;	
	mKeysDownMask[keyEntryIndex]= isDown ?	mKeysDownMask[keyEntryIndex] | keyBitMask : 
											mKeysDownMask[keyEntryIndex] & ~keyBitMask;
}

bool CmdBackground::operator==(const CmdBackground& cmp)const
{
	bool sameValue(true);
	for(size_t i(0); i<sizeof(CmdBackground)/8; i++){
		sameValue &= reinterpret_cast<const uint64_t*>(this)[i] == reinterpret_cast<const uint64_t*>(&cmp)[i];
	}
	return sameValue;
}

bool CmdBackground::operator!=(const CmdBackground& cmp)const
{
	return (*this == cmp) == false;
}

}} // namespace NetImgui::Internal
