

%module wxManta_helper

%include "std_string.i"
%include "std_vector.i"
%include "exception.i"

%{
#include <MantaTypes.h>
#include <Interface/RayPacket.h>
#include <Interface/MantaInterface.h>
#include <Core/Util/CallbackHelpers.h>


#include <SwigInterface/wxManta.h>
%}

%include <MantaTypes.h>
%include <Interface/RayPacket.h>
%include <Core/Util/CallbackHelpers.h>
%include <Interface/MantaInterface.h>


%include <SwigInterface/wxManta.h>
