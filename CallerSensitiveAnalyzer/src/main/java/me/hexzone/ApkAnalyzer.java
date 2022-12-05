package me.hexzone;

import com.google.gson.*;
import soot.*;
import soot.jimple.Stmt;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class ApkAnalyzer extends BodyTransformer {
    protected static List<String> excludePackagesList = new ArrayList<String>();

    static
    {
        excludePackagesList.add("java.");
        excludePackagesList.add("android.");
        excludePackagesList.add("androidx.");
        excludePackagesList.add("javax.");
        excludePackagesList.add("kotlin.");
        excludePackagesList.add("kotlin");
        excludePackagesList.add("kotlinx.");
        excludePackagesList.add("android.support.");
        excludePackagesList.add("org.intellij.");
        excludePackagesList.add("sun.");
        excludePackagesList.add("com.google.");
    }
    static Set<String> classes = Collections.synchronizedSet(new HashSet<String>());
    static Set<String> methods = Collections.synchronizedSet(new HashSet<String>());
    static Set<SootMethod> rawMethods = Collections.synchronizedSet(new HashSet<SootMethod>());

    static JsonObject nativeCalls =new JsonObject();
    static Map<String, HashMap<Integer, Value>> paramsForNativeCalls = Collections.synchronizedMap(new HashMap<String, HashMap<Integer, Value>>());

    public static void printNativeCallInfo(){
        for (Map.Entry<String, JsonElement> entry : nativeCalls.entrySet()) {
            String callerMethodName = entry.getKey();
            HashMap<Integer, Value> params = paramsForNativeCalls.get(callerMethodName);
            System.out.println(callerMethodName + " / param count:" + params.size());
        }

        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        String prettyJsonStr = gson.toJson(nativeCalls);
        System.out.println(prettyJsonStr);
    }

    public static void printNativeCallInfo(String filename) throws IOException {
        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        String prettyJsonStr = gson.toJson(nativeCalls);
//        System.out.println(prettyJsonStr);

        File file = new File(filename);

        if(file.getParentFile()!=null&&!file.getParentFile().exists()){
            //若父目录不存在则创建父目录
            file.getParentFile().mkdirs();
        }

        if(file.exists()){
            //若文件存在，则删除旧文件
            file.delete();
        }

        file.createNewFile();
        Writer writer = new OutputStreamWriter(new FileOutputStream(filename), StandardCharsets.UTF_8);
        writer.write(prettyJsonStr);
        writer.flush();
        writer.close();
    }

    public static void printClasses(){
        for(String c : classes){
            System.out.println(c);
        }
    }

    public static void printMethods(){
        for(String m : methods){
            System.out.println(m);
        }
    }
    public static void printRawMethods(boolean onlyNative){
        for(SootMethod m : rawMethods){
            if(onlyNative){
                if(m.isNative()){
                    System.out.println(m.getName() + ":::::" + m.getSignature());
                }
            }else{
                System.out.println(m.getName() + ":::::" + m.getSignature());
            }
        }
    }

    public static void printCallerMethods(){
        for(SootMethod m : rawMethods){
            if(m.getName().startsWith("caller")){
                System.out.println(m.getName() + ":::::" + m.getSignature());
                Body b2 = m.retrieveActiveBody();
                System.out.println("[body]" + b2);

                Iterator<Unit> i = b2.getUnits().snapshotIterator();
                while (i.hasNext()) {
                    Unit u = i.next();
                    System.out.println("[Unit]" + u);
                }

            }
        }
    }
    /**
     * 是否是例外的包名
     * @param sootClass 当前的类
     * @return 检查结果
     */
    protected boolean isExcludeClass(SootClass sootClass)
    {
        if (sootClass.isPhantom())
        {
            return true;
        }

        String packageName = sootClass.getPackageName();
        for (String exclude : excludePackagesList)
        {
            if (packageName.startsWith(exclude))
            {
                return true;
            }
        }

        return false;
    }

    protected boolean isExcludeClass(String name)
    {
        for (String exclude : excludePackagesList)
        {
            if (name.startsWith(exclude))
            {
                return true;
            }
        }

        return false;
    }

    @Override
    protected void internalTransform(Body b, String phaseName, Map<String, String> options) {

        SootClass cls = b.getMethod().getDeclaringClass();
        if(isExcludeClass(cls)){
            return;
        }

        Iterator<Unit> i=b.getUnits().snapshotIterator();
        while(i.hasNext())
        {
            Unit u=i.next();
//            ((soot.jimple.Stmt)u).getInvokeExpr().getMethod().isNative();
            if(u instanceof Stmt stmt && ((soot.jimple.Stmt)u).containsInvokeExpr()){
                SootMethod callerMethod = b.getMethod();
                SootMethod calleeMethod = stmt.getInvokeExpr().getMethod();

                if(calleeMethod.isNative() && !isExcludeClass(stmt.getInvokeExpr().getMethod().getDeclaringClass())){

                    JsonObject callerInfoContainer =new JsonObject();
                    JsonObject calleeInfoContainer =new JsonObject();
                    JsonArray callerInfoParams = new JsonArray();
                    JsonArray calleeInfoParams = new JsonArray();

                    callerInfoContainer.addProperty("returnType", callerMethod.getReturnType().toString());
                    for(Type t: callerMethod.getParameterTypes()){
                        callerInfoParams.add(t.toString());
                    }
                    callerInfoContainer.add("paramTypes", callerInfoParams);

                    System.out.println(b.getMethod().getSignature());
                    String nativeMethodClass = stmt.getInvokeExpr().getMethod().getDeclaringClass().getName();
                    String nativeMethodName = stmt.getInvokeExpr().getMethod().getName();
                    String nativeMethodCPPName = "Java_" + nativeMethodClass.replace(".", "_") + "_" + nativeMethodName;
                    System.out.println("\t===CALL===>>>\t" + nativeMethodCPPName);

                    calleeInfoContainer.addProperty("javaName", nativeMethodName);
                    calleeInfoContainer.addProperty("nativeName", nativeMethodCPPName);
                    calleeInfoContainer.addProperty("returnType", stmt.getInvokeExpr().getMethod().getReturnType().toString());
                    for(Type t: stmt.getInvokeExpr().getMethod().getParameterTypes()){
                        calleeInfoParams.add(t.toString());
                    }
                    calleeInfoContainer.add("paramTypes", calleeInfoParams);

                    callerInfoContainer.add("callee", calleeInfoContainer);

                    nativeCalls.add(callerMethod.getName(), callerInfoContainer);
                    HashMap<Integer, Value> params = new HashMap<Integer, Value>();
                    int index = 0;
                    for(Value p: stmt.getInvokeExpr().getArgs()){
                        params.put(index ++, p);
                    }
                    paramsForNativeCalls.put(callerMethod.getName(), params);
                }
            }
        }

    }
}
