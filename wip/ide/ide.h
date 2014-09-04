//////////////////////////////////////////////////////////////////////////////
// Original authors:  Wyo, John Labenski, Otto Wyss
// Copyright: (c) wxGuide, (c) John Labenski, Otto Wyss
// Modified by: Randy Hollines
// Licence: wxWindows licence
//////////////////////////////////////////////////////////////////////////////

#ifndef __IDE_H__
#define __IDE_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wx/app.h"
#include "wx/aui/aui.h"
#include "wx/treectrl.h"
#include "wx/artprov.h"
#include "wx/wxhtml.h"
#include "wx/utils.h"
#include "wx/process.h"
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

#include "edit.h"

class MyApp : public wxApp {

public:
  bool OnInit();
};

//----------------------------------------------------------------------------
//! EditProperties
class GlobalOptions : public wxDialog {

public:

  //! constructor
  GlobalOptions(wxWindow* parent, long style = 0);

private:
  wxStaticText* m_fontSelect;
  wxTextCtrl* m_textCtrl4;
  wxButton* m_pathButton;
  wxRadioButton* m_winEnding;
  wxRadioButton* m_unixEnding;
  wxRadioButton* m_macEndig;
  wxRadioButton* m_tabIdent;
  wxRadioButton* m_spaceIdent;
  wxSpinCtrl* m_identSize;
  wxComboBox* m_comboBox1;
  wxSpinCtrl* font_size;
  wxStdDialogButtonSizer* m_sdbSizer1;
  wxButton* m_sdbSizer1OK;
  wxButton* m_sdbSizer1Cancel;
};

class MyFrame : public wxFrame {
  enum {
    ID_SampleItem
  };

  wxAuiManager aui_manager;
  Notebook* m_notebook;
  size_t m_newPageCount;
  GlobalOptions* m_globalOptions;
  
  void DoUpdate();
  
  wxMenuBar* CreateMenuBar();
  wxAuiToolBar* CreateToolBar();
  wxTreeCtrl* CreateTreeCtrl();
  Notebook* CreateNotebook();
  wxAuiNotebook* CreateInfoCtrl();
  
  wxString ReadInputStream(wxInputStream* in) {
    if (!in) {
      return wxEmptyString;
    }
    
    wxString out;
    while (in->CanRead() && !in->Eof()) {
      wxChar c = in->GetC();
      if(iswprint(c) || isspace(c)) {
        out.Append(c);
      }
    }
    
    return out;
  }

public:
  MyFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE | wxSUNKEN_BORDER);
  ~MyFrame();

  // common
  void OnClose(wxCloseEvent &event);
  // file
  void OnFileNew(wxCommandEvent &event);
  void OnFileNewFrame(wxCommandEvent &event);
  void OnFileOpen(wxCommandEvent &event);
  void OnFileOpenFrame(wxCommandEvent &event);
  void OnFileSave(wxCommandEvent &event);
  void OnFileSaveAs(wxCommandEvent &event);
  void OnFileClose(wxCommandEvent &event);
  void OnEdit(wxCommandEvent &event);
  void OnFindReplace(wxCommandEvent &event);
  void OnOptions(wxCommandEvent &event);

  DECLARE_EVENT_TABLE()
};

// TODO: move this into a class
class MyProcess : public wxProcess {
public:
  MyProcess() : wxProcess(wxPROCESS_REDIRECT) {}

  ~MyProcess() {}

  void OnTerminate(int pid, int status) {}
};

#endif
