// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "WorldContext.h"


namespace Utilities
{
	const FWorldContext* GetWorldContextFromNetMode(ENetMode NetMode)
	{
		checkf(GEngine, TEXT("GEngine required to get list of worlds."));

		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.World() && WorldContext.World()->GetNetMode() == NetMode)
			{
				return &WorldContext;
			}
		}

		return nullptr;
	}
}
