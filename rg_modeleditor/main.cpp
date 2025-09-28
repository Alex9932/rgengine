#define GAME_DLL
#include <rgentrypoint.h>
#include <event.h>
#include <rgstring.h>
#include <filedialog.h>
#include <filesystem.h>
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui.h>

#include "renderer.h"
#include "vertexbuffer.h"
#include <camera.h>
#include <lookatcameracontroller.h>

#include "geom_importer.h"
#include <pm2exporter.h>
#include <pm2importer.h>

#define ALBEDO_TEXTURE 0
#define NORMAL_TEXTURE 1
#define PBR_TEXTURE    2

using namespace Engine;

static char MDL_NAME[512];
static char MDL_PATH[512];
static char MDL_EXT[32];

static Camera* camera = NULL;
static LookatCameraController* camcontrol = NULL;
static RenderState* rstate = NULL;

static R3DStaticModelInfo info = {};
static ModelExtraData model_extra;
static Bool isModelLoaded = false;

static Bool skipFirstMat  = false;

static PM2Importer pm2_import;
static PM2Exporter pm2_export;

static void RecalculateCameraProjection() {
	ivec2 newsize;
	GetRenderSize(rstate, &newsize);
	camera->SetAspect((Float32)newsize.x / (Float32)newsize.y);
	camera->ReaclculateProjection();
}

static Bool CEventHandler(SDL_Event* event) {
	ImGui_ImplSDL3_ProcessEvent(event);

	//rgLogInfo(RG_LOG_RENDER, "Type: %d %d", event->type, event->window.event);
	if (event->type  == SDL_EVENT_WINDOW_RESIZED) {
		ResizeRender(rstate);
		RecalculateCameraProjection();
	}

	if (event->type == SDL_EVENT_DROP_FILE) {
		rgLogInfo(RG_LOG_SYSTEM, "Drag'n'drop event: %d", event->type);
	}

	return true;
}

static void LoadModel() {

	// TODO: Rewrite this

	char file[512];
	SDL_snprintf(file, 512, "%s.%s", MDL_NAME, MDL_EXT);


	if (rg_streql(MDL_EXT, "pm2")) {


		char fullpath[512];
		SDL_snprintf(fullpath, 512, "%s/%s", MDL_PATH, file);

		pm2_import.ImportModel(fullpath, &info);
		model_extra.mat_names = (NameField*)rg_malloc(sizeof(NameField) * info.matCount);
		for (Uint32 i = 0; i < info.matCount; i++) {
			SDL_snprintf(model_extra.mat_names[i].name, 128, "%s", info.matInfo[i].texture);
		}
	}
	else {
		// Use custom loaders
		ImportStaticModelInfo importinfo = {};
		importinfo.path = MDL_PATH;
		importinfo.file = file;
		importinfo.info = &info;
		importinfo.extra = &model_extra;
		importinfo.skipFirstMat = skipFirstMat;
		ImportStaticModel(&importinfo);
	}

	// Copy textures to "tmpdata"

	//obj_importer.ImportModel("gamedata/models/untitled2.obj", &info);
	//obj_importer.ImportModel(file, &info);
	//rgLogInfo(RG_LOG_RENDER, "Loaded model: %d %d %d %d", info.vCount, info.iCount, info.mCount, info.iType);
	MakeVBuffer(&info, MDL_PATH);
	isModelLoaded = true;

}

static void OpenModel() {
	// Open filedialog
	char raw_path[512] = {};
	char path[512] = {};
	FD_Filter filters[6] = {
		{"Wavefront model", "obj"},
		{"COLLADA dae", "dae"},
		{"FBX model", "fbx"},
		{"PM2 Model file", "pm2"},
		{"MMD Polygon model", "pmd"},
		{"MMD eXtended polygon model", "pmx"}
	};
	if (ShowOpenDialog(raw_path, 512, filters, 6)) {
		FS_ReplaceSeparators(path, raw_path);
		rgLogInfo(RG_LOG_SYSTEM, ":> %s", path);

		// Parse path
		// Save %MODELNAME% in MDL_NAME and %PATH% in MDL_PATH
		// Path example: /%PATH%/%MODELNAME%.%EXT%
		// /home/alex9932/rgengine/rawmodels/cube/cube.obj
		// - %PATH% -> /home/alex9932/rgengine/rawmodels/cube
		// - %MODELNAME% -> cube
		// - %EXT% -> obj

		// /blah-blah-blah/cube.obj
		//             sep^ ext^
		Sint32 sep = rg_strcharate(path, '/'); // separator
		Sint32 ext = rg_strcharate(path, '.'); // extension
		Uint32 len = SDL_strlen(path);

		if ((len - ext) - 1 >= 32) {
			rgLogError(RG_LOG_SYSTEM, "Invalid filename!");
			return;
		}

		// Clear buffers
		SDL_memset(MDL_PATH, 0, 512);
		SDL_memset(MDL_NAME, 0, 512);
		SDL_memset(MDL_EXT, 0, 32);

		// Copy data
		SDL_memcpy(MDL_PATH, path, sep); // path w/o filename
		SDL_memcpy(MDL_NAME, &path[sep + 1], len - sep - (len - ext) - 1); // only filename
		SDL_memcpy(MDL_EXT, &path[ext + 1], len - ext - 1); // only extension

		rgLogInfo(RG_LOG_SYSTEM, "-> %s", MDL_PATH);
		rgLogInfo(RG_LOG_SYSTEM, "-> %s", MDL_NAME);
		rgLogInfo(RG_LOG_SYSTEM, "-> %s", MDL_EXT);

		LoadModel();
	}
}

static void CopyTexture(String dst, String src) {
#if 0
	FILE* fsrc = fopen(src, "rb");
	if (!fsrc) { rgLogError(RG_LOG_SYSTEM, "Source texture %s open failure!", src); return; }

	FILE* fdst = fopen(src, "wb");
	if (!fsrc) { rgLogError(RG_LOG_SYSTEM, "Dest texture %s open failure!", src); return; }

	char buffer[1024]; // 1Kb
	while (feof(fsrc) != 0) {
		size_t readed = fread(buffer, 1, 1024, fsrc);
		fwrite(buffer, 1, readed, fdst);

	}

	fclose(fdst);
	fclose(fsrc);
#endif

	FSReader fsrc(src);
	FSWriter fdst(dst);

	rgLogInfo(RG_LOG_RENDER, "Copy texture %s to %s", src, dst);

	char buffer[1024]; // 1Kb
	while (!fsrc.EndOfStream()) {
		size_t readed = fsrc.Read(buffer, 1024);
		fdst.Write(buffer, readed);
	}

}

static void SaveModel() {
	// Copy textures (if needed, we can use exist texture)
	// gamedata/textures/%material_name%.png
	// gamedata/textures/%material_name%_norm.png
	// gamedata/textures/%material_name%_pbr.png
	//
	// Save geometry data
	// gamedata/models/%MODELNAME%.pm2

	if (!isModelLoaded) return;

	char path[256];
	String gamedata_path = GetGamedataPath();
	VertexBuffer* buffer = GetVertexbuffer();

	// Copy data
	R3DStaticModelInfo mdlinfo = info;

	// Make copy of materials array (need for replace full texture path to texture name in gamedata)
	mdlinfo.matInfo = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo) * mdlinfo.matCount);
	SDL_memcpy(mdlinfo.matInfo, info.matInfo, sizeof(R3D_MaterialInfo) * mdlinfo.matCount);

	for (Uint32 i = 0; i < info.matCount; i++) {
		// Copy texture
		Texture* tx = buffer->textures[i * 3 + ALBEDO_TEXTURE];

		SDL_snprintf(path, 256, "%s/textures/%s.png", gamedata_path, model_extra.mat_names[i].name);
		CopyTexture(path, tx->tex_name);

		// Replace path to name
		SDL_snprintf(mdlinfo.matInfo[i].texture, 128, "%s", model_extra.mat_names[i].name);
	}

	mat4 mdl_matrix;
	CalculateModelMatrix(rstate, &mdl_matrix);
	SDL_snprintf(path, 256, "%s/models/%s.pm2", gamedata_path, MDL_NAME);


	pm2_export.ExportModel(path, &mdlinfo, &mdl_matrix);

	// Free array copy
	rg_free(mdlinfo.matInfo);
}

static void ReplaceTexture(Uint32 matid, Uint32 txidx) {
	// Open filedialog
	char raw_path[512] = {};
	char path[512] = {};
	FD_Filter filters[1] = {
		{"PNG image", "png"}
	};
	if (ShowOpenDialog(raw_path, 512, filters, 1)) {
		VertexBuffer* vb = GetVertexbuffer();

		FS_ReplaceSeparators(path, raw_path);
		rgLogInfo(RG_LOG_SYSTEM, ":> %s", path);
		Texture* tx = GetTexture(path); // New texture
		FreeTexture(vb->textures[matid * 3 + txidx]); // Free previous texture
		vb->textures[matid * 3 + txidx] = tx; // Replace albedo texture
		rgLogInfo(RG_LOG_RENDER, "Replaced material %d with texture %s", matid, tx->tex_name);
	}
}

static void DrawGUI() {
	ImGui::Begin("Model importer");

	VertexBuffer* buffer = GetVertexbuffer(); // Loaded model

	Uint32 uid = 0;

	if (ImGui::BeginTabBar("##tabs")) {
		if (ImGui::BeginTabItem("Model")) {

			if (ImGui::Button("Load")) {
				OpenModel();
			}
			ImGui::SameLine();

			if (ImGui::Button("Save")) {
				SaveModel();
			}
			ImGui::SameLine();
#if 0
			if (ImGui::Button("Test model")) {
				//obj_importer.ImportModel("gamedata/models/untitled2.obj", &info);
				obj_importer.ImportModel("gamedata/models/doublesided_cape.obj", &info);
				//rgLogInfo(RG_LOG_RENDER, "Loaded model: %d %d %d %d", info.vCount, info.iCount, info.mCount, info.iType);
				MakeVBuffer(&info);
				isModelLoaded = true;
			}
			ImGui::SameLine();
#endif
			if (ImGui::Button("Free")) {
				FreeVBuffer(GetVertexbuffer());
				FreeStaticModel(&info, &model_extra);
				//obj_importer.FreeModelData(&info);
				isModelLoaded = false;
			}

			ImGui::Checkbox("Skip first material", &skipFirstMat);
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Use for skip first \"Default Material\" in some models");
				ImGui::EndTooltip();
			}

			ImGui::Separator();

			ImGui::Text("Loaded model: %s", MDL_NAME);
			ImGui::Text("Path: %s", MDL_PATH);

			ImGui::Text("Vertices: %d", info.vCount);
			ImGui::Text("Indices: %d", info.iCount);
			ImGui::Text("IDX size: %d", info.iType);

			ImGui::Checkbox("Wireframe", GetRenderWireframe(rstate));

			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

				vec3* size = GetRenderMdlsizePtr(rstate);
				vec3* rot = GetRenderMdlrotPtr(rstate);
				ImGui::InputFloat3("Scale##input3", size->array);
				ImGui::SliderFloat3("Scale##slider3", size->array, 0, 10);
				ImGui::SliderAngle("Rotation X##slider", &rot->x);
				ImGui::SliderAngle("Rotation Y##slider", &rot->y);
				ImGui::SliderAngle("Rotation Z##slider", &rot->z);

				if (ImGui::Button("Reset Transform")) {
					rot->x = 0; rot->y = 0; rot->z = 0;
					size->x = 1; size->y = 1; size->z = 1;
				}
			}

			ImGui::EndTabItem();
		}

		if (!isModelLoaded) { ImGui::BeginDisabled(); }

		if (ImGui::BeginTabItem("Mesh")) {

			for (Uint32 i = 0; i < buffer->meshes; i++) {
				ImGui::PushID(uid);
				if (ImGui::TreeNode("##xx", "[%d] %s", i, model_extra.mesh_names[i].name)) {
					ImGui::Text("Index: %d (%d)", buffer->pairs[i].start, buffer->pairs[i].count);
					Uint32 midx = buffer->mat[i];
					ImGui::Text("Material: %d (%s)", midx, model_extra.mat_names[midx].name);
					ImGui::Checkbox("Flip UV", &buffer->pairs[i].flipuv);
					ImGui::TreePop();
				}
				ImGui::PopID();
				uid++;
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Materials")) {

			for (Uint32 i = 0; i < buffer->tcount; i++) {
				ImGui::PushID(uid);
				if (ImGui::TreeNode("##xx", "[%d] %s", i, model_extra.mat_names[i].name)) {

					Texture* tx;

					if (ImGui::BeginTable("MaterialTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::Text("Name");
						ImGui::TableSetColumnIndex(1); ImGui::InputText("##matName", model_extra.mat_names[i].name, 128);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::Text("Color");
						ImGui::TableSetColumnIndex(1); ImGui::ColorEdit3("##matColor", buffer->colors[i].array);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						tx = buffer->textures[i * 3 + ALBEDO_TEXTURE];
						ImGui::Text("Albedo");
						ImGui::PushID(i * 3 + ALBEDO_TEXTURE); // Same texture id
						if (ImGui::Button("Replace texture")) {
							ReplaceTexture(i, ALBEDO_TEXTURE);
						}
						ImGui::PopID();
						ImGui::TableSetColumnIndex(1);
						ImGui::Image((ImTextureID)tx->tex_id, ImVec2(32, 32));
						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::Text("Path: %s", tx->tex_name);
							ImGui::Image((ImTextureID)tx->tex_id, ImVec2(256, 256));
							ImGui::EndTooltip();
						}

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						tx = buffer->textures[i * 3 + NORMAL_TEXTURE];
						ImGui::Text("Normal map");
						ImGui::PushID(i * 3 + NORMAL_TEXTURE); // Same texture id
						if (ImGui::Button("Replace texture")) {
							ReplaceTexture(i, NORMAL_TEXTURE);
						}
						ImGui::PopID();
						ImGui::TableSetColumnIndex(1);
						ImGui::Image((ImTextureID)tx->tex_id, ImVec2(32, 32));
						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::Text("Path: %s", tx->tex_name);
							ImGui::Image((ImTextureID)tx->tex_id, ImVec2(256, 256));
							ImGui::EndTooltip();
						}

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						tx = buffer->textures[i * 3 + PBR_TEXTURE];
						ImGui::Text("PBR");
						ImGui::PushID(i * 3 + PBR_TEXTURE); // Same texture id
						if (ImGui::Button("Replace texture")) {
							ReplaceTexture(i, PBR_TEXTURE);
						}
						ImGui::PopID();
						ImGui::TableSetColumnIndex(1);
						ImGui::Image((ImTextureID)tx->tex_id, ImVec2(32, 32));
						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::Text("Path: %s", tx->tex_name);
							ImGui::Image((ImTextureID)tx->tex_id, ImVec2(256, 256));
							ImGui::EndTooltip();
						}

						ImGui::EndTable();
					}

					ImGui::TreePop();
				}
				ImGui::PopID();
				uid++;
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Skeleton")) {

			ImGui::EndTabItem();
		}

		if (!isModelLoaded) { ImGui::EndDisabled(); }

		ImGui::BeginDisabled();
		if (ImGui::BeginTabItem("Animations")) {

			ImGui::EndTabItem();
		}
		ImGui::EndDisabled();


		ImGui::EndTabBar();
	}

	ImGui::End();
}

class Application : public BaseGame {
public:
	Application() {
		this->isClient = true;
		this->isGraphics = false;
	}
	~Application() {}

	String GetName() { return "Model importer"; }

	void MainUpdate() {
		camcontrol->Update();
		camera->Update(GetDeltaTime());

		DoRender(rstate, camera);
	}

	void Initialize() {
		World* world = GetWorld();

		camera = new Camera(world, 0.1f, 1000, rgToRadians(75), 1.777f);
		//camera->GetTransform()->SetPosition({ 0.0, 1.0, 0.0 });
		//camera->GetTransform()->SetRotation({ 0.0, 0.0, 0.0 });
		camcontrol = new LookatCameraController(camera);
		camcontrol->SetLength(3.14f);
		//camcontrol->SetLookAtPosition();

		rstate = InitializeRenderer(DrawGUI);
		RegisterEventHandler(CEventHandler);
		RecalculateCameraProjection();
	}

	void Quit() {
		delete camcontrol;
		delete camera;
		DestroyRenderer(rstate);
		FreeEventHandler(CEventHandler);
	}

private:
};

static Application* app;

void Module_Initialize() {
	app = new Application();
}

void Module_Destroy() {
	delete app;
}

BaseGame* Module_GetApplication() {
	return app;
}




#if 0


//#include <zlib.h>
//#include <allocator.h>
//ProcessSFile("E:/.../TRAINS/TRAINSET/rz_VL10-1487/rz_vl10-1487b.s");
//ProcessSFile("E:/.../TRAINS/TRAINSET/tsrLoco_VL8-1718/vl8-1718a.s");
static void ProcessSFile(String file) {
	rgLogInfo(RG_LOG_GAME, "Open: %s", file);
	Resource* res = Engine::GetResource(file);

	if (!res) {
		rgLogError(RG_LOG_GAME, "File not found!");
		return;
	}

	Uint8* h = (Uint8*)res->data; // data pointer

	size_t buff_len = 1024 * 1024 * 16;
	void* out = rg_malloc(buff_len);

	rgLogInfo(RG_LOG_GAME, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x", h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7], h[8], h[9], h[10], h[11], h[12], h[13], h[14], h[15]);
	rgLogInfo(RG_LOG_GAME, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x", h[16], h[17], h[18], h[19], h[20], h[21], h[22], h[23], h[24], h[25], h[26], h[27], h[28], h[29], h[30], h[31]);

	if (h[16] == 0x78 && (h[17] == 0x9C || h[17] == 0xDA)) {
		rgLogInfo(RG_LOG_GAME, "Decompressing...");

		z_stream zstr = {};

		zstr.next_in = (Bytef*)&h[16];
		zstr.avail_in = res->length - 16;
		zstr.next_out = (Bytef*)out;
		zstr.avail_out = buff_len;

		if (inflateInit(&zstr) != Z_OK) {
			rgLogInfo(RG_LOG_GAME, "Zlib initialize error!");
			goto ret;
		}

		int z_result = 0;
		if ((z_result = inflate(&zstr, Z_FINISH)) != Z_STREAM_END) {
			rgLogInfo(RG_LOG_GAME, "Zlib decompress error (%d)!", z_result);
			goto ret;
		}

		inflateEnd(&zstr);


		rgLogInfo(RG_LOG_GAME, "Decompressed: %d", zstr.total_out);

		rgLogInfo(RG_LOG_GAME, "Write readed data");
		FSWriter writer("decompressed.txt");
		writer.Write(out, zstr.total_out);

	}

ret:

	rgLogInfo(RG_LOG_GAME, "Close!");
	rg_free(out);
	Engine::FreeResource(res);
	return;

}
#endif