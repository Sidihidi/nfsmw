// Sets GQR0 = 0 over every executable block BEFORE auto-analysis: run it as a -preScript of the import.
//
// NFSMW writes GQR0 only five times (80003304, 8030f8a4, 8030fb5c and 8032226c are "li rN,0; mtspr GQR0,rN";
// 8030f930 is the FPU context reload of a value that was already 0). Without this, every psq_l/psq_st
// decompiles into 15 lines of ifs over the quantization type. GQR1..GQR7 are NOT set: their quantization is
// real (269 accesses).
//@category NFSMW

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.mem.MemoryBlock;
import java.math.BigInteger;

public class SetGqr extends GhidraScript {
    @Override
    public void run() throws Exception {
        Register gqr0 = currentProgram.getLanguage().getRegister("GQR0");
        println("LANGUAGE = " + currentProgram.getLanguageID());
        if (gqr0 == null) {
            println("SETGQR: this language has no GQR0 register -- nothing set");
            return;
        }
        int n = 0;
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            if (!b.isExecute()) {
                continue;
            }
            Address s = b.getStart();
            Address e = b.getEnd();
            currentProgram.getProgramContext().setValue(gqr0, s, e, BigInteger.ZERO);
            println("SETGQR: GQR0=0 in " + b.getName() + " [" + s + " .. " + e + "]");
            n++;
        }
        println("SETGQR: executable blocks processed = " + n);
    }
}
