
namespace NetImgui { namespace Internal { namespace Client {

void ClientTexture::Set( CmdTexture* pCmdTexture )
{
	netImguiDeleteSafe(mpCmdTexture);
	mpCmdTexture	= pCmdTexture;
	mbSent			= pCmdTexture == nullptr;
}

bool ClientTexture::IsValid()const
{
	return mpCmdTexture != nullptr;
}
	
}}} // namespace NetImgui::Internal::Client
