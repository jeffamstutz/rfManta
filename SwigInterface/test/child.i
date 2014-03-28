%module(package="test") child

%import "parent.i"

%{
#include "child.h"
%}

%include "child.h"
