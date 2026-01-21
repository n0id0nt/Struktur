// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: debug

class Profile {
    // Begin a profile scope.
    foreign static begin(arg0)
    // End a profile scope.
    foreign static end()
}

class Debug {
    // Call Debug Info.
    foreign static info(arg0)
    // Call Debug Warning.
    foreign static warning(arg0)
    // Call Debug Error.
    foreign static error(arg0)
    // Call Debug Fatal.
    foreign static fatal(arg0)
    // Call Debug Assert.
    foreign static assert(arg0)
    // Call Debug Assert with Message.
    foreign static assertMsg(arg0,arg1)
    // Call Debug Break.
    foreign static breakpoint()
    // Call Debug Break with Message.
    foreign static breakpointMsg(arg0)
}

