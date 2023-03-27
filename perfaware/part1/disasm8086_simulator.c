typedef struct
{
   short ip; // instruction pointer

   short ax;
   short cx;
   short dx;
   short bx;
   short sp;
   short bp;
   short si;
   short di;

   char memory[1024]; // todo
} Machine;

Machine TheMachine = {0};

short *machine_register_by_name(char *name)
{
   if (strcmp(name, "ax") == 0)
   {
      return &TheMachine.ax;
   }
   else if (strcmp(name, "cx") == 0)
   {
      return &TheMachine.cx;
   }
   else if (strcmp(name, "dx") == 0)
   {
      return &TheMachine.dx;
   }
   else if (strcmp(name, "bx") == 0)
   {
      return &TheMachine.bx;
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
      *r = value;
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
   machine_move_immediate(dest, srcvalue);
}

void machine_print()
{
   printf("\nRegisters:\n");
   printf("\tax: %04x\n", TheMachine.ax);
   printf("\tbx: %04x\n", TheMachine.bx);
   printf("\tcx: %04x\n", TheMachine.cx);
   printf("\tdx: %04x\n", TheMachine.dx);
   printf("\tsp: %04x\n", TheMachine.sp);
   printf("\tbp: %04x\n", TheMachine.bp);
   printf("\tsi: %04x\n", TheMachine.si);
   printf("\tdi: %04x\n\n", TheMachine.di);
}