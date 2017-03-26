// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

namespace ScopeGuards
{
	// Saves snapshot of the object state and restores it during destruction.
	template<typename T>
	class TStateSaver
	{
	public:

		// Constructor taking target object in state that we want to save.
		TStateSaver(T& Target)
			: Ptr(&Target)
			, Value(Target)
		{
		}

		// Move constructor allowing to transfer state out of scope.
		TStateSaver(TStateSaver&& Other)
			: Ptr(Other.Ptr)
			, Value(MoveTemp(Other.Value))
		{
			// Release responsibility from the other object (std::exchange currently not supported by all platforms).
			Other.Ptr = nullptr;
		}

		// Non-assignable to enforce acquisition on construction.
		TStateSaver& operator=(TStateSaver&&) = delete;

		// Non-copyable.
		TStateSaver(const TStateSaver&) = delete;
		TStateSaver& operator=(const TStateSaver&) = delete;

		~TStateSaver()
		{
			if (Ptr)
			{
				*Ptr = Value;
			}
		}

	private:

		T* Ptr;
		T Value;
	};

	// Create a state saver for target object. Unless saver is moved, state will be restored at the end of scope.
	// @param Target - Target object in state that we want to save
	// @returns State saver that unless moved, will restore target's state during scope exit
	template<typename T>
	TStateSaver<T> MakeStateSaver(T& Target)
	{
		return TStateSaver<T>{ Target };
	}
}
