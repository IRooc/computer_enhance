#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;

static char* byteregisters[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
static char* wordregisters[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

int main(int argc, char **argv)
{
   if (argc != 2) {
      printf("Usage: desasm8086.exe <binary-fileinput> ");
      exit(0);
   }
   char* filename = argv[1];

   //get the filesize
   FILE *file = fopen(filename, "r");
   if (file = NULL) {
      printf("could not open file\n");
      exit(errno);
   }
   if (fseek(file, 0, SEEK_END) < 0) {
      printf("could not seek to end of file\n");
      exit(errno);
   }

   long filesize = ftell(file);
   if (filesize < 0) {
      printf("could not get filesize\n");
      exit(errno);
   }
   if (fseek(file, 0, SEEK_SET) < 0) {
      printf("could not set file to beginning");
      exit(errno);
   }

   //read the whole file
   u8* content = (u8*)malloc(filesize*sizeof(u8*));

   fread(content, filesize, 1, file);

   //asm header
   printf("; output from file %s\n\nbits 16\n\n", filename);
   //start the asm output, assume 16 bits so 2 byte jumps
   for(int i = 0; i < filesize; i = i + 2) {
      if (i+1 >= filesize) {
         printf("Only one byte left at the end of the file, stopping early");
         break;
      }
      u8 firstbyte = content[i];
      u8 secondbyte = content[i+1];
      
      u8 opcode = (firstbyte & 0xFC);

      if(opcode == 0x88) { // MOV
         printf("mov ");
         u8 type = (firstbyte & 0x3);

         if (type == 0x01) { //word size
            u8 reg1 = secondbyte & 0x7;
            u8 reg2 = (secondbyte & 0x38) >> 3;
            printf("%s, %s", wordregisters[reg1], wordregisters[reg2]);
            printf(" ; %x\n", secondbyte & 0xff);

         } else if (type == 0) { //byte size
            u8 reg1 = secondbyte & 0x7;
            u8 reg2 = (secondbyte & 0x38) >> 3;
            printf("%s, %s", byteregisters[reg1], byteregisters[reg2]);
            printf(" ; %x\n", secondbyte & 0xff);

         } else {
            printf("; UNKNOWN MOV TYPE %x\n", type);
         }

      } else {
         printf("; UNKNOWN OPCODE %x\n", opcode);
      }
   }
   free(content);
   exit(0);
}

