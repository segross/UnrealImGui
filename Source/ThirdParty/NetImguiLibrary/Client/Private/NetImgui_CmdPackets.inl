namespace NetImgui { namespace Internal
{
// @Sammyfreg TODO: Make Offset/Pointer test safer
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
	return (mKeysDownMask[key/64] & (uint64_t(1)<<(key%64))) != 0;
}

}} // namespace NetImgui::Internal
