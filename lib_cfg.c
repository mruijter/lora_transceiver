#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <lib_cfg.h>
#include <lib_str.h>
#include <lib_log.h>

int
read_s_cfg (char *cfgfile, char *value, int size)
{

  int fd;
  int rc;
  char *ptr;

  fd = open (cfgfile, O_RDONLY);
  if (fd == -1)
  {
      snprintf (value, size, strerror (errno));
      return (-1);
  }

  rc = read (fd, value, size);
  if (rc == -1)
  {
      snprintf (value, size, strerror (errno));
      return (-1);
  }
  else if (rc == 0)
  {
      snprintf (value, size, "Read 0 bytes");
      return (-1);
  }

  if ((ptr = strchr (value, '\n')) != NULL)
    *ptr = '\0';

  if (iscntrl (value[0]) != 0)
  {
      snprintf (value, size, "%s starts with an iscntrl character", cfgfile);
      return (-1);
  }

  if (isspace (value[0]) != 0)
  {
      snprintf (value, size, "%s starts with an isspace character", cfgfile);
      return (-1);
  }

  if (value[0] == '#')
  {
      snprintf (value, size, "%s starts with a comment character (#)",
		cfgfile);
      return (-1);
  }

  if ((ptr = strchr (value, ' ')) != NULL)
    *ptr = '\0';

  if ((ptr = strchr (value, '\t')) != NULL)
    *ptr = '\0';

  if ((ptr = strchr (value, '#')) != NULL)
    *ptr = '\0';

  if (close (fd) == -1)
  {
      snprintf (value, size, "failed to close %s", cfgfile);
      return (0);
  }
  return (0);
}


int
read_m_cfg (char *cfgfile, char *value, char *value2, int size)
{

  int fd;
  int rc;
  char *ptr;

  fd = open (cfgfile, O_RDONLY);
  if (fd == -1)
    {
      snprintf (value, size, strerror (errno));
      return (-1);
    }

  rc = read (fd, value, size);
  if (rc == -1)
    {
      snprintf (value, size, strerror (errno));
      return (-1);
    }
  else if (rc == 0)
    {
      snprintf (value, size, "Read 0 bytes");
      return (-1);
    }

  if ((ptr = strchr (value, '\n')) != NULL)
    *ptr = '\0';

  if (iscntrl (value[0]) != 0)
    {
      snprintf (value, size, "%s starts with an iscntrl character", cfgfile);
      return (-1);
    }

  if (isspace (value[0]) != 0)
    {
      snprintf (value, size, "%s starts with an isspace character", cfgfile);
      return (-1);
    }

  if (value[0] == '#')
    {
      snprintf (value, size, "%s starts with a comment character (#)",
		cfgfile);
      return (-1);
    }

  if ((ptr = strchr (value, ' ')) != NULL)
    *ptr = '\0';

  if ((ptr = strchr (value, '\t')) != NULL)
    *ptr = '\0';

  if ((ptr = strchr (value, '#')) != NULL)
    *ptr = '\0';

  if ((ptr = strchr (value, ':')) == NULL)
    {
      snprintf (value, size, "%s does not contain a semi-colon", cfgfile);
      snprintf (value2, size, "(null)");
      return (-1);
    }

  snprintf (value2, size, ptr + 1);
  *ptr = '\0';

  if (close (fd) == -1)
    {
      snprintf (value, size, "failed to close %s", cfgfile);
      return (0);
    }

  return (0);

}


/***********************************************************************
 * Sub: r_env_cfg
 * Doel: Inlezen v.e. configuratie file en de waardes in in de
 *       env. zetten
 * Parameters : pad/naam van het configuratie bestand
 * Returns : 0 on succes, -1 if failed
***********************************************************************/
int
r_env_cfg (char *configfile)
{
  FILE *config;
  char buf[1024];		/* Configuration files with lines longer > 1024 bytes will not work */
  char *token;
  char *value;
  char *debug;

  config = fopen (configfile, "r");
  if (NULL == config)
    {
      fprintf (stderr, "Failed to open configuration file: %s\n", configfile);
      return (-1);
    }
  while (1)
    {
      debug = getenv ("DEBUG");
      fgets (buf, 1023, config);
      if (feof (config))
	break;
      if (0 != unrem (buf))
      {
	  if (NULL == (value = strchr (buf, '='))) continue;
          token=buf;
          *value=0;
          value++;
	  trim (token);
	  trim (value);
	  if (debug != NULL)
		{
	           if (atoi (debug) >= 64)
		   fprintf (stderr, "setenv |%s|=|%s|\n", token, value);
          }
	  if (-1 == (setenv (token, value, 1)))
          {
		  fprintf (stderr, "Setenv failed, out of resources.\n");
		  exit (-1);
          }
	}
     }
  return (0);
}


/***********************************************************************
 * Sub: read_val
 * Doel: leest een waarde uit de env.
 * Parameters : input token
 * Returns : exit on error or value
***********************************************************************/
char * read_val(char *token)
{
   char *a, *env;
   int debug=0;
 
   env=getenv("DEBUG");
   if ( NULL != env ) debug=atoi(env);
   a=getenv(token);
   if ( NULL == a ) {
      log(1,"Fatal error, couldn't read %s\n.",token);
      exit(-1);
   }
   if ( debug >= 32 ) log(1,"%s=%s\n",token,a);
   return a;
}
