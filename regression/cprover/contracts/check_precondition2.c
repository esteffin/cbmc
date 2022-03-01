void callee(int parameter)
  __CPROVER_requires(parameter >= 10)
{
}

void caller(int some_value)
  __CPROVER_requires(1)
{
  if(some_value >= 10)
    callee(some_value); // precondition passes
}
