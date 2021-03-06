awesomeX: an automatic function JITifier

----------------------------------------

awesomeX is designed to automatically convert a standard C file's function
exports into LLVM bitcode. It goes one step further and automatically generates
shim functions to run these bitcoded functions in the original calling code.

The intended use case is some kind of numerically intensive function that would
benefit from JIT optimizations. Since awesomeX is intended to work automatically
on the file level, existing code can be converted without extreme rewrites.

A more advanced use case is planned that exposes function objects themselves. A
use here would be to optimize and store functions with partial application.
Imagine a regex function with a user-specified regex that is meant to run over
tremendous amounts of input. Optimizing with a specific regex could expose
unnecessary branch statements to the JIT which can remove those branches
entirely.

awesomeX is in a very preliminary state right now! While it functions as a proof
of concept, there is a lot of hard-coded gunk and is non-functional for real
code at the moment. If you're interested, patches are certainly welcome.

You will need a recentish LLVM library (with C bindings, usually in
/usr/include/llvm-c) and an llc with -march=cpp support. If you're using gentoo,
you will have to specify the 'multitarget' USE flag to get this.

Right now: Running the Makefile should generate two programs, stockprint and
jitprint. Inspecting the Makefile, stockprint is linked to the stock code from
print.c, while jitprint is linked to an autogenerated LLVM bitcode
representation of print.c, called through the jitprint.c shim function.

awesomex.cpp is generated from print.c through llc's -march=cpp functionality.
Note that awesomex.cpp contains code that uses LLVM's API to build up print.c
'in memory.' Then, codegen.c is linked to awesomex.cpp, and function prototype
information plus a shim loader/optimizer is output and saved into jitprint.c. In
the future, codegen should be a program that reads straight off of the bitcode
created in the original clang export of print.c (before llc). This linking thing
is nonsense.

Finally, awesomex, jitprint and main are linked together for the jitprint
binary.

Most of the terribleness is in the codegen phase, there are parts that range
from bad to awful. I have a feeling LLVM has functions that do what I want
buried somewhere and that codegen.c poorly duplicates this functionality
If you have any ideas on how to clean it up please let me know!

<!-- vim: set tw=80: -->
