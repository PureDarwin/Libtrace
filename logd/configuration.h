#ifndef _LOGD_CONFIGURATION_H_
#define _LOGD_CONFIGURATION_H_

#include <map>
#include <string>
#include <optional>

namespace logd::configuration {
	enum class PersistLevel {
		Inherit,
		Default,
		Info,
		Debug
	};

	enum class PrivacyLevel {
		Private,
		Public
	};

	class Level {
	public:
		PersistLevel Enabled { PersistLevel::Default };
		PersistLevel Persist { PersistLevel::Default };
	};

	class Category {
	public:
		Level LogLevel;
		// Note that log privacy is not currently implemented.
		// They key is currently accepted, but its value is ignored.
		PrivacyLevel DefaultPrivacySetting { PrivacyLevel::Private };
		bool EnableOversizeMessages { false };
	};

	// A "Domain" is either a Subsystem or a Process,
	// as they share the same semantics.
	class Domain {
	public:
		std::map<std::string, Category> Categories;
		std::optional<Category> DefaultCategory { std::nullopt };
	};
}

#endif /* _LOGD_CONFIGURATION_H_ */
