#include "include/lib_log.h"
/* Howto use this function:
 * Just log("something,%s\n",somestring);
 * Howto change the facility: export FACILITY=LOG_AUTH;
 * Howto change the priority: export PRIORIY=LOG_DEBUG;
 * You can set these in your program startupscript use let r_env_cfg() do it.
 */ 

int debug;

int log (int level, const char *fmt, ...)
{
  int size = 512;
  int facility=LOG_USER, priority=LOG_INFO;
  int res;
  char *p;
  char *logname, *env;
  va_list ap;

  if ( level >= debug ) {
    return 0;
  }

  if ((p = (char *)malloc(size)) == NULL)
    return (-1);

  while (1)
    {
      va_start (ap, fmt);
      res = vsnprintf (p, size, fmt, ap);
      va_end (ap);

      if (res > -1 && res < size)
	break;

      if (res > -1)
	size = res + 1;
      else
	size *= 2;

      if ((p = (char *)realloc (p, size)) == NULL)
	return (-1);
    }
  logname=getenv("PRGNAME");
  env=getenv("FACILITY");
  if ( NULL != env )
  {
     if ( 0 == strcasecmp(env,"LOG_USER")) facility=LOG_USER;
     if ( 0 == strcasecmp(env,"LOG_AUTH")) facility=LOG_AUTHPRIV;
     if ( 0 == strcasecmp(env,"LOG_AUTHPRIV")) facility=LOG_AUTHPRIV;
     if ( 0 == strcasecmp(env,"LOG_CRON")) facility=LOG_CRON;
     if ( 0 == strcasecmp(env,"LOG_KERN")) facility=LOG_KERN;
     if ( 0 == strcasecmp(env,"LOG_LOCAL0")) facility=LOG_LOCAL0;
     if ( 0 == strcasecmp(env,"LOG_LOCAL1")) facility=LOG_LOCAL1;
     if ( 0 == strcasecmp(env,"LOG_LOCAL2")) facility=LOG_LOCAL2;
     if ( 0 == strcasecmp(env,"LOG_LOCAL3")) facility=LOG_LOCAL3;
     if ( 0 == strcasecmp(env,"LOG_LOCAL4")) facility=LOG_LOCAL4;
     if ( 0 == strcasecmp(env,"LOG_LOCAL5")) facility=LOG_LOCAL5;
     if ( 0 == strcasecmp(env,"LOG_LOCAL6")) facility=LOG_LOCAL6;
     if ( 0 == strcasecmp(env,"LOG_LOCAL7")) facility=LOG_LOCAL7;
     if ( 0 == strcasecmp(env,"LOG_LPR")) facility=LOG_LPR;
     if ( 0 == strcasecmp(env,"LOG_MAIL")) facility=LOG_MAIL;
     if ( 0 == strcasecmp(env,"LOG_NEWS")) facility=LOG_NEWS;
     if ( 0 == strcasecmp(env,"LOG_SYSLOG")) facility=LOG_SYSLOG;
     if ( 0 == strcasecmp(env,"LOG_UUCP")) facility=LOG_UUCP;
  }
  env=getenv("PRIORITY");
  if ( NULL != env )
  {
     if ( 0 == strcasecmp(env,"LOG_EMERG")) priority=LOG_EMERG;
     if ( 0 == strcasecmp(env,"LOG_ALERT")) priority=LOG_ALERT;
     if ( 0 == strcasecmp(env,"LOG_CRIT")) priority=LOG_CRIT;
     if ( 0 == strcasecmp(env,"LOG_ERR")) priority=LOG_ERR;
     if ( 0 == strcasecmp(env,"LOG_WARNING")) priority=LOG_WARNING;
     if ( 0 == strcasecmp(env,"LOG_NOTICE")) priority=LOG_NOTICE;
     if ( 0 == strcasecmp(env,"LOG_INFO")) priority=LOG_INFO;
     if ( 0 == strcasecmp(env,"LOG_DEBUG")) priority=LOG_DEBUG;
  }
  openlog (logname, LOG_PID, facility);
  syslog (priority, p);
  closelog ();
  free (p);
  return (0);
}
