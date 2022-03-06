#include <iostream>
#include <cassert>
#include "simpleparser/Tokenizer.hpp"
#include "simpleparser/Parser.hpp"
#include "bytecodeinterpreter/BytecodeInterpreter.hpp"
#include "bytecodeinterpreter/Instruction.hpp"

using namespace std;
using namespace simpleparser;
using namespace bytecodeinterpreter;

struct Parameter {
    string mName;
    size_t mIndex;
};

struct CompiledFunction {
    size_t mInstructionOffset;
    size_t mNumArguments;
    bool mReturnsSomething;
};

void GenerateCodeForStatement(const Statement & currStmt,
                              const std::map<string, int16_t> &variableOffsets,
                              map<string, Parameter> parameters,
                              std::vector<int16_t> &returnCmdJumpInstructions,
                              vector<Instruction> &compiledCode,
                              map<string, CompiledFunction> &functionNameToInstruction) {
    switch(currStmt.mKind) {
        case StatementKind::VARIABLE_DECLARATION:
            switch (currStmt.mType.mType) {
                case INT32: {
                    if (!currStmt.mParameters.empty()) {
                        const auto& initialValueParsed = currStmt.mParameters[0];
                        if (initialValueParsed.mKind != StatementKind::LITERAL) {
                            auto foundVar = variableOffsets.find(currStmt.mName);
                            if (foundVar == variableOffsets.end()) {
                                throw runtime_error(string("Unknown variable \"") + currStmt.mName + "\".");
                            }
                            GenerateCodeForStatement(initialValueParsed, variableOffsets, parameters, returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
                            compiledCode.push_back(Instruction{bytecodeinterpreter::STORE_INT_BASEPOINTER_RELATIVE, 0, foundVar->second});
                        }
                    }
                    break;
                }
            }
            break;

        case StatementKind::FUNCTION_CALL:
            if (currStmt.mName == "return") {
                if (currStmt.mParameters.size() != 1) {
                    throw runtime_error("Function \"return\" expects a single parameter.");
                }
                GenerateCodeForStatement(currStmt.mParameters[0], variableOffsets, parameters, returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
                compiledCode.push_back(Instruction{bytecodeinterpreter::STORE_INT_BASEPOINTER_RELATIVE, 0, int16_t(-2 - parameters.size())});
                returnCmdJumpInstructions.push_back(compiledCode.size());
                compiledCode.push_back(Instruction{bytecodeinterpreter::JUMP_BY, 0, 0});
            } else if (currStmt.mName == "printNum") {
                if (currStmt.mParameters.size() != 1) {
                    throw runtime_error("Function \"printNum\" expects a single parameter.");
                }
                GenerateCodeForStatement(currStmt.mParameters[0], variableOffsets, parameters,
                                         returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
                compiledCode.push_back(Instruction{bytecodeinterpreter::PRINT_INT, 0, 0});
            } else {
                auto foundFunction = functionNameToInstruction.find(currStmt.mName);
                if (foundFunction == functionNameToInstruction.end()) {
                    throw runtime_error(string("Unknown function \"") + currStmt.mName + "\" called.");
                }

                if (foundFunction->second.mReturnsSomething) {
                    compiledCode.push_back(Instruction{bytecodeinterpreter::PUSH_INT, 0, 0});
                }

                if (foundFunction->second.mNumArguments != currStmt.mParameters.size()) {
                    throw runtime_error(string("Function ") + currStmt.mName + " requires " + to_string(foundFunction->second.mNumArguments) + " arguments, but received " + to_string(currStmt.mParameters.size()) + ".");
                }

                for (auto &currParam : currStmt.mParameters) {
                    GenerateCodeForStatement(currParam, variableOffsets, parameters, returnCmdJumpInstructions,
                                             compiledCode, functionNameToInstruction);
                }
                size_t relativeJumpAddress = foundFunction->second.mInstructionOffset - compiledCode.size();
                compiledCode.push_back(Instruction{bytecodeinterpreter::CALL, 0, int16_t(relativeJumpAddress)});
                for (size_t x = currStmt.mParameters.size(); x > 0; --x) {
                    compiledCode.push_back(Instruction{bytecodeinterpreter::POP_INT, 0, 0});
                }
            }
            break;
        case StatementKind::LITERAL:
            switch (currStmt.mType.mType) {
                case VOID:
                    break;
                case INT8:
                    break;
                case UINT8:
                    break;
                case INT32: {
                    int32_t initialValue = stoi(currStmt.mName);
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
        case StatementKind::OPERATOR_CALL:
            if (currStmt.mParameters.size() != 2) {
                throw runtime_error(string("Wrong number of parameters passed to operator \"") + currStmt.mName + "\".");
            }
            if (currStmt.mName == "+" || currStmt.mName == "<") {
                Opcode op = ADD_INT;
                if (currStmt.mName == "<") {
                    op = COMP_INT_LT;
                };
                for (auto& currParam : currStmt.mParameters) {
                    GenerateCodeForStatement(currParam, variableOffsets, parameters, returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
                }
                compiledCode.push_back(Instruction{op, 0, 0});
            } else if (currStmt.mName == "=") {
                auto foundVar = variableOffsets.find(currStmt.mParameters[0].mName);
                if (foundVar == variableOffsets.end()) {
                    throw runtime_error(string("Unknown variable \"") + currStmt.mParameters[0].mName + "\".");
                }
                GenerateCodeForStatement(currStmt.mParameters[1], variableOffsets, parameters, returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
                compiledCode.push_back(Instruction{bytecodeinterpreter::STORE_INT_BASEPOINTER_RELATIVE, 0, foundVar->second});
            }
            break;
        case StatementKind::VARIABLE_NAME: {
            auto foundVar = variableOffsets.find(currStmt.mName);
            if (foundVar != variableOffsets.end()) {
                compiledCode.push_back(Instruction{bytecodeinterpreter::LOAD_INT_BASEPOINTER_RELATIVE, 0, foundVar->second});
                break;
            }
            auto foundParam = parameters.find(currStmt.mName);
            if (foundParam != parameters.end()) {
                compiledCode.push_back(Instruction{bytecodeinterpreter::LOAD_INT_BASEPOINTER_RELATIVE, 0, int16_t(-1 -parameters.size() + foundParam->second.mIndex)});
                break;
            }
            throw runtime_error(string("Unknown variable \"") + currStmt.mParameters[0].mName + "\".");
            break;
        }
        case StatementKind::WHILE_LOOP: {
            size_t conditionOffset = compiledCode.size();
            GenerateCodeForStatement(currStmt.mParameters[0], variableOffsets, parameters, returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
            size_t conditionFalseJumpInstructionOffset = compiledCode.size();
            compiledCode.push_back(Instruction{bytecodeinterpreter::JUMP_BY_IF_ZERO, 0, 0});
            for (auto stmt = currStmt.mParameters.begin() + 1; stmt != currStmt.mParameters.end(); ++stmt ) {
                GenerateCodeForStatement(*stmt, variableOffsets, parameters, returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
            }
            compiledCode.push_back(Instruction{bytecodeinterpreter::JUMP_BY, 0, int16_t(conditionOffset - compiledCode.size())});
            compiledCode[conditionFalseJumpInstructionOffset].p2 = int16_t(compiledCode.size() - conditionFalseJumpInstructionOffset);
            break;
        }
    }
}

void GenerateCodeForFunction(const FunctionDefinition &currFunc, vector<Instruction> &compiledCode,
                             map<string, CompiledFunction> &functionNameToInstruction) {
    // First, create all variables based on variable declarations.
    int numIntVariables = 0;
    vector<int16_t> returnCmdJumpInstructions;
    map<string, int16_t> variableOffsets;
    map<string, Parameter> parameters;

    functionNameToInstruction[currFunc.mName] = CompiledFunction{
        compiledCode.size(),
        currFunc.mParameters.size(),
        currFunc.mReturnsSomething
    };

    size_t paramIndex = 0;
    for (auto& currParamDef : currFunc.mParameters) {
        parameters[currParamDef.mName] = Parameter{currParamDef.mName, paramIndex++};
    }

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
                        variableOffsets[currStmt.mName] = numIntVariables;
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
        GenerateCodeForStatement(currStmt, variableOffsets, parameters, returnCmdJumpInstructions, compiledCode, functionNameToInstruction);
    }

    // Clean up stack space reserved for variables:
    size_t cleanupCodeOffset = compiledCode.size();
    for (auto returnCmdJumpInstructionIndex : returnCmdJumpInstructions) {
        compiledCode[returnCmdJumpInstructionIndex].p2 = cleanupCodeOffset - returnCmdJumpInstructionIndex;
    }
    for (int x = 0; x < numIntVariables; ++x) {
        compiledCode.push_back(Instruction{bytecodeinterpreter::POP_INT, 0, 0});
    }
    compiledCode.push_back(Instruction{bytecodeinterpreter::RETURN, 0, 0});
}

int main(int argc, const char* argv[]) {
    try {
        std::cout << "ParserAndCompiler 0.1\n" << endl;

        if (argc < 2) {
            throw runtime_error("First argument must be script file to run.");
        }

        FILE *fh = fopen(argv[1], "r");
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
        map<string,CompiledFunction> functionNameToInstruction;
        for (const pair<string,FunctionDefinition>& currFunc : functions) {
            GenerateCodeForFunction(currFunc.second, compiledCode, functionNameToInstruction);
        }

//        Instruction code[] = {
//                Instruction{PUSH_INT, 0, 0}, // x
//                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, 0}, // load x
//                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, -2}, // load parameter 1
//                Instruction{COMP_INT_LT, 0, 0}, // x < 10
//                Instruction{JUMP_BY_IF_ZERO, 0, 10}, // if x >= 10 bail!
//
//                Instruction{PUSH_INT, 0, 4000},
//                Instruction{PUSH_INT, 0, 1042},
//                Instruction{ADD_INT, 0, 0},
//                Instruction{PRINT_INT, 0, 0},
//
//                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, 0}, // load x
//                Instruction{PUSH_INT, 0, 1}, // load 1
//                Instruction{ADD_INT, 0, 0}, // x + 1
//                Instruction{STORE_INT_BASEPOINTER_RELATIVE, 0, 0}, // x = (x + 1)
//                Instruction{JUMP_BY, 0, -12}, // loop back to condition.
//
//                Instruction{PUSH_INT, 0, 0}, // load 0.
//                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, -2}, // load parameter 1.
//                Instruction{COMP_INT_LT, 0, 0}, // 0 < parameter 1.
//                Instruction{JUMP_BY_IF_ZERO, 0, 8}, // jump past recursive call.
//                Instruction{PUSH_INT, 0, 0}, // reserve space for result.
//                Instruction{LOAD_INT_BASEPOINTER_RELATIVE, 0, -2}, // load parameter 1.
//                Instruction{PUSH_INT, 0, -1}, // load -1.
//                Instruction{ADD_INT, 0, 0}, // parameter 1 - 1
//                Instruction{CALL, 0, -22}, // call ourselves.
//                Instruction{POP_INT, 0, 0}, // pop parameter.
//                Instruction{POP_INT, 0, 0}, // pop result.
//
//                Instruction{PUSH_INT, 0, 42}, // load 42
//                Instruction{STORE_INT_BASEPOINTER_RELATIVE, 0, -3}, // returnValue = 42
//                Instruction{JUMP_BY, 0, 1}, // jump to function's epilog + actually return.
//                Instruction{POP_INT, 0, 0}, // delete 'x'.
//                Instruction{RETURN, 0, 0}
//        };

        int16_t resultValue = 0;
        size_t mainFunctionOffset = SIZE_MAX;
        auto foundFunction = functionNameToInstruction.find("main");
        if (foundFunction == functionNameToInstruction.end()) {
            throw runtime_error("Couldn't find main function.");
        }
        BytecodeInterpreter::Run(compiledCode.data() + foundFunction->second.mInstructionOffset,
                                 { 3 }, &resultValue);

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



