// file-compat
// https://github.com/brackeen/file-compat

#ifndef FILE_COMPAT_H
#define FILE_COMPAT_H

/**
    ## Redefined Functions (Windows, Android):

    | Function            | Windows                      | Android
    |---------------------|------------------------------|------------------------------------------
    | `printf`            | Uses `OutputDebugString`     | Uses `__android_log_print`
    | `fopen`             | Uses `fopen_s`               | Uses `AAssetManager_open` if read mode
    | `fclose`            | Adds `NULL` check            | No change

    Note: `OutputDebugString` is only used if the debugger is present and no console is allocated.
    Otherwise uses `printf`.

    ## Added Functions (Windows, Linux, macOS, iOS, Android, Emscripten):

    | Function     | Description
    |--------------|--------------------------------------------------------------------------------
    | `fc_locale`  | Gets the user's preferred language (For example, "en-US").
    | `fc_resdir`  | Gets the current executable's resources directory.
    | `fc_datadir` | Gets the current executable's data directory, useful for saving preferences.
    | `fc_cachedir`| Gets the current executable's cache directory, useful for saving downloaded files.
    
    ## Usage:

    For Android, define `FILE_COMPAT_ANDROID_ACTIVITY` to be a reference to an `ANativeActivity`
    instance or to a function that returns an `ANativeActivity` instance.

        #include "file_compat.h"
        #define FILE_COMPAT_ANDROID_ACTIVITY functionThatGetsAndroidActivity()

        ANativeActivity *functionThatGetsAndroidActivity(void);

 */

#include <stdio.h>
#include <errno.h>

#if defined(_WIN32)
#  define FC_DIRECTORY_SEPARATOR '\\'
#else
#  define FC_DIRECTORY_SEPARATOR '/'
#endif

#if defined(__GNUC__)
#  define FC_UNUSED __attribute__ ((unused))
#else
#  define FC_UNUSED
#endif

/// Gets the path to the current executable's resources directory.
/// The path will have a trailing slash (or backslash on Windows).
///
/// The returned path is:
///   * Windows:            The path to the executable's directory.
///   * Linux:              The path to the executable's directory.
///   * macOS (executable): The path to the executable's directory.
///   * macOS (bundled):    The path to the bundle's resources.
///   * iOS:                The path to the bundle's resources.
///   * Android:            An empty string.
///   * Emscripten          An empty string.
///
/// - Parameters:
///   - path: The buffer to fill the path. No more than `path_max` bytes are written to the buffer,
///           including the trailing 0. If failure occurs, the path is set to an empty string.
///   - path_max: The length of the buffer. Should be `PATH_MAX`.
///
/// - Returns: 0 on success, -1 on failure.
static int fc_resdir(char *path, size_t path_max) FC_UNUSED;

/// Gets the path to the current executable's data directory. It is useful for saving preferences.
/// The path will have a trailing slash (or backslash on Windows).
///
/// The data directory is writable and unique to the executable.
///
/// The returned path is:
///  * Windows:             `%HOMEPATH%\\AppData\\Roaming\\<app_id>\\`
///  * Linux:               `~/.local/share/<app_id>/`
///  * macOS (executable):  `~/Library/Application Support/<app_id>/`
///  * macOS (bundled):     `~/Library/Application Support/<bundle_id>/`
///  * macOS (sandboxed):   `~/Library/Containers/<bundle_id>/Data/Library/Application Support/`
///  * iOS:                 Local path determined by the system (not using `app_id`).
///  * Android:             Local path from `getFilesDir` (not using `app_id`).
///  * Emscripten:          `/home/web_user/.local/share/<app_id>/`
///
/// The directory will be created if it does not exist.
///
/// On Unix-like platforms, if a subdirectory of this directory is needed, it should be created
/// with mode `0700` (octal).
///
/// On Emscripten, to persist data, the path has to be mounted and synchronized to an IDBFS
/// instance. Otherwise, the files created only exist in memory.
///
/// - Parameters:
///   - app_id: The application id, like "MyApp".
///   - path: The buffer to fill the path. No more than `path_max` bytes are written to the buffer,
///           including the trailing 0. If failure occurs, the path is set to an empty string.
///   - path_max: The length of the buffer. Should be `PATH_MAX`.
///
/// - Returns: 0 on success, -1 on failure.
static int fc_datadir(const char *app_id, char *path, size_t path_max) FC_UNUSED;

/// Gets the path to the current executable's cache directory.
/// It is useful for saving downloaded files.
/// The path will have a trailing slash (or backslash on Windows).
///
/// The cache directory is writable and unique to the executable.
///
/// The returned path is:
///  * Windows:             `%HOMEPATH%\\AppData\\Local\\<app_id>\\`
///  * Linux:               `~/.cache/<app_id>/`
///  * macOS (executable):  `~/Library/Caches/<app_id>/`
///  * macOS (bundled):     `~/Library/Caches/<bundle_id>/`
///  * macOS (sandboxed):   `~/Library/Containers/<bundle_id>/Data/Library/Caches/`
///  * iOS:                 Local path determined by the system (not using `app_id`).
///  * Android:             Local path from `getCacheDir` (not using `app_id`).
///  * Emscripten:          `/home/web_user/.cache/<app_id>/`
///
/// The directory will be created if it does not exist.
///
/// On Unix-like platforms, if a subdirectory of this directory is needed, it should be created
/// with mode `0700` (octal).
///
/// On Emscripten, to persist data, the path has to be mounted and synchronized to an IDBFS
/// instance. Otherwise, the files created only exist in memory.
///
/// - Parameters:
///   - app_id: The application id, like "MyApp".
///   - path: The buffer to fill the path. No more than `path_max` bytes are written to the buffer,
///           including the trailing 0. If failure occurs, the path is set to an empty string.
///   - path_max: The length of the buffer. Should be `PATH_MAX`.
///
/// - Returns: 0 on success, -1 on failure.
static int fc_cachedir(const char *app_id, char *path, size_t path_max) FC_UNUSED;

/// Gets the preferred user language in BCP-47 format.
///
/// Valid examples are "en", "en-US", "zh-Hans", and "zh-Hans-HK". Some platforms may return values
/// in lowercase ("en-us" instead of "en-US").
///
/// - Parameters:
///   - locale: The buffer to fill the locale. No more than `locale_max` bytes are written to the
///             buffer, including the trailing 0. If failure occurs, the locale is set to an empty
///             string.
///   - locale_max: The length of the buffer. This value must be at least 3.
///
/// - Returns: 0 on success, -1 on failure.
static int fc_locale(char *locale, size_t locale_max) FC_UNUSED;

// MARK: - Private

#if !defined(_WIN32)
#  include <limits.h> // PATH_MAX
#endif

#ifdef __cplusplus
#  define FC_STATIC_CAST(value_type) static_cast<value_type>
#  define FC_REINTERPRET_CAST(value_type) reinterpret_cast<value_type>
#else
#  define FC_STATIC_CAST(value_type) (value_type)
#  define FC_REINTERPRET_CAST(value_type) (value_type)
#endif

static void fc__locale_clean(char *locale) {
    // Convert underscore to dash ("en_US" to "en-US")
    // Remove encoding ("en-US.UTF-8" to "en-US")
    char *ch = locale;
    while (*ch != 0) {
        if (*ch == '_') {
            *ch = '-';
        } else if (*ch == '.') {
            *ch = 0;
            break;
        }
        ch++;
    }
}

#if defined(__unix__) && !defined(__ANDROID__)

#include <stdlib.h> // getenv
#include <sys/stat.h> // mkdir

/// *Unix only:* Gets a path from an environment variable, and appends `app_id` to it.
/// If the environment variable is not found, the `default_path` is used.
///
/// If `env_var` is available, the resulting path is `getenv(env_var)/app_id/`.
/// Otherwise, the resulting path is `getenv("HOME")/default_path/app_id/`.
///
/// Example: `fc__unixdir("XDG_DATA_HOME", ".local/share", "MyApp", path, path_max);`
static int fc__unixdir(const char *env_var, const char *default_path,
                       const char *app_id, char *path, size_t path_max) {
    int result = -1;
    const char *env_path = getenv(env_var);
    if (env_path && *env_path) {
        result = snprintf(path, path_max, "%s/%s/", env_path, app_id);
    } else {
        const char *home_path = getenv("HOME");
        if (home_path && *home_path) {
            result = snprintf(path, path_max, "%s/%s/%s/", home_path, default_path, app_id);
        }
    }
    if (result <= 0 || FC_STATIC_CAST(size_t)(result) >= path_max) {
        path[0] = 0;
        return -1;
    }
    char *ch = path;
    while (*(++ch)) {
        if (*ch == '/') {
            *ch = 0;
            if (mkdir(path, 0700) != 0 && errno != EEXIST) {
                path[0] = 0;
                return -1;
            }
            *ch = '/';
        }
    }
    return 0;
}

#endif // defined(__unix__)

// MARK: - Apple

#if defined(__APPLE__)

#include <TargetConditionals.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <sys/stat.h> // mkdir
#if defined(__OBJC__) && __has_feature(objc_arc)
#  define FC_AUTORELEASEPOOL_BEGIN @autoreleasepool {
#  define FC_AUTORELEASEPOOL_END }
#else
#  define FC_MSG_SEND (FC_REINTERPRET_CAST(id (*)(id, SEL))(objc_msgSend))
#  define FC_AUTORELEASEPOOL_BEGIN { \
       id autoreleasePool = FC_MSG_SEND(FC_MSG_SEND(FC_REINTERPRET_CAST(id)(objc_getClass("NSAutoreleasePool")), \
           sel_registerName("alloc")), sel_registerName("init"));
#  define FC_AUTORELEASEPOOL_END \
       FC_MSG_SEND(autoreleasePool, sel_registerName("release")); }
#endif

#ifdef __cplusplus
extern "C"
#else
extern
#endif
id NSSearchPathForDirectoriesInDomains(NSUInteger directory, NSUInteger domainMask,
                                       BOOL expandTilde);

/// *Apple only:* Gets a path using `NSSearchPathForDirectoriesInDomains`
/// - Parameter searchPathDirectory: A `NSSearchPathDirectory`.
static int fc__appledir(NSUInteger searchPathDirectory,
                        const char *app_id, char *path, size_t path_max) {
    const NSUInteger NSUserDomainMask = 1;
    int result = -1;

#if TARGET_OS_OSX
    CFBundleRef bundle = NULL;
    int bundle_id_appended = 0;
#endif

    CFStringRef dir = NULL;
    Boolean success = NO;
    unsigned long length = 0;

    FC_AUTORELEASEPOOL_BEGIN
    CFArrayRef array =
#if __has_feature(objc_arc)
    FC_REINTERPRET_CAST(__bridge CFArrayRef)
#else
    FC_REINTERPRET_CAST(CFArrayRef)
#endif
    (NSSearchPathForDirectoriesInDomains(searchPathDirectory, NSUserDomainMask, TRUE));
    if (!array || CFArrayGetCount(array) == 0) {
        goto fc_datadir_fail;
    }
    dir = FC_REINTERPRET_CAST(CFStringRef)(CFArrayGetValueAtIndex(array, 0));
    success = CFStringGetFileSystemRepresentation(dir, path, FC_STATIC_CAST(CFIndex)(path_max) - 1);
    if (!success) {
        goto fc_datadir_fail;
    }
    length = strlen(path);
    if (length == 0 || length + 1 >= path_max) {
        goto fc_datadir_fail;
    }
    // Add trailing slash
    if (path[length - 1] != FC_DIRECTORY_SEPARATOR) {
        path[length] = FC_DIRECTORY_SEPARATOR;
        path[length + 1] = 0;
        length++;
    }
    if (mkdir(path, 0700) != 0 && errno != EEXIST) {
        goto fc_datadir_fail;
    }
    result = 0;

#if TARGET_OS_OSX
    bundle = CFBundleGetMainBundle();
    if (bundle) {
        CFStringRef bundle_id = CFBundleGetIdentifier(bundle);
        if (bundle_id) {
            if (CFStringFind(dir, bundle_id, 0).length != 0) {
                // macOS sandboxed app
                bundle_id_appended = 1;
            } else {
                // Append bundle_id (macOS bundled, non-sandboxed app)
                CFIndex bundle_id_length = CFStringGetLength(bundle_id);
                bundle_id_length = CFStringGetMaximumSizeForEncoding(bundle_id_length,
                                                                     kCFStringEncodingUTF8);
                if (bundle_id_length > 0 &&
                    length + FC_STATIC_CAST(unsigned long)(bundle_id_length) + 1 < path_max - 1 &&
                    CFStringGetCString(bundle_id, path + length, bundle_id_length,
                                       kCFStringEncodingUTF8)) {
                    path[length + FC_STATIC_CAST(unsigned long)(bundle_id_length)] = FC_DIRECTORY_SEPARATOR;
                    path[length + FC_STATIC_CAST(unsigned long)(bundle_id_length) + 1] = 0;
                    if (mkdir(path, 0700) != 0 && errno != EEXIST) {
                        result = -1;
                        goto fc_datadir_fail;
                    }
                    bundle_id_appended = 1;
                }
            }
        }
    }
    if (!bundle_id_appended) {
        // Append app_id (macOS executable)
        if (!app_id || !*app_id) {
            result = -1;
        } else {
            size_t app_id_length = strlen(app_id);
            if (length + app_id_length + 1 < path_max - 1) {
                strcpy(path + length, app_id);
                path[length + app_id_length] = FC_DIRECTORY_SEPARATOR;
                path[length + app_id_length + 1] = 0;
                if (mkdir(path, 0700) != 0 && errno != EEXIST) {
                    result = -1;
                    goto fc_datadir_fail;
                }
            } else {
                result = -1;
            }
        }
    }
#else
    (void)app_id;
#endif

fc_datadir_fail:
    if (result != 0) {
        path[0] = 0;
    }
    FC_AUTORELEASEPOOL_END
    return result;
}

static int fc_resdir(char *path, size_t path_max) {
    if (!path || path_max == 0) {
        return -1;
    }
    int result = -1;
    FC_AUTORELEASEPOOL_BEGIN
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle) {
        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundle);
        if (resourcesURL) {
            Boolean success = CFURLGetFileSystemRepresentation(resourcesURL, TRUE,
                                                               FC_REINTERPRET_CAST(UInt8 *)(path),
                                                               FC_STATIC_CAST(CFIndex)(path_max) - 1);
            CFRelease(resourcesURL);
            if (success) {
                unsigned long length = strlen(path);
                if (length > 0 && length < path_max - 1) {
                    // Add trailing slash
                    if (path[length - 1] != FC_DIRECTORY_SEPARATOR) {
                        path[length] = FC_DIRECTORY_SEPARATOR;
                        path[length + 1] = 0;
                    }
                    result = 0;
                }
            }
        }
    }
    FC_AUTORELEASEPOOL_END
    if (result != 0) {
        path[0] = 0;
    }
    return result;
}

static int fc_datadir(const char *app_id, char *path, size_t path_max) {
    const NSUInteger NSApplicationSupportDirectory = 14;
    return fc__appledir(NSApplicationSupportDirectory, app_id, path, path_max);
}

static int fc_cachedir(const char *app_id, char *path, size_t path_max) {
    const NSUInteger NSCachesDirectory = 13;
    return fc__appledir(NSCachesDirectory, app_id, path, path_max);
}

static int fc_locale(char *locale, size_t locale_max) {
    if (!locale || locale_max < 3) {
        return -1;
    }
    int result = -1;
    FC_AUTORELEASEPOOL_BEGIN
    CFArrayRef languages = CFLocaleCopyPreferredLanguages();
    if (languages) {
        if (CFArrayGetCount(languages) > 0) {
            CFStringRef language = FC_REINTERPRET_CAST(CFStringRef)(CFArrayGetValueAtIndex(languages, 0));
            if (language) {
                CFIndex length = CFStringGetLength(language);
                if (length > FC_STATIC_CAST(CFIndex)(locale_max) - 1) {
                    length = FC_STATIC_CAST(CFIndex)(locale_max) - 1;
                }
                CFIndex outLength = CFStringGetBytes(language, CFRangeMake(0, length),
                                                     kCFStringEncodingUTF8, 0, FALSE,
                                                     FC_REINTERPRET_CAST(UInt8 *)(locale),
                                                     FC_STATIC_CAST(CFIndex)(locale_max) - 1, NULL);
                locale[outLength] = 0;
                result = 0;
            }
        }
        CFRelease(languages);
    }
    FC_AUTORELEASEPOOL_END
    if (result == 0) {
        fc__locale_clean(locale);
    } else {
        locale[0] = 0;
    }
    return result;
}

#endif // defined(__APPLE__)

// MARK: - Linux

#if defined(__linux__) && !defined(__ANDROID__)

#include <locale.h>
#include <string.h>
#include <unistd.h> // readlink
#include <sys/stat.h> // mkdir

static int fc_resdir(char *path, size_t path_max) {
    if (!path || path_max == 0) {
        return -1;
    }
    ssize_t length = readlink("/proc/self/exe", path, path_max - 1);
    if (length > 0 && FC_STATIC_CAST(size_t)(length) < path_max) {
        for (ssize_t i = length - 1; i > 0; i--) {
            if (path[i] == FC_DIRECTORY_SEPARATOR) {
                path[i + 1] = 0;
                return 0;
            }
        }
    }
    path[0] = 0;
    return -1;
}

static int fc_datadir(const char *app_id, char *path, size_t path_max) {
    return fc__unixdir("XDG_DATA_HOME", ".local/share", app_id, path, path_max);
}

static int fc_cachedir(const char *app_id, char *path, size_t path_max) {
    return fc__unixdir("XDG_CACHE_HOME", ".cache", app_id, path, path_max);
}

static int fc_locale(char *locale, size_t locale_max) {
    if (!locale || locale_max < 3) {
        return -1;
    }
    int result = -1;
    setlocale(LC_ALL, "");
    char *lang = setlocale(LC_ALL, NULL);
    if (lang && lang[0] != 0 && !(lang[0] == 'C' && lang[1] == 0)) {
        result = 0;
        strncpy(locale, lang, locale_max);
        locale[locale_max - 1] = 0;
    }
    if (result == 0) {
        fc__locale_clean(locale);
    } else {
        locale[0] = 0;
    }
    return result;
}

#endif // defined(__unix__)

// MARK: - Windows

#if defined(_WIN32)

#if !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <Shlobj.h>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#include <stdlib.h> // wcstombs_s
#if !defined(PATH_MAX)
#  define PATH_MAX MAX_PATH
#endif

/// *Windows only:* Gets a path using `SHGetKnownFolderPath`, and appends `app_id` to it.
static int fc__win32dir(REFKNOWNFOLDERID folder_id,
                        const char *app_id, char *path, size_t path_max) {
    wchar_t *wpath = NULL;
    size_t count = 0; // Output count including NULL
    size_t app_id_length = strlen(app_id);
    int success = (SUCCEEDED(SHGetKnownFolderPath(folder_id, 0, NULL, &wpath)) &&
                   wcstombs_s(&count, path, path_max, wpath, path_max - 1) == 0 &&
                   count > 1 && count + app_id_length + 2 <= path_max);
    CoTaskMemFree(wpath);
    if (!success) {
        path[0] = 0;
        return -1;
    }
    if (path[count - 2] != FC_DIRECTORY_SEPARATOR) {
        path[count - 1] = FC_DIRECTORY_SEPARATOR;
        path[count] = 0;
        count++;
    }
    strcpy_s(path + count - 1, path_max - count, app_id);
    count += app_id_length;
    if (path[count - 2] != FC_DIRECTORY_SEPARATOR) {
        path[count - 1] = FC_DIRECTORY_SEPARATOR;
        path[count] = 0;
    }
    int result = SHCreateDirectoryExA(NULL, path, NULL);
    if (result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS) {
        return 0;
    } else {
        path[0] = 0;
        return -1;
    }
}

static int fc_resdir(char *path, size_t path_max) {
    if (!path || path_max == 0) {
        return -1;
    }
    size_t length = FC_STATIC_CAST(size_t)(GetModuleFileNameA(NULL, path, FC_STATIC_CAST(DWORD)(path_max)));
    if (length > 0 && length < path_max) {
        for (size_t i = length - 1; i > 0; i--) {
            if (path[i] == FC_DIRECTORY_SEPARATOR) {
                path[i + 1] = 0;
                return 0;
            }
        }
    }
    path[0] = 0;
    return -1;
}

static int fc_datadir(const char *app_id, char *path, size_t path_max) {
#ifdef __cplusplus
    return fc__win32dir(FOLDERID_RoamingAppData, app_id, path, path_max);
#else
    return fc__win32dir(&FOLDERID_RoamingAppData, app_id, path, path_max);
#endif
}

static int fc_cachedir(const char *app_id, char *path, size_t path_max) {
#ifdef __cplusplus
    return fc__win32dir(FOLDERID_LocalAppData, app_id, path, path_max);
#else
    return fc__win32dir(&FOLDERID_LocalAppData, app_id, path, path_max);
#endif
}

static int fc_locale(char *locale, size_t locale_max) {
    if (!locale || locale_max < 3) {
        return -1;
    }
    int result = -1;
    wchar_t wlocale[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(wlocale, LOCALE_NAME_MAX_LENGTH) > 0) {
        size_t count = 0;
        if (wcstombs_s(&count, locale, locale_max, wlocale, locale_max - 1) == 0) {
            result = 0;
        }
    }
    if (result == 0) {
        fc__locale_clean(locale);
    } else {
        locale[0] = 0;
    }
    return result;
}

static inline FILE *fc__windows_fopen(const char *filename, const char *mode) {
    FILE *file = NULL;
    fopen_s(&file, filename, mode);
    return file;
}

static inline int fc__windows_fclose(FILE *stream) {
    // The Windows fclose() function will crash if stream is NULL
    if (stream) {
        return fclose(stream);
    } else {
        return 0;
    }
}

#define fopen(filename, mode) fc__windows_fopen(filename, mode)
#define fclose(file) fc__windows_fclose(file)

#if defined(_DEBUG)

// Outputs to debug window if there is no console and IsDebuggerPresent() returns true.
static int fc__printf(const char *format, ...) {
    int result;
    if (IsDebuggerPresent() && GetStdHandle(STD_OUTPUT_HANDLE) == NULL) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        result = vsprintf_s(buffer, sizeof(buffer), format, args);
        va_end(args);
        if (result >= 0) {
            OutputDebugStringA(buffer);
        }
    } else {
        va_list args;
        va_start(args, format);
        result = vprintf(format, args);
        va_end(args);
    }
    return result;
}

#define printf(format, ...) fc__printf(format, __VA_ARGS__)

#endif // _DEBUG

#endif // _WIN32

// MARK: - Android

#if defined(__ANDROID__)

#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <jni.h>
#include <pthread.h>
#include <string.h>

static JNIEnv *fc__jnienv(JavaVM *vm);

/// *Android Only:* Gets a path from a `Context` method like `getFilesDir` or `getCacheDir`.
static int fc__android_dir(ANativeActivity *activity, const char *method_name,
                           const char *app_id, char *path, size_t path_max) {
    (void)app_id;
    if (!activity) {
        path[0] = 0;
        return -1;
    }
    int result = -1;

#ifdef __cplusplus
    JNIEnv *jniEnv = fc__jnienv(activity->vm);
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionClear();
    }

    if (jniEnv->PushLocalFrame(16) == JNI_OK) {
        jclass activityClass = jniEnv->GetObjectClass(activity->clazz);
        jmethodID getDirMethod = jniEnv->GetMethodID(activityClass, method_name, "()Ljava/io/File;");
        jobject file = jniEnv->CallObjectMethod(activity->clazz, getDirMethod);
        jclass fileClass = jniEnv->FindClass("java/io/File");
        jmethodID getAbsolutePathMethod = jniEnv->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
        jstring valueString = reinterpret_cast<jstring>(jniEnv->CallObjectMethod(file, getAbsolutePathMethod));

        const char *nativeString = jniEnv->GetStringUTFChars(valueString, 0);
        if (nativeString) {
            result = 0;
            snprintf(path, path_max, "%s/", nativeString);
            jniEnv->ReleaseStringUTFChars(valueString, nativeString);
        }
        if (jniEnv->ExceptionCheck()) {
            jniEnv->ExceptionClear();
        }
        jniEnv->PopLocalFrame(NULL);
    }
#else
    JNIEnv *jniEnv = fc__jnienv(activity->vm);
    if ((*jniEnv)->ExceptionCheck(jniEnv)) {
        (*jniEnv)->ExceptionClear(jniEnv);
    }
    if ((*jniEnv)->PushLocalFrame(jniEnv, 16) == JNI_OK) {
        jclass activityClass = (*jniEnv)->GetObjectClass(jniEnv, activity->clazz);
        jmethodID getDirMethod = (*jniEnv)->GetMethodID(jniEnv, activityClass, method_name, "()Ljava/io/File;");
        jobject file = (*jniEnv)->CallObjectMethod(jniEnv, activity->clazz, getDirMethod);
        jclass fileClass = (*jniEnv)->FindClass(jniEnv, "java/io/File");
        jmethodID getAbsolutePathMethod = (*jniEnv)->GetMethodID(jniEnv, fileClass, "getAbsolutePath", "()Ljava/lang/String;");
        jstring valueString = (jstring)(*jniEnv)->CallObjectMethod(jniEnv, file, getAbsolutePathMethod);

        const char *nativeString = (*jniEnv)->GetStringUTFChars(jniEnv, valueString, 0);
        if (nativeString) {
            result = 0;
            snprintf(path, path_max, "%s/", nativeString);
            (*jniEnv)->ReleaseStringUTFChars(jniEnv, valueString, nativeString);
        }
        if ((*jniEnv)->ExceptionCheck(jniEnv)) {
            (*jniEnv)->ExceptionClear(jniEnv);
        }
        (*jniEnv)->PopLocalFrame(jniEnv, NULL);
    }
#endif
    if (result != 0) {
        path[0] = 0;
    }
    return result;
}

static int fc__android_locale(ANativeActivity *activity,
                              char *locale, size_t locale_max) {
    if (!locale || locale_max < 3 || !activity) {
        return -1;
    }
    int result = -1;
    // getResources().getConfiguration().locale.toString()
#ifdef __cplusplus
    JNIEnv *jniEnv = fc__jnienv(activity->vm);
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionClear();
    }

    if (jniEnv->PushLocalFrame(16) == JNI_OK) {
        jclass activityClass = jniEnv->GetObjectClass(activity->clazz);
        jmethodID getResourcesMethod = jniEnv->GetMethodID(activityClass,
                "getResources", "()Landroid/content/res/Resources;");
        jobject resources = jniEnv->CallObjectMethod(activity->clazz, getResourcesMethod);
        jclass resourcesClass = jniEnv->GetObjectClass(resources);
        jmethodID getConfigurationMethod = jniEnv->GetMethodID(resourcesClass,
                "getConfiguration", "()Landroid/content/res/Configuration;");
        jobject configuration = jniEnv->CallObjectMethod(resources, getConfigurationMethod);
        jclass configurationClass = jniEnv->GetObjectClass(configuration);
        jfieldID localeField = jniEnv->GetFieldID(configurationClass, "locale", "Ljava/util/Locale;");
        jobject localeObject = jniEnv->GetObjectField(configuration, localeField);
        jclass localeClass = jniEnv->GetObjectClass(localeObject);
        jmethodID toStringMethod = jniEnv->GetMethodID(localeClass, "toString", "()Ljava/lang/String;");
        jstring valueString = reinterpret_cast<jstring>(jniEnv->CallObjectMethod(localeObject, toStringMethod));

        const char *nativeString = jniEnv->GetStringUTFChars(valueString, 0);
        if (nativeString) {
            result = 0;
            strncpy(locale, nativeString, locale_max);
            locale[locale_max - 1] = 0;
            jniEnv->ReleaseStringUTFChars(valueString, nativeString);
        }
        if (jniEnv->ExceptionCheck()) {
            jniEnv->ExceptionClear();
        }
        jniEnv->PopLocalFrame(NULL);
    }
#else
    JNIEnv *jniEnv = fc__jnienv(activity->vm);
    if ((*jniEnv)->ExceptionCheck(jniEnv)) {
        (*jniEnv)->ExceptionClear(jniEnv);
    }

    if ((*jniEnv)->PushLocalFrame(jniEnv, 16) == JNI_OK) {
        jclass activityClass = (*jniEnv)->GetObjectClass(jniEnv, activity->clazz);
        jmethodID getResourcesMethod = (*jniEnv)->GetMethodID(jniEnv, activityClass,
            "getResources", "()Landroid/content/res/Resources;");
        jobject resources = (*jniEnv)->CallObjectMethod(jniEnv, activity->clazz,
            getResourcesMethod);
        jclass resourcesClass = (*jniEnv)->GetObjectClass(jniEnv, resources);
        jmethodID getConfigurationMethod = (*jniEnv)->GetMethodID(jniEnv, resourcesClass,
            "getConfiguration", "()Landroid/content/res/Configuration;");
        jobject configuration = (*jniEnv)->CallObjectMethod(jniEnv, resources,
            getConfigurationMethod);
        jclass configurationClass = (*jniEnv)->GetObjectClass(jniEnv, configuration);
        jfieldID localeField = (*jniEnv)->GetFieldID(jniEnv, configurationClass, "locale",
            "Ljava/util/Locale;");
        jobject localeObject = (*jniEnv)->GetObjectField(jniEnv, configuration, localeField);
        jclass localeClass = (*jniEnv)->GetObjectClass(jniEnv, localeObject);
        jmethodID toStringMethod = (*jniEnv)->GetMethodID(jniEnv, localeClass, "toString",
            "()Ljava/lang/String;");
        jstring valueString = (*jniEnv)->CallObjectMethod(jniEnv, localeObject, toStringMethod);

        const char *nativeString = (*jniEnv)->GetStringUTFChars(jniEnv, valueString, 0);
        if (nativeString) {
            result = 0;
            strncpy(locale, nativeString, locale_max);
            locale[locale_max - 1] = 0;
            (*jniEnv)->ReleaseStringUTFChars(jniEnv, valueString, nativeString);
        }
        if ((*jniEnv)->ExceptionCheck(jniEnv)) {
            (*jniEnv)->ExceptionClear(jniEnv);
        }
        (*jniEnv)->PopLocalFrame(jniEnv, NULL);
    }
#endif // !defined(__cplusplus)
    if (result == 0) {
        fc__locale_clean(locale);
    } else {
        locale[0] = 0;
    }
    return result;
}

static int fc_resdir(char *path, size_t path_max) {
    if (!path || path_max == 0) {
        return -1;
    }
    path[0] = 0;
    return 0;
}

/// For Android, define `FILE_COMPAT_ANDROID_ACTIVITY` to be a reference to an `ANativeActivity`
/// instance or to a function that returns an `ANativeActivity` instance.
/// For example:
///
///     ANativeActivity *functionThatGetsAndroidActivity(void);
///     #define FILE_COMPAT_ANDROID_ACTIVITY functionThatGetsAndroidActivity()
#define fc__android_activity() FILE_COMPAT_ANDROID_ACTIVITY

#define fc_datadir(app_id, path, path_max) \
    fc__android_dir(fc__android_activity(), "getFilesDir", (app_id), (path), (path_max))

#define fc_cachedir(app_id, path, path_max) \
    fc__android_dir(fc__android_activity(), "getCacheDir", (app_id), (path), (path_max))

#define fc_locale(locale, locale_max) \
    fc__android_locale(fc__android_activity(), (locale), (locale_max))

#if !defined(_BSD_SOURCE)
FILE* funopen(const void* __cookie,
              int (*__read_fn)(void*, char*, int),
              int (*__write_fn)(void*, const char*, int),
              fpos_t (*__seek_fn)(void*, fpos_t, int),
              int (*__close_fn)(void*));
#endif // _BSD_SOURCE

static pthread_key_t fc__jnienv_key;
static pthread_once_t fc__jnienv_key_once = PTHREAD_ONCE_INIT;

static void fc__jnienv_detach(void *value) {
    if (value) {
        JavaVM *vm = FC_REINTERPRET_CAST(JavaVM *)(value);
#ifdef __cplusplus
        vm->DetachCurrentThread();
#else
        (*vm)->DetachCurrentThread(vm);
#endif
    }
}

static void fc__create_jnienv_key() {
    pthread_key_create(&fc__jnienv_key, fc__jnienv_detach);
}

static JNIEnv *fc__jnienv(JavaVM *vm) {
    JNIEnv *jniEnv = NULL;
    int setThreadLocal;
#ifdef __cplusplus
    setThreadLocal = (vm->GetEnv(FC_REINTERPRET_CAST(void **)(&jniEnv), JNI_VERSION_1_4) != JNI_OK &&
            vm->AttachCurrentThread(&jniEnv, NULL) == JNI_OK);
#else
    setThreadLocal = ((*vm)->GetEnv(vm, (void **)&jniEnv, JNI_VERSION_1_4) != JNI_OK &&
            (*vm)->AttachCurrentThread(vm, &jniEnv, NULL) == JNI_OK);
#endif
    if (setThreadLocal) {
        pthread_once(&fc__jnienv_key_once, fc__create_jnienv_key);
        pthread_setspecific(fc__jnienv_key, vm);
    }
    return jniEnv;
}

static int fc__android_read(void *cookie, char *buf, int size) {
    return AAsset_read(FC_REINTERPRET_CAST(AAsset *)(cookie), buf, FC_STATIC_CAST(size_t)(size));
}

static int fc__android_write(void *cookie, const char *buf, int size) {
    (void)cookie;
    (void)buf;
    (void)size;
    errno = EACCES;
    return -1;
}

static fpos_t fc__android_seek(void *cookie, fpos_t offset, int whence) {
    return AAsset_seek(FC_REINTERPRET_CAST(AAsset *)(cookie), offset, whence);
}

static int fc__android_close(void *cookie) {
    AAsset_close(FC_REINTERPRET_CAST(AAsset *)(cookie));
    return 0;
}

static FILE *fc__android_fopen(ANativeActivity *activity, const char *filename, const char *mode) {
    AAssetManager *assetManager = NULL;
    AAsset *asset = NULL;
    if (activity) {
        assetManager = activity->assetManager;
    }
    if (assetManager && mode && mode[0] == 'r') {
        asset = AAssetManager_open(assetManager, filename, AASSET_MODE_UNKNOWN);
    }
    if (asset) {
        return funopen(asset, fc__android_read, fc__android_write, fc__android_seek,
                       fc__android_close);
    } else {
        return fopen(filename, mode);
    }
}

#define printf(...) __android_log_print(ANDROID_LOG_INFO, "stdout", __VA_ARGS__)
#define fopen(filename, mode) fc__android_fopen(fc__android_activity(), (filename), (mode))

#endif // defined(__ANDROID__)

// MARK: - Emscripten

#if defined(__EMSCRIPTEN__)

#include <emscripten/emscripten.h>
#include <string.h>
#include <stdlib.h> // getenv
#if !defined(PATH_MAX)
#  define PATH_MAX 4096
#endif

static int fc_resdir(char *path, size_t path_max) {
    if (!path || path_max == 0) {
        return -1;
    }
    path[0] = 0;
    return 0;
}

static int fc_datadir(const char *app_id, char *path, size_t path_max) {
    return fc__unixdir("XDG_DATA_HOME", ".local/share", app_id, path, path_max);
}

static int fc_cachedir(const char *app_id, char *path, size_t path_max) {
    return fc__unixdir("XDG_CACHE_HOME", ".cache", app_id, path, path_max);
}

static int fc_locale(char *locale, size_t locale_max) {
    if (!locale || locale_max < 3) {
        return -1;
    }
    int result = -1;
    static const char *script =
        "(function() { try {"
        "var lang = navigator.language || navigator.userLanguage || navigator.browserLanguage;"
        "if (typeof lang === 'string') { return lang; } else { return ''; }"
        "} catch(err) { return ''; } }())";

    char *lang = emscripten_run_script_string(script);
    if (lang && lang[0] != 0) {
        result = 0;
        strncpy(locale, lang, locale_max);
        locale[locale_max - 1] = 0;
    }
    if (result == 0) {
        fc__locale_clean(locale);
    } else {
        locale[0] = 0;
    }
    return result;
}

#endif // defined(__EMSCRIPTEN__)

#endif // FILE_COMPAT_H
