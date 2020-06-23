#include <backend/cogen.hpp>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

namespace ir
{

struct generator
{
  generator(std::string name)
    : ctx(), module(name.c_str(), ctx), _builder(ctx), _alloca(ctx)
  {

  }

  ~generator()
  {
  }

  llvm::LLVMContext ctx;
  llvm::Module module;
  llvm::IRBuilder<> _builder;
  llvm::IRBuilder<> _alloca;
};

bool cogen(generator& gen, const Node* ref)
{

}

bool cogen(std::string name, const Node* ref)
{
  generator gen(name);
  cogen(gen, ref);

  llvm::verifyModule(gen.module);

  if(/*optimize*/true)
  {
    auto fpm = std::make_unique<llvm::legacy::FunctionPassManager>(&gen.module);

    //add mem2reg pass
    fpm->add(llvm::createPromoteMemoryToRegisterPass());

    //add our own pass
    fpm->add(llvm::createSCCPPass());

    fpm->doInitialization();
    for(auto it = gen.module.begin(); it != gen.module.end(); ++it)
    {
      if(!it->isDeclaration())
          fpm->run(*it);
    }
    fpm->doFinalization();
  }
  const std::string base_filename = name.substr(name.find_last_of("/\\") + 1);
  const std::size_t len(base_filename.find_last_of('.'));
  const std::string iroutfile = base_filename.substr(0, len) + ".ll";

  std::error_code errc;
  llvm::raw_fd_ostream os(iroutfile, errc, llvm::sys::fs::OpenFlags::F_Text);
  gen.module.print(os, nullptr);
}

}


