//
// Created by kiroshkumar on 03/08/19.
//

#include "costEstimator.h"

char LlvmSP::Cost::ID = 0;



void LlvmSP::Cost::getCost(Function *FI, int &FunCost) {
    int tempCount = 0;
    for (BasicBlock &bb : *FI) {
        for (Instruction &i: bb) {
            tempCount++;
            //TODO:add the cost based on the type of instruction.need some research....(now the total instruction count is taken)
            if (llvm::isa<llvm::CallInst>(i) ||
                llvm::isa<llvm::InvokeInst>(i)) {
//                errs() << "Im a Call Instruction!\n";
            } else if (llvm::isa<llvm::AllocaInst>(i) ||
                       llvm::isa<llvm::LoadInst>(i) ||
                       llvm::isa<llvm::StoreInst>(i) ||
                       llvm::isa<llvm::AtomicCmpXchgInst>(i) ||
                       llvm::isa<llvm::AtomicRMWInst>(i)) {
//                errs() << "Memory access/addressing Instruction!\n";
            } else if (llvm::isa<llvm::BinaryOperator>(i)) {
//                errs() << "binary Instruction!\n";
            } else if (llvm::isa<llvm::ExtractElementInst>(i)
                       || llvm::isa<llvm::InsertElementInst>(i)
                       || llvm::isa<llvm::ShuffleVectorInst>(i)) {
//                errs() << "vector Instruction!\n";
            } else if (llvm::isa<llvm::ExtractValueInst>(i) ||
                       llvm::isa<llvm::InsertValueInst>(i)) {
//                errs() << " Aggregate Instruction!\n";
            } else {
//                errs() << "some other Instruction!\n";
            }
        }
    }
    //TODO:identify this block has loop & if multiply instructions by loop iterrations before add to the total
    //FIXME:loop information :errors while tried to calculate loop count
// if the BB is inside a loop, multiply instructions by the trip count
//                const llvm::LoopInfo &LI = getAnalysis<llvm::LoopInfo>(*FI);
//                llvm::ScalarEvolution &SE = getAnalysis<llvm::ScalarEvolution>(*FI);
//                if (Loop *l = LI.getLoopFor(&bb)) {
//                    int tripCount = SE.getSmallConstantTripCount(l, nullptr);
//                    errs() << tripCount << " is the tripCount for this BB\n";
//                    if (tripCount > 0) {
//                        // the trip count is known! multiply all our numbers by it
//                        insCount *=tripCount;
//
//                    }
//                }
    FunCost = tempCount;
//    errs() << FunCost << " : cost\n";
}

void LlvmSP::Cost::getCallsList(Function &F, Module &M, int &FunTCost) {
    std::deque<std::string> FunctionList;
    FunctionList.clear();
    FunctionList.push_back(F.getName());
    int totalCost = 0;
    while (!FunctionList.empty()) {
        int cost = 0;
        std::string funName = FunctionList.front();
//        errs() << funName << "Fuction taken \n";
        Function *func = M.getFunction(funName);
        getCost(func, cost);
        totalCost += cost;
//        errs() << totalCost << " -total cost now";
//        errs() << FunctionList.size() << "!\n";
        FunctionList.pop_front();
        for (BasicBlock &bb : *func) {
            for (Instruction &i: bb) {
                //callsite helps us extract if call/invoke instruction
                CallSite cs(&i);
                // Check whether the instruction is actually a call
                if (!cs.getInstruction()) {
                    continue;
                }
                Value *called = cs.getCalledValue()->stripPointerCasts();
                if (Function *f = dyn_cast<Function>(called)) {
                    FunctionList.push_back(f->getName());
//                    errs() << "found call instruction";
                }
            }
        }
    }
    FunTCost = totalCost;
    FunctionList.clear();
}


void LlvmSP::Cost::writeResults(std::map<std::string, int> &results) {
    std::ofstream outfile("results.txt");
    std::map<std::string, int>::iterator i = results.begin();
    std::map<std::string, int>::iterator e = results.end();
    while (i != e) {
        outfile << i->first << ": " << i->second << std::endl;
        i++;
    }
    outfile.close();
}

//TODO:Implement this method
bool LlvmSP::Cost::decideTransform(std::map<std::string, int> &results) {
    const int syncAndSpawnCost = 0;  //FIXME:determine this
    int syncCost = 0;
    int asyncCost = 0;
    //is it either sync or async???? is that a only possibility
    int maxFunCost = 0;
    for (auto const &x : results) {
        syncCost += x.second;
        if (x.second > maxFunCost) {
            maxFunCost = x.second;
        }
    }
    errs() << "sync cost : " << syncCost;
    errs() << "\n";
    asyncCost = maxFunCost + syncAndSpawnCost;
    errs() << "async cost : " << asyncCost;
    errs() << "\n";
    if (syncCost < asyncCost) {
        return false;
    }
    return true;
}


void LlvmSP::Cost::getFunctions(std::vector<std::string> &functions) {
    std::ifstream inFile;
    inFile.open("input.txt");
    std::string funcName;
    if (!inFile) {
        errs() << "Unable to open file";
        exit(1);
    }
    while (inFile >> funcName) {
        functions.push_back(funcName);
    }
    inFile.close();
}


bool LlvmSP::Cost::runOnModule(Module &M) {
    static std::map<std::string, int> costListPrep;
    static std::vector<std::string> inputFunctionList;
    getFunctions(inputFunctionList);
    //iterating through functions and find our required functions
    Module::FunctionListType &functions = M.getFunctionList();
    for (Module::FunctionListType::iterator FI = functions.begin(), FE = functions.end(); FI != FE; ++FI) {
        bool isFound = false;
        for (auto const &x : inputFunctionList) {
            if (FI->getName().contains(x)) {
                costListPrep[FI->getName()] = 0;
                isFound = true;
            }
        }
        if (isFound) {
            int totalFunCost = 0;
            errs() << FI->getName();
            getCallsList(*FI, M, totalFunCost);
            costListPrep[FI->getName()] = totalFunCost;
        }
    }
    std::map<std::string, int>::iterator i = costListPrep.begin();
    std::map<std::string, int>::iterator e = costListPrep.end();
    while (i != e) {
        errs() << i->first << ": " << i->second << "\n";
        i++;
    }
    errs() << "\n";
    decideTransform(costListPrep);
    writeResults(costListPrep);
    costListPrep.clear();
    return false;
}


static RegisterPass<LlvmSP::Cost> X("cost", "costAnalysis", false, true);

