#!/usr/bin/python

# (fset 'callback-write-wrap
#    [tab ?s ?e ?l ?f ?. ?w ?r ?i ?t ?e ?( ?\" ?\C-e ?\" ?) ?\C-a ?\C-n])

def pair(t1, t2, seperator=" "):
    '''
    Performs a pairwise addition of tuple elements with a separator inbetween.

    pair(('a', 'b'), ('c', 'd')) => ('a c', 'b d')
    '''
    return map(lambda a,b: "%s%s%s" % (a,seperator,b), t1, t2)

def format(args, begin_seperator = "", prefix = "", seperator = ","):
    '''
    Returns a string made up of this expression:

    [begin_seperator][prefix][args[0][[seperator][prefix][args[1]]][...]

    The idea is you can prefix each argument, specify the separator between
    arguments, and an optional beginning string.  If the tuple is empty, you get
    back "".

    '''
    result = ""
    # if value is "" you get "", otherwise you get ("%s " % value)
    begin_seperator_text = begin_seperator and "%s " % begin_seperator
    #
    prefix_text = prefix and "%s " % prefix
    #
    seperator_text = seperator and "%s " % seperator
    #
    for arg in args[:1]:
        result += "%s%s%s" % (begin_seperator, prefix_text, arg)
    for arg in args[1:]:
        result += "%s%s%s" % (seperator_text, prefix_text, arg)
    return result

class Function:
    def __init__(self, num_call_time_args = 0, num_creation_time_args = 0):
        self.num_call_time_args = num_call_time_args
        self.num_creation_time_args = num_creation_time_args

    def templateParameter(self):
        arg_list = self.typeList()
        if (len(arg_list) == 0):
            return ""
        return "template<" + format(arg_list, prefix="typename") + ">"

    def baseName(self, withTemplate=False):
        base = "CallbackBase_%dData" % self.num_call_time_args
        if (withTemplate and self.num_call_time_args > 0):
            base+= "<" + format(self.dataList()) + ">"
        return base

    def staticClassName(self, withTemplate=False):
        base = "Callback_Static_%dData_%dArg" % (self.num_call_time_args, self.num_creation_time_args)
        type_list = self.typeList()
        if (withTemplate and len(type_list) > 0):
            base += "<" + format(type_list) + ">"
        return base

    def className(self, constant=False, withTemplate=False):
        if (constant):
            constant_text = "_const"
        else:
            constant_text = ""
        base = "Callback_%dData_%dArg%s" % (self.num_call_time_args, self.num_creation_time_args, constant_text)
        if (withTemplate):
            type_list = self.typeList()
            base += "<T" + format(type_list, begin_seperator=", ") + ">"
        return base

    def typeList(self):
        return self.dataList() + self.argList()

    def dataList(self, prefix="Data"):
        list = []
        for arg in range(1, self.num_call_time_args+1):
            list.append("%s%d" % (prefix, arg))
        return tuple(list)

    def argList(self, prefix="Arg"):
        list = []
        for arg in range(1, self.num_creation_time_args+1):
            list.append("%s%d" %(prefix,arg))
        return tuple(list)

    def functionArgList(self):
        return format(pair(self.argList(prefix="Arg"), self.argList(prefix="arg")),
                           begin_seperator=", ")

    def functionCallCreateList(self):
        return format(self.argList(prefix="arg"), begin_seperator=", ")

    def callArgList(self):
        return format(pair(self.dataList(prefix="Data"), self.dataList(prefix="data")))

class SourceFile:
    def __init__(self, filename):
        self.file = open(filename, "w")
        self.indent_ammount = 0

    def __del__(self):
        if (not self.file.closed):
            self.file.close()

    def indent(self):
        self.indent_ammount += 2;

    def unindent(self):
        self.indent_ammount -= 2;
        if (self.indent_ammount < 0):
            self.indent_ammount = 0

    def write(self, string, indent_ammount = None):
        if (indent_ammount == None):
            indent_ammount = self.indent_ammount
        if (string == ""):
            self.file.write("\n")
        else:
            self.file.write(" " * indent_ammount + string + "\n")

class CallbackH (SourceFile):
    def __init__(self, filename = "Callback.h"):
        SourceFile.__init__(self, filename)

    def writeStart(self):
        self.write("/*")
        self.indent()
        self.write("This file was automatically generated.  Don't modify by hand.")
        self.write("")
        self.write("For a detailed explaination of the Callback construct please see:")
        self.write("")
        self.write("doc/Callbacks.txt")
        self.write("")
        self.write("Briefly explained:")
        self.write("")
        self.write("1.  Only void return functions are supported.")
        self.write("2.  Data is the call time bound arguments.")
        self.write("3.  Arg  is the callback creation time bound arguments.")
        self.write("4.  Data parameters preceed the Arg parameters of the callback function.")
        self.write("5.  If you don't find a create function that match the number of")
        self.write("Data and Arg parameters you need, add the create function and")
        self.write("corresponding Callback_XData_XArg class.")
        self.write("")
        self.unindent()
        self.write("*/")
        self.write("")
        self.write("#ifndef Manta_Interface_Callback_h")
        self.write("#define Manta_Interface_Callback_h")
        self.write("")
        self.write("#include <Core/Util/CallbackHelpers.h>")
        self.write("")
        self.write("namespace Manta {")
        self.indent()
        self.write("class Callback {")
        self.write("public:")
        self.indent()

    def writeEnd(self):
        self.unindent()
        self.write("private:")
        self.indent()
        self.write("Callback(const Callback&);")
        self.write("Callback& operator=(const Callback&);")
        self.unindent()
        self.write("}; // end class Callback")
        self.unindent()
        self.write("} // end namespace Manta")
        self.write("#endif // #ifndef Manta_Interface_Callback_h")
        # Close the file
        self.file.close()

    def writeGlobalStaticFunctions(self, functions):
        self.write("/" * 30)
        self.write("// Global functions or static class member functions")
        self.write("");
        for function in functions:
            num_call_time_args = function.num_call_time_args
            num_creation_time_args = function.num_creation_time_args
            self.write("// %d call time args --- %d creation time args" % (num_call_time_args, num_creation_time_args))
            tp = function.templateParameter()
            self.write("%s%sstatic" % (tp, tp and ' ')) # add a space based on if tp is ''.
            self.write(function.baseName(withTemplate=True) + "*")
            self.write("create(void (*pmf)(" + format(function.typeList()) + ")" + function.functionArgList() + ") {")
            self.indent()
            self.write("return new " + function.staticClassName(withTemplate=True) + "(pmf" + function.functionCallCreateList() + ");")
            self.unindent()
            self.write("}")
            self.write("")

    def writeClassMemberFunctions(self, functions, constant=False):
        if (constant):
            constant_text = " const"
        else:
            constant_text = ""
        self.write("/" * 30)
        self.write("// Class member functions")
        self.write("");
        for function in functions:
            num_call_time_args = function.num_call_time_args
            num_creation_time_args = function.num_creation_time_args
            if (constant):
                self.write("#ifndef SWIG // For now these const versions won't compile in swig", indent_ammount=0)
            self.write("// %d call time args --- %d creating time args" % (num_call_time_args, num_creation_time_args))
            self.write("template<class T" + format(function.typeList(), begin_seperator=", ", prefix="typename") + "> static")
            self.write(function.baseName(withTemplate=True) + "*")
            self.write("create(T* ptr, void (T::*pmf)(" + format(function.typeList()) + ")" + constant_text + function.functionArgList() + ") {")
            self.indent()
            self.write("return new " + function.className(constant, withTemplate=True) + "(ptr, pmf" + function.functionCallCreateList() + ");")
            self.unindent()
            self.write("}")
            if (constant):
                self.write("#endif // #ifndef SWIG", indent_ammount=0)
            self.write("")

class CallbackHelpersH (SourceFile):
    def __init__(self, filename = "CallbackHelpers.h"):
        SourceFile.__init__(self, filename)
        self.writtenClasses = []

    def classDone(self, name):
        if (name in self.writtenClasses):
            return True
        else:
            self.writtenClasses.append(name)
            return False

    def writeStart(self):
        self.write("")
        self.write("#ifndef Manta_Interface_CallbackHelpers_h")
        self.write("#define Manta_Interface_CallbackHelpers_h")
        self.write("")
        self.write("#include <Core/Util/CallbackHandle.h>")
        self.write("")
        self.write("// You should not use these directly - use Callback::create instead")
        self.write("namespace Manta {")
        self.indent()

    def writeEnd(self):
        self.unindent()
        self.write("} // end namespace Manta")
        self.write("#endif // #ifndef Manta_Interface_CallbackHelpers_h")
        # Close the file
        self.file.close()

    def writeBaseClasses(self, functions):
        self.write("/" * 30)
        self.write("// Base classes that can be used by both the static and non-static")
        self.write("// Callback classes.")
        self.write("//")
        self.write("")

        for function in functions:
            class_name = function.baseName(withTemplate=False)
            if (self.classDone(class_name)): continue
            self.write("// %d call time args" % function.num_call_time_args)
            data_list = function.dataList()
            if (len(data_list) > 0):
                self.write("template<%s>" % format(data_list, prefix="typename"))
            self.write("class %s : public CallbackHandle {" % class_name)
            self.write("public:")
            self.indent()
            self.write("%s()" % class_name)
            self.write("{")
            self.write("}")
            self.write("virtual ~%s()" % class_name)
            self.write("{")
            self.write("}")
            self.write("virtual void call(%s) = 0;" % function.callArgList())
            self.unindent()
            self.write("private:")
            self.indent()
            self.write("%s(const %s&);" % (class_name, class_name))
            self.write("%s& operator=(const %s&);" % (class_name, class_name))
            self.unindent()
            self.write("};")
            self.write("")

    def writeGlobalStaticFunctions(self, functions):
        self.write("/" * 30)
        self.write("// Global functions or static class member functions")
        self.write("//")
        self.write("")

        for function in functions:
            class_name = function.staticClassName(withTemplate=False)
            if (self.classDone(class_name)): continue
            self.write("// %d call time args --- %d creating time args" % (function.num_call_time_args, function.num_creation_time_args))
            self.write("%s" % function.templateParameter() )
            self.write("class %s : public %s {" % (class_name, function.baseName(withTemplate=True)))
            self.write("public:")
            self.indent()
            self.write("%s(void (*pmf)(%s)%s)" % (class_name, format(function.typeList()), function.functionArgList()))
            self.indent()
            initializers = function.argList(prefix="arg")
            initializers = map(lambda a: "%s(%s)" % (a,a), initializers)
            self.write(": pmf(pmf)%s" % format( initializers, begin_seperator=", " ) )
            self.unindent()
            self.write("{")
            self.write("}")
            self.write("virtual ~%s()" % class_name)
            self.write("{")
            self.write("}")
            self.write("virtual void call(%s)" % function.callArgList())
            self.write("{")
            self.indent()
            self.write("pmf(%s);" % format( function.dataList(prefix="data") + function.argList(prefix="arg") ))
            self.unindent()
            self.write("}")
            self.unindent()
            self.write("private:")
            self.indent()
            self.write("void (*pmf)(%s);" % format(function.typeList()))
            map(lambda A,a: self.write("%s %s;" % (A,a)),
                function.argList(prefix="Arg"),
                function.argList(prefix="arg"))
            self.unindent()
            self.write("};")
            self.write("")

    def writeClassMemberFunctions(self, functions, constant=False):
        if (constant):
            constant_text = " const"
        else:
            constant_text = ""
        if (constant):
            self.write("#ifndef SWIG // For now these const versions won't compile in swig", indent_ammount=0)
        self.write("/" * 30)
        self.write("// Class member functions")
        self.write("");
        for function in functions:
            class_name = function.className(constant, withTemplate=False)
            if (self.classDone(class_name)): continue
            self.write("// %d call time args --- %d creating time args" % (function.num_call_time_args, function.num_creation_time_args))
            self.write("template<class T" + format(function.typeList(), begin_seperator=", ", prefix="typename") + ">")
            self.write("class %s : public %s {" % (class_name, function.baseName(withTemplate=True)))
            self.write("public:")
            self.indent()
            self.write("%s(T* ptr, void (T::*pmf)(%s)%s%s)" % (class_name, format(function.typeList()), constant_text, function.functionArgList()))
            self.indent()
            initializers = function.argList(prefix="arg")
            initializers = map(lambda a: "%s(%s)" % (a,a), initializers)
            self.write(": ptr(ptr), pmf(pmf)%s" % format( initializers, begin_seperator=', '))
            self.unindent()
            self.write("{")
            self.write("}")
            self.write("virtual ~%s()" % class_name)
            self.write("{")
            self.write("}")
            self.write("virtual void call(%s)" % function.callArgList())
            self.write("{")
            self.indent()
            self.write("(ptr->*pmf)(%s);" % format( function.dataList(prefix="data") + function.argList(prefix="arg") ))
            self.unindent()
            self.write("}")
            self.unindent()
            self.write("private:")
            self.indent()
            self.write("T* ptr;")
            self.write("void (T::*pmf)(%s)%s;" %
                       (format(function.typeList()), constant_text))
            map(lambda A,a: self.write("%s %s;" % (A,a)),
                function.argList(prefix="Arg"),
                function.argList(prefix="arg"))
            self.unindent()
            self.write("};")
            self.write("")

        # end for loop
        if (constant):
            self.write("#endif // #ifndef SWIG", indent_ammount=0)
            self.write("")




def addFunctions(functions, num_call_time_args, creation_time_arg_sizes):
    for T in creation_time_arg_sizes:
        functions.append(Function(num_call_time_args, T))

def main():
    static_functions = []
    const_member_functions = []
    member_functions = []

    addFunctions(static_functions, 0, (0,1,2,3,5))
    addFunctions(static_functions, 1, (0,1,2))
    addFunctions(static_functions, 2, (0,2))
    addFunctions(static_functions, 3, (1,2))
    addFunctions(static_functions, 4, (0,1,2,))

    addFunctions(member_functions, 0, (0,1,2,3,4,5))
    addFunctions(const_member_functions, 0, (0,1,2))

    addFunctions(member_functions, 1, (0,1,2,3,4,5))
    addFunctions(member_functions, 2, (0,1,2,3))
    addFunctions(member_functions, 3, (0,1))
    addFunctions(member_functions, 4, (0,1,2))

    addFunctions(member_functions, 5, (0,))
    addFunctions(member_functions, 6, (0,))

    callbackH = CallbackH()
    callbackH.writeStart()
    callbackH.writeGlobalStaticFunctions(static_functions)
    callbackH.writeClassMemberFunctions(member_functions)
    callbackH.writeClassMemberFunctions(const_member_functions, constant=True)
    callbackH.writeEnd()

    callbackHelpersH = CallbackHelpersH()
    callbackHelpersH.writeStart()
    callbackHelpersH.writeBaseClasses(static_functions + member_functions + const_member_functions)
    callbackHelpersH.writeGlobalStaticFunctions(static_functions)
    callbackHelpersH.writeClassMemberFunctions(member_functions)
    callbackHelpersH.writeClassMemberFunctions(const_member_functions, constant=True)
    callbackHelpersH.writeEnd()

if __name__ == "__main__":
    main()


