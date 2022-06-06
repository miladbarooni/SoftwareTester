/*
    name:           Aria Abrishamdar
    matric number:  9632411
    email:          ariaabrishamdar@gmail.com

    name:           Milad Barooni
    matric number:  9632459
    email:          milad.barooni99@gmail.com

*/


#include <cstdio>
#include <iostream>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>

#include <vector>
#include <string>
#include <sstream>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

LLVMContext &getGlobalContext() {
  static LLVMContext context;
  return context;
}

void generatePath(BasicBlock* BB);
std::string getSimpleNodeLabel(const BasicBlock *Node);
void analyze_instruction(Instruction* I);
bool comparison_analysis(std::string str);
void fuzzing();


// global variables
std::set<std::pair<std::string,BasicBlock*>>allBlocks;
std::set<std::pair<std::string,BasicBlock*>>visitedBlocks;
std::set<std::pair<std::string,BasicBlock*>> visitedBlocksInPath;
std::vector<std::pair<std::string, int>> input_variables;                  // a1, a2, ...
std::vector<std::pair<std::string, int>> all_variables;    // b, c, 0 (%0), 1 (%1), ...
std::vector<BasicBlock*> path_of_blocks;
std::string global_succ_name = "";

std::vector<std::vector<int>> fuzz_values;
std::vector<std::string> fuzz_operation = {"negate", "change_digit", "add_remove_digit"};



int main(int argc, char **argv) {

    srand(time(NULL));

    // Read the IR file.
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
    if (M == nullptr)
    {
      fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
      return EXIT_FAILURE;
    }

    // Collecting all basic blocks
    for (auto &F: *M)
        for (auto &BB: F)
        	allBlocks.insert(std::pair<std::string,BasicBlock* >(F.getName(),&BB));

    visitedBlocks.clear();

    for (int i=0; i<5 && (visitedBlocks.size()<allBlocks.size()); i++) {
        for (auto &F: *M)
            if (strncmp(F.getName().str().c_str(),"main",4) == 0){
                path_of_blocks.clear();
                visitedBlocksInPath.clear();
                BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
                generatePath(BB);
            }

        // output
        for (int i=0; i<input_variables.size(); i++)
            llvm::outs() << input_variables[i].first << " = " << input_variables[i].second << "\n";

        llvm::outs() << "Sequence of Basic Blocks:\n";

        for (auto &BB : path_of_blocks)
            llvm::outs() << getSimpleNodeLabel(BB) << "\n";

        llvm::outs() << "block coverage: " << visitedBlocks.size()*100/allBlocks.size() << "%\n\n";

        fuzzing();
    }

    return 0;
}

void generatePath(BasicBlock* BB) {

    path_of_blocks.push_back(BB);

    visitedBlocks.insert(std::pair<std::string,BasicBlock* >(BB->getParent()->getName(),BB));
    visitedBlocksInPath.insert(std::pair<std::string,BasicBlock* >(BB->getParent()->getName(),BB));

    // Analyze the block
    global_succ_name = "";
    for (auto &I: *BB) {
        std::string str;
        llvm::raw_string_ostream(str) << I;
        analyze_instruction(&I);

    }

    const Instruction *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();
    if (NSucc == 0)
        return;

    else if (NSucc == 1){
        BasicBlock *Succ = TInst->getSuccessor(0);
        generatePath(Succ);
    }

    else {

        // Find succ number
        int succ_number = 0;
        if (global_succ_name == getSimpleNodeLabel(TInst->getSuccessor(1)))
            succ_number = 1;

        BasicBlock *Succ = TInst->getSuccessor(succ_number);

        // Flip successor if this one is visited
        /*
        std::string fname = (std::string) TInst->getSuccessor(0)->getParent()->getName();
        if(visitedBlocksInPath.find(std::pair<std::string,BasicBlock* >(fname ,Succ)) != visitedBlocksInPath.end())
            Succ = TInst->getSuccessor(NSucc-succ_number-1);

        if(visitedBlocksInPath.find(std::pair<std::string,BasicBlock* >(fname,BB)) != visitedBlocksInPath.end() &&
            visitedBlocksInPath.find(std::pair<std::string,BasicBlock* >(fname,Succ)) != visitedBlocksInPath.end())
        Succ = TInst->getSuccessor(1); */

        generatePath(Succ);
    }
}

std::string getSimpleNodeLabel(const BasicBlock *Node) {
    if (!Node->getName().empty())
        return Node->getName().str();
    std::string Str;
    raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}


void analyze_instruction(Instruction* inst) {

    std::string str;
    llvm::raw_string_ostream(str) << (*inst);

    if (isa<AllocaInst> (inst)){

        int pos = str.find("%");
        str = str.substr(pos, str.size()-pos);

        std::string variable_name = str.substr(1, str.find(" ")-1);

        for (int i=0; i<all_variables.size(); i++)
            if (all_variables[i].first == variable_name)
                return;

        int val = -1;

        if ((str[1] == 'a') && (str[2] >= '0') && (str[2] <= '9')) {
            unsigned rnd = std::rand() / (RAND_MAX/199);
            val = rnd-99;
            input_variables.push_back({variable_name, val});

            std::vector<int> input_vec;
            input_vec.push_back(val);
            fuzz_values.push_back(input_vec);
        }


        all_variables.push_back({variable_name, val});
    }

    else if (isa<LoadInst> (inst)) {
        int pos11 = str.find("%");
        int pos12 = str.find("=");
        std::string str1 = str.substr(pos11+1, pos12-pos11-2);
        std::string str2 = (std::string) (inst->getOperand(0))->getName();

        int val = -1;
        for (int i=0; i<all_variables.size(); i++) {
            if (str2 == all_variables[i].first) {
                val = all_variables[i].second;
                break;
            }
        }

        for (int i=0; i<all_variables.size(); i++) {
            if (str1 == all_variables[i].first) {
                all_variables[i].second = val;
                return;
            }
        }
        all_variables.push_back({str1, val});

    }

    else if (isa<StoreInst> (inst)) {

        int val = -1;

        if (isa<ConstantInt>(inst->getOperand(0))) {
            val = dyn_cast<ConstantInt>(inst->getOperand(0))->getSExtValue();
        }

        else {
            int pos11 = str.find("%");
            int pos12 = str.find(",");
            std::string str1 = str.substr(pos11+1, pos12-pos11-1);
            for (int i=0; i<all_variables.size(); i++) {
                if (str1 == all_variables[i].first) {
                    val = all_variables[i].second;
                    break;
                }
            }
        }
        for (int i=0; i<all_variables.size(); i++) {
            if ((inst->getOperand(1))->getName() == all_variables[i].first) {
                all_variables[i].second = val;
                break;
            }
        }
    }

    else if (isa<CmpInst> (inst)) {
        int cmp_result = comparison_analysis(str);
        int pos11 = str.find("%");
        int pos12 = str.find("=");
        std::string str1 = str.substr(pos11+1, pos12-pos11-2);

        for (int i=0; i<all_variables.size(); i++) {
            if (str1 == all_variables[i].first) {
                all_variables[i].second = cmp_result;
                return;
            }
        }
        all_variables.push_back({str1, cmp_result});
    }

    else if (isa<BranchInst> (inst)) {
        if ((inst->getNumOperands()) == 1)
            return;
        else {
            std::string str1 = (std::string) inst->getOperand(0)->getName();
            int val = -1;
            for (int i=0; i<all_variables.size(); i++)
                if (all_variables[i].first == str1) {
                    val = all_variables[i].second;
                    break;
                }
            if (val == 1)
                global_succ_name = (std::string) inst->getOperand(2)->getName();
            else
                global_succ_name = (std::string) inst->getOperand(1)->getName();
        }
    }

    else if (isa<BinaryOperator> (inst)) {

        int opcode = inst->getOpcode();
        int val_1 = -1;
        int val_2 = -1;
        int res = -1;

        // split the instruction into words
        std::string str1;
        str1 = str;
        std::replace(str1.begin(), str1.end(), ',', ' ');
        std::stringstream ss(str1);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> words(begin, end);
        int len = words.size();

        // 1st operand
        if (isa<ConstantInt>(inst->getOperand(0)))
            val_1 = dyn_cast<ConstantInt>(inst->getOperand(0))->getSExtValue();
        else {
            std::string str2 = words[len-2].substr(1, words[len-2].size()-1);
            for (int i=0; i<all_variables.size(); i++)
                if (all_variables[i].first == str2) {
                    val_1 = all_variables[i].second;
                    break;
                }
        }

        // 2nd operand
        if (isa<ConstantInt>(inst->getOperand(1)))
            val_2 = dyn_cast<ConstantInt>(inst->getOperand(1))->getSExtValue();
        else {
            std::string str3 = words[len-1].substr(1, words[len-1].size()-1);
            for (int i=0; i<all_variables.size(); i++)
                if (all_variables[i].first == str3) {
                    val_2 = all_variables[i].second;
                    break;
                }
        }

        // apply the opcode
        if      (inst->getOpcode() == Instruction::Add)
            res = val_1 + val_2;
        else if (inst->getOpcode() == Instruction::Sub)
            res = val_1 - val_2;
        else if (inst->getOpcode() == Instruction::SDiv)
            res = val_1 / val_2;
        else if (inst->getOpcode() == Instruction::Mul)
            res = val_1 * val_2;


        // create or update the register
        int pos11 = str.find("%");
        int pos12 = str.find("=");
        std::string str4 = str.substr(pos11+1, pos12-pos11-2);

        for (int i=0; i<all_variables.size(); i++) {
            if (str4 == all_variables[i].first) {
                all_variables[i].second = res;
                return;
            }
        }
        all_variables.push_back({str4, res});
    }
}

bool comparison_analysis(std::string str) {


    // Analyze the comparison instruction
    // Get the line
    std::string str2;
    str2 = str;
    std::replace(str2.begin(), str2.end(), ',', ' ');
    std::stringstream ss(str2);
    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;
    std::vector<std::string> words(begin, end);

    // Get the condition
    std::string condition = words[words.size()-4];

    // Get the argument and the number and change the condition based on the order
    std::string first  = words[words.size()-2];
    std::string second = words[words.size()-1];

    std::string arg1 = "";
    std::string arg2 = "";
    int val1 = -1;
    int val2 = -1;

    if (first[0] == '%') {
        arg1 = first.substr(1, first.size()-1);
        for (int i=0; i<all_variables.size(); i++)
            if (all_variables[i].first == arg1) {
                val1 = all_variables[i].second;
                break;
            }
    }
    else
        val1 = std::stoi(first);

    if (second[0] == '%') {
        arg2 = second.substr(1, second.size()-1);
        for (int i=0; i<all_variables.size(); i++)
            if (all_variables[i].first == arg2) {
                val2 = all_variables[i].second;
                break;
            }
    }
    else
        val2 = std::stoi(second);


    // Return the result
    if (condition == "eq")
        return (val1 == val2);

    else if (condition == "ne")
        return (val1 != val2);

    else if (condition == "gt")
        return (val1 > val2);

    else if (condition == "lt")
        return (val1 < val2);

    else if (condition == "ge")
        return (val1 >= val2);

    else if (condition == "le")
        return (val1 <= val2);

    else if (condition == "sgt")
        return (val1 > val2);

    else if (condition == "slt")
        return (val1 < val2);

    else if (condition == "sge")
        return (val1 >= val2);

    else if (condition == "sle")
        return (val1 <= val2);

    else if (condition == "ugt")
        return (val1 > val2);

    else if (condition == "ult")
        return (val1 < val2);

    else if (condition == "uge")
        return (val1 >= val2);

    else if (condition == "ule")
        return (val1 <= val2);

    return 0;
}


void fuzzing() {

    bool flag = true;
    std::vector<int> vec;
    while (flag) {
        vec.clear();
        for (auto value: fuzz_values) {

            int prev_value = *value.rbegin();
            int new_fuzz_value;

            int rand_operation = std::rand() % fuzz_operation.size();
            std::string operation = fuzz_operation[rand_operation];

            if (operation == "negate") {
                new_fuzz_value = (-prev_value);
            }
            else if (operation == "change_digit") {
                if (std::abs(prev_value) < 10){
                    new_fuzz_value = std::rand()%10;
                    if (prev_value < 0)
                        new_fuzz_value *= (-1);
                }
                else {
                    int random_select = rand()%2;
                    if (random_select == 0){ // change 2nd digit
                        new_fuzz_value = ((rand()%10)*10) + (std::abs(prev_value)%10);
                        if (prev_value < 0)
                            new_fuzz_value *= (-1);
                    }
                    else{ // change 1st digit
                        new_fuzz_value = ((std::abs(prev_value)/10)*10) + (rand()%10);
                        if (prev_value < 0)
                            new_fuzz_value *= (-1);
                    }
                }
            }
            else if (operation == "add_remove_digit") {
                if (std::abs(prev_value) < 10){
                    int random_select = rand()%2;
                    if (random_select == 0){ // add a digit to the left
                        new_fuzz_value = ((rand()%10)*10) + std::abs(prev_value);
                        if (prev_value < 0)
                            new_fuzz_value *= (-1);
                    }
                    else{ // add a digit to the right
                        new_fuzz_value = (std::abs(prev_value)* 10) + (rand()%10);
                        if (prev_value < 0)
                            new_fuzz_value *= (-1);
                    }
                }
                else{
                    int random_select = rand()%2;
                    if (random_select == 0){ // remove the 2nd digit
                        new_fuzz_value = std::abs(prev_value)%10;
                        if (prev_value < 0)
                            new_fuzz_value *= (-1);
                    }
                    else{ // remove the 1st digit
                        new_fuzz_value = std::abs(prev_value)/10;
                        if (prev_value < 0)
                            new_fuzz_value *= (-1);
                    }
                }
            }
            vec.push_back(new_fuzz_value);
        }

        for (int i=0; i<fuzz_values[0].size(); i++) {
            bool dup = true;
            for (int j=0; j<fuzz_values.size(); j++) {
                if (vec[j] != fuzz_values[j][i]) {
                    dup = false;
                    break;
                }
            }
            if (dup)
                break;

            if (i==(fuzz_values[0].size()-1))
                flag = false;
        }

        if (!flag) {
            for (int i=0; i<fuzz_values.size(); i++) {
                fuzz_values[i].push_back(vec[i]);
                input_variables[i].second = vec[i];
                for (int j=0; j<all_variables.size(); j++) {
                    if (all_variables[j].first == input_variables[i].first) {
                        all_variables[j].second = vec[i];
                    }
                }
            }
        }
    }
}
