#include "adiak.hpp"

#if defined(MPI_VERSION)
void adiak::init(MPI_Comm *communicator) {
   adiak_init(communicator);
}
#else
void adiak::init() {
   adiak_init(NULL);
}   
#endif

void adiak::fini() {
   adiak_fini();
}

bool adiak::user() {
   return adiak_user() == 0;
}

bool adiak::uid() {
   return adiak_uid() == 0;
}

bool adiak::launchdate() {
   return adiak_launchdate() == 0;
}

bool adiak::executable() {
   return adiak_executable() == 0;
}

bool adiak::executablepath() {
   return adiak_executablepath() == 0;
}

bool adiak::libraries() {
   return adiak_libraries() == 0;
}

bool adiak::cmdline() {
   return adiak_cmdline() == 0;
}

bool adiak::hostname() {
   return adiak_hostname() == 0;
}

bool adiak::clustername() {
   return adiak_clustername() == 0;
}

bool adiak::walltime() {
   return adiak_walltime() == 0;
}

bool adiak::systime() {
   return adiak_systime() == 0;
}

bool adiak::cputime() {
   return adiak_cputime() == 0;
}   

bool adiak::jobsize() {
   return adiak_job_size() == 0;
}

bool adiak::hostlist() {
   return adiak_hostlist() == 0;
}
