int main()
{
  for (;;)
    asm volatile ("nop" ::: "memory");
}
