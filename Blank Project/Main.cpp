#include "../nclgl/window.h"
#include "Renderer.h"

int main() {
	Window w("Coursework!", 2560, 1440, true);
	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w);
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		renderer.RenderScene();
		renderer.SwapBuffers();

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) {
			renderer.ToggleScene();
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2)) {
			renderer.ToggleSplitScreen();
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_3)) {
			renderer.ToggleManualCamera();
		}
	}

	return 0;
}

/*Art Credits:
* 
* Skybox - Emil Persson, aka Humus (https://opengameart.org/content/field-skyboxes)
* Grassland Texture - Liberated Pixel Cup (https://lpc.opengameart.org/content/seamless-grass-texture-ii)
* Grassland Bumpmap - Filter Forge (https://www.filterforge.com/filters/4739-bump.html)
* Water Texture - Newcastle University
* Lava Texture - Miloslav Ciz (https://gitlab.com/drummyfish/minetest-texturepack/-/blob/master/Hand%20Painted%20Pack%20by%20Drummyfish/default_lava.png?ref_type=heads)
* Mountainous Skybox - GameBanana (https://gamebanana.com/mods/download/8348#FileInfo_39158)
* Rock Texture - qubodup (https://opengameart.org/node/8032)
* 
*/