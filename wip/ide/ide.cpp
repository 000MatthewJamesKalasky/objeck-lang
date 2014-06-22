//////////////////////////////////////////////////////////////////////////////
// Purpose:     Objeck IDE Demo
// Maintainer:  Modified by Randy Hollines
// Maintainer:  Otto Wyss
// Created:     2003-09-01
// Modified By: Randy Hollines (c) 2014
// Copyright:   (c) wxGuide
// Licence:     wxWindows licence
//////////////////////////////////////////////////////////////////////////////

#include "ide.h"

/////////////////////////
// MyApp
/////////////////////////

bool MyApp::OnInit() 
{
  if (!wxApp::OnInit()) {
    return false;
  }

  wxFrame* frame = new MyFrame(NULL, wxID_ANY, wxT("wxAUI Sample Application"), wxDefaultPosition, wxSize(800, 600));
  frame->Show();

  return true;
}

DECLARE_APP(MyApp)
IMPLEMENT_APP(MyApp)

/////////////////////////
// MyFrame
/////////////////////////

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
  // common
  EVT_CLOSE(MyFrame::OnClose)
  // file
  EVT_MENU(wxID_OPEN, MyFrame::OnFileOpen)
  EVT_MENU(wxID_SAVE, MyFrame::OnFileSave)
  EVT_MENU(wxID_SAVEAS, MyFrame::OnFileSaveAs)
  EVT_MENU(wxID_CLOSE, MyFrame::OnFileClose)
END_EVENT_TABLE()

MyFrame::MyFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : 
    wxFrame(parent, id, title, pos, size, style) 
{
  // setup window manager
  aui_manager.SetManagedWindow(this);
  aui_manager.AddPane(CreateTreeCtrl(), wxAuiPaneInfo().Left());
  aui_manager.AddPane(CreateNotebook(), wxAuiPaneInfo().Centre());
  aui_manager.AddPane(CreateTextCtrl(wxT("blah")), wxAuiPaneInfo().Bottom());
  
  // set menu and status bars
  SetMenuBar(CreateMenuBar());
  CreateStatusBar();
  GetStatusBar()->SetStatusText(wxT("Ready"));

  // set windows sizes
  SetMinSize(wxSize(400, 300));
  
  // set tool bar
  aui_manager.AddPane(CreateToolBar(), wxAuiPaneInfo().
    Name(wxT("toolbar")).Caption(wxT("Toolbar 3")).
    ToolbarPane().Top().Row(1).Position(1));

  // update
  aui_manager.Update();
}

MyFrame::~MyFrame() 
{
  aui_manager.UnInit();
}

void MyFrame::DoUpdate() 
{
  aui_manager.Update();
}

// common event handlers
void MyFrame::OnClose(wxCloseEvent &event) 
{
  /*
  wxCommandEvent evt;
  OnFileClose(evt);
  if (m_edit && m_edit->Modified()) {
    if (event.CanVeto()) event.Veto(true);
    return;
  }
  Destroy();
  */
}

// file event handlers
void MyFrame::OnFileOpen(wxCommandEvent &WXUNUSED(event)) 
{
  /*
  if (!m_edit) return;
#if wxUSE_FILEDLG
  wxString fname;
  wxFileDialog dlg(this, wxT("Open file"), wxEmptyString, wxEmptyString, wxT("Any file (*)|*"),
    wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
  if (dlg.ShowModal() != wxID_OK) return;
  fname = dlg.GetPath();
  FileOpen(fname);
#endif // wxUSE_FILEDLG
  */
}

void MyFrame::OnFileSave(wxCommandEvent &WXUNUSED(event)) 
{
  /*
  if (!m_edit) return;
  if (!m_edit->Modified()) {
    wxMessageBox(_("There is nothing to save!"), _("Save file"),
      wxOK | wxICON_EXCLAMATION);
    return;
  }
  m_edit->SaveFile();
  */
}

void MyFrame::OnFileSaveAs(wxCommandEvent &WXUNUSED(event)) 
{
  /*
  if (!m_edit) return;
#if wxUSE_FILEDLG
  wxString filename = wxEmptyString;
  wxFileDialog dlg(this, wxT("Save file"), wxEmptyString, wxEmptyString, wxT("Any file (*)|*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  filename = dlg.GetPath();
  m_edit->SaveFile(filename);
#endif // wxUSE_FILEDLG
  */
}

void MyFrame::OnFileClose(wxCommandEvent &WXUNUSED(event)) 
{
  /*
  if (!m_edit) return;
  if (m_edit->Modified()) {
    if (wxMessageBox(_("Text is not saved, save before closing?"), _("Close"),
      wxYES_NO | wxICON_QUESTION) == wxYES) {
      m_edit->SaveFile();
      if (m_edit->Modified()) {
        wxMessageBox(_("Text could not be saved!"), _("Close abort"),
          wxOK | wxICON_EXCLAMATION);
        return;
      }
    }
  }
  m_edit->SetFilename(wxEmptyString);
  m_edit->ClearAll();
  m_edit->SetSavePoint();
  */
}

wxMenuBar* MyFrame::CreateMenuBar()
{
  // File menu
  wxMenu *menuFile = new wxMenu;
  menuFile->Append(wxID_OPEN, _("&Open ..\tCtrl+O"));
  menuFile->Append(wxID_SAVE, _("&Save\tCtrl+S"));
  menuFile->Append(wxID_SAVEAS, _("Save &as ..\tCtrl+Shift+S"));
  menuFile->Append(wxID_CLOSE, _("&Close\tCtrl+W"));
  menuFile->AppendSeparator();

  wxMenuBar* menu_bar = new wxMenuBar;
  menu_bar->Append(menuFile, wxT("&File"));
  menu_bar->Append(new wxMenu, wxT("&View"));
  menu_bar->Append(new wxMenu, wxT("&Perspectives"));
  menu_bar->Append(new wxMenu, wxT("&Options"));
  menu_bar->Append(new wxMenu, wxT("&Notebook"));
  menu_bar->Append(new wxMenu, wxT("&Help"));

  return menu_bar;
}

wxAuiToolBar* MyFrame::CreateToolBar()
{
  wxAuiToolBarItemArray prepend_items;
  wxAuiToolBarItemArray append_items;
  wxAuiToolBar* toolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
    wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_OVERFLOW);
  toolbar->SetToolBitmapSize(wxSize(16, 16));
  wxBitmap tb3_bmp1 = wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16));
  toolbar->AddTool(ID_SampleItem + 16, wxT("Check 1"), tb3_bmp1, wxT("Check 1"), wxITEM_CHECK);
  toolbar->AddTool(ID_SampleItem + 17, wxT("Check 2"), tb3_bmp1, wxT("Check 2"), wxITEM_CHECK);
  toolbar->AddTool(ID_SampleItem + 18, wxT("Check 3"), tb3_bmp1, wxT("Check 3"), wxITEM_CHECK);
  toolbar->AddTool(ID_SampleItem + 19, wxT("Check 4"), tb3_bmp1, wxT("Check 4"), wxITEM_CHECK);
  toolbar->AddSeparator();
  toolbar->AddTool(ID_SampleItem + 20, wxT("Radio 1"), tb3_bmp1, wxT("Radio 1"), wxITEM_RADIO);
  toolbar->AddTool(ID_SampleItem + 21, wxT("Radio 2"), tb3_bmp1, wxT("Radio 2"), wxITEM_RADIO);
  toolbar->AddTool(ID_SampleItem + 22, wxT("Radio 3"), tb3_bmp1, wxT("Radio 3"), wxITEM_RADIO);
  toolbar->AddSeparator();
  toolbar->AddTool(ID_SampleItem + 23, wxT("Radio 1 (Group 2)"), tb3_bmp1, wxT("Radio 1 (Group 2)"), wxITEM_RADIO);
  toolbar->AddTool(ID_SampleItem + 24, wxT("Radio 2 (Group 2)"), tb3_bmp1, wxT("Radio 2 (Group 2)"), wxITEM_RADIO);
  toolbar->AddTool(ID_SampleItem + 25, wxT("Radio 3 (Group 2)"), tb3_bmp1, wxT("Radio 3 (Group 2)"), wxITEM_RADIO);
  toolbar->SetCustomOverflowItems(prepend_items, append_items);
  toolbar->Realize();

  return toolbar;
}

// Demo tree
wxTreeCtrl* MyFrame::CreateTreeCtrl() 
{
  wxTreeCtrl* tree = new wxTreeCtrl(this, wxID_ANY,
    wxPoint(0, 0), wxSize(160, 250),
    wxTR_DEFAULT_STYLE | wxNO_BORDER);

  wxImageList* imglist = new wxImageList(16, 16, true, 2);
  imglist->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
  imglist->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16)));
  tree->AssignImageList(imglist);

  wxTreeItemId root = tree->AddRoot(wxT("wxAUI Project"), 0);
  wxArrayTreeItemIds items;



  items.Add(tree->AppendItem(root, wxT("Item 1"), 0));
  items.Add(tree->AppendItem(root, wxT("Item 2"), 0));
  items.Add(tree->AppendItem(root, wxT("Item 3"), 0));
  items.Add(tree->AppendItem(root, wxT("Item 4"), 0));
  items.Add(tree->AppendItem(root, wxT("Item 5"), 0));


  int i, count;
  for (i = 0, count = items.Count(); i < count; ++i)
  {
    wxTreeItemId id = items.Item(i);
    tree->AppendItem(id, wxT("Subitem 1"), 1);
    tree->AppendItem(id, wxT("Subitem 2"), 1);
    tree->AppendItem(id, wxT("Subitem 3"), 1);
    tree->AppendItem(id, wxT("Subitem 4"), 1);
    tree->AppendItem(id, wxT("Subitem 5"), 1);
  }


  tree->Expand(root);

  return tree;
}

wxAuiNotebook* MyFrame::CreateNotebook()
{
  // create the notebook off-window to avoid flicker
  wxSize client_size = GetClientSize();

  wxAuiNotebook* ctrl = new wxAuiNotebook(this, wxID_ANY,
    wxPoint(client_size.x, client_size.y),
    wxSize(430, 200),
    wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER);
  ctrl->Freeze();

  wxBitmap page_bmp = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16));

  ctrl->AddPage(CreateHTMLCtrl(ctrl), wxT("Welcome to wxAUI"), false, page_bmp);
  ctrl->SetPageToolTip(0, "Welcome to wxAUI (this is a page tooltip)");

  wxPanel *panel = new wxPanel(ctrl, wxID_ANY);
  wxFlexGridSizer *flex = new wxFlexGridSizer(4, 2, 0, 0);
  flex->AddGrowableRow(0);
  flex->AddGrowableRow(3);
  flex->AddGrowableCol(1);
  flex->Add(5, 5);   flex->Add(5, 5);
  flex->Add(new wxStaticText(panel, -1, wxT("wxTextCtrl:")), 0, wxALL | wxALIGN_CENTRE, 5);
  flex->Add(new wxTextCtrl(panel, -1, wxT(""), wxDefaultPosition, wxSize(100, -1)),
    1, wxALL | wxALIGN_CENTRE, 5);
  flex->Add(new wxStaticText(panel, -1, wxT("wxSpinCtrl:")), 0, wxALL | wxALIGN_CENTRE, 5);
  flex->Add(new wxSpinCtrl(panel, -1, wxT("5"), wxDefaultPosition, wxSize(100, -1),
    wxSP_ARROW_KEYS, 5, 50, 5), 0, wxALL | wxALIGN_CENTRE, 5);
  flex->Add(5, 5);   flex->Add(5, 5);
  panel->SetSizer(flex);
  ctrl->AddPage(panel, wxT("wxPanel"), false, page_bmp);  
  
  
  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 1"), false, page_bmp);

  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some more text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 2"));

  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some more text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 3"));

  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some more text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 4"));

  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some more text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 5"));

  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some more text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 6"));

  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some more text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 7 (longer title)"));
  ctrl->SetPageToolTip(ctrl->GetPageCount() - 1,
    "wxTextCtrl 7: and the tooltip message can be even longer!");

  ctrl->AddPage(new wxTextCtrl(ctrl, wxID_ANY, wxT("Some more text"),
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxNO_BORDER), wxT("wxTextCtrl 8"));
  
  ctrl->Thaw();
  return ctrl;
}

wxHtmlWindow* MyFrame::CreateHTMLCtrl(wxWindow* parent)
{
  if (!parent)
    parent = this;

  wxHtmlWindow* ctrl = new wxHtmlWindow(parent, wxID_ANY,
    wxDefaultPosition,
    wxSize(400, 300));
  ctrl->SetPage(GetIntroText());
  return ctrl;
}

// Demo text
wxTextCtrl* MyFrame::CreateTextCtrl(const wxString& ctrl_text)
{
  static int n = 0;

  wxString text;
  if (!ctrl_text.empty())
    text = ctrl_text;
  else
    text.Printf(wxT("This is text box %d"), ++n);

  return new wxTextCtrl(this, wxID_ANY, text,
    wxPoint(0, 0), wxSize(150, 90),
    wxNO_BORDER | wxTE_MULTILINE);
}
