package me.hexzone;

import soot.*;
import soot.jimple.*;
import soot.jimple.toolkits.callgraph.CallGraphBuilder;
import soot.options.Options;

import java.io.IOException;
import java.util.Collections;
import java.util.Iterator;
import java.util.Map;

import static java.lang.System.exit;
import static java.lang.System.out;

public class Main {
    public static void main(String[] args) {
        out.println("APK Native Call Analyzer");

        if(args.length!=1){
            out.println("Need 1 param for the APK file path.");
            return;
        }

        String apkFilePath = args[0];
        out.println("APK File:" + apkFilePath);

        Options.v().set_keep_line_number(true);
        Options.v().set_whole_program(true);
        Options.v().setPhaseOption("cg", "verbose:false");
        Options.v().setPhaseOption("cg", "trim-clinit:true");
        Options.v().setPhaseOption("cg.spark", "on");
        Options.v().set_src_prec(Options.src_prec_class);
        Options.v().set_prepend_classpath(true);
        Options.v().setPhaseOption("wjop", "enabled:false");
        Options.v().set_src_prec(Options.src_prec_apk);
        Options.v().set_output_format(Options.output_format_jimple);
//        Options.v().set_output_format(Options.output_format_dex);
        Options.v().set_android_jars("D:\\AS\\sdk\\platforms");
        Options.v().set_keep_line_number(true);
        Options.v().set_allow_phantom_refs(true);
        Options.v().set_process_multiple_dex(true);

        Options.v().set_process_dir(Collections.singletonList(apkFilePath));

        Scene.v().loadNecessaryClasses();

        PackManager.v().getPack("jtp").add(new Transform("jtp.myAnalysis", new ApkAnalyzer()));

        PackManager.v().runPacks();
        PackManager.v().writeOutput();
//        System.out.println(Scene.v().getCallGraph());
//        PointsToAnalysis pta = Scene.v().getPointsToAnalysis();
        try {
            ApkAnalyzer.printNativeCallInfo("ret.json");
        } catch (IOException e) {
            out.println("写json文件错误");
            throw new RuntimeException(e);
        }

    }
}