#ifndef __CFPlusPlus__
#define __CFPlusPlus__

#ifndef __cplusplus
#error This file requires C++.
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <memory>

namespace CFPlusPlus {
class CFDeleter {
public:
	void operator() (CFTypeRef *ref) {
		CFRelease(*ref);
	}
};

template<typename CFType>
class CFHolder : std::unique_ptr<CFType, CFDeleter> {
public:
	operator CFType() {
		return this->get();
	}
};

using CFString = CFHolder<CFStringRef>;
using CFDictionary = CFHolder<CFDictionaryRef>;
using CFArray = CFHolder<CFArrayRef>;
}

using namespace CFPlusPlus;

#endif /* __CFPlusPlus__ */
