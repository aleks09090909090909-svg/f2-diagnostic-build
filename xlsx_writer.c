#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/* Standalone OOXML XLSX writer: ZIP stored entries, no external libraries. */
typedef struct {char *s;size_t n,cap;} Buf;
static void append(Buf *b,const void *p,size_t n){
 if(b->n+n+1>b->cap){size_t c=b->cap?b->cap:1024;while(c<b->n+n+1)c*=2;char *q=realloc(b->s,c);if(!q){fprintf(stderr,"out of memory\n");exit(2);}b->s=q;b->cap=c;}
 memcpy(b->s+b->n,p,n);b->n+=n;b->s[b->n]=0;
}
static void a(Buf *b,const char *s){append(b,s,strlen(s));}
static void esc(Buf *b,const char *s){
 for(const unsigned char *p=(const unsigned char*)s;*p;p++){
  switch(*p){case '&':a(b,"&amp;");break;case '<':a(b,"&lt;");break;case '>':a(b,"&gt;");break;case '"':a(b,"&quot;");break;
  default:if(*p>=32 || *p==9)append(b,p,1);}
 }
}
static uint32_t crc(const unsigned char *p,size_t n){
 uint32_t c=~0U;while(n--){c^=*p++;for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320U & (0U-(c&1)));}return ~c;
}
static void u16(FILE *f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}
static void u32(FILE *f,uint32_t v){u16(f,v&65535);u16(f,v>>16);}
typedef struct {char name[96];uint32_t offset,size,crc;} Entry;
static void entry(FILE *out,Entry *e,const char *name,const Buf *b){
 memset(e,0,sizeof(*e));snprintf(e->name,sizeof(e->name),"%s",name);
 e->offset=(uint32_t)ftell(out);e->size=(uint32_t)b->n;e->crc=crc((const unsigned char*)b->s,b->n);
 u32(out,0x04034b50);u16(out,20);u16(out,0);u16(out,0);u16(out,0);u16(out,0);
 u32(out,e->crc);u32(out,e->size);u32(out,e->size);u16(out,(unsigned)strlen(name));u16(out,0);
 fwrite(name,1,strlen(name),out);fwrite(b->s,1,b->n,out);
}
static void finish(FILE *out,Entry *e,int n){
 uint32_t start=(uint32_t)ftell(out);
 for(int i=0;i<n;i++){
  Entry *x=&e[i];u32(out,0x02014b50);u16(out,20);u16(out,20);u16(out,0);u16(out,0);u16(out,0);u16(out,0);
  u32(out,x->crc);u32(out,x->size);u32(out,x->size);u16(out,(unsigned)strlen(x->name));
  u16(out,0);u16(out,0);u16(out,0);u16(out,0);u32(out,0);u32(out,x->offset);
  fwrite(x->name,1,strlen(x->name),out);
 }
 uint32_t end=(uint32_t)ftell(out);u32(out,0x06054b50);u16(out,0);u16(out,0);u16(out,n);u16(out,n);
 u32(out,end-start);u32(out,start);u16(out,0);
}
static void colname(int col,char *s){int j=0;do{s[j++]=(char)('A'+(col%26));col=col/26-1;}while(col>=0 && j<7);s[j]=0;for(int x=0;x<j/2;x++){char t=s[x];s[x]=s[j-1-x];s[j-1-x]=t;}}
static void spreadsheet(Buf *b,const char *filename){
 a(b,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetViews><sheetView workbookViewId=\"0\"/></sheetViews><sheetFormatPr defaultRowHeight=\"17\"/><sheetData>");
 FILE *f=fopen(filename,"rb");if(f){
  char *line=NULL;size_t cap=0;ssize_t n;int row=0;
  while((n=getline(&line,&cap,f))>=0 && row<60000){row++;while(n>0&&(line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;
   char r[40];snprintf(r,sizeof(r),"<row r=\"%d\">",row);a(b,r);
   char *begin=line;int col=0;
   while(col<80){char *tab=strchr(begin,'\t');if(tab)*tab=0;
    char c[16];colname(col++,c);char attr[80];snprintf(attr,sizeof(attr),"<c r=\"%s%d\" t=\"inlineStr\"%s><is><t>",c,row,row==1?" s=\"1\"":"");a(b,attr);
    esc(b,begin);a(b,"</t></is></c>");if(!tab)break;begin=tab+1;
   }
   a(b,"</row>");
  }
  free(line);fclose(f);
 }
 a(b,"</sheetData></worksheet>");
}
int main(int argc,char **argv){
 if(argc<5||argc>12||(argc-2)%2){fprintf(stderr,"usage: xlsx_writer output.xlsx sheetname input.tsv [...]\n");return 2;}
 int sheets=(argc-2)/2;if(sheets>5)return 2;FILE *f=fopen(argv[1],"wb");if(!f){perror(argv[1]);return 1;}
 Entry e[12];int ne=0;Buf b={0};a(&b,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Default Extension=\"xml\" ContentType=\"application/xml\"/><Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/><Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>");
 for(int i=0;i<sheets;i++){char t[200];snprintf(t,sizeof(t),"<Override PartName=\"/xl/worksheets/sheet%d.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>",i+1);a(&b,t);}a(&b,"</Types>");entry(f,&e[ne++],"[Content_Types].xml",&b);free(b.s);b=(Buf){0};
 a(&b,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/></Relationships>");entry(f,&e[ne++],"_rels/.rels",&b);free(b.s);b=(Buf){0};
 a(&b,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheets>");
 for(int i=0;i<sheets;i++){char t[100];snprintf(t,sizeof(t),"<sheet name=\"");a(&b,t);esc(&b,argv[2+i*2]);snprintf(t,sizeof(t),"\" sheetId=\"%d\" r:id=\"rId%d\"/>",i+1,i+1);a(&b,t);}a(&b,"</sheets></workbook>");entry(f,&e[ne++],"xl/workbook.xml",&b);free(b.s);b=(Buf){0};
 a(&b,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">");
 for(int i=0;i<sheets;i++){char t[250];snprintf(t,sizeof(t),"<Relationship Id=\"rId%d\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet%d.xml\"/>",i+1,i+1);a(&b,t);}
 a(&b,"<Relationship Id=\"rId99\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/></Relationships>");entry(f,&e[ne++],"xl/_rels/workbook.xml.rels",&b);free(b.s);b=(Buf){0};
 a(&b,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><fonts count=\"2\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font><font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts><fills count=\"1\"><fill><patternFill patternType=\"none\"/></fill></fills><borders count=\"1\"><border/></borders><cellStyleXfs count=\"1\"><xf fontId=\"0\" fillId=\"0\" borderId=\"0\" numFmtId=\"0\"/></cellStyleXfs><cellXfs count=\"2\"><xf fontId=\"0\" fillId=\"0\" borderId=\"0\" numFmtId=\"0\" xfId=\"0\"/><xf fontId=\"1\" fillId=\"0\" borderId=\"0\" numFmtId=\"0\" xfId=\"0\"/></cellXfs></styleSheet>");entry(f,&e[ne++],"xl/styles.xml",&b);free(b.s);b=(Buf){0};
 for(int i=0;i<sheets;i++){char name[100];snprintf(name,sizeof(name),"xl/worksheets/sheet%d.xml",i+1);spreadsheet(&b,argv[3+i*2]);entry(f,&e[ne++],name,&b);free(b.s);b=(Buf){0};}
 finish(f,e,ne);if(fclose(f)){perror("write xlsx");return 1;}return 0;
}
