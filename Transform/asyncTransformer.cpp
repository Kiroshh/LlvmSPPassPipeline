////
//// Created by kiroshkumar on 19/08/19.
////

#include "asyncTransformer.h"

char Transformer::ID = 0;

void Transformer::getFunctions(std::vector<std::string> &functions) {
    std::ifstream inFile;
    inFile.open("results.txt");
    std::string funcName;
    int result;
    if (!inFile) {
        errs() << "Unable to open file";
        exit(1);
    }
    while (inFile >> funcName >> result) {
        if (result == 1) {
            functions.push_back(funcName);
        }
    }
    inFile.close();
}

bool Transformer::runOnModule(Module &M) {

    static std::vector<std::string> resultFunctionList;
    getFunctions(resultFunctionList);

    Module::FunctionListType &functions = M.getFunctionList();

    //creating constants that capture the function we need to insert
    FunctionType *functionType = M.getFunction("_ZN10ThreadPool21releaseMeWhenFinishedEv")->getFunctionType();
    Constant *hook = M.getOrInsertFunction("_ZN10ThreadPool21releaseMeWhenFinishedEv", functionType);


    FunctionType *functionType2 = M.getFunction(
            "_Z11syncExecuteiNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPFvS4_E")->getFunctionType();
    Constant *hook2 = M.getOrInsertFunction(
            "_Z11syncExecuteiNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPFvS4_E", functionType2);

    //vector to grab thread pool reference
    std::vector<Value *> args;
    std::vector<Value *> argsT;

    //stack to keep the instructions needed to be deleted
    std::stack<Instruction *> delInstructions;
    Instruction *pi;

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


                            errs() << "setting tpool instruction";

                            //collect the necessary arguments
                            for (size_t x = 0; x < i.getNumOperands(); ++x) {
//                                errs() << i.getNumOperands() << "first\n";
//                                dbgs() << i.getOperand(x)->getType() << "\n";
//                                dbgs() << i << "\n";
//                                dbgs() << i.getOperand(x) << "\n";
                                args.push_back(i.getOperand(x));
//                                dbgs() << x << "\n";

                            }
                            args.pop_back();
                            args.pop_back();
                            args.pop_back();

                        }
                        if (f->getName() ==
                            "_ZN10ThreadPool21releaseMeWhenFinishedEv") {

                            errs() << "release instructions";

                            delInstructions.push(dyn_cast<Instruction>(&i));

                        }
                        if (f->getName() ==
                            "_Z22keepPlaceForThreadJoinv") {
                            pi = dyn_cast<Instruction>(&i);

                        }
                    }
                }
            }
        }
    }

    //lets insert necessary call instructions

    bool isSelected = false;


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
                            "_Z12asyncExecuteiNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPFvS4_EP10ThreadPool") {
                            std::vector<Value *> argsAsync;

                            //collect the necessary arguments for our function call
                            for (size_t x = 0; x < i.getNumOperands(); ++x) {
                                dbgs() << i.getOperand(x)->getType() << "\n";
                                dbgs() << i << "\n";

                                dbgs() << i.getOperand(x) << "\n";
                                if (std::find(resultFunctionList.begin(), resultFunctionList.end(),
                                              i.getOperand(x)->getName()) != resultFunctionList.end()) {
                                    errs() << "this is what we need to check \n";
                                    isSelected = true;
                                }
                                argsAsync.push_back(i.getOperand(x));

                            }
                            argsAsync.pop_back();
                            argsAsync.pop_back();
                            argsAsync.pop_back();
                            argsAsync.pop_back();


                            if (!isSelected) {
                                //creating and inserting function call
                                CallInst::Create(hook2, argsAsync)->insertBefore(&i);
                                //change the operand to 0
                                Constant *zero = ConstantInt::get(llvm::Type::getInt32Ty(f->getContext()), 0);
                                errs() << i;
                                i.setOperand(0, zero);
                                errs() << i;


                            }

                            errs() << "async transformation done \n";

                        }
                        if (f->getName() ==
                            "_Z22keepPlaceForThreadJoinv") {
                            if (isSelected) {
                                CallInst::Create(hook, args)->insertBefore(&i);
                                errs() << "Successfully added the instruction\n";
                            }

                        }
                    }
                }
            }
        }
    }

    Instruction *tmp = NULL;
    errs() << "Lets Delete obsolete instructions:\n";
//    if (isSelected) {
//        while (!delInstructions.empty()) {
//            tmp = delInstructions.top();
//            delInstructions.pop();
//            dbgs() << tmp << "\n";
//            BasicBlock::iterator ii(pi);
//
//            ReplaceInstWithInst(tmp->getParent()->getInstList(), ii,
//            pi);
//        }
//    }

    return true;
}

static RegisterPass<Transformer>
        W("transform", "Pass to change async code");


