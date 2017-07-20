# AX_CHECK_PYTHON(MAJOR-VERSION[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
AC_DEFUN([AX_CHECK_PYTHON], [
ax_python_ok=no

AC_CHECK_PROGS([PYTHON$1], [python$1 python])

AS_IF([test -n "${PYTHON$1}"], [
	AC_MSG_CHECKING([whether ${PYTHON$1} is version $1])
	ax_python_version=$(${PYTHON$1} -V 2>&1 | grep -o 'Python @<:@0-9@:>@' | grep -o '@<:@0-9@:>@')
	AS_IF([test "$ax_python_version" = "$1"], [ax_python_ok=yes])
	AC_MSG_RESULT([$ax_python_ok])
])

AS_IF([test "$ax_python_ok" = "yes"], [
	m4_default([$2], [:])
], [
	PYTHON$1=
	m4_default([$3], [:])
])
AC_SUBST([PYTHON$1])
])
