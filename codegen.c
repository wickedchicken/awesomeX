// Headers required by LLVM
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>


// General stuff
#include <stdlib.h>
#include <stdio.h>

#include "awesomex.h"

// this is probably wrong
const char* typedecode(LLVMTypeRef type, int c_types){
  unsigned width;
  switch(LLVMGetTypeKind(type)){
    case LLVMVoidTypeKind:
      if(c_types){return "void";} else { return "XXXXXXXXXXXXXX"; }
      break;
    case LLVMFloatTypeKind:
      if(c_types){return "float";} else { return "LLVMCreateGenericValueOfFloat(LLVMFloatType(), arg%d)";}
      break;
    case LLVMDoubleTypeKind:
      if(c_types){return "double";} else { return "LLVMCreateGenericValueOfFloat(LLVMDoubleType(), arg%d)";}
      break;
    case LLVMIntegerTypeKind:
      width = LLVMGetIntTypeWidth(type);
      switch(width){
        case 8:
          if(c_types){return "int8_t";} else { return "LLVMCreateGenericValueOfInt(LLVMInt8Type(), arg %d, 0)";}
          break;
        case 16:
          if(c_types){return "int16_t";} else { return "LLVMCreateGenericValueOfInt(LLVMInt16Type(), arg%d, 0)";}
          break;
        case 32:
          if(c_types){return "int32_t";} else { return "LLVMCreateGenericValueOfInt(LLVMInt32Type(), arg%d, 0)";}
          break;
        case 64:
          if(c_types){return "int64_t";} else { return "LLVMCreateGenericValueOfInt(LLVMInt64Type(), arg%d, 0)";}
          break;
        case 128:
          if(c_types){return "int128_t";} else { return "LLVMCreateGenericValueOfInt(LLVMInt8Type(),";}
          break;
        default:
          fprintf(stderr, "who the hell has size %d, ints?!\n", width);
          exit(1);
      }
      break;
    case LLVMPointerTypeKind:
      if(c_types){ return "void *"; } else { return "LLVMCreateGenericValueOfPointer(arg%d)"; }
      break;
    default:
      fprintf(stderr, "this function does't know how to handle this type: %d, sorry!\n", LLVMGetTypeKind(type));
      exit(1);
  }
}

void print_preamble(const char *filename){
  printf("// Headers required by LLVM\n\
#include <llvm-c/Core.h>\n\
#include <llvm-c/Analysis.h>\n\
#include <llvm-c/ExecutionEngine.h>\n\
#include <llvm-c/Target.h>\n\
#include <llvm-c/Transforms/Scalar.h>\n\
\n\
\n\
// General stuff\n\
#include <stdlib.h>\n\
#include <stdio.h>\n\
#include <sys/types.h>\n\
\n\
#include \"%s\"\n\
\n\
static char llvm_initialized = 0;\n\
\n\
void init_llvm(){\n\
  LLVMLinkInJIT();\n\
  LLVMInitializeNativeTarget();\n\
  llvm_initialized = 1;\n\
}\n\
\n", filename);
}

void print_funcsetup(const char *name){
  printf("LLVMModuleRef mod = makeLLVMModule();\n\
  LLVMValueRef myfn = LLVMGetNamedFunction(mod, \"%s\");\n\
\n\
  LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);\n\
  LLVMDisposeMessage(error); // Handler == LLVMAbortProcessAction -> No need to check errors\n\
\n\
\n\
  LLVMExecutionEngineRef engine;\n\
  LLVMModuleProviderRef provider = LLVMCreateModuleProviderForExistingModule(mod);\n\
  error = NULL;\n\
  if(LLVMCreateJITCompiler(&engine, provider, 2, &error) != 0) {\n\
    fprintf(stderr, \"%%s\\n\", error);\n\
    LLVMDisposeMessage(error);\n\
    abort();\n\
  }\n\
\n\
  LLVMPassManagerRef pass = LLVMCreatePassManager();\n\
  LLVMAddTargetData(LLVMGetExecutionEngineTargetData(engine), pass);\n\
  LLVMAddConstantPropagationPass(pass);\n\
  LLVMAddInstructionCombiningPass(pass);\n\
  LLVMAddPromoteMemoryToRegisterPass(pass);\n\
  // LLVMAddDemoteMemoryToRegisterPass(pass); // Demotes every possible value to memory\n\
  LLVMAddGVNPass(pass);\n\
  LLVMAddCFGSimplificationPass(pass);\n\
  LLVMRunPassManager(pass, mod);\n\
//  LLVMDumpModule(mod);\n\n", name);
}

void print_exec(LLVMValueRef *args, unsigned paramcount){
  printf("  LLVMGenericValueRef exec_args[] = {\n");
  for(int i=0;i<(paramcount-1);i++){
    printf(typedecode(LLVMTypeOf(args[i]), 0), i);
    printf(",\n");
  }
  if(paramcount > 0){
    printf(typedecode(LLVMTypeOf(args[paramcount-1]), 0), paramcount-1);
  }
  printf("};\n\
  LLVMGenericValueRef exec_res = LLVMRunFunction(engine, myfn, %d, exec_args);\n\
  int retval = LLVMGenericValueToInt(exec_res, 0); // wrong, make type-depdendent\n\
  LLVMDisposePassManager(pass);\n\
  LLVMDisposeExecutionEngine(engine);\n\
  return retval;\n\
}\n\
  ", paramcount);
}

void print_func(LLVMValueRef fn){
  if(!LLVMIsDeclaration(fn)){ // make sure func is defined here, not printf
    const char *x = LLVMGetValueName(fn);
    LLVMTypeRef r = LLVMGetReturnType(LLVMGetReturnType(LLVMTypeOf(fn)));
    const char *decr = typedecode(r, 1);
    printf("%s %s(", decr, x);

    unsigned paramcount = LLVMCountParams(fn);
    LLVMValueRef *args = (LLVMValueRef *)malloc(paramcount * sizeof(LLVMValueRef));
    LLVMGetParams(fn, args);
    if(paramcount > 0){
      printf("%s arg0", typedecode(LLVMTypeOf(args[0]),1));
    }
    for(int i=0;i<(paramcount-1);i++){
      printf(", %s arg%d", typedecode(LLVMTypeOf(args[i+1]),1), i+1);
    }
    printf("){\n\
      if(!llvm_initialized){ init_llvm(); }\n\
  char *error = NULL; // Used to retrieve messages from functions\n\
      \n");
    print_funcsetup(x);
    print_exec(args, paramcount);
    free(args);
  }

}


int main (int argc, char const *argv[])
{
//  LLVMLinkInJIT();
//  LLVMInitializeNativeTarget();
  print_preamble("awesomex.h"); 
  LLVMModuleRef mod = makeLLVMModule();
  LLVMValueRef myfn = LLVMGetFirstFunction(mod);
  while(myfn != LLVMGetLastFunction(mod)){
    print_func(myfn);
    myfn = LLVMGetNextFunction(myfn);
  }
  print_func(myfn);

//  LLVMValueRef print = LLVMGetNamedFunction(mod, "print");
//  LLVMModuleRef mod = LLVMModuleCreateWithName("fac_module");
//  LLVMTypeRef fac_args[] = { LLVMInt32Type() };
//  LLVMValueRef fac = LLVMAddFunction(mod, "fac", LLVMFunctionType(LLVMInt32Type(), fac_args, 1, 0));
//  LLVMSetFunctionCallConv(fac, LLVMCCallConv);
//  LLVMValueRef n = LLVMGetParam(fac, 0);
//
//  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fac, "entry");
//  LLVMBasicBlockRef iftrue = LLVMAppendBasicBlock(fac, "iftrue");
//  LLVMBasicBlockRef iffalse = LLVMAppendBasicBlock(fac, "iffalse");
//  LLVMBasicBlockRef end = LLVMAppendBasicBlock(fac, "end");
//  LLVMBuilderRef builder = LLVMCreateBuilder();
//
//  LLVMPositionBuilderAtEnd(builder, entry);
//  LLVMValueRef If = LLVMBuildICmp(builder, LLVMIntEQ, n, LLVMConstInt(LLVMInt32Type(), 0, 0), "n == 0");
//  LLVMBuildCondBr(builder, If, iftrue, iffalse);
//
//  LLVMPositionBuilderAtEnd(builder, iftrue);
//  LLVMValueRef res_iftrue = LLVMConstInt(LLVMInt32Type(), 1, 0);
//  LLVMBuildBr(builder, end);
//
//  LLVMPositionBuilderAtEnd(builder, iffalse);
//  LLVMValueRef n_minus = LLVMBuildSub(builder, n, LLVMConstInt(LLVMInt32Type(), 1, 0), "n - 1");
//  LLVMValueRef call_fac_args[] = {n_minus};
//  LLVMValueRef call_fac = LLVMBuildCall(builder, fac, call_fac_args, 1, "fac(n - 1)");
//  LLVMValueRef res_iffalse = LLVMBuildMul(builder, n, call_fac, "n * fac(n - 1)");
//  LLVMBuildBr(builder, end);
//
//  LLVMPositionBuilderAtEnd(builder, end);
//  LLVMValueRef res = LLVMBuildPhi(builder, LLVMInt32Type(), "result");
//  LLVMValueRef phi_vals[] = {res_iftrue, res_iffalse};
//  LLVMBasicBlockRef phi_blocks[] = {iftrue, iffalse};
//  LLVMAddIncoming(res, phi_vals, phi_blocks, 2);
//  LLVMBuildRet(builder, res);

  //----------------------------
//  LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
//  LLVMDisposeMessage(error); // Handler == LLVMAbortProcessAction -> No need to check errors
//
//
//  LLVMExecutionEngineRef engine;
//  LLVMModuleProviderRef provider = LLVMCreateModuleProviderForExistingModule(mod);
//  error = NULL;
//  if(LLVMCreateJITCompiler(&engine, provider, 2, &error) != 0) {
//    fprintf(stderr, "%s\n", error);
//    LLVMDisposeMessage(error);
//    abort();
//  }
//
//  LLVMPassManagerRef pass = LLVMCreatePassManager();
//  LLVMAddTargetData(LLVMGetExecutionEngineTargetData(engine), pass);
//  LLVMAddConstantPropagationPass(pass);
//  LLVMAddInstructionCombiningPass(pass);
//  LLVMAddPromoteMemoryToRegisterPass(pass);
//  // LLVMAddDemoteMemoryToRegisterPass(pass); // Demotes every possible value to memory
//  LLVMAddGVNPass(pass);
//  LLVMAddCFGSimplificationPass(pass);
//  LLVMRunPassManager(pass, mod);
//  LLVMDumpModule(mod);
//
//  LLVMGenericValueRef exec_args[] = {LLVMCreateGenericValueOfInt(LLVMInt32Type(), 10, 0)};
//  //LLVMGenericValueRef exec_res = LLVMRunFunction(engine, fac, 1, exec_args);
//  LLVMGenericValueRef exec_res = LLVMRunFunction(engine, print, 1, exec_args);
//  fprintf(stderr, "\n");
//  //fprintf(stderr, "; Running fac(10) with JIT...\n");
//  fprintf(stderr, "; Running print(10) with JIT...\n");
//  fprintf(stderr, "; Result: %d\n", LLVMGenericValueToInt(exec_res, 0));
//
//  LLVMDisposePassManager(pass);
//  // LLVMDisposeBuilder(builder);
//  LLVMDisposeExecutionEngine(engine);
  return 0;
}

