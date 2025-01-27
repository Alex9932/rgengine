/*
 * rgEngine engine.h
 *
 *  Created on: Feb 9, 2022
 *      Author: alex9932
 * 
 *

 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2018-2025 Alex9932

 THIRDPATY SOFTWERE
 Used libraries:
  * Mujs by Artifex.                          <https://artifex.com/>
  * SDL2 by Simple DirectMedia Layer project. <https://www.libsdl.org/credits.php>
  * Dear ImGui.                               <https://github.com/ocornut/imgui/tree/docking>


 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 */

#ifndef _ENGINE_H
#define _ENGINE_H

#include "rgtypes.h"

#define RG_BUILD         385
#define RG_VERSION_MAJ   0
#define RG_VERSION_MIN   9
#define RG_VERSION_PATCH 0

#define RG_ERROR				Engine::HandleError("");
#define RG_ERROR_MSG(msg)       Engine::HandleError(msg);
#define RG_ASSERT               SDL_assert_always
#define RG_ASSERT_MSG(c, msg)   if(c == 0) rgLogError(RG_LOG_ASSERT, "Internal error! %s", msg); \
								if(c == 0) Engine::SetAssertMessage(msg);                        \
								SDL_assert_always(c)

class RG_DECLSPEC BaseGame {
    protected:
        Bool isClient = false;
        Bool isGraphics = false;

    public:
        BaseGame() {}
        virtual ~BaseGame() {}
        virtual void MainUpdate() {}
        virtual void Initialize() {}
        virtual void Quit() {}
        virtual String GetName()    { return "unnamed";  }
        RG_INLINE Bool IsClient()   { return isClient;   }
        RG_INLINE Bool IsGraphics() { return isGraphics; }
};

namespace Engine {

    class World;
    class SoundSystem;
    class ModelSystem;
    class LightSystem;
    class Profiler;
    class RGPhysics;

    RG_DECLSPEC void SetAssertMessage(String str);

    // DL Library
    RG_DECLSPEC LibraryHandle DL_LoadLibrary(String name);
    RG_DECLSPEC ProcHandle    DL_GetProcAddress(LibraryHandle handle, String proc_name);
    RG_DECLSPEC void          DL_UnloadLibrary(LibraryHandle handle);

    // Arguments
    RG_DECLSPEC Sint32 ProcessArguments(int argc, String* argv);
    RG_DECLSPEC Bool   IsArgument(String arg);

    RG_DECLSPEC void Initialize();
    RG_DECLSPEC void Start();
    RG_DECLSPEC void RequestShutdown();
    RG_DECLSPEC void Quit();
    RG_DECLSPEC void ForceQuit();
    RG_DECLSPEC void HandleError(String message);

    RG_DECLSPEC Bool IsDebug();
    RG_DECLSPEC Bool IsRunning();

    RG_DECLSPEC Float64   GetDeltaTime();
    RG_DECLSPEC Float64   GetUptime();
    RG_DECLSPEC Uint32    GetThreads();
    RG_DECLSPEC BaseGame* GetGame();
    RG_DECLSPEC String    GetEngineVersion();
    RG_DECLSPEC String    GetEnginePlatform();

    RG_DECLSPEC World* GetWorld();

    RG_DECLSPEC SoundSystem* GetSoundSystem();
    RG_DECLSPEC ModelSystem* GetModelSystem();
    RG_DECLSPEC LightSystem* GetLightSystem();

    RG_DECLSPEC String    GetProfile(Uint32 idx);
    RG_DECLSPEC Profiler* GetProfiler();

    RG_DECLSPEC RGPhysics* GetPhysics();

}

#endif