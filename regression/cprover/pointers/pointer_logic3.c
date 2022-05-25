int main()
{
  char *base, *p;
  __CPROVER_size_t size, offset;

  __CPROVER_assume(__CPROVER_r_ok(base, size));
  __CPROVER_assume(offset < size);

  p = base + offset;

  __CPROVER_assert(base <= p, "property 1"); // should pass
  __CPROVER_assert(p < base + size , "property 2"); // should pass

  return 0;
}
