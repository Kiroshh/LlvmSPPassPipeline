////
//// Created by kiroshkumar on 19/08/19.
////

#include "asyncTransformer.h"

char Transformer::ID = 0;


bool Transformer::runOnModule(Module &M) {

    Module::FunctionListType &functions = M.getFunctionList();

    FunctionType *functionType = M.getFunction("_ZN10ThreadPool21releaseMeWhenFinishedEv")->getFunctionType();
    Constant *hook = M.getOrInsertFunction("_ZN10ThreadPool21releaseMeWhenFinishedEv", functionType);

    std::vector<Value *> args;
    std::vector<Value *> argsT;


    FunctionType *functionType2 = M.getFunction("_Z14asyncTransformiPFviEP10ThreadPool")->getFunctionType();
    Constant *hook2 = M.getOrInsertFunction("_Z14asyncTransformiPFviEP10ThreadPool", functionType2);

    //TODO:lets put some placeholder functions is main.cpp so that we can easily add instructions needed there.(not necessary when finalised)

    //    lets grab thread pool operand and other values here for our later reference when transforming
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
                            "_ZN10ThreadPool20setThreadPoolWorkingEv") {


                            for (size_t x = 0; x < i.getNumOperands(); ++x) {
                                errs()<<i.getNumOperands()<<"first\n";
                                dbgs() << i.getOperand(x)->getType() << "\n";
                                dbgs() << i << "\n";
                                dbgs() << i.getOperand(x) << "\n";
                                args.push_back(i.getOperand(x));
                                dbgs() << x << "\n";

                            }
                            args.pop_back();
                        }


                        if (f->getName() ==
                            "_Z13syncTransformiPFviE") {
                            errs()<<i.getNumOperands()<<"synctrasform\n";
                            dbgs() << i << "\n";

                            for (size_t x = 0; x < i.getNumOperands(); ++x) {
                                dbgs() << i.getOperand(x)->getType() << "\n";
                                dbgs() << i.getOperand(x) << "\n";

                                argsT.push_back(i.getOperand(x));

                            }
                            argsT.pop_back();
                            argsT.insert(argsT.end(), args.begin(), args.end());

                        }
                    }
                }
            }
        }
    }

    //lets insert a call instructions needed
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
                            errs() << "Successfully added the instruction\n";
                            //TODO:erase /replace old instruction
//                            i.eraseFromParent();

                        }
                        if (f->getName() ==
                            "_Z13syncTransformiPFviE") {

                            dbgs() << argsT.size() <<"async trans arg size\n";
                            CallInst::Create(hook2, argsT)->insertBefore(&i);
                            errs() << "identified writetofile and replaced it\n";
                            //TODO:erase /replace old instruction
//                            i.eraseFromParent();

                        }
                    }
                }
            }
        }
    }

    return true;
}

static RegisterPass<Transformer>
        W("transform", "Pass to change async code");


