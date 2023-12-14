#ifndef _UICOMPONENT_H
#define _UICOMPONENT_H

#include <rgvector.h>
#include <imgui/imgui.h>

class UIComponent {
	public:
		UIComponent(String name, ImU32 flags = 0);
		~UIComponent();

		void DrawComponent();

		virtual void Draw() {}
		virtual void OnReposition(ivec2 newpos);
		virtual void OnResize(ivec2 newsize);

	protected:
		ivec2 m_wndsize = {};
		ivec2 m_wndpos  = {};
		char  m_wndname[64];
		ImU32 m_flags   = 0;

};

#endif