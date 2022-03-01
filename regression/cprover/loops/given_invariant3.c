int x;

int main()
{
  for(x=0; x<100; x++)
    __CPROVER_loop_invariant(x>=0 && x<=100) // should pass
  {
    // whatnot
  }

  __CPROVER_assert(x == 100, "non-inductive property"); // should pass

  return 0;
}
