default: tests.log

include ../../src/config.inc
include ../../src/common

test:
	@../test.pl -e -p -c '../../../src/cprover/cprover'

tests.log:
	@../test.pl -e -p -c '../../../src/cprover/cprover'

clean:
	$(RM) tests.log
