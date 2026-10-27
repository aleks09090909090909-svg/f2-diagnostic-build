#define _POSIX_C_SOURCE 200809L
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static void cell(FILE *f,const unsigned char *v){
 if(!v)return;
 int n=0;while(*v && n<400){unsigned char c=*v++;fputc((c==9||c==10||c==13||c<32)?' ':c,f);n++;}
}
static int type_of(const char *name){
 char b[256];snprintf(b,sizeof(b),"%s",name);
 for(char *p=b;*p;p++)if(*p>='A'&&*p<='Z')*p+=32;
 if(strstr(b,"error")||strstr(b,"alarm")||strstr(b,"fault")||strstr(b,"warn")||strstr(b,"trouble")||strstr(b,"exception"))return 2;
 if(strstr(b,"drink")||strstr(b,"cup")||strstr(b,"sale")||strstr(b,"order")||strstr(b,"count")||strstr(b,"coffee")||strstr(b,"product")||strstr(b,"stat"))return 1;
 return 0;
}

/* Probe confirmed marker across every SQLite table, including WAL-visible rows.
   Never call untrusted SQL functions, only quoted identifiers and SELECT.
   Limit each table to 5000 rows, each value to 16384 bytes. */
static void probe_code5(sqlite3 *db,const char *usb){
 char path[1024];snprintf(path,sizeof(path),"%s/F2_code5_search.tsv",usb);
 FILE *out=fopen(path,"wb");if(!out)return;
 fputs("База\tТаблица\tСтрока\tПоле\tСовпадение\tКонтекст\tПроверено строк\tПримечание\n",out);
 sqlite3_stmt *tables=NULL;int hits=0,examined=0;
 if(sqlite3_prepare_v2(db,"SELECT name FROM sqlite_master WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'",-1,&tables,NULL)==SQLITE_OK){
  while(sqlite3_step(tables)==SQLITE_ROW&&examined<200){
   const char *orig=(const char*)sqlite3_column_text(tables,0);if(!orig)continue;
   char name[240];snprintf(name,sizeof(name),"%s",orig);
   char ident[500];int k=0;
   for(const char *p=name;*p&&k<495;p++){if(*p=='"')ident[k++]='"';ident[k++]=*p;}ident[k]=0;
   char sql[700];snprintf(sql,sizeof(sql),"SELECT * FROM \"%s\" LIMIT 5000",ident);
   sqlite3_stmt *rows=NULL;
   if(sqlite3_prepare_v2(db,sql,-1,&rows,NULL)!=SQLITE_OK)continue;
   examined++;int nrow=0;
   while(sqlite3_step(rows)==SQLITE_ROW && nrow<5000 && hits<500){
    nrow++;int cols=sqlite3_column_count(rows);
    for(int col=0;col<cols&&col<80;col++){
     const char *cn=sqlite3_column_name(rows,col);if(!cn)continue;
     const unsigned char *v=sqlite3_column_text(rows,col);if(!v || sqlite3_column_type(rows,col)==SQLITE_BLOB)continue;
     int n=sqlite3_column_bytes(rows,col);if(n>16384)n=16384;
     for(int j=0;j+5<=n;j++){
      if((v[j]=='c'||v[j]=='C')&&(v[j+1]=='o'||v[j+1]=='O')&&(v[j+2]=='d'||v[j+2]=='D')&&(v[j+3]=='e'||v[j+3]=='E')&&v[j+4]=='5'){
       int st=j>90?j-90:0,ed=j+95<n?j+95:n;
       fputs("machine.db\t",out);cell(out,(const unsigned char*)name);fprintf(out,"\t%d\t",nrow);
       cell(out,(const unsigned char*)cn);fputs("\tcode5\t",out);
       for(int z=st;z<ed;z++){int ch=v[z];fputc((ch<32||ch==9||ch==10||ch==13)?' ':ch,out);}
       fprintf(out,"\t%d\tСовпадение в SQLite; требуется подтверждение назначения\n",nrow);
       hits++;break;
      }
     }
    }
   }
   if(hits>=500){sqlite3_finalize(rows);break;}
   sqlite3_finalize(rows);
  }
 }
 if(tables)sqlite3_finalize(tables);
 if(hits==0)fprintf(out,"machine.db\t(все доступные)\t\t\tcode5\tНе найдено в текстовых значениях\t\tВозможен журнал HMI или MCU; отсутствие совпадений не значит отсутствие ошибки\n");
 fclose(out);
 fprintf(stdout,"CODE5_SQLITE_TABLES_SCANNED=%d\nCODE5_SQLITE_MATCHES=%d\n",examined,hits);
}
static void export_observations(sqlite3 *db,const char *usb){
 char dp[1024],ep[1024],sp[1024];
 snprintf(dp,sizeof(dp),"%s/F2_drink_statistics.tsv",usb);
 snprintf(ep,sizeof(ep),"%s/F2_error_records.tsv",usb);
 snprintf(sp,sizeof(sp),"%s/F2_statistics_sources.tsv",usb);
 FILE *drink=fopen(dp,"wb"),*err=fopen(ep,"wb"),*schema=fopen(sp,"wb");
 if(drink)fputs("Напиток\tСтатус записи\tКоличество записей\tОписание\n",drink);
 if(err)fputs("Таблица\tСтрока\tПоле\tЗначение\tПримечание\n",err);
 if(schema)fputs("Таблица\tПоле\tТип\tСтрок исследовано\tКатегория\n",schema);
 sqlite3_stmt *t=NULL;
 if(sqlite3_prepare_v2(db,"SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'",-1,&t,NULL)==SQLITE_OK){
  int tables=0;
  while(sqlite3_step(t)==SQLITE_ROW && tables<150){
   const char *raw=(const char*)sqlite3_column_text(t,0);if(!raw)continue;
   char name[256];snprintf(name,sizeof(name),"%s",raw);int kind=type_of(name);
   if(!kind)continue;tables++;
   char escaped[520];int j=0;
   for(const char *p=name;*p&&j<510;p++){if(*p=='"')escaped[j++]='"';escaped[j++]=*p;}escaped[j]=0;
   char sql[640];snprintf(sql,sizeof(sql),"SELECT * FROM \"%s\" LIMIT 1000",escaped);
   sqlite3_stmt *rows=NULL;if(sqlite3_prepare_v2(db,sql,-1,&rows,NULL)!=SQLITE_OK)continue;
   FILE *out=(kind==2)?err:NULL;int row=0,cols=sqlite3_column_count(rows);
   while(sqlite3_step(rows)==SQLITE_ROW && row<1000){
    row++;for(int col=0;col<cols&&col<50;col++){
     const char *cn=sqlite3_column_name(rows,col);if(!cn)continue;
     if(type_of(cn)==2 && kind!=2){} /* Original field kept for traceability. */
     char tmp[128];snprintf(tmp,sizeof(tmp),"%s",cn);
     for(char *p=tmp;*p;p++)if(*p>='A'&&*p<='Z')*p+=32;
     if(strstr(tmp,"password")||strstr(tmp,"token")||strstr(tmp,"secret")||strstr(tmp,"auth")||strstr(tmp,"key")||strstr(tmp,"mac")||strstr(tmp,"serial")||strstr(tmp,"uuid"))continue;
     if(!out)continue;
     cell(out,(const unsigned char*)name);fprintf(out,"\t%d\t",row);cell(out,(const unsigned char*)cn);fputc('\t',out);
     if(sqlite3_column_type(rows,col)==SQLITE_BLOB)fputs("[BLOB скрыт]",out);
     else cell(out,sqlite3_column_text(rows,col));
     fputs("\tИсходные данные, интерпретация не подтверждена\n",out);
    }
   }
   if(schema)for(int col=0;col<cols&&col<50;col++){
    cell(schema,(const unsigned char*)name);fputc('\t',schema);cell(schema,(const unsigned char*)sqlite3_column_name(rows,col));
    fprintf(schema,"\t%s\t%d\t%s\n",sqlite3_column_decltype(rows,col)?sqlite3_column_decltype(rows,col):"",row,kind==2?"ошибки":"напитки/статистика");
   }
   sqlite3_finalize(rows);
  }
 }
 if(t)sqlite3_finalize(t);
 /* Vendor database confirms yk_device_make.drink_uuid -> yk_asset_drink.uuid.
    status semantics are not established: show counts separated by status. */
 if(drink){
  sqlite3_stmt *groups=NULL;
  const char *q="SELECT COALESCE(NULLIF(d.name,''),NULLIF(d.eng_name,''),m.drink_uuid,'Неизвестно'),"
                "COALESCE(CAST(m.status AS TEXT),'NULL'),COUNT(*) "
                "FROM yk_device_make m LEFT JOIN yk_asset_drink d ON m.drink_uuid=d.uuid "
                "GROUP BY m.drink_uuid,m.status ORDER BY COUNT(*) DESC LIMIT 3000";
  if(sqlite3_prepare_v2(db,q,-1,&groups,NULL)==SQLITE_OK){
   int rows=0;
   while(sqlite3_step(groups)==SQLITE_ROW){
    cell(drink,sqlite3_column_text(groups,0));fputc('\t',drink);
    cell(drink,sqlite3_column_text(groups,1));fprintf(drink,"\t%lld\t",sqlite3_column_int64(groups,2));
    fputs("Записи истории приготовления, статус успеха пока не расшифрован\n",drink);rows++;
   }
   if(!rows)fputs("Записи отсутствуют\t\t0\tИстория приготовления пуста\n",drink);
  }else fputs("Нет совместимой таблицы\t\t\tНе удалось прочитать историю приготовления\n",drink);
  if(groups)sqlite3_finalize(groups);
 }
 /* Also inspect service history. These are not confirmed fault codes. */
 if(err){
  sqlite3_stmt *maint=NULL;
  if(sqlite3_prepare_v2(db,"SELECT cmd,step,start_at,end_at FROM yk_asset_maintain_record ORDER BY start_at DESC LIMIT 500",-1,&maint,NULL)==SQLITE_OK){
   int row=0;while(sqlite3_step(maint)==SQLITE_ROW){
    row++;const char *names[]={"cmd","step","start_at","end_at"};
    for(int i=0;i<4;i++)fprintf(err,"История обслуживания\t%d\t%s\t%lld\tСервисная запись, не код ошибки\n",row,names[i],sqlite3_column_int64(maint,i));
   }
  }if(maint)sqlite3_finalize(maint);
 }
 if(drink)fclose(drink);if(err)fclose(err);if(schema)fclose(schema);
}
static const char *keys[4]={"password","password2","password3","passwordSw"};
int main(int argc,char **argv){
 const char *dbpath=argc>1?argv[1]:"/root/machine.db";
 const char *usb=argc>2?argv[2]:"/mnt/nand_disk";
 sqlite3 *db=NULL;sqlite3_stmt *stmt=NULL;
 char vals[4][512]={{0}};int found[4]={0},rc,result=1;
 fprintf(stdout,"F2_DIAGNOSTIC_VERSION=1.0.10-universal\nSQLITE_RUNTIME_VERSION=%s\n",sqlite3_libversion());
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

 /* Export key/value settings as Excel-friendly TSV, excluding password fields. */
 char settings_path[1024];
 snprintf(settings_path,sizeof(settings_path),"%s/F2_settings.tsv",usb);
 FILE *sf=fopen(settings_path,"wb");
 if(sf){
  fputs("Раздел\tПараметр\tЗначение\tИсточник\n",sf);
  sqlite3_stmt *ss=NULL;
  if(sqlite3_prepare_v2(db,"SELECT * FROM yk_asset_setting",-1,&ss,NULL)==SQLITE_OK){
   int sr;
   while((sr=sqlite3_step(ss))==SQLITE_ROW){
    const char *k=(const char*)sqlite3_column_text(ss,0);
    const char *v=(const char*)sqlite3_column_text(ss,1);
    if(!k||!v)continue;
    int secret=0;
    for(int i=0;i<4;i++)if(strcmp(k,keys[i])==0)secret=1;
    if(secret)continue;
    fputs("Настройки\t",sf);
    for(const unsigned char *p=(const unsigned char*)k;*p;p++)fputc((*p=='\t'||*p=='\r'||*p=='\n')?' ':*p,sf);
    fputc('\t',sf);
    for(const unsigned char *p=(const unsigned char*)v;*p;p++)fputc((*p=='\t'||*p=='\r'||*p=='\n')?' ':*p,sf);
    fputs("\tmachine.db/yk_asset_setting\n",sf);
   }
  }
  if(ss)sqlite3_finalize(ss);
  fclose(sf);
  fprintf(stdout,"OK: %s\n",settings_path);
 } else fprintf(stderr,"WARN: cannot create F2_settings.tsv: %s\n",strerror(errno));

 /* Statistics: document available candidate tables, without guessing counts. */
 char statpath[1024],schema_path[1024];
 snprintf(statpath,sizeof(statpath),"%s/F2_drink_statistics.tsv",usb);
 snprintf(schema_path,sizeof(schema_path),"%s/F2_statistics_sources.tsv",usb);
 FILE *stf=fopen(statpath,"wb");
 if(stf){fputs("Напиток\tПриготовлено, раз\tПримечание\n",stf);fputs("Нет подтвержденных данных\t\tДоступные таблицы перечислены в листе Источники\n",stf);fclose(stf);}
 FILE *scf=fopen(schema_path,"wb");
 if(scf){
  fputs("Таблица\tСтолбцы\tТип данных\n",scf);
  sqlite3_stmt *list=NULL;
  if(sqlite3_prepare_v2(db,"SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'",-1,&list,NULL)==SQLITE_OK){
   while(sqlite3_step(list)==SQLITE_ROW){
    const char *tn=(const char*)sqlite3_column_text(list,0);if(!tn)continue;
    char lower[256];snprintf(lower,sizeof(lower),"%s",tn);
    for(char *p=lower;*p;p++)if(*p>='A'&&*p<='Z')*p+=32;
    if(!strstr(lower,"drink")&&!strstr(lower,"cup")&&!strstr(lower,"sale")&&!strstr(lower,"order")&&!strstr(lower,"count")&&!strstr(lower,"coffee")&&!strstr(lower,"product")&&!strstr(lower,"stat"))continue;
    char q[512];snprintf(q,sizeof(q),"PRAGMA table_info(\"%s\")",tn);
    sqlite3_stmt *cols=NULL;
    if(sqlite3_prepare_v2(db,q,-1,&cols,NULL)!=SQLITE_OK)continue;
    while(sqlite3_step(cols)==SQLITE_ROW){
     const char *cn=(const char*)sqlite3_column_text(cols,1);
     const char *ct=(const char*)sqlite3_column_text(cols,2);
     if(!cn)continue;
     fprintf(scf,"%s\t%s\t%s\n",tn,cn,ct?ct:"");
    }
    sqlite3_finalize(cols);
   }
  }
  if(list)sqlite3_finalize(list);
  fclose(scf);
 }
 export_observations(db,usb);
 probe_code5(db,usb);
 fprintf(stdout,"OK: %s\n",path);result=0;
out:
 if(stmt)sqlite3_finalize(stmt);if(db)sqlite3_close(db);return result;
}