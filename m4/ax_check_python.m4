# SYNOPSIS
#
#   AX_CHECK_PYTHON(MAJOR-VERSION[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   This macro checks if the specified Python version is available. If
#   found, the PYTHON2 or PYTHON3 output variable points to the
#   corresponding interpreter binary.
#
# LICENSE
#
#   Copyright (c) 2018-2020 Lely Industries N.V.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

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
