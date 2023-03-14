#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;

static char *byteregisters[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
static char *wordregisters[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

static char *rmtable[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

int main(int argc, char **argv)
{
   if (argc != 2)
   {
      printf("Usage: desasm8086.exe <binary-fileinput> ");
      exit(0);
   }
   char *filename = argv[1];

   // get the filesize
   FILE *file = fopen(filename, "r");
   if (file == NULL)
   {
      printf("could not open file\n");
      exit(errno);
   }
   if (fseek(file, 0, SEEK_END) < 0)
   {
      printf("could not seek to end of file\n");
      exit(errno);
   }

   long filesize = ftell(file);
   if (filesize < 0)
   {
      printf("could not get filesize\n");
      exit(errno);
   }
   if (fseek(file, 0, SEEK_SET) < 0)
   {
      printf("could not set file to beginning");
      exit(errno);
   }

   // read the whole file
   u8 *content = (u8 *)malloc(filesize * sizeof(u8 *));

   long readsize = fread(content, 1, filesize, file);
   if (readsize != filesize)
   {
      printf("could not read the full file");
      exit(1);
   }
   fclose(file);

   // asm header
   printf("; output from file %s\n\nbits 16\n\n", filename);

   // start the asm output, assume 16 bits so 2 byte jumps
   int i = 0;
   while (i < filesize)
   {
      if (i + 1 >= filesize)
      {
         printf("Only one byte left at the end of the file, stopping early");
         break;
      }
      // decode the 16 bit instruction
      u8 firstbyte = content[i];
      i += 1; // consumed a byte

      u8 sw_dw = (firstbyte & 0b11); // wide?
      u8 iswide = sw_dw & 0b1;

      // opcodes
      if (((firstbyte & 0b11111100) == 0b10001000)    // MOV 1000 10xx  reg/mem to/from reg
          || ((firstbyte & 0b11111100) == 0b00111000) // cmp 
          || ((firstbyte & 0b11111100) == 0b00101000) // SUB
          || ((firstbyte & 0b11111100) == 0))         // ADD 0000 00xx
      {
         u8 *operation = "mov";
         if ((firstbyte & 0b11111100) == 0)
         {
            operation = "add";
         }
         else if ((firstbyte & 0b11111100) == 0b00101000)
         {
            operation = "sub";
         }
         else if ((firstbyte & 0b11111100) == 0b00111000)
         {
            operation = "cmp";
         }
         u8 secondbyte = content[i];
         i += 1; // consumed a byte
         u8 mod = (secondbyte & 0xC0) >> 6;
         u8 reg = (secondbyte & 0x38) >> 3;
         u8 rm = secondbyte & 0x7;

         char **regnames = byteregisters;
         if (iswide)
         {
            regnames = wordregisters;
         }

         if (mod == 0b11) // register to register
         {
            u8 dst = rm;
            u8 src = reg;
            if (sw_dw & 0b10) // if the D bit is set swap dst and src;
            {
               dst = reg;
               src = rm;
            }
            printf("%s %s, %s\n", operation, regnames[dst], regnames[src]);
         }
         else // mem to reg or reg to mem
         {
            char *memaddr = rmtable[rm];
            char displacement[32] = "";
            signed short disp = 0;
            if (mod == 1) // 8 bit displacement
            {
               u8 displow = content[i];
               i += 1; // consumed a byte
               if (displow)
               {
                  signed char disp = (signed char)displow;
                  if (disp < 0)
                  {
                     snprintf(displacement, 32, " - %d", disp * -1);
                  }
                  else
                  {
                     snprintf(displacement, 32, " + %d", disp);
                  }
               }
            }
            else if (mod == 0b10 || (mod == 0 && rm == 0b110)) // 16 bit displacement
            {
               u8 displow = content[i];
               i += 1; // consumed a byte
               u8 disphigh = content[i];
               i += 1; // consumed a byte
               disp = (signed short)((disphigh << 8) + displow);
               if (disp)
               {
                  if (disp < 0)
                  {
                     disp *= -1;
                     snprintf(displacement, 32, " - %d", disp);
                  }
                  else
                  {
                     snprintf(displacement, 32, " + %d", disp);
                  }
               }
            }

            if (sw_dw & 0b10) // is the destination a registry
            {
               if (rm == 0b110 && mod == 0) // direct address so not based of memaddr
               {
                  printf("%s %s, [%d]\n", operation, regnames[reg], disp);
               }
               else
               {
                  printf("%s %s, [%s%s]\n", operation, regnames[reg], memaddr, displacement);
               }
            }
            else
            {
               printf("%s [%s%s], %s\n", operation, memaddr, displacement, regnames[reg]);
            }
         }
      }
      else if ((firstbyte & 0b11110000) == 0b10110000) // MOV immediate to register
      {
         u8 reg2 = firstbyte & 7;
         char **regnames = byteregisters;
         short datalow = content[i];
         i += 1;                             // consumed a byte
         if ((firstbyte & 0b1000) == 0b1000) // wide bit set?
         {
            regnames = wordregisters;
            u8 datahigh = content[i];
            i += 1; // consumed a byte
            datalow += (datahigh << 8);
            printf("mov %s, %d\n", regnames[reg2], (signed short)datalow);
         }
         else
         {
            printf("mov %s, %d\n", regnames[reg2], (signed char)datalow);
         }
      }
      else if ((firstbyte & 0b11111110) == 0b11000110) // MOV immediate to reg/mem
      {
         u8 secondbyte = content[i];
         i += 1; // consumed a byte
         u8 mod = (secondbyte & 0xC0) >> 6;
         u8 rm = secondbyte & 0x7;
         char *memaddr = rmtable[rm];
         char displacement[32] = "";
         char *sizeprefix = "byte";
         signed short disp = 0;
         if (mod == 0b10 || mod == 0b01)
         {
            u8 displow = content[i];
            i += 1; // consumed a byte
            u8 disphigh = 0;
            if (iswide)
            { // if wide bit is set consume another byte
               disphigh = content[i];
               i += 1; // consumed a byte
               sizeprefix = "word";
            }
            disp = (signed short)((disphigh << 8) + displow);
         }
         if (disp && mod) // write displacement suffix
         {
            if (disp < 0)
            {
               disp *= -1;
               snprintf(displacement, 32, " - %d", disp);
            }
            else
            {
               snprintf(displacement, 32, " + %d", disp);
            }
         }
         short data = content[i];
         i += 1;     // consumed a byte
         if (iswide) // wide?
         {
            short data2 = content[i];
            i += 1; // consumed a byte
            data += (data2 << 8);
         }

         if (mod) // immediate to memory read the data bytes
         {
            printf("mov [%s%s], %s %d\n", memaddr, displacement, sizeprefix, data);
         }
         else // immediate to registry
         {
            printf("mov [%s], %s %d\n", memaddr, sizeprefix, data);
         }
      }
      else if (((firstbyte & 0b111111100) == 0b10000000)     // add immediate to reg/mem
               || ((firstbyte & 0b111111100) == 0b10000000)) // sub
      {
         u8 isreg = 0;
         u8 secondbyte = content[i];
         i += 1; // consumed a byte
         u8 mod = (secondbyte & 0xC0) >> 6;
         char *operation = "add";
         u8 oper = (secondbyte & 0b00111000) >> 3;
         if (oper == 0b101)
         {
            operation = "sub";
         }
         else if (oper == 0b111)
         {
            operation = "cmp";
         }
         u8 rm = secondbyte & 0x7;
         char *memaddr = rmtable[rm];
         char displacement[32] = "";
         char *sizeprefix = "byte";
         signed short disp = 0;
         if (mod == 0b11)
         {
            isreg = 1;
            // dest is reg
            memaddr = iswide ? wordregisters[rm] : byteregisters[rm];
         }
         else if (mod == 0b10 || mod == 0b01)
         {
            u8 displow = content[i];
            i += 1; // consumed a byte
            u8 disphigh = 0;
            if (iswide)
            { // if wide bit is set consume another byte
               disphigh = content[i];
               i += 1; // consumed a byte
               sizeprefix = "word";
            }
            disp = (signed short)((disphigh << 8) + displow);
         }
         if (disp && mod) // write displacement suffix
         {
            if (disp < 0)
            {
               disp *= -1;
               snprintf(displacement, 32, " - %d", disp);
            }
            else
            {
               snprintf(displacement, 32, " + %d", disp);
            }
         }
         short data = content[i];
         i += 1;            // consumed a byte
         if (sw_dw == 0b01) // extra byte?
         {
            short data2 = content[i];
            i += 1; // consumed a byte
            data += (data2 << 8);
         }
         if (mod == 0 && rm == 0b110) //exception on the rule so the 2 bytes are the address and after that comes the value
         {
            short data2 = content[i];
            i += 1; // consumed a byte
            data += (data2 << 8);

            short value = content[i];
            i += 1; 
            printf("%s %s [%d%s], %d\n", operation, sizeprefix, data, displacement, value);
         }
         else if (mod == 0b11)
         {
            printf("%s %s, %d\n", operation, memaddr, data);
         }
         else
         {
            printf("%s %s [%s%s], %d\n", operation, sizeprefix, memaddr, displacement, data);
         }
      }
      else if ((firstbyte & 0b11111110) == 0b10100000) // mem to accum
      {
         short memaddr = content[i];
         i += 1; // consumed a byte
         if (iswide)
         {
            short secondpart = content[i];
            i += 1; // consumed a byte
            memaddr += (secondpart << 8);
         }
         printf("mov ax, [%d]\n", memaddr);
      }
      else if ((firstbyte & 0b11111110) == 0b10100010) // accum to mem
      {
         short memaddr = content[i];
         i += 1; // consumed a byte
         if (iswide)
         {
            short secondpart = content[i];
            i += 1; // consumed a byte
            memaddr += (secondpart << 8);
         }
         printf("mov [%d], ax\n", memaddr);
      }
      else if (((firstbyte & 0b11111110) == 0b00000100)     // add immediate to accum
               || ((firstbyte & 0b11111110) == 0b00111100)  //cmp
               || ((firstbyte & 0b11111110) == 0b00101100)) //sub
      {
         char *operation = "add";
         if ((firstbyte & 0b00111110) == 0b00101100)
         {
            operation = "sub";
         } else if ((firstbyte & 0b00111110) == 0b00111100)
         {
            operation = "cmp";
         }
         short memaddr = content[i];
         i += 1; // consumed a byte
         if (iswide)
         {
            short secondpart = content[i];
            i += 1; // consumed a byte
            memaddr += (secondpart << 8);
            printf("%s ax, %d\n", operation, memaddr);
         }
         else
         {
            signed short imm = (signed short)(char)memaddr;
            printf("%s al, %d\n", operation, imm);
         }
      }
      else if (firstbyte == 0b01110100) //jz/je
      {
         signed char data = content[i];
         i += 1; // consumed a byte
         printf("jz $+2%+d\n", data);
      }
      else if (firstbyte == 0b01110101) //jnz/jne
      {
         signed char data = content[i];
         i += 1; // consumed a byte
         printf("jnz $+2%+d\n", data);
      } else if (firstbyte == 0b01111100) //jnz/jne
      {
         signed char data = content[i];
         i += 1; // consumed a byte
         printf("jl $+2%+d\n", data);
      }      
      else
      {
         printf("; UNKNOWN OPCODE %x\n", firstbyte);
         i += 1; // just skip to next byte
      }
   }
   free(content);
   exit(0);
}
