#include "SVF-FE/SVFIRBuilder.h"
#include "SVF-FE/LLVMUtil.h"
#include "CallerSensitive.h"

using namespace SVF;

class CallerSVFIRBuilder : public SVFIRBuilder
{

public:

    CallerSVFIRBuilder(SVFModule* mod): SVFIRBuilder(mod)
    {
    }
    /// Destructor
    virtual ~CallerSVFIRBuilder()
    {
    }

    SVFIR* getCallerPAG(SVFModule* svfModule, std::vector<std::string> callees, std::map<std::string, std::vector<int>> tainedArgs)
    {
        SVFIR* callerPAG = getPAG();

        /// handle functions
        for (SVFModule::iterator fit = svfModule->begin(), efit = svfModule->end();
                fit != efit; ++fit)
        {
            const SVFFunction& fun = **fit;
            /// collect return node of function fun
            if(!fun.isDeclaration() && std::count(callees.begin(), callees.end(), fun.getName()))
            {
                NodeID tainedDummyNode = -1;
                u32_t index = 0;
                std::vector<int> aliasedArgs = tainedArgs[fun.getName()];
                for (Function::arg_iterator I = fun.getLLVMFun()->arg_begin(), E = fun.getLLVMFun()->arg_end(); I != E; ++I)
                {
                    setCurrentLocation(&*I,&fun.getLLVMFun()->getEntryBlock());
                    NodeID argValNodeId = callerPAG->getValueNode(&*I);
                    std::cout << argValNodeId << std::endl;
                    if (std::count(aliasedArgs.begin(), aliasedArgs.end(), index - 2))
                    {
                        if(tainedDummyNode == -1)
                        {
                            NodeID dummy = callerPAG->addDummyValNode();
                            NodeID obj = callerPAG->addDummyObjNode((&*I)->getType());
                            addAddrEdge(obj, dummy);
                            addCopyEdge(dummy, argValNodeId);
                            tainedDummyNode = dummy;
                        }
                        else
                        {
                            addCopyEdge(tainedDummyNode, argValNodeId);
                        }
                    }
                    else
                    {
                        NodeID dummy = callerPAG->addDummyValNode();
                        NodeID obj = callerPAG->addDummyObjNode((&*I)->getType());
                        addAddrEdge(obj, dummy);
                        addCopyEdge(dummy, argValNodeId); 
                    }
                    // if (LLVMUtil::isUncalledFunction(fun.getLLVMFun()) && fun.getName() != "main")
                    // if (std::count(callees.begin(), callees.end(), fun.getName()))
                    // {
                    //     NodeID dummy = callerPAG->addDummyValNode();
                    //     NodeID obj = callerPAG->addDummyObjNode((&*I)->getType());
                    //     addAddrEdge(obj, dummy);
                    //     addCopyEdge(dummy, argValNodeId);
                    // }

                    index++;
                }
            }
            for (Function::iterator bit = fun.getLLVMFun()->begin(), ebit = fun.getLLVMFun()->end();
                bit != ebit; ++bit)
            {
                BasicBlock& bb = *bit;
                for (BasicBlock::iterator it = bb.begin(), eit = bb.end();
                        it != eit; ++it)
                {
                    Instruction& inst = *it;
                    // setCurrentLocation(&inst,&bb);
                    // visit(inst);

                    if (SVFUtil::isCallSite(&inst))
                    {
                        setCurrentLocation(&inst,&bb);
                        CallSite cs = SVFUtil::getLLVMCallSite(callerPAG->getICFG()->getCallICFGNode(&inst)->getCallSite());
                        const SVFFunction *callee = SVFUtil::getCallee(SVFUtil::getLLVMCallSite(callerPAG->getICFG()->getCallICFGNode(&inst)->getCallSite()));
                        if (callee && (std::find(CallerSensitive::getCallerSensitive()->JNIJavaFieldID.begin(), CallerSensitive::getCallerSensitive()->JNIJavaFieldID.end(), callee->getName()) != CallerSensitive::getCallerSensitive()->JNIJavaFieldID.end()||
                            std::find(CallerSensitive::getCallerSensitive()->JNIJavaMethodID.begin(), CallerSensitive::getCallerSensitive()->JNIJavaMethodID.end(), callee->getName()) != CallerSensitive::getCallerSensitive()->JNIJavaMethodID.end() ||
                            std::find(CallerSensitive::getCallerSensitive()->JNIJavaFieldGet.begin(), CallerSensitive::getCallerSensitive()->JNIJavaFieldGet.end(), callee->getName()) != CallerSensitive::getCallerSensitive()->JNIJavaFieldGet.end() || 
                            std::find(CallerSensitive::getCallerSensitive()->JNIJavaMethodGet.begin(), CallerSensitive::getCallerSensitive()->JNIJavaMethodGet.end(), callee->getName()) != CallerSensitive::getCallerSensitive()->JNIJavaMethodGet.end()))
                        {
                            // std::cout << funName << std::endl;
                            // std::cout << SVFUtil::value2String(cs.getInstruction()) << std::endl;
                            NodeID dummy = callerPAG->addDummyValNode();
                            NodeID obj = callerPAG->addDummyObjNode(cs.getInstruction()->getType());
                            addAddrEdge(obj, dummy);
                            addCopyEdge(dummy, callerPAG->getValueNode((cs.getInstruction())));
                        }
                    }
                }
            }
        }
        
        return callerPAG;
    }

};