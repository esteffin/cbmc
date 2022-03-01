#define N 100

int main()
{
  int array[N];

  for(int i = 0; i != N; i++)
    __CPROVER_loop_invariant(i >= 0 && i <= N) // passes
  {
    array[i] = 0; // safe and passes
  }

  return 0;
}
