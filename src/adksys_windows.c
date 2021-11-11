#include "adksys.h"
#include <direct.h>
#include <assert.h>
#include <psapi.h>
#include <winsock.h>

#define SECURITY_WIN32
#include <security.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

int adksys_get_libraries(char*** libraries, int* libraries_size, int* libnames_need_free)
{
   BOOL result;
   HMODULE *module_handles = NULL;
   HANDLE proc;
   int fresult = -1, libs_size = 0, nresult;
   DWORD bytes_needed = 0;
   char** lib_names = NULL, lib_name_buffer[4096];
   
   proc = GetCurrentProcess();
   if (!proc)
      goto done;

   result = EnumProcessModules(proc, NULL, 0, &bytes_needed);
   if (!result)
      goto done;

   module_handles = (HMODULE *) malloc(bytes_needed);
   if (!module_handles)
      goto done;

   result = EnumProcessModules(proc, module_handles, bytes_needed, &bytes_needed);
   if (!result)
      goto done;

   libs_size = bytes_needed / sizeof(HMODULE);
   lib_names = (char**) malloc(libs_size * sizeof(char*));
   if (!lib_names)
      goto done;

   memset(lib_names, 0, libs_size * sizeof(char*));
   
   for (int i = 0; i < libs_size; i++) {
      nresult = GetModuleFileName(module_handles[i], lib_name_buffer, sizeof(lib_name_buffer));
      if (!nresult)
         goto done;
      lib_names[i] = strdup(lib_name_buffer);
   }
   
   *libraries = lib_names;
   *libraries_size = libs_size;
   *libnames_need_free = 1;

   fresult = 0;
done:
   if (module_handles)
      free(module_handles);
 
   if (fresult == -1 && lib_names) {
      for (int i = 0; i < libs_size; i++) {
         if (lib_names[i])
            free(lib_names[i]);
      }
      free(lib_names);
   }
  
   return fresult;
  
}

#define EPOCH_DIFFERENCE 11644473600UL
static void filetime_to_timeval(FILETIME ft, struct timeval* tv, BOOL calendar_time)
{
   unsigned long long tmp;

   tmp = (((unsigned long long) ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
   tv->tv_sec = (unsigned long) (tmp / 10000000);
   tv->tv_usec = (unsigned long) (tmp % 10000000) / 10;
   if (calendar_time)
      tv->tv_sec = (unsigned long) (tv->tv_sec - EPOCH_DIFFERENCE);
}

int adksys_get_times(struct timeval* sys, struct timeval* cpu)
{
   HANDLE proc;
   FILETIME creation_time, exit_time, user_time, sys_time;
   BOOL result;

   proc = GetCurrentProcess();

   result = GetProcessTimes(proc, &creation_time, &exit_time, &sys_time, &user_time);
   if (!result)
      return -1;

   if (sys)
      filetime_to_timeval(sys_time, sys, FALSE);
   if (cpu)
      filetime_to_timeval(user_time, cpu, FALSE);

   return 0;
}

int adksys_curtime(struct timeval* tm)
{
   FILETIME curtime;
   GetSystemTimeAsFileTime(&curtime);
   filetime_to_timeval(curtime, tm, TRUE);
   return 0;
}

int adksys_hostname(char* outbuffer, int buffer_size)
{
   static char* hostname = NULL;
   static int err_ret = 0;
   WSADATA unused;
   int result;

   if (hostname || err_ret) {
      if (err_ret)
         return err_ret;
      strncpy(outbuffer, hostname, buffer_size);
      outbuffer[buffer_size - 1] = '\0';
      return 0;
   }

   result = WSAStartup(MAKEWORD(1, 0), &unused);
   if (result != 0) {
      err_ret = -1;
      return -1;
   }
   result = gethostname(outbuffer, buffer_size);
   if (result != 0) {
      err_ret = -1;
      return -1;
   }
   outbuffer[buffer_size - 1] = '\0';

   hostname = strdup(outbuffer);
   err_ret = 0;
   
   WSACleanup();

   return 0;
}

int adksys_starttime(struct timeval* tv)
{
   HANDLE proc;
   FILETIME creation_time, exit_time, user_time, sys_time;
   BOOL result;

   proc = GetCurrentProcess();

   result = GetProcessTimes(proc, &creation_time, &exit_time, &sys_time, &user_time);
   if (!result)
      return -1;

   filetime_to_timeval(creation_time, tv, TRUE);
   return 0;
}

int adksys_get_executable(char* outpath, size_t outpath_size)
{
   HANDLE proc;
   int result;
   
   proc = GetCurrentProcess();
   result = GetProcessImageFileName(proc, outpath, (int) outpath_size);
   if (!result)
      return -1;
   return 0;
}

int adksys_get_cmdline_buffer(char** output_buffer, int* output_size)
{
   LPWSTR cmdline;
   LPWSTR *argv;
   char* buffer;
   int argc, buffer_size = 0, pos = 0, bytes_written;

   cmdline = GetCommandLineW();
   argv = CommandLineToArgvW(cmdline, &argc);

   for (int i = 0; i < argc; i++) {
      buffer_size += WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
      buffer_size++;
   }
   
   buffer = (char *) malloc(buffer_size);
   for (int i = 0; i < argc; i++) {
      bytes_written = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, buffer + pos, buffer_size - pos, NULL, NULL);
      pos += bytes_written;
   }
   assert(pos <= buffer_size);
   LocalFree(argv);

   *output_buffer = buffer;
   *output_size = pos;

   return 0;
}

int adksys_get_names(char** uid, char** user)
{
   BOOL result;
   char localname[4096];
   unsigned long lname_size = sizeof(localname);

   if (user) {
      result = GetUserNameEx(NameDisplay, localname, &lname_size);
      if (!result)
         return -1;
      *user = strdup(localname);
   }

   if (uid) {
      lname_size = sizeof(localname);
      result = GetUserNameEx(NameCanonical, localname, &lname_size);
      if (!result) {
         free(*user);
         return -1;
      }
      *uid = strdup(localname);
   }

   return 0;
}

int adksys_get_cwd(char* cwd, size_t max_size)
{
   char* p;
   p = getcwd(cwd, (int) max_size);
   if (!p)
      return -1;
   return 0;
}

void* adksys_get_public_adiak_symbol()
{
   return NULL;
}
