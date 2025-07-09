#include<guiLayout.h>
#include<unordered_set>
static char path[MAX_PATH] = { 0 };
static float progress = 0.0f;
static bool analyzed = false;
static bool maximized = false;
static volumeSystem_t* img = NULL;
bool showtreeview = true;
bool showbrowse = true;
ImGuiIO* io = nullptr;
static bool showDone = false;
static bool showCheckBoxes = false;
using InodeKey = std::pair<const fileSystem_t*, const fileNode_t*>;
struct InodeKeyHash {
	size_t operator()(const InodeKey& key) const noexcept {
		size_t h1 = std::hash<const fileSystem_t*>{}(key.first);
		size_t h2 = std::hash<const fileNode_t*>{}(key.second);
		return h1 ^ (h2 << 1); 
	}
};

static std::unordered_set<InodeKey,InodeKeyHash> selected_nodes;
void browsePage(char* file, disk_t* physicalDrives, uint16_t len) {
	ImGui::Begin("browsePage", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::PushFont(io->Fonts->Fonts[2]);
	ImGui::PushFont(io->Fonts->Fonts[3]);
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Choose a drive to analyse or browse for disk images").x / 2);
	ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.15f);
	ImGui::Text("Choose a drive to analyse or browse for disk images");
	ImVec2 windowDim = ImGui::GetWindowSize();
	ImGui::SetNextWindowPos({ windowDim.x * 0.25f,windowDim.y * 0.40f });
	ImGui::SetCursorPos({ windowDim.x * 0.25f,windowDim.y * 0.30f });
	ImGui::SetNextItemWidth(windowDim.x * 0.42f);
	char button[7] = "Browse";
	ImGui::PopFont();
	ImGui::InputText("##file", file, MAX_PATH - 1, ImGuiInputTextFlags_ElideLeft);
	ImGui::SameLine();
	if (file[0] != 0) { strcpy(button, "Done"); }
	ImGui::PushFont(io->Fonts->Fonts[4]);
	if (ImGui::Button(button)) {
		if (file[0] == 0) {
			gui::ShowFileOpenDialog(gui::window, file, MAX_PATH - 1);
		}
		else {
			if (!pathexists(file)) {
				MessageBoxA(gui::window, "given file does not exist", "ERROR", MB_OK | MB_ICONWARNING);
				memset(file, 0, MAX_PATH);
			}
			else {
				ImGui::PopFont();
				ImGui::PopFont();
				ImGui::End();
				showbrowse = false;
				return;
			}
		}

	}
	ImGui::PopFont();
	ImGui::BeginChild("physicaldrives", { windowDim.x * 0.5f,windowDim.y * 0.5f }, 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
	for (uint16_t i = 0; i < len; i++) {
		ImGui::PushFont(io->Fonts->Fonts[1]);
		ImGui::Text(u8"\uf0a0");
		ImGui::PopFont();
		ImGui::SameLine();
		if (ImGui::Selectable(physicalDrives[i].model)) {
			memset(file, 0, MAX_PATH);
			std::snprintf(file, MAX_PATH, "\\\\.\\PhysicalDrive%d", physicalDrives[i].drivenumber);
		}
	}
	ImGui::EndChild();
	ImGui::PopFont();
	ImGui::End();
}
static void recoverStrip() {

	float fullWidth = ImGui::GetContentRegionAvail().x;
	ImGuiStyle& style = ImGui::GetStyle();

	float totalButtonsWidth = ImGui::CalcTextSize("Change Path").x +
		ImGui::CalcTextSize("Start Recovery").x +
		style.FramePadding.x * 4 + style.ItemSpacing.x;

	if (showDone) {
		ImGui::Text("Recovery Successful");
		totalButtonsWidth -= ImGui::CalcTextSize("Change Path").x +
			ImGui::CalcTextSize("Start Recovery").x -
			ImGui::CalcTextSize("Done").x + style.FramePadding.x * 2;
	}
	else {

		ImGui::Text("Recover to: %s", path);
	}
	ImGui::SameLine(fullWidth - totalButtonsWidth);

	if (!showDone) {
		if (ImGui::Button("Change Path")) {
			gui::BrowseFolder(path, MAX_PATH);
		}
	ImGui::SameLine();
	}


	if (path[0] != 0 && (!showDone)) {
		if (ImGui::Button("Start Recovery")) {
			TSK_TCHAR recoveryPath[MAX_PATH];
			Utf8ToUtf16(path, recoveryPath, MAX_PATH);

			for (auto it = selected_nodes.begin(); it != selected_nodes.end(); ) {
				if (it->second->isDir) {
					for (size_t i = 0; i < it->second->len; i++) {
						selected_nodes.erase(InodeKey(it->first, it->second->children[i]));
					}
					it = selected_nodes.erase(it);
				}
				else {
					++it;
				}
			}

			for (auto& node : selected_nodes) {
				recover(node.second->inum, node.first, recoveryPath);
			}

			showDone = true;
			selected_nodes.clear();
		}
	}
	else if (showDone) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.75, 0.0, 0.5));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0, 0.75, 0.0, 0.25));
		if (ImGui::Button("Done")) {
			showDone = false;
		}
		ImGui::PopStyleColor(2);
	}
	else {
		ImGui::NewLine();
	}

	ImGui::Separator();
}
static inline void selectedFile(const fileNode_t* node, const fileSystem_t* fs) {
	selected_nodes.insert(InodeKey(fs, node));
	if (!node->parent) return;
	if (selected_nodes.find(InodeKey(fs,node->parent)) == selected_nodes.end()) {
		bool allSelected = true;
		for (size_t i = 0; i < node->parent->len; i++) {
			if (selected_nodes.find(InodeKey(fs,node->parent->children[i])) == selected_nodes.end()) {
				allSelected = false;
				break;
			}
		}
		if (allSelected) {
			selected_nodes.insert(InodeKey(fs,node->parent));
		}
	}
}
static inline void deselectFile(const fileNode* node,const fileSystem_t* fs) {
	selected_nodes.erase(InodeKey(fs, node));
	if (!node->parent) return;
	selected_nodes.erase(InodeKey(fs, node->parent));
}
static void selectFolder(const fileNode_t* node,const fileSystem_t* fs) {
	selected_nodes.insert(InodeKey(fs, node));
	for (size_t i = 0; i < node->len; i++) {
		if (node->children[i]->isDir) {
			selectFolder(node->children[i],fs);
		}
		else {
			selected_nodes.insert(InodeKey(fs,node->children[i]));
		}
	}
	if (!node->parent) {
		return;
	}
	bool allSelected = true;
	for (size_t i = 0; i < node->parent->len;i++) {
		if (selected_nodes.find(InodeKey(fs, node->parent->children[i])) == selected_nodes.end()) {
			allSelected = false;
			break;
		}
	}
	if (allSelected) {
		selected_nodes.insert(InodeKey(fs,node->parent));
	}
}
static void deselectFolder(const fileNode_t* node,const fileSystem_t* fs) {
	selected_nodes.erase(InodeKey(fs, node));
	for (size_t i = 0; i < node->len; i++) {
		if (node->children[i]->isDir) {
			deselectFolder(node->children[i],fs);
		}
		else {
			selected_nodes.erase(InodeKey(fs,node->children[i]));
		}
	}
}

static void renderFileSys(const fileNode_t* node, const fileSystem_t* fs) {

	if (node->deleted) {
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
		ImGui::PushFont(io->Fonts->Fonts[7]);
		float iconWidth = node->isDir ? ImGui::CalcTextSize(u8"\uf07b").x : ImGui::CalcTextSize(u8"\uf15b").x;
		if (showCheckBoxes) {
			iconWidth += ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x-1.0f;
		}
		ImGui::PopFont();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		pos.x += 2 * iconWidth;
		float width = ImGui::GetContentRegionMax().x;
		float line_height = ImGui::GetTextLineHeight();
		if (showCheckBoxes) {
			width -= ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x;
		}
		ImGui::GetWindowDrawList()->AddRectFilled(
			pos,
			ImVec2(width, pos.y + line_height),
			IM_COL32(255, 0, 0, 105)
		);

	}

		char Name[1024];
		if (node->recycledPath!=NULL) {
			std::snprintf(Name, 1024, "%hs(%hs)(%llu)", node->name,node->recycledPath, node->inum);
		}
		else {
			std::snprintf(Name, 1024, "%hs(%llu)", node->name, node->inum);
		}
	if (node->isDir) {
		if (showCheckBoxes)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f,0.0f });
			ImGui::PushID(node->inum);
			bool selected_dir = (selected_nodes.find(InodeKey(fs,node)) != selected_nodes.end());
			if (ImGui::Checkbox("##checkbox", &selected_dir))
			{
				if (selected_dir) {
					selectFolder(node,fs);
				}
				else {
					deselectFolder(node,fs);
				}
			}
			ImGui::PopID();
			ImGui::PopStyleVar();
			ImGui::SameLine();
		}
		ImGui::PushFont(io->Fonts->Fonts[7]);
		ImGui::Text(u8"\uf07b");
		ImGui::PopFont();
		ImGui::SameLine();
		bool opened = ImGui::TreeNodeEx(Name, ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			showCheckBoxes ^= 1;
			selected_nodes.clear();
			memset(path, 0, MAX_PATH);
		}
		if (opened) {
			for (int i = 0; i < node->len; i++) {
				ImGui::PushID(i);
				renderFileSys(node->children[i], fs);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}
	else {
		if (showCheckBoxes)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f,0.0f });
			ImGui::PushID(node->inum);
			bool selected = (selected_nodes.find(InodeKey(fs,node)) != selected_nodes.end());
			if (ImGui::Checkbox("##checkbox_leaf", &selected))
			{
				if (selected) {
					selectedFile(node,fs);
				}
				else {
					deselectFile(node,fs);
				}
			}
			ImGui::PopID();
			ImGui::PopStyleVar();
			ImGui::SameLine();
		}
		ImGui::PushFont(io->Fonts->Fonts[7]);
		ImGui::Text(u8"\uf15b");
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::TreeNodeEx(Name, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			showCheckBoxes ^= 1;
			selected_nodes.clear();
			memset(path, 0, MAX_PATH);
		}
		if (node->deleted) {
			ImGui::SameLine();
			char percent[50];
			std::snprintf(percent, 50, "%d%%",  node->deleted -1);
			float textWidth = ImGui::CalcTextSize(percent).x;
			float windowWidth = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + windowWidth - textWidth - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(percent);
		}

		ImGui::TreePop();


	}
	if (node->deleted) {
		ImGui::PopStyleColor(2);
	}
}
static void renderDiskImg() {
	ImGui::PushFont(io->Fonts->Fonts[5]);
	if (showCheckBoxes)
	{
		recoverStrip();
	}
	ImVec2 scrollRegion = ImGui::GetContentRegionAvail();
	ImGui::BeginChild("ScrollRegion", scrollRegion, false, ImGuiWindowFlags_HorizontalScrollbar);
	TSK_VS_PART_INFO* part = img->vs->part_list;
	for (int i = 0; i < img->size; i++) {
		ImGui::PushID(i);
		char des[512];
		if (img->validFs[i]) {
			if (ImGui::TreeNodeEx(part->desc, ImGuiTreeNodeFlags_SpanFullWidth)) {
				std::snprintf(des, 512, "File System Detected: %hs (%LfMB or %llu bytes)", tsk_fs_type_toname(img->fileSystems[i]->fs->ftype), (part->len * img->vs->img_info->sector_size) / (1024.0 * 1024.0), part->len * img->vs->img_info->sector_size);
				ImGui::Text(des);
				renderFileSys(img->fileSystems[i]->root, img->fileSystems[i]);
				ImGui::TreePop();
			}
		}
		else {
			std::snprintf(des, 256, "%hs (%LfMB or %llu bytes)", part->desc, (part->len * img->vs->img_info->sector_size) / (1024.0 * 1024.0), part->len * img->vs->img_info->sector_size);
			if (ImGui::TreeNodeEx(des, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth)) {
				ImGui::TreePop();
			}
		}
		ImGui::PopID();

		part = part->next;
	}
	ImGui::EndChild();
	ImGui::PopFont();
}
void Treeviewpage(const char* file) {
	if (!analyzed) {
		if (strncmp(file, "\\\\.\\PhysicalDrive", 17) == 0) {
			if (!IsRunningAsAdmin()) {
				RelaunchAsAdmin(file);
				exit(-1);
			}
		}
		std::thread a(analyseImg, file, TSK_IMG_TYPE_DETECT, 0, &img);
		a.detach();
		analyzed = true;
	}
	if (!maximized) {
		gui::changeWindowStyle(gui::window, WS_OVERLAPPEDWINDOW, WS_POPUPWINDOW | WS_BORDER, SW_MAXIMIZE);
		maximized = true;
	}

	ImGui::SetNextWindowSize({ (float)gui::width,(float)gui::height });
	ImGui::SetNextWindowPos({ 0.0f,0.0f });
	ImGui::PushFont(io->Fonts->Fonts[4]);
	ImGui::Begin("Image Overview", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopFont();
	if (done) {
		renderDiskImg();
	}
	else {
		if (total_file_sys == 0) { ImGui::ProgressBar(0.0f); }
		else {
			float temp = (float)((float)file_sys_analyzed / (float)total_file_sys);
			ImGui::SetCursorPosX(3.0f * ((float)gui::width) / 8.0f);
			ImGui::SetCursorPosY(((float)gui::height - 24.0f) * 0.5f);
			ImGui::ProgressBar(temp, { (float)(gui::width) / 4.0f,24.0f });
		}
	}
	ImGui::End();

}
void LoadFonts() {
	io->IniFilename = nullptr;
	io->Fonts->AddFontDefault();
	ImFontConfig font_cfg;
	font_cfg.PixelSnapH = true;
	static const ImWchar icons_ranges[] = { 0x20, 0xffff, 0 };
	io->Fonts->AddFontFromFileTTF("..\\..\\..\\gui\\fonts\\font_awesome\\Font Awesome 6 Free-Regular-400.otf", 32.0f, &font_cfg, icons_ranges);
	io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Seguisb.ttf", 16.0f, nullptr, icons_ranges);
	io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Segoeuib.ttf", 24.0f, nullptr, icons_ranges);
	io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Segoeuib.ttf", 18.0f, nullptr, icons_ranges);
	io->Fonts->AddFontFromFileTTF("..\\..\\..\\gui\\fonts\\jetbrains_mono\\JetBrainsMonoNL-SemiBold.ttf", 20.0f, nullptr, icons_ranges);
	io->Fonts->AddFontFromFileTTF("..\\..\\..\\gui\\fonts\\jetbrains_mono\\JetBrainsMono-Regular.ttf", 17.0f, nullptr, icons_ranges);
	io->Fonts->AddFontFromFileTTF("..\\..\\..\\gui\\fonts\\font_awesome\\Font Awesome 6 Free-Regular-400.otf", 17.0f, &font_cfg, icons_ranges);
}
void freeImage() {
	freeVolumeSystem(img);
	img = NULL;
	done = false;
}