#include<analyse.h>
#include<recover.h>
#include<guiLayout.h>
int main(int argc, char* argv[]) {
	gui::CreateAppwindow(L"main", L"app");
	io = gui::createImgui();
	LoadFonts();
	char  file[MAX_PATH] = { 0 };
	if (argc == 2) {
		showbrowse = false;
		strcpy(file, argv[1]);
	}
	uint16_t size;
	disk_t* physicaldrives = ListPhysicalDrives(size);
	while (!gui::quit) {
		if (!gui::BeginRender()) {
			continue;
		}
		ImGui::SetNextWindowPos({ 0,0 });
		ImGui::SetNextWindowSize({ (float)gui::width,(float)gui::height });
		if (showbrowse) {
			browsePage(file, physicaldrives, size);
		}
		else if (showtreeview)
		{
			Treeviewpage(file);
		}

		gui::endRender();
	}
	free(physicaldrives);
	gui::DestroyImgui();
	gui::DestroyAppwindow();
	return 0;
}