#!/bin/csh
# csh .login
#
# Files you make look like rw-r--r--
umask 022

set defterm="vt100"

if (! ${?term}) set term=dumb

if ($term == "dumb" || $term == "network" || $term == "unknown" || $term == "dialup") then
   if ($tty != "console") then
      echo -n "TERM = ("$defterm") "
      set term=$<
      if ( $term =~ "" ) set term=$defterm
      setenv TERM $term
   else
      set term=st52
   endif
else
   echo term=$term
endif
