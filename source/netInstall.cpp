/*
Copyright (c) 2017-2018 Adubbz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cstring>
#include <string>
#include <sys/socket.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <sstream>
#include <curl/curl.h>
#include <thread>
#include <switch.h>
#include "netInstall.hpp"
#include "install/install_nsp.hpp"
#include "install/http_nsp.hpp"
#include "install/install_xci.hpp"
#include "install/http_xci.hpp"
#include "install/install.hpp"
#include "util/error.hpp"
#include "util/network_util.hpp"
#include "util/config.hpp"
#include "util/util.hpp"
#include "util/curl.hpp"
#include "util/lang.hpp"
#include "ui/MainApplication.hpp"
#include "ui/instPage.hpp"

const unsigned int MAX_URL_SIZE = 1024;
const unsigned int MAX_URLS = 256;
const int REMOTE_INSTALL_PORT = 2000;
static int m_serverSocket = 0;
static int m_clientSocket = 0;
bool netConnected = false;

namespace inst::ui {
    extern MainApplication *mainApp;
}

namespace netInstStuff{

    void InitializeServerSocket() try
    {
        // Create a socket
        m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

        if (m_serverSocket < -1)
        {
            THROW_FORMAT("Failed to create a server socket. Error code: %u\n", errno);
        }

        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(REMOTE_INSTALL_PORT);
        server.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(m_serverSocket, (struct sockaddr*) &server, sizeof(server)) < 0)
        {
            THROW_FORMAT("Failed to bind server socket. Error code: %u\n", errno);
        }

        // Set as non-blocking
        fcntl(m_serverSocket, F_SETFL, fcntl(m_serverSocket, F_GETFL, 0) | O_NONBLOCK);

        if (listen(m_serverSocket, 5) < 0) 
        {
            THROW_FORMAT("Failed to listen on server socket. Error code: %u\n", errno);
        }
    }
    catch (std::exception& e)
    {
        LOG_DEBUG("Failed to initialize server socket!\n");
        fprintf(stdout, "%s", e.what());

        if (m_serverSocket != 0)
        {
            close(m_serverSocket);
            m_serverSocket = 0;
        }
        inst::ui::mainApp->CreateShowDialog("Failed to initialize server socket!", (std::string)e.what(), {"OK"}, true);
    }

    void OnUnwound()
    {
        LOG_DEBUG("unwinding view\n");
        if (m_clientSocket != 0)
        {
            close(m_clientSocket);
            m_clientSocket = 0;
        }

        curl_global_cleanup();
    }

    void installTitleNet(std::vector<std::string> ourUrlList, int ourStorage, std::vector<std::string> urlListAltNames, std::string ourSource)
    {
        inst::util::initInstallServices();
        inst::ui::instPage::loadInstallScreen();
        bool nspInstalled = true;
        NcmStorageId m_destStorageId = NcmStorageId_SdCard;

        if (ourStorage) m_destStorageId = NcmStorageId_BuiltInUser;
        unsigned int urlItr;

        std::vector<std::string> urlNames;
        if (urlListAltNames.size() > 0) {
            for (long unsigned int i = 0; i < urlListAltNames.size(); i++) {
                urlNames.push_back(inst::util::shortenString(urlListAltNames[i], 38, true));
            }
        } else {
            for (long unsigned int i = 0; i < ourUrlList.size(); i++) {
                urlNames.push_back(inst::util::shortenString(inst::util::formatUrlString(ourUrlList[i]), 38, true));
            }
        }

        std::vector<int> previousClockValues;
        if (inst::config::overClock) {
            previousClockValues.push_back(inst::util::setClockSpeed(0, 1785000000)[0]);
            previousClockValues.push_back(inst::util::setClockSpeed(1, 76800000)[0]);
            previousClockValues.push_back(inst::util::setClockSpeed(2, 1600000000)[0]);
        }

        try {
            for (urlItr = 0; urlItr < ourUrlList.size(); urlItr++) {
                LOG_DEBUG("%s %s\n", "Install request from", ourUrlList[urlItr].c_str());
                inst::ui::instPage::setTopInstInfoText("inst.info_page.top_info0"_lang + urlNames[urlItr] + ourSource);
                std::unique_ptr<tin::install::Install> installTask;

                if (inst::curl::downloadToBuffer(ourUrlList[urlItr], 0x100, 0x103) == "HEAD") {
                    auto httpXCI = std::make_shared<tin::install::xci::HTTPXCI>(ourUrlList[urlItr]);
                    installTask = std::make_unique<tin::install::xci::XCIInstallTask>(m_destStorageId, inst::config::ignoreReqVers, httpXCI);
                } else {
                    auto httpNSP = std::make_shared<tin::install::nsp::HTTPNSP>(ourUrlList[urlItr]);
                    installTask = std::make_unique<tin::install::nsp::NSPInstall>(m_destStorageId, inst::config::ignoreReqVers, httpNSP);
                }

                LOG_DEBUG("%s\n", "Preparing installation");
                inst::ui::instPage::setInstInfoText("inst.info_page.preparing"_lang);
                inst::ui::instPage::setInstBarPerc(0);
                installTask->Prepare();
                installTask->Begin();
            }
        }
        catch (std::exception& e) {
            LOG_DEBUG("Failed to install");
            LOG_DEBUG("%s", e.what());
            fprintf(stdout, "%s", e.what());
            inst::ui::instPage::setInstInfoText("inst.info_page.failed"_lang + urlNames[urlItr]);
            inst::ui::instPage::setInstBarPerc(0);
            std::string audioPath = "";
            if (std::filesystem::exists(inst::config::appDir + "/sounds/OHNO.WAV")) {
            		audioPath = (inst::config::appDir + "/sounds/OHNO.WAV");
            		}
            		else {
            			audioPath = "romfs:/audio/bark.wav";
            		}
            std::thread audioThread(inst::util::playAudio,audioPath);
            inst::ui::mainApp->CreateShowDialog("inst.info_page.failed"_lang + urlNames[urlItr] + "!", "inst.info_page.failed_desc"_lang + "\n\n" + (std::string)e.what(), {"common.ok"_lang}, true);
            audioThread.join();
            nspInstalled = false;
        }

        if (previousClockValues.size() > 0) {
            inst::util::setClockSpeed(0, previousClockValues[0]);
            inst::util::setClockSpeed(1, previousClockValues[1]);
            inst::util::setClockSpeed(2, previousClockValues[2]);
        }

        LOG_DEBUG("Telling the server we're done installing\n");
        // Send 1 byte ack to close the server
        u8 ack = 0;
        tin::network::WaitSendNetworkData(m_clientSocket, &ack, sizeof(u8));

        if(nspInstalled) {
            inst::ui::instPage::setInstInfoText("inst.info_page.complete"_lang);
            inst::ui::instPage::setInstBarPerc(100);
            std::string audioPath = "";
            
            if (inst::config::useSound) {
            	if (std::filesystem::exists(inst::config::appDir + "/sounds/YIPPEE.WAV")) {
            		audioPath = (inst::config::appDir + "/sounds/YIPPEE.WAV");
            		}
            		else {
            			audioPath = "romfs:/audio/ameizing.mp3";
            		}
            	std::thread audioThread(inst::util::playAudio,audioPath);	

              if (ourUrlList.size() > 1) inst::ui::mainApp->CreateShowDialog(std::to_string(ourUrlList.size()) + "inst.info_page.desc0"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
              else inst::ui::mainApp->CreateShowDialog(urlNames[0] + "inst.info_page.desc1"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
              audioThread.join();
            }
            
            else{
            	if (ourUrlList.size() > 1) inst::ui::mainApp->CreateShowDialog(std::to_string(ourUrlList.size()) + "inst.info_page.desc0"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
              else inst::ui::mainApp->CreateShowDialog(urlNames[0] + "inst.info_page.desc1"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
            }
        }
        
        LOG_DEBUG("Done");
        inst::ui::instPage::loadMainMenu();
        inst::util::deinitInstallServices();
        return;
    }

    std::vector<std::string> OnSelected()
    {
    		/*
    		https://switchbrew.github.io/libnx/hid_8h.html#aa163470a1a7b811662e5c38905cc86fba4d9ae7fa7e27704abaf86c8a8a5398bd
    		*/
    		padConfigureInput(8, HidNpadStyleSet_NpadStandard);
    		PadState pad;
    		padInitializeAny(&pad);
        
        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();

        OnUnwound();

        try
        {
            ASSERT_OK(curl_global_init(CURL_GLOBAL_ALL), "Curl failed to initialized");

            // Initialize the server socket if it hasn't already been
            if (m_serverSocket == 0)
            {
                InitializeServerSocket();

                if (m_serverSocket <= 0)
                {
                    THROW_FORMAT("Server socket failed to initialize.\n");
                    close(m_serverSocket); //close if already open.
                    m_serverSocket = 0; //reset so we can try again.
                }
            }

            std::string ourIPAddress = inst::util::getIPAddress();
            inst::ui::mainApp->netinstPage->pageInfoText->SetText("inst.net.top_info1"_lang + ourIPAddress);
            inst::ui::mainApp->CallForRender();
            netConnected = false;
            LOG_DEBUG("%s %s\n", "Switch IP is ", ourIPAddress.c_str());
            LOG_DEBUG("%s\n", "Waiting for network");
            LOG_DEBUG("%s\n", "B to cancel");
            
            std::vector<std::string> urls;
            
            while (true)
            {
            		padUpdate(&pad);
            		
            		// If we don't update the UI occasionally the Switch basically crashes on this screen if you press the home button
                u64 newTime = armGetSystemTick();
                if (newTime - startTime >= freq * 0.25) {
                    startTime = newTime;
                    inst::ui::mainApp->CallForRender();
                }

                // Break on input pressed
                u64 kDown = padGetButtonsDown(&pad);

                if (kDown & HidNpadButton_B)
                {
                    break;
                } 
                if (kDown & HidNpadButton_Y)
                {
                    return {"supplyUrl"};
                }
                if (kDown & HidNpadButton_X)
                {
                    inst::ui::mainApp->CreateShowDialog("inst.net.help.title"_lang, "inst.net.help.desc"_lang, {"common.ok"_lang}, true);
                }
                
                if (kDown & HidNpadButton_Minus) {
                    std::string url = inst::util::softwareKeyboard("inst.net.url.hint"_lang, inst::config::httpIndexUrl, 500);
                    if(url == "") {
                    	url = "http://127.0.0.1";
                    }
                    
                    else {
                    	
                    	inst::config::httpIndexUrl = url;
                    	inst::config::setConfig();
                    

                      std::string response;
                      if (inst::util::formatUrlString(url) == "" || url == "https://" || url == "http://")
                          inst::ui::mainApp->CreateShowDialog("inst.net.url.invalid"_lang, "", {"common.ok"_lang}, false);
                      else {
                          if (url[url.size() - 1] != '/')
                              url += '/';
                          response = inst::curl::downloadToBuffer(url);
                      }

                      if (!response.empty()) {
                          if (response[0] == '{')              
                              try {
                              	nlohmann::json j = nlohmann::json::parse(response);
                              	for (const auto &file : j["files"]) {
                              		urls.push_back(file["url"]);
                            		}
                                return urls;
                              } 
                              catch (const nlohmann::detail::exception& ex) {
                              	LOG_DEBUG("Failed to parse JSON\n");
                              }
                          
                          else if (response[0] == '<') {
                              std::size_t index = 0;
                              while (index < response.size()) {
                                  std::string link;
                                  auto found = response.find("href=\"", index);
                                  if (found == std::string::npos)
                                      break;
                                  
                                  index = found + 6;
                                  while (index < response.size()) {
                                      if (response[index] == '"') {
                                          if (link.find("../") == std::string::npos)
                                              if (link.find(".nsp") != std::string::npos || link.find(".nsz") != std::string::npos || link.find(".xci") != std::string::npos || link.find(".xcz") != std::string::npos){
                                              	if (inst::config::useoldphp) {
                                              		urls.push_back(url + link);
                                              	}
                                              	else {
                                              		urls.push_back(link);
                                              	}
                                              }
                                          break;
                                      }
                                      link += response[index++];
                                  }

                              }
                              if (urls.size() > 0){
                              	/*
                              	//debug
                            		FILE * fp;
                            		std::string debug = urls[0];
                            		const char *info = debug.c_str();
                            		fp = fopen ("debug.txt", "w+");
                            		fprintf(fp, "%s", info);
                            		fclose(fp);
                            		*/
                            		return urls;
                              }
                                  
                              LOG_DEBUG("Failed to parse games from HTML\n");
                          }
                      } 
                      
                      else {
                      	LOG_DEBUG("Failed to fetch game list\n");
                      	inst::ui::mainApp->CreateShowDialog("inst.net.index_error"_lang, "inst.net.index_error_info"_lang, {"common.ok"_lang}, true);
                      }
                    }
                }

                struct sockaddr_in client;
                socklen_t clientLen = sizeof(client);

                m_clientSocket = accept(m_serverSocket, (struct sockaddr*)&client, &clientLen);

                if (m_clientSocket >= 0)
                {
                    LOG_DEBUG("%s\n", "Server accepted");
                    u32 size = 0;
                    tin::network::WaitReceiveNetworkData(m_clientSocket, &size, sizeof(u32));
                    size = ntohl(size);

                    LOG_DEBUG("Received url buf size: 0x%x\n", size);

                    if (size > MAX_URL_SIZE * MAX_URLS)
                    {
                        THROW_FORMAT("URL size %x is too large!\n", size);
                    }

                    // Make sure the last string is null terminated
                    auto urlBuf = std::make_unique<char[]>(size+1);
                    memset(urlBuf.get(), 0, size+1);

                    tin::network::WaitReceiveNetworkData(m_clientSocket, urlBuf.get(), size);

                    // Split the string up into individual URLs
                    std::stringstream urlStream(urlBuf.get());
                    std::string segment;

                    while (std::getline(urlStream, segment, '\n')) urls.push_back(segment);
                    std::sort(urls.begin(), urls.end(), inst::util::ignoreCaseCompare);

                    break;
                }
                else if (errno != EAGAIN)
                {
                    THROW_FORMAT("Failed to open client socket with code %u\n", errno);
                }
            }

            return urls;

        }
        catch (std::runtime_error& e)
        {
        		close(m_serverSocket);
            m_serverSocket = 0;
            LOG_DEBUG("Failed to perform remote install!\n");
            LOG_DEBUG("%s", e.what());
            fprintf(stdout, "%s", e.what());
            inst::ui::mainApp->CreateShowDialog("inst.net.failed"_lang, (std::string)e.what(), {"common.ok"_lang}, true);
            return {};
        }
    }
}
