#ifndef __DIALOGS_H__
#define __DIALOGS_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/statline.h>

///////////////////////////////////////////////////////////////////////////////
/// Class GeneralOptions
///////////////////////////////////////////////////////////////////////////////
class GeneralOptions : public wxDialog {
  wxStaticText* m_PathLabel;
  wxTextCtrl* m_pathText;
  wxStaticText* m_spacingLabel;
  wxRadioButton* m_tabSpacingButton;
  wxRadioButton* m_spacesTabButton;
  wxTextCtrl* m_numSpacesText;
  wxRadioBox* m_lineFeedRadio;
  wxStdDialogButtonSizer* m_OkCancelSizer;
  wxButton* m_OkCancelSizerSave;
  wxButton* m_OkCancelSizerCancel;
	
 public:	
  GeneralOptions( wxWindow* parent, const wxString &objeck_path, const wxString &indentation, const wxString &line_endings, 
                  wxWindowID id = wxID_ANY, const wxString& title = wxT("Options"), const wxPoint& pos = wxDefaultPosition, 
                  const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
  ~GeneralOptions();
  
  wxString GetObjeckPath() {
    return wxEmptyString;
  }

  wxString GetIdentSpacing() {
    return wxEmptyString;
  }
  
  wxString GetLineEnding() {
    return wxEmptyString;
  }
};

///////////////////////////////////////////////////////////////////////////////
/// Class NewProject
///////////////////////////////////////////////////////////////////////////////
class NewProject : public wxDialog 
{
  wxStaticText* m_PathLabel;
  wxTextCtrl* m_pathText;
  wxStaticText* m_PathLabel1;
  wxTextCtrl* m_pathText1;
  wxButton* m_button1;
  wxStdDialogButtonSizer* m_sdbSizer4;
  wxButton* m_sdbSizer4OK;
  wxButton* m_sdbSizer4Cancel;
	
 public:	
  NewProject( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("New Project"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
  ~NewProject();
};

#endif
