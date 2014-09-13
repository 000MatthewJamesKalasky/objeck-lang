#ifndef __UTILS_H__
#define __UTILS_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#include <string>
#include <vector>
#include <map>
#include <fstream>

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/statbox.h>
#include <wx/dialog.h>
#include <wx/artprov.h>
#include <wx/wxhtml.h>
#include <wx/utils.h>
#include <wx/process.h>
#include <wx/arrstr.h>
#include <wx/hashset.h>

#include "../../src/shared/sys.h"
#include "defsext.h"

using namespace std;

class MyFrame;

//----------------------------------------------------------------------------
//! IniManager

WX_DECLARE_HASH_SET(wxString,
                    wxStringHash,
                    wxStringEqual,
                    StringSet);

class IniManager {
  map<const wxString, map<const wxString, wxString>*> section_map;
  wxString filename, input;
  wchar_t cur_char, next_char;
  size_t cur_pos;
  bool locked;
  wxString objeck_path;
  wxString indent_spacing;
  wxString line_ending;
  
  wxString LoadFile(const wxString &fn);
  void NextChar();
  void Clear();
  wxString Serialize();
  void Deserialize();
  
public:
  IniManager(const wxString &fn);
  ~IniManager();

  // writes a file
  static bool WriteFile(const wxString &fn, const wxString &out);

  // set/retrieve values
  bool IsLocked() { 
    return locked; 
  }
  wxString GetValue(const wxString &sec, const wxString &key);
  bool SetValue(const wxString &sec, const wxString &key, const wxString &value);

  wxArrayString GetListValues(const wxString &sec, const wxString &key);
  bool AddListValue(const wxString &sec, const wxString &key, const wxString &val);
  bool RemoveListValue(const wxString &sec, const wxString &key, const wxString &val);

  // load and save to file 
  bool Load();
  bool Save();
};

//----------------------------------------------------------------------------
//! GeneralOptionsManager
class GeneralOptionsManager {
  IniManager* iniManager;
  
 public:
  GeneralOptionsManager(const wxString &filename);
  ~GeneralOptionsManager();
  
  // options
  void ShowOptionsDialog(wxWindow* parent);
  
  void SetObjeckPath(const wxString &path) {
    iniManager->SetValue(L"Options", L"objeck_path", path);
  }
  
  wxString GetObjeckPath() {
    return iniManager->GetValue(L"Options", L"objeck_path");
  }

  void SetIdentSpacing(const wxString &spacing) {
    iniManager->SetValue(L"Options", L"indent_spacing", spacing);
  }

  wxString GetIdentSpacing() {
    return iniManager->GetValue(L"Options", L"indent_spacing");
  }

  void SetLineEnding(const wxString &ending) {
    iniManager->SetValue(L"Options", L"line_ending", ending);
  }

  wxString GetLineEnding() {
    return iniManager->GetValue(L"Options", L"line_ending");
  }

  void Load() {
    iniManager->Load();
  }
  
  void Save() {
    iniManager->Save();
  }
};

//----------------------------------------------------------------------------
//! IniManager
class ProjectManager {
  IniManager* iniManager;
  
 public:
  ProjectManager(MyFrame* parent, const wxString &name, const wxString &filename);
  ProjectManager(MyFrame* parent, const wxString &filename);
  ~ProjectManager();

  void Load() {
    iniManager->Load();
  }
  
  void Save() {
    iniManager->Save();
  }
  
  bool AddFile(const wxString &filename);
  bool RemoveFile(const wxString &filename);
  wxArrayString GetFiles(); 

  bool AddLibrary(const wxString &name);
  bool RemoveLibrary(const wxString &name);

  bool Compile();
};

#endif
