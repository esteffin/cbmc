int global;

void my_function1(int parameter)
  __CPROVER_ensures(global == parameter) // passes
  __CPROVER_assigns(global)
{
  parameter++;
  global = parameter;
}

void my_function2(int parameter)
  __CPROVER_ensures(global == parameter) // fails
  __CPROVER_assigns(global)
{
  global = parameter;
  parameter++;
}
