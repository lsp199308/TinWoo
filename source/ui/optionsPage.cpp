#include <filesystem>
#include <switch.h>
#include "ui/MainApplication.hpp"
#include "ui/mainPage.hpp"
#include "ui/instPage.hpp"
#include "ui/optionsPage.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/curl.hpp"
#include "util/unzip.hpp"
#include "util/lang.hpp"
#include "ui/instPage.hpp"
#include "sigInstall.hpp"

#define COLOR(hex) pu::ui::Color::FromHex(hex)


namespace inst::ui {
    extern MainApplication *mainApp;
    s32 prev_touchcount=0;

    std::vector<std::string> languageStrings = {"En", "Jpn", "Fr", "De", "It", "Ru", "繁体", "简体"};

    optionsPage::optionsPage() : Layout::Layout() {
    		this->infoRect = Rectangle::New(0, 95, 1280, 60, COLOR("#00000080"));
    		this->SetBackgroundColor(COLOR("#000000FF"));
        this->topRect = Rectangle::New(0, 0, 1280, 94, COLOR("#000000FF"));
        this->botRect = Rectangle::New(0, 659, 1280, 61, COLOR("#000000FF"));
        
		if (inst::config::gayMode) {
			if (std::filesystem::exists(inst::config::appDir + "/images/Settings.png")) this->titleImage = Image::New(0, 0, (inst::config::appDir + "/images/Settings.png"));
			else this->titleImage = Image::New(0, 0, "romfs:/images/Settings.png");
			if (std::filesystem::exists(inst::config::appDir + "/images/Background.png")) this->SetBackgroundImage(inst::config::appDir + "/images/Background.png");
			else this->SetBackgroundImage("romfs:/images/Background.png");
            this->appVersionText = TextBlock::New(1200, 680, "v" + inst::config::appVersion);
        }
     else {
			this->SetBackgroundImage("romfs:/images/Background.png");
            this->titleImage = Image::New(0, 0, "romfs:/images/Settings.png");
            this->appVersionText = TextBlock::New(1200, 680, "v" + inst::config::appVersion);
        }
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->appVersionText->SetFont(pu::ui::MakeDefaultFontName(20));
        this->pageInfoText = TextBlock::New(10, 109, "options.title"_lang);
        this->pageInfoText->SetFont(pu::ui::MakeDefaultFontName(30));
        this->pageInfoText->SetColor(COLOR("#FFFFFFFF"));
        this->butText = TextBlock::New(10, 678, "options.buttons"_lang);
        this->butText->SetColor(COLOR("#FFFFFFFF"));
        this->menu = pu::ui::elm::Menu::New(0, 156, 1280, COLOR("#FFFFFF00"), COLOR("#4f4f4d33"), 84, (506 / 84));
        this->menu->SetItemsFocusColor(COLOR("#4f4f4dAA"));
        this->menu->SetScrollbarColor(COLOR("#1A1919FF"));
        this->Add(this->topRect);
        this->Add(this->infoRect);
        this->Add(this->botRect);
        this->Add(this->titleImage);
        this->Add(this->appVersionText);
        this->Add(this->butText);
        this->Add(this->pageInfoText);
        this->setMenuText();
        this->Add(this->menu);
    }

    void optionsPage::askToUpdate(std::vector<std::string> updateInfo) {
            if (!mainApp->CreateShowDialog("options.update.title"_lang, "options.update.desc0"_lang + updateInfo[0] + "options.update.desc1"_lang, {"options.update.opt0"_lang, "common.cancel"_lang}, false)) {
                inst::ui::instPage::loadInstallScreen();
                inst::ui::instPage::setTopInstInfoText("options.update.top_info"_lang + updateInfo[0]);
                inst::ui::instPage::setInstBarPerc(0);
                inst::ui::instPage::setInstInfoText("options.update.bot_info"_lang + updateInfo[0]);
                try {
                    std::string downloadName = inst::config::appDir + "/temp_download.zip";
                    inst::curl::downloadFile(updateInfo[1], downloadName.c_str(), 0, true);
                    romfsExit();
                    inst::ui::instPage::setInstInfoText("options.update.bot_info2"_lang + updateInfo[0]);
                    inst::zip::extractFile(downloadName, "sdmc:/");
                    std::filesystem::remove(downloadName);
                    mainApp->CreateShowDialog("options.update.complete"_lang, "options.update.end_desc"_lang, {"common.ok"_lang}, false);
                } catch (...) {
                    mainApp->CreateShowDialog("options.update.failed"_lang, "options.update.end_desc"_lang, {"common.ok"_lang}, false);
                }
                mainApp->FadeOut();
                mainApp->Close();
            }
        return;
    }

    std::string optionsPage::getMenuOptionIcon(bool ourBool) {
        if(ourBool) return "romfs:/images/icons/check-box-outline.png";
        else return "romfs:/images/icons/checkbox-blank-outline.png";
    }

    std::string optionsPage::getMenuLanguage(int ourLangCode) {
    	 if (ourLangCode >= 0) return languageStrings[ourLangCode];
    	 	else {
    	 		return "options.language.system_language"_lang;
    	 	}
    }
    
    void sigPatchesMenuItem_Click() {
        sig::installSigPatches();
    }
    
    void thememessage() {
    	int ourResult = inst::ui::mainApp->CreateShowDialog("main.theme.title"_lang, "main.theme.desc"_lang, {"common.ok"_lang, "common.cancel"_lang}, true);
            if (ourResult != 0) {
            	//
            }
            else{
            	mainApp->FadeOut();
            	mainApp->Close();
            }
		}

    void optionsPage::setMenuText() {
        this->menu->ClearItems();
        auto ignoreFirmOption = pu::ui::elm::MenuItem::New("options.menu_items.ignore_firm"_lang);
        ignoreFirmOption->SetColor(COLOR("#FFFFFFFF"));
        ignoreFirmOption->SetIcon(this->getMenuOptionIcon(inst::config::ignoreReqVers));
        this->menu->AddItem(ignoreFirmOption);
        auto validateOption = pu::ui::elm::MenuItem::New("options.menu_items.nca_verify"_lang);
        validateOption->SetColor(COLOR("#FFFFFFFF"));
        validateOption->SetIcon(this->getMenuOptionIcon(inst::config::validateNCAs));
        this->menu->AddItem(validateOption);
        auto overclockOption = pu::ui::elm::MenuItem::New("options.menu_items.boost_mode"_lang);
        overclockOption->SetColor(COLOR("#FFFFFFFF"));
        overclockOption->SetIcon(this->getMenuOptionIcon(inst::config::overClock));
        this->menu->AddItem(overclockOption);
        auto deletePromptOption = pu::ui::elm::MenuItem::New("options.menu_items.ask_delete"_lang);
        deletePromptOption->SetColor(COLOR("#FFFFFFFF"));
        deletePromptOption->SetIcon(this->getMenuOptionIcon(inst::config::deletePrompt));
        this->menu->AddItem(deletePromptOption);
        auto autoUpdateOption = pu::ui::elm::MenuItem::New("options.menu_items.auto_update"_lang);
        autoUpdateOption->SetColor(COLOR("#FFFFFFFF"));
        autoUpdateOption->SetIcon(this->getMenuOptionIcon(inst::config::autoUpdate));
        this->menu->AddItem(autoUpdateOption);
        
        auto gayModeOption = pu::ui::elm::MenuItem::New("options.menu_items.gay_option"_lang);
        gayModeOption->SetColor(COLOR("#FFFFFFFF"));
        gayModeOption->SetIcon(this->getMenuOptionIcon(inst::config::gayMode));
        this->menu->AddItem(gayModeOption);
        
        auto useSoundOption = pu::ui::elm::MenuItem::New("options.menu_items.useSound"_lang);
        useSoundOption->SetColor(COLOR("#FFFFFFFF"));
        useSoundOption->SetIcon(this->getMenuOptionIcon(inst::config::useSound));
        this->menu->AddItem(useSoundOption);
        
        auto useoldphp = pu::ui::elm::MenuItem::New("options.menu_items.useoldphp"_lang);
        useoldphp->SetColor(COLOR("#FFFFFFFF"));
        useoldphp->SetIcon(this->getMenuOptionIcon(inst::config::useoldphp));
        this->menu->AddItem(useoldphp);
        
        auto httpkeyboard = pu::ui::elm::MenuItem::New("options.menu_items.usehttpkeyboard"_lang);
        httpkeyboard->SetColor(COLOR("#FFFFFFFF"));
        httpkeyboard->SetIcon(this->getMenuOptionIcon(inst::config::httpkeyboard));
        this->menu->AddItem(httpkeyboard);
        
        auto SigPatch = pu::ui::elm::MenuItem::New("main.menu.sig"_lang);
        SigPatch->SetColor(COLOR("#FFFFFFFF"));
        this->menu->AddItem(SigPatch);
        
        auto sigPatchesUrlOption = pu::ui::elm::MenuItem::New("options.menu_items.sig_url"_lang + inst::util::shortenString(inst::config::sigPatchesUrl, 42, false));
        sigPatchesUrlOption->SetColor(COLOR("#FFFFFFFF"));
        this->menu->AddItem(sigPatchesUrlOption);
        
        auto httpServerUrlOption = pu::ui::elm::MenuItem::New("options.menu_items.http_url"_lang + inst::util::shortenString(inst::config::httpIndexUrl, 42, false));
        httpServerUrlOption->SetColor(COLOR("#FFFFFFFF"));
        this->menu->AddItem(httpServerUrlOption);  
        
        auto languageOption = pu::ui::elm::MenuItem::New("options.menu_items.language"_lang + this->getMenuLanguage(inst::config::languageSetting));
        languageOption->SetColor(COLOR("#FFFFFFFF"));
        this->menu->AddItem(languageOption);
        auto updateOption = pu::ui::elm::MenuItem::New("options.menu_items.check_update"_lang);
        updateOption->SetColor(COLOR("#FFFFFFFF"));
        this->menu->AddItem(updateOption);
        auto creditsOption = pu::ui::elm::MenuItem::New("options.menu_items.credits"_lang);
        creditsOption->SetColor(COLOR("#FFFFFFFF"));
        this->menu->AddItem(creditsOption);
    }

    void optionsPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint touch_pos) {
        
        if (Down & HidNpadButton_B) {
            mainApp->LoadLayout(mainApp->mainPage);
        }
        
        if (Down & HidNpadButton_ZL)
        	this->menu->SetSelectedIndex(std::max(0, this->menu->GetSelectedIndex() - 6));
        
        if (Down & HidNpadButton_ZR)
        	this->menu->SetSelectedIndex(std::min((s32)this->menu->GetItems().size() - 1, this->menu->GetSelectedIndex() + 6));
        
        HidTouchScreenState state={0};
        
        if  (hidGetTouchScreenStates(&state, 1)) {
          
          if ((Down & HidNpadButton_A) || (state.count != prev_touchcount))
          {
              prev_touchcount = state.count;
              
              if (prev_touchcount != 1) {
      
              std::string keyboardResult;
              int rc;
              std::vector<std::string> downloadUrl;
              std::vector<std::string> languageList;
              int index = this->menu->GetSelectedIndex();
              switch (index) {
                  case 0:
                      inst::config::ignoreReqVers = !inst::config::ignoreReqVers;
                      inst::config::setConfig();
                      this->setMenuText();
                      //makes sure to jump back to the selected item once the menu is reloaded
                      this->menu->SetSelectedIndex(index);
                      //
                      break;
                  case 1:
                      if (inst::config::validateNCAs) {
                          if (inst::ui::mainApp->CreateShowDialog("options.nca_warn.title"_lang, "options.nca_warn.desc"_lang, {"common.cancel"_lang, "options.nca_warn.opt1"_lang}, false) == 1) inst::config::validateNCAs = false;
                      } else inst::config::validateNCAs = true;
                      inst::config::setConfig();
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      break;
                  case 2:
                      inst::config::overClock = !inst::config::overClock;
                      inst::config::setConfig();
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      break;
                  case 3:
                      inst::config::deletePrompt = !inst::config::deletePrompt;
                      inst::config::setConfig();
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      break;
                  case 4:
                      inst::config::autoUpdate = !inst::config::autoUpdate;
                      inst::config::setConfig();
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      break;
                  case 5:
                      if (inst::config::gayMode) {
                          inst::config::gayMode = false;
                          mainApp->mainPage->awooImage->SetVisible(false);
  						
                      }
                      else {
                          inst::config::gayMode = true;
                          mainApp->mainPage->awooImage->SetVisible(true);
                      }
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      thememessage();
                      inst::config::setConfig();
                      break;
                  
                  case 6:
                      if (inst::config::useSound) {
                          inst::config::useSound = false;
                      }
                      else {
                          inst::config::useSound = true;
                      }
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      inst::config::setConfig();
                      break;
                 
                  case 7:
                      if (inst::config::useoldphp) {
                          inst::config::useoldphp = false;
                      }
                      else {
                          inst::config::useoldphp = true;
                      }
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      inst::config::setConfig();
                      break;
                  
                  case 8:
                      if (inst::config::httpkeyboard) {
                          inst::config::httpkeyboard = false;
                      }
                      else {
                          inst::config::httpkeyboard = true;
                      }
                      this->setMenuText();
                      this->menu->SetSelectedIndex(index);
                      inst::config::setConfig();
                      break;
                  
                  case 9:
                      sigPatchesMenuItem_Click();
                      break;
                  
                  case 10:
                      keyboardResult = inst::util::softwareKeyboard("options.sig_hint"_lang, inst::config::sigPatchesUrl.c_str(), 500);
                      if (keyboardResult.size() > 0) {
                          inst::config::sigPatchesUrl = keyboardResult;
                          inst::config::setConfig();
                          this->setMenuText();
                          this->menu->SetSelectedIndex(index);
                      }
                      break;
                  
                  case 11:
                      keyboardResult = inst::util::softwareKeyboard("inst.net.url.hint"_lang, inst::config::httpIndexUrl.c_str(), 500);
                      if (keyboardResult.size() > 0) {
                          inst::config::httpIndexUrl = keyboardResult;
                          inst::config::setConfig();
                          this->setMenuText();
                          this->menu->SetSelectedIndex(index);
                      }
                      break;
                  
                  case 12:
                      languageList = languageStrings;
                      languageList.push_back("options.language.system_language"_lang);
                      rc = inst::ui::mainApp->CreateShowDialog("options.language.title"_lang, "options.language.desc"_lang, languageList, false);
                      if (rc == -1) break;
                      switch(rc) {
                          case 0:
                              inst::config::languageSetting = 0;
                              break;
                          case 1:
                              inst::config::languageSetting = 1;
                              break;
                          case 2:
                              inst::config::languageSetting = 2;
                              break;
                          case 3:
                              inst::config::languageSetting = 3;
                              break;
                          case 4:
                              inst::config::languageSetting = 4;
                              break;
                          case 5:
                              inst::config::languageSetting = 5;
                              break;
                          case 6:
                              inst::config::languageSetting = 6;
                              break;
			  case 7:
                              inst::config::languageSetting = 7;
                              break;
                          default:
                              inst::config::languageSetting = 99;
                      }
                      inst::config::setConfig();
                      mainApp->FadeOut();
                      mainApp->Close();
                      break;
                  case 13:
                      if (inst::util::getIPAddress() == "1.0.0.127") {
                          inst::ui::mainApp->CreateShowDialog("main.net.title"_lang, "main.net.desc"_lang, {"common.ok"_lang}, true);
                          break;
                      }
                      downloadUrl = inst::util::checkForAppUpdate();
                      if (!downloadUrl.size()) {
                          mainApp->CreateShowDialog("options.update.title_check_fail"_lang, "options.update.desc_check_fail"_lang, {"common.ok"_lang}, false);
                          break;
                      }
                      this->askToUpdate(downloadUrl);
                      break;
                  case 14:
                      inst::ui::mainApp->CreateShowDialog("options.credits.title"_lang, "options.credits.desc"_lang, {"common.close"_lang}, true);
                      break;
                  default:
                      break;
              }
          }
        }
    }
  }
}
