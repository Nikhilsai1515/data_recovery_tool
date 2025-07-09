#pragma once
#include"gui.h"
#include"recover.h"
#include"analyse.h"
#include<thread>

void browsePage(char* file, disk_t* physicalDrives, uint16_t len);
void Treeviewpage(const char* file);
void LoadFonts();
extern ImGuiIO* io;
extern bool showbrowse;
extern bool showtreeview;
