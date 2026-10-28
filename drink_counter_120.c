#include <stdio.h>
#include <sqlite3.h>
int main(int argc,char**argv){
 if(argc!=3)return 2;
 sqlite3 *db=NULL;
 int rc=sqlite3_open_v2(argv[1],&db,SQLITE_OPEN_READONLY|SQLITE_OPEN_PRIVATECACHE,NULL);
 if(rc!=SQLITE_OK){if(db)sqlite3_close(db);return 3;}
 sqlite3_busy_timeout(db,1500);
 sqlite3_stmt *st=NULL;
 const char *q="SELECT COUNT(*),COALESCE(SUM(CASE WHEN order_type=1 THEN 1 ELSE 0 END),0),COALESCE(SUM(CASE WHEN order_type<>1 THEN 1 ELSE 0 END),0) FROM yk_device_make";
 rc=sqlite3_prepare_v2(db,q,-1,&st,NULL);
 if(rc!=SQLITE_OK){sqlite3_close(db);return 4;}
 rc=sqlite3_step(st);
 if(rc!=SQLITE_ROW){sqlite3_finalize(st);sqlite3_close(db);return 5;}
 long long total=sqlite3_column_int64(st,0),test=sqlite3_column_int64(st,1),regular=sqlite3_column_int64(st,2);
 sqlite3_finalize(st);sqlite3_close(db);
 if(total<0||total>1000000000||regular+test!=total)return 6;
 FILE*f=fopen(argv[2],"w");if(!f)return 7;
 fprintf(f,"TOTAL=%lld\nREGULAR=%lld\nTEST=%lld\n",total,regular,test);
 return fclose(f)?8:0;
}
