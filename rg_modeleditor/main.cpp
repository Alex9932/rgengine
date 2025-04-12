#define GAME_DLL
#include <rgentrypoint.h>
#include <event.h>
#include <rgstring.h>
#include <filedialog.h>
#include <filesystem.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui.h>

#include "renderer.h"
#include "vertexbuffer.h"
#include <camera.h>
#include <lookatcameracontroller.h>


#include <objimporter.h>

using namespace Engine;

static ObjImporter obj_importer;

static char MDL_NAME[512];
static char MDL_PATH[512];
static char MDL_EXT[32];

static Camera* camera = NULL;
static LookatCameraController* camcontrol = NULL;
static RenderState* rstate = NULL;

static R3DStaticModelInfo info = {};
static Bool isModelLoaded = false;

static void RecalculateCameraProjection() {
	ivec2 newsize;
	GetRenderSize(rstate, &newsize);
	camera->SetAspect((Float32)newsize.x / (Float32)newsize.y);
	camera->ReaclculateProjection();
}

static Bool CEventHandler(SDL_Event* event) {
	ImGui_ImplSDL2_ProcessEvent(event);

	//rgLogInfo(RG_LOG_RENDER, "Type: %d %d", event->type, event->window.event);
	if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED) {
		ResizeRender(rstate);
		RecalculateCameraProjection();
	}

	return true;
}

static void LoadModel() {

	// TODO: Rewrite this

	char file[512];
	SDL_snprintf(file, 512, "%s/%s.%s", MDL_PATH, MDL_NAME, MDL_EXT);

	//obj_importer.ImportModel("gamedata/models/untitled2.obj", &info);
	obj_importer.ImportModel(file, &info);
	//rgLogInfo(RG_LOG_RENDER, "Loaded model: %d %d %d %d", info.vCount, info.iCount, info.mCount, info.iType);
	MakeVBuffer(&info);
	isModelLoaded = true;

}

static void OpenModel() {
	// Open filedialog
	char raw_path[512] = {};
	char path[512] = {};
	FD_Filter filters[4] = {
		{"Wavefront model", "obj"},
		{"PM2 Model file", "pm2"},
		{"MMD Polygon model", "pmd"},
		{"MMD Extended polygon model", "pmx"}
	};
	if (ShowOpenDialog(raw_path, 512, filters, 4)) {
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
		SDL_memcpy(MDL_NAME, &path[sep + 1], len - sep - (len - ext) - 1); // filename w/o extension
		SDL_memcpy(MDL_EXT, &path[ext + 1], len - ext - 1); // filename w/o extension

		rgLogInfo(RG_LOG_SYSTEM, "-> %s", MDL_PATH);
		rgLogInfo(RG_LOG_SYSTEM, "-> %s", MDL_NAME);
		rgLogInfo(RG_LOG_SYSTEM, "-> %s", MDL_EXT);

		LoadModel();
	}
}

static void SaveModel() {
	// Save geometry data
	// gamedata/models/%MODELNAME%.pm2
	// 
	// Save textures (if needed, we can use exist texture)
	// gamedata/textures/%TEXTURE%.png
}

static void DrawGUI() {
	ImGui::Begin("Model");

	if (ImGui::Button("Load")) {
		OpenModel();
	}
	ImGui::SameLine();
	if (ImGui::Button("Save")) {
		SaveModel();
	}
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
		obj_importer.FreeModelData(&info);
		isModelLoaded = false;
	}
	ImGui::Text("Loaded model: %s", MDL_NAME);
	ImGui::Text("Path: %s", MDL_PATH);

	ImGui::Checkbox("Wireframe", GetRenderWireframe(rstate));

	if (isModelLoaded) {

		ImGui::Text("Vertices: %d", info.vCount);
		ImGui::Text("Indices: %d", info.iCount);
		ImGui::Text("IDX size: %d", info.iType);

		Uint32 uid = 0;
		// Meshes
		if (ImGui::TreeNode("Meshes")) {
			
			for (Uint32 i = 0; i < info.mCount; i++) {
				ImGui::PushID(uid);
				if (ImGui::TreeNode("##xx", "Mesh [%d]", i)) {
					R3D_MatMeshInfo* mesh = &info.mInfo[i];
					ImGui::Text("Index: %d (%d)", mesh->indexOffset, mesh->indexCount);
					ImGui::Text("Material: %d", mesh->materialIdx);
					ImGui::TreePop();
				}
				ImGui::PopID();
				uid++;
			}

			ImGui::TreePop();
		}

		// Materials
		if (ImGui::TreeNode("Materials")) {

			for (Uint32 i = 0; i < info.matCount; i++) {
				ImGui::PushID(uid);
				if (ImGui::TreeNode("##xx", "Material [%d]", i)) {
					R3D_MaterialInfo* mat = &info.matInfo[i];
					ImGui::InputText("Texture", mat->texture, 128);
					ImGui::ColorEdit3("Color", mat->color.array);
					ImGui::TreePop();
				}
				ImGui::PopID();
				uid++;
			}

			ImGui::TreePop();
		}

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