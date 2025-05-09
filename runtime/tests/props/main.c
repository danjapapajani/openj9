/*******************************************************************************
 * Copyright IBM Corp. and others 2001
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "j9port.h"
#include "CuTest.h"
#include "exelib_api.h"
#include <string.h>
#include "omrformatconsts.h"

extern CuSuite *GetTestSuite(void);

J9PortLibrary* g_PortLibrary;

UDATA RunAllTests(J9PortLibrary *portLibrary)
{
	UDATA start, end;
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();
	PORT_ACCESS_FROM_PORT(portLibrary);

	CuSuiteAddSuite(suite, GetTestSuite());
 
	start = j9time_usec_clock();
	CuSuiteRun(suite);
	end = j9time_usec_clock();

	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);

	printf("%s\n", output->buffer);
	printf("Tests took %" OMR_PRIuPTR " usec to run.\n", end - start);

	if (suite->failCount == 0){
		return 0;
	} else {
		return 1;
	}
}

UDATA
signalProtectedMain(struct J9PortLibrary *portLibrary, void *arg)
{
	struct j9cmdlineOptions * startupOptions = (struct j9cmdlineOptions *) arg;
	PORT_ACCESS_FROM_PORT(portLibrary);
	
	g_PortLibrary = portLibrary;	

#if defined(J9VM_OPT_MEMORY_CHECK_SUPPORT)
	/* This should happen before anybody allocates memory!  Otherwise, shutdown will not work properly. */
	memoryCheck_parseCmdLine( PORTLIB, startupOptions->argc-1, startupOptions->argv );
#endif /* J9VM_OPT_MEMORY_CHECK_SUPPORT */

	cutest_parseCmdLine( PORTLIB, startupOptions->argc-1, startupOptions->argv);

	return RunAllTests(portLibrary);
}
