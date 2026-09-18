// Runs the DWARF1ProgramAnalyzer of the ghidra-dwarf1 extension (patched with ghidra-dwarf1-gcc-ext.patch for
// GCC's six attributes 0x8000..0x8050) through REFLECTION: the analyzer is registered with
// setDefaultEnablement(false), so headless auto-analysis never runs it. In the GUI, use
// Analysis > One Shot > DWARF1 instead.
//
// Arguments: [output.txt]. Writes a census before and after, so that the import can be checked.
//@category NFSMW

import ghidra.app.script.GhidraScript;
import ghidra.app.util.importer.MessageLog;
import ghidra.program.model.data.DataTypeManager;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.Union;
import ghidra.program.model.data.DataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.SourceType;
import java.io.PrintWriter;
import java.util.Iterator;

public class Dwarf1Run extends GhidraScript {

    private PrintWriter w;

    private void census(String tag) {
        int nfun = 0, sigImported = 0, withParams = 0;
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            Function f = it.next();
            nfun++;
            if (f.getSignatureSource() == SourceType.IMPORTED) {
                sigImported++;
            }
            if (f.getParameterCount() > 0) {
                withParams++;
            }
        }
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        int nstruct = 0, withFields = 0, nunion = 0, nenum = 0;
        Iterator<Structure> sit = dtm.getAllStructures();
        while (sit.hasNext()) {
            Structure s = sit.next();
            nstruct++;
            if (s.getNumDefinedComponents() > 0) {
                withFields++;
            }
        }
        Iterator<DataType> dit = dtm.getAllDataTypes();
        while (dit.hasNext()) {
            DataType dt = dit.next();
            if (dt instanceof Union) {
                nunion++;
            }
            else if (dt instanceof ghidra.program.model.data.Enum) {
                nenum++;
            }
        }
        w.println("CENSUS " + tag);
        w.println("  functions                : " + nfun);
        w.println("  signature == IMPORTED    : " + sigImported);
        w.println("  with >=1 parameter       : " + withParams);
        w.println("  structs                  : " + nstruct + " (with fields: " + withFields + ")");
        w.println("  unions                   : " + nunion);
        w.println("  enums                    : " + nenum);
        w.println("  types in the DTM         : " + dtm.getDataTypeCount(true));
        w.flush();
    }

    @Override
    public void run() throws Exception {
        String out = getScriptArgs().length > 0 ? getScriptArgs()[0] : "dwarf1run.txt";
        w = new PrintWriter(out, "UTF-8");
        w.println("program   : " + currentProgram.getName());
        w.println("language  : " + currentProgram.getLanguageID());
        w.println("ghidra    : " + ghidra.framework.Application.getApplicationVersion());
        w.println();
        census("BEFORE");
        w.println();

        long t0 = System.currentTimeMillis();
        MessageLog log = new MessageLog();
        boolean ok;
        try {
            Class<?> cls = Class.forName("com.github.rafalh.ghidra.dwarfone.DWARF1ProgramAnalyzer");
            Object pa = cls.getConstructors()[0].newInstance(currentProgram,
                    currentProgram.getMemory(), monitor, log);
            ok = (Boolean) cls.getMethod("process").invoke(pa);
        }
        catch (Throwable e) {
            Throwable c = e.getCause() != null ? e.getCause() : e;
            w.println("EXCEPTION WHILE ANALYZING: " + c);
            for (StackTraceElement s : c.getStackTrace()) {
                w.println("    at " + s);
            }
            ok = false;
        }
        w.println("process() = " + ok + "  in " + (System.currentTimeMillis() - t0) / 1000 + " s");
        String lg = log.toString();
        w.println("MessageLog: " + lg.length() + " chars (first 4000)");
        w.println(lg.length() > 4000 ? lg.substring(0, 4000) : lg);
        w.println();
        census("AFTER");
        w.close();
        println("DWARF1 process() = " + ok);
    }
}
