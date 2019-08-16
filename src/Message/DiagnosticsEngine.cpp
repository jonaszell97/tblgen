//
// Created by Jonas Zell on 15.03.18.
//

#include "tblgen/Message/DiagnosticsEngine.h"

using namespace tblgen;
using namespace tblgen::diag;

Diagnostic::Diagnostic(DiagnosticsEngine &Engine,
                       std::string_view Msg,
                       SeverityLevel Severity)
   : Engine(Engine), Msg(Msg), Severity(Severity)
{ }

DiagnosticsEngine::DiagnosticsEngine(DiagnosticConsumer *Consumer,
                                     fs::FileManager *FileMgr)
   : Consumer(Consumer), FileMgr(FileMgr),
     TooManyErrorsMsgEmitted(false)
{}

void DiagnosticsEngine::finalizeDiag(std::string_view msg,
                                     SeverityLevel Sev) {
   NumArgs = 0;
   NumSourceRanges = 0;

   switch (Sev) {
   case SeverityLevel::Warning: ++NumWarnings; break;
   case SeverityLevel::Error:
      ++NumErrors;
      if (MaxErrors && NumErrors > MaxErrors) {
         if (!TooManyErrorsMsgEmitted) {
            Diag(fatal_too_many_errors);
            TooManyErrorsMsgEmitted = true;
         }
      }

      break;
   case SeverityLevel::Fatal: EncounteredFatalError = true; break;
   default: break;
   }

   if (Consumer && !TooManyErrorsMsgEmitted)
      Consumer->HandleDiagnostic(Diagnostic(*this, msg, Sev));
}
