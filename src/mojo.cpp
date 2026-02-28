#include <fmt/core.h>
#include <fmt/color.h>
#include <json/json.h>
#include <cpr/cpr.h>
#include <ncurses.h>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>

std::time_t now = std::time(nullptr);
std::tm* local_time = std::localtime(&now);

std::string datatime = std::to_string(1900 + local_time->tm_year) + "-" + std::to_string(1 + local_time->tm_mon) + "-" + std::to_string(local_time->tm_mday) + "_" + std::to_string(local_time->tm_hour) + ":" + std::to_string(local_time->tm_min) + std::to_string(local_time->tm_sec);

class ProgressBar {
private:
    int width;
    std::string prefix;
    int last_percent = -1;
    
public:
    ProgressBar(const std::string& text = "", int w = 40) 
        : width(w), prefix(text) {}
    
    void update(int percent) {
        if (percent == last_percent) return;
        
        std::cout << "\r" << std::string(prefix.length() + width + 20, ' ') << "\r";
        
        std::cout << prefix << " [";
        int pos = width * percent / 100;
        for (int i = 0; i < width; ++i) {
            if (i < pos) std::cout << "█";
            else std::cout << "░";
        }
        std::cout << "] " << std::setw(3) << percent << "%";
        std::cout.flush();
        
        last_percent = percent;
    }
    
    void complete() {
        update(100);
        std::cout << " ✓" << std::endl;
    }
};

int instances(std::vector<std::string> inst, int& signaled) {
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true); 
  
  WINDOW *label = newwin(20, 43, 23, 3);
  box(label, 0,0);
  wmove(label, 10, 5);
  wprintw(label, "Select to your instances mojo");
  wmove(label, 18, 2);
  wprintw(label, "KEY_UP,KEY_DOWN - move Enter - select");
  
  refresh();
  wrefresh(label);
  
  WINDOW *instwin = newwin(20, 43, 3, 3);
  box(instwin, 0,0);
  
  int cur_inst_mods = 0;
  int size_inst_mods = inst.size();
  
  start_color();
  init_pair(1, COLOR_BLACK, COLOR_BLUE);
  
  while (true) {
    werase(instwin);
    box(instwin, 0,0);
    for (int i = 0; i < size_inst_mods; i++) {
      if (i == cur_inst_mods) {
        wattron(instwin, COLOR_PAIR(1));
        mvwprintw(instwin, i + 1, 2, "> %s", inst[i].c_str());
        wattroff(instwin, COLOR_PAIR(1));
      } else {
        mvwprintw(instwin, i + 1, 2, " %s", inst[i].c_str());
      }
    }
    
    refresh();
    wrefresh(instwin);
    wrefresh(label);
    
    int key = getch();
    if (key == KEY_UP) {
      cur_inst_mods = (cur_inst_mods > 0) ? cur_inst_mods - 1 : size_inst_mods - 1;
    } else if (key == KEY_DOWN) {
      cur_inst_mods = (cur_inst_mods < size_inst_mods - 1) ? cur_inst_mods + 1 : 0;
    } else if (key == 10) {
      signaled = cur_inst_mods;
      endwin();
      return 1;
    }
  }
  
  refresh();
  wrefresh(instwin);
  wrefresh(label);
  
  return 0;
}

int mods(int argcM, char* argvM[]) {
  std::vector<std::string> modinst; 
  int id;
  std::string modsearch;
  std::string modlimit = "10";
  int offset = 0; 
  if (argcM > 2 && std::string(argvM[2]) == "--url") {
    if (argcM > 3 && std::string(argvM[3]) != "") {
      std::string url_mod = std::string(argvM[3]);
      
      for (const auto& perebor_instances : std::filesystem::directory_iterator("/storage/emulated/0/Android/data/git.artdeell.mojo/files/instances")) {
          modinst.push_back(perebor_instances.path().filename().string());
       
      }
      
      instances(modinst, id);
      std::string getmods = "wget -P /storage/emulated/0/Android/data/git.artdeell.mojo/files/instances/" + modinst[id] + "/mods " + url_mod;
      system(getmods.c_str());
    } else {
      std::cout << "mojo: url not found" << std::endl;
    }
    return 0;
  }
  
  for (int i = 1; i < argcM; i++) {
    if (std::string(argvM[i]) == "--search" && i + 1 < argcM) {
      modsearch = std::string(argvM[i + 1]);
    } else if (std::string(argvM[i]) == "--limit" && i + 1 < argcM) {
      modlimit = std::string(argvM[i + 1]);
    }
  }
  
  for (const auto& perebor_instances : std::filesystem::directory_iterator("/storage/emulated/0/Android/data/git.artdeell.mojo/files/instances")) {
    modinst.push_back(perebor_instances.path().filename().string());
  }
  
  instances(modinst, id);
  
  std::cout << "📃 list mods:" << std::endl;
  
  ProgressBar bar("🌐 Connect to Modrinth...", 50);
  bar.update(1);
  
  cpr::Response modlist = cpr::Get(cpr::Url{"https://api.modrinth.com/v2/search"}, cpr::Parameters{{"query", modsearch},{"limit", modlimit},{"offset", std::to_string(offset)}}, cpr::Header{{"User-agent", "application/json"}, {"User-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36)"}, {"Accept", "application/json"}}, cpr::ProgressCallback([&bar](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadNow, 
  cpr::cpr_off_t uploadTotal, 
  cpr::cpr_off_t uploadNow, 
  intptr_t userdata) -> bool {
    if (downloadTotal > 0) {
      float progress = static_cast<float>(downloadNow) / downloadTotal;
      bar.update(progress);
    }
    
    return true;
  }));
  
  if (modlist.status_code != 200) {
    std::cout << "\n ⚠️ Error: status code: " << modlist.status_code << std::endl;
    return 1;
  }
  
  bar.complete();
  
  std::ofstream modjson_out("mods.json");
  modjson_out << modlist.text;
  modjson_out.close();
  
  std::ifstream modjson("mods.json");
  Json::Value modroot;
  modjson >> modroot;
  
  int mod_i = 0;
  for (const auto& title : modroot["hits"]) {
    fmt::print(fmt::fg(fmt::color::yellow), std::to_string(mod_i));
    std::cout << ". 📦 " << title["title"].asString() << std::endl;
    mod_i++;
  }
  
  fmt::print(fmt::fg(fmt::color::gray), "select mod: ");
  int modid;
  std::cin >> modid;
  std::cout << std::endl;
  
  std::cout << "📊 info mod" << std::endl;
  std::cout << "📢 Name: ";
  fmt::print(fmt::fg(fmt::color::yellow), modroot["hits"][modid]["title"].asString());
  std::cout << std::endl;
  
  std::cout << "🗣️ Author: ";
  fmt::print(fmt::fg(fmt::color::yellow), modroot["hits"][modid]["author"].asString());
  std::cout << std::endl;
  
  std::cout << "📝 Description: ";
  fmt::print(fmt::fg(fmt::color::yellow), modroot["hits"][modid]["description"].asString());
  std::cout << std::endl;
  
  std::cout << "📦 Categories: ";
  for (const auto& loaderlist : modroot["hits"][modid]["display_categories"]) {
    fmt::print(fmt::fg(fmt::color::yellow), loaderlist.asString());
    std::cout << ", ";
  }
  std::cout << std::endl;
  
  std::cout << "🌐 Versions: ";
  for (const auto& verslist : modroot["hits"][modid]["versions"]) {
    fmt::print(fmt::fg(fmt::color::yellow), verslist.asString());
    std::cout << ", ";
  }
  std::cout << std::endl;
  
  fmt::print(fmt::fg(fmt::color::gray), "version: ");
  std::string modversion;
  std::cin >> modversion;
  std::cout << std::endl;
  
  fmt::print(fmt::fg(fmt::color::gray), "loader: ");
  std::string modloader;
  std::cin >> modloader;
  std::cout << std::endl;
  
  std::cout << "⏳ Connect..." << std::endl;
  
  /* std::thread t([&](){
    while (true) {
      std::cout << "\r" << R"( \)" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      std::cout << "\r |" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      std::cout << "\r /" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      std::cout << "\r —" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    std::cout << std::endl;
  });
  t.join(); */
  
  cpr::Response getlinkmod = cpr::Get(cpr::Url{"https://api.modrinth.com/v2/project/" + modroot["hits"][modid]["project_id"].asString() + "/version"},cpr::Parameters{{"game_versions", "[\"" + modversion + "\"]"},{"loaders", "[\"" + modloader + "\"]"}},cpr::Header{{"User-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36)"},{"Accept", "application/json"}});
  
  if (getlinkmod.status_code != 200) {
    std::cout << "⚠️ error: code: " << getlinkmod.status_code << std::endl;
    return 0;
  }
  
  std::ofstream linkmodjson_out("get.json");
  linkmodjson_out << getlinkmod.text;
  linkmodjson_out.close();
  
  std::ifstream linkmodjson("get.json");
  Json::Value linkmodj;
  linkmodjson >> linkmodj;
  
  std::string modlink = "wget -P /storage/emulated/0/Android/data/git.artdeell.mojo/files/instances/" + modinst[id] + "/mods " + linkmodj[0]["files"][0]["url"].asString();
  
  system(modlink.c_str());
  
  std::filesystem::remove("mods.json");
  std::filesystem::remove("get.json");
  
  return 0;
}

int status() {
  std::ifstream user("/storage/emulated/0/Android/data/git.artdeell.mojo/files/shader_dir/usercache.json");
  
  Json::Value userj;
  user >> userj;
  
  std::cout << "📊 status:" << std::endl;
  std::cout << "🗣️ Name: ";
  fmt::print(fmt::fg(fmt::color::yellow), userj[0]["name"].asString());
  std::cout << std::endl;
  
  std::cout << "⏳ Date: ";
  fmt::print(fmt::fg(fmt::color::yellow), userj[0]["expiresOn"].asString());
  std::cout << std::endl;
  return 0;
}

int downinstances(int arg1, char* arg2[]) {
  if (arg1 > 2 && std::string(arg2[2]) == "--instance") {
    if (arg1 > 3 && std::string(arg2[3]) != "") {
      std::string instname = std::string(arg2[3]);
      if (arg1 > 4 && std::string(arg2[4]) == "--v") {
        if (arg1 > 5 && std::string(arg2[5]) != "") {
          std::string instvers = std::string(arg2[5]);
          
          std::string instpath = "/storage/emulated/0/Android/data/git.artdeell.mojo/files/instances/" + instname;
          
          std::filesystem::create_directories(instpath);
          
          std::cout << "[10%] Make instance dir: path: " << instpath << std::endl;
          
          std::string rppath = instpath + "/resourcepacks";
          
          
          std::filesystem::create_directories(rppath);
          
          std::cout << "[15%] make resourcepacks dir" << std::endl;
          
          std::string libpath = instpath + "/lib";
          
          std::filesystem::create_directories(libpath);
          
          std::cout << "[20%] make lib dir" << std::endl;
          
          std::string worldpath = instpath + "/saves";
          
          std::filesystem::create_directories(worldpath);
          
          std::cout << "[30%] make world dir" << std::endl;
          
          std::string confpath = instpath + "/config";
          
          std::filesystem::create_directories(confpath);
          
          std::cout << "[34%] make config dir" << std::endl;
          
          std::cout << "Creating json..." << std::endl;
          
          std::ofstream configinst(instpath + "/mojo_instance.json");
          
          Json::Value configinst_json;
          
          configinst_json["argsMode"] = 0;
          configinst_json["icon"] = "default";
          configinst_json["name"] = instname;
          configinst_json["renderer"] = "opengles2";
          configinst_json["sharedData"] = false;
          configinst_json["versionId"] = instvers;
          
          configinst << configinst_json.toStyledString();
          
          if (!configinst.is_open()) {
            std::cout << "[ERROR] mojo_instance.json was not created" << std::endl;
            return 1;
          } else {
            std::cout << "[40%] make json config instance" << std::endl;
          }
          configinst.close();
          
          std::filesystem::create_directories("/storage/emulated/0/Android/data/git.artdeell.mojo/files/.minecraft/versions/" + instname);
          
          // ...
          
        } else {
          fmt::print(fmt::fg(fmt::color::red), "mojo: error: version not found\n");
          return 1;
        }
      } else {
        fmt::print(fmt::fg(fmt::color::red), "mojo: error: version not found\n");
        return 1;
      }
    } else {
      fmt::print(fmt::fg(fmt::color::red), "mojo: error: instances not found\n");
      return 1;
    }
  }
  return 0;
}

int versionlist() {
  for (const auto& verslist : std::filesystem::directory_iterator("/storage/emulated/0/Android/data/git.artdeell.mojo/files/.minecraft/versions")) {
    fmt::print(fmt::fg(fmt::color::yellow), verslist.path().filename().string());
    std::cout << std::endl;
  }
  return 0;
}

int help() {
  std::cout << "mojo - simple CLI for MojoLauncher" << std::endl;
  std::cout << std::endl;
  std::cout << "template: mojo [OPTION] [ARGUMENT]" << std::endl;
  std::cout << std::endl;
  
  std::cout << "  mojo list    — list minecraft version" << std::endl;
  std::cout << "  mojo --help    — help list" << std::endl;
  std::cout << "  mojo mods    — Mod manager" << std::endl;
  std::cout << R"(       |— --url "URL" — install mod to url(no mod manager))" << std::endl;
  std::cout << R"(       |)" << std::endl;
  std::cout << R"(       |— --search "MOD" — Mod search (mod manager))" << std::endl;
  std::cout << R"(       |_ --limit — Mod Limit (mod manager))" << std::endl;
  std::cout << "  mojo create" << std::endl;
  std::cout << R"(          |— --instance "NAME" — Create an instance)" << std::endl;
  std::cout << R"(          |_ --v [VERSION] — Game version (after --insInstalling the mod at the linktance))" << std::endl;
  std::cout << "  mojo status    — user information" << std::endl;
  std::cout << std::endl;
  std::cout << "Thank you ;)" << std::endl;
  std::cout << "GitHub — https://github.com/Nos0kkk" << std::endl;
  std::cout << "Telegram — https://t.me/BioNos0k" << std::endl;
  return 0;
}

int main(int argc, char* argv[]) { 
  if (std::string(argv[1]) != "status" && std::string(argv[1]) != "mods" && std::string(argv[1]) != "create" && std::string(argv[1]) != "list" && std::string(argv[1]) != "--help") {
    std::cout << "mojo: '" << std::string(argv[1]) << "' not found" << std::endl;
    std::cout << "mojo: 'mojo --help' more information" << std::endl;
    return 0;
  }
  if (std::string(argv[1]) == "mods") {
    mods(argc, argv);
    return 1;
  } else if (std::string(argv[1]) == "status") {
    status();
  } else if (std::string(argv[1]) == "create") {
    downinstances(argc, argv);
  } else if (std::string(argv[1]) == "list") {
    versionlist();
  } else if (std::string(argv[1]) == "--help") {
    help();
  }
  
  return 0;
}