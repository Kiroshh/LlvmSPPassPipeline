Environment::

    1.Architecture:        x86_64
    2.CPU op-mode(s):      32-bit, 64-bit
    3.LLVM 6.0


Running the pass::

    1.Build the source 
    
    2.specify functions in input.txt
    
    3.Run the pass with opt->
        opt -load <libcostAnalysis.so> -cost < ourbitcodefile.bc > /dev/null
        
        
For testing::

    1.generate call graph ->
        opt -dot-callgraph test.bc

    