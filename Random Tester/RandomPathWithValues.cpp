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
void analyze_instruction(std::string);
void traverse_path();


// global variables
std::vector<BasicBlock*> blocks;
std::vector<std::string> input_names;                   // a1, a2, ...
std::vector<std::pair<std::string, std::string>> local_variables;   // b, c, 0, 1, ...
std::vector<std::vector<int>> input_limits;    // -100 to 100 for each variable


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


    for (auto &F: *M)
        if (strncmp(F.getName().str().c_str(),"main",4) == 0){
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            blocks.push_back(BB);
            //llvm::outs() << getSimpleNodeLabel(BB) << "\n";
            generatePath(BB);

            // Find the input variable names
            for (auto &B:F) {
                for (auto &I:B) {
                    std::string str;
                    llvm::raw_string_ostream(str) << I;
                    analyze_instruction(str);
                }
            }
        }


    traverse_path();

    // print arguments with random valid values
    for (int i=0; i<input_names.size(); i++) {
        int rnd_index = (rand()%input_limits[i].size());
        llvm::outs() << input_names[i] << " = " << std::to_string(input_limits[i][rnd_index]) << "\n";
    }

    // print the path
    llvm::outs() << "Sequence of Basic Blocks:\n";
    for (auto &BB : blocks)
        llvm::outs() << getSimpleNodeLabel(BB) << "\n";

    return 0;
}

void generatePath(BasicBlock* BB) {

  const Instruction *TInst = BB->getTerminator();
  unsigned NSucc = TInst->getNumSuccessors();
  if (NSucc == 1){
      BasicBlock *Succ = TInst->getSuccessor(0);
      blocks.push_back(Succ);
      //llvm::outs() << getSimpleNodeLabel(Succ) << ",\n";
      generatePath(Succ);
  }else if (NSucc>1){
      //srand(time(NULL));
      unsigned rnd = std::rand() / (RAND_MAX/NSucc); // rand() return a number between 0 and RAND_MAX
      BasicBlock *Succ = TInst->getSuccessor(rnd);
      blocks.push_back(Succ);
      //llvm::outs() << getSimpleNodeLabel(Succ) << "\n";
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

void analyze_instruction(std::string str) {

    if (str.find("%") == std::string::npos)
        return;


    if (str.find("alloca") != std::string::npos) {

        int pos = str.find("%");
        str = str.substr(pos, str.size()-pos);

        if ((str[1] == 'a') && (str[2] >= '0') && (str[2] <= '9')) {

            std::string variable_name = str.substr(1, str.find(" ")-1);

            input_names.push_back(variable_name);

            std::vector<int> temp;
            for (int i=(-100); i<=100; i++)
                temp.push_back(i);
            input_limits.push_back(temp);

        }
        else {
            std::string variable_name = str.substr(1, str.find(" ")-1);
            local_variables.push_back({variable_name, ""});
        }
    }

    else if (str.find("load") != std::string::npos) {
        for (int i=0; i<input_names.size(); i++) {
            if (str.find("%" + input_names[i] + ",") != std::string::npos) {
                int pos = str.find("%");
                str = str.substr(pos, str.size()-pos);
                std::string variable_name = str.substr(1, str.find(" ")-1);
                local_variables.push_back({variable_name, input_names[i]});
                break;
            }
        }
        for (int i=0; i<local_variables.size(); i++) {
            if (str.find("%" + local_variables[i].first + ",") != std::string::npos) {
                int pos = str.find("%");
                str = str.substr(pos, str.size()-pos);
                std::string variable_name = str.substr(1, str.find(" ")-1);
                local_variables.push_back({variable_name, local_variables[i].second});
                break;
            }
        }
    }

    else if (str.find("store") != std::string::npos) {
        if (std::count(str.begin(), str.end(), '%') > 1) {
            for (int i=0; i<local_variables.size(); i++) {
                int pos11 = str.find_last_of("%");
                int pos12 = str.find_last_of(",");
                if (str.substr(pos11+1, pos12-pos11-1) == local_variables[i].first) {
                    for (int j=0; j<local_variables.size(); j++) {
                        int pos21 = str.find("%");
                        int pos22 = str.find(",");
                        if (str.substr(pos21+1, pos22-pos21-1) == local_variables[j].first){
                            local_variables[i].second = local_variables[j].second;
                            break;
                        }

                    }
                }
            }
        }

    }
}

void traverse_path() {

    for (auto &BB : blocks) {

        unsigned NSucc = BB->getTerminator()->getNumSuccessors();

        if (NSucc > 1) {

            // Check for the comparison result
            bool comp_result;
            std::string str;
            llvm::raw_string_ostream(str) << BB->back();
            int index = str.find_last_of(' ')+2; // starting_index_of_last_word
            str = str.substr(index, str.size()-index);
            comp_result = (str != getSimpleNodeLabel(*(&BB + 1)));

            // Analyze the comparison instruction
            // Get the line
            std::string str2;
            llvm::raw_string_ostream(str2) << *std::next(BB->rbegin());
            std::replace( str2.begin(), str2.end(), ',', ' ');
            std::stringstream ss(str2);
            std::istream_iterator<std::string> begin(ss);
            std::istream_iterator<std::string> end;
            std::vector<std::string> words(begin, end);

            // Get the condition
            std::string condition = words[words.size()-4];

            // Get the argument and the number and change the condition based on the order
            std::string first  = words[words.size()-2];
            std::string second = words[words.size()-1];

            std::string arg = "";
            int value;

            if (first[0] == '%') {
                arg = first;
                value = std::stoi(second);
            }
            else {
                arg = second;
                value = std::stoi(first);

                if      (condition == "gt")     condition = "lt";
                else if (condition == "lt")     condition = "gt";
                else if (condition == "ge")     condition = "le";
                else if (condition == "le")     condition = "ge";
                else if (condition == "sgt")    condition = "slt";
                else if (condition == "slt")    condition = "sgt";
                else if (condition == "sge")    condition = "sle";
                else if (condition == "sle")    condition = "sge";
                else if (condition == "ugt")    condition = "ult";
                else if (condition == "ult")    condition = "ugt";
                else if (condition == "uge")    condition = "ule";
                else if (condition == "ule")    condition = "uge";
            }

            // Negative the condition if comp_result is false
            if (!comp_result) {

                if      (condition == "gt")     condition = "le";
                else if (condition == "lt")     condition = "ge";
                else if (condition == "ge")     condition = "lt";
                else if (condition == "le")     condition = "gt";
                else if (condition == "sgt")    condition = "sle";
                else if (condition == "slt")    condition = "sge";
                else if (condition == "sge")    condition = "slt";
                else if (condition == "sle")    condition = "sgt";
                else if (condition == "ugt")    condition = "ule";
                else if (condition == "ult")    condition = "uge";
                else if (condition == "uge")    condition = "ult";
                else if (condition == "ule")    condition = "ugt";
                else if (condition == "eq")     condition = "ne";
                else if (condition == "ne")     condition = "eq";
            }

            // Find the input arugment that is stored in the comparison argument
            std::string arg_name;
            arg = arg.substr(1, arg.size()-1);
            for (int i=0; i<local_variables.size(); i++) {
                if (local_variables[i].first == arg) {
                    arg_name = local_variables[i].second;
                    break;
                }
            }

            // Modify the limit of the argument
            for (int i=0; i<input_names.size(); i++) {
                if (input_names[i] == arg_name) {
                    for (int j=0; j<input_limits[i].size(); j++) {
                        if (condition == "eq") {
                            if (input_limits[i][j] == value)    continue;
                        }
                        else if (condition == "ne") {
                            if (input_limits[i][j] != value)    continue;
                        }
                        else if (condition == "gt") {
                            if (input_limits[i][j] > value)     continue;
                        }
                        else if (condition == "lt") {
                            if (input_limits[i][j] < value)     continue;
                        }
                        else if (condition == "ge") {
                            if (input_limits[i][j] >= value)     continue;
                        }
                        else if (condition == "le") {
                            if (input_limits[i][j] <= value)     continue;
                        }
                        else if (condition == "sgt") {
                            if (input_limits[i][j] > value)     continue;
                        }
                        else if (condition == "slt") {
                            if (input_limits[i][j] < value)     continue;
                        }
                        else if (condition == "sge") {
                            if (input_limits[i][j] >= value)     continue;
                        }
                        else if (condition == "sle") {
                            if (input_limits[i][j] <= value)     continue;
                        }
                        else if (condition == "ugt") {
                            if (input_limits[i][j] > value)     continue;
                        }
                        else if (condition == "ult") {
                            if (input_limits[i][j] < value)     continue;
                        }
                        else if (condition == "uge") {
                            if (input_limits[i][j] >= value)     continue;
                        }
                        else if (condition == "ule") {
                            if (input_limits[i][j] <= value)     continue;
                        }

                        input_limits[i].erase( input_limits[i].begin()+j);
                        j--;
                    }
                    break;
                }
            }
        }
    }
}
