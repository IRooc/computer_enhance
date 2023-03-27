#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
typedef uint8_t u8;
typedef u8 bool;


#include "disasm8086_simulator.c"

static char *byteregisters[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
static char *wordregisters[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

static char *rmtable[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

static char *arithmatic_opperations[8] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};

typedef enum
{
   IT_BARE,
   IT_SINGLEBYTE,
   IT_SINGLEBYTE_UNSIGNED,
   IT_EXTRABYTE_POSSIBLE_WIDE
} InstructionType;

typedef struct
{
   u8 pattern;
   char *instruction;
   char *printformat;
   InstructionType type;
} SimpleInstruction;

SimpleInstruction jumps[] = {
    // single byte instructutions
    {0b00011111, "pop ds", "pop ds\n", IT_BARE},
    {0b00001110, "push cs", "push cs\n", IT_BARE},
    {0b11010111, "xlat", "xlat\n", IT_BARE},
    {0b10011111, "lahf", "lahf\n", IT_BARE},
    {0b10011110, "sahf", "sahf\n", IT_BARE},
    {0b10011100, "pushf", "pushf\n", IT_BARE},
    {0b10011101, "popf", "popf\n", IT_BARE},
    {0b00110111, "aaa", "aaa\n", IT_BARE},
    {0b00100111, "daa", "daa\n", IT_BARE},
    {0b00111111, "aas", "aas\n", IT_BARE},
    {0b00101111, "das", "das\n", IT_BARE},
    {0b10011000, "cbw", "cbw\n", IT_BARE},
    {0b10011001, "cwd", "cwd\n", IT_BARE},

    // two byte instructutions
    {0b01110100, "jz", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01110101, "jnz", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111100, "jl", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111110, "jlz", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01110010, "jb", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01110110, "jbe", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111010, "jp", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01110000, "jo", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111000, "js", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111101, "jnl", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111111, "jg", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01110011, "jnb", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01110111, "ja", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111011, "jnp", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01110001, "jno", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b01111001, "jns", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b11100010, "loop", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b11100001, "loopz", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b11100000, "loopnz", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b11100011, "jcxz", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b11100011, "jcxz", "%s $+2%+d\n", IT_SINGLEBYTE},
    {0b11100011, "jcxz", "%s $+2%+d\n", IT_SINGLEBYTE},
    
    {0b11010100, "aam", "%s\n", IT_SINGLEBYTE}, //extra byte is ignored
    {0b11010101, "aad", "%s\n", IT_SINGLEBYTE}, //extra byte is ignored

    {0b11101100, "in", "%s al, dx\n", IT_BARE},
    {0b11101101, "in", "%s ax, dx\n", IT_BARE},
    {0b11100100, "in", "%s al, %d\n", IT_SINGLEBYTE_UNSIGNED},
    {0b11100101, "in", "%s ax, %d\n", IT_SINGLEBYTE_UNSIGNED},
    
    {0b11101110, "out", "%s dx, al\n", IT_BARE},
    {0b11101111, "out", "%s dx, ax\n", IT_BARE},
    {0b11100110, "out", "%s %d, al\n", IT_SINGLEBYTE_UNSIGNED},
    {0b11100111, "out", "%s %d, ax\n", IT_SINGLEBYTE_UNSIGNED},

    {0b10100000, "mov", "%s ax, [%d]\n", IT_EXTRABYTE_POSSIBLE_WIDE},
    {0b10100001, "mov", "%s ax, [%d]\n", IT_EXTRABYTE_POSSIBLE_WIDE},
    
    {0b10100010, "mov", "%s [%d], ax\n", IT_EXTRABYTE_POSSIBLE_WIDE},
    {0b10100011, "mov", "%s [%d], ax\n", IT_EXTRABYTE_POSSIBLE_WIDE},
};

// bytestream
u8 *content;

u8 read_byte()
{
   u8 result = content[TheMachine.ip];
   TheMachine.ip += 1;
   return result;
}
// used only on single instances
char displacement[32] = "";
char *read_displacement(u8 mod, u8 rm, signed short *disp)
{
   sprintf(displacement, "");
   *disp = 0;
   if (mod != 0 || (mod == 0 && rm == 0b110))
   {
      u8 disphigh = 0;
      u8 displow = read_byte();

      if (mod == 0b10 || (mod == 0 && rm == 0b110)) // it's 16bit displacement
      {
         disphigh = read_byte();

         *disp = (signed short)((disphigh << 8) + displow);
      }
      else
      {
         *disp = (signed char)displow;
      }
      if (*disp)
      {
         if (mod == 0 && rm == 0b110)
         {
            sprintf(displacement, "%d", *disp);
         }
         else
         {
            if (*disp < 0)
            {
               snprintf(displacement, 32, " - %d", *disp * -1);
            }
            else
            {
               snprintf(displacement, 32, " + %d", *disp);
            }
         }
      }
   }
   return displacement;
}

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
   content = (u8 *)malloc(filesize * sizeof(u8 *));

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
   while (TheMachine.ip < filesize)
   {
      if (TheMachine.ip + 1 >= filesize)
      {
         printf("Only one byte left at the end of the file, stopping early");
         break;
      }
      // decode the 16 bit instruction
      u8 firstbyte = read_byte();

      u8 sw_dw = (firstbyte & 0b11); // wide?
      u8 iswide = sw_dw & 0b1;

      // opcodes
      if (((firstbyte & 0b11111100) == 0b10001000)    // MOV 1000 10xx  reg/mem to/from reg
          || ((firstbyte & 0b11111100) == 0b00111000) // cmp
          || ((firstbyte & 0b11111100) == 0b00101000) // SUB
          || ((firstbyte & 0b11111100) == 0b00011000) // SBB
          || ((firstbyte & 0b11111100) == 0b00010000) // ADC 0001 00xx
          || ((firstbyte & 0b11111100) == 0))         // ADD 0000 00xx
      {
         u8 *operation = "mov";

         if ((firstbyte & 0b10000000) == 0) // if first bit is not set its not a mov
         {
            u8 opp = (firstbyte & 0b00111000) >> 3;
            operation = arithmatic_opperations[opp];
         }
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0xC0) >> 6;
         u8 reg = (secondbyte & 0x38) >> 3;
         u8 rm = secondbyte & 0x7;

         char **regnames = iswide ? wordregisters : byteregisters;

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
            machine_move(regnames[dst], regnames[src]);
         }
         else // mem to reg or reg to mem
         {
            char *memaddr = rmtable[rm];
            signed short disp;
            char *displacement = read_displacement(mod, rm, &disp);

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
         short datalow = read_byte();

         if ((firstbyte & 0b1000) == 0b1000) // wide bit set?
         {
            regnames = wordregisters;
            u8 datahigh = read_byte();

            datalow += (datahigh << 8);
            printf("mov %s, %d\n", regnames[reg2], (signed short)datalow);
            machine_move_immediate(regnames[reg2], datalow);
         }
         else
         {
            printf("mov %s, %d\n", regnames[reg2], (signed char)datalow);
         }
      }
      else if ((firstbyte & 0b11111110) == 0b11000110) // MOV immediate to reg/mem
      {
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0xC0) >> 6;
         u8 rm = secondbyte & 0x7;
         char *memaddr = rmtable[rm];
         char *sizeprefix = iswide ? "word" : "byte";
         signed short disp;
         char *displacement = read_displacement(mod, rm, &disp);

         short data = read_byte();
         if (iswide) // wide?
         {
            short data2 = read_byte();
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
      else if ((firstbyte & 0b111111100) == 0b10000000)     // add immediate to reg/mem {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};
      {
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0xC0) >> 6;
         u8 rm = secondbyte & 0x7;

         u8 opp = (secondbyte & 0b00111000) >> 3;
         char *operation = arithmatic_opperations[opp];

         char *memaddr = rmtable[rm];
         signed short disp = 0;
         if (mod == 0b10 || mod == 0b01)
         {
            signed short displow = read_byte();

            u8 disphigh = 0;
            if (mod == 0b10)
            { // if wide bit is set consume another byte
               disphigh = read_byte();
            }
            disp = (signed short)((disphigh << 8) + displow);
         }
         char displacement[32] = "";
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
         char *sizeprefix = iswide ? "word" : "byte";

         short data = read_byte();

         if (sw_dw == 0b01) // extra byte if sw is 01 only
         {
            short data2 = read_byte();

            data += (data2 << 8);
         }
         if (mod == 0 && rm == 0b110) // exception on the rule so the 2 bytes are the address and after that comes the value
         {
            short data2 = read_byte();

            data += (data2 << 8);

            short value = read_byte();

            printf("%s %s [%d%s], %d\n", operation, sizeprefix, data, displacement, value);
         }
         else if (mod == 0b11)
         {
            memaddr = iswide ? wordregisters[rm] : byteregisters[rm];
            printf("%s %s, %d\n", operation, memaddr, data);
         }
         else
         {
            printf("%s %s [%s%s], %d\n", operation, sizeprefix, memaddr, displacement, data);
         }
      }
      else if (((firstbyte & 0b11111110) == 0b00000100)     // add immediate to accum
               || ((firstbyte & 0b11111110) == 0b00010100)  // adc
               || ((firstbyte & 0b11111110) == 0b00011100)  // sbb
               || ((firstbyte & 0b11111110) == 0b00111100)  // cmp
               || ((firstbyte & 0b11111110) == 0b00101100)) // sub
      {
         u8 opp = (firstbyte & 0b00111000) >> 3;
         char *operation = arithmatic_opperations[opp];

         short memaddr = read_byte();

         if (iswide)
         {
            short secondpart = read_byte();

            memaddr += (secondpart << 8);
            printf("%s ax, %d\n", operation, memaddr);
         }
         else
         {
            signed short imm = (signed short)(char)memaddr;
            printf("%s al, %d\n", operation, imm);
         }
      }
      else if (firstbyte == 0b11111111) // push
      {
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0b11000000) >> 6;
         u8 reg = (secondbyte & 0b00111000) >> 3;
         u8 rm = secondbyte & 0b111;

         char *inst[8] = {"inc", "dec", "call", "call", "jmp", "jmp", "push", "--"}; // stolen from https://gist.github.com/tweetandcode/8aa6e9ce3eee0b19fd9ab0ba3c0085a3 thanks @vbyte
         if (mod == 0b11)
         {
            printf("%s %s\n", inst[reg], wordregisters[rm]);
         }
         else
         {
            char *sizeprefix = iswide ? "word" : "byte";
            signed short disp;
            char *displacement = read_displacement(mod, rm, &disp);
            if (mod == 0 && rm == 0b110)
            {
               printf("%s %s [%d]\n", inst[reg], sizeprefix, disp);
            }
            else
            {
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
               char *memaddr = rmtable[rm];
               printf("%s %s [%s%s]\n", inst[reg], sizeprefix, memaddr, displacement);
            }
         }
      }
      else if (firstbyte == 0b10001111) // pop rm16
      {
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0b11000000) >> 6;
         u8 reg = (secondbyte & 0b00111000) >> 3;
         u8 rm = secondbyte & 0b111;

         if (mod == 3)
            printf("pop %s\n", wordregisters[rm]);
         else
         {
            char *sizeprefix = iswide ? "word" : "byte";
            signed short disp;
            char *displacement = read_displacement(mod, rm, &disp);
            if (mod == 0 && rm == 0b110)
            {
               printf("%s %s [%d]\n", "pop", sizeprefix, disp);
            }
            else
            {
               char *memaddr = rmtable[rm];
               printf("%s %s [%s%s]\n", "pop", sizeprefix, memaddr, displacement);
            }
         }
      }
      else if ((firstbyte & 0b11110000) == 0b01010000) // push/pop register
      {
         u8 reg = firstbyte & 0b111;
         u8 pop = (firstbyte >> 3) & 0b1;
         printf("%s %s\n", pop ? "pop" : "push", wordregisters[reg]);
      }
      else if ((firstbyte & 0b11111110) == 0b10000110) // xchg
      {
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0b11000000) >> 6;
         u8 reg = (secondbyte & 0b00111000) >> 3;
         u8 rm = secondbyte & 0b111;

         char **memaddr = iswide ? wordregisters : byteregisters;
         if (mod == 0b11)
         {
            printf("xchg %s, %s\n", memaddr[reg], memaddr[rm]);
         }
         else
         {
            signed short disp;
            char *displacement = read_displacement(mod, rm, &disp);
            char *rr = rmtable[rm];
            printf("xchg %s, [%s%s];TODO check order\n", memaddr[reg], rr, displacement);
         }
      }
      else if ((firstbyte & 0b11111100) == 0b11010000) // shifts:
      {

         int v = (firstbyte >> 1) & 0x1;
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0b11000000) >> 6;
         u8 reg = (secondbyte & 0b00111000) >> 3;
         u8 rm = secondbyte & 0b111;

         char *opp[8] = {"rol", "ror", "rcl", "rcr", "shl", "shr", "--", "sar"};
         char *source[2] = {"1", "cl"};

         char **memaddr = iswide ? wordregisters : byteregisters;
         if (mod == 0b11)
         {
            printf("%s %s, %s\n", opp[reg], memaddr[rm], source[v]);
         }
         else
         {
            signed short disp;
            char *displacement = read_displacement(mod, rm, &disp);
            char *sizeprefix = iswide ? "word" : "byte";
            char *rr = rmtable[rm];
            if (mod == 0 && rm == 0b110)
            {
               rr = "";
            }
            printf("%s %s [%s%s], %s\n", opp[reg], sizeprefix, rr, displacement, source[v]);
         }
      }
      else if ((firstbyte & 0b11111000) == 0b10010000) // xchg to accum
      {
         u8 reg = firstbyte & 0b111;
         printf("xchg ax, %s\n", wordregisters[reg]);
      }
      else if (((firstbyte & 0b11111000) == 0b01000000)     // inc register
               || ((firstbyte & 0b11111000) == 0b01001000)) // dec
      {
         u8 reg = firstbyte & 0b111;
         char *opp = (firstbyte & 0b00001000) ? "dec" : "inc";
         printf("%s %s\n", opp, wordregisters[reg]);
      }
      else if (((firstbyte & 0b11111110) == 0b11111110)     // inc // dec
               || ((firstbyte & 0b11111110) == 0b11110110)) // neg
      {
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0b11000000) >> 6;
         u8 reg = (secondbyte & 0b00111000) >> 3;
         u8 rm = secondbyte & 0b111;

         char *inst[8] = {"inc", "dec", "not", "neg", "mul", "imul", "div", "idiv"};
         char *opp = inst[reg];
         char **memaddr = iswide ? wordregisters : byteregisters;
         if (mod == 0b11)
         {
            printf("%s %s\n", opp, memaddr[rm]);
         }
         else
         {
            char *sizeprefix = iswide ? "word" : "byte";

            if (mod == 0 && rm == 0b110)
            {
               u8 low = read_byte();
               u8 high = read_byte();

               short rr = (high << 8) + low;
               printf("%s %s [%d]\n", opp, sizeprefix, rr);
            }
            else
            {
               signed short disp;
               char *displacement = read_displacement(mod, rm, &disp);
               char *rr = rmtable[rm];
               printf("%s %s [%s%s]\n", opp, sizeprefix, rr, displacement);
            }
         }
      }
      else if ((firstbyte == 0b10001101) // lea, lds, les
               || (firstbyte == 0b11000101) || (firstbyte == 0b11000100))
      {
         u8 secondbyte = read_byte();

         u8 mod = (secondbyte & 0b11000000) >> 6;
         u8 reg = (secondbyte & 0b00111000) >> 3;
         u8 rm = secondbyte & 0b111;

         char *opp = (firstbyte == 0b11000100) ? "les" : (firstbyte == 0b11000101) ? "lds"
                                                                                   : "lea";

         char **memaddr = wordregisters;
         if (mod == 0b11)
         {
            printf("xchg %s, %s\n", memaddr[reg], memaddr[rm]);
         }
         else
         {
            signed short disp;
            char *displacement = read_displacement(mod, rm, &disp);
            char *rr = rmtable[rm];
            printf("%s %s, [%s%s]\n", opp, memaddr[reg], rr, displacement);
         }
      }
      else if ((firstbyte & 0b11111100) == 0b00100000) // AND r/m with reg)
      {
         u8 secondbyte = read_byte();
         u8 mod = (secondbyte & 0b11000000) >> 6;
         u8 reg = (secondbyte & 0b00111000) >> 3;
         u8 rm = secondbyte & 0b111;

         char *opp = "and";

         char **memaddr = iswide ? wordregisters : byteregisters;
         if (mod == 0b11)
         {
            char *dst = memaddr[rm];
            char *src = memaddr[reg];
            if (sw_dw & 0b10) // if the D bit is set swap dst and src;
            {
               char *t = dst;
               dst = src;
               src = t;
            }
            printf("%s %s, %s\n", opp, dst, src);
         }
         else
         {
            signed short disp;
            char *displacement = read_displacement(mod, rm, &disp);
            char *rr = rmtable[rm];
            if (mod == 0 && rm == 0b110)
            {
               rr = "";
            }
            if (sw_dw & 0b10) // if the D bit is set swap dst and src;
            {
               printf("%s %s, [%s%s]\n", opp, memaddr[reg], rr, displacement);
            }
            else
            {
               printf("%s [%s%s], %s\n", opp, rr, displacement, memaddr[reg]);
            }
         }
      }
      else if ((firstbyte & 0b11111110) == 0b00100100) // AND immediate to accumulator
      {
         short data = read_byte();
         if (iswide)
         {
            u8 datahigh = read_byte();
            data += (datahigh << 8);
         }
         char *dst = iswide ? "ax" : "al";
         printf("and %s, %d\n", dst, data);
      }
      else
      {
         u8 handled = 0;
         // simple instructions
         for (int i = 0; i < ArrayCount(jumps); i++)
         {
            SimpleInstruction instr = jumps[i];
            if (instr.pattern == firstbyte)
            {
               if (instr.type == IT_BARE)
               {
                  printf(instr.printformat, instr.instruction);
                  handled = 1;
                  break;
               }
               else if (instr.type == IT_SINGLEBYTE)
               {
                  signed char data = read_byte();
                  printf(instr.printformat, instr.instruction, data);
                  handled = 1;
                  break;
               }
               else if (instr.type == IT_SINGLEBYTE_UNSIGNED)
               {
                  u8 data = read_byte();
                  printf(instr.printformat, instr.instruction, data);
                  handled = 1;
                  break;
               }
               else if (instr.type == IT_EXTRABYTE_POSSIBLE_WIDE)
               {
                  short data = read_byte();
                  if (iswide)
                  {
                     short secondpart = read_byte();
                     data += (secondpart << 8);
                  }
                  printf(instr.printformat, instr.instruction, data);
                  handled = 1;
                  break;
               }
               else
               {
                  printf("ERROR: UNKNONW INstruction Type %x", instr.type);
               }
            }
         }

         if (!handled)
         {
            printf("; UNKNOWN OPCODE %x\n", firstbyte);
            free(content);
            exit(1);
         }
      }
   }
   machine_print();
   free(content);
   exit(0);
}
