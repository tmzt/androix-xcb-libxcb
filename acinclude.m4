dnl Detection and configuration of the visibility feature of gcc
dnl Vincent Torri 2006-02-11
dnl
dnl GCC_CHECK_VISIBILITY([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Check the visibility feature of gcc
dnl
AC_DEFUN([GCC_CHECK_VISIBILITY],
   [AC_MSG_CHECKING([whether ${CC} supports visibility feature])
    save_CFLAGS=${CFLAGS}
    CFLAGS="$CFLAGS -fvisibility=hidden -fvisibility-inlines-hidden"
    AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[
#pragma GCC visibility push(hidden)
extern void f(int);
#pragma GCC visibility pop
          ]],
          [[]]
        )],
       [AC_DEFINE(
           GCC_HAS_VISIBILITY,
           [],
           [Defined if GCC supports the vilibility feature])
        m4_if([$1], [], [:], [$1])
        AC_MSG_RESULT(yes)],
       [m4_if([$2], [], [:], [$2])
        AC_MSG_RESULT(no)])
    CFLAGS=${save_CFLAGS}
   ])
