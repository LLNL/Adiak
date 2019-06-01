#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "adksys.h"

int adksys_get_names(char **uid, char **user)
{
   struct passwd *p;
   char *firstcomma;

   p = getpwuid(getuid());
   if (!p)
      return -1;

   if (user) {
      *user = strdup(p->pw_name);
      firstcomma = strchr(*user, ',');
      if (firstcomma)
         *firstcomma = '\0';
   }
   if (uid) {
      *uid = strdup(p->pw_gecos);
   }
   return 0;
}
