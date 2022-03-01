int main()
{
  for(int x = 0; x!=100; x++)
    __CPROVER_loop_invariant(0) // fails base case
  {
    __CPROVER_assert(0, "property 1");
  }

  return 0;
}
