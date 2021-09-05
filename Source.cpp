#include <cmath>
#include <filesystem>
#include <thread>
#include <Shlobj.h>

#include "replay.h"

namespace fs = std::filesystem;

uint16_t build_v = 6114;
bool docs_found = false;
bool maps_copied = false;
const string maps_location{"Maps/1.32.10.17380"};

void copy_maps(const fs::path dir_from, const fs::path to)
{
    PWSTR   ppszPath;    // variable to receive the path memory block pointer.
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &ppszPath);
    std::wstring documents;
    if (SUCCEEDED(hr)) 
    {
        documents = ppszPath;      // make a local copy of the path
    }
    CoTaskMemFree(ppszPath);    // free up the path memory block
    if (!documents.length())
        return;
    
    docs_found = true;
    auto dir_to = fs::path{documents + to.wstring()};
    if (fs::exists(dir_to))
        if (!fs::is_empty(dir_to))
            return;
    
    fs::copy(dir_from, dir_to);
    maps_copied = true;
}

void repack_replays(const string dir, int& amount_success, int& amount)
{
    if (!fs::exists("Converted"))
        fs::create_directory("Converted");
    for (auto& p : fs::recursive_directory_iterator(dir + "Replays"))
        if (p.is_regular_file())
        {
            auto name = p.path();
            amount++;
            if (!repack_replay(name.string().c_str(), (dir + "Converted/converted_" + name.filename().string()).c_str(), build_v, amount_success, maps_location))
                cout << name.filename() << ".. \nSkipped.\n\n";
        }
}

int main()
{
    // Init
    auto amount = 0, amount_success = 0;
    fs::path dir_to("/Warcraft III/" + maps_location);
    fs::path dir_from("don't touch this one!/1.32.10.17380");
    string dir_main("");

    thread t1(copy_maps, dir_from, dir_to);
    thread t2(repack_replays, dir_main, std::ref(amount_success), std::ref(amount));

    t1.join();
    t2.join();

    if (maps_copied)
        cout << "Maps were successfully copied to \"Documents" << dir_to.string() << "\"\n\n";
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    cout << "Conversion is done. " << amount_success << " replays of " << amount << " were successfully re-packed.\n"
        << "Build " << (int)build_v << endl;
    
    if (!docs_found)
        cout << "\n ***ATTENTION!*** The maps were not successfully copied to your documents directory\n"
            << " In order to make the replays work properly, please copy the maps\n From folder: \""
            << dir_from.string() << "/\"\n To your: \"Documents/Warcraft III/Maps/"
            << dir_to.filename().string() << "/\" folder\n";

    cout << "\npress enter to finish...";
    cin.get();
}
