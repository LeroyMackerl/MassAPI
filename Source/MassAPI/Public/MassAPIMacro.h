#pragma once

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	#define MASS_FRAGMENT_ACCEPT_NOT_TRIVIALLY_COPYABLE(FragmentType) \
		template<> \
		struct TMassFragmentTraits<FragmentType> \
		{ \
			enum \
			{ \
				AuthorAcceptsItsNotTriviallyCopyable = true \
			}; \
		}
#else
	#define MASS_FRAGMENT_ACCEPT_NOT_TRIVIALLY_COPYABLE(FragmentType)
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
