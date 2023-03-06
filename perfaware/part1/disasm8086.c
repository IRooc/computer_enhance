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
      if ((firsta == 0b1000) && ((firstb & 0b1100) == 0b1000)) { // MOV 1000 10xx  reg/mem to/from reg
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
         if (dw & 0b1) { //if the W bit is set use the word size registers
            regnames = wordregisters;
         } 

         if (mod == 0b11) { //register to register
            printf("mov %s, %s\n", regnames[dst], regnames[src]);
         } else { //mem to reg or reg to mem
            char* memaddr = rmtable[rm];
            char displacement[32] = "";
            signed short disp =0;
            if (mod == 1) {
               u8 displow = content[i];
               i += 1;//consumed a byte
               if (displow) {
                  signed char disp = (signed char)displow;
                  if (disp < 0) {
                     snprintf(displacement, 32, " - %d", disp * -1);
                  } else {
                     snprintf(displacement, 32, " + %d", disp);
                  }
               }
            } else if (mod == 0b10 || (mod == 0 && rm == 0b110)) {
               u8 displow = content[i];
               i += 1;//consumed a byte
               u8 disphigh = content[i];
               i += 1;//consumed a byte
               disp = (signed short)((disphigh<<8) + displow);
               if (disp){
                  if (disp < 0 ) {
                     disp *= -1;
                     snprintf(displacement, 32, " - %d", disp);
                  }else {
                     snprintf(displacement, 32, " + %d", disp);                     
                  }
               }
            }
            
            if (dw & 0b10) {
               if (rm == 0b110 && mod == 0) { //direct address so not based of memaddr
                  printf("mov %s, [%d]\n", regnames[reg], disp);
               } else {
                  printf("mov %s, [%s%s]\n", regnames[reg], memaddr, displacement);
               }
            } else {
               printf("mov [%s%s], %s\n", memaddr, displacement, regnames[reg]);
            }
         } 
      } else if (firsta == 0b1011) { //1011 immediate to register
         u8 reg2 = firstb & 7;
         char** regnames = byteregisters;
         int datalow = content[i];
         i += 1; //consumed a byte
         if ((firstb & 8) == 8) {
            u8 datahigh = content[i];
            i += 1; //consumed a byte
            datalow += (datahigh<<8);
            regnames = wordregisters;
         }
         printf("mov %s, %d\n", regnames[reg2], (signed short)datalow);
      } else if (firsta == 0b1100 && ((firstb & 0b1110) == 0b0110)) { //1100 011 immediate to reg/mem
         u8 iswide = (firstb & 1 == 1);
         u8 secondbyte = content[i];
         i += 1;//consumed a byte
         u8 mod = (secondbyte & 0xC0) >> 6;
         u8 rm = secondbyte & 0x7;
         char* memaddr = rmtable[rm];
         char displacement[32] = "";
         u8 displow = content[i];
         i += 1;//consumed a byte
         u8 disphigh = 0;
         char* sizeprefix = "byte";
         if (iswide) { //if wide bit is set consume another byte
            disphigh = content[i];
            i += 1;//consumed a byte
            sizeprefix = "word";
         }
         int disp = (signed short)((disphigh<<8) + displow);
         
         if (disp && mod){
            if (disp < 0 ) {
               disp *= -1;
               snprintf(displacement, 32, " - %d", disp);
            }else {
               snprintf(displacement, 32, " + %d", disp);                     
            }
         }
         if (mod) { //immediate to memory read the data bytes
            int data = content[i];
            i += 1;//consumed a byte
            if (iswide) {
               int data2 = content[i];
               i += 1;//consumed a byte
               data += (data2<<8);
            }
            printf("mov [%s%s], %s %d\n", memaddr, displacement, sizeprefix, data);
         } else { //immediate to registry
            printf("mov [%s], %s %d\n", memaddr, sizeprefix, disp);
         }
      } else if (firsta == 0b1010 && (firstb & 0b1110) == 0) { //mem to accum
         int memaddr = content[i];
         i += 1;//consumed a byte
         if (firstb & 1) {
            short secondpart = content[i];
            i += 1;//consumed a byte
            memaddr += (secondpart << 8);
         }
         printf("mov ax, [%d]\n", memaddr);
      } else if (firsta == 0b1010 && (firstb & 0b1110) == 0b0010) { //accum to mem
         int memaddr = content[i];
         i += 1;//consumed a byte
         if (firstb & 1) {
            short secondpart = content[i];
            i += 1;//consumed a byte
            memaddr += (secondpart << 8);
         }
         printf("mov [%d], ax\n", memaddr);
      } else {
         printf("; UNKNOWN OPCODE %x %x\n", firsta, firstb);
         i += 1; //just skip to next byt
      }
   }
   free(content);
   exit(0);
}

