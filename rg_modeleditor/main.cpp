#define GAME_DLL
#include <rgentrypoint.h>

#include "renderer.h"

//#include <zlib.h>

//#include <filesystem.h>
//#include <allocator.h>

using namespace Engine;
#if 0
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

	rgLogInfo(RG_LOG_GAME, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x", h[0],  h[1],  h[2],  h[3],  h[4],  h[5],  h[6],  h[7],  h[8],  h[9],  h[10], h[11], h[12], h[13], h[14], h[15]);
	rgLogInfo(RG_LOG_GAME, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x", h[16], h[17], h[18], h[19], h[20], h[21], h[22], h[23], h[24], h[25], h[26], h[27], h[28], h[29], h[30], h[31]);

	if (h[16] == 0x78 && (h[17] == 0x9C || h[17] == 0xDA)) {
		rgLogInfo(RG_LOG_GAME, "Decompressing...");

		z_stream zstr = {};

		zstr.next_in   = (Bytef*)&h[16];
		zstr.avail_in  = res->length - 16;
		zstr.next_out  = (Bytef*)out;
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

class Application : public BaseGame {
	public:
		Application() {
			this->isClient   = true;
			this->isGraphics = false;
		}
		~Application() {}

		String GetName() { return "Model importer"; }

		void MainUpdate() {
			DoRender(rstate);
		}

		void Initialize() {
			rstate = InitializeRenderer();

			//ProcessSFile("E:/.../TRAINS/TRAINSET/rz_VL10-1487/rz_vl10-1487b.s");
			//ProcessSFile("E:/.../TRAINS/TRAINSET/tsrLoco_VL8-1718/vl8-1718a.s");
		

		}

		void Quit() {
			DestroyRenderer(rstate);
		}

	private:

		RenderState* rstate = NULL;

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
