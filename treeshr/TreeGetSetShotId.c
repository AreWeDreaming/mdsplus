/*------------------------------------------------------------------------------

		Name:   TreeGetCurrentShotId

		Type:   C function

		Author:	Thomas W. Fredian

		Date:   11-OCT-1989

		Purpose: Get current shot number

------------------------------------------------------------------------------

	Call sequence:

int TreeGetCurrentShotId(experiment,shot)

------------------------------------------------------------------------------
   Copyright (c) 1989
   Property of Massachusetts Institute of Technology, Cambridge MA 02139.
   This program cannot be copied or distributed in any form for non-MIT
   use without specific written approval of MIT Plasma Fusion Center
   Management.
------------------------------------------------------------------------------*/
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <mdsdescrip.h>
#include <mdsshr.h>
#include <libroutines.h>
#include <strroutines.h>

extern char *TranslateLogical();

static char *cvsrev = "@(#)$RCSfile$ $Revision$ $Date$";

#define _ToLower(c) (((c) >= 'A' && (c) <= 'Z') ? (c) | 0x20 : (c))

static FILE *CreateShotIdFile(char *experiment)
{
  char pathname[512];
  char *path;
  FILE *file = 0;
  char *semi;
  strcpy(pathname,experiment);
  strcat(pathname,"_path");
  path = (char *)TranslateLogical(pathname);
  if (path != NULL)
  {
    if ((semi = (char *)index(path, ';')) != 0)
      semi = '\0';
    strncpy(pathname,path,500);
    TranslateLogicalFree(path);
#ifdef _WINDOWS
    strcat(pathname,"\\");
#else
    strcat(pathname,"//");
#endif
    strcat(pathname,"shotid.sys");
    file = fopen(pathname,"w+b");
  }
  return file;
}

static FILE *OpenShotIdFile(char *experiment,char *mode)
{
  FILE *file = 0;
  static struct descriptor file_d = {0, DTYPE_T, CLASS_D, 0};
  static DESCRIPTOR(suffix_d,"_path:shotid.sys");
  static struct descriptor experiment_d = {0, DTYPE_T, CLASS_S, 0};
  static struct descriptor filename = {0, DTYPE_T, CLASS_D, 0};
  unsigned short len;
  void *ctx = 0;
  int status = 0;
  int i;
  experiment_d.length = strlen(experiment);
  experiment_d.pointer = experiment;
  StrTrim(&file_d,&experiment_d,&len);
  for (i=0;i<file_d.length;i++) 
    file_d.pointer[i] = _ToLower(file_d.pointer[i]);
  StrAppend(&file_d,&suffix_d);
  if (LibFindFile(&file_d,&filename,&ctx) & 1)
  {
    static DESCRIPTOR(nullstr,"\0");
    StrAppend(&filename,&nullstr);
    file = fopen(filename.pointer,mode);
  }
  else
    file = CreateShotIdFile(experiment);
  return file;
}


int       TreeGetCurrentShotId(char *experiment)
{
  int shot = 0;
  int status = 0;
  char *path = 0;
  char *exp = strcpy(malloc(strlen(experiment)+6),experiment);
  int i;
  int slen;
  for (i=0;exp[i] != '\0';i++)
    exp[i] = _ToLower(exp[i]);
  strcat(exp,"_path");
  path = TranslateLogical(exp);
  if (path && ((slen = strlen(path)) > 2) && (path[slen-1] == ':') && (path[slen-2] == ':'))
    status = TreeGetCurrentShotIdRemote(experiment, path, &shot);
  else
  {
    FILE *file = OpenShotIdFile(experiment,"rb");
    if (file)
    {
      status = fread(&shot,sizeof(shot),1,file) == 1;
      fclose(file);
#ifdef _big_endian
      if (status & 1)
      {
        int lshot = shot;
        int i;
        char *optr = (char *)&shot;
        char *iptr = (char *)&lshot;
        for (i=0;i<4;i++) optr[i] = iptr[3-i];
      }
#endif
    }
  }
  if (path)
    TranslateLogicalFree(path);
  free(exp);
  return (status & 1) ? shot : 0;
}

int       TreeSetCurrentShotId(char *experiment, int shot)
{
  int status = 0;
  char *path = 0;
  char *exp = strcpy(malloc(strlen(experiment)+6),experiment);
  int slen;
  int i;
  for (i=0;exp[i] != '\0';i++)
    exp[i] = _ToLower(exp[i]);
  strcat(exp,"_path");
  path = TranslateLogical(exp);
  if (path && ((slen = strlen(path)) > 2) && (path[slen-1] == ':') && (path[slen-2] == ':'))
    status = TreeSetCurrentShotIdRemote(experiment, path, shot);
  else
  {
    FILE *file = OpenShotIdFile(experiment,"r+b");
    if (file)
    {
      int lshot = shot;
#ifdef _big_endian
      int i;
      char *optr = (char *)&lshot;
      char *iptr = (char *)&shot;
      for (i=0;i<4;i++) optr[i] = iptr[3-i];
#endif
      status = fwrite(&lshot,sizeof(shot),1,file) == 1;
      fclose(file);
    }
  }
  if (path)
    TranslateLogicalFree(path);
  free(exp);
  return status;
}

