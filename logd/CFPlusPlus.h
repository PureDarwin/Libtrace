#ifndef __CFPlusPlus__
#define __CFPlusPlus__

#ifndef __cplusplus
#error This file requires C++.
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <memory>
#include <type_traits>

namespace CFPlusPlus {
template<typename CFType>
class CFDeleter {
public:
	void operator() (CFType ref) {
		CFRelease(ref);
	}
};

template<typename CFType>
class CFHolder : std::unique_ptr<typename std::remove_pointer<CFType>::type, CFDeleter<CFType>> {
public:
	typedef typename std::remove_pointer<CFType>::type struct_type;

	CFHolder(CFType ref) {
		std::unique_ptr<struct_type, CFDeleter<CFType>> other(ref);
		this->swap(other);
	}

	CFHolder<CFType>& operator =(CFType ref) {
		this->reset(&ref);
		return *this;
	}

	operator CFType() {
		return this->get();
	}

	bool null(void) const {
		return this->get() != nullptr;
	}
};

using CFString = CFHolder<CFStringRef>;
using CFDictionary = CFHolder<CFDictionaryRef>;
using CFArray = CFHolder<CFArrayRef>;
}

using namespace CFPlusPlus;

#endif /* __CFPlusPlus__ */
