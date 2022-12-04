#ifndef CALLERSENSITIVE_H_
#define CALLERSENSITIVE_H_
// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/IR/IRBuilder.h"
#include "Util/BasicTypes.h"
#include "Graphs/ConsG.h"
#include "WPA/Andersen.h"
#include "MemoryModel/SVFIR.h"
#include <sys/stat.h>
#include "Util/config.h"
#include "Util/cJSON.h"
#include <set>

// using namespace llvm;

namespace SVF
{

    // static LLVMContext llvmContext;
    // static Module *module;
    // static llvm::IRBuilder<> irBuilder(llvmContext);

    class CallerSensitive
    {

    public:
        // enum JavaDataType
        // {
        //     JBOOLEAN = 0,  // unsigned 8 bits
        //     JBYTE,         // signed 8 bits
        //     JCHAR,         // unsigned 16 bits
        //     JSHORT,        // signed 16 bits
        //     JINT,          // unsigned 32 bits
        //     JLONG,         // signed 64 bits
        //     JFLOAT,        // 32 bits
        //     JDOUBLE,       // 64 bits
        //     JOBJECT,       // any java object
        //     JSTRING,       // string object
        //     JCLASS,        // class object
        //     JOBJECTARRAY,  // object array
        //     JBOOLEANARRAY, // boolean array
        //     JBYTEARRAY,    // byte array
        //     JCHARARRAY,    // char array
        //     JSHORTARRAY,   // short array
        //     JINTARRAY,     // int array
        //     JLONGARRAY,    // long array
        //     JFLOATARRAY,   // float array
        //     JDOUBLEARRAY,  // double array
        //     VOID,          // void
        //     JAVA_NULL
        // };

        enum JavaInstanceFieldGet
        {
            // GetFieldID,
            GetObjectField = 0,
            GetBooleanField,
            GetByteField,
            GetCharField,
            GetShortField,
            GetIntField,
            GetLongField,
            GetFloatField,
            GetDoubleField
        };

        enum JavaStaticFieldGet
        {
            // GetStaticFieldID,
            GetStaticObjectField = 0,
            GetStaticBooleanField,
            GetStaticByteField,
            GetStaticCharField,
            GetSStatichortField,
            GetStaticIntField,
            GetStaticLongField,
            GetStaticFloatField,
            GetStaticDoubleField
        };

        enum JavaInstanceCallMethod
        {
            CallObjectMethod = 0,
            CallObjectMethodV,
            CallObjectMethodA,

            CallBooleanMethod,
            CallBooleanMethodV,
            CallBooleanMethodA,

            CallByteMethod,
            CallByteMethodV,
            CallByteMethodA,

            CallCharMethod,
            CallCharMethodV,
            CallCharMethodA,

            CallShortMethod,
            CallShortMethodV,
            CallShortMethodA,

            CallIntMethod,
            CallIntMethodV,
            CallIntMethodA,

            CallLongMethod,
            CallLongMethodV,
            CallLongMethodA,

            CallFloatMethod,
            CallFloatMethodV,
            CallFloatMethodA,

            CallDoubleMethod,
            CallDoubleMethodV,
            CallDoubleMethodA,

            CallVoidMethod,
            CallVoidMethodV,
            CallVoidMethodA,

            CallNonvirtualObjectMethod,
            CallNonvirtualObjectMethodV,
            CallNonvirtualObjectMethodA,

            CallNonvirtualBooleanMethod,
            CallNonvirtualBooleanMethodV,
            CallNonvirtualBooleanMethodA,

            CallNonvirtualByteMethod,
            CallNonvirtualByteMethodV,
            CallNonvirtualByteMethodA,

            CallNonvirtualCharMethod,
            CallNonvirtualCharMethodV,
            CallNonvirtualCharMethodA,

            CallNonvirtualShortMethod,
            CallNonvirtualShortMethodV,
            CallNonvirtualShortMethodA,

            CallNonvirtualIntMethod,
            CallNonvirtualIntMethodV,
            CallNonvirtualIntMethodA,

            CallNonvirtualLongMethod,
            CallNonvirtualLongMethodV,
            CallNonvirtualLongMethodA,

            CallNonvirtualFloatMethod,
            CallNonvirtualFloatMethodV,
            CallNonvirtualFloatMethodA,

            CallNonvirtualDoubleMethod,
            CallNonvirtualDoubleMethodV,
            CallNonvirtualDoubleMethodA,

            CallNonvirtualVoidMethod,
            CallNonvirtualVoidMethodV,
            CallNonvirtualVoidMethodA
        };

        enum JavaStaticCallMethod
        {
            CallStaticObjectMethod = 0,
            CallStaticObjectMethodV,
            CallStaticObjectMethodA,

            CallStaticBooleanMethod,
            CallStaticBooleanMethodV,
            CallStaticBooleanMethodA,

            CallStaticByteMethod,
            CallStaticByteMethodV,
            CallStaticByteMethodA,

            CallStaticCharMethod,
            CallStaticCharMethodV,
            CallStaticCharMethodA,

            CallStaticShortMethod,
            CallStaticShortMethodV,
            CallStaticShortMethodA,

            CallStaticIntMethod,
            CallStaticIntMethodV,
            CallStaticIntMethodA,

            CallStaticLongMethod,
            CallStaticLongMethodV,
            CallStaticLongMethodA,

            CallStaticFloatMethod,
            CallStaticFloatMethodV,
            CallStaticFloatMethodA,

            CallStaticDoubleMethod,
            CallStaticDoubleMethodV,
            CallStaticDoubleMethodA,

            CallStaticVoidMethod,
            CallStaticVoidMethodV,
            CallStaticVoidMethodA
        };

        // std::map<std::string, JavaDataType> javaBasicTypes =
        //     {
        //         {"bool", JBOOLEAN},
        //         {"byte", JBYTE},
        //         {"char", JCHAR},
        //         {"short", JSHORT},
        //         {"int", JINT},
        //         {"long long", JLONG},
        //         {"float", JFLOAT},
        //         {"double", JDOUBLE},
        //         {"void", VOID},
        //         {"", JAVA_NULL}};

        std::vector<std::string> JNIJavaFieldID = {
            "_ZN7JNIEnv_10GetFieldIDEP7_jclassPKcS3_",
            "_ZN7JNIEnv_16GetStaticFieldIDEP7_jclassPKcS3_"};

        std::vector<std::string> JNIJavaMethodID = {
            "_ZN7JNIEnv_11GetMethodIDEP7_jclassPKcS3_",
            "_ZN7JNIEnv_17GetStaticMethodIDEP7_jclassPKcS3_"};

        std::vector<std::string> JNIJavaFieldGet = {
            "_ZN7JNIEnv_14GetObjectFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_15GetBooleanFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_12GetByteFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_12GetCharFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_13GetShortFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_11GetIntFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_12GetLongFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_13GetFloatFieldEP8_jobjectP9_jfieldID",
            "_ZN7JNIEnv_14GetDoubleFieldEP8_jobjectP9_jfieldID",

            "_ZN7JNIEnv_20GetStaticObjectFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_21GetStaticBooleanFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_18GetStaticByteFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_18GetStaticCharFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_19GetStaticShortFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_17GetStaticIntFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_18GetStaticLongFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_19GetStaticFloatFieldEP7_jclassP9_jfieldID",
            "_ZN7JNIEnv_20GetStaticDoubleFieldEP7_jclassP9_jfieldID",};

        std::vector<std::string> JNIJavaMethodGet = {
            "_ZN7JNIEnv_16CallObjectMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_17CallBooleanMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_14CallByteMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_14CallCharMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_15CallShortMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_13CallIntMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_14CallLongMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_15CallFloatMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_16CallDoubleMethodEP8_jobjectP10_jmethodIDz",
            "_ZN7JNIEnv_14CallVoidMethodEP8_jobjectP10_jmethodIDz",

            "_ZN7JNIEnv_22CallStaticObjectMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_23CallStaticBooleanMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_20CallStaticByteMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_20CallStaticCharMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_21CallStaticShortMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_19CallStaticIntMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_20CallStaticLongMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_21CallStaticFloatMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_22CallStaticDoubleMethodEP7_jclassP10_jmethodIDz",
            "_ZN7JNIEnv_20CallStaticVoidMethodEP7_jclassP10_jmethodIDz"};

        

        class JavaMethod
        {
        public:
            bool isStatic = false;
            std::string methodName;
            std::string methodSignature;
            // std::map<int, std::string> taintedArgs;
            std::map<int, std::vector<std::string>> taintedArgs;
            std::map<int, std::string> taintedFields;

        };

        class JavaField
        {
        public:
            bool isStatic = false;
            std::string fieldName;
            std::string fieldSignature;
            NodeID fieldRetId;
        };

        class CMethod
        {
        public:
            std::string name;
            std::vector<JavaMethod *> javaMethods;
            std::vector<JavaField *> javaFields;
            bool isRetTained = false;
            std::string TaintedjavaMethod = "";
            // int tainedArg = -1;
            std::vector<int> taintedArgs;
        };

        std::vector<std::string> callees;
        // std::map<std::string, std::vector<JavaMethod *>> methods;
        std::vector<CMethod> cmethods;

        std::map<std::string, std::vector<int>> tainedArgs;

        // // Except for the basic data types, other java data types are mapped to (void *) in C language
        // std::set<std::string> javaPyTypes{"void *", "jobject", "jclass", "jstring", "jobjectArray", "jbooleanArray", "jbyteArray", "jcharArray", "jshortArray", "jintArray", "jlongArray", "jfloatArray", "jdoubleArray"};

        static CallerSensitive *callerSen;

        static CallerSensitive *getCallerSensitive()
        {
            if (callerSen == nullptr)
            {
                callerSen = new CallerSensitive();
            }
            return callerSen;
        }

        // Get specifications of caller functions
        cJSON *parseCallerJson(std::string path);

        // Get basic Java data type
        // JavaDataType getBasicType(std::string type);

        // Get the data type of the C language corresponding to the java data type
        // Type *getType(std::string type);

        // Get the data types of the a function's parameters
        // std::vector<Type *> getParams(std::vector<std::string> args);

        // Output final .ll file
        // void output2file(std::string outPath);

        // Create function declaration IR
        // Function *functionDeclarationIR(std::string funName, std::string ret, std::vector<std::string> args);

        // Get Alloca and Store IR instructions
        // std::map<std::string, Value *> allocaStore(std::vector<std::string> params, Function *func);

        // Get Alloca, Store and Load IR instructions
        // Value *allocaStoreLoad(std::string typeStr, Value *value);

        // Get callee's arguments
        // Value *getCalleeArgs(std::string typeStr, std::string name, std::map<std::string, Value *> callerArgs, std::map<std::string, Value *> rets);

        // Get number of a argument or return vale (e.g. arg0, ret1)
        // u32_t getNumber(std::string s);

        // Get .json file path from input
        std::string getJsonFile(int &argc, char **argv);

        // Create caller IR
        // void callerIRCreate(std::string jsonPath, std::string irPath);

        void getCallees(std::string jsonPath);

        // u32_t getAliasedParamsPosition(const SVFIR::SVFVarList& funArgList, const PointsTo& pts, ConstraintGraph* consCG);

        // If java fields are the arguments of callback java functions
        void JavaFieldsAsCallBackArgs(std::vector<JavaField *> javaFields, CallSite callee, AndersenBase* ander, JavaMethod *jm);

        // If caller(c/c++)'s args and callee(java) 's args are aliased
        void aliasedCallerAndCalleeArgs(const SVFFunction& fun, CallSite callee, AndersenBase* ander, JavaMethod *jm);

        void taintChecking(SVFModule* svfModule, AndersenBase* ander);

        JavaMethod* getCalledJavaMethod(std::vector<CallSite> methodIdcs, CallSite cs, AndersenBase* ander);

        JavaField* getCalledJavaField(std::vector<CallSite> fieldIdcs, CallSite cs, AndersenBase* ander);

        // If caller's args and return value are aliased
        void aliasedCallerArgsAndRet(const SVFFunction& fun, AndersenBase* ander, CMethod *cm);

        // If caller's return value and callee's return value are aliased
        void aliasedCallerAndCalleeRet(const SVFFunction& caller, CallSite callee, AndersenBase *ander, CMethod *cm);

        void CreateSpecification();
    };

}

#endif /* CALLERSENSITIVE_ */