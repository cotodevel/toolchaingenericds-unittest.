#include "c_regression.h"
#include "c_partial_mock.h"
#include "opmock.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "posixHandleTGDS.h"

/*
 * Captures problem with warning about unexpected call
*/

void testVerifyTGDSPosixFilehandle_fopen_method()
{
  int res = -1;
  FILE * fh = fopen("0:/ToolchainGenericDS-UnitTest.nds", "r");
  if(fh != NULL){
	res = 0;
  }
  fclose(fh);
  OP_ASSERT_EQUAL_INT(0, res);
  // should pass the test
  OP_VERIFY();
}
