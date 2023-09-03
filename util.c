// Typedefs

typedef Uint8  u8;
typedef Uint16 u16;
typedef Uint32 u32;
typedef Uint64 u64;

typedef float  f32;
typedef double f64;

typedef u8 bool;
#define true  1
#define false 0

#define nullptr 0

// Miscellaneous

#define assert(x) if(!(x)){__debugbreak();}

// Replacements for stdlib functions

#pragma function(memset)
void *memset(void *dest, int ch, u64 count) {
    void *result = dest;
    u8 *buf = dest;
    
    while (count) {
        *buf++ = (u8)ch;
        --count;
    }
    
    return result;
}

#pragma function(strlen)
u64 strlen(const char *string) {
    u64 result = 0;
    while (string[result])
        result++;
    return result;
}

// Allocator

HANDLE process_heap = 0;
void heap_init() { process_heap = GetProcessHeap(); }

#define allocate(x)            HeapAlloc(process_heap, HEAP_ZERO_MEMORY, x)
#define reallocate(x, newsize) HeapReAlloc(process_heap, HEAP_ZERO_MEMORY, x, newsize)
#define deallocate(x)          HeapFree(process_heap, 0, x)

// Strings, not null-terminated.

#define cs(str) (String){str, sizeof(str)-1, sizeof(str)-1}

typedef struct String
{
    char *buffer;
    u64 length, capacity;
} String;

#define SHORT_STRING_CAPACITY 512
typedef struct Short_String
{
    char buffer[SHORT_STRING_CAPACITY];
    u64 length;
} Short_String;


String make_string(u64 capacity) {
    String string = {0};
    
    string.buffer = (char*)allocate(capacity);
    string.capacity = capacity;
    
    return string;
}

void string_reallocate(String *str) {
    str->capacity *= 2;
    str->buffer = reallocate(str->buffer, str->capacity);
}

void string_clear(String *str) {
    memset(str->buffer, 0, str->capacity);
    str->length = 0;
}

String as_string(char *str) {
    u64 len = strlen(str);
    return (String){str, len, len}; // Use len as the capacity as we don't store the '\0'
}

bool string_compare(String a, String b) {
    if (a.length != b.length) return false;
    
    for (u64 i = 0; i < a.length; i++)
        if (a.buffer[i] != b.buffer[i]) return false;
    
    return true;
}

void string_copy(String *dst, String src) {
    if (!dst->buffer) return;
    if (!src.buffer) return;
    
    while (dst->capacity < src.length) string_reallocate(dst);
    
    for (u64 i = 0; i < src.length; i++) {
        dst->buffer[i] = src.buffer[i];
    }
    
    dst->length = src.length;
}

String string_slice_duplicate(String src, u64 start, u64 end) {
    assert(end < src.length);
    
    if (src.length == 0) return (String){0};
    
    String result = { 0, end-start+1, end-start+1 };
    result.buffer = (char*)allocate(result.capacity);
    
    for (u64 i = start; i <= end; i++) {
        result.buffer[i-start] = src.buffer[i];
    }
    
    return result;
}

void string_append(String *dst, String src) {
    while (dst->length + src.length > dst->capacity) {
        string_reallocate(dst);
    }
    
    for (u64 i = 0; i < src.length; i++) {
        dst->buffer[dst->length+i] = src.buffer[i];
    }
    
    dst->length += src.length;
}

void string_append_char(String *dst, char ch) {
    while (dst->length + 1 > dst->capacity) string_reallocate(dst);
    dst->buffer[dst->length++] = ch;
}

void string_insert_char(String *dst, int pos, char ch) {
    while (dst->length + 1 > dst->capacity) string_reallocate(dst);
    for (int i = (int)dst->length-1; i >= pos; i--) {
        if (i < 0) i = 0;
        dst->buffer[i+1] = dst->buffer[i];
    }
    dst->buffer[pos] = ch;
    dst->length++;
}

int string_insert(String *dst, int pos, String src) {
    for (int i = 0; i < src.length; i++) {
        string_insert_char(dst, pos++, src.buffer[i]);
    }
    return pos;
}

bool string_delete_char(String *str, u64 pos) {
    if (str->length == 0) return false;
    
    str->length--;
    for (u64 i = pos; i < str->length; i++) {
        str->buffer[i] = str->buffer[i+1];
    }
    
    return true;
}

void string_delete_range(String *str, u64 start, u64 end) {
    assert(str);
    if (!str->buffer) return;
    if (end >= str->length) return;
    
    if (start > end) {
        u64 temp = start;
        start = end;
        end = temp;
    }
    
    for (int i = 0; i <= end-start; i++) {
        if (!string_delete_char(str, start)) return;
    }
}

// Standard Output

HANDLE stdout;

void stdout_init() {
    stdout = GetStdHandle(STD_OUTPUT_HANDLE);
}

void print(String string) {
    WriteFile(stdout, string.buffer, (DWORD)string.length, 0, 0);
}

// File Stuff

typedef struct File {
    HANDLE handle;
    bool invalid;
} File;

typedef enum {
    FILE_READ,
    FILE_WRITE
} File_Mode;

File open_file(String filename, File_Mode mode) {
    File file = {0};
    
    u32 desired_access = (mode == FILE_READ) ? GENERIC_READ : GENERIC_WRITE;
    u32 creation = (mode == FILE_READ) ? OPEN_EXISTING : OPEN_ALWAYS;
    
    file.handle = CreateFile(filename.buffer,
                             desired_access,
                             0,
                             nullptr,
                             creation,
                             FILE_ATTRIBUTE_NORMAL,
                             nullptr);
    
    if (file.handle == INVALID_HANDLE_VALUE) {
        file.invalid = true;
    }
    
    return file;
}

void close_file(File file) {
    CloseHandle(file.handle);
}

bool file_exists(String filename) {
    DWORD attributes = GetFileAttributes(filename.buffer);
    
    const DWORD invalid = INVALID_FILE_ATTRIBUTES;
    
    bool exists = (attributes != invalid &&
                   !(attributes & FILE_ATTRIBUTE_DIRECTORY));
    return exists;
}

String read_file(File file) {
    u32 file_size = GetFileSize(file.handle, nullptr);
    String result = make_string(file_size);
    
    DWORD bytes_read = 0;
    bool ok = (bool)ReadFile(file.handle, result.buffer, file_size, &bytes_read, nullptr);
    assert(ok);
    
    result.length = bytes_read;
    
    return result;
}

bool write_to_file(File file, String buffer) {
    assert(file.handle != INVALID_HANDLE_VALUE);
    bool ok = (bool)WriteFile(file.handle, buffer.buffer, (u32)buffer.length, nullptr, nullptr);
    
    if (!ok) {
        u32 error = GetLastError();
        (void)error;
        __debugbreak();
    }
    SetEndOfFile(file.handle);
    
    return ok;
}

// Windows things...

typedef enum PROCESS_DPI_AWARENESS
{
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef BOOL (WINAPI * SETPROCESSDPIAWARE_T)(void);
typedef HRESULT (WINAPI * SETPROCESSDPIAWARENESS_T)(PROCESS_DPI_AWARENESS);

inline bool win32_SetProcessDpiAware(void) {
    HMODULE shcore = LoadLibraryA("Shcore.dll");
    SETPROCESSDPIAWARENESS_T SetProcessDpiAwareness = nullptr;
    if (shcore) {
        SetProcessDpiAwareness = (SETPROCESSDPIAWARENESS_T) GetProcAddress(shcore, "SetProcessDpiAwareness");
    }
    HMODULE user32 = LoadLibraryA("User32.dll");
    SETPROCESSDPIAWARE_T SetProcessDPIAware = nullptr;
    if (user32) {
        SetProcessDPIAware = (SETPROCESSDPIAWARE_T) GetProcAddress(user32, "SetProcessDPIAware");
    }
    
    bool ret = false;
    if (SetProcessDpiAwareness) {
        ret = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) == S_OK;
    } else if (SetProcessDPIAware) {
        ret = SetProcessDPIAware() != 0;
    }
    
    if (user32) {
        FreeLibrary(user32);
    }
    if (shcore) {
        FreeLibrary(shcore);
    }
    return ret;
}

void get_command_line_args(int *argc, char ***argv) {
    if (!argc || !argv) return;
    
    // Get the command line arguments as wchar_t strings
    wchar_t ** wargv = CommandLineToArgvW( GetCommandLineW(), argc );
    if (!wargv) { *argc = 0; *argv = nullptr; return; }
    
    // Count the number of bytes necessary to store the UTF-8 versions of those strings
    int n = 0;
    for (int i = 0;  i < *argc;  i++)
        n += WideCharToMultiByte( CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr ) + 1;
    
    // Allocate the argv[] array + all the UTF-8 strings
    *argv = allocate( (*argc + 1) * sizeof(char *) + n );
    if (!*argv) { *argc = 0; return; }
    
    // Convert all wargv[] --> argv[]
    char * arg = (char *)&((*argv)[*argc + 1]);
    for (int i = 0;  i < *argc;  i++) {
        (*argv)[i] = arg;
        arg += WideCharToMultiByte( CP_UTF8, 0, wargv[i], -1, arg, n, nullptr, nullptr ) + 1;
    }
    (*argv)[*argc] = nullptr;
}