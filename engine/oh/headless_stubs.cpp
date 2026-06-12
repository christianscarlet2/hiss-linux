// headless_stubs.cpp — Linux server build.
//
// The screen-scraper, OCR, autoplayer button/slider widgets, chat window and
// MFC document/frame are the *physical-interaction + GUI* layer of OpenHoldem.
// On the headless server they are REPLACED by the Scarlet Beast API feed, so
// here they are inert stubs: just enough to satisfy the linker. Table state is
// injected via the API bridge, not scraped.
#include "stdafx.h"

#include "CScraper.h"
#include "CAutoOcr.h"
#include "CAutoplayerButton.h"
#include "CBetsizeInputBox.h"
#include "CBetSlider.h"
#include "CCasinoHotkey.h"
#include "ChatTerminalWindow.h"

// ---------------------------------------------------------------- singletons
CScraper*          p_scraper          = nullptr;  // scraper replaced by API feed; pooled-new must not run at static init
CAutoOcr*          p_auto_ocr         = nullptr;
CChatTerminalWindow* p_chat_terminal  = nullptr;

// chat module global (declared in PokerChat.hpp)
CString _the_chat_message;

// ---------------------------------------------------------------- CScraper
CScraper::CScraper(void) {}
CScraper::~CScraper(void) {}
void CScraper::CreateBitmaps(void) {}
void CScraper::DeleteBitmaps(void) {}
bool CScraper::EvaluateRegion(CString, CString* result) { if (result) *result = ""; return false; }
bool CScraper::IsCommonAnimation() { return false; }
bool CScraper::IsIdenticalScrape() { return false; }
void CScraper::PreOcrParallel() {}
void CScraper::RefreshObserverState() {}
void CScraper::ClearAllPlayerNames() {}
void CScraper::ScrapeDealer() {}
void CScraper::ScrapeButtons(CString, CString) {}
void CScraper::ScrapeActionButtons() {}
void CScraper::ScrapeActionButtonLabels() {}
void CScraper::ScrapeInterfaceButtons() {}
void CScraper::ScrapeBetpotButtons() {}
void CScraper::ScrapeName(const int) {}
void CScraper::ScrapePlayerCards(int) {}
void CScraper::ScrapeSlider() {}
void CScraper::ScrapeCommonCards() {}
void CScraper::ScrapeSeatedActive() {}
void CScraper::ScrapeBetsAndBalances() {}
void CScraper::ScrapeAllPlayerCards() {}
void CScraper::ScrapeColourCodes() {}
void CScraper::ScrapeMTTRegions() {}
void CScraper::ScrapePots() {}
void CScraper::ScrapeLimits() {}

// ---------------------------------------------------------------- CAutoOcr
CAutoOcr::~CAutoOcr() {}
void CAutoOcr::LoadModelSettings() {}

// ---------------------------------------------------------------- user.dll interface (no external formula DLL on the server)
extern "C" {
double ProcessQuery(const char*) { return 0.0; }
void   DLLUpdateOnConnection() {}
void   DLLUpdateOnHandreset() {}
void   DLLUpdateOnNewRound() {}
void   DLLUpdateOnMyTurn() {}
void   DLLUpdateOnHeartbeat() {}
void   DLLUpdateOnNewFormula() {}
}

// ---------------------------------------------------------------- gamestate validation (state arrives pre-validated from the API)
bool ValidateGamestate(bool, bool, bool) { return true; }

// ---------------------------------------------------------------- MFC app accessor
static CWinApp g_hiss_app;
CWinApp* AfxGetAppPtr() { return &g_hiss_app; }

// ---------------------------------------------------------------- chat terminal (GUI; inert on the server)
void ChatTerminalClear() {}
void ChatTerminalClearScreen(CString) {}
void ChatTerminalAppend(int, CString) {}
void ChatTerminalAppendToScreen(CString, int, CString) {}
void ChatTerminalStreamToScreen(CString, int, CString) {}

// ---------------------------------------------------------------- OCR + scraper-output dialog + doc/frame (GUI; inert on the server)
#include "DialogScraperOutput.h"
#include "MainFrm.h"
#include "OpenHoldemDoc.h"
#include "BringKeyboard.h"
#include "CompareArgs.h"

CAutoOcr::CAutoOcr() {}
CString CAutoOcr::GetDetectTemplateResult(CString, CString, RECT*) { return CString(); }
CAutoOcr* AutoOcr() { return p_auto_ocr; }  // null on the server (OCR replaced by API)

CDlgScraperOutput* m_ScraperOutputDlg = nullptr;
void CDlgScraperOutput::DestroyWindowSafely() {}
void CDlgScraperOutput::UpdateDisplay() {}
void CDlgScraperOutput::Reset() {}

void CMainFrame::ResetDisplay() {}

void CheckBringKeyboard() {}

CompareArgs::CompareArgs() {}
CompareArgs::~CompareArgs() {}

// ---------------------------------------------------------------- last GUI accessors (inert)
#include "ChatTerminalWindow.h"
void CChatTerminalWindow::MaybeUpdatePotOddsFromTableState() {}

COpenHoldemDoc* COpenHoldemDoc::GetDocument() { return nullptr; }
CMainFrame* PMainframe() { return nullptr; }
