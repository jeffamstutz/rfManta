%module test

%import "parent.i"
%import "child.i"

%pythoncode %{

from parent import *
from child import *

%}



