
#include "tblgen/Backend/TableGenBackends.h"
#include "tblgen/Basic/FileManager.h"
#include "tblgen/Message/DiagnosticsEngine.h"
#include "tblgen/Parser.h"
#include "tblgen/Record.h"
#include "tblgen/Support/Allocator.h"
#include "tblgen/Support/DynamicLibrary.h"
#include "tblgen/Support/StringSwitch.h"
#include "tblgen/TableGen.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace tblgen;
using namespace tblgen::diag;
using namespace tblgen::support;

using std::string;

namespace {

enum Backend {
   B_Template,
   B_Custom,
   B_PrintRecords,
   B_EmitClassHierarchy,
};

/// Transform a pass name argument into the symbol name to search for, e.g.
/// -do-something --> EmitDoSomething
string symbolFromPassName(string passName)
{
   string s("Emit");
   s.reserve(passName.size());

   assert(passName.front() == '-');
   passName = passName.substr(1);

   bool lastWasDash = true;
   for (auto c : passName) {
      if (lastWasDash) {
         s += (char)::toupper(c);
         lastWasDash = false;
      }
      else if (c == '-') {
         lastWasDash = true;
      }
      else {
         s += c;
      }
   }

   return s;
}

struct Options {
   /// The definition (*.tg) file.
   string tgFile;

   /// The file to print the output to. If empty, use stdout.
   string outFile;

   /// The backend to use, or B_Custom if a custom one was supplied.
   Backend backend = B_PrintRecords;

   /// The name of the custom backend, if applicable.
   string backendName;

   /// The path to the dynamic library containing the custom backend.
   string customBackendLib;

   /// The template file to apply the definitions to.
   string templateFile;
};

void printHelpDialog(std::ostream &OS)
{
   OS << "TblGen, a tool for structured code generation\n"
      << "Version 0.3, Copyright 2019 by Jonas Zell\n"
      << "Usage: tblgen <definition file> <backend> [<backend library>] [-o <output file>]\n"
      << "Refer to /examples for example usage.\n";
}

Options parseOptions(DiagnosticsEngine &Diags, int argc, char **argv)
{
   Options opts{};
   for (int i = 1; i < argc; ++i) {
      string arg(argv[i]);
      if (arg.front() == '-') {
         if (arg == "-o") {
            if (!opts.outFile.empty())
               Diags.Diag(err_generic_error) << "output file already specified";

            if (++i == argc) {
               Diags.Diag(err_generic_error)
                   << "expecting output filename after -o";

               break;
            }

            opts.outFile = argv[i];
         }
         else if (arg == "-t") {
            if (++i == argc) {
               Diags.Diag(err_generic_error)
                  << "expecting template filename after -t";

               break;
            }

            opts.templateFile = argv[i];
            opts.backend = B_Template;
         }
         else if (arg == "--help" || arg == "--version") {
            printHelpDialog(std::cout);
         }
         else {
            opts.backendName = arg;
            opts.backend
                = StringSwitch<Backend>(opts.backendName)
                      .Case("-print-records", B_PrintRecords)
                      .Case("-emit-class-hierarchy", B_EmitClassHierarchy)
                      .Default(B_Custom);

            if (opts.backend == B_Custom) {
               if (++i >= argc) {
                  Diags.Diag(err_generic_error)
                      << "expecting shared library file name";

                  break;
               }

               opts.customBackendLib = argv[i];
            }
         }
      }
      else if (!opts.tgFile.empty()) {
         Diags.Diag(err_generic_error) << "filename already specified";

         break;
      }
      else {
         opts.tgFile = arg;
      }
   }

   return opts;
}

class TblGenDiagConsumer : public DiagnosticConsumer {
public:
   void HandleDiagnostic(const Diagnostic &Diag) override
   {
      std::cout << Diag.getMsg();
      std::cout.flush();
   }
};

} // anonymous namespace

extern "C" void __asan_version_mismatch_check_apple_clang_1100() {}

int main(int argc, char **argv)
{
   if (argc == 1) {
      printHelpDialog(std::cout);
      return 0;
   }

   fs::FileManager FileMgr;
   TblGenDiagConsumer Consumer;

   ArenaAllocator Allocator;
   DiagnosticsEngine Diags(Allocator, &Consumer, &FileMgr);

   Options opts = parseOptions(Diags, argc, argv);
   if (Diags.getNumErrors() != 0) {
      return 1;
   }

   if (opts.tgFile.empty()) {
      Diags.Diag(err_generic_error) << "no input file specified";
      return 1;
   }

   auto maybeBuf = FileMgr.openFile(opts.tgFile);
   if (!maybeBuf) {
      Diags.Diag(err_generic_error) << "file not found: " + opts.tgFile;
      return 1;
   }

   auto &buf = maybeBuf.getValue();
   TableGen TG(Allocator, FileMgr, Diags);
   Parser parser(TG, buf.Buf, buf.SourceId, buf.BaseOffset);

   if (!parser.parse()) {
      return 1;
   }

   // use a string stream first so that the actual file is not affected if
   // TblGen crashes
   std::stringstream OS;

   auto &RK = *TG.GlobalRK;
   std::cout << "main.cpp: RK address: " << (void*)(&RK) << "\n";

   switch (opts.backend) {
   case B_Custom: {
      std::string errMsg;
      auto DyLib = DynamicLibrary::Open(opts.customBackendLib, &errMsg);

      if (!errMsg.empty()) {
         Diags.Diag(err_generic_error) << "error opening dylib: " + errMsg;

         return 1;
      }

      auto Sym = symbolFromPassName(opts.backendName);
      void *Ptr = DyLib.getAddressOfSymbol(Sym);

      if (!Ptr) {
         Diags.Diag(err_generic_error)
             << "dylib does not contain symbol '" + Sym + "'";

         return 1;
      }

      auto Backend
          = reinterpret_cast<void (*)(std::ostream &, RecordKeeper &)>(
              Ptr);

      std::cout << "main.cpp: RK address #2: " << (void*)(&RK) << "\n";
      Backend(OS, RK);
      break;
   }
   case B_PrintRecords:
      PrintRecords(OS, RK);
      break;
   case B_EmitClassHierarchy:
      EmitClassHierarchy(OS, RK);
      break;
   case B_Template: {
      if (!opts.backendName.empty() || !opts.customBackendLib.empty()) {
         Diags.Diag(warn_generic_warn)
            << "backend unused because a template was specified";
      }

      auto maybeTemplateBuf = FileMgr.openFile(opts.templateFile);
      if (!maybeTemplateBuf) {
         Diags.Diag(err_generic_error)
            << "file not found: " + opts.templateFile;

         return 1;
      }

      auto &templateBuf = maybeTemplateBuf.getValue();
      TemplateParser parser(TG, templateBuf.Buf, templateBuf.SourceId,
                             templateBuf.BaseOffset);

      if (!parser.parseTemplate()) {
         return 1;
      }

      OS << parser.getResult();
      break;
   }
   }

   if (!opts.outFile.empty()) {
      std::ofstream ofs(opts.outFile);
      if (ofs.fail()) {
         Diags.Diag(err_generic_error) << strerror(errno);

         return 1;
      }

      ofs << OS.str();
   }
   else {
      std::cout << OS.str();
   }
}