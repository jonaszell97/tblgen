
#include "tblgen/Backend/TableGenBackends.h"
#include "tblgen/Basic/FileManager.h"
#include "tblgen/Message/DiagnosticsEngine.h"
#include "tblgen/Parser.h"
#include "tblgen/Record.h"
#include "tblgen/Support/DynamicLibrary.h"
#include "tblgen/Support/StringSwitch.h"
#include "tblgen/TableGen.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace tblgen;
using namespace tblgen::diag;
using namespace tblgen::support;

using std::string;

namespace {

enum Backend {
   B_Custom,
   B_PrintRecords,
   B_EmitClassHierarchy,
};

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
   string TGFile;
   string OutFile;

   Backend backend = B_PrintRecords;
   string backendName;
   string CustomBackendLib;
};

Options parseOptions(DiagnosticsEngine &Diags, int argc, char **argv)
{
   Options opts{};
   for (int i = 1; i < argc; ++i) {
      string arg(argv[i]);
      if (arg.front() == '-') {
         if (arg == "-o") {
            if (!opts.OutFile.empty())
               Diags.Diag(err_generic_error) << "output file already specified";

            if (++i == argc) {
               Diags.Diag(err_generic_error)
                   << "expecting output filename after -o";

               break;
            }

            opts.OutFile = argv[i];
         }
         else {
            opts.backendName = arg;
            opts.backend
                = StringSwitch<Backend>(opts.backendName)
                      .Case("-print-records", B_PrintRecords)
                      .Case("-emit-class-hierarchy", B_EmitClassHierarchy)
                      .Default(B_Custom);

            if (opts.backend == B_Custom) {
               if (i++ == argc) {
                  Diags.Diag(err_generic_error)
                      << "expecting shared library file name";

                  break;
               }

               opts.CustomBackendLib = argv[i];
            }
         }
      }
      else if (!opts.TGFile.empty()) {
         Diags.Diag(err_generic_error) << "filename already specified";

         break;
      }
      else {
         opts.TGFile = arg;
      }
   }

   return opts;
}

class TblGenDiagConsumer : public DiagnosticConsumer {
public:
   void HandleDiagnostic(const Diagnostic &Diag) override
   {
      std::cerr << Diag.getMsg();
   }
};

} // anonymous namespace

int main(int argc, char **argv)
{
   fs::FileManager FileMgr;
   TblGenDiagConsumer Consumer;
   DiagnosticsEngine Diags(&Consumer, &FileMgr);

   Options opts = parseOptions(Diags, argc, argv);
   if (Diags.getNumErrors() != 0) {
      return 1;
   }

   if (opts.TGFile.empty()) {
      Diags.Diag(err_generic_error) << "no input file specified";

      return 1;
   }

   auto buf = FileMgr.openFile(opts.TGFile);
   if (!buf.Buf) {
      Diags.Diag(err_generic_error) << "file not found";

      return 1;
   }

   TableGen TG(FileMgr, Diags);
   Parser parser(TG, *buf.Buf, buf.SourceId);

   if (!parser.parse()) {
      return 1;
   }

   // use a string stream first so that the actual file is not affected if
   // TblGen crashes
   std::string str;
   std::stringstream OS(str);

   auto &RK = parser.getRK();
   switch (opts.backend) {
   case B_Custom: {
      std::string errMsg;
      auto DyLib = DynamicLibrary::Open(opts.CustomBackendLib.data(), &errMsg);

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
          = reinterpret_cast<void (*)(llvm::raw_ostream &, RecordKeeper &)>(
              Ptr);

      Backend(OS, RK);
      break;
   }
   case B_PrintRecords:
      PrintRecords(OS, RK);
      break;
   case B_EmitClassHierarchy:
      EmitClassHierarchy(OS, RK);
      break;
   }

   if (!opts.OutFile.empty()) {
      std::ofstream ofs(opts.OutFile);
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