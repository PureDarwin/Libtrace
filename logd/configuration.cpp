#include "configuration.h"
#include "CFPlusPlus.h"
#include "logd_common.h"
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>

using namespace logd::configuration;

namespace /* anonymous */ {

template<typename T>
class malloc_deleter {
public:
	void operator() (T ptr) {
		free(ptr);
	}
};

template<typename TValue>
inline TValue CFDictionaryCopyObject(CFDictionaryRef dict, CFStringRef key) {
	return (TValue) CFRetain((CFTypeRef) CFDictionaryGetValue(dict, key));
}

inline bool stri_equal(const char *lhs, const char *rhs) {
	return strcasecmp(lhs, rhs) == 0;
}

std::optional<Category> ParseCategory(const CFDictionary& plistValue) {
	CFDictionary levelDict = CFDictionaryCopyObject<CFDictionaryRef>(plistValue, CFSTR("Level"));
	if (levelDict.null()) return Category();

	if (levelDict.typeID() != CFDictionaryGetTypeID()) return std::nullopt;

	CFString enable = CFDictionaryCopyObject<CFStringRef>(levelDict, CFSTR("Enable"));
	CFString persist = CFDictionaryCopyObject<CFStringRef>(levelDict, CFSTR("Persist"));

	PersistLevel enableLevel = PersistLevel::Inherit;
	if (!enable.null()) {
		if (enable.typeID() != CFStringGetTypeID()) return std::nullopt;
		const char *enableValue = CFStringGetCStringPtr(enable, kCFStringEncodingUTF8);

		if (stri_equal(enableValue, "Inherit")) enableLevel = PersistLevel::Inherit;
		else if (stri_equal(enableValue, "Default")) enableLevel = PersistLevel::Default;
		else if (stri_equal(enableValue, "Info")) enableLevel = PersistLevel::Info;
		else if (stri_equal(enableValue, "Debug")) enableLevel = PersistLevel::Debug;
		else return std::nullopt;
	}

	PersistLevel persistLevel = PersistLevel::Inherit;
	if (!persist.null()) {
		if (enable.typeID() != CFStringGetTypeID()) return std::nullopt;
		const char *persistValue = CFStringGetCStringPtr(persist, kCFStringEncodingUTF8);

		if (stri_equal(persistValue, "Inherit")) persistLevel = PersistLevel::Inherit;
		else if (stri_equal(persistValue, "Default")) persistLevel = PersistLevel::Default;
		else if (stri_equal(persistValue, "Info")) persistLevel = PersistLevel::Info;
		else if (stri_equal(persistValue, "Debug")) persistLevel = PersistLevel::Debug;
		else return std::nullopt;
	}

	Category retval;
	retval.LogLevel.Enabled = enableLevel;
	retval.LogLevel.Persist = persistLevel;

	CFString privacySetting = CFDictionaryCopyObject<CFStringRef>(plistValue, CFSTR("Default-Privacy-Setting"));
	if (!privacySetting.null()) {
		if (privacySetting.typeID() != CFStringGetTypeID()) return std::nullopt;
		const char *privacyValue = CFStringGetCStringPtr(privacySetting, kCFStringEncodingUTF8);

		if (stri_equal(privacyValue, "Private")) retval.DefaultPrivacySetting = PrivacyLevel::Private;
		else if (stri_equal(privacyValue, "Public")) retval.DefaultPrivacySetting = PrivacyLevel::Public;
		else return std::nullopt;
	}

	CFHolder<CFBooleanRef> enableOversized = CFDictionaryCopyObject<CFBooleanRef>(plistValue, CFSTR("Enable-Oversize-Messages"));
	if (!enableOversized.null()) {
		if (enableOversized.typeID() != CFBooleanGetTypeID()) return std::nullopt;
		retval.EnableOversizeMessages = CFBooleanGetValue(enableOversized);
	}

	return retval;
}

} // anonymous namespace

std::optional<Domain> Domain::ReadDomain(const char *plistPath) {
	CFString cfPath = CFStringCreateWithCString(nullptr, plistPath, kCFStringEncodingUTF8);
	if (cfPath.null()) return std::nullopt;

	CFURL url = CFURLCreateWithFileSystemPath(nullptr, cfPath, kCFURLPOSIXPathStyle, false);
	if (url.null()) return std::nullopt;

	CFHolder<CFReadStreamRef> readStream = CFReadStreamCreateWithFile(nullptr, url);
	if (readStream.null()) return std::nullopt;

	CFDictionary domainPlist = (CFDictionaryRef) CFPropertyListCreateWithStream(nullptr, readStream, 0, 0, nullptr, nullptr);
	if (domainPlist.null()) return std::nullopt;

	Domain domain;

	CFIndex count = CFDictionaryGetCount(domainPlist);
	std::unique_ptr<void *, malloc_deleter<void *>> keys((void **)calloc(count, sizeof(void *)));
	std::unique_ptr<void *, malloc_deleter<void *>> values((void **)calloc(count, sizeof(void *)));
	CFDictionaryGetKeysAndValues(domainPlist, (const void **)keys.get(), (const void **)values.get());

	for (CFIndex i = 0; i < count; i++) {
		CFStringRef key = (CFStringRef)keys.get()[i];
		CFDictionaryRef value = (CFDictionaryRef)values.get()[i];

		auto category = ParseCategory((CFDictionaryRef)CFRetain(value));
		if (category.has_value()) {
			if (CFEqual(key, CFSTR("DEFAULT-OPTIONS"))) {
				domain.DefaultCategory = category.value();
			} else {
				std::string keyString(CFStringGetCStringPtr(key, kCFStringEncodingUTF8));
				domain.Categories[keyString] = category.value();
			}
		} else {
			return std::nullopt;
		}
	}

	return domain;
}

namespace /* anonymous */
{

void ParseDomainsInDirectory(const char *directory, std::map<std::string, Domain> &domains) {
	DIR *dirp = opendir(directory);
	if (dirp != nullptr) {
		struct dirent *entry;
		while ((entry = readdir(dirp)) != nullptr) {
			if (entry->d_type == DT_REG) {
				char *plistPath;
				asprintf(&plistPath, "%s/%s", directory, entry->d_name);
				std::optional<Domain> domain = Domain::ReadDomain(plistPath);
				free(plistPath);

				if (domain) {
					char *identifier = strdup(entry->d_name);
					identifier[strlen(identifier) - strlen(".plist")] = '\0';
					domains[identifier] = domain.value();
				} else {
					char *message;
					asprintf(&message, "Ignoring \"%s/%s\": Syntax or semantic error found while loading plist", directory, entry->d_name);
					logd_append_log_entry(OS_LOG_TYPE_ERROR, "com.apple.logd", "DomainLoading", message, time(NULL), nullptr, 0);
					free(message);
				}
			} else {
				char *message;
				asprintf(&message, "Ignoring \"%s/%s\": Not a regular file", directory, entry->d_name);
				logd_append_log_entry(OS_LOG_TYPE_ERROR, "com.apple.logd", "DomainLoading", message, time(NULL), nullptr, 0);
				free(message);
			}
		}
	} else {
		char *message;
		asprintf(&message, "Could not open \"/System/Library/Preferences/Logging/Subsystems\": %s", strerror(errno));
		logd_append_log_entry(OS_LOG_TYPE_ERROR, "com.apple.logd", "DomainLoading", message, time(NULL), nullptr, 0);
		free(message);
	}
}

}

std::map<std::string, Domain> Domain::GetAllUserDomains(bool rescan) {
	static std::map<std::string, Domain> domains;

	if (rescan) domains.clear();
	if (domains.size() == 0) {
		domains.clear();

		ParseDomainsInDirectory("/System/Library/Preferences/Logging/Subsystems", domains);
		ParseDomainsInDirectory("/System/Library/Preferences/Logging/Processes", domains);
		ParseDomainsInDirectory("/Library/Preferences/Logging/Subsystems", domains);
		ParseDomainsInDirectory("/Library/Preferences/Logging/Processes", domains);
	}

	return domains;
}

std::map<std::string, Domain> Domain::GetDomainsForUser(uid_t uid, bool rescan) {
	static std::map<uid_t, std::map<std::string, Domain>> userMap;

	if (rescan) userMap.erase(uid);
	if (userMap.find(uid) == userMap.end()) {
		std::map<std::string, Domain> domains;

		long pwd_bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (pwd_bufsize == -1) {
			_setcrashlogmessage("sysconf(_SC_GETPW_R_SIZE_MAX) failed: %s", strerror(errno));
			__builtin_trap();
		}

		char pwd_buffer[pwd_bufsize];
		struct passwd pwd, *pwd_result;
		if (getpwuid_r(uid, &pwd, pwd_buffer, pwd_bufsize, &pwd_result) == 0 && pwd_result != nullptr) {
			char *path;

			asprintf(&path, "%s/Library/Preferences/Logging/Subsystems", pwd.pw_dir);
			ParseDomainsInDirectory(path, domains);
			free(path);

			asprintf(&path, "%s/Library/Preferences/Logging/Processes", pwd.pw_dir);
			ParseDomainsInDirectory(path, domains);
			free(path);
		} else {
			char *message;
			asprintf(&message, "getpwuid_r(%d) failed: %s", uid, strerror(errno));
			logd_append_log_entry(OS_LOG_TYPE_ERROR, "com.apple.logd", "DomainLoading", message, time(NULL), nullptr, 0);
			free(message);
		}

		userMap[uid] = domains;
	}

	return userMap[uid];
}
