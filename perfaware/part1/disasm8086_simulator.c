typedef struct
{
   short ip; // instruction pointer

   short ax;
   short bx;
   short cx;
   short dx;
   short sp;
   short bp;
   short si;
   short di;

   // segment registers
   short es;
   short ss;
   short ds;

   short cs; // unused?

   char memory[1024]; // todo
} Machine;

Machine TheMachine = {0};

short *machine_register_by_name(char *name)
{
   if (strcmp(name, "ax") == 0 || strcmp(name, "ah") == 0 || strcmp(name, "al") == 0)
   {
      return &TheMachine.ax;
   }
   else if (strcmp(name, "bx") == 0 || strcmp(name, "bh") == 0 || strcmp(name, "bl") == 0)
   {
      return &TheMachine.bx;
   }
   else if (strcmp(name, "cx") == 0 || strcmp(name, "ch") == 0 || strcmp(name, "cl") == 0)
   {
      return &TheMachine.cx;
   }
   else if (strcmp(name, "dx") == 0 || strcmp(name, "dh") == 0 || strcmp(name, "dl") == 0)
   {
      return &TheMachine.dx;
   }
   else if (strcmp(name, "sp") == 0)
   {
      return &TheMachine.sp;
   }
   else if (strcmp(name, "bp") == 0)
   {
      return &TheMachine.bp;
   }
   else if (strcmp(name, "si") == 0)
   {
      return &TheMachine.si;
   }
   else if (strcmp(name, "di") == 0)
   {
      return &TheMachine.di;
   }
   else if (strcmp(name, "es") == 0)
   {
      return &TheMachine.es;
   }
   else if (strcmp(name, "cs") == 0)
   {
      return &TheMachine.cs;
   }
   else if (strcmp(name, "ss") == 0)
   {
      return &TheMachine.ss;
   }
   else if (strcmp(name, "ds") == 0)
   {
      return &TheMachine.ds;
   }
   else
   {
      printf("ERROR: UNKNOWN register %s", name);
      exit(1);
   }
   return 0;
}

void machine_move_immediate(char *reg, short value)
{
   short *r = machine_register_by_name(reg);
   if (r)
   {
      if (reg[1] == 'l')
      {
         // low byte only
         *r = (*r & 0b1111111100000000) | (value & 0b11111111);
      }
      else if (reg[1] == 'h')
      {
         // high byte only
         *r = (*r & 0b0000000011111111) | ((value & 0b11111111) << 8);
      }
      else
      {
         *r = value;
      }
   }
   else
   {
      printf("ERROR: UNKNOWN REG %s", reg);
      exit(1);
   }
}
void machine_move(char *dest, char *src)
{
   short srcvalue = *machine_register_by_name(src);
   if (src[1] == 'l')
   {
      srcvalue = (srcvalue & 0b11111111);
   }
   else if (src[1] == 'h')
   {
      srcvalue = (srcvalue & 0b1111111100000000) >> 8;
   }
   machine_move_immediate(dest, srcvalue);
}

void machine_print()
{
   printf("\nRegisters:\n");
   printf("\tax: 0x%04x\n", TheMachine.ax);
   printf("\tbx: 0x%04x\n", TheMachine.bx);
   printf("\tcx: 0x%04x\n", TheMachine.cx);
   printf("\tdx: 0x%04x\n", TheMachine.dx);
   printf("\tsp: 0x%04x\n", TheMachine.sp);
   printf("\tbp: 0x%04x\n", TheMachine.bp);
   printf("\tsi: 0x%04x\n", TheMachine.si);
   printf("\tdi: 0x%04x\n\n", TheMachine.di);
}