#include "adiak.h"
#include "adiak_internal.h"

#include <errno.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>

#include <stdlib.h>
#include <string.h>

static struct passwd *get_passwd()
{
   static struct passwd pwd;
   static int found = 0;
   struct passwd *p;

   if (found == -1)
      return NULL;
   if (found)
      return &pwd;

   p = getpwuid(getuid());
   if (!p) {
      found = -1;
      return NULL;
   }
   pwd.pw_name = strdup(p->pw_name);
   pwd.pw_passwd = NULL;
   pwd.pw_uid = p->pw_uid;
   pwd.pw_gid = p->pw_gid;
   pwd.pw_gecos = strdup(p->pw_gecos);
   pwd.pw_dir = NULL;
   pwd.pw_shell = NULL;
   found = 1;
   return &pwd;
}

int adiak_user()
{
   struct passwd *p;
   char *firstcomma, *user;
  
   p = get_passwd();
   if (!p)
      return -1;
   user = p->pw_gecos;
   firstcomma = strchr(user, ',');
   if (firstcomma)
      *firstcomma = '\0';

   adiak_value("user", adiak_general, "%s", user);
   return 0;
}

int adiak_uid()
{  
   struct passwd *p;
   
   p = get_passwd();
   if (!p)
      return -1;
   
   adiak_value("uid", adiak_general, "%s", p->pw_name);
   return 0;
}

int adiak_hostname()
{
   char hostname[512];
   int result;
   
   result = gethostname(hostname, sizeof(hostname));
   if (result == -1)
      return -1;
   hostname[sizeof(hostname)-1] = '\0';

   return adiak_value("hostname", adiak_general, "%s", hostname);
}

int adiak_clustername()
{
   char hostname[512];
   char clustername[512];
   int result;
   int i = 0;
   char *c;

   memset(hostname, 0, sizeof(hostname));
   memset(clustername, 0, sizeof(hostname));   
   result = gethostname(hostname, sizeof(hostname));
   if (result == -1)
      return -1;

   for (c = hostname; *c; c++) {
      if (*c >= '0' && *c <= '9')
         continue;
      if (*c == '.')
         break;
      clustername[i++] = *c;
   }
   clustername[i] = '\0';

   return adiak_value("cluster", adiak_general, "%s", clustername);
}

int adiak_measure_times(int systime, int cputime)
{
   clock_t tics;
   struct tms buf;
   const char *name;
   long tics_per_sec;

   tics = times(&buf);
   if (tics == (clock_t) -1)
      return -1;      

   if (systime) {
      tics = buf.tms_stime;
      name = "systime";
   }
   else if (cputime) {
      tics = buf.tms_utime;
      name = "cputime";
   }
   else
      return -1;

   errno = 0;
   tics_per_sec = sysconf(_SC_CLK_TCK);
   if (errno) {
      tics_per_sec = 100;
   }
      
   struct timeval *v = (struct timeval *) malloc(sizeof(struct timeval));
   v->tv_sec = tics / tics_per_sec;
   v->tv_usec = (tics % tics_per_sec) * (1000000 / tics_per_sec);
   return adiak_value(name, adiak_performance, "%t", v);
}

#if defined(MPI_VERSION)
#define MAX_HOSTNAME_LEN 1025

static int hostname_color(char *str, int index)
{
   int hash = 5381;
   int c;
   
   while ((c = *str++))
      hash = ((hash << 5) + hash) + (c^index); /* hash * 33 + c */
   
   return hash;
}

static int get_unique_host(char *name, int global_rank)
{
   int size, rank, color;
   int set_oldcomm = 0;
   MPI_Comm adiak_communicator = adiak_globals()->adiak_communicator;
   MPI_Comm newcomm = MPI_COMM_NULL, oldcomm = adiak_communicator;
   char oname[MAX_HOSTNAME_LEN];
   int result, err_ret = -1;
   int index = 0;

   MPI_Comm_rank(adiak_communicator, &global_rank);

   for (;;) {
      color = hostname_color(name, index++);
      if (color < 0)
         color *= -1;
      
      result = MPI_Comm_split(oldcomm, color, global_rank, &newcomm);
      if (result != MPI_SUCCESS) {
         goto error;
      }
      
      if (set_oldcomm) {
         MPI_Comm_free(&oldcomm);
         oldcomm = MPI_COMM_NULL;
      }

      MPI_Comm_rank(newcomm, &rank);
      MPI_Comm_size(newcomm, &size);
      
      if (rank == 0)
         strncpy(oname, name, MAX_HOSTNAME_LEN);
      result = MPI_Bcast(oname, MAX_HOSTNAME_LEN, MPI_CHAR, 0, newcomm);
      if (result != MPI_SUCCESS) {
         goto error;
      }

      int global_str_match = 0;
      int str_match = (strcmp(name, oname) == 0);
      result = MPI_Allreduce(&str_match, &global_str_match, 1, MPI_INT, MPI_LAND, newcomm);
      if (result != MPI_SUCCESS) {
         goto error;
      }

      if (global_str_match) {
         break;
      }
      
      set_oldcomm = 1;
      oldcomm = newcomm;
   }
   
   err_ret = 0;
  error:

   if (oldcomm != adiak_communicator && oldcomm != MPI_COMM_NULL)
      MPI_Comm_free(&oldcomm);
   if (newcomm != adiak_communicator && newcomm != MPI_COMM_NULL)
      MPI_Comm_free(&newcomm);
   
   if (err_ret == -1)
      return err_ret;
   return (rank == 0);
}

static int gethostlist(char **hostlist, int *num_hosts, int *max_hostlen)
{
   int unique_host;
   int rank;
   MPI_Comm hostcomm;
   char name[MAX_HOSTNAME_LEN], *firstdot;
   int namelen, hostlist_size;

   adiak_t* global_adiak = adiak_globals();
   MPI_Comm adiak_communicator = global_adiak->adiak_communicator;

   *hostlist = NULL;
   memset(name, 0, MAX_HOSTNAME_LEN);
   gethostname(name, MAX_HOSTNAME_LEN-1);
   firstdot = strchr(name, '.');
   if (firstdot)
      *firstdot = '\0';
   namelen = strlen(name);

   MPI_Comm_rank(adiak_communicator, &rank);

   unique_host = get_unique_host(name, rank);
   if (unique_host == -1)
      return -1;

   MPI_Comm_split(adiak_communicator, unique_host, rank, &hostcomm);
   if (unique_host) {
      MPI_Comm_size(hostcomm, num_hosts);
      MPI_Allreduce(&namelen, max_hostlen, 1, MPI_INT, MPI_MAX, hostcomm);
      *max_hostlen += 1;

      hostlist_size = (*max_hostlen) * (*num_hosts);
      if (rank == 0)
         *hostlist = (char *) malloc(hostlist_size);
      MPI_Gather(name, *max_hostlen, MPI_CHAR, *hostlist, *max_hostlen, MPI_CHAR, 0, hostcomm);
   }

   MPI_Comm_free(&hostcomm);

   if (!global_adiak->report_on_all_ranks)
      return 0;

   MPI_Bcast(max_hostlen, 1, MPI_INT, 0, adiak_communicator);
   MPI_Bcast(num_hosts, 1, MPI_INT, 0, adiak_communicator);   
   hostlist_size = (*max_hostlen) * (*num_hosts);
   if (!(*hostlist))
       *hostlist = (char *) malloc(hostlist_size);
   MPI_Bcast(*hostlist, hostlist_size, MPI_CHAR, 0, adiak_communicator);

   return 0;
}
#endif

int adiak_hostlist()
{
#if MPI_VERSION
   int num_hosts = 0, max_hostlen = 0, result, i;
   char *hostlist = NULL;
   char **hostlist_array = NULL;
   int err_ret = -1;

   if (!adiak_globals()->use_mpi)
      return -1;
   
   result = gethostlist(&hostlist, &num_hosts, &max_hostlen);
   if (result == -1)
      goto error;

   if (!hostlist)
      goto done;

   hostlist_array = malloc(sizeof(char *) * num_hosts);
   for (i = 0; i < num_hosts; i++)
      hostlist_array[i] = hostlist + (i * max_hostlen);

   result = adiak_value("hostlist", adiak_general, "[%s]", num_hosts, hostlist_array);
   if (result == -1)
      goto error;

  done:
   err_ret = 0;
  error:
   if (hostlist)
      free(hostlist);
   if (hostlist_array)
      free(hostlist_array);
   return err_ret;
#else
   return -1;
#endif   
}
