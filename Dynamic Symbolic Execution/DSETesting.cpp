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

#include <regex>
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
using namespace std;


class Node {
  public:
    bool visited_left;
    bool visited_right;
    std::string symbolic_value;
    std::string name;
    Node* left;
    Node* right;

    Node()
    {
        name = "";
        symbolic_value = "";
        visited_left = false;
        visited_right = false;
        left = NULL;
        right = NULL;
    }
};

LLVMContext &getGlobalContext() {
  static LLVMContext context;
  return context;
}

void generatePath(BasicBlock* BB);
std::string getSimpleNodeLabel(const BasicBlock *Node);
void analyze_instruction(Instruction* I);
bool comparison_analysis(std::string str);
bool tree_traverse();
Node* make_tree(BasicBlock* BB);
std::string analyze_instructions_operation(BasicBlock* BB);
// global variables
std::set<std::pair<std::string,BasicBlock*>>allBlocks;
std::set<std::pair<std::string,BasicBlock*>>visitedBlocks;
std::set<std::pair<std::string,BasicBlock*>> visitedBlocksInPath;
std::vector<std::pair<std::string, int>> input_variables;                  // a1, a2, ...
std::vector<std::pair<std::string, int>> all_variables;    // b, c, 0 (%0), 1 (%1), ...
std::vector<BasicBlock*> path_of_blocks;
std::string comparison_analysis_symbolic(std::string str);
std::string global_succ_name = "";
std::map<string, int> visit_number;
vector<Node*> node_list;
Node* main_root;

std::vector<std::vector<int>> dse_values;
std::vector<std::pair<std::string, std::string>> dse_variable_change;
// std::vector<std::string> fuzz_operation = {"negate", "change_digit", "add_remove_digit"};

void print_tree(Node* root){
    if (root == NULL) return;
    if (root->left && root->right){
        std::cout << "Root: " << root->symbolic_value << "       " << "left:" << root->left->symbolic_value << "    " << "Right:" << root->right->symbolic_value << std::endl;
    }
    else if (root->left){
        std::cout << "Root: " << root->symbolic_value << "       " << "left:" << root->left->symbolic_value << "    " << "Right:" << "Null" << std::endl;
    }
    else if (root->right) {
        std::cout << "Root: " << root->symbolic_value << "       " << "left:" << "Null" << "    " << "Right:" << root->right->symbolic_value << std::endl;
    }
    else{
        std::cout << "Root: " << root->symbolic_value << "       " << "left:" << "Null" << "    " << "Right:" << "Null" << std::endl;

    }
    print_tree(root->right);
    print_tree(root->left);
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos){
        return false;
    }

    str.replace(start_pos, from.length(), to);
    return true;
}


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
    for (auto &F: *M)
        if (strncmp(F.getName().str().c_str(),"main",4) == 0){
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            //std::cout << "Make Tree begins" << std::endl;
            Node* root = make_tree(BB);
            main_root = root;
            //print_tree(root);
            //std::cout << "Make Tree ends" << std::endl;
    }
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

        if (tree_traverse())
            break;
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

        for (auto node : node_list) {
            if (node->name == getSimpleNodeLabel(BB)) {
                if (succ_number == 0)
                    node->visited_left = true;
                else
                    node->visited_right = true;
            }
        }

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
            dse_values.push_back(input_vec);
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

std::string comparison_analysis_symbolic(std::string str){
// Analyze the comparison instruction
    // Get the line
    std::string str2;
    str2 = str;
    std::replace(str2.begin(), str2.end(), ',', ' ');
    std::stringstream ss(str2);
    std::istream_iterator<std::string> begin1(ss);
    std::istream_iterator<std::string> end1;
    std::vector<std::string> words(begin1, end1);

    // Get the condition
    std::string condition = words[words.size()-4];

    // Get the argument and the number and change the condition based on the order
    std::string first  = words[words.size()-2];
    std::string second = words[words.size()-1];
    // std::cout << "Compare:   " <<  first << " " << second << std::endl;

    // Return the result
    if (condition == "eq")
        return first + " == " + second;

    else if (condition == "ne")
        return first + " != " + second;

    else if (condition == "gt")
        return first + " > " + second;

    else if (condition == "lt")
        return first + " < " + second;

    else if (condition == "ge")
        return first + " >= " + second;

    else if (condition == "le")
        return first + " <= " + second;

    else if (condition == "sgt")
        return first + " > " + second;

    else if (condition == "slt")
        return first + " < " + second;

    else if (condition == "sge")
        return first + " >= " + second;

    else if (condition == "sle")
        return first + " <= " + second;

    else if (condition == "ugt")
        return first + " > " + second;

    else if (condition == "ult")
        return first + " < " + second;

    else if (condition == "uge")
        return first + " >= " + second;

    else if (condition == "ule")
        return first + " <= " + second;

    return "None";


}

std::string analyze_instructions_operation(BasicBlock* BB){

    std::string symbolic = BB->getName().str();

    for (auto &inst: reverse(*BB)) {
        // std::cout << symbolic << std::endl;
        std::string str;
        llvm::raw_string_ostream(str) << (inst);
        if (isa<BranchInst> (inst)) {
            continue; // not so important
        }
        else if (isa<CmpInst> (inst)) {
            symbolic = comparison_analysis_symbolic(str);
        }
        else if (isa<AllocaInst> (inst)){
            int pos = str.find("%");
            str = str.substr(pos, str.size()-pos);
            // std:: cout << str << std::endl;
            std::string variable_name = str.substr(1, str.find(" ")-1);

            if ((str[1] == 'a') && (str[2] >= '0') && (str[2] <= '9')) {
                std::vector<std::string> vect;
                std::pair <std::string, std::vector<std::string>> pair1= std::make_pair(str.substr(0, 3), vect);
                //std::cout << pair1.first << `::endl;
            }
        }
        else if (isa<LoadInst> (inst)) {

            int pos11 = str.find("%");
            int pos12 = str.find("=");
            std::string str1 = str.substr(pos11, pos12-pos11-1);
            std::string str2 = "%" + (std::string) (inst.getOperand(0))->getName();
            // std::cout << "load:  "  << str1 << "   " << str2 << std::endl;
            for (int i=0; i < dse_variable_change.size();i++) {
                if (dse_variable_change[i].second == str1){
                    dse_variable_change[i].second = str2;
                }
            }
             for (int i=0; i < dse_variable_change.size();i++) {
                if (dse_variable_change[i].first == str2){
                    str2 = dse_variable_change[i].second;
                }
            }
            replace(symbolic, str1, str2);
        }
        else if (isa<StoreInst> (inst)) {
            std::string str1;
            if (isa<ConstantInt>(inst.getOperand(0))) {
                str1 = std::to_string(dyn_cast<ConstantInt>(inst.getOperand(0))->getSExtValue());
            }

            else {
                int pos11 = str.find("%");
                int pos12 = str.find(",");
                str1 = str.substr(pos11, pos12-pos11);
            }

            std::string str2 = "%" + (std::string) (inst.getOperand(1))->getName();
            // std::cout << "Store" << str1 << "   " << str2 << std::endl;
            std::string alpahbet = "mnopqrstuvwxyz";
            std::pair<std::string, std::string> variable_change;
            if (alpahbet.find(str2[1]) != std::string::npos){
                variable_change.first = str2;
                variable_change.second = str1;
                dse_variable_change.push_back(variable_change);
            }

            replace(symbolic, str2, str1);
        }
        else if (isa<BinaryOperator> (inst)) {
            // split the instruction into words
            std::string str1;
            str1 = str;
            std::replace(str1.begin(), str1.end(), ',', ' ');
            std::stringstream ss(str1);
            std::istream_iterator<std::string> begin(ss);
            std::istream_iterator<std::string> end;
            std::vector<std::string> words(begin, end);
            int len = words.size();

            std::string first_operand;
            if (isa<ConstantInt>(inst.getOperand(0)))
                first_operand = std::to_string(dyn_cast<ConstantInt>(inst.getOperand(0))->getSExtValue());
            else {
                first_operand = "%" +  words[len-2].substr(1, words[len-2].size()-1);
            }
            std::string second_operand;
            if (isa<ConstantInt>(inst.getOperand(1)))
                second_operand = std::to_string(dyn_cast<ConstantInt>(inst.getOperand(1))->getSExtValue());
            else {
                second_operand = "%" + words[len-1].substr(1, words[len-1].size()-1);

            }

            int pos11 = str.find("%");
            int pos12 = str.find("=");
            std::string variable_str = str.substr(pos11, pos12-pos11-1);
            // std::cout << "operation" << "   " << first_operand << "    " << second_operand << "  " << variable_str << std::endl;

            // apply the opcode
            if (inst.getOpcode() == Instruction::Add)
                replace(symbolic, variable_str, "(" + first_operand + "+" + second_operand + ") ");
            else if (inst.getOpcode() == Instruction::Sub)
                replace(symbolic, variable_str, "(" + first_operand + "-" + second_operand + ") ");
            else if (inst.getOpcode() == Instruction::SDiv)
                replace(symbolic, variable_str, "(" + first_operand + "/" + second_operand + ") ");
            else if (inst.getOpcode() == Instruction::Mul)
                replace(symbolic, variable_str, "(" + first_operand + "*" + second_operand + ") ");
        }

    }

    return symbolic;
}

Node* make_tree(BasicBlock* BB){
    std::string name = BB->getName().str();

    std::string symbolic = analyze_instructions_operation(BB);

    Node* current_node = new Node();
    current_node->symbolic_value = symbolic;

    current_node->name = BB->getName().str();
    node_list.push_back(current_node);

    if (visit_number.find(name) == visit_number.end()) {
        visit_number[name] = 1;
    }
    else {
        if (visit_number[name] >= 10)
            return current_node;
        else
            visit_number[name] += 1;
    }

    const Instruction *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();
    if (NSucc == 0)
        return current_node;

    else if (NSucc == 1){
        BasicBlock *Succ = TInst->getSuccessor(0);
        current_node->right = make_tree(Succ);
        current_node->left = NULL;
    }

    else {
        BasicBlock *right_Succ = TInst->getSuccessor(1);
        BasicBlock *left_Succ = TInst->getSuccessor(0);
        current_node->right = make_tree(right_Succ);
        current_node->left = make_tree(left_Succ);
    }
    return current_node;

}


bool tree_traverse() {

    bool full_coverage = true;
    for (auto node : node_list) {
        if ((node->right != NULL) && (node->left != NULL)) {
            //std::cout << "\t\t" << node->visited_left << "\t" << node->visited_right<< endl;
            if ((!(node->visited_left)) || (!(node->visited_right))) {
                std::string str1 = node->symbolic_value;
                std::string str2;
                for (int i=0; i<input_variables.size(); i++) {

                    if (str1.find("%" + input_variables[i].first) != std::string::npos) {

                        int indx = 0;
                        for (int j=0; j<all_variables.size(); j++) {
                            if (all_variables[j].first == input_variables[i].first) {
                                indx = j;
                                break;
                            }
                        }

                        std::stringstream ss(str1);
                        std::istream_iterator<std::string> begin1(ss);
                        std::istream_iterator<std::string> end1;
                        std::vector<std::string> words(begin1, end1);

                        std::string condition = words[words.size()-2];
                        std::string first  = words[words.size()-0];
                        std::string second = words[words.size()-1];

                        unsigned rnd = 10+ (std::rand() / (RAND_MAX/199));

                        if (!(node->visited_left)) {
                            if (condition == "==")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second);
                            else if (condition == "!=")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)+rnd;
                            else if (condition == ">")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)+rnd;
                            else if (condition == ">=")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)+rnd;
                            else if (condition == "<")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)-rnd;
                            else if (condition == "<=")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)-rnd;
                        }
                        else {
                            if (condition == "==")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)+rnd;
                            else if (condition == "!=")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second);
                            else if (condition == ">")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)-rnd;
                            else if (condition == ">=")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)-rnd;
                            else if (condition == "<")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)+rnd;
                            else if (condition == "<=")
                                all_variables[indx].second = input_variables[i].second = std::stoi(second)+rnd;
                        }
                    }
                }
                full_coverage = false;
                break;
            }
        }
    }
    return (full_coverage);
}
