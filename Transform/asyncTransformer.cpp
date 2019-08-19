//
// Created by kiroshkumar on 19/08/19.
//
#include "asyncTransformer.h"

char Transformer::ID = 0;


bool Transformer::runOnModule(Module &M) {

    Module::FunctionListType &functions = M.getFunctionList();
    for (Module::FunctionListType::iterator FI = functions.begin(), FE = functions.end(); FI != FE; ++FI) {
        //read from results and identify necessary transformations
        //modify code
        //delete sync functions
        //add selected tasks to thread pool
        //add threads joining code
        //finish

    }
    return true;
}

static RegisterPass<Transformer>
        W("transform", "Pass to change async code");