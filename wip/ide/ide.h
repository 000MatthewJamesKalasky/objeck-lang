//////////////////////////////////////////////////////////////////////////////
// Original authors:  Wyo, John Labenski, Otto Wyss
// Copyright: (c) wxGuide, (c) John Labenski, Otto Wyss
// Modified by: Randy Hollines
// Licence: wxWindows licence
//////////////////////////////////////////////////////////////////////////////

#ifndef __IDE_H__
#define __IDE_H__

// #include <vld.h>

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

#include "editor.h"
#include "dialogs.h"
#include "opers.h"

class MyFrame;

//----------------------------------------------------------------------------
//! ProjectTreeCtrl
class ProjectTreeCtrl : public wxTreeCtrl {
  MyFrame* m_frame;
  TreeData* item_data;

  void OnItemMenu(wxTreeEvent& event);
  void OnItemActivated(wxTreeEvent& event);

public:
  ProjectTreeCtrl(MyFrame* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size, long style);
  ~ProjectTreeCtrl() {}

  TreeData* GetData() {
    return item_data;
  }
  
  DECLARE_EVENT_TABLE();
};

//----------------------------------------------------------------------------
//! BuildTextCtrl
class BuildTextCtrl : public wxTextCtrl {

public:
  BuildTextCtrl(wxWindow* parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size, long style);
  ~BuildTextCtrl();

  void BuildSuccess(const wxString &output);
  void SyntaxError(const wxString &output);
  void ContextError(const wxString &output);

  DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
//! ExecuteTextCtrl
class ExecuteTextCtrl : public wxTextCtrl {

public:
  ExecuteTextCtrl(wxWindow* parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size, long style);
  ~ExecuteTextCtrl();

  DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
//! MyFrame
class MyFrame : public wxFrame {
  enum {
    ID_SampleItem
  };

  wxAuiManager m_auiManager;
  BuildTextCtrl* m_buildOutput;
  ExecuteTextCtrl* m_executeOutput;
  Notebook* m_notebook;
  size_t m_newPageCount;
  GeneralOptionsManager* m_optionsManager;
  ProjectManager* m_projectManager;
  wxMenu* m_projectView;
  ProjectTreeCtrl* m_tree;

  void DoUpdate();
  
  // tree
  ProjectTreeCtrl* CreateTreeCtrl();
  // menu and toolbar
  wxMenuBar* CreateMenuBar();
  wxAuiToolBar* DoCreateToolBar();
  // tabbed editor
  Notebook* CreateNotebook();
  wxAuiNotebook* CreateInfoCtrl();

  wxString ReadInputStream(wxInputStream* in) {
    if(!in) {
      return wxEmptyString;
    }

    wxString out;
    while(in->CanRead() && !in->Eof()) {
      wxChar c = in->GetC();
      if(iswprint(c) || isspace(c)) {
        out.Append(c);
      }
    }

    return out;
  }

  bool IsProjectLoaded() {
    return m_tree != NULL;
  }

public:
  MyFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos = wxDefaultPosition,
          const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE | wxSUNKEN_BORDER);
  ~MyFrame();

  void OpenFile(const wxString &path) {
    if(m_notebook && path.size() > 0) {
      m_notebook->OpenFile(path);
    }
  }

  // project operations
  ProjectManager* GetProjectManager() {
    return m_projectManager;
  }

  void EnableProjectMenu() {
    m_projectView->Enable(myID_BUILD_PROJECT, true);
    m_projectView->Enable(myID_ADD_FILE_PROJECT, true);
    m_projectView->Enable(myID_REMOVE_FILE_PROJECT, true);
    m_projectView->Enable(myID_PROJECT_OPTIONS, true);
  }

  void DisableProjectMenu() {
    m_projectView->Enable(myID_BUILD_PROJECT, false);
    m_projectView->Enable(myID_ADD_FILE_PROJECT, false);
    m_projectView->Enable(myID_REMOVE_FILE_PROJECT, false);
    m_projectView->Enable(myID_PROJECT_OPTIONS, false);
  }

  void EnableProjectNode(wxTreeItemId id) {
    if(m_tree) {
      m_tree->Expand(id);
    }
  }

  void AddProjectSource(const wxString &source);
  
  // common
  void OnClose(wxCloseEvent &event);
  // project
  void OnProjectNew(wxCommandEvent &event);
  void OnProjectOpen(wxCommandEvent &event);
  void OnProjectClose(wxCommandEvent &event);
  void OnProjectBuild(wxCommandEvent &event);
  void OnAddProjectFile(wxCommandEvent &event);
  void OnRemoveProjectFile(wxCommandEvent &event);
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

//----------------------------------------------------------------------------
//! BuildProcess
class BuildProcess : public wxProcess {
public:
  BuildProcess() : wxProcess(wxPROCESS_REDIRECT) {}

  ~BuildProcess() {}
  
  void OnTerminate(int pid, int status) {}
};

//----------------------------------------------------------------------------
//! MyApp
class MyApp : public wxApp {

public:
  bool OnInit();
};
#endif
