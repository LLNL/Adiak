// Copyright 2019 Lawrence Livermore National Security, LLC
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include "adksys.h"

#if !defined(USE_MPI)
#error Building adksys_mpi without USE_MPI defined
#endif

#define MAX_HOSTNAME_LEN 1025

static MPI_Comm adiak_communicator;

static int hostname_color(char *str, int index)
{
   int hash = 5381;
   int c;
   
   while ((c = *str++))
      hash = ((hash << 5) + hash) + (c^index); /* hash * 33 + c */
   
   return hash;
}

// Return -1 on error, 1 for the lowest rank on each node, and 0 for all other nodes
static int get_unique_host(char *name, int global_rank)
{
   int size, rank, color;
   int set_oldcomm = 0;
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
   return (rank == 0 ? 1 : 0);
}

static int gethostlist(char **hostlist, int *num_hosts, int *max_hostlen, int all_ranks)
{
   int unique_host;
   int rank;
   MPI_Comm hostcomm;
   char name[MAX_HOSTNAME_LEN], *firstdot;
   int namelen, hostlist_size;

   *hostlist = NULL;
   memset(name, 0, MAX_HOSTNAME_LEN);
   adksys_hostname(name, MAX_HOSTNAME_LEN-1);
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

   MPI_Bcast(max_hostlen, 1, MPI_INT, 0, adiak_communicator);
   MPI_Bcast(num_hosts, 1, MPI_INT, 0, adiak_communicator);   
   hostlist_size = (*max_hostlen) * (*num_hosts);
   if (!(*hostlist))
       *hostlist = (char *) malloc(hostlist_size);
   MPI_Bcast(*hostlist, hostlist_size, MPI_CHAR, 0, adiak_communicator);

   return 0;
}

int adksys_hostlist(char ***out_hostlist_array, int *out_num_hosts, char **out_name_buffer, int all_ranks)
{
   static char *hostlist = NULL;
   static char **hostlist_array = NULL;
   static int num_hosts = 0;
   static int had_error = 0;
   int max_hostlen = 0, result, i;

   if (had_error)
      return -1;
   
   if (hostlist)
      goto done;

   result = gethostlist(&hostlist, &num_hosts, &max_hostlen, all_ranks);
   if (result == -1)
      goto error;

   if (!hostlist)
      goto done;

   hostlist_array = malloc(sizeof(char *) * num_hosts);
   for (i = 0; i < num_hosts; i++)
      hostlist_array[i] = hostlist + (i * max_hostlen);      

  done:
   *out_hostlist_array = hostlist_array;
   *out_num_hosts = num_hosts;
   *out_name_buffer = hostlist;
   return 0;
  error:
   if (hostlist)
      free(hostlist);
   if (hostlist_array)
      free(hostlist_array);
   had_error = 1;
   return -1;
}

int adksys_jobsize(int *size)
{
   int result;

   result = MPI_Comm_size(adiak_communicator, size);
   if (result != MPI_SUCCESS)
      return -1;
   return 0;
}

int adksys_reportable_rank()
{
   int result;
   int rank;

   result = MPI_Comm_rank(adiak_communicator, &rank);
   if (result != MPI_SUCCESS)
      return -1;

   return (rank == 0);
}

int adksys_mpi_init(void *mpi_communicator_p)
{
   adiak_communicator = *((MPI_Comm *) mpi_communicator_p);
   return 0;
}

int adksys_mpi_initialized()
{
   int result = 0;
   MPI_Initialized(&result);
   return result;
}
