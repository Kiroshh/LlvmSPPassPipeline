////
//// Created by kiroshkumar on 19/08/19.
////

#include "asyncTransformer.h"

char Transformer::ID = 0;


bool Transformer::runOnModule(Module &M) {

    Module::FunctionListType &functions = M.getFunctionList();

    //creating constants that capture the function we need to insert
    FunctionType *functionType = M.getFunction("_ZN10ThreadPool21releaseMeWhenFinishedEv")->getFunctionType();
    Constant *hook = M.getOrInsertFunction("_ZN10ThreadPool21releaseMeWhenFinishedEv", functionType);


    FunctionType *functionType2 = M.getFunction("_Z14asyncTransformiPFviEP10ThreadPool")->getFunctionType();
    Constant *hook2 = M.getOrInsertFunction("_Z14asyncTransformiPFviEP10ThreadPool", functionType2);

    //vector to grab thread pool reference
    std::vector<Value *> args;
    std::vector<Value *> argsT;

    for (Module::FunctionListType::iterator FI = functions.begin(), FE = functions.end(); FI != FE; ++FI) {
        if (FI->getName() == "main") {
            //iterating over the basic blocks of main function
            for (BasicBlock &bb : *FI) {
                for (Instruction &i: bb) {

                    CallSite cs(&i);
                    //check whether instruction is a call
                    if (!cs.getInstruction()) {
                        continue;
                    }
                    // Check whether the called function is directly invoked and then the function is what we need
                    Value *called = cs.getCalledValue()->stripPointerCasts();
                    if (Function *f = dyn_cast<Function>(called)) {
                        if (f->getName() ==
                            "_ZN10ThreadPool20setThreadPoolWorkingEv") {

                            //collect the necessary arguments
                            for (size_t x = 0; x < i.getNumOperands(); ++x) {
                                errs() << i.getNumOperands() << "first\n";
                                dbgs() << i.getOperand(x)->getType() << "\n";
                                dbgs() << i << "\n";
                                dbgs() << i.getOperand(x) << "\n";
                                args.push_back(i.getOperand(x));
                                dbgs() << x << "\n";

                            }
                            args.pop_back();
                        }


                    }
                }
            }
        }
    }

    //lets insert necessary call instructions

    //stack to keep the instructions needed to be deleted
    std::stack<Instruction *> delInstructions;

    for (Module::FunctionListType::iterator FI = functions.begin(), FE = functions.end(); FI != FE; ++FI) {

        if (FI->getName() == "main") {
            for (BasicBlock &bb : *FI) {
                for (Instruction &i: bb) {

                    CallSite cs(&i);
                    if (!cs.getInstruction()) {
                        continue;
                    }

                    Value *called = cs.getCalledValue()->stripPointerCasts();
                    if (Function *f = dyn_cast<Function>(called)) {

                        if (f->getName() ==
                            "_Z22keepPlaceForThreadJoinv") {

                            CallInst::Create(hook, args)->insertBefore(&i);
                            errs() << "Successfully added the thread join instructions\n";

                            delInstructions.push(dyn_cast<Instruction>(&i));
                        }

                        if (f->getName() ==
                            "_Z13syncTransformiPFviE") {
                            std::vector<Value *> argsAsync;
                            bool isSelected = false;

                            //collect the necessary arguments for our function call
                            for (size_t x = 0; x < i.getNumOperands(); ++x) {
                                dbgs() << i.getOperand(x)->getType() << "\n";
//                                dbgs() << i.getOperand(x) << "\n";
                                if (i.getOperand(x)->getName() == "_Z11writeToFilei") {
                                    errs() << "this is what we need to check \n";
                                    isSelected = true;
                                    //have to generalise this for all task-functions in the config file
                                }
                                argsAsync.push_back(i.getOperand(x));

                            }
                            argsAsync.pop_back();
                            argsAsync.insert(argsAsync.end(), args.begin(), args.end());
                            //TODO:if isselected is true do the transformations

                            //creating and inserting function call
                            CallInst::Create(hook2, argsAsync)->insertBefore(&i);
                            errs() << "async transformation done \n";

                            delInstructions.push(dyn_cast<Instruction>(&i));
                        }
                    }
                }
            }
        }
    }

    Instruction *tmp = NULL;
    errs() << "Lets Delete obsolete instructions:\n";

    while (!delInstructions.empty()) {
        tmp = delInstructions.top();
        delInstructions.pop();
        dbgs() << tmp << "\n";
        tmp->eraseFromParent();
    }

    return true;
}

static RegisterPass<Transformer>
        W("transform", "Pass to change async code");


