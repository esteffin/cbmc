void my_function(int parameter)
  __CPROVER_requires(parameter >= 10)
{
}

int main()
{
  my_function(-123); // fails
  my_function(123); // passes
  return 0;
}
