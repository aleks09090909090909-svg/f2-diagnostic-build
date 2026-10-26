#define _POSIX_C_SOURCE 200809L
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static const char *keys[4]={"password","password2","password3","passwordSw"};
int main(int argc,char **argv){
 const char *dbpath=argc>1?argv[1]:"/root/machine.db";
 const char *usb=argc>2?argv[2]:"/mnt/nand_disk";
 sqlite3 *db=NULL;sqlite3_stmt *stmt=NULL;
 char vals[4][512]={{0}};int found[4]={0},rc,result=1;
 fprintf(stdout,"F2_DIAGNOSTIC_VERSION=1.0.3-glibc\nSQLITE_RUNTIME_VERSION=%s\n",sqlite3_libversion());
 rc=sqlite3_open_v2(dbpath,&db,SQLITE_OPEN_READONLY|SQLITE_OPEN_PRIVATECACHE,NULL);
 if(rc!=SQLITE_OK){fprintf(stderr,"ERROR: sqlite open: %s\n",db?sqlite3_errmsg(db):"unknown");goto out;}
 sqlite3_busy_timeout(db,10000);
 rc=sqlite3_prepare_v2(db,"SELECT * FROM yk_asset_setting",-1,&stmt,NULL);
 if(rc!=SQLITE_OK){fprintf(stderr,"ERROR: sqlite prepare: %s\n",sqlite3_errmsg(db));goto out;}
 if(sqlite3_column_count(stmt)<2){fprintf(stderr,"ERROR: table layout\n");goto out;}
 while((rc=sqlite3_step(stmt))==SQLITE_ROW){
  const char *key=(const char*)sqlite3_column_text(stmt,0);
  const char *val=(const char*)sqlite3_column_text(stmt,1);
  if(!key||!val)continue;
  for(int i=0;i<4;i++)if(strcmp(key,keys[i])==0){
   size_t n=strlen(val);if(n>=sizeof vals[i]){fprintf(stderr,"ERROR: oversized %s\n",key);goto out;}
   memcpy(vals[i],val,n+1);found[i]=1;
  }
 }
 if(rc!=SQLITE_DONE){fprintf(stderr,"ERROR: sqlite step: %s\n",sqlite3_errmsg(db));goto out;}
 for(int i=0;i<4;i++)if(!found[i]){fprintf(stderr,"ERROR: missing %s\n",keys[i]);goto out;}
 char path[1024],tmp[1024];snprintf(path,sizeof(path),"%s/F2_passwords.txt",usb);snprintf(tmp,sizeof(tmp),"%s/F2_passwords.tmp",usb);
 FILE *f=fopen(tmp,"wb");if(!f){fprintf(stderr,"ERROR: report open: %s\n",strerror(errno));goto out;}
 int ok=fprintf(f,"DR COFFEE F2 - service settings readout\nSource: /root/machine.db\nREAD ONLY - NO PARAMETERS MODIFIED\n\n")>=0;
 for(int i=0;i<4;i++)if(fprintf(f,"%s = %s\n",keys[i],vals[i])<0)ok=0;
 if(fprintf(f,"\nConfidential - keep this file secure.\n")<0)ok=0;
 if(fflush(f)!=0)ok=0;if(fclose(f)!=0)ok=0;
 if(!ok||rename(tmp,path)!=0){fprintf(stderr,"ERROR: report write\n");remove(tmp);goto out;}
 fprintf(stdout,"OK: %s\n",path);result=0;
out:
 if(stmt)sqlite3_finalize(stmt);if(db)sqlite3_close(db);return result;
}