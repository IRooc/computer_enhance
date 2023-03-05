#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;

static char* byteregisters[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
static char* wordregisters[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

static char* rmtable[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

int main(int argc, char **argv)  
{
   if (argc != 2) {
      printf("Usage: desasm8086.exe <binary-fileinput> ");
      exit(0);
   }
   char* filename = argv[1];

   //get the filesize
   FILE *file = fopen(filename, "r");
   if (file == NULL) {
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
   int i = 0;
   while(i < filesize) {
      if (i+1 >= filesize) {
         printf("Only one byte left at the end of the file, stopping early");
         break;
      }
      //decode the 16 bit instruction
      u8 firstbyte = content[i];
      i += 1; //consumed a byte
      
      u8 firsta = (firstbyte & 0xF0) >> 4;
      u8 firstb = (firstbyte & 0xF); 

      u8 dw = (firstbyte & 0x3);


      //opcodes
      if ((firsta == 0x8) && ((firstb & 0xC) == 0x8)) { // MOV 1000 10xx  reg/mem to/from reg
         u8 secondbyte = content[i];
         i += 1;//consumed a byte
         u8 mod = (secondbyte & 0xC0) >> 6;
         u8 reg = (secondbyte & 0x38) >> 3;
         u8 rm = secondbyte & 0x7;

         u8 dst = rm;
         u8 src = reg;

         if (dw & 0x2) { //if the D bit is set swap dst and src;
            dst = reg;
            src = rm;
         }

         char** regnames = byteregisters;
         if (dw & 0x1) { //if the W bit is set use the word size registers
            regnames = wordregisters;
         } 

         if (mod == 0x3) {
            printf("mov ");
            printf("%s, %s", regnames[dst], regnames[src]);
            printf(" ; %x\n", secondbyte & 0xff);
         } else {
            char* memaddr = rmtable[rm];
            char displacement[32] = "";
            if (mod == 1) {
               u8 lowaddr = content[i];
               i += 1;//consumed a byte
               if (lowaddr)
                  snprintf(displacement, 32, " + %d", lowaddr);
            } else if (mod == 2) {
               u8 lowaddr = content[i];
               i += 1;//consumed a byte
               u8 highaddr = content[i];
               i += 1;//consumed a byte
               int disp = ((highaddr<<8) + lowaddr);
               if (disp)
                  snprintf(displacement, 32, " + %d", disp);
            }
            
            if (dw & 0x2) {
               printf("mov %s, [%s%s]; rm = %x\n", regnames[reg], memaddr, displacement, rm);
            } else {
               printf("mov [%s%s], %s; rm = %x\n", memaddr, displacement, regnames[reg], rm);
            }
         } 
      } else if (firsta == 0xB) {
         u8 reg2 = firstb & 7;
         int datalow = content[i];
         char** regnames = byteregisters;
         i += 1; //consumed a byte
         if ((firstb & 8) == 8) {
            u8 datahigh = content[i];
            i += 1; //consumed a byte
            datalow += (datahigh<<8);
            regnames = wordregisters;
         }
         printf("mov %s, %d\n", regnames[reg2], datalow);
      } else {
         printf("; UNKNOWN OPCODE %x %x\n", firsta, firstb);
         i += 1; //just skip to next byt
      }
   }
   free(content);
   exit(0);
}

