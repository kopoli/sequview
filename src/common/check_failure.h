#ifndef CHECK_FAILURE_HEADER
#define CHECK_FAILURE_HEADER

#include "commonconf.h"

#include <errno.h>
#include <string.h>

#include "iolet.h"

/*****************************************************************************/
/*
  Currently defines the following macros:

  CHECK_FAILURE                   (assertion,failvalue,retvalue)
  CHECK_FAILURE_ACT               (assertion,failvalue,action)
  CHECK_FAILURE_WITH_ERRNO        (assertion,failvalue,retvalue)
  CHECK_FAILURE_WITH_ERRNO_ACT    (assertion,failvalue,action)
  CHECK_FAILURE_WITH_EXCUSE       (assertion,failvalue,retvalue,excuse)
  CHECK_FAILURE_WITH_EXCUSE_ACT   (assertion,failvalue,action,excuse)
  CHECK_FAILURE_WARNING           (assertion,failvalue,excuse)

*/

#if 1
/* control. Define these somewhere else */
#define ASSERTION_REPORT          1 /* if reporting of assertions is used */
#define CHECK_FAILURE_PRINT_PLACE 1 /* should the sourcefile (and place be 
                                       printed) */
#endif

/*****************************************************************************/

#define CHECK_FAILURE_BAREBONES(asserteval,action) \
  do { \
    if(asserteval) \
    { \
      action \
    } \
  } while(0)

#define CHECK_FAILURE_NORMAL_ASSERTION_EVALUATION(assertion,failvalue) \
  ((assertion) == (failvalue))


#ifdef ASSERTION_REPORT

/*****************************************************************************/
/* configuration of reporting */

#define CHECK_FAILURE_REPORT_FUNCTION   print_err

#define CHECK_FAILURE_STRING_ERROR      "Error: "
#define CHECK_FAILURE_STRING_WARNING    "Warning: "
#define CHECK_FAILURE_STRING_ASSERTION  \
  "Assertion \'%s\' failed (returned \'%s\')"
#define CHECK_FAILURE_STRING_PLACE      "In function \'%s\' at \'%s:%d\'.\n"
#define CHECK_FAILURE_ARGS_PLACE        THIS_FUNCTION,__FILE__,__LINE__

/*****************************************************************************/

#define CHECK_FAILURE_PRINT_ASSERTION(prefix,assertion,failvalue) \
  CHECK_FAILURE_REPORT_FUNCTION(prefix \
    CHECK_FAILURE_STRING_ASSERTION,#assertion,#failvalue \
  )

#ifdef CHECK_FAILURE_PRINT_PLACE

#define CHECK_FAILURE_PRINT_PLACE_FUNC() \
  CHECK_FAILURE_REPORT_FUNCTION( \
    CHECK_FAILURE_STRING_PLACE,CHECK_FAILURE_ARGS_PLACE \
  )

#define CHECK_FAILURE_FAILREPORT_VERBOSE(assertion,failvalue,excuse) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_ERROR,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(": \"%s\". ",(excuse)); \
  CHECK_FAILURE_PRINT_PLACE_FUNC();

#define CHECK_FAILURE_FAILREPORT_NORMAL(assertion,failvalue) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_ERROR,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(". "); \
  CHECK_FAILURE_PRINT_PLACE_FUNC();

#define CHECK_FAILURE_FAILREPORT_ERRNO(assertion,failvalue) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_ERROR,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(". Errno: \"%s\" ",strerror(errno)); \
  CHECK_FAILURE_PRINT_PLACE_FUNC();

#define CHECK_FAILURE_FAILREPORT_WARNING(assertion,failvalue,excuse) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_WARNING,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(": \"%s\". ",(excuse)); \
  CHECK_FAILURE_PRINT_PLACE_FUNC();

#else 

#define CHECK_FAILURE_FAILREPORT_VERBOSE(assertion,failvalue,excuse) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_ERROR,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(": \"%s\".\n",(excuse)); 

#define CHECK_FAILURE_FAILREPORT_NORMAL(assertion,failvalue) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_ERROR,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(".\n");

#define CHECK_FAILURE_FAILREPORT_ERRNO(assertion,failvalue) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_ERROR,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(". Errno: \"%s\"\n",strerror(errno));

#define CHECK_FAILURE_FAILREPORT_WARNING(assertion,failvalue,excuse) \
  CHECK_FAILURE_PRINT_ASSERTION(CHECK_FAILURE_STRING_WARNING,assertion,failvalue); \
  CHECK_FAILURE_REPORT_FUNCTION(": \"%s\".\n",(excuse));

#endif /* CHECK_FAILURE_PRINT_PLACE */

/*****************************************************************************/

/* does something in stead of returning a value */
#define CHECK_FAILURE_ACT(assertion,failvalue,action) \
  CHECK_FAILURE_BAREBONES( \
    CHECK_FAILURE_NORMAL_ASSERTION_EVALUATION(assertion,failvalue), \
    CHECK_FAILURE_FAILREPORT_NORMAL(assertion,failvalue); action \
  )

#define CHECK_FAILURE_WITH_ERRNO_ACT(assertion,failvalue,action) \
  CHECK_FAILURE_BAREBONES( \
    CHECK_FAILURE_NORMAL_ASSERTION_EVALUATION(assertion,failvalue), \
    CHECK_FAILURE_FAILREPORT_ERRNO(assertion,failvalue); action \
  )

#define CHECK_FAILURE_WITH_EXCUSE_ACT(assertion,failvalue,action,excuse) \
  CHECK_FAILURE_BAREBONES( \
    CHECK_FAILURE_NORMAL_ASSERTION_EVALUATION(assertion,failvalue), \
    CHECK_FAILURE_FAILREPORT_VERBOSE(assertion,failvalue,excuse); action \
  )

/* only issues a warning of a failure */
#define CHECK_FAILURE_WARNING(assertion,failvalue,excuse) \
  CHECK_FAILURE_WITH_EXCUSE_ACT(assertion,failvalue,failvalue,,excuse)

/*****************************************************************************/

/* no reporting */
#else

#define CHECK_FAILURE_ACT(assertion,failvalue,action) \
  CHECK_FAILURE_BAREBONES( \
    CHECK_FAILURE_NORMAL_ASSERTION_EVALUATION(assertion,failvalue), \
    action \
  )

#define CHECK_FAILURE_WITH_ERRNO_ACT(assertion,failvalue,action) \
  CHECK_FAILURE_ACT(assertion,failvalue,action)

#define CHECK_FAILURE_WITH_EXCUSE_ACT(assertion,failvalue,action,excuse) \
  CHECK_FAILURE_ACT(assertion,failvalue,action)

#define CHECK_FAILURE_WARNING(assertion,failvalue,excuse)

/*****************************************************************************/

#endif

/* reports a failure */
#define CHECK_FAILURE(assertion,failvalue,retvalue) \
  CHECK_FAILURE_ACT(assertion,failvalue,return (retvalue);)

/* reports a failure with an excuse */
#define CHECK_FAILURE_WITH_EXCUSE(assertion,failvalue,retvalue,excuse) \
  CHECK_FAILURE_WITH_EXCUSE_ACT(assertion,failvalue,return (retvalue);,excuse)

#define CHECK_FAILURE_WITH_ERRNO(assertion,failvalue,retvalue) \
  CHECK_FAILURE_WITH_ERRNO_ACT(assertion,failvalue,return (retvalue);)


#endif
