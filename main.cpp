#include <iostream>
#include <cassert>
#include "simpleparser/Tokenizer.hpp"
#include "simpleparser/Parser.hpp"
#include "bytecodeinterpreter/BytecodeInterpreter.hpp"
#include "bytecodeinterpreter/Instruction.hpp"

using namespace std;
using namespace simpleparser;
using namespace bytecodeinterpreter;

void GenerateCodeForFunction(const FunctionDefinition &currFunc, vector<Instruction> compiledCode) {
    // First, create all variables based on variable declarations.
    int numIntVariables = 0;
    std::vector<int16_t> returnCmdJumpInstructions;

    for (const auto& currStmt : currFunc.mStatements) {
        switch(currStmt.mKind) {
            case StatementKind::VARIABLE_DECLARATION:
                switch (currStmt.mType.mType) {
                    case VOID:
                        break;
                    case INT8:
                        break;
                    case UINT8:
                        break;
                    case INT32: {
                        int32_t initialValue = 0;
                        if (!currStmt.mParameters.empty()) {
                            const auto& initialValueParsed = currStmt.mParameters[0];
                            if (initialValueParsed.mKind == StatementKind::LITERAL) {
                                assert(initialValueParsed.mType.mType == currStmt.mType.mType);
                                initialValue = stoi(initialValueParsed.mName);
                            }
                        }
                        ++numIntVariables;
                        compiledCode.push_back(Instruction{bytecodeinterpreter::PUSH_INT, 0, int16_t(initialValue)});
                        break;
                    }
                    case UINT32:
                        break;
                    case DOUBLE:
                        break;
                    case STRUCT:
                        break;
                }
                break;
            default:
                break;
        }
    }

    // Actually generate code for the non-literal-variable statements:
    for (const auto& currStmt : currFunc.mStatements) {
        switch(currStmt.mKind) {
            case StatementKind::VARIABLE_DECLARATION:
                break;
            case StatementKind::FUNCTION_CALL:
                break;
            case StatementKind::LITERAL:
                break;
            case StatementKind::OPERATOR_CALL:
                break;
        }
    }

    // Clean up stack space reserved for variables:
    size_t cleanupCodeOffset = compiledCode.size() * sizeof(Instruction);
    for (auto returnCmdJumpInstructionIndex : returnCmdJumpInstructions) {
        compiledCode[returnCmdJumpInstructionIndex].p2 = cleanupCodeOffset;
    }
    for (int x = 0; x < numIntVariables; ++x) {
        compiledCode.push_back(Instruction{bytecodeinterpreter::POP_INT, 0, 0});
    }
    compiledCode.push_back(Instruction{bytecodeinterpreter::RETURN, 0, 0});
}

int main(int argc, const char* argv[]) {
    try {
        std::cout << "ParserAndCompiler 0.1\n" << endl;

        FILE *fh = fopen("D:\\CLionProjects\\ParserAndCompiler\\simpleparser\\test.myc", "r");
        if (!fh) { cerr << "Can't find file." << endl; }
        fseek(fh, 0, SEEK_END);
        size_t fileSize = ftell(fh);
        fseek(fh, 0, SEEK_SET);
        string fileContents(fileSize, ' ');
        fread(fileContents.data(), 1, fileSize, fh);

        cout << fileContents << endl << endl;

        Tokenizer tokenizer;
        vector<Token> tokens = tokenizer.parse(fileContents);

        for(Token currToken : tokens) {
            currToken.debugPrint();
        }

        Parser parser;
        parser.parse(tokens);

        parser.debugPrint();

        vector<Instruction> compiledCode;

        map<string, FunctionDefinition> functions = parser.GetFunctions();
        for (const pair<string,FunctionDefinition>& currFunc : functions) {
            GenerateCodeForFunction(currFunc.second, compiledCode);
        }

        Instruction code[] = {
                Instruction{PUSH_INT, 0, 0}, // x
                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, 0}, // load x
                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, -2}, // load parameter 1
                Instruction{COMP_INT_LT, 0, 0}, // x < 10
                Instruction{JUMP_BY_IF_ZERO, 0, 10}, // if x >= 10 bail!

                Instruction{PUSH_INT, 0, 4000},
                Instruction{PUSH_INT, 0, 1042},
                Instruction{ADD_INT, 0, 0},
                Instruction{PRINT_INT, 0, 0},

                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, 0}, // load x
                Instruction{PUSH_INT, 0, 1}, // load 1
                Instruction{ADD_INT, 0, 0}, // x + 1
                Instruction{STORE_INT_BASEPOINTER_RELATIVE, 0, 0}, // x = (x + 1)
                Instruction{JUMP_BY, 0, -12}, // loop back to condition.

                Instruction{PUSH_INT, 0, 0}, // load 0.
                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, -2}, // load parameter 1.
                Instruction{COMP_INT_LT, 0, 0}, // 0 < parameter 1.
                Instruction{JUMP_BY_IF_ZERO, 0, 8}, // jump past recursive call.
                Instruction{PUSH_INT, 0, 0}, // reserve space for result.
                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, -2}, // load parameter 1.
                Instruction{PUSH_INT, 0, -1}, // load -1.
                Instruction{ADD_INT, 0, 0}, // parameter 1 - 1
                Instruction{CALL, 0, -22}, // call ourselves.
                Instruction{POP_INT, 0, 0}, // pop parameter.
                Instruction{POP_INT, 0, 0}, // pop result.

                Instruction{PUSH_INT, 0, 42}, // load 42
                Instruction{STORE_INT_BASEPOINTER_RELATIVE, 0, -3}, // returnValue = 42
                Instruction{JUMP_BY, 0, 1}, // jump to function's epilog + actually return.
                Instruction{POP_INT, 0, 0}, // delete 'x'.
                Instruction{RETURN, 0, 0}
        };

        int16_t resultValue = 0;
        BytecodeInterpreter::Run(code, { 3 }, &resultValue);

        cout << "\nResult: " << resultValue << "\ndone." << endl;

    } catch (exception& err) {
        cerr << "Error: " << err.what() << endl;
        return 2;
    } catch (...) {
        cerr << "Unknown Error." << endl;
        return 1;
    }

    return 0;
}
