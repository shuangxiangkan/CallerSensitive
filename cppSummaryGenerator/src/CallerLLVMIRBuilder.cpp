#include "CallerLLVMIRBuilder.h"
#include "SVF-FE/LLVMUtil.h"
#include "WPA/Andersen.h"
#include <fstream>
#include <string>

using namespace SVF;

// CallerLLVMIRBuilder *CallerLLVMIRBuilder::callerSen = nullptr;

cJSON *CallerLLVMIRBuilder::parseCallerJson(std::string path)
{
    FILE *file = NULL;
    std::string FILE_NAME = path;
    file = fopen(FILE_NAME.c_str(), "r");
    if (file == NULL)
    {
        assert(false && "Open Caller Json file fail!");
        return nullptr;
    }

    struct stat statbuf;
    stat(FILE_NAME.c_str(), &statbuf);
    u32_t fileSize = statbuf.st_size;

    char *jsonStr = (char *)malloc(sizeof(char) * fileSize + 1);
    memset(jsonStr, 0, fileSize + 1);

    u32_t size = fread(jsonStr, sizeof(char), fileSize, file);
    if (size == 0)
    {
        assert(false && "Read Caller Json file fails!");
        return nullptr;
    }
    fclose(file);

    // convert json string to json pointer variable
    cJSON *root = cJSON_Parse(jsonStr);
    if (!root)
    {
        free(jsonStr);
        return nullptr;
    }
    free(jsonStr);
    return root;
}

CallerLLVMIRBuilder::JavaDataType CallerLLVMIRBuilder::getBasicType(std::string type)
{
    std::map<std::string, JavaDataType>::const_iterator it = javaBasicTypes.find(type);
    if (it == javaBasicTypes.end())
        return JAVA_NULL;
    else
        return it->second;
}

Type *CallerLLVMIRBuilder::getType(std::string type)
{
    if (javaPyTypes.count(type))
        return irBuilder.getInt8PtrTy();
    JavaDataType ctype = getBasicType(type);
    switch (ctype)
    {
    case JBOOLEAN:
        return irBuilder.getInt1Ty();
    case JBYTE:
        return irBuilder.getInt8Ty();
    case JCHAR:
        return irBuilder.getInt16Ty();
    case JSHORT:
        return irBuilder.getInt16Ty();
    case JINT:
        return irBuilder.getInt32Ty();
    case JLONG:
        return irBuilder.getInt64Ty();
    case JFLOAT:
        return irBuilder.getFloatTy();
    case JDOUBLE:
        return irBuilder.getDoubleTy();
    case VOID:
        return irBuilder.getVoidTy();
    default:
        assert(false && "illegal Java Type!!!");
        break;
    }
    return nullptr;
}

std::vector<Type *> CallerLLVMIRBuilder::getParams(std::vector<std::string> args)
{
    std::vector<Type *> params;
    for (std::vector<std::string>::iterator it = args.begin(); it != args.end(); ++it)
    {
        params.push_back(getType(*it));
    }
    return params;
}

void CallerLLVMIRBuilder::output2file(std::string outPath)
{
    std::error_code EC;
    raw_fd_ostream output(outPath, EC);
    WriteBitcodeToFile(*module, output);
}

Function *CallerLLVMIRBuilder::functionDeclarationIR(std::string funName, std::string ret, std::vector<std::string> args)
{
    Type *retType = getType(ret);
    std::vector<Type *> params = getParams(args);
    FunctionType *type = FunctionType::get(retType, params, false);
    return Function::Create(type, GlobalValue::ExternalLinkage, funName, module);
}

std::map<std::string, Value *> CallerLLVMIRBuilder::allocaStore(std::vector<std::string> params, Function *func)
{
    std::string arg = "arg";
    std::map<std::string, Value *> callerArgsMap;
    for (u32_t i = 0; i < params.size(); i++)
        callerArgsMap[arg + std::to_string(i)] = irBuilder.CreateAlloca(getType(params[i]));
    for (u32_t i = 0; i < params.size(); i++)
        irBuilder.CreateStore(func->getArg(i), callerArgsMap[arg + std::to_string(i)]);
    return callerArgsMap;
}

Value *CallerLLVMIRBuilder::allocaStoreLoad(std::string typeStr, Value *value)
{
    Type *type = getType(typeStr);
    Value *alloca = irBuilder.CreateAlloca(type, nullptr);
    irBuilder.CreateStore(value, alloca);
    return irBuilder.CreateLoad(type, alloca);
}

Value *CallerLLVMIRBuilder::getCalleeArgs(std::string typeStr, std::string name, std::map<std::string, Value *> callerArgs, std::map<std::string, Value *> rets)
{
    Type *type = getType(typeStr);
    if (strstr(name.c_str(), "arg"))
    {
        return irBuilder.CreateLoad(type, callerArgs[name]);
    }
    else if (strstr(name.c_str(), "ret"))
    {
        return allocaStoreLoad(typeStr, rets[name]);
    }
    return nullptr;
}

u32_t CallerLLVMIRBuilder::getNumber(std::string s)
{

    u32_t argPos = -1;
    u32_t start = 0;
    while (start < s.size() && isalpha(s[start]))
        start++;
    u32_t end = start + 1;
    while (end < s.size() && isdigit(s[end]))
        end++;
    std::string digitStr = s.substr(start, end - start);
    argPos = atoi(digitStr.c_str());
    return argPos;
}

std::string CallerLLVMIRBuilder::getJsonFile(int &argc, char **argv)
{
    for (int i = 0; i < argc; ++i)
    {
        std::string argument(argv[i]);
        // File is a .json file not a IR file
        if (strstr(argument.c_str(), ".json") && !LLVMUtil::isIRFile(argument))
        {
            for (int j = i; j < argc && j + 1 < argc; ++j)
                argv[j] = argv[j + 1];
            argc--;
            return argument;
        }
    }
    return nullptr;
}

void CallerLLVMIRBuilder::callerIRCreate(std::string jsonPath, std::string irPath)
{
    module = new Module("Callers", llvmContext);
    Function *callerFun = nullptr;
    Function *calleeFun = nullptr;
    std::string callerRet;
    std::vector<std::string> callerParams;
    std::map<std::string, Value *> callerArgs;
    std::map<std::string, Value *> rets;
    std::string path = jsonPath;
    cJSON *root = parseCallerJson(path);
    cJSON *callerJson = root->child;
    while (callerJson)
    {
        cJSON *item = cJSON_GetObjectItemCaseSensitive(root, callerJson->string);
        if (item != NULL)
        {
            cJSON *obj = item->child;
            if (strcmp(obj->string, "return") == 0)
                callerRet = obj->valuestring;
            obj = obj->next;
            if (strcmp(obj->string, "parameters") == 0)
            {
                cJSON *param = obj->child;
                while (param)
                {
                    callerParams.push_back(param->valuestring);
                    param = param->next;
                }
            }
            callerFun = functionDeclarationIR(callerJson->string, callerRet, callerParams);
            BasicBlock *entryBB = BasicBlock::Create(llvmContext, "entry", callerFun);
            irBuilder.SetInsertPoint(entryBB);
            callerArgs = allocaStore(callerParams, callerFun);
            u32_t index = 1;
            while (obj)
            {
                if (strstr(obj->string, "CopyStmt"))
                {
                    cJSON *stmt = obj->child;
                    std::string src, dst;
                    if (strcmp(stmt->string, "src") == 0)
                        src = stmt->valuestring;
                    stmt = stmt->next;
                    if (strcmp(stmt->string, "dst") == 0)
                        dst = stmt->valuestring;
                    if (strcmp(src.c_str(), "void") == 0)
                    {
                        irBuilder.CreateRetVoid();
                    }
                    else if (strstr(src.c_str(), "ret"))
                    {
                        Value *retV = allocaStoreLoad(callerRet, rets[src]);
                        irBuilder.CreateRet(retV);
                    }
                }
                else if (strstr(obj->string, "callee"))
                {
                    std::string calleeName;
                    std::string calleeRet;
                    std::vector<std::string> calleeParams;
                    std::vector<Value *> putsargs;
                    cJSON *callee = obj->child;
                    if (strcmp(callee->string, "name") == 0)
                        calleeName = callee->valuestring;
                    callee = callee->next;
                    if (strcmp(callee->string, "return") == 0)
                        calleeRet = callee->valuestring;
                    callee = callee->next;
                    if (strcmp(callee->string, "parameters") == 0)
                    {
                        cJSON *item = callee->child;
                        while (item)
                        {
                            calleeParams.push_back(item->valuestring);
                            item = item->next;
                        }
                    }
                    calleeFun = functionDeclarationIR(calleeName, calleeRet, calleeParams);
                    callee = callee->next;
                    if (strcmp(callee->string, "arguments") == 0)
                    {
                        cJSON *item = callee->child;
                        u32_t i = 0;
                        while (item)
                        {
                            putsargs.push_back(getCalleeArgs(calleeParams[i], item->valuestring, callerArgs, rets));
                            item = item->next;
                            i++;
                        }
                    }
                    ArrayRef<Value *> argRef(putsargs);
                    Value *ret = irBuilder.CreateCall(calleeFun, argRef);
                    std::string retStr = "ret";
                    rets[retStr + std::to_string(index)] = ret;
                    index++;
                    callee = callee->next;
                }
                obj = obj->next;
            }
        }
        callerJson = callerJson->next;
    }
    std::string outPath = "./Callers.ll";
    // Output call IR file
    output2file(outPath);
    std::string resultFile = "./caller_callee.ll";
    // Link caller.ll and callee.ll
    std::string linkCommand = PROJECT_PATH + std::string("/llvm-13.0.0.obj/bin/llvm-link ") + outPath + std::string(" ") + irPath + std::string(" -o ") + resultFile;
    if (system(linkCommand.c_str()) == -1)
        assert(false && "Link caller.ll and callee.ll fails");
}

// void CallerLLVMIRBuilder::getCallees(std::string jsonPath)
// {
//     cJSON *root = parseCallerJson(jsonPath);
//     cJSON *callerJson = root->child;
//     while (callerJson)
//     {
//         cJSON *item = cJSON_GetObjectItemCaseSensitive(root, callerJson->string);
//         if (item != NULL)
//         {
//             cJSON *obj = item->child;
//             while (obj)
//             {
//                 if (obj && strstr(obj->string, "callee"))
//                 {
//                     cJSON *callee = obj->child;
//                     if (strcmp(callee->string, "name") == 0)
//                         CallerLLVMIRBuilder::callees.push_back(callee->valuestring);
//                 }
//                 obj = obj->next;
//             }
//         }
//         callerJson = callerJson->next;
//     }
// }

// u32_t CallerSensitive::getAliasedParamsPosition(const SVFIR::SVFVarList &funArgList, const PointsTo &pts, ConstraintGraph *consCG)
// {
//     u32_t argIndex = 0;
//     SVFIR::SVFVarList::const_iterator funArgIt = funArgList.begin(), funArgEit = funArgList.end();
//     for (; funArgIt != funArgEit; funArgIt++)
//     {
//         const PAGNode *fun_arg = *funArgIt;
//         ConstraintNode *argNode = consCG->getConstraintNode(fun_arg->getId());
//         for (auto eit = argNode->getCopyInEdges().begin(); eit != argNode->getCopyInEdges().end(); eit++)
//         {
//             ConstraintNode *srcNode = (*eit)->getSrcNode();
//             for (auto it = srcNode->getAddrInEdges().begin(); it != srcNode->getAddrInEdges().end(); it++)
//             {
//                 NodeID argDummyNode = (*it)->getSrcID();
//                 std::cout << "DummyNodeId: " << argDummyNode << std::endl;
//                 if (pts.test((*it)->getSrcID()))
//                     return argIndex;
//             }
//         }
//         argIndex++;
//     }
//     return -1;
// }

// void CallerSensitive::aliasedCallerArgsAndRet(const SVFFunction& fun, AndersenBase* ander, CMethod *cm)
// {
//     const PointsTo &pts = ander->getPts(ander->getPAG()->getReturnNode(&fun));
//     for (u32_t j = 2; j < fun.arg_size(); ++j)
//     {
//         const Value *callerArg = fun.getArg(j);
//         ConstraintNode *argNode = ander->getConstraintGraph()->getConstraintNode(ander->getPAG()->getValueNode(callerArg));
//         for (auto eit = argNode->getCopyInEdges().begin(); eit != argNode->getCopyInEdges().end(); eit++)
//         {
//             ConstraintNode *srcNode = (*eit)->getSrcNode();
//             for (auto it = srcNode->getAddrInEdges().begin(); it != srcNode->getAddrInEdges().end(); it++)
//             {
//                 if (pts.test((*it)->getSrcID()))
//                 {
//                     cm->isRetTained = true;
//                     cm->tainedArg = j - 2;
//                     // std::cout << "----------------------- aliased with " << j - 2 << " -----------------------" << std::endl;
//                     return;
//                 }
//             }
//         }
//     }
// }

// void CallerSensitive::JavaFieldsAsCallBackArgs(std::vector<JavaField *> javaFields, CallSite callee, AndersenBase *ander, JavaMethod *jm)
// {
//     for (u32_t i = 2; i < callee.arg_size(); ++i)
//     {
//         NodeID javaMethodArgID = ander->getPAG()->getValueNode(callee.getArgument(i));
//         for (std::vector<JavaField *>::iterator it = javaFields.begin(); it != javaFields.end(); ++it)
//         {
//             NodeID javaFieldRetID = (*it)->fieldRetId;
//             if (ander->alias(javaMethodArgID, javaFieldRetID))
//             {
//                 jm->taintedArgs[i - 3] = (*it)->fieldName;
//             }
//         }
//     }
// }

// void CallerSensitive::aliasedCallerAndCalleeArgs(const SVFFunction &fun, CallSite callee, AndersenBase *ander, JavaMethod *jm)
// {
//     for (u32_t i = 2; i < callee.arg_size(); ++i)
//     {
//         const PointsTo &pts = ander->getPts(ander->getPAG()->getValueNode(callee.getArgument(i)));
//         for (u32_t j = 2; j < fun.arg_size(); ++j)
//         {
//             const Value *callerArg = fun.getArg(j);
//             ConstraintNode *argNode = ander->getConstraintGraph()->getConstraintNode(ander->getPAG()->getValueNode(callerArg));
//             for (auto eit = argNode->getCopyInEdges().begin(); eit != argNode->getCopyInEdges().end(); eit++)
//             {
//                 ConstraintNode *srcNode = (*eit)->getSrcNode();
//                 for (auto it = srcNode->getAddrInEdges().begin(); it != srcNode->getAddrInEdges().end(); it++)
//                 {
//                     if (pts.test((*it)->getSrcID()))
//                     {
//                         // jm->taintedArgs[i - 3] = j - 2;
//                         jm->taintedArgs[i - 3] = std::to_string(j - 2);
//                         // std::cout << "---------------" << i - 3 << "-----------------" << std::endl;
//                         // std::cout << "---------------" << j - 2 << "-----------------" << std::endl;
//                     }
//                 }
//             }
//         }
//     }
// }
// CallerSensitive::JavaField *CallerSensitive::getCalledJavaField(std::vector<CallSite> fieldIdcs, CallSite cs, AndersenBase *ander)
// {
//     NodeID callerSecArgID = ander->getPAG()->getValueNode(cs.getArgument(2));
//     for (std::vector<CallSite>::const_iterator it = fieldIdcs.begin(); it != fieldIdcs.end(); it++)
//     {
//         NodeID calleeRetID = ander->getPAG()->getValueNode(it->getInstruction());
//         // if (callerSecArgID == calleeRetID)
//         if (ander->alias(callerSecArgID, calleeRetID))
//         {
//             JavaField *jf = new JavaField();
//             for (int i = 2; i < 4; i++)
//             {
//                 const GetElementPtrInst *gepinst = SVFUtil::dyn_cast<GetElementPtrInst>(it->getArgument(i));
//                 const Constant *arrayinst = SVFUtil::dyn_cast<Constant>(gepinst->getOperand(0));
//                 const ConstantDataArray *cxtarray = SVFUtil::dyn_cast<ConstantDataArray>(arrayinst->getOperand(0));
//                 const std::string vthdcxtstring = cxtarray->getAsCString().str();
//                 if (i == 2)
//                 {
//                     jf->fieldName = vthdcxtstring;
//                     // std::cout << vthdcxtstring << std::endl; // 删掉
//                 }
//                 else if (i == 3)
//                 {
//                     jf->fieldSignature = vthdcxtstring;
//                     // std::cout << vthdcxtstring << std::endl; // 删掉
//                 }
//             }
//             if (SVFUtil::getCallee(cs)->getName().c_str(), "Static")
//                 jf->isStatic = true;
//             return jf;
//         }
//     }
//     return nullptr;
// }

// CallerSensitive::JavaMethod *CallerSensitive::getCalledJavaMethod(std::vector<CallSite> methodIdcs, CallSite cs, AndersenBase *ander)
// {
//     NodeID callerSecArgID = ander->getPAG()->getValueNode(cs.getArgument(2));
//     for (std::vector<CallSite>::const_iterator it = methodIdcs.begin(); it != methodIdcs.end(); it++)
//     {
//         NodeID calleeRetID = ander->getPAG()->getValueNode(it->getInstruction());
//         // if (callerSecArgID == calleeRetID)
//         if (ander->alias(callerSecArgID, calleeRetID))
//         {
//             JavaMethod *jm = new JavaMethod();
//             for (int i = 2; i < 4; i++)
//             {
//                 const GetElementPtrInst *gepinst = SVFUtil::dyn_cast<GetElementPtrInst>(it->getArgument(i));
//                 const Constant *arrayinst = SVFUtil::dyn_cast<Constant>(gepinst->getOperand(0));
//                 const ConstantDataArray *cxtarray = SVFUtil::dyn_cast<ConstantDataArray>(arrayinst->getOperand(0));
//                 const std::string vthdcxtstring = cxtarray->getAsCString().str();
//                 if (i == 2)
//                 {
//                     jm->methodName = vthdcxtstring;
//                     // std::cout << vthdcxtstring << std::endl; // 删掉
//                 }
//                 else if (i == 3)
//                 {
//                     jm->methodSignature = vthdcxtstring;
//                     // std::cout << vthdcxtstring << std::endl; // 删掉
//                 }
//             }
//             if (SVFUtil::getCallee(cs)->getName().c_str(), "Static")
//                 jm->isStatic = true;
//             return jm;
//         }
//     }
//     return nullptr;
// }

// void CallerSensitive::aliasedCallerAndCalleeRet(const SVFFunction& caller, CallSite callee, AndersenBase *ander, CMethod *cm)
// {
//     NodeID callerRetId = ander->getPAG()->getReturnNode(&caller);
//     NodeID calleeRetId = ander->getPAG()->getValueNode((callee.getInstruction()));
//     if (ander->alias(callerRetId, calleeRetId))
//     {
//         // std::cout << "----------------------- return value aliased -----------------------" << std::endl;
//         cm->isRetTained = true;
//         cm->TaintedjavaMethod = SVFUtil::getCallee(callee)->getName();
//     }
// }

// void CallerSensitive::taintChecking(SVFModule *svfModule, AndersenBase *ander)
// {
//     ander->analyze();
//     SVFIR *pag = ander->getPAG();
//     /// handle functions
//     for (SVFModule::iterator fit = svfModule->begin(), efit = svfModule->end(); fit != efit; ++fit)
//     {
//         const SVFFunction &fun = **fit;
//         if (LLVMUtil::isUncalledFunction(fun.getLLVMFun()) && std::find(CallerSensitive::callees.begin(), CallerSensitive::callees.end(), fun.getName()) != CallerSensitive::callees.end())
//         {
//             CMethod *cm = new CMethod();
//             std::vector<JavaMethod *> javaMethods;
//             std::vector<JavaField *> javaFields;
//             std::vector<CallSite> methodIdcs;
//             std::vector<CallSite> fieldIdcs;
//             std::vector<CallSite> fieldCallcs;
//             cm->name = fun.getName();
//             // std::cout << "-----------------------" << fun.getName() << "-----------------------" << std::endl;
//             for (Function::iterator bit = fun.getLLVMFun()->begin(), ebit = fun.getLLVMFun()->end(); bit != ebit; ++bit)
//             {
//                 BasicBlock &bb = *bit;
//                 for (BasicBlock::iterator it = bb.begin(), eit = bb.end(); it != eit; ++it)
//                 {
//                     Instruction &inst = *it;
//                     if (SVFUtil::isCallSite(&inst))
//                     {
//                         const SVFFunction *callee = SVFUtil::getCallee(SVFUtil::getLLVMCallSite(pag->getICFG()->getCallICFGNode(&inst)->getCallSite()));
//                         if (callee && std::find(JNIJavaFieldID.begin(), JNIJavaFieldID.end(), callee->getName()) != JNIJavaFieldID.end())
//                         {
//                             CallSite cs = SVFUtil::getLLVMCallSite(pag->getICFG()->getCallICFGNode(&inst)->getCallSite());
//                             fieldIdcs.push_back(cs);
//                         }
//                         else if (callee && std::find(JNIJavaMethodID.begin(), JNIJavaMethodID.end(), callee->getName()) != JNIJavaMethodID.end())
//                         {
//                             CallSite cs = SVFUtil::getLLVMCallSite(pag->getICFG()->getCallICFGNode(&inst)->getCallSite());
//                             methodIdcs.push_back(cs);
//                         }
//                         else if (callee && std::find(JNIJavaFieldGet.begin(), JNIJavaFieldGet.end(), callee->getName()) != JNIJavaFieldGet.end())
//                         {
//                             CallSite cs = SVFUtil::getLLVMCallSite(pag->getICFG()->getCallICFGNode(&inst)->getCallSite());
//                             JavaField *jf = getCalledJavaField(fieldIdcs, cs, ander);
//                             if (jf)
//                             {
//                                 NodeID fieldRetId = pag->getValueNode((cs.getInstruction()));
//                                 jf->fieldRetId = fieldRetId;
//                                 javaFields.push_back(jf);
//                             }
//                         }
//                         else if (callee && std::find(JNIJavaMethodGet.begin(), JNIJavaMethodGet.end(), callee->getName()) != JNIJavaMethodGet.end())
//                         {
//                             CallSite cs = SVFUtil::getLLVMCallSite(pag->getICFG()->getCallICFGNode(&inst)->getCallSite());
//                             JavaMethod *jm = getCalledJavaMethod(methodIdcs, cs, ander);
//                             if (jm)
//                             {
//                                 javaMethods.push_back(jm);
//                                 JavaFieldsAsCallBackArgs(javaFields, cs, ander, jm);
//                                 aliasedCallerAndCalleeArgs(fun, cs, ander, jm);
//                                 aliasedCallerAndCalleeRet(fun, cs, ander, cm);
//                             }
//                         }
//                     }
//                 }
//             }
//             aliasedCallerArgsAndRet(fun, ander, cm);
//             cm->javaMethods = javaMethods;
//             cm->javaFields = javaFields;
//             cmethods.push_back(*cm);
//         }
//     }
// }

// void CallerSensitive::CreateSpecification()
// {
//     cJSON *root = cJSON_CreateObject();

//     for (std::vector<CMethod>::iterator it = cmethods.begin(); it != cmethods.end(); it++)
//     {
//         cJSON *CMethod = cJSON_CreateObject();
//         for (std::vector<JavaMethod *>::iterator it2 = it->javaMethods.begin(); it2 != it->javaMethods.end(); it2++)
//         {
//             cJSON *JMethod = cJSON_CreateObject();
//             cJSON_AddItemToObject(JMethod, "methodSignature", cJSON_CreateString((*it2)->methodSignature.c_str()));
//             cJSON_AddItemToObject(JMethod, "isStatic", cJSON_CreateBool((*it2)->isStatic));
//             if ((*it2)->taintedArgs.size() > 0)
//             {
//                 u32_t index = 1;
//                 for (std::map<int, std::string>::iterator it3 = (*it2)->taintedArgs.begin(); it3 != (*it2)->taintedArgs.end(); it3++)
//                 {
//                     cJSON *tainedArgs = cJSON_CreateObject();
//                     cJSON_AddItemToObject(tainedArgs, "JavaArg", cJSON_CreateString(std::to_string((it3)->first).c_str()));
//                     cJSON_AddItemToObject(tainedArgs, "tainedArg", cJSON_CreateString((it3)->second.c_str()));
//                     cJSON_AddItemToObject(JMethod, ("tainedArgs" + std::to_string(index++)).c_str(), tainedArgs);
//                 }
//             }
//             cJSON_AddItemToObject(CMethod, (*it2)->methodName.c_str(), JMethod);
//         }
//         // Ouput java fields
//         for (std::vector<JavaField *>::iterator it3 = it->javaFields.begin(); it3 != it->javaFields.end(); it3++)
//         {
//             cJSON *JMethod = cJSON_CreateObject();
//             cJSON_AddItemToObject(JMethod, "fieldSignature", cJSON_CreateString((*it3)->fieldSignature.c_str()));
//             cJSON_AddItemToObject(JMethod, "isStatic", cJSON_CreateBool((*it3)->isStatic));
//             cJSON_AddItemToObject(CMethod, (*it3)->fieldName.c_str(), JMethod);
//         }
//         cJSON_AddItemToObject(CMethod, "isRetTained", cJSON_CreateBool(it->isRetTained));
//         cJSON_AddItemToObject(CMethod, "TaintedjavaMethod", cJSON_CreateString(it->TaintedjavaMethod.c_str()));
//         cJSON_AddItemToObject(CMethod, "tainedArg", cJSON_CreateString(std::to_string(it->tainedArg).c_str()));
//         cJSON_AddItemToObject(root, it->name.c_str(), CMethod);
//     }

//     FILE *file = NULL;
//     file = fopen("java.json", "w");
//     if (file == NULL)
//     {
//         std::cout << "Open file fail!" << std::endl;
//         // Release pointer
//         cJSON_Delete(root);
//         return;
//     }

//     char *cjValue = cJSON_Print(root);
//     // Write to file
//     int ret = fputs(cjValue, file);
//     if (ret == EOF)
//     {
//         std::cout << "Write file fail!" << std::endl;
//     }

//     fclose(file);
//     free(cjValue);

//     cJSON_Delete(root);
// }