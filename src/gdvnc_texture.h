#ifndef GDNVNC_TEXTURE_H
#define GDNVNC_TEXTURE_H

#include <Godot.hpp>
#include <GlobalConstants.hpp>
#include <PoolArrays.hpp>
#include <Image.hpp>
#include <ImageTexture.hpp>
#include <rfb/rfbclient.h>

namespace godot {

class GDVNC_Texture : public godot::GodotScript<ImageTexture> {
	GODOT_CLASS(GDVNC_Texture)

private:
	rfbClient *rfbCl;
	int width;
	int height;
	int bits_per_pixel;
	char password[128]; // assume a password will be less then 128 characters. May make this a pointer some day
	bool is_connected;
	bool framebuffer_allocated;
	bool framebuffer_is_dirty;

	float time_passed_since_update;
	float target_fps;

public:
	static void _register_methods();
	static rfbBool gdvnc_rfb_resize(rfbClient *p_client);
	static void gdvnc_rfb_update(rfbClient *p_client, int p_x, int p_y, int p_w, int p_h);
	static void gdvnc_rfb_got_cut_text(rfbClient *p_client, const char *p_text, int p_textlen);
	static void gdvnc_rfb_kbd_leds(rfbClient *p_client, int p_value, int p_pad);
	static void gdvnc_rfb_text_chat(rfbClient *p_client, int p_value, char *p_text);
	static char *gdvnc_rfb_get_password(rfbClient *p_client);

	GDVNC_Texture();
	~GDVNC_Texture();

	void set_target_fps(float p_fps);
	float get_target_fps();

	bool connect(String p_host, String p_password);
	void disconnect();
	Vector2 get_screen_size();
	bool update(float p_delta);
	bool updateMouseState(int p_x, int p_y, int p_mask);
	bool updateKeyState(int p_scancode, bool p_is_down);
};

}

#endif /* !GDNVNC_TEXTURE_H */
