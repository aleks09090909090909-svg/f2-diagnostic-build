#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fb.h>
static const char *node="/dev/fb0";
int main(int argc,char **argv){
 const char *usb=argc>1?argv[1]:"/mnt/nand_disk";
 char path[512];snprintf(path,sizeof(path),"%s/F2_display_check.txt",usb);
 FILE *out=fopen(path,"w");if(!out)return 1;
 fprintf(out,"F2_DISPLAY_CHECK=1.0.12\nMODE=INSPECT_ONLY\nPASSWORDS_RENDERED=NO\nFRAMEBUFFER_MODIFIED=NO\n");
 int fd=open(node,O_RDONLY|O_CLOEXEC|O_NONBLOCK);
 if(fd<0){fprintf(out,"FRAMEBUFFER_OPEN=FAIL\nERRNO=%d\nERROR=%s\n",errno,strerror(errno));fclose(out);return 0;}
 fprintf(out,"FRAMEBUFFER_OPEN=YES_READ_ONLY\n");
 struct fb_fix_screeninfo fix;struct fb_var_screeninfo var;
 if(ioctl(fd,FBIOGET_FSCREENINFO,&fix)==0){
  fprintf(out,"FB_ID=%.16s\nFB_MEMORY_BYTES=%u\nFB_LINE_LENGTH=%u\nFB_TYPE=%u\nFB_VISUAL=%u\n",fix.id,fix.smem_len,fix.line_length,fix.type,fix.visual);
 }else fprintf(out,"FIX_IOCTL_ERROR=%d\n",errno);
 if(ioctl(fd,FBIOGET_VSCREENINFO,&var)==0){
  fprintf(out,"XRES=%u\nYRES=%u\nXRES_VIRTUAL=%u\nYRES_VIRTUAL=%u\nBPP=%u\nROTATE=%u\nXOFFSET=%u\nYOFFSET=%u\nRED_OFFSET=%u\nRED_LENGTH=%u\nGREEN_OFFSET=%u\nGREEN_LENGTH=%u\nBLUE_OFFSET=%u\nBLUE_LENGTH=%u\n",var.xres,var.yres,var.xres_virtual,var.yres_virtual,var.bits_per_pixel,var.rotate,var.xoffset,var.yoffset,var.red.offset,var.red.length,var.green.offset,var.green.length,var.blue.offset,var.blue.length);
 }else fprintf(out,"VAR_IOCTL_ERROR=%d\n",errno);
 close(fd);fprintf(out,"NOTE=No framebuffer drawing attempted. Qt may repaint concurrently.\n");fclose(out);return 0;
}
