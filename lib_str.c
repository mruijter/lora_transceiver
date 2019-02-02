/*
 * This file contains some handy string manipulation routines...
 *
 */
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>

/******************************************************
* Procedure trim()                                    *
* Doel : Ontdoe een regel van spaties,tabs,nl,cr      *
* Input: char naam logbestand                         *
* Output: filehandler anders -1                       *
******************************************************/
void
trim (char *regel)
{
  int tel;
  int einde, lengte;
  char *tmpstr;

  if (regel == NULL)
    return;			/* Een lege string is niet te trimmen */

  lengte = strlen (regel);
  tmpstr = strdup (regel);
  if (NULL == tmpstr)
    {
      fprintf (stderr, "Malloc failed, Insufficient memory.\n");
      exit (-1);
    }

  for (tel = 0; tel <= lengte; tel++)
    {
      if (regel[tel] != ' ' && regel[tel] != '\t')
	break;
    }

  einde = lengte;
  if (einde == 0)
    {
      regel[0] = 0;
      free (tmpstr);
      return;			/* Na het begin v.d. regel trimmen niets over */
    }

  while (einde > 0)
    {
      einde--;
      if (regel[einde] != ' ' && regel[einde] != '\t' && regel[einde] != '\n'
	  && regel[einde] != '\r')
	break;
    }

  lengte = einde - tel + 1;
  memcpy (&regel[0], &tmpstr[tel], lengte);
  regel[lengte] = 0;
  free (tmpstr);
  return;
}

/***********************************************************************
Procedure stripchar
Doel : Verwijderd een %c uit een %s
***********************************************************************/
void
stripchar (char *a, char c)
{
  char *p;
  p=a;
  while (*a != 0)
  {
     if ( *a != c )
     {
         *p=*a;
         *p++;
     }
     *a++;
  }
  *p=0;
}

/***********************************************************************
Procedure lcase
Doel : Convert een string naar lowercase
***********************************************************************/
void
lcase (char *convert)
{
  while (*convert != 0)
    {
      *convert = tolower (*convert);
      *convert++;
    }
}

/***********************************************************************
Procedure ucase
Doel : Convert een string naar uppercase
***********************************************************************/
void
ucase (char *convert)
{
  while (*convert != 0)
    {
      *convert = tolower (*convert);
      *convert++;
    }
}

/***********************************************************************
Procedure unrem
Invoer regel[]
Uitvoer regel zonder remark's
return lengte ge 'unremde' string
***********************************************************************/
int
unrem (char regel[])
{
  char *c;

  c = strchr (regel, '#');
  if (c != NULL)
    c[0] = 0;
  return (strlen (regel));
}
